#define _XOPEN_SOURCE // Habilita o sscanf para processamento de datas.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_TICKETS_URGENCIA 10
#define MAX_TICKETS 100
#define NUM_ESPECIALIDADES 6
#define NUM_SALAS_TRIAGEM 5
#define NUM_GABINETES 8
#define NUM_BALCAO 3

// Estrutura TicketConsultaMarcada
typedef struct
{
    int id;
    char HoraGerado[20];
    char HoraAtendido[20];
    int balcao;
    char medico[50];
    char especialidade[50];
    int gabinete;
    int valorConsulta;
    bool atendido; // false = não atendido, true = atendido
} TicketConsultaMarcada;

// Estrutura TicketConsultaUrgencia
typedef struct
{
    int id;
    char HoraGerado[20];
    char HoraAtendido[20];
    int balcao;
    char salaTriagem[20];
    bool atendido; // false = não atendido, true = atendido
} TicketConsultaUrgencia;

// Variáveis globais
TicketConsultaMarcada consultasMarcadas[MAX_TICKETS];
TicketConsultaUrgencia consultasUrgentes[MAX_TICKETS_URGENCIA];
int totalMarcadas = 0, totalUrgentes = 0;
int salasOcupadas[NUM_SALAS_TRIAGEM] = {0}; // 0 = livre, 1 = ocupada

const char *especialidades[NUM_ESPECIALIDADES] = {
    "Medicina Geral", "Cardiologia", "Neurologia",
    "Ortopedia", "Dermatologia", "Oftalmologia"};

const char *medicos[NUM_ESPECIALIDADES][5] = {
    {"Dr. José Francisco", "Dr. José Mota", "Dr. Willemberg Toledo", "Dr. Daniel Soares", "Dr. Filinto Augusto"},
    {"Dr. Tomás Rocha", "Dr. Rogger Gonçalves"},
    {"Dr. João Cardoso", "Dr. João Amaral"},
    {"Dr. João Gonçalves", "Dr. Guilherme"},
    {"Dr. Diogo Lobo", "Dra. Daniela Afonso"},
    {"Dr. André Miguel", "Dr. José Jorge"}};

const char *salasTriagem[NUM_SALAS_TRIAGEM] = {"Sala 1", "Sala 2", "Sala 3", "Sala 4", "Sala 5"};

long long calcularSegundos(const char *tm_data)
{
    int horas, minutos, segundos;

    // Parcela a string para retirar as horas minutos e segundos
    if (sscanf(tm_data, "%d:%d:%d", &horas, &minutos, &segundos) != 3)
    {
        fprintf(stderr, "Formato de hora inválido: %s\n", tm_data);
        return -1; // Retorna erro caso o formato de hora seja invalido
    }
    // Calcula o total de segundos
    return (long long)horas * 3600 + minutos * 60 + segundos;
}

int horaNoIntervalo(const char *hora, const char *inicio, const char *fim)
{
    struct tm tmHora, tmInicio, tmFim;

    // Verificar entradas nulas
    if (!hora || !inicio || !fim)
    {
        fprintf(stderr, "Erro: parâmetros nulos fornecidos a horaNoIntervalo.\n");
        return 0; // Fora do intervalo por padrão
    }
    long long segundosHora = calcularSegundos(hora);
    long long segundosInicio = calcularSegundos(inicio);
    long long segundosFim = calcularSegundos(fim);

    // Adicionar verificação extra de hora para evitar intervalos inválidos
    if (segundosInicio > segundosFim)
    {
        fprintf(stderr, "Erro: o intervalo de tempo fornecido é inválido (hora de início maior que hora de fim).\n");
        return 0;
    }

    return (segundosHora >= segundosInicio && segundosHora <= segundosFim);
}

