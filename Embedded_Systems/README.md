# Sistemas Embebidos e Controlo de Automação (ATmega128)

Firmware desenvolvido para o microcontrolador **ATmega128** (arquitetura AVR) no âmbito da unidade curricular de **Microcontroladores e Microprocessadores (MICMIC)** no ISEP.

Os projetos evoluem de forma incremental (`Func1` a `Func5`), explorando manipulação direta de registos, interrupções, temporizadores, PWM, ADC e comunicação série USART/RS232.

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

## Tecnologias e Periféricos

* **Microcontrolador:** ATmega128 @ 16 MHz (Atmel / Microchip Studio 7)
* **Linguagens:** C, Assembly AVR
* **Periféricos:** Timers/Counters (Normal, CTC, Fast/Phase-Correct PWM), Interrupções externas & *Input Capture* (ICP1), ADC (8 bits), USART1 (RS232)

---
*Projetos desenvolvidos para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
