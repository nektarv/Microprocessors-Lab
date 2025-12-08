#ifndef _KEYPAD_
#define _KEYPAD_

#include "utils.h"
#include "pca9555.h"

uint16_t scan_row(int row){
	// in order to scan a row, we need to set
	// its corresponding bit in the OUTPUT1
	// register to 0
	
	// when a button is pressed, the corresponding
	// column will be pulled low
	
	// we detect this flip to understand which button was pressed
	
	// alternatively, we can think of the button press as a switch
	// that connects the circuit and allows the current to pass from
	// high (Vcc) to low (the corresponding row)
	
	// set the according row's bit to 0
	uint8_t set_row = ~(1<<row);
	PCA9555_0_write(REG_OUTPUT_1, set_row);

	// check if any button is pressed
	// store IO1_7 - IO1_4 at the least important bits
	uint16_t buttons = PCA9555_0_read(REG_INPUT_1);
	buttons >>= 4;
	
	// restore everyone to HIGH (no button detection allowed)
	PCA9555_0_write(REG_OUTPUT_1, 0xFF);
	
	return buttons;
}

uint16_t scan_keypad(){
	// scan_keypad() checks the current state of the keyboard
	// as in who is pressed right now
	
	// pressed will hold they current state of the keyboard
	// first 4 MSB's will be the IO1_3 row
	// next 4 will be the IO1_2 row
	// next 4 will be the IO1_1 row
	// 4 LSB's will be the IO1_0 row
	uint16_t pressed = 0;
	
	for (int i=3; i>=0; i--){
		uint16_t current = scan_row(i);
		pressed = (pressed << 4) | current;
	}
	
	return pressed;
}

uint16_t scan_keypad_rising_edge(){
	// scan_keypad_rising_edge() doesn't check
	// who is pressed right now like scan_keypad() does
	
	// it checks if any new button was pressed since we last
	// called it, using the pressed_keys variable
	
	// initialize pressed_keys (only the first time the function is called)
	static uint16_t pressed_keys = 0xFFFF;
	
	// take first scan and a debouncing scan
	uint16_t pressed_keys_tempo = scan_keypad();
	_delay_ms(20);
	uint16_t debounced_keys = scan_keypad();
	
	// the following OR operation is a conservative approach towards
	// treating the keypad since it wouldn't allow for a key to be pressed
	// and released within 20ms (aka went from 0 to 1 between the two scans)
	// for a real word application where physical presses should be over 20ms
	// this approach is good enough
	
	// 0 = pressed, we only want to keep the consistent 0's
	// set to 1 everyone that was 1 in either scan
	pressed_keys_tempo |= debounced_keys;
	
	uint16_t just_pressed = 0;
	for (int i=0; i<16; i++){
		uint16_t mask = (1<<i);
		// if a key was not previously pressed but is pressed now
		// set the according bit to 1 in the just_pressed variable
		if ((pressed_keys_tempo & mask) == 0){
			if ((pressed_keys & mask) != 0){
				just_pressed |= mask;
			}
		}
	}
	
	// update pressed_keys for next call
	pressed_keys = pressed_keys_tempo;
	
	// return the inverse of just_pressed (0's correspond to presses)
	return ~just_pressed;
}

char keypad_to_ascii(uint16_t keys){
	switch(keys){
		case 0b1111111111111110: return '*';
		case 0b1111111111111101: return '0';
		case 0b1111111111111011: return '#';
		case 0b1111111111110111: return 'D';
		case 0b1111111111101111: return '7';
		case 0b1111111111011111: return '8';
		case 0b1111111110111111: return '9';
		case 0b1111111101111111: return 'C';
		case 0b1111111011111111: return '4';
		case 0b1111110111111111: return '5';
		case 0b1111101111111111: return '6';
		case 0b1111011111111111: return 'B';
		case 0b1110111111111111: return '1';
		case 0b1101111111111111: return '2';
		case 0b1011111111111111: return '3';
		case 0b0111111111111111: return 'A';
		
		default: return 0;
	}
}

#endif /*KEYPAD*/