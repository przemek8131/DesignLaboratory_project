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
volatile uint8_t flaga = 0;		//flaga oznaczaj¹ca zebranie wszystkich czasów
volatile uint8_t komenda = 0;	//komenda przes³ana przez pilot
volatile uint8_t komenda_negacja = 0;	//komenda przes³ana przez pilot
volatile uint8_t error = 0; // flaga bledu odbioru
volatile uint8_t przytrzymanie = 0;
char rx_buf[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
//

void uart_putc(unsigned char c){
	while(!(UCSR0A & (1<<UDRE0)))
	{}
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
	TCCR1B |= (1<<ICES1) | (1<<CS11) | (1<<ICNC1); //wybranie narastaj¹cego zbocza i preskalera 8 razy - dok³adnoœæ do 0.5us i reduktor szumu (opóŸnienie 4 taktów zegara)
	TIMSK1 |= (1<<ICIE1);	//W³¹czenie przerwania w trybie input capture
}

ISR(TIMER1_CAPT_vect){
	if(flaga==0){
		pomiary[licznik] = ICR1/2;	//zapisanie przechwyconego czasu, przez 2 aby by³a dok³adnoœæ co do 1us
		TCNT1 = 0;					//wyzerowanie wartoœci licznika po przechwyceniu czasu
		if(licznik==33){			
			licznik = 0;				//wyzerowanie licznika po przechwyceniu wszystkich czasów
			flaga = 1;
		}
		else if(2500 <= pomiary[1] && pomiary[1] <= 3000){
			licznik = 0;
			pomiary[1] = 0;
			przytrzymanie = 1;
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

	sei();	//W³¹czenie globalnych przerwañ
	
    while (1) 
    {	if(przytrzymanie == 1){
	    sprintf(rx_buf,"Przytrzymany: %d\n\r",komenda);
	    uart_puts(rx_buf);
		przytrzymanie = 0;
		}
		if(flaga == 1){
			_delay_ms(200);
			if (4500 <= pomiary[1] && pomiary[1] <= 5500){    //sprawdzenie czy rozpoznana zosta³a poprawna sekwencja, pierwszy czas to odstêp pomiêdzy kolejnymi komendami
				for(int i = 18; i < 26; i++){
					if(900 <= pomiary[i] && pomiary[i] <=1500){
						komenda = (komenda >> 1);
					}
					else if(1800 <= pomiary[i] && pomiary[i] <= 2500){
						komenda = ((komenda >> 1) | 128);
					}
					else{
						komenda = 0;
						error = 1;
						break;
					}					
				}
				for(int i = 26; i < 34; i++){
					if(900 <= pomiary[i] && pomiary[i] <=1500){
						komenda_negacja = ((komenda_negacja >> 1) | 128);			//zapisywanie zanegowanej komendy w odwrotny sposób
					}
					else if(1800 <= pomiary[i] && pomiary[i] <= 2500){
						komenda_negacja = (komenda_negacja >> 1);
					}
					else{
						komenda_negacja = 0;
						error = 1;
						break;
					}
				}
				if((!error) && (komenda == komenda_negacja)){
					sprintf(rx_buf,"%d\n\r",komenda);
					uart_puts(rx_buf);
				}
				else{
					error = 0;
					sprintf(rx_buf,"Nie dziala\n\r");
					uart_puts(rx_buf);
				}
			}
			flaga = 0;
			sprintf(rx_buf,"-------\n\r");
			uart_puts(rx_buf);
			
		//if(flaga==1){		//wyswietlanie wszystkich pomiarow
			/*for(int i = 0;i < 34;i++){
			sprintf(rx_buf,"%d\n\r",pomiary[i]);
			uart_puts(rx_buf);
			pomiary[i] = 0;
			}
			sprintf(rx_buf,"------\n\r");
			uart_puts(rx_buf);
			//flaga = 0;*/
		
		}
	}
}