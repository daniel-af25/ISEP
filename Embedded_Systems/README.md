# Sistemas Embebidos e Controlo de Automação (ATmega128)

Este repositório contém o firmware desenvolvido para o microcontrolador ATmega128 (arquitetura AVR) no âmbito da cadeira de Microcontroladores e Microprocessadores (MICMIC), focado na interação direta com periféricos físicos e no controlo de potência. O repositório está dividido em duas diretórias principais que exploram desde operações de I/O a baixo nível até ao controlo de sistemas eletromecânicos através de protocolos de comunicação e processamento de sinais.

## Objetivos do Repositório
Demonstrar a implementação de lógicas de automação industrial, controlo de atuadores (motores DC e de passo) e gestão de interfaces homem-máquina (displays de 7 segmentos e botões), explorando os recursos internos do microcontrolador (Timers, ADC, USART, Interrupções) com rigorosas restrições de tempo real.

## 📂 Estrutura de Pastas e Projetos

### 📁 [1. Automation_StateMachine_ASM](./Automation_StateMachine_ASM)
**Lógica de Automação e I/O (Assembly)**  
Desenvolvido inteiramente em **Assembly**, este módulo explora a manipulação detalhada de registos, gestão de memória e interrupções de hardware.
* **Automação de Linha de Perfuração:** Desenvolvimento de uma máquina de estados para simular um sistema industrial de perfuração de placas. Inclui o controlo de uma passadeira e broca (Motores M e MF), leitura de sensores de presença (SP) e validação de botões com rotinas de *debouncing* (leituras espaçadas por 2ms).
* **Controlo de Interface (Displays & LEDs):** Mapeamento de sinais lógicos para ativação sequencial de LEDs (com temporizações de 0.5s e 1s) e controlo multiplexado de 4 displays de 7 segmentos.
* **Sistemas Temporizados:** Implementação de uma "Roleta Numérica" e de um jogo "Mastermind" totalmente funcional, baseados em bases de tempo precisas geradas pelo Timer0 no modo *Clear Timer on Compare* (CTC).

### 📁 [2. Motor_Control_PWM_C](./Motor_Control_PWM_C)
**Controlo de Motores e Comunicações (C & Assembly)**  
Focado no controlo de potência e processamento de sinais mistos, este módulo foi desenvolvido em **C**, integrando rotinas críticas de aquisição de dados em **Assembly**.
* **Modulação PWM e Controlo de Motor DC:** Utilização do Timer2 nos modos *Fast PWM* / *Phase Correct PWM* (frequência de ~500Hz) para controlo de velocidade nominal e sentido de rotação, implementando paragens temporizadas de segurança (250ms) durante a inversão de marcha.
* **Comunicação Série Assíncrona (USART):** Implementação da interface RS232 (9600 bps, 8 data bits, 2 stop bits) para controlo remoto do sistema via PC. Permite o envio de comandos de aceleração, inversão de marcha, e requisição do *duty cycle* atual.
* **Aquisição Analógico-Digital (ADC):** Conversão analógica de 8 bits baseada num potenciómetro (0V a 5V) para mapeamento direto da velocidade do motor, recorrendo a médias de leituras consecutivas calculadas em rotinas de Assembly puro.
* **Controlo de Motores de Passo:** Atuação mecânica precisa em modos *full-step* e *half-step*, suportando incremento/decremento de passos a cada 25ms, através de comandos enviados via terminal.
* **Telemetria de Velocidade (RPM):** Leitura de um *encoder* ótico através da interrupção *Input Capture* (ICP1) do Timer1, medindo a janela temporal entre pulsos para o cálculo matemático da velocidade real do motor (RPM).

## Tecnologias e Periféricos Explorados
* **Hardware:** Microcontrolador ATmega128, Motores DC, Motores de Passo, Displays de 7 segmentos, Encoders óticos, Sensores de presença.
* **Linguagens:** C, Assembly (Instruções AVR).
* **Módulos do Microcontrolador:** 
  * Timers/Counters (Modos Normal, CTC e PWM)
  * Interrupções (Externas e *Input Capture* / ICP1)
  * Conversor Analógico-Digital (ADC)
  * Módulo USART (RS232)

---
*Projetos desenvolvidos para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*