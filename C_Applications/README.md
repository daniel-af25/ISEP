# Sistema de Gestão de Tickets de Centro de Saúde

Aplicação interativa em consola desenvolvida em **linguagem C** para simular a gestão de atendimento e triagem de pacientes num centro de saúde. Projeto realizado na unidade curricular **APROG (Algoritmia e Programação)** da LEEC no ISEP.

## Funcionalidades Principais

* **Gestão de Fluxos de Pacientes:** Criação e encaminhamento de tickets para consultas marcadas e urgências.
* **Alocação Dinâmica de Recursos:** Atribuição de balcões de atendimento, especialidades médicas, profissionais de saúde, gabinetes e salas de triagem.
* **Módulos Analíticos (Reporting):**
  * Volume de pacientes atendidos por intervalo de tempo.
  * Cálculo de tempos médios de espera através de `<time.h>`.
  * Produtividade individual por balcão de atendimento.
  * Relatório financeiro de receitas geradas por período.

## Dinâmica do Sistema

Através de um menu interativo em terminal, o sistema acompanha todo o ciclo de vida de cada ticket (emissão, encaminhamento e conclusão), registando horas exatas com recurso ao relógio do sistema e validando a disponibilidade de salas e recursos.

## Tecnologias Utilizadas

* **Linguagem:** C (`stdio.h`, `stdlib.h`, `string.h`, `time.h`, `stdbool.h`)
* **Ficheiro principal:** [`clinical_ticketing.c`](./clinical_ticketing.c)

## Como Compilar e Executar

```bash
gcc clinical_ticketing.c -o clinical_ticketing
./clinical_ticketing
```

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
