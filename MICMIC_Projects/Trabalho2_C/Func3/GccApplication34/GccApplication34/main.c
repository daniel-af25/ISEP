#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct USARTRX
{
	char receiver_buffer;
	unsigned char status;
	unsigned char receiver : 1;
	unsigned char error : 1;
} USARTRX_st;

char function_mode; // Modo de funcionamento
char do_mode; // Funçao a fazer dentro do modo de funcionamento especificico
char msg[20]; // Mensagem a enviar para o Putty
volatile USARTRX_st rxUSART = {0, 0, 0, 0}; // Inicializar Struct de comunicação de dados a 0
volatile unsigned char flag_stop250ms = 0; // Flag para esperar 250ms para inverter
volatile unsigned char flag_invert = 0; // Flag para iniciar o contador de 250ms
volatile unsigned char contador_250ms = 0; // Contador de 250ms
volatile unsigned char timer_passos = 0; // Contador de 25ms para o motor passo a passo
volatile signed char n_passos = 0; // Nº de Passo atual
unsigned char ocr_percentagem = 0; // Percentagem OCR
unsigned char temp_duty = 0; // Duty Cycle
const unsigned char digitos[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90}; // Digitos para o display 0-9
const unsigned char array_passes_p[] = {0b00001001, 0b00001100, 0b00000110, 0b00000011}; // Tabela passo completo para o motor PP
const unsigned char array_passes_n[] = {0b00000011, 0b00000110, 0b00001100, 0b00001001}; // Tabela passo completo para o motor PP
unsigned char numerosdisplay[3]; // Vetor para mostrar no display os valores
unsigned char sinal = 0; // Flag de Sinal
unsigned char valores_analog[4]; // Vetor para calcular a media no modo analogico
unsigned char contador_analog = 0; // Contador para circular o vetor no modo analogico
unsigned char contador0 = 0; // Contador para o display
unsigned char contador1 = 0; // Contador para o display
unsigned char contador2 = 0; // Contador para o display
unsigned char contador3 = 0; // Contador para o display
unsigned char analog_values[4];
signed char posicao_atual = 0; // Posiçao atual comparado á referencia
unsigned char passo_funcs_positivo = 0; // Contador para circular o vetor de passos completo do motor PP
unsigned char passo_funcs_negativo = 0; // Contador para circular inversamente o vetor de passos completo do motor PP
unsigned char contador_receiver = 0; // Contador para verificar valor de passos enviado do PC
char temp_str[4]; // Vetor de Chars para montar o Nº de passos enviado pelo PC
unsigned char contador_str = 0; // Contador para circular e  montar o vetor de Chars que irá er transformado em nº de passos a utilizar

unsigned char ler_valor_asm(); // Funcao Assembly
void handle_receiver_buffer(); // Tratamento do buffer
void AtualizarDisplays(unsigned char value, char posicao_atual); // Atualizar Displays
unsigned char lerSWf(void); // Ler SW
void sw1(); // SW1
void sw2(); // SW2
void sw3(); // SW3
void sw4(); // SW4
void sw5(); // SW5
void duty_analog(); // Função para calcular a média do potenciometro para meter no OCR.
void SendString(char *msg); // Funçao enviar String
unsigned char calculo_OCR(unsigned char percentagem); // Função cálculo OCR a partir da percentagem.
void motorpp_negativo(void); // Função para andar no sentido positivo do motor PP.
void motorpp_positivo(void);  // Função para andar no sentido negativo do motor PP.

void init(void)
{
	DDRC = 0XFF;
	PORTC = 0XFF;
	DDRA = 0b11000000;
	PORTA = 0b10000000;
	DDRE = 0b00001111;
	PORTE = 0b11111111;
	DDRB = 0b11100000;
	PORTB = 0b00111110;
	OCR0 = 77;
	TCCR0 = 0b00001111;
	TIMSK |= 0b00000010;
	TCCR2 = 0b01100011;
	OCR2 = 0;
	UBRR1L = 207;
	UBRR1H = 0;
	ADMUX = (1 << ADLAR) | (1 << MUX0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	UCSR1A = (1 << U2X1);
	UCSR1B = (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1);
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10) | (1 << USBS1);
	sei();
}

ISR(TIMER0_COMP_vect)
{
	timer_passos += 1;
	if (timer_passos == 5)
	{
		timer_passos = 0;
		if (n_passos > 0)
		{
			motorpp_positivo();
		}
		else if (n_passos < 0)
		{
			motorpp_negativo();
		}

		else
		{
			PORTE = 0b11111111;
		}
	}
	AtualizarDisplays(ocr_percentagem, posicao_atual);
	if (flag_stop250ms == 1)
	{
		contador_250ms++;
	}
	if (contador_250ms == 50)
	{
		flag_stop250ms = 0;
		contador_250ms = 0;
		flag_invert = 1;
	}
}

