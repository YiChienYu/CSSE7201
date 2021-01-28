/*
 * AVR-PROGRAMMING.c
 *
 * Created: 10/21/2020 10:37:56 AM
 * Author : Yu Yi-Chien
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>


/*
** The following macros can be used to check for buttons being pushed.
*/
#define START ((PIND & (1<<2)) >> 2)
#define RESET ((PIND & (1<<3)) >> 3)

volatile uint16_t count = 0;
volatile uint16_t seconds = 0;

/*
** Check whether or not the operation is ongoing.
** 1 is ongoing
*/
volatile uint16_t operation_ongoing = 0;

/*
** Leds for second stage at wash stage available or not.
** 1 is available.
*/
volatile uint16_t blank_leds = 0;

/*
** Leds for second stage at rinse stage available or not.
** 1 is available.
*/
volatile uint16_t blank_leds2 = 0;

/*
** Leds for second stage at spin stage available or not.
** 1 is available.
*/
volatile uint16_t blank_leds3 = 0;

/*
** The stage is available or not.
** 1 is available.
*/
volatile uint16_t wash_available = 0;
volatile uint16_t rinse_available = 0;
volatile uint16_t spin_available = 0;

/*
** The SSDs is changeable or not.
** 1 is changeable.
*/
volatile uint16_t changeable= 1;

/*
** Which side SSDs is displaying.
** 1 is left SSD; 0 is right one.
*/
volatile uint8_t digit;

/*
** Check which mode rinse is in.
** 1 is Normal Mode; 0 is Extended Mode.
*/
volatile uint8_t spin_time = 0;

/*
** Check if current operation has finished.
** 0 is finished, this means SSDs will display 00.
*/
volatile uint8_t finished = 1;


uint8_t water_level[4] = {8,64,1,121};
uint8_t modes[2] = {84,121};
	


void display_digit(uint8_t selected_level, uint8_t selected_mode, uint8_t digit, uint8_t finished)
{
	if (finished) {
		if (digit == 0) {
			PORTA = water_level[selected_level] | (digit<<7);
			} else {
			PORTA = modes[selected_mode] | (digit<<7);
		}
	} else {
		PORTA = 63 | (digit<<7);
	}
	
}

int main(void)
{

	/*
	** Let port A and first 5 bits of port B be outputs.
	** Let port D be inputs.
	*/
	DDRA = 0xFF;
	DDRD = 0;
	DDRB = (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4);
	
	/* Set up timer/counter 0 so that we get an 
	** interrupt 1000 times per second.
	*/
	OCR0A = 124;
	TCCR0A = (1<<WGM00)|(1<<WGM01); /* Fast PWM mode */
	TCCR0B = (1<<WGM02)| (1<<CS01)| (1<<CS00); /* Divide clock by 64 */
	
	/* Ensure interrupt flag is cleared */
	TIFR0 = (1<<OCF0B);
	
	/* Enable interrupt on timer 0 on output compare match 
	*/
	TIMSK0 = (1<<OCIE0B);
	
	/* Set up timer/counter 1 so that we get an 
	** interrupt 1000 times per second.
	*/
	OCR1A = 124;
	TCCR1A = 0; /* CTC mode */
	TCCR1B = (1<<WGM12)| (1<<CS11)| (1<<CS10); /* Divide clock by 64 */
	
	/* Ensure interrupt flag is cleared */
	TIFR1 = (1<<OCF1A);
	
	/* Enable interrupt on timer 1 on output compare match 
	*/
	TIMSK1 = (1<<OCIE1A);
	
	/* Turn on global interrupts */
	sei();
	
	
	digit = 0;
    while (1) {
		/* Initial state of leds */
		if(!operation_ongoing) {
			PORTB = 0;
		}
		
		uint8_t s0 = PIND & (1<<0);
		uint8_t s1 = ((PIND & (1<<1)) >> 1);
		
		/* Duty cycle control */
		if (wash_available) {
			OCR0B = 112;
		} 
		
		if (rinse_available) {
			OCR0B = 62;
		}
			
		if (spin_available) {
			OCR0B = 12;
		}
		
		if (changeable) {
			/* Rinse ode selection */
			if (((PIND & (1<<4)) >> 4) == 0) {
				spin_time = 1;
				} else if (((PIND & (1<<4)) >> 4) == 1) {
				spin_time = 0;
			}
		}
		

		if (RESET) {
			/* Reset button has been pushed, wait for it to be released */
			while(RESET) {
				/* Do nothing */
			}
			wash_available = 0;
			blank_leds = 0;
			blank_leds2 = 0;
			blank_leds3 = 0;
			rinse_available = 0;
			spin_available = 0;
			seconds = 0;
			count = 0;
			changeable = 1;
			PORTB = 0;
			finished = 1;
			operation_ongoing = 0;
		}

		if (START) {
			/* Start button has been pushed, wait for it to be released */
			while(START) {
				/* Do nothing */
			}
			if (changeable) {
				/* When operation start, press start button will do nothing */
				if (s1 != 1 || s0 !=1) {
					PORTB = 0;
					changeable = 0;
					operation_ongoing = 1;
					wash_available = 1;
					
				}
				finished = 1;
			}
			
		}
    }
}