// Função para obter valor da consulta
int obterValorConsulta(int escolhaEspecialidade)
{
    int material = 20;                                       // Constante a ser adicionada a todos os valores base da consulta
    return (escolhaEspecialidade == 1 ? 50 : 60) + material; // Para a array de especialidades, a primeira especialidade na array tem o preço de 50 e as restantes 60
}

// Função para obter data e hora atual
void obterDataHoraAtual(char *buffer)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, 9, "%H:%M:%S", tm_info); // Formato obtido na hora gerada e hora atendida consoante as horas do pc.
}

// Função para escolher um médico
int escolherMedico(int especialidadeIndex)
{
    int numMedicos = (especialidadeIndex == 0) ? 5 : 2;
    int escolha;
    printf("\nEscolha o médico:\n");
    for (int i = 0; i < numMedicos; i++)
    {
        printf("%d - %s\n", i + 1, medicos[especialidadeIndex][i]);
    }
    do
    {
        printf("Digite o número correspondente ao médico: ");
        scanf("%d", &escolha);
    } while (escolha < 1 || escolha > numMedicos);
    return escolha - 1;
}

// Função para liberar uma sala
void liberarSala(int salaId)
{
    salasOcupadas[salaId] = 0;
    printf("Sala %d liberada.\n", salaId + 1);
}

// Função para escolher uma sala aleatória
int escolherSalaAleatoria()
{
    bool todasOcupadas = true;
    for (int i = 0; i < NUM_SALAS_TRIAGEM; i++) // Verifica se todas as salas de triagem estao ocupadas e se nao estiverem colocar a variavel booleana previamente definida como verdadeiro a falso
    {
        if (salasOcupadas[i] == 0)
        {
            todasOcupadas = false;
            break;
        }
    }

    if (todasOcupadas)
    {
        printf("Todas as salas de triagem estão ocupadas. Tente novamente mais tarde.\n");
        return -1;
    }

    int sala;
    do
    {
        sala = rand() % NUM_SALAS_TRIAGEM; // Escolhe uma sala de 1 a 5 aleatoriamente e coloca a mesma como ocupada até ser libertada
    } while (salasOcupadas[sala] == 1);

    salasOcupadas[sala] = 1;
    return sala;
}

// Função para gerar ticket de consulta marcada
void gerarTicketConsultaMarcada()
{
    if (totalMarcadas >= MAX_TICKETS) // Validaçao para verificar se o numero maximo de tickets de consulta marcada diarios foi atingido.
    {
        printf("Limite de tickets atingido!\n");
        return;
    }
    TicketConsultaMarcada *t = &consultasMarcadas[totalMarcadas]; // Estrutura de ticket de consulta marcada criada para facilitar o transporte de dados na funçao
    t->id = totalMarcadas + 1;                                    // Adiciona um ao total de consultas marcadas de forma ao ID ser de 1 a 100
    t->atendido = false;                                          // Torna o status do ID associado nao atendido
    t->balcao = (rand() % 2) + 1;                                 // Escolhe um balcao aleatoriamente 1 ou 2.
    obterDataHoraAtual(t->HoraGerado);                            // Recolhe a data e hora atual do PC.
    printf("\nTicket de Consulta Marcada Gerado:\n");
    printf("ID: %d| Data/Hora: %s\n",
           t->id, t->HoraGerado);
    totalMarcadas++;
}

// Função para gerar ticket de consulta urgente
void gerarTicketConsultaUrgencia()
{
    if (totalUrgentes >= MAX_TICKETS_URGENCIA) // Validaçao para verificar se o numero maximo de tickets diarios urgentes foi atingido
    {
        printf("Limite de tickets urgentes atingido!\n");
        return;
    }
    TicketConsultaUrgencia *t = &consultasUrgentes[totalUrgentes]; // Estrutura de ticket de consulta urgente criada para facilitar o transporte de dados na funçao
    t->balcao = (rand() % 3) + 1; // Associa o balcao destinado para consultas urgentes ao respetivo ticket
    t->id = totalUrgentes + 1;                                     // Adiciona um ao total de consultas urgente de forma ao ID ser de 1 a 10
    obterDataHoraAtual(t->HoraGerado);                             // Recolhe a data e hora atual do PC.
    t->atendido = false;                                           // Torna o status do ID associado nao atendido
    printf("\nTicket de Consulta Urgente Gerado:\n");
    printf("ID: %d | Data/Hora: %s\n",
           t->id, t->HoraGerado);

    totalUrgentes++;
}