ISR(USART1_RX_vect)
{
	rxUSART.status = UCSR1A;
	if (rxUSART.status & ((1 << FE1) | (1 << DOR1) | (1 << UPE1)))
	{
		rxUSART.error = 1;
	}
	else
	{
		rxUSART.receiver_buffer = UDR1;
		rxUSART.receiver = 1;
	}
	handle_receiver_buffer();
}

void motorpp_positivo(void)
{
	if (posicao_atual >= 99)
	{
		n_passos = 0;
		return;
	}
	PORTE = array_passes[passo_funcs_positivo];
	passo_funcs_positivo++;
	if (passo_funcs_positivo == 4)
	{
		passo_funcs_positivo = 0;
	}
	n_passos--;
	posicao_atual++;
}

void motorpp_negativo(void)
{
	if (posicao_atual <= -99)
	{
		n_passos = 0;
		return;
	}
	PORTE = array_passes[passo_funcs_negativo];
	passo_funcs_negativo++;
	if (passo_funcs_negativo == 4)
	{
		passo_funcs_negativo = 0;
	}
	n_passos++;
	posicao_atual--;
}

void AtualizarDisplays(unsigned char value, char pos_atual)
{
	contador3++;
	if (contador3 > 3)
	{
		contador3 = 0;
	}
	if (contador3 == 3)
	{
		PORTA = 0b00000000;
		if (function_mode == 's' || function_mode == 'S')
		{
			PORTC = digitos[5];
		}
		else if (function_mode == 'd' || function_mode == 'D')
		{
			PORTC = 0b10100001;
		}
		else if (function_mode == 'a' || function_mode == 'A')
		{
			PORTC = 0b10001000;
		}
		else if (function_mode == 'm' || function_mode == 'M')
		{
			PORTC = 0b10000110;
		}
		else
		{
			PORTC = 0b11111111;
		}
		return;
	}
	if (function_mode == 'M' || function_mode == 'm')
	{
		if (pos_atual == 0)
		{
			PORTA = 0b11000000;
			PORTC = digitos[0];
		}
		else if (pos_atual < 0)
		{
			pos_atual *= -1;
			int i = 0;
			while (pos_atual != 0)
			{
				numerosdisplay[i] = pos_atual % 10;
				pos_atual = pos_atual / 10;
				i++;
			}
			if (i == 1)
			{
				if (contador1 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
					contador1 = 1;
				}
				else if (contador1 == 1)
				{
					PORTA = 0b10000000;
					PORTC = 0b11111111;
					contador1 = 2;
				}
				else
				{
					PORTA = 0b01000000;
					PORTC = 0b10111111;
					contador1 = 0;
				}
			}
			if (i == 2)
			{
				if (contador2 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
					contador2 = 1;
				}
				else if (contador2 == 1)
				{
					PORTA = 0b10000000;
					PORTC = digitos[numerosdisplay[1]];
					contador2 = 2;
				}
				else
				{
					PORTA = 0b01000000;
					PORTC = 0b10111111;
					contador2 = 0;
				}
			}
		}
		else
		{
			int i = 0;
			while (pos_atual != 0)
			{
				numerosdisplay[i] = pos_atual % 10;
				pos_atual = pos_atual / 10;
				i++;
			}
			if (i == 1)
			{
				if (contador1 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
					contador1 = 1;
				}
				else if (contador1 == 1)
				{
					PORTA = 0b10000000;
					PORTC = 0b11111111;
					contador1 = 2;
				}
				else
				{
					PORTA = 0b01000000;
					PORTC = 0b11111111;
					contador1 = 0;
				}
			}
			if (i == 2)
			{
				if (contador2 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
					contador2 = 1;
				}
				else
				{
					PORTA = 0b10000000;
					PORTC = digitos[numerosdisplay[1]];
					contador2 = 0;
				}
			}
		}
	}
	else
	{
		if (value == 0)
		{
			if (sinal == 1)
			{
				if (contador0 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[0];
					contador0 = 1;
				}
				else
				{
					PORTA = 0b10000000;
					PORTC = 0b10111111;
					contador0 = 0;
				}
			}
			else
			{
				PORTA = 0b11000000;
				PORTC = digitos[0];
			}
		}
		else if (value == 100)
		{
			if (sinal == 1)
			{
				if (contador2 == 0)
				{
					PORTA = 0b11000000;
					PORTC = 0x90;
					contador2 = 1;
				}
				else if (contador2 == 1)
				{
					PORTA = 0b10000000;
					PORTC = 0x90;
					contador2 = 2;
				}
				else
				{
					PORTA = 0b01000000;
					PORTC = 0b10111111;
					contador2 = 0;
				}
			}
			else
			{
				if (contador2 == 0)
				{
					PORTA = 0b11000000;
					PORTC = 0x90;
					contador2 = 1;
				}
				else
				{
					PORTA = 0b10000000;
					PORTC = 0x90;
					contador2 = 0;
				}
			}
		}
		else
		{
			int i = 0;
			while (value != 0)
			{
				numerosdisplay[i] = value % 10;
				value = value / 10;
				i++;
			}
			if (i == 1)
			{
				if (sinal == 1)
				{
					if (contador1 == 0)
					{
						PORTA = 0b11000000;
						PORTC = digitos[numerosdisplay[0]];
						contador1 = 1;
					}
					else
					{
						PORTA = 0b10000000;
						PORTC = 0b10111111;
						contador1 = 0;
					}
				}
				else
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
				}
			}
			if (i == 2)
			{
				if (sinal == 1)
				{
					if (contador2 == 0)
					{
						PORTA = 0b11000000;
						PORTC = digitos[numerosdisplay[0]];
						contador2 = 1;
					}
					else if (contador2 == 1)
					{
						PORTA = 0b10000000;
						PORTC = digitos[numerosdisplay[1]];
						contador2 = 2;
					}
					else
					{
						PORTA = 0b01000000;
						PORTC = 0b10111111;
						contador2 = 0;
					}
				}
				else
				{
					if (contador2 == 0)
					{
						PORTA = 0b11000000;
						PORTC = digitos[numerosdisplay[0]];
						contador2 = 1;
					}
					else
					{
						PORTA = 0b10000000;
						PORTC = digitos[numerosdisplay[1]];
						contador2 = 0;
					}
				}
			}
		}
	}
	for (int j = 0; j < 3; j++)
	{
		numerosdisplay[j] = 0;
	}
}

