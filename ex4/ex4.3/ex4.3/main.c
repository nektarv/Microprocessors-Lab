#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/interrupt.h>

volatile int adc; // holds adc readings
volatile bool newData = false;	// set when adc isr completes

void write_2_nibbles(uint8_t input){
	uint8_t control_bits =  PIND & 0x0F;
	uint8_t output = control_bits + (input & 0xF0);
	
	PORTD = output;
	
	PORTD |= (1<<PD3);	// enable pulse
	_delay_ms(1);
	PORTD &= ~(1<<PD3);
	
	input <<= 4;
	output = control_bits + (input & 0xF0);
	
	PORTD = output;
	
	PORTD |= (1<<PD3);
	_delay_ms(1);
	PORTD &= ~(1<<PD3);
}

void lcd_data(uint8_t data){
	PORTD |= (1<<PD2);	// RS=1, data
	write_2_nibbles(data);
	_delay_us(250);
}

void lcd_command(uint8_t command){
	PORTD &= ~(1<<PD2);	// RS=0, instruction
	write_2_nibbles(command);
	_delay_us(250);
}

void lcd_clear_display(){
	lcd_command(1);
	_delay_ms(5);
}

void lcd_print_gas(){
	lcd_data('G');
	lcd_data('A');
	lcd_data('S');
	lcd_data(' ');
	lcd_data('D');
	lcd_data('E');
	lcd_data('T');
	lcd_data('E');
	lcd_data('C');
	lcd_data('T');
	lcd_data('E');
	lcd_data('D');
}

void lcd_print_clear(){
	lcd_data('C');
	lcd_data('L');
	lcd_data('E');
	lcd_data('A');
	lcd_data('R');
}

void lcd_init(){
	_delay_ms(200);
	
	for (int i=0; i<3; i++){
		PORTD = 0x30;
		PORTD |= (1<<PD3);
		_delay_ms(1);
		PORTD &= ~(1<<PD3);
		_delay_us(250);
	}

	PORTD = 0x20;
	PORTD |= (1<<PD3);
	_delay_ms(1);
	PORTD &= ~(1<<PD3);
	_delay_us(250);

	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_clear_display();
	lcd_command(0x06);
}

ISR(ADC_vect){
	adc = ADC;
	newData = true;
}

int main(){
	DDRD = 0xFF;	// D setup for lcd screen
	DDRC &= ~(1<<PD3);
	PORTC &= ~(1<<PD3);		// pc3 correctly setup for ADC3
	DDRB = 0xFF;	// B is output
	
	// setup adc registers
	ADMUX |= (1<<REFS0)|(1<<MUX1)|(1<<MUX0);
	ADCSRA |= (1<<ADEN)|(1<<ADIE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	
	lcd_init();
	
	float voltage = 0.0;	// Vin
	float ppm = 0.0;		// concentration
	
	sei();	// enable global interrupts
	
	while(1){
		newData = false;		// reset newData flag
		ADCSRA |= (1<<ADSC);	// enable adc conversion
		while(!newData);		// wait until conversion is done
		
		voltage = (adc*5.0)/1024.0;		// compute Vin
		ppm = (voltage - 0.1) * 77.52;	// compute concentration
		
		// display accordingly
		if (ppm < 75){
			lcd_clear_display();
			lcd_print_clear();
				
			if (ppm < 56.848){
				PORTB = 0b00000001;			
				_delay_ms(100);
			}
			else{
				PORTB = 0b00000011;
				_delay_ms(100);		
			}
		}
		else{
			lcd_clear_display();
			lcd_print_gas();
			
			if (ppm < 121.448){
				PORTB = 0b00000011;
				_delay_ms(50);
				PORTB = 0b00000000;
				_delay_ms(50);	
			}
			else if (ppm < 186.048){
				PORTB = 0b00000111;
				_delay_ms(50);
				PORTB = 0b00000000;
				_delay_ms(50);				
			}
			else if (ppm < 250.648){
				PORTB = 0b00001111;
				_delay_ms(50);
				PORTB = 0b00000000;
				_delay_ms(50);
			}			
			else if (ppm < 315.248){
				PORTB = 0b00011111;
				_delay_ms(50);
				PORTB = 0b00000000;
				_delay_ms(50);
			}
			else{
				PORTB = 0b00111111;
				_delay_ms(50);
				PORTB = 0b00000000;
				_delay_ms(50);
			}			
		}
	}
}