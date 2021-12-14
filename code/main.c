/*
 * Projekt_DL.c
 *
 * Created: 13.12.2021 21:07:46
 * Author : Przemek
 */ 

#define F_CPU 16000000UL
//
#define BAUD 9600UL
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)

#include <avr/io.h>
#include <avr/interrupt.h>
//
#include <stdlib.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <stdio.h>

volatile uint8_t licznik = 0;
volatile uint16_t pomiary[34] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
volatile uint8_t flaga = 0;		//flaga oznaczająca zebranie wszystkich czasów
char rx_buf[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
//

void uart_putc(unsigned char c){
	while(!(UCSR0A & (1<<UDRE0)))
	{
		
	}
	UDR0 = c;
}

void uart_puts (char*s){
	while(*s){
		uart_putc(*s);
		s++;
	}
}

void uart_init(void){
	UBRR0 = UBRR_VAL;
	UCSR0B |= (1<<TXEN0);
	UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);
	
}

void timer1_init(void){
	TCCR1B |= (1<<ICES1) | (1<<CS11) | (1<<ICNC1); //wybranie narastającego zbocza i preskalera 8 razy - dokładność do 0.5us i reduktor szumu (opóźnienie 4 taktów zegara)
	TIMSK1 |= (1<<ICIE1);	//Włączenie przerwania w trybie input capture
}

ISR(TIMER1_CAPT_vect){
	if(flaga==0){
		pomiary[licznik] = ICR1/2;	//zapisanie przechwyconego czasu, przez 2 aby była dokładność co do 1us
		TCNT1 = 0;					//wyzerowanie wartości licznika po przechwyceniu czasu
		if(licznik==33){			
			licznik = 0;				//wyzerowanie licznika po przechwyceniu wszystkich czasów
			flaga = 1;
		}
		else{
			licznik++;				//inkrementacja licznika dla kolejnego pomiaru
		}
	}	
}


int main(void)
{	
	timer1_init();
	uart_init();

	sei();	//Włączenie globalnych przerwań
	
    while (1) 
    {	
		if(flaga == 1){
			for(int i = 0;i < 34;i++){
			sprintf(rx_buf,"%d\n\r",pomiary[i]);
			uart_puts(rx_buf);
			pomiary[i] = 0;
			}
			sprintf(rx_buf,"------\n\r");
			uart_puts(rx_buf);
			flaga = 0;
		}
	}
}