# Controlador Analógico de Temperatura e Atuação

Sistema integral de instrumentação e controlo analógico térmico desenvolvido na unidade curricular de **Eletrónica Aplicada (ELEAPL)** da LEEC no ISEP. O projeto cobre desde a aquisição do sinal físico até à atuação em hardware, simulação em PSpice e monitorização em LabVIEW.

## Objetivo

Projetar e implementar uma cadeia de condicionamento de sinal para um termístor NTC (excursão térmica de 17 °C a 37 °C mapeada em 0 V a 5 V) e utilizar essa tensão para controlar a luminosidade de atuadores LED por dois métodos físicos distintos: **controlo linear por corrente** e **modulação PWM analógica**.

---

## Arquitetura do Sistema

### 1. [1_Signal_Conditioning_and_DAQ](./1_Signal_Conditioning_and_DAQ)
Aquisição, proteção analógica e telemetria digital ([Relatório Parte 1 (PDF)](./1_Signal_Conditioning_and_DAQ/relatorio_parte1.pdf)):
* **Ponte de Wheatstone:** Conversão da variação resistiva não-linear do termístor NTC numa tensão diferencial.
* **Amplificador de Instrumentação:** Amplificação diferencial com ganho de 1.58 V/V e elevada rejeição de modo comum (CMRR).
* **Limitador de Precisão:** Proteção baseada em AMPOPs para manter a saída estritamente entre 0 V e 5 V, protegendo o ADC.
* **Firmware e Interface DAQ:**
  * [`labview_code.c`](./1_Signal_Conditioning_and_DAQ/labview_code.c) / [`putty_code.c`](./1_Signal_Conditioning_and_DAQ/putty_code.c): Leitura por média de 64 amostras no ADC (10 bits) de um **Arduino Mega 2560** e transmissão série via USART (9600 bps).
  * [`ntc_view.vi`](./1_Signal_Conditioning_and_DAQ/ntc_view.vi): Painel **LabVIEW** com monitorização temporal/XY, alarmes térmicos e *data logging* em `.xlsx`.
  * [`trabalho_NTCLE100E3_10k.xlsx`](./1_Signal_Conditioning_and_DAQ/trabalho_NTCLE100E3_10k.xlsx): Caracterização e análise experimental do sensor NTC.

### 2. [2_Control_and_Actuation](./2_Control_and_Actuation)
Malhas de atuação física sobre a carga e simulações PSpice ([Relatório Parte 2 (PDF)](./2_Control_and_Actuation/relatorio_parte2.pdf)):
* **Controlo Linear por Corrente:** Somador não-inversor para ajuste de ganho/offset seguido de amplificador de corrente com transístor NPN (2N3904).
* **Controlo por Modulação PWM Analógica:** Oscilador de onda triangular de 3 andares (onda quadrada limitada por Zener, integrador e *level shifter*) ligado a um comparador com histerese (*Schmitt Trigger*).
* **Simulação PSpice ([`simulations_pspice_2project`](./2_Control_and_Actuation/simulations_pspice_2project)):** Esquema OrCAD Capture (`2PARTE_TRABALHO.DSN`) e perfis de análise DC, transitória e Monte Carlo. *(Nota: a aquisição de dados em [`data_logging_labview.xlsx`](./2_Control_and_Actuation/data_logging_labview.xlsx) reutiliza o firmware e o `.vi` da Parte 1.)*



---

## Tecnologias e Ferramentas

* **Hardware:** AMPOPs (TL082/TL081), Transístores BJT (2N3904), Díodos Zener, Termístor NTC (10 kΩ), Arduino Mega 2560
* **Simulação:** PSpice for TI (DC Sweep, Regime Transitório, Monte Carlo)
* **Software:** C (AVR/Arduino), LabVIEW, Microsoft Excel

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
