/*
 * Projekt_DL.c
 *
 * Created: 13.12.2021 21:07:46
 * Author : Przemek
 */ 

#define F_CPU 16000000UL

#define BAUD 9600UL
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)
#define Button_1 12
#define Button_2 24
#define Button_3 94
#define Button_4 8
#define Button_5 28
#define Button_6 90
#define Button_7 66
#define Button_8 82
#define Button_9 74
#define Power_OFF 69	//button ch-
#define Power_ON 70		//button ch+

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <stdio.h>

volatile uint8_t licznik = 0;
volatile uint16_t pomiary[34] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//array with saved times
volatile uint8_t flaga = 0;		//flag indicating that all times were measured and saved
volatile uint8_t komenda = 0;	//command send by IR remote
volatile uint8_t komenda_negacja = 0;	//inversed command send by IR remote
volatile uint8_t error = 0; //receive error flag
volatile uint8_t przytrzymanie = 0; //button holding flag
char rx_buf[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};


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
	TCCR1B |= (1<<ICES1) | (1<<CS11) | (1<<ICNC1); //choosing rising edge,prescaler 8 times - accuracy to 0.5us and noise reductor(4 timer ticks delay)
	TIMSK1 |= (1<<ICIE1);	//Interrupt enable in input capture mode
}

void react(void){
	switch(komenda){
		case Button_1:
			PORTD ^= (1 << 2);
		break;
		case Button_2:
			PORTD ^= (1 << 3);
		break;
		case Button_3:
			PORTD ^= (1 << 4);
		break;
		case Button_4:
			PORTD ^= (1 << 5);
		break;
		case Power_ON:
			PORTD &= ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));
		break;
		case Power_OFF:
		PORTD = ((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));
		break;
	}
}

void relays_init(void){
	DDRD = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
	PORTD = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
}

ISR(TIMER1_CAPT_vect){
	if(flaga==0){
		pomiary[licznik] = ICR1/2;	//captured time saving, time is divided by 2 to ensure 1us accuracy
		TCNT1 = 0;					//Timer for time measurement clearing
		if(licznik==33){			
			licznik = 0;				//all times counter clearing
			flaga = 1;
		}
		else if(2500 <= pomiary[1] && pomiary[1] <= 3000){
			licznik = 0;
			pomiary[1] = 0;
			przytrzymanie = 1;
		}
		else{
			licznik++;				//counter incrementation for following measurement
		}
	}	
}


int main(void)
{	
	timer1_init();
	uart_init();
	relays_init();

	sei();	//Enabling global interruptions 
	
    while (1) 
    {	if(przytrzymanie == 1){
	    sprintf(rx_buf,"Przytrzymany: %d\n\r",komenda);
	    uart_puts(rx_buf);
		przytrzymanie = 0;
		}
		if(flaga == 1){			
			_delay_ms(200);		/* if only a fragment of the signal was received, an incomplete message would be received when the button was pressed again,
									and subsequent interrupts would increment the counter incorrectly, the delay causes more than 34 interrupts to be skipped,
									the counter will be correctly reset and the next command will be correctly received*/
			
			if (4500 <= pomiary[1] && pomiary[1] <= 5500){    //checking if the correct sequence has been recognized, the first time is the interval between subsequent commands
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
						komenda_negacja = ((komenda_negacja >> 1) | 128);			//saving negated command in negation
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
				if((!error) && (komenda == komenda_negacja)){			//checking whether the command is correct
					react();
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
			
			//wyswietlanie wszystkich pomiarow
			
			//if(flaga==1){		
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