// Função para listar todos os tickets
void listarTickets()
{
    printf("\n--- Tickets de Consulta Marcada ---\n");
    for (int i = 0; i < totalMarcadas; i++)
    {
        if (consultasMarcadas[i].atendido == false) // Se o status na variavel atendido da array de consultas marcadas for falsa (0) entao faz:
        {
            printf("ID: %d | Status: %s | Hora Gerada: %s\n",
                   consultasMarcadas[i].id,
                   consultasMarcadas[i].atendido ? "Atendido" : "Pendente",
                   consultasMarcadas[i].HoraGerado);
        }
        else // Se nao:
        {
            printf("ID: %d | Balcão: %d | Especialidade: %s | Médico: %s | Gabinete: %d | Valor Consulta: %d€ | Status: %s | Hora Gerada: %s | Hora Atendida: %s \n",
                   consultasMarcadas[i].id,
                   consultasMarcadas[i].balcao,
                   consultasMarcadas[i].especialidade,
                   consultasMarcadas[i].medico,
                   consultasMarcadas[i].gabinete,
                   consultasMarcadas[i].valorConsulta,
                   consultasMarcadas[i].atendido ? "Atendido" : "Pendente",
                   consultasMarcadas[i].HoraGerado,
                   consultasMarcadas[i].HoraAtendido);
        }
    }
    printf("\n--- Tickets de Consulta Urgente ---\n");
    for (int i = 0; i < totalUrgentes; i++)
    {
        if (consultasUrgentes[i].atendido == false)
        {
            printf("ID: %d | Status: %s | Hora Gerada: %s \n",
                   consultasUrgentes[i].id,
                   consultasUrgentes[i].atendido ? "Atendido" : "Pendente",
                   consultasUrgentes[i].HoraGerado);
        }
        else
        {
            printf("ID: %d | Balcão: %d | Sala de Triagem: %s | Status: %s | Hora Gerada: %s | Hora Atendida: %s \n",
                   consultasUrgentes[i].id,
                   consultasUrgentes[i].balcao,
                   consultasUrgentes[i].salaTriagem,
                   consultasUrgentes[i].atendido ? "Atendido" : "Pendente",
                   consultasUrgentes[i].HoraGerado,
                   consultasUrgentes[i].HoraAtendido);
        }
    }
}

