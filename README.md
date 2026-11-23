# Portefólio Académico — Engenharia Eletrotécnica e de Computadores (ISEP)

<p align="left">
  <img src="https://img.shields.io/badge/ISEP-LEEC-004B87?style=flat-square" alt="ISEP LEEC" />
  <img src="https://img.shields.io/badge/C-Embedded%20%26%20Systems-A8B9CC?style=flat-square&logo=c&logoColor=white" alt="C" />
  <img src="https://img.shields.io/badge/AVR-ATmega128%20Assembly-red?style=flat-square" alt="AVR Assembly" />
  <img src="https://img.shields.io/badge/Python-Django-092E20?style=flat-square&logo=django&logoColor=white" alt="Django" />
  <img src="https://img.shields.io/badge/PHP-MySQL-777BB4?style=flat-square&logo=php&logoColor=white" alt="PHP" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="License MIT" />
</p>

Bem-vindo ao meu repositório central de projetos desenvolvidos ao longo da licenciatura em **Engenharia Eletrotécnica e de Computadores (LEEC)** no **Instituto Superior de Engenharia do Porto (ISEP)** — **Daniel Afonso**.

Este portefólio reúne trabalhos práticos de hardware analógico, programação *bare-metal* de microcontroladores, aplicações em C e plataformas web *full-stack*.

---

## Visão Geral dos Projetos

| Área | Projeto | Tecnologias | Documentação |
| :--- | :--- | :--- | :--- |
| **Sistemas Embebidos** | [Máquina de Estados & Automação (ASM)](./Embedded_Systems/Automation_StateMachine_ASM) | Assembly AVR, ATmega128, Timers/CTC | [README](./Embedded_Systems/README.md) · [Relatório PDF](./Embedded_Systems/Automation_StateMachine_ASM/Projeto1_README.pdf) |
| **Sistemas Embebidos** | [Controlo de Motores PWM & Telemetria](./Embedded_Systems/Motor_Control_PWM_C) | C, Assembly AVR, PWM, ADC, USART, ICP1 | [README](./Embedded_Systems/README.md) · [Relatório PDF](./Embedded_Systems/Motor_Control_PWM_C/Projeto2_README.pdf) |
| **Sistemas Embebidos** | [Telemetria Automóvel em Tempo Real (LABSIS)](./Embedded_Systems/Automotive_Telemetry_LABSIS) | C (ATmega328P), ESP8266, MQTT, Node-RED, SQLite | [README](./Embedded_Systems/Automotive_Telemetry_LABSIS/README.md) · [Especificação PDF](./Embedded_Systems/Automotive_Telemetry_LABSIS/docs/Especificacao_Telemetria_Automovel.pdf) |
| **Eletrónica Analógica** | [Sistema de Controlo Térmico Analógico](./Analog_Electronics/Thermal_Control_System) | Op-Amps, PSpice, Arduino Mega 2560, LabVIEW | [README](./Analog_Electronics/Thermal_Control_System/README.md) · [Parte 1 PDF](./Analog_Electronics/Thermal_Control_System/1_Signal_Conditioning_and_DAQ/relatorio_parte1.pdf) · [Parte 2 PDF](./Analog_Electronics/Thermal_Control_System/2_Control_and_Actuation/relatorio_parte2.pdf) |
| **Aplicações em C** | [Gestão de Fluxos Clínicos (Ticketing)](./C_Applications) | C, POSIX `<time.h>`, CLI | [README](./C_Applications/README.md) · [Código-fonte](./C_Applications/clinical_ticketing.c) |
| **Aplicações Web** | [Beer Shop (E-Commerce)](./Web_Applications/beer_shop) | Python, Django, SQLite, HTML/CSS | [README](./Web_Applications/beer_shop/README.md) |
| **Aplicações Web** | [Gestão de Stock e Encomendas](./Web_Applications/stock_management) | PHP, MySQL/MariaDB, JavaScript, HTML/CSS | [README](./Web_Applications/stock_management/README.md) |

---

## Estrutura do Repositório

