# 🎓 Portefólio de Engenharia - Daniel Afonso

Bem-vindo ao meu repositório central. Sou estudante de **Engenharia Eletrotécnica e de Computadores (LEEC)** no Instituto Superior de Engenharia do Porto (ISEP).

Este espaço documenta a minha evolução técnica e a capacidade de interligar múltiplas disciplinas da engenharia. O meu foco estende-se desde o desenho de circuitos analógicos e programação *bare-metal* de sistemas embebidos, até ao desenvolvimento de arquiteturas de *software* de alto nível e plataformas web completas.

---

## 📂 Arquitetura do Repositório e Projetos

Para facilitar a navegação, os meus projetos estão organizados por domínios de engenharia e estruturados em subpastas modulares. Clica nas hiperligações abaixo para explorar o código-fonte e a documentação detalhada de cada sistema:

### ⚙️ 1. [Embedded_Systems](./Embedded_Systems)
**Desenvolvimento de Firmware e Controlo de Automação (ATmega128)**  
Programação de baixo nível para a arquitetura AVR, dividida em dois módulos principais:
* **[Automation_StateMachine_ASM](./Embedded_Systems/Automation_StateMachine_ASM):** Lógica de automação industrial implementada em **Assembly** (máquina de estados para linha de perfuração, *debouncing*, controlo de LEDs e displays de 7 segmentos, e jogos temporizados via Timer0/CTC).
* **[Motor_Control_PWM_C](./Embedded_Systems/Motor_Control_PWM_C):** Controlo de motores DC e de passo em **C** e Assembly (gestão de PWM via Timer2, telemetria de velocidade via *Input Capture* - ICP1, conversão ADC otimizada e interface de controlo remoto via USART/RS232).

### 🔌 2. [Analog_Electronics](./Analog_Electronics/Thermal_Control_System)
**Sistema de Instrumentação e Controlo Analógico Térmico**  
Desenvolvimento integral de hardware analógico de ponta a ponta, validado por simulações PSpice e implementado fisicamente. O projeto divide-se em duas etapas em cascata:
* **[1_Signal_Conditioning_and_DAQ](./Analog_Electronics/Thermal_Control_System/1_Signal_Conditioning_and_DAQ):** Aquisição com termístor NTC, Ponte de Wheatstone, Amplificador de Instrumentação (alto CMRR), Limitador de Precisão (0-5V) e conversão A/D num Arduino Mega 2560 com telemetria e *logging* em tempo real via **LabVIEW**.
* **[2_Control_and_Actuation](./Analog_Electronics/Thermal_Control_System/2_Control_and_Actuation):** Malhas de atuação física sobre a carga, incluindo controlo linear de corrente por BJT e modulação PWM analógica gerada por um oscilador de onda triangular de 3 andares acoplado a um comparador *Schmitt Trigger*.

### 💻 3. [C_Applications](./C_Applications)
**Sistema de Gestão de Fluxos Clínicos (Ticketing)**  
Desenvolvimento de *software* estruturado em C (`clinical_ticketing.c`) para simular o motor de atendimento e triagem de um centro de saúde, contemplando alocação dinâmica de recursos, gestão de tempos por POSIX `<time.h>` e relatórios analíticos de produtividade e faturação.

### 🌐 4. [Web_Applications](./Web_Applications)
**Desenvolvimento de Plataformas Web e E-Commerce**  
Agregador de aplicações web modulares com separação clara de responsabilidades:
* **[beer_shop](./Web_Applications/beer_shop):** Plataforma de e-commerce desenvolvida em Python (Django) com arquitetura MVT, gestão de catálogo, cesto de compras, *checkout*, sessões e geração de guias de remessa em PDF.
* **[stock_management](./Web_Applications/stock_management):** Sistema de gestão de stock e encomendas desenvolvido em PHP e JavaScript, com controlo de acessos por níveis de utilizador, painéis administrativos e envio automatizado de faturas por email.

---

## 🛠️ Stack Tecnológico Global

* **Hardware & Sistemas Embebidos:** ATmega128 | Arduino Mega 2560 | PWM | ADC | Timers/Counters | USART/RS232 | Desenho de Circuitos Analógicos
* **Linguagens de Programação:** C | Assembly (AVR) | Python | PHP | JavaScript | HTML5/CSS3 | SQL
* **Ferramentas e Software:** PSpice | LabVIEW | Linux Mint | Git | Django

---

## 📬 Contacto
* **Email:** danielkyyr@gmail.com