void SendString(char *msg)
{
	unsigned char i = 0;
	while (msg[i] != '\0')
	{
		while ((UCSR1A & (1 << UDRE1)) == 0)
		;
		UDR1 = msg[i];
		i++;
	}
}

unsigned char calculo_OCR(unsigned char percentagem)
{
	return (unsigned char)(percentagem * 255 / 100);
}

unsigned char lerSWf(void)
{
	for (int i = 0; i < 6; i++)
	{
		if ((PINA & (1 << i)) == 0)
		{
			_delay_ms(5);
			if ((PINA & (1 << i)) == 0)
			return i + 1;
		}
	}
	return 0;
}

void sw1(void)
{
	ocr_percentagem += 5;
	if (ocr_percentagem >= 100)
	{
		ocr_percentagem = 100;
	}
	OCR2 = calculo_OCR(ocr_percentagem);
}

void sw2(void)
{
	if (ocr_percentagem < 5)
	{
		ocr_percentagem = 0;
	}
	else
	ocr_percentagem -= 5;

	OCR2 = calculo_OCR(ocr_percentagem);
}

void sw3(void)
{
	ocr_percentagem = 25;
	OCR2 = calculo_OCR(25);
}

void sw4(void)
{
	ocr_percentagem = 50;
	OCR2 = calculo_OCR(50);
}

void sw5(void)
{
	flag_invert = 0;

	if (PORTB & (1 << 5))
	{
		PORTB &= ~(1 << 5);
		PORTB |= (1 << 6);
	}
	else
	{
		PORTB &= ~(1 << 6);
		PORTB |= (1 << 5);
	}

	sinal = (sinal == 0) ? 1 : 0;

	OCR2 = temp_duty;
}

void handle_receiver_buffer(void)
{
	if (rxUSART.receiver == 1)
	{
		if (rxUSART.error == 1)
		{
			rxUSART.error = 0;
		}
		else
		{
			if (rxUSART.receiver_buffer == 's' || rxUSART.receiver_buffer == 'S' || rxUSART.receiver_buffer == 'd' || rxUSART.receiver_buffer == 'D' || rxUSART.receiver_buffer == 'a' || rxUSART.receiver_buffer == 'A' || rxUSART.receiver_buffer == 'm' || rxUSART.receiver_buffer == 'M')
			{
				function_mode = rxUSART.receiver_buffer;
			}
			else if (rxUSART.receiver_buffer == 'n' || rxUSART.receiver_buffer == 'N')
			{
				if (n_passos != 0)
				{
					return;
				}
				contador_receiver = 1;
			}
			else if (contador_receiver == 1)
			{
				if (rxUSART.receiver_buffer == '\n' || rxUSART.receiver_buffer == '\r')
				{
					temp_str[contador_str] = '\0';
					contador_str = 0;
					contador_receiver = 0;
					n_passos = atoi(temp_str);
				}
				else
				{
					if (contador_str >= 4)
					{
						contador_str = 0;
					}
					temp_str[contador_str] = rxUSART.receiver_buffer;
					contador_str++;
				}
			}
			else
			{
				do_mode = rxUSART.receiver_buffer;
			}
		}
	}
	rxUSART.receiver = 0;
	rxUSART.receiver_buffer = 0;
}

