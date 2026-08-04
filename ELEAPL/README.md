# Controlador Analógico de Temperatura e Atuação

Este repositório contém o desenvolvimento integral de um sistema de instrumentação e controlo analógico térmico, desenvolvido no âmbito da unidade curricular de Eletrónica Aplicada da LEEC no ISEP. O projeto engloba desde a aquisição do sinal físico até à atuação em hardware e monitorização via software.

## Objetivo
Desenvolver uma cadeia de condicionamento de sinal para um termístor NTC e utilizar a tensão resultante (0V a 5V, correspondente à excursão térmica de 17°C a 37°C) para controlar a luminosidade de atuadores (LEDs). A atuação foi implementada através de dois métodos físicos distintos: controlo linear por corrente e controlo por modulação PWM. Todo o sistema foi validado teoricamente, simulado em PSpice e implementado fisicamente.

## Arquitetura do Sistema (Hardware)

### 1. Condicionamento de Sinal
* **Ponte de Wheatstone:** Converte a variação de resistência não-linear do sensor NTC num diferencial de tensão.
* **Amplificador de Instrumentação:** Amplifica o sinal diferencial (com ganho de 1.58 V/V) e elimina ruído de modo comum (CMRR).
* **Limitador de Precisão:** Circuito de proteção baseado em AMPOPs que garante que a tensão de saída se mantém estritamente no intervalo de 0V a 5V, protegendo o ADC do microcontrolador contra sobretensões fora da gama térmica.

### 2. Controlo e Atuação
* **Controlo por Corrente:** Integra um circuito condicionador de sinal (somador não-inversor) para ajustar o ganho e o offset, seguido de um amplificador de corrente auxiliado por um transístor NPN (2N3904) para modular de forma linear a intensidade luminosa dos LEDs.
* **Controlo por Tensão (PWM):** Gera um sinal PWM ajustável cruzando a tensão de controlo com uma onda de referência. Implementado com recurso a um **oscilador de onda triangular de 3 andares** (gerador de onda quadrada limitada por Zener, integrador e level shifter) e a um **comparador com histerese (Schmitt Trigger)**.

## Software e Aquisição de Dados (DAQ)
* **Firmware (C/C++):** Leitura do sinal analógico pelo ADC (10 bits) de um Arduino Mega 2560 e transmissão contínua dos dados via comunicação série assíncrona (USART).
* **Interface LabVIEW:** Painel de controlo desenvolvido para receber os dados série, permitindo a monitorização em tempo real (gráficos temporais e de linearidade XY), gestão de alarmes visuais para os limites térmicos e sistema de persistência de dados (Data Logging automático em .xlsx).

## Tecnologias e Ferramentas
* **Hardware:** AMPOPs (TL082), Transístores BJT, Díodos Zener, Termístor NTC, Arduino Mega 2560.
* **Simulação:** PSpice for TI (Análise DC Sweep, Regime Transitório e Análise Monte Carlo).
* **Software & Código:** C, LabVIEW, Microsoft Excel (para análise de regressões e validação estatística).

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
