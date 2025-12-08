#ifndef _ADC_
#define _ADC_

#include "utils.h"

float read_pressure(){
	// start conversion
	ADCSRA |= (1<<ADSC);
	// wait for conversion to finish
	while(ADCSRA & (1<<ADSC));
	// store adc measurement
	uint16_t adc = ADCL | (ADCH<<8);
	// return pressure
	return (adc / 1023.0) * 20.0;
}

#endif /*ADC*/