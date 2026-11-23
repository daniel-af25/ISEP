# Sistemas Embebidos e Controlo de Automação (AVR & IoT)

Projetos de firmware *bare-metal* e sistemas embebidos desenvolvidos para microcontroladores **ATmega128** (unidade curricular **MICMIC**) e **ATmega328P + ESP8266** (unidade curricular **LABSIS**) no ISEP.

Exploram manipulação direta de registos, interrupções, temporizadores, PWM, ADC, I2C, comunicação série USART/RS232 e telemetria sem fios via MQTT.

---

## 1. [Automation_StateMachine_ASM](./Automation_StateMachine_ASM)
**Lógica de Automação e I/O em Assembly AVR** ([Relatório Projeto 1 (PDF)](./Automation_StateMachine_ASM/Projeto1_README.pdf))

| Módulo | Descrição | Código-Fonte |
| :--- | :--- | :--- |
| **Func 1** | I/O digital básico e leitura de interruptores com controlo de LEDs | [`main.asm`](./Automation_StateMachine_ASM/Func1/AssemblerApplication21/main.asm) |
| **Func 2** | Sequências temporizadas de LEDs com pausa/retoma e seleção de modo | [`main.asm`](./Automation_StateMachine_ASM/Func2/AssemblerApplication20/main.asm) |
| **Func 3** | Máquina de estados para linha de perfuração industrial (motores `M`/`MF`, sensores e displays) | [`main.asm`](./Automation_StateMachine_ASM/Func3/AssemblerApplication22/main.asm) |
| **Func 4** | Roleta numérica temporizada com *Start/Stop* e sinalização em displays de 7 segmentos | [`main.asm`](./Automation_StateMachine_ASM/Func4/AssemblerApplication23/AssemblerApplication23/main.asm) |
| **Func 5** | Jogo *Mastermind* / seleção multiplexada de 4 dígitos baseada em Timer0 (CTC, 1 ms) | [`main.asm`](./Automation_StateMachine_ASM/Func5/AssemblerApplication30/AssemblerApplication30/main.asm) |

---

## 2. [Motor_Control_PWM_C](./Motor_Control_PWM_C)
**Controlo de Motores e Comunicações em C & Assembly AVR** ([Relatório Projeto 2 (PDF)](./Motor_Control_PWM_C/Projeto2_README.pdf))

| Módulo | Descrição | Código-Fonte |
| :--- | :--- | :--- |
| **Func 1** | Controlo PWM (Timer2) de motor DC por botões (`SW1`–`SW5`), inversão segura (250 ms) e displays | [`main.c`](./Motor_Control_PWM_C/Func1/GccApplication26/GccApplication26/main.c) |
| **Func 2** | Adição de comunicação série **USART1** (RS232, 9600 bps) para controlo remoto via PC | [`main.c`](./Motor_Control_PWM_C/Func2/GccApplication27/GccApplication27/main.c) |
| **Func 3** | Controlo de motor passo a passo (*full/half-step*) + conversão **ADC** em Assembly (`adc_read.S`) | [`main.c`](./Motor_Control_PWM_C/Func3/GccApplication34/GccApplication34/main.c) · [`adc_read.S`](./Motor_Control_PWM_C/Func3/GccApplication34/GccApplication34/adc_read.S) |
| **Func 4** | Gestão bidirecional de posição (`signed char`) no motor passo a passo com controlo série/ADC | [`main.c`](./Motor_Control_PWM_C/Func4/GccApplication37/GccApplication37/main.c) · [`adc_read.S`](./Motor_Control_PWM_C/Func4/GccApplication37/GccApplication37/adc_read.S) |
| **Func 5** | Telemetria de velocidade (RPS/RPM) via *Input Capture* (**Timer1 / ICP1**) com apresentação nos displays | [`main.c`](./Motor_Control_PWM_C/Func5/GccApplication41/GccApplication41/main.c) · [`adc_read.S`](./Motor_Control_PWM_C/Func5/GccApplication41/GccApplication41/adc_read.S) |

---

## 3. [Automotive_Telemetry_LABSIS](./Automotive_Telemetry_LABSIS)
**Sistema de Telemetria Automóvel em Tempo Real — LABSIS** ([README Completo](./Automotive_Telemetry_LABSIS/README.md) · [Especificação (PDF)](./Automotive_Telemetry_LABSIS/docs/Especificacao_Telemetria_Automovel.pdf))

| Componente | Descrição | Ficheiros |
| :--- | :--- | :--- |
| **ATmega328P (C)** | Aquisição a 5 Hz (MPU6050 I2C, GPS NEO-6M UBX/UART, LM35Z + bateria ADC), 3 ISRs, `PROGMEM` e ventoinha PWM 10 bits autónoma | [`atmega328.c`](./Automotive_Telemetry_LABSIS/firmware/atmega328p/atmega328.c) |
| **ESP8266 (C++)** | Ponte UART → MQTT sem perdas a 5 Hz, WiFiManager + EEPROM, *Last Will* e painel OLED 128×64 (I2C 400 kHz) | [`esp8266.cpp`](./Automotive_Telemetry_LABSIS/firmware/esp8266/esp8266.cpp) |
| **Node-RED** | Dashboard *live* com mapa GPS, forças G, dinâmica de curva, deteção de acidentes (Email + Telegram) e debrief automático em PDF | [`flows.json`](./Automotive_Telemetry_LABSIS/nodered/flows.json) |
| **SQLite** | Esquema relacional de 5 tabelas + base de dados *seed* pronta a usar com catálogo de 25 374 veículos | [`schema.sql`](./Automotive_Telemetry_LABSIS/database/schema.sql) · [`labsis_telemetry_seed.db`](./Automotive_Telemetry_LABSIS/database/labsis_telemetry_seed.db) |

---

## Tecnologias e Periféricos

* **Microcontroladores & Módulos:** ATmega128 @ 16 MHz · ATmega328P @ 16 MHz · ESP8266 (Wi-Fi/MQTT) · OLED SSD1306 · MPU6050 · GPS NEO-6M · LM35Z
* **Linguagens:** C, C++, Assembly AVR, SQL
* **Periféricos & Protocolos:** Timers/Counters (Normal, CTC, Fast/Phase-Correct PWM), Interrupções externas & *Input Capture* (ICP1), ADC (8/10 bits), I2C (TWI), UART/USART (RS232 & UBX/NMEA), MQTT, Node-RED, SQLite

---
*Projetos desenvolvidos para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
