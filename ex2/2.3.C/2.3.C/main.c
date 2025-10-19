#define F_CPU 1000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<avr/cpufunc.h>
#include<stdbool.h>

bool lights_on = false;
bool refresh = false;

ISR(INT1_vect){
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
	EICRA = (1 << ISC11);
	// enable INT1
	EIMSK = (1 << INT1);
	
	// set ports
	DDRB = 0xFF;
	DDRD = 0x00;
	PORTD |= (1 << PD3);
	
	// enable global interrupts
	sei();
	
	while(1){
		
		if (lights_on && !refresh){
			
			// turn on PB3 for 4sec
			PORTB = 0b00001000;
			for (int i=0; i<4000; i++){
				_delay_ms(1);
				if (refresh) break;
			}
			if (refresh) continue;	// skip the next command (don't turn the lights off, we need to be in refresh-ready mode)
			
			// 4sec passed
			lights_on = false;
		}
		
		else if (refresh){
			// if another interrupt happens, we need a way to know we re-refreshed
			refresh = false;
			
			// turn on PB1-PB5 for 1sec
			PORTB = 0b00111110;
			for (int i=0; i<1000; i++){
				_delay_ms(1);
				if (refresh) break;	// re-refresh
			}
			if (refresh) continue;	// skip the next commands (only PB3 would light up for a split second)
			
			// keep PB3 on for another 3sec
			PORTB = 0b00001000;
			for (int i=0; i<3000; i++){
				_delay_ms(1);
				if (refresh) break;	// re-refresh
			}
			if (refresh) continue;	// skip the next command (don't turn the lights off, we need to be in refresh-ready mode)
			
			// 4sec passed
			lights_on = false;
		}
		
		else{
			// turn off the lights
			PORTB = 0b00000000;
		}
	}
}