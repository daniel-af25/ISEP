#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================================
// 0. CREDENCIAIS E CONFIGURAÇÕES POR DEFEITO (HARDCODED - DEFAULT CREDENTIALS)
// =====================================================================
const char* DEFAULT_SSID        = "YOUR_WIFI_SSID";   
const char* DEFAULT_PASSWORD    = "YOUR_WIFI_PASSWORD";   
const char* DEFAULT_MQTT_SERVER = "192.168.1.100"; 
const uint16_t DEFAULT_MQTT_PORT = 1883;

// Tópicos MQTT
const char* topic_telemetry = "isep/labsis/telemetry_01";
const char* topic_debug     = "isep/labsis/debug_01";

// =====================================================================
// 1. CONFIGURAÇÃO DO DISPLAY OLED I2C (128x64 DUAL COLOR AMARELO/AZUL)
// =====================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oled_ok = false;
char current_gps_status = 'V';       // 'A' = Fixed, 'V' = Searching
char current_temp_str[10]  = "--.-"; // Ex: "24.5"
char current_speed_str[10] = "0.0";  // Ex: "45.2"
uint8_t current_battery    = 100;    // 25, 50, 75, 100

// =====================================================================
// 2. ESTRUTURA DE PERSISTÊNCIA (EEPROM) E BUFFERS
// =====================================================================
#define EEPROM_SIZE 64
#define EEPROM_MAGIC 0x4C414256 // Identificador de validação da EEPROM

struct Settings {
  uint32_t magic;
  char mqtt_server[40];
  uint16_t mqtt_port;
};

Settings config;
bool shouldSaveConfig = false;

WiFiClient espClient;
PubSubClient client(espClient);

// Buffer estático de 512B para leitura UART não-bloqueante (a 5 Hz / 200ms)
char rx_buffer[512];
uint16_t rx_index = 0;

uint32_t total_lines_received = 0;
uint32_t last_heartbeat = 0;
uint8_t mqtt_fail_count = 0;

const uint8_t MAX_MQTT_FAILS = 10;          // 10 falhas (~20s) abre o portal para corrigir IP
const uint32_t HEARTBEAT_INTERVAL = 10000;  // Heartbeat a cada 10s

// =====================================================================
// FUNÇÕES DE RENDERIZAÇÃO OLED E EXTRAÇÃO ULTRARRÁPIDA (ZERO-HEAP)
// =====================================================================

// Ecrã exclusivo de arranque ("Connecting")
void showBootConnecting() {
  if (!oled_ok) return;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 24);
  display.print(F("Connecting"));
  display.display();
}

// Ecrã de aviso caso caia no Captive Portal (AP Mode)
void showPortalScreen() {
  if (!oled_ok) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 4);
  display.print(F("WiFi: AP Portal Mode"));
  display.drawLine(0, 15, 128, 15, SSD1306_WHITE);
  display.setCursor(0, 24);
  display.print(F("SSID: ESP-Telemetry"));
  display.setCursor(0, 40);
  display.print(F("IP:   192.168.4.1"));
  display.display();
}

// Desenha 4 barras de sinal Wi-Fi após "WiFi:Conn" (X: 57..67, Y: 3..12)
void drawRSSIBars(int32_t rssi) {
  uint8_t bars = 0;
  if (rssi >= -60)      bars = 4;
  else if (rssi >= -70) bars = 3;
  else if (rssi >= -80) bars = 2;
  else if (rssi > -100) bars = 1;

  for (uint8_t b = 0; b < 4; b++) {
    uint8_t bar_h = (b + 1) * 2 + 2; // Alturas: 4, 6, 8, 10 px
    uint8_t x = 57 + (b * 3);        // Espaçamento de 3px por barra (largura 2px)
    uint8_t y = 13 - bar_h;          // Alinhadas pela base em Y = 13

    if (b < bars) {
      display.fillRect(x, y, 2, bar_h, SSD1306_WHITE); // Barra ativa preenchida
    } else {
      display.drawPixel(x, 12, SSD1306_WHITE);         // Ponto na base se inativa
    }
  }
}

// Desenha ícone de bateria horizontal no canto superior direito (X: 104..126, Y: 3..12)
// 1 barra <= 25%, 2 barras <= 50%, 3 barras <= 75%, 4 barras <= 100%
void drawBatteryIcon(uint8_t bat_pct) {
  // Corpo principal (20x10 px) + polo positivo à direita (2x4 px)
  display.drawRect(104, 3, 20, 10, SSD1306_WHITE);
  display.fillRect(124, 6, 2, 4, SSD1306_WHITE);

  uint8_t portions = 0;
  if (bat_pct == 0)         portions = 0;
  else if (bat_pct <= 25)   portions = 1;
  else if (bat_pct <= 50)   portions = 2;
  else if (bat_pct <= 75)   portions = 3;
  else                      portions = 4;

  // Cada segmento interno tem 3x6 px com 1 px de espaçamento (X = 106, 110, 114, 118)
  for (uint8_t i = 0; i < portions; i++) {
    display.fillRect(106 + (i * 4), 5, 3, 6, SSD1306_WHITE);
  }
}

