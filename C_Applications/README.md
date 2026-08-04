# Sistema de Gestão de Tickets de Centro de Saúde

Este projeto consiste num sistema interativo desenvolvido em linguagem C para simular a gestão de atendimento e triagem de pacientes num centro de saúde. 
O programa foi desenvolvido no âmbito académico da licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) no ISEP, dentro da unidade curricular APROG(Algoritmia e Programação).

## Funcionalidades Principais
* **Gestão de Fluxos de Pacientes:** Criação e encaminhamento de tickets distintos para consultas marcadas e de urgência, garantindo a organização do fluxo de utentes.
* **Alocação Dinâmica de Recursos:** Atribuição interativa de balcões de atendimento, especialidades médicas, profissionais de saúde, gabinetes e salas de triagem.
* **Módulos Analíticos (Reporting):** Geração de mapas estatísticos detalhados para apoio à gestão clínica, incluindo:
  * Volume de pacientes atendidos por intervalo de tempo.
  * Cálculo de tempos médios de espera.
  * Análise da produtividade individual por balcão de atendimento.
  * Relatório financeiro de receitas geradas pelas consultas num determinado período.

## Dinâmica do Sistema
O programa funciona através de um menu interativo que permite gerir todo o ciclo de vida de um paciente na clínica. Desde o momento em que o ticket é gerado até à sua conclusão, o sistema utiliza o relógio da máquina para registar os tempos exatos de cada operação, validando a disponibilidade física das salas e encaminhando o paciente para o recurso adequado.

## Tecnologias Utilizadas
* **Linguagem:** C
* **Recursos Técnicos:** Manipulação de tempo real (para cálculo de durações e métricas estatísticas), processamento de texto e validação de rotinas via terminal.

## Como Executar
1. Compilar o código: gcc sistema_tickets.c -o sistema_tickets (Requer compilador de C instalado no sistema operativo)
2. Executar o programa: ./sistema_tickets

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
