#define F_CPU 16000000UL //Frequência do CPU, 16MHZ
//Bibliotecas do microcontrolador
#include <avr/io.h>
#include <avr/interrupt.h>
//Bibliotecas C standard
#include <stdio.h>
#include <stdlib.h>

#define BAUD 9600 // Define do BAUDRATE a 9600
#define MYUBRR F_CPU/16/BAUD-1

volatile unsigned char timer_flag = 0; // FLAG para interrupção global

//Inicialização de funções para aquando pre-processamento o compiler saber que as mesmas existem
void timer5_init(void);
void AD_init(void);
void usart_init(unsigned int ubrr);
void usart_putchar(char data);
int adc_read(int ch);
void usart_print_string(char* s);

//Interrupção global 500ms
ISR(TIMER5_COMPA_vect) {
	timer_flag = 1;
}

//Inicialização interrupção global
void timer5_init(void){
	// WGM52: Modo CTC | CS52 e CS50: Prescaler de 1024
	TCCR5B |= (1 << WGM52) | (1 << CS52) | (1 << CS50);
	OCR5A = 7812; //Valor para assegurar 500ms
	TIMSK5 |= (1 << OCIE5A);  //Habilita a interrupção por CTC no A
	return;
}

//Inicialização do AD
void AD_init(void) {
	// REFS0: Tensão de referência AVCC (5V no Mega2560)
	ADMUX = (1 << REFS0);
	// ADEN: Habilita ADC | ADPS2:0: Prescaler de 128 (16MHz / 128 = 125kHz de amostragem)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	return;
	}

//Inicialização da comunicação série assincrona
void usart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << TXEN0); // Habilitar envio de dados
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8bits, 1 stop bit
	return;
}

//Enviar temperatura/tensao via serie para o PC byte a byte
void usart_putchar(char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data; //Coloca char recebido no UDR0, registo de transmissão
	return;
}

//Leituras do AD
int adc_read(int ch) { //Receber o AD que está a ser utilizado (0 default)
	ch &= 0x07; 
	ADMUX = (ADMUX & 0xF8) | ch;
	ADCSRA |= (1 << ADSC); // Habilitamos a leitura
	while (ADCSRA & (1 << ADSC));// Espera fim da leitura
	return ADC;
}


void usart_print_string(char* s) {
	while (*s) usart_putchar(*s++);//Utiliza a função usart_putchar para enviar todos os chars(bit) presentes no array de chars do bufff
}


//Main
int main(void)
{
	char buffer[64]; //Buffer para enviar dados de temperatura e tensao para o PC
	char float_str[12];//Variavel para converter valor de temperatura de float para array de chars.
	char tension_str [6];//Variavel para converter valor de tensao de float para array de chars.
	unsigned int samples[64]; //Array para armazenar as leituras do AD e fazer a media
	unsigned int sum;// Variavel que armazena a soma dos valores da array das leituras
	//Inicializações
	timer5_init();
	usart_init(MYUBRR);
	AD_init();
	sei();
	//Main Loop
    while (1) 
    {
		if (timer_flag == 1) //De 500ms em 500ms..
		{
			sum = 0; //Inicializar sum a 0
			//Calcular uma média de 64 valores para a leitura do AD
			for (int i = 0; i < 64; i++) {
				samples[i] = adc_read(0); 
				sum += samples[i];
			}
			unsigned int ad_mean = (unsigned int) (sum/64);//Calcular media das leituras do ad (0 <= ad_mean <= 5)
			float tension_read = (float) (ad_mean * 5.0 / 1023.0);//Converter em tensão a variavel ad_mean
			float ad_converted = (float) (tension_read + 4.3346 / 0.2507);//Converter em temperatura a variavel tension_read
			dtostrf(ad_converted, 4, 2, float_str);//Transformar a temperatura convertida de float em str para poder ser enviado via comunicação serie
			dtostrf(tension_read, 4, 2, tension_str);//Transformar a tensão convertida de float em str para poder ser enviado via comunicação serie
			//Bloco condicional para verificar a temperatura e definir a correta mensagem para colocar no buffer.
			if (ad_converted >= 37.0)
			{
				sprintf(buffer, "Temperature: >=%s Celsius | Tension: %s V | AD Raw: %u \r\n", float_str, tension_str, ad_mean);
			}
			else if (ad_converted <= 17.0)
			{
				sprintf(buffer, "Temperature: <=%s Celsius | Tension: %s V | AD Raw: %u \r\n", float_str, tension_str, ad_mean);
			}
			else{
				sprintf(buffer, "Temperature: %s Celsius | Tension: %s V | AD Raw: %u \r\n", float_str, tension_str, ad_mean);
			}
			usart_print_string(buffer);//Envio dos dados para o PC
			timer_flag = 0;//Reset FLAG
		}
    }
	return 0;
}