// Função para atualizar tickets
void atualizarTicket()
{
    int tipo, id;
    printf("Escolha o tipo de ticket (1 - Marcada, 2 - Urgência): ");
    scanf("%d", &tipo);
    printf("Digite o ID do ticket: ");
    scanf("%d", &id);

    if (tipo == 1 && id > 0 && id <= totalMarcadas) // Se o ticker for de consulta marcada e estiver dentro de um ID valido (menor que o total de marcadas) faz
    {
        TicketConsultaMarcada *t = &consultasMarcadas[id - 1];
        if (!t->atendido) // Verifica se o ticket ja foi atendido
        {
            printf("\nPara qual especialidade médica deseja atualizar o ticket?:\n");
            for (int i = 0; i < NUM_ESPECIALIDADES; i++)
            {
                printf("%d - %s\n", i + 1, especialidades[i]); // Amostra as especialidades de consulta possiveis somando 1 á sua localização na array de forma a que começe em 1
            }

            int escolhaEspecialidade;
            do
            {
                printf("Digite o número da especialidade: ");
                scanf("%d", &escolhaEspecialidade);
            } while (escolhaEspecialidade < 1 || escolhaEspecialidade > NUM_ESPECIALIDADES); // Enquanto nao for dado como input uma especialidade valida a pergunta persiste

            strcpy(t->especialidade, especialidades[escolhaEspecialidade - 1]); // Coloca na estrutura temporaria a especilidade escolhida no input
            int indiceMedico = escolherMedico(escolhaEspecialidade - 1);        // Utiliza a funçao escolher medico para escolher um medico e colocar na variavel IndiceMedico
            strcpy(t->medico, medicos[escolhaEspecialidade - 1][indiceMedico]); // Coloca na estrutura temporaria o nome do mo medico escolhido anteriormente
            t->valorConsulta = obterValorConsulta(escolhaEspecialidade);        // Utiliza a funçao que calcular o valor da consulta por especialidade e coloca na estrutura temporaria

            printf("Digite o número do gabinete (1-%d): ", NUM_GABINETES);
            scanf("%d", &t->gabinete); // Recolhe o numero do gabinete

            obterDataHoraAtual(t->HoraAtendido);
            t->atendido = true; // Atualiza o ticket para atendido

            printf("Ticket de Consulta Marcada ID %d atendido no balcão %d:\n", t->id, t->balcao);
            printf("Especialidade: %s | Médico: %s | Gabinete: %d | Valor: %d€ | Data/Hora Atendido: %s\n",
                   t->especialidade, t->medico, t->gabinete, t->valorConsulta, t->HoraAtendido);
        }
        else
        {
            printf("Este ticket já foi atendido.\n");
        }
    }
    else if (tipo == 2 && id > 0 && id <= totalUrgentes)
    {
        TicketConsultaUrgencia *t = &consultasUrgentes[id - 1];
        if (!t->atendido) // Se o ticket nao estiver atendido:
        {
            int sala = escolherSalaAleatoria(); // Associada á variavel sala uma sala aleatoria entre 1 e 5 com a funçao escolhersalaaleatoria
            if (sala == -1)
            {
                return;
            }
            strcpy(t->salaTriagem, salasTriagem[sala]); // Associa a sala escolhida á estrutura temporaria.
            obterDataHoraAtual(t->HoraAtendido);        // Obtem a data e hora atual do PC para colocar a respetiva date e hora de atendimento
            t->atendido = true;                         // Transforma o ticket em atendido
            printf("Ticket de Consulta Urgente ID %d atualizado para atendido:\n", t->id);
            printf("ID: %d | Sala de Triagem: %s | Balcão: %d | Data/Hora: %s\n",
                   t->id, t->salaTriagem, t->balcao, t->HoraAtendido);
        }
        else
        {
            printf("Este ticket já foi atendido.\n");
        }
    }
    else
    {
        printf("ID inválido!\n");
    }
}
// Gerar mapa de médias de espera
void gerarMapaMediaEspera(const char *inicio, const char *fim)
{
    double somaEsperaMarcada = 0, somaEsperaUrgente = 0;
    int countMarcada = 0, countUrgente = 0;

    for (int i = 0; i < totalMarcadas; i++)
    {
        if (consultasMarcadas[i].atendido &&
            horaNoIntervalo(consultasMarcadas[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            long long segundosGerado = calcularSegundos(consultasMarcadas[i].HoraGerado);     // Calcula o total em segundos das horas em que o ticket de consulta marcada foi gerado
            long long segundosAtendido = calcularSegundos(consultasMarcadas[i].HoraAtendido); // Calcula o total em segundos das horas em que o ticket foi atendido
            somaEsperaMarcada += (segundosAtendido - segundosGerado) / 60.0;                  // Calcula em minutos o tempo total de espera entre o ticket ser gerado e atendido
            countMarcada++;
        }
    }

    for (int i = 0; i < totalUrgentes; i++)
    {
        if (consultasUrgentes[i].atendido &&
            horaNoIntervalo(consultasUrgentes[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            long long segundosGerado = calcularSegundos(consultasUrgentes[i].HoraGerado);     // Calcula o total em segundos das horas em que o ticket de consulta urgente foi gerado
            long long segundosAtendido = calcularSegundos(consultasUrgentes[i].HoraAtendido); // Calcula o total em segundos das horas em que o ticket de consulta urgente foi atendido
            somaEsperaUrgente += (segundosAtendido - segundosGerado) / 60.0;
            countUrgente++;
        }
    }

    printf("Média de Espera (em minutos):\n");
    printf("Consultas Marcadas: %.2f minutos\n", countMarcada ? somaEsperaMarcada / countMarcada : 0);
    printf("Consultas Urgentes: %.2f minutos\n", countUrgente ? somaEsperaUrgente / countUrgente : 0);
}

// Função para saber a produtividade de cada balcão
void gerarMapaBalcoesProdutividade(const char *inicio, const char *fim)
{
    int balcaoContagem[NUM_BALCAO] = {0}; // Array para colocar a contagem tickets atendidos no respetivo balcao no intervalo de tempo definido

    for (int i = 0; i < totalMarcadas; i++)
    {
        if (consultasMarcadas[i].atendido &&
            horaNoIntervalo(consultasMarcadas[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            balcaoContagem[consultasMarcadas[i].balcao - 1]++; // Ex: Se o balcao for o segundo, ele retira um para ser colocado no segundo espaço da array([1]) em vez do terceiro ([2]) e coloca um contador para ir aumentando dependedo do balcao em que o ticket foi atendido.
        }
    }

    for (int i = 0; i < totalUrgentes; i++)
    {
        if (consultasUrgentes[i].atendido &&
            horaNoIntervalo(consultasUrgentes[i].HoraAtendido, inicio, fim))
        {
            balcaoContagem[consultasUrgentes[i].balcao - 1]++;
        }
    }

    printf("Produtividade dos Balcões:\n");
    for (int i = 0; i < NUM_BALCAO; i++)
    {
        printf("Balcão %d: %d tickets atendidos\n", i + 1, balcaoContagem[i]);
    }
}

// Função para saber as receitas das consultas marcadas num intervalo de horas
void gerarMapaReceitas(const char *inicio, const char *fim)
{
    int totalReceitas = 0;

    for (int i = 0; i < totalMarcadas; i++)
    {
        if (consultasMarcadas[i].atendido &&
            horaNoIntervalo(consultasMarcadas[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            totalReceitas += consultasMarcadas[i].valorConsulta; // Somatorio de todos os valores das consultas marcadas.
        }
    }

    printf("Receitas de Consultas Marcadas no Intervalo: %d€\n", totalReceitas);
}

// Função para saber o número de tickets atendidos num intervalo de horas
void mapaticketsatendido(const char *inicio, const char *fim)
{
    int volumemarcadas = 0;
    int volumeurgentes = 0;
    int IDSMarcados[100];
    int IDSUrgentes[10];
    int indexMarcados = 0;
    int indexUrgentes = 0;

    for (int i = 0; i < totalMarcadas; i++)
    {
        if (consultasMarcadas[i].atendido &&
            horaNoIntervalo(consultasMarcadas[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            volumemarcadas++;                                       // Contador das consultas marcadas atendidas nesse intervalo
            IDSMarcados[indexMarcados++] = consultasMarcadas[i].id; // Adiciona os IDS das consultas marcadas á array
        }
    }

    for (int i = 0; i < totalUrgentes; i++)
    {
        if (consultasUrgentes[i].atendido &&
            horaNoIntervalo(consultasUrgentes[i].HoraAtendido, inicio, fim)) // Verifica se o ticket ja se encontra atendido e se o intervalo dado como input e valido
        {
            volumeurgentes++;                                       // Contador das consultas urgentes atendidas nesse intervalo
            IDSUrgentes[indexUrgentes++] = consultasUrgentes[i].id; // Adiciona os IDS das consultas urgentes á array
        }
    }

    printf("---Volume de Consultas---\n");
    printf("Consultas Marcadas: %d\n", volumemarcadas);
    for (int i = 0; i < volumemarcadas; i++)
    {
        printf("ID: %d\n", IDSMarcados[i]);
    }
    printf("Consultas Urgentes: %d\n", volumeurgentes);
    for (int i = 0; i < volumeurgentes; i++)
    {
        printf("ID: %d\n", IDSUrgentes[i]);
    }
}

int main()
{
    srand(time(NULL));
    int opcao, salaId;
    char horaInicio[9]; // Formato HH:MM:SS
    char horaFim[9];    // Formato HH:MM:SS

    do
    {
        printf("\n--- Menu Principal ---\n");
        printf("1. Gerar Ticket Consulta Marcada\n");
        printf("2. Gerar Ticket Consulta Urgente\n");
        printf("3. Listar Tickets\n");
        printf("4. Atualizar Ticket\n");
        printf("5. Libertar Sala de Triagem\n");
        printf("6. Gerar Mapa de tickets atendidos\n");
        printf("7. Gerar Mapa de tempo de espera entre atendimento de tickets\n");
        printf("8. Gerar Mapa de balcões produtivos\n");
        printf("9. Gerar Mapa de receitas de consultas marcadas\n");
        printf("10. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        switch (opcao)
        {
        case 1:
            gerarTicketConsultaMarcada();
            break;
        case 2:
            gerarTicketConsultaUrgencia();
            break;
        case 3:
            listarTickets();
            break;
        case 4:
            atualizarTicket();
            break;
        case 5:
            printf("Digite o número da sala a ser liberada (1-%d): ", NUM_SALAS_TRIAGEM);
            scanf("%d", &salaId);

            if (salaId < 1 || salaId > NUM_SALAS_TRIAGEM) // Valida se a sala for menor que 1 e maior que o definido
            {
                printf("Número de sala inválido!\n");
            }
            else if (salasOcupadas[salaId - 1] == 0)
            {
                printf("A sala %d já está livre.\n", salaId); // Valida se a sala ja esta livre
            }
            else
            {
                liberarSala(salaId - 1); // Liberta a sala em questao
            }
            break;
        case 6:
            printf("Digite a hora início (HH:MM:SS): ");
            scanf("%8s", horaInicio); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;

            printf("Digite a hora fim (HH:MM:SS): ");
            scanf("%8s", horaFim); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ; //%8s pois sao 8 chars (6 numeros e 2 :)
            mapaticketsatendido(horaInicio, horaFim);
            break;
        case 7:
            printf("Digite a hora início (HH:MM:SS): ");
            scanf("%8s", horaInicio); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;

            printf("Digite a hora fim (HH:MM:SS): ");
            scanf("%8s", horaFim); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;
            gerarMapaMediaEspera(horaInicio, horaFim);
            break;
        case 8:
            printf("Digite a hora início (HH:MM:SS): ");
            scanf("%8s", horaInicio); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;

            printf("Digite a hora fim (HH:MM:SS): ");
            scanf("%8s", horaFim); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;
            gerarMapaBalcoesProdutividade(horaInicio, horaFim);
            break;
        case 9:
            printf("Digite a hora início (HH:MM:SS): ");
            scanf("%8s", horaInicio); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;

            printf("Digite a hora fim (HH:MM:SS): ");
            scanf("%8s", horaFim); // %8s pois sao 8 chars (6 numeros e 2 :)
            while (getchar() != '\n' && getchar() != EOF)
                ;
            gerarMapaReceitas(horaInicio, horaFim);
            break;
        case 10:
            printf("Saindo do programa.\n");
            break;
        default:
            printf("Opção inválida!\n");
        }
    } while (opcao != 10);

    return 0;
}