void wash(){
	if(wash_available) {
		/* If we've reached 1000, then wrap counter around to 0. */
		count++;
		if (count == 1000) {
			count = 0;
		}
		PORTB = 0;
		if (count <= 250 ) {
			PORTB = 1;
			} else if (250 < count && count <= 500) {
			PORTB = 2;
			}else if (500 < count && count <= 750) {
			PORTB = 4;
			} else if (750 < count && count < 1001) {
			PORTB = 8;
		}
		if (seconds < 3000) {
			seconds++;
			/* If we've reached 3000, then wrap second around to 0. */
			if(seconds == 3000) {
				PORTB = 0;
				wash_available = 0;
				blank_leds = 1;
			}
		}
	}
	
	if (blank_leds) {
		count++;
		PORTB = 0;
		if (count == 1000) {
			count = 0;
		}
		if (count < 1001) {
			PORTB = 15;
		}
		if (seconds < 6000) {
			seconds++;
			if(seconds == 6000) {
				PORTB = 0;
				blank_leds = 0;
				seconds = 0;
				rinse_available = 1;
			}
		}
	}
	
	
}

void rinse(){
	if(rinse_available) {
		count++;
		if (count == 1000) {
			count = 0;
		}
		PORTB = 0;
		if (count <= 250 ) {
			PORTB = 8;
			} else if (250 < count && count <= 500) {
			PORTB = 4;
			}else if (500 < count && count <= 750) {
			PORTB = 2;
			} else if (750 < count && count < 1001) {
			PORTB = 1;
		}
		if (seconds < 3000) {
			seconds++;
			if(seconds == 3000) {
				PORTB = 0;
				rinse_available = 0;
				blank_leds2 = 1;
			}
		}
	}
	
	if (blank_leds2) {
		count++;
		PORTB = 0;
		if (count == 1000) {
			count = 0;
		}
		if (count <= 500) {
			PORTB = 15;
			} else if (500 < count && count < 1001) {
			PORTB  = 0;
		}
		if (seconds < 6000) {
			seconds++;
			if(seconds == 6000) {
				PORTB = 0;
				 if (spin_time == 0) {
					 blank_leds2 = 0;
					 spin_time++;
					 seconds = 0;
					 rinse_available = 1;
					 spin_available = 0;
				 } else {
					 PORTB = 0;
					  blank_leds2 = 0;
					  seconds = 0;
					  spin_available = 1;
				 }
					
			}
		}
	}
}

void spin() {
	if(spin_available) {
		count++;
		if (count == 1000) {
			count = 0;
		}
		PORTB = 0;
		if (count <= 125 ) {
			PORTB = 8;
			} else if (125 < count && count <= 250) {
			PORTB = 4;
			}else if (250 < count && count <= 375) {
			PORTB = 2;
			} else if (375 < count && count < 500) {
			PORTB = 1;
			} else if (500 < count && count <= 625) {
			PORTB = 1;
			} else if (625 < count && count <= 750) {
			PORTB = 2;
			} else if (750 < count && count <= 875) {
			PORTB = 4;
			} else if (875 < count && count < 1001) {
			PORTB = 8;
			}
		if (seconds < 3000) {
			seconds++;
			if(seconds == 3000) {
				PORTB = 0;
				spin_available = 0;
				blank_leds3 = 1;
			}
		}
	}
	
	if (blank_leds3) {
		count++;
		PORTB = 0;
		if (count == 500) {
			count = 0;
		}
		if (count <= 250) {
			PORTB = 0;
			} else if (250 < count && count < 501) {
			PORTB = 15;
		}
		if (seconds < 6000) {
			seconds++;
			if(seconds == 6000) {
				PORTB = 0;
				blank_leds3 = 0;
				seconds = 0;
				changeable = 1;
				finished = 0;
				operation_ongoing = 0;
			}
		}
	}
}
void start() {
	if (operation_ongoing) {
		wash();
		rinse();
		spin();
	}
}

ISR(TIMER1_COMPA_vect) {
	uint8_t a = PIND & (1<<0);
	uint8_t b = ((PIND & (1<<1)) >> 1);
	uint8_t c = a * 2;
	uint8_t d = ((PIND & (1<<4)) >> 4);
	uint8_t selected_level = b + c;
	uint8_t selected_mode = d;
	
	display_digit(selected_level, selected_mode, digit, finished);
	/* Change the digit flag for next time. if 0 becomes 1, if 1 becomes 0. */
	digit = 1 - digit;
	start();
	
}

ISR(TIMER0_COMPB_vect) {
	/* Interrupt for led 7(PWM). */
	if (operation_ongoing == 0) {
		PORTB = 0;
	} else if (operation_ongoing == 1) {
		PORTB |=(1<<4);
	}
	
}