// Ecrã principal em tempo real (5 Hz)
// Zona Amarela (Y: 0..15) -> WiFi:Conn + Barras RSSI + M:OK/E0.. + Bateria Horizontal
// Zona Azul    (Y: 16..63) -> GPS + Temp + Speed (todos no mesmo tamanho de letra)
void updateTelemetryOLED() {
  if (!oled_ok) return;
  display.clearDisplay();

  // --- 1. BARRA SUPERIOR AMARELA (0 a 15 px) ---
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 4);

  if (WiFi.status() == WL_CONNECTED) {
    display.print(F("WiFi:Conn")); // 9 chars (54px -> X: 0..53)

    // Barras de sinal logo a seguir a WiFi:Conn (X: 57..67)
    drawRSSIBars(WiFi.RSSI());

    // Indicador de Broker MQTT (X: 72..99)
    if (client.connected()) {
      display.setCursor(74, 4);
      display.print(F("M:OK"));
    } else {
      // Destaque invertido quando MQTT falha (M:E0, M:E1...)
      display.fillRect(72, 2, 27, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.setCursor(74, 4);
      display.print(F("M:E"));
      display.print(mqtt_fail_count);
      display.setTextColor(SSD1306_WHITE); // Restaura cor normal
    }
  } else {
    display.print(F("WiFi:Wait"));
  }

  // Ícone horizontal da bateria (X: 104..126)
  drawBatteryIcon(current_battery);

  // Linha horizontal no limite exato entre a zona amarela e azul
  display.drawLine(0, 15, 128, 15, SSD1306_WHITE);

  // --- 2. ZONA INFERIOR AZUL (16 a 63 px) ---
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Linha 1: Estado do GPS (Y = 22)
  display.setCursor(0, 22);
  if (current_gps_status == 'A') {
    display.print(F("FIXED GPS"));
  } else {
    display.print(F("Searching GPS"));
  }

  // Linha 2: Temperatura no mesmo tamanho de fonte (Y = 36)
  display.setCursor(0, 36);
  display.print(F("Temp : "));
  display.print(current_temp_str);
  display.print(F(" "));
  display.cp437(true);
  display.write(248); // Símbolo '°' em CP437
  display.print(F("C"));

  // Linha 3: Velocidade abaixo da temperatura (Y = 50)
  display.setCursor(0, 50);
  display.print(F("Speed: "));
  display.print(current_speed_str);
  display.print(F(" km/h"));

  display.display();
}

// Extrai "status", "temp", "speed" e "battery_life" diretamente do rx_buffer (Zero-Heap)
void parseFieldsForOLED(const char* json) {
  // 1. Procura "status":"X"
  const char* p_status = strstr(json, "\"status\":\"");
  if (p_status != nullptr) {
    current_gps_status = p_status[10];
  }

  // 2. Procura "temp":XX.X
  const char* p_temp = strstr(json, "\"temp\":");
  if (p_temp != nullptr) {
    p_temp += 7;
    uint8_t i = 0;
    while (*p_temp != '\0' && *p_temp != ',' && *p_temp != '}' && *p_temp != ' ' && i < (sizeof(current_temp_str) - 1)) {
      current_temp_str[i++] = *p_temp++;
    }
    current_temp_str[i] = '\0';
  }

  // 3. Procura "speed":"XX.X"
  const char* p_speed = strstr(json, "\"speed\":\"");
  if (p_speed != nullptr) {
    p_speed += 9; // Avança após "speed":"
    uint8_t i = 0;
    while (*p_speed != '\0' && *p_speed != '\"' && *p_speed != ',' && i < (sizeof(current_speed_str) - 1)) {
      current_speed_str[i++] = *p_speed++;
    }
    current_speed_str[i] = '\0';
  }

  // 4. Procura "battery_life":XX
  const char* p_bat = strstr(json, "\"battery_life\":");
  if (p_bat != nullptr) {
    p_bat += 15; // Avança após "battery_life":
    int val = atoi(p_bat);
    if (val >= 0 && val <= 100) {
      current_battery = (uint8_t)val;
    }
  }
}

// =====================================================================
// GESTÃO DE EEPROM E WIFI
// =====================================================================
void saveConfigCallback() {
  shouldSaveConfig = true;
}

