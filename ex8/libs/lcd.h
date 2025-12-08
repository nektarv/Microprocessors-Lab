#ifndef _LCD_
#define _LCD_

#include "utils.h"
#include "pca9555.h"

extern volatile uint8_t state;

void write_2_nibbles(uint8_t input){
	// preserve 4 control bits
	uint8_t control_bits = state & 0x0F;
	// combine control bits with high nibble of input
	uint8_t output = control_bits + (input & 0xF0);
	
	// update state and send to LCD
	state = output;
	PCA9555_0_write(REG_OUTPUT_0, state);
	
	// enable pulse
	state |= (1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_us(1);
	state &= ~(1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	
	// combine control bits with low nibble of input
	input <<= 4;
	output = control_bits + (input & 0xF0);
	
	// update state and send to LCD
	state = output;
	PCA9555_0_write(REG_OUTPUT_0, state);
	
	// enable pulse
	state |= (1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_us(1);
	state &= ~(1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
}

void lcd_command(uint8_t command){
	// RS=0, send command
	state &= ~(1<<PD2);
	PCA9555_0_write(REG_OUTPUT_0, state);
	write_2_nibbles(command);
	_delay_us(250);
}

void lcd_data(uint8_t data){
	// RS=1, send data
	state |= (1<<PD2);
	PCA9555_0_write(REG_OUTPUT_0, state);
	write_2_nibbles(data);
	_delay_us(250);
}

void lcd_clear_display(){
	lcd_command(1);
	_delay_ms(5);
}

void lcd_init(){
	_delay_ms(200);
	
	// send 0x30 three times - 8bit mode
	for (int i=0; i<3; i++){
		state = 0x30;
		PCA9555_0_write(REG_OUTPUT_0, state);
		state |= (1<<PD3);
		PCA9555_0_write(REG_OUTPUT_0, state);
		_delay_us(1);
		state &= ~(1<<PD3);
		PCA9555_0_write(REG_OUTPUT_0, state);
		_delay_us(250);
	}
	
	// send 0x20 - switch to 4bit mode
	state = 0x20;
	PCA9555_0_write(REG_OUTPUT_0, state);
	state |= (1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_us(1);
	state &= ~(1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_us(250);
	
	// screen setup from lab 4
	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_clear_display();
	lcd_command(0x06);
}

#endif /*LCD*/