```text
ISEP/
├── Analog_Electronics/
│   └── Thermal_Control_System/        # Condicionamento NTC, PSpice, DAQ Arduino + LabVIEW e atuação linear/PWM
├── C_Applications/                    # Simulador de atendimento e triagem clínica em C
├── Embedded_Systems/                  # Firmware AVR (ATmega128 / ATmega328P) e IoT (ESP8266)
│   ├── Automation_StateMachine_ASM/   # Func1–Func5 em Assembly AVR
│   ├── Automotive_Telemetry_LABSIS/   # Telemetria automóvel 5 Hz (ATmega328P, ESP8266, Node-RED, SQLite)
│   └── Motor_Control_PWM_C/           # Func1–Func5 em C + ADC em Assembly
└── Web_Applications/                  # Aplicações web e bases de dados
    ├── beer_shop/                     # E-commerce modular em Django (MVT)
    └── stock_management/              # Gestão de inventário e encomendas em PHP + MySQL
```

---

## Detalhes por Domínio

### 1. [Sistemas Embebidos](./Embedded_Systems)
Programação *bare-metal* para **ATmega128** (MICMIC) e **ATmega328P + ESP8266** (LABSIS):
* **[Automation_StateMachine_ASM](./Embedded_Systems/Automation_StateMachine_ASM):** Automação industrial em **Assembly AVR** (linha de perfuração, *debouncing*, multiplexagem de displays de 7 segmentos e jogos temporizados por Timer0/CTC).
* **[Motor_Control_PWM_C](./Embedded_Systems/Motor_Control_PWM_C):** Controlo de motores DC e de passo em **C** + rotina **ADC em Assembly** (Timer2 PWM, comunicação série USART/RS232 e telemetria RPM via *Input Capture* ICP1).
* **[Automotive_Telemetry_LABSIS](./Embedded_Systems/Automotive_Telemetry_LABSIS):** Sistema de telemetria automóvel a 5 Hz em **C *bare-metal* (ATmega328P)** + ponte **ESP8266/MQTT** com painel OLED, dashboard **Node-RED**, base de dados **SQLite** (catálogo de 25 374 veículos), alertas de acidente e debrief automático em PDF *(com simulador 2D/3D adjacente em [telemetry-sim](https://github.com/daniel-af25/telemetry-sim))*.

### 2. [Eletrónica Analógica](./Analog_Electronics/Thermal_Control_System)
Sistema completo de instrumentação e controlo térmico (unidade curricular de Eletrónica Aplicada — ELEAPL), validado em **PSpice** e implementado em hardware:
* **[1_Signal_Conditioning_and_DAQ](./Analog_Electronics/Thermal_Control_System/1_Signal_Conditioning_and_DAQ):** Termístor NTC em Ponte de Wheatstone, Amplificador de Instrumentação, Limitador de Precisão (0–5 V), leitura ADC em Arduino Mega 2560 e interface **LabVIEW** com *data logging*.
* **[2_Control_and_Actuation](./Analog_Electronics/Thermal_Control_System/2_Control_and_Actuation):** Atuação linear de corrente por transístor BJT e modulação PWM analógica (oscilador de onda triangular de 3 andares + *Schmitt Trigger*).

### 3. [Aplicações em C](./C_Applications)
* **[clinical_ticketing.c](./C_Applications/clinical_ticketing.c):** Motor de atendimento e triagem hospitalar em **C** (desenvolvido em APROG), com alocação dinâmica de balcões/salas/especialidades, cálculo de tempos em tempo real via `<time.h>` e relatórios estatísticos e financeiros.

### 4. [Aplicações Web](./Web_Applications)
* **[beer_shop](./Web_Applications/beer_shop):** Loja online em **Django** (arquitetura MVT) com catálogo, promoções, carrinho em sessão, *checkout*, notificações e geração de guias de remessa em PDF.
* **[stock_management](./Web_Applications/stock_management):** Plataforma em **PHP + MySQL** (desenvolvida em DEAPC) com autenticação por níveis (Admin/Utilizador), gestão de clientes, produtos, encomendas e faturação.

---

## Stack Tecnológico

* **Hardware & Embebidos:** ATmega128 · ATmega328P · ESP8266 · Arduino Mega 2560 · PWM · ADC · I2C · Timers/Counters · USART/RS232 · MQTT · Circuitos Analógicos (AMPOPs, BJT)
* **Linguagens:** C · C++ · Assembly (AVR) · Python · PHP · JavaScript · HTML5/CSS3 · SQL
* **Ferramentas & Frameworks:** Node-RED · Mosquitto · Django · PSpice for TI · LabVIEW · Atmel/Microchip Studio · MySQL/MariaDB · SQLite · Git

---

## Licença e Contacto

* Distribuído sob a licença [MIT](./LICENSE).
* **Autor:** Daniel Afonso · **Email:** danielkyyr@gmail.com
