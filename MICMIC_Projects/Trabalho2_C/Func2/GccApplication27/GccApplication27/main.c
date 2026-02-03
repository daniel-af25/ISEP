++++++++++#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

typedef struct USARTRX
{
	char receiver_buffer;
	unsigned char status;
	unsigned char receiver : 1;
	unsigned char error : 1;
} USARTRX_st;


char function_mode;
char do_mode;
char msg[20];
volatile USARTRX_st rxUSART = {0, 0, 0, 0};
volatile unsigned char flag_stop250ms = 0;
volatile unsigned char flag_invert = 0;
volatile unsigned char contador_250ms = 0;
unsigned char ocr_percentagem = 0;
unsigned char temp_duty = 0;
const unsigned char digitos[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90};
unsigned char numerosdisplay[3];
unsigned char sinal = 0;
unsigned char contador0 = 0;
unsigned char contador1 = 0;
unsigned char contador2 = 0;
unsigned char contador3 = 0;


void handle_receiver_buffer();
void AtualizarDisplays(unsigned char value);
unsigned char lerSWf(void);
void sw1();
void sw2();
void sw3();
void sw4();
void sw5();


void init(void)
{
	DDRC = 0XFF;
	PORTC = 0XFF;
	DDRA = 0b11000000;
	PORTA = 0b10000000;
	DDRB = 0b11100000;
	PORTB = 0b00111110;
	OCR0 = 77;
	TCCR0 = 0b00001111;, 
	TIMSK |= 0b00000010;
	TCCR2 = 0b01100011;
	OCR2 = 0;
	UBRR1L = 207;
	UBRR1H = 0;
	UCSR1A = (1 << U2X1);
	UCSR1B = (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1);
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10) | (1<< USBS1);
	sei();
}


ISR(TIMER0_COMP_vect)
{
	AtualizarDisplays(ocr_percentagem);
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

void AtualizarDisplays(unsigned char value)
{
	contador3++;
	if (contador3 > 3){
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
		else
		{
			PORTC = 0b11111111;
		}
		return;
	}
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
		for (int j = 0; j < 3; j++)
		numerosdisplay[j] = 0;
	}
}

void SendString(char *msg)
{
	unsigned char i = 0;
	while (msg[i] != '\0')
	{
		while ((UCSR1A & (1 << UDRE1)) == 0);
		UDR1 = msg[i];
		i++;
	}
}

unsigned char calculo_numero(unsigned char percentagem)
{
	return percentagem;
}


unsigned char calculo_OCR(unsigned char percentagem)
{
	return (unsigned long)percentagem * 255UL / 100UL;
}


unsigned char lerSWf(void)
{
	for(int i = 0; i < 6; i++)
	{
		if((PINA & (1 << i)) == 0)
		{
			_delay_ms(5);
			if((PINA & (1 << i)) == 0)
			return i + 1;
		}
	}
	return 0;
}


void sw1()
{
	ocr_percentagem += 5;
	if(ocr_percentagem >= 100){
		ocr_percentagem = 100;
	}
	OCR2 = calculo_OCR(ocr_percentagem);
}

void sw2()
{
	if(ocr_percentagem < 5){
		ocr_percentagem = 0;
	}
	else
	ocr_percentagem -= 5;

	OCR2 = calculo_OCR(ocr_percentagem);
}

void sw3()
{
	ocr_percentagem = 25;
	OCR2 = calculo_OCR(25);
}

void sw4()
{
	ocr_percentagem = 50;
	OCR2 = calculo_OCR(50);
}

void sw5()
{
	flag_invert = 0;

	if(PORTB & (1 << 5))
	{
		PORTB &= ~(1 << 5);
		PORTB |=  (1 << 6);
	}
	else
	{
		PORTB &= ~(1 << 6);
		PORTB |=  (1 << 5);
	}

	sinal = (sinal == 0) ? 1 : 0;

	OCR2 = temp_duty;
}


void handle_receiver_buffer()
{
	if (rxUSART.receiver == 1)
	{
		if (rxUSART.error == 1)
		{
			rxUSART.error = 0;
		}
		else
		{
			if (rxUSART.receiver_buffer == 's' || rxUSART.receiver_buffer == 'S' || rxUSART.receiver_buffer == 'd' || rxUSART.receiver_buffer == 'D')
			{
				function_mode = rxUSART.receiver_buffer;
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
				sprintf(msg, "Duty");
				SendString(msg);
				do_mode = 0;
				break;
				default:
				break;
			}
		}
	}
	return 0;
}