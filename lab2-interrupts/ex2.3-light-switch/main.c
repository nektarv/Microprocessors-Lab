// we delay using for loops with 1msec delays
// because interrupts can not occur while
// we're in the _delay_ms() function

#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<avr/cpufunc.h>
#include<stdbool.h>

volatile bool lights_on = false;
volatile bool refresh = false;

ISR(INT1_vect){
	// debouncing: check if EIFR has been reset due to bouncing
	while(EIFR & (1 << INT1)){
		EIFR = (1 << INT1);
		_delay_ms(5);
	}
	
	if(!lights_on){
		// turn on the lights
		lights_on = true;
	}
	else{
		// lights already on, refresh
		refresh = true;
	}
}

int main(){
	// interrupt on falling edge of INT1
	EICRA = (1 << ISC11 | 1 << ISC10);
	// enable INT1
	EIMSK = (1 << INT1);
	
	// set ports
	DDRB = 0xFF;	// output
	DDRD = 0x00;	// input
	PORTD = (1 << PD3);	// enable pull-up
	
	// enable global interrupts
	sei();
	
	while(1){
		
		// case 1: first interrupt -> turn on PB3 for 4sec
		if (lights_on && !refresh){
			PORTB = 0b00001000;
			for (int i=0; i<4000; i++){
				_delay_ms(1);
				if (refresh) break;	// a second interrupt occurred while we were here, break loop
			}
			if (refresh) continue;	// skip the next command (don't turn the lights off, we need to be in refresh-ready mode)
			
			// 4sec passed
			lights_on = false;
		}
		
		// case 2: interrupt occurs before previous interrupt finishes
		else if (refresh){
			// if another interrupt occurs, we need a way to know we re-refreshed inside the for-loops
			refresh = false;
			
			// turn on PB1-PB5 for 1sec
			PORTB = 0b00111110;
			for (int i=0; i<1000; i++){
				_delay_ms(1);
				if (refresh) break;	// another interrupt occurred while we were here, re-refresh
			}
			if (refresh) continue;	// skip the next commands (only PB3 would light up for a split second)
			
			// keep PB3 on for another 3sec
			PORTB = 0b00001000;
			for (int i=0; i<3000; i++){
				_delay_ms(1);
				if (refresh) break;	// another interrupt occurred while we were here, re-refresh
			}
			if (refresh) continue;	// skip the next command (don't turn the lights off, we need to be in refresh-ready mode)
			
			// 4sec passed
			lights_on = false;
		}
		
		// case 3: no interrupts, lights are off
		else{
			PORTB = 0b00000000;
		}
	}
}