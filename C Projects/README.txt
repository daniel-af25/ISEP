Projeto realizado no âmbito da unidade curricular de APROG.
Licenciatura em Engenharia Eletrotécnica de Computadores - 2024/2025
Daniel Afonso 2025

Sistema de Tickets de Atendimento - Centro de Saúde

Descrição do Projeto

Este projeto consiste no desenvolvimento de um programa em linguagem C para simular o funcionamento de um sistema de tickets de atendimento ao público num centro de saúde.

Funcionalidades Principais

Gerar tickets para:

-Consulta Marcada: Inclui número sequencial, data e hora do pedido. Quando atendido, registra informações do atendimento.

	-Consulta de Urgência: Inclui número sequencial, data e hora do pedido. Quando atendido, registra dados da triagem.

	-Listagem de tickets ordenados por data e hora.

	-Atualização de tickets no momento do atendimento.

	-Geração de relatórios, incluindo:

	-Volume de tickets atendidos por intervalo de datas.

	-Média de tempo de espera entre atendimentos.

	-Balcões mais e menos produtivos.

	-Receita de consultas marcadas.

Regras e Restrições

	-A numeração dos tickets é renovada diariamente.

	-Existem três balcões, sendo que o balcão 3 não atende consultas marcadas.

	-O valor das consultas marcadas deve ser zero ou superior.

	-Existe um limite de tickets para consultas de urgência por dia.

	-O horário dos pedidos deve respeitar o do sistema.

Tecnologias Utilizadas

-Linguagem C

Compilar:

-Compilar o código: gcc sistema_tickets.c -o sistema_tickets

-Executar o programa: ./sistema_tickets