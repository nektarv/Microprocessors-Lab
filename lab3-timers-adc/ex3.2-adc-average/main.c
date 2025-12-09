#define F_CPU 16000000UL
#include<avr/io.h>
#include<util/delay.h>

int table[17] = {5, 20, 36, 51, 66, 82, 97, 112, 128, 
				143, 158, 173, 189, 204, 219, 235, 250};

int increase(int *i){
	// 98% check
	if (*i == 16) return table[*i];
	// debouncing
	while(!(PINB & (1 << PB4))) _delay_ms(5);
	// increase DC
	return table[++(*i)];
}

int decrease(int *i){
	// 2% check
	if (*i == 0) return table[*i];
	// debouncing
	while(!(PINB & (1 << PB5))) _delay_ms(5);
	// decrease DC
	return table[--(*i)];
}

int main(){
	// setup PORTB
	DDRB = 0b00000010;
	PORTB = 0b00110000;

	// setup 8bit fast PWM
	// prescaler = 1
	// non-inverting mode
	TCCR1A = (1<<COM1A1)|(1<<WGM10);
	TCCR1B = (1<<WGM12)|(1<<CS10);
	
	// initial DC=50%
	int pwm = 8;
	int DC_VALUE = table[pwm];
	OCR1AH = 0;
	OCR1AL = DC_VALUE;
	
	// ------------------
	
	// the adc table will hold the last 16 measurements
	// current is our last measurement
	// oldest is the oldest measurement in the table
	// sum is the sum of the last 16 measurements
	// avg is the average of the last 16 measurements
	int current = 0;
	int oldest = 0;
	int sum = 0;
	int avg = -1;
	int adc[16];
	for(int i=0; i<16; i++) adc[i]=0;	// initialize measurements table
	
	// setup D as output
	DDRD = 0xFF;
	
	// setup adc, right adjusted
	ADMUX = (1<<REFS0)|(1<<MUX0);
	ADCSRA = (1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);

	while(1){
		// check if either pin is pressed
		
		if(!(PINB & (1 << PB4))){
			DC_VALUE = increase(&pwm);
			OCR1AL = DC_VALUE;
		}
		
		if(!(PINB & (1 << PB5))){
			DC_VALUE = decrease(&pwm);
			OCR1AL = DC_VALUE;
		}
		
		// read input
		ADCSRA |= (1<<ADSC);			// begin converting
		while(ADCSRA & (1 << ADSC));	// wait for adc to finish
		current = ADC;					// read ADCL first, then ADCH
		
		// calculate average
		sum += current - adc[oldest];	// add new one, subtract oldest one
		adc[oldest] = current;			// add current measurement to the table
		if(++oldest>15) oldest=0;		// wrap around the table
		avg = sum>>4;					// avg=sum/16
		
		// print
		if (avg<=200) PORTD = 1;
		else if (avg<=400) PORTD = 2;
		else if (avg<=600) PORTD = 4;
		else if (avg<=800) PORTD = 8;
		else PORTD = 16;
		
		_delay_ms(100);
	}
}