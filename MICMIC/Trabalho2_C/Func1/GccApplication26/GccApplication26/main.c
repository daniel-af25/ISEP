#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


volatile unsigned char flag_stop250ms = 0;
volatile unsigned char flag_invert = 0;
volatile unsigned char contador_250ms = 0;
unsigned char ocr_percentagem = 50;
unsigned char temp_duty = 0;
const unsigned char digitos[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
unsigned char numerosdisplay[3];
unsigned char sinal = 0;
unsigned char contador0 = 0;
unsigned char contador1 = 0;
unsigned char contador2 = 0;


void AtualizarDisplays(unsigned char value);
unsigned char lerSWf(void);
unsigned char calculo_numero(unsigned char percentagem);
unsigned char calculo_OCR(unsigned char percentagem);
void sw1();
void sw2();
void sw3();
void sw4();
void sw5();


void init(void)
{
	DDRC = 0xFF;
	PORTC = 0xFF;

	DDRA = 0b11000000;
	PORTA = 0xFF;

	DDRB = 0b11100000;
	PORTB = 0b00111110;

	OCR0 = 77;
	TCCR0 = 0b00001111;
	TIMSK |= 0b00000010;

	TCCR2 = 0b01100011;
	OCR2 = 128;

	sei();
}


ISR(TIMER0_COMP_vect)
{
	AtualizarDisplays(ocr_percentagem);

	if(flag_stop250ms == 1)
	contador_250ms++;

	if(contador_250ms == 50)
	{
		flag_stop250ms = 0;
		contador_250ms = 0;
		flag_invert = 1;
	}
}


void AtualizarDisplays(unsigned char value)
{
	if(value == 0)
	{
		if(sinal == 1)
		{
			if(contador0 == 0)
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
	else if(value == 100)
	{
		if(sinal == 1)
		{
			if(contador2 == 0)
			{
				PORTA = 0b11000000;
				PORTC = 0x90;
				contador2 = 1;
			}
			else if(contador2 == 1)
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
			if(contador2 == 0)
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
		while(value != 0)
		{
			numerosdisplay[i] = value % 10;
			value = value / 10;
			i++;
		}

		if(i == 1)
		{
			if(sinal == 1)
			{
				if(contador1 == 0)
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

		if(i == 2)
		{
			if(sinal == 1)
			{
				if(contador2 == 0)
				{
					PORTA = 0b11000000;
					PORTC = digitos[numerosdisplay[0]];
					contador2 = 1;
				}
				else if(contador2 == 1)
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
				if(contador2 == 0)
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
		for(int j = 0; j < 3; j++)
		numerosdisplay[j] = 0;
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
	if(ocr_percentagem >= 100)
	ocr_percentagem = 100;

	OCR2 = calculo_OCR(ocr_percentagem);
}

void sw2()
{
	if(ocr_percentagem < 5)
	ocr_percentagem = 0;
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


int main(void)
{
	unsigned char stateSW;
	unsigned char lastSW = 0;

	init();

	while(1)
	{
		stateSW = lerSWf();

		if(flag_invert == 1)
		sw5();

		switch(stateSW)
		{
			case 0:
			lastSW = 0;
			break;

			case 1:
			if(flag_stop250ms == 0 && lastSW != 1)
			{
				sw1();
				lastSW = 1;
			}
			break;

			case 2:
			if(flag_stop250ms == 0 && lastSW != 2)
			{
				sw2();
				lastSW = 2;
			}
			break;

			case 3:
			if(flag_stop250ms == 0)
			{
				sw3();
				lastSW = 3;
			}
			break;

			case 4:
			if(flag_stop250ms == 0)
			{
				sw4();
				lastSW = 4;
			}
			break;

			case 5:
			if(lastSW != 5)
			{
				flag_stop250ms = 1;
				temp_duty = OCR2;
				PORTB |= (1 << 5) | (1 << 6);
				lastSW = 5;
			}
			break;

			case 6:
			if(flag_stop250ms == 0)
			{
				OCR2 = 0;
				ocr_percentagem = 0;
				lastSW = 6;
			}
			break;
		}
	}
}