void saveSettings() {
  config.magic = EEPROM_MAGIC;
  EEPROM.put(0, config);
  EEPROM.commit();
}

void resetToDefaultSettings() {
  config.magic = EEPROM_MAGIC;
  strncpy(config.mqtt_server, DEFAULT_MQTT_SERVER, sizeof(config.mqtt_server) - 1);
  config.mqtt_server[sizeof(config.mqtt_server) - 1] = '\0';
  config.mqtt_port = DEFAULT_MQTT_PORT;
  saveSettings();
}

void loadSettings() {
  EEPROM.begin(EEPROM_SIZE);
  Settings temp;
  EEPROM.get(0, temp);
  
  if (temp.magic == EEPROM_MAGIC) {
    config = temp;
  } else {
    // 1.ª vez após flashar: carrega os defaults para a EEPROM
    resetToDefaultSettings();
  }
}

bool waitForWiFi(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    delay(250);
  }
  return (WiFi.status() == WL_CONNECTED);
}

// =====================================================================
// 3. GESTÃO DO PORTAL WIFIMANAGER E FALLBACK DE 20 SEGUNDOS
// =====================================================================
void startWiFiManagerPortal(bool forcePortal) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true); // Garante que alterações são guardadas na Flash

  if (!forcePortal) {
    // Mostra apenas "Connecting" enquanto tenta ligar à Wi-Fi
    showBootConnecting();

    if (WiFi.SSID().length() > 0) {
      // Já tem uma Wi-Fi guardada na Flash: tenta ligar durante 20s
      WiFi.begin();
      if (waitForWiFi(20000)) {
        client.setServer(config.mqtt_server, config.mqtt_port);
        updateTelemetryOLED(); // Ligou à Wi-Fi -> substitui por Connected
        return;
      }
    } else {
      // Flash limpa (1.º arranque após flashar): grava e tenta o DEFAULT durante 20s
      WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD);
      if (waitForWiFi(20000)) {
        client.setServer(config.mqtt_server, config.mqtt_port);
        updateTelemetryOLED(); // Ligou à Wi-Fi -> substitui por Connected
        return;
      }
    }
  }

  // Se demorou > 20s ou se o Broker MQTT falhou 10x, avisa no OLED e abre o Captive Portal
  showPortalScreen();

  WiFiManager wm;
  wm.setDebugOutput(false); // Impede lixo na UART (Serial.swap)
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setConfigPortalTimeout(180);
  
  // Permite clicar em "Save" mesmo deixando os campos SSID/Pass vazios (para usar a Checkbox de Reset)
  wm.setBreakAfterConfig(true);

  char port_str[6];
  snprintf(port_str, sizeof(port_str), "%u", config.mqtt_port);

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Broker IP", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", port_str, 6);
  
  // Checkbox HTML para restaurar os Defaults do S25 Hotspot
  WiFiManagerParameter custom_reset_default(
    "reset_def", 
    "Return to Default (Default Wi-Fi / MQTT)", 
    "1", 2, 
    "type=\"checkbox\" style=\"width:auto; display:inline-block; margin-right:8px;\"", 
    WFM_LABEL_AFTER
  );
  WiFiManagerParameter custom_hint(
    "<p style=\"font-size:12px;color:#666;margin-top:2px;\">"
    "Nota: Para voltar ao default, marca a caixa acima e clica directamente em Save (podes deixar SSID e Pass em branco)."
    "</p>"
  );

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_reset_default);
  wm.addParameter(&custom_hint);

  shouldSaveConfig = false;

  // Abre o Access Point "ESP-Telemetry-Setup"
  wm.startConfigPortal("ESP-Telemetry-Setup");

  if (shouldSaveConfig) {
    // Verifica se a checkbox "Return to Default" foi selecionada
    if (strncmp(custom_reset_default.getValue(), "1", 1) == 0) {
      resetToDefaultSettings();
      WiFi.mode(WIFI_STA);
      WiFi.persistent(true);
      WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD); // Sobrepõe a Flash com o Default
      delay(500);
      ESP.restart(); // Reinicia limpo com as credenciais Default
    } else {
      // Guarda o novo IP e Porta introduzidos manualmente
      strncpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server) - 1);
      config.mqtt_server[sizeof(config.mqtt_server) - 1] = '\0';
      
      int parsed_port = atoi(custom_mqtt_port.getValue());
      config.mqtt_port = (parsed_port > 0 && parsed_port <= 65535) ? parsed_port : DEFAULT_MQTT_PORT;
      
      saveSettings();
    }
  }

  // Se ao sair do portal ainda não estiver ligado ao Wi-Fi, reinicia para tentar novamente
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  client.setServer(config.mqtt_server, config.mqtt_port);
  updateTelemetryOLED(); // Ligado com sucesso após portal -> mostra Connected

  // Limpa bytes antigos acumulados na UART durante o bloqueio do portal
  while (Serial.available()) {
    Serial.read();
  }
  rx_index = 0;
}

