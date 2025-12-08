#ifndef _DS18BS20_
#define _DS18BS20_

#include "utils.h"

#define clear(register, bit) (register &= ~(1 << bit))
#define set(register, bit) (register |= (1 << bit))

bool one_wire_reset(){
	set(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(480);
	
	clear(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(100);
	
	uint8_t temp = PIND;
	_delay_us(380);
	
	if ((temp & (1<<PD4)) == 0) return 1;
	return 0;
}

uint8_t one_wire_receive_bit(){
	set(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(2);
	
	clear(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(10);

	uint8_t temp = 0;
	if ((PIND & (1 << PD4)) != 0)  temp = 1;
	
	_delay_us(49);
	
	return temp;
}

void one_wire_transmit_bit(bool output_bit){
	set(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(2);
	
	if (output_bit) set(PORTD, PD4);
	else clear(PORTD, PD4);
	
	_delay_us(58);
	
	clear(DDRD, PD4);
	clear(PORTD, PD4);
	_delay_us(1);
}

uint8_t one_wire_receive_byte(){
	uint8_t received_byte = 0;
	
	for (int i=0; i<8; i++){
		uint8_t b = one_wire_receive_bit();
		received_byte |= (b<<i);
	}
	
	return received_byte;
}

void one_wire_transmit_byte(uint8_t byte){
	bool bit;
	for (int i=0; i<8; i++){
		bit = (byte>>i) & 1;
		one_wire_transmit_bit(bit);
	}
}

int16_t read_temp(){
	// check if a device is connected
	if (!one_wire_reset()) return 0x8000;
	// skip choosing a device
	one_wire_transmit_byte(0xCC);
	
	// start a conversion
	one_wire_transmit_byte(0x44);
	// wait for it to finish
	while (!one_wire_receive_bit());
	
	// reset again and skip device choosing
	one_wire_reset();
	one_wire_transmit_byte(0xCC);
	
	// read the conversion
	one_wire_transmit_byte(0xBE);
	uint8_t low = one_wire_receive_byte();
	uint8_t high = one_wire_receive_byte();
	
	int16_t temp_measured = 0;
	temp_measured |= (high<<8);
	temp_measured |= low;
	
	return temp_measured;
}

#endif /*DS18BS20*/