void duty_analog(void)
{
	analog_values[contador_analog] = ler_valor_asm();
	contador_analog++;
	if (contador_analog == 4)
	{
		int soma = 0;
		for (int i = 0; i < 4; i++)
		{
			soma += analog_values[i];
		}
		OCR2 = (unsigned char)(soma / 4);
		contador_analog = 0;
	}
	ocr_percentagem = (unsigned char)((unsigned int)OCR2 * 100 / 255);
}

int main(void)
{
	unsigned char stateSW;
	unsigned char lastSW;
	init();
	while (1)
	{
		if (function_mode == 's' || function_mode == 'S')
		{
			stateSW = lerSWf();
			if (flag_invert == 1)
			{
				sw5();
			}
			switch (stateSW)
			{
				case 0:
				lastSW = 0;
				break;
				case 1:
				if (flag_stop250ms == 1)
				{
					break;
				}
				if (lastSW == 1)
				{
					break;
				}
				sw1();
				lastSW = 1;
				break;
				case 2:
				if (flag_stop250ms == 1)
				{
					break;
				}
				if (lastSW == 2)
				{
					break;
				}
				sw2();
				lastSW = 2;
				break;
				case 3:
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw3();
				lastSW = 3;
				break;
				case 4:
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw4();
				lastSW = 4;
				break;
				case 5:
				if (lastSW == 5)
				{
					break;
				}
				flag_stop250ms = 1;
				temp_duty = OCR2;
				PORTB |= (1 << 5) | (1 << 6);
				lastSW = 5;
				break;
				case 6:
				if (flag_stop250ms == 1)
				{
					break;
				}
				OCR2 = 0;
				lastSW = 6;
				break;

				default:
				break;
			}
		}
		else if (function_mode == 'd' || function_mode == 'D')
		{
			if (flag_invert == 1)
			{
				sw5();
			}
			switch (do_mode)
			{
				case '+':
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw1();
				do_mode = 0;
				break;
				case '-':
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw2();
				do_mode = 0;
				break;
				case '1':
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw3();
				do_mode = 0;
				break;
				case '2':
				if (flag_stop250ms == 1)
				{
					break;
				}
				sw4();
				do_mode = 0;
				break;
				case 'I':
				case 'i':
				flag_stop250ms = 1;
				temp_duty = OCR2;
				PORTB |= (1 << 5) | (1 << 6);
				do_mode = 0;
				break;
				case 'p':
				case 'P':
				if (flag_stop250ms == 1)
				{
					break;
				}
				OCR2 = 0;
				do_mode = 0;
				break;
				case 'b':
				case 'B':
				sprintf(msg, "Duty : %d %% \r\n", ocr_percentagem);
				SendString(msg);
				do_mode = 0;
				break;
				default:
				break;
			}
		}
		else if (function_mode == 'm' || function_mode == 'M')
		{
			switch (do_mode)
			{
				case '+':
				if (n_passos != 0)
				{
					break;
				}
				n_passos++;
				do_mode = 0;
				break;
				case '-':
				if (n_passos != 0)
				{
					break;
				}
				n_passos--;
				do_mode = 0;
				break;
				case 'r':
				case 'R':
				posicao_atual = 0;
				do_mode = 0;
				break;
				case 'b':
				case 'B':
				sprintf(msg, "Distanciado %d passos da posicao de referencia.\r\n", posicao_atual);
				SendString(msg);
				do_mode = 0;
				break;
				default:
				break;
			}
		}
		else if (function_mode == 'a' || function_mode == 'A')
		{
			if (do_mode == 'B' || do_mode == 'b')
			{
				sprintf(msg, "Duty : %d %% \r\n", ocr_percentagem);
				SendString(msg);
				do_mode = 0;
			}
			duty_analog();
			lastSW = 0;
			_delay_ms(5);
			if (flag_invert == 1)
			{
				sw5();
			}
			if ((PINA & (1 << 4)) == 0)
			{
				if (lastSW == 5)
				{
					break;
				}
				flag_stop250ms = 1;
				temp_duty = OCR2;
				PORTB |= (1 << 5) | (1 << 6);
				lastSW = 5;
			}
		}
	}
	return 0;
}