// =====================================================================
// 4. RECONEXÃO MQTT E LOOP PRINCIPAL (5 Hz SEM PERDAS)
// =====================================================================
void reconnect() {
  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      startWiFiManagerPortal(false);
    }

    String clientId = "ESP-Telemetry-" + String(ESP.getChipId(), HEX);
    
    // LWT (Last Will): avisa o Node-RED se o ESP desligar abruptamente
    if (client.connect(clientId.c_str(), topic_debug, 0, true, "{\"status\":\"OFFLINE\"}")) {
      mqtt_fail_count = 0;
      
      char connectMsg[128];
      snprintf(connectMsg, sizeof(connectMsg), 
               "{\"status\":\"ONLINE\",\"ip\":\"%s\",\"broker\":\"%s\"}", 
               WiFi.localIP().toString().c_str(), config.mqtt_server);
      client.publish(topic_debug, connectMsg, true);
      
      updateTelemetryOLED(); // Atualiza badge para M:OK

      // Descarta fragmentos de UART acumulados durante a reconexão
      while (Serial.available()) {
        Serial.read();
      }
      rx_index = 0;
    } else {
      mqtt_fail_count++;
      updateTelemetryOLED(); // Mostra M:E1, M:E2... invertido no topo do OLED

      if (mqtt_fail_count >= MAX_MQTT_FAILS) {
        mqtt_fail_count = 0;
        startWiFiManagerPortal(true); // Abre o portal se o broker não responder em ~20s
      } else {
        delay(2000);
      }
    }
  }
}

void setup() {
  // Aumenta o buffer RX de RAM da UART para 1024B (impede overflow a 5 Hz)
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  Serial.swap(); // UART alternativa: TX = GPIO15 (D8), RX = GPIO13 (D7)

  // Inicializa I2C nos pinos dedicados: SDA = GPIO4 (D2), SCL = GPIO5 (D1)
  Wire.begin(4, 5);
  Wire.setClock(400000); // Fast Mode I2C (400 kHz) -> refresh do OLED em ~6ms

  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    oled_ok = true;
    showBootConnecting(); // Mostra apenas "Connecting" ao ligar o ESP
  }

  // Desativa o algoritmo de Nagle (envia pacotes TCP/MQTT imediatamente sem atraso)
  espClient.setNoDelay(true);

  loadSettings();
  client.setBufferSize(512);
  
  startWiFiManagerPortal(false);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura UART não-bloqueante byte a byte
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n') {
      rx_buffer[rx_index] = '\0';

      // Remove '\r' caso exista no final
      if (rx_index > 0 && rx_buffer[rx_index - 1] == '\r') {
        rx_index--;
        rx_buffer[rx_index] = '\0';
      }

      // Valida se a linha tem chavetas de abertura e fecho JSON
      if (rx_index > 1 && rx_buffer[0] == '{' && rx_buffer[rx_index - 1] == '}') {
        total_lines_received++;
        
        // 1. Publica imediatamente no Broker MQTT
        if (!client.publish(topic_telemetry, rx_buffer)) {
          char err[64];
          snprintf(err, sizeof(err), "{\"error\":\"PUBLISH_FAILED\",\"payload_len\":%u}", rx_index);
          client.publish(topic_debug, err);
        }

        // 2. Extrai "status", "temp", "speed" e "battery_life" e atualiza o OLED (5 Hz)
        parseFieldsForOLED(rx_buffer);
        updateTelemetryOLED();
      }
      rx_index = 0;
    } 
    else if (rx_index < sizeof(rx_buffer) - 1) {
      // Ignora lixo antes do carácter '{'
      if (rx_index > 0 || c == '{') {
        rx_buffer[rx_index++] = c;
      }
    } 
    else {
      // Reset de segurança em caso de linha corrompida > 511 bytes
      rx_index = 0;
    }
  }

  // Heartbeat de diagnóstico a cada 10s
  uint32_t now = millis();
  if (now - last_heartbeat >= HEARTBEAT_INTERVAL) {
    last_heartbeat = now;
    char hb[96];
    snprintf(hb, sizeof(hb), "{\"heartbeat_s\":%lu,\"lines_rx\":%lu,\"rssi_dbm\":%d}", 
             now / 1000, total_lines_received, WiFi.RSSI());
    client.publish(topic_debug, hb);
  }
}