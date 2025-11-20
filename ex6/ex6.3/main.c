#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

#define PCA9555_0_ADDRESS 0x40          // A0=A1=A2=0 by hardware
#define TWI_READ 1                      // reading from TWI device
#define TWI_WRITE 0                     // writing to TWI device
#define SCL_CLOCK 100000L               // twi clock in Hz

// Fscl=Fcpu/(16+2*TWBR0_VALUE*PRESCALER_VALUE)
#define TWBR0_VALUE ((F_CPU/SCL_CLOCK)-16)/2

volatile uint8_t state = 0;	// tracks PCA PORT0 bits

// PCA9555 REGISTERS
typedef enum {
	REG_INPUT_0 = 0,
	REG_INPUT_1 = 1,
	REG_OUTPUT_0 = 2,
	REG_OUTPUT_1 = 3,
	REG_POLARITY_INV_0 = 4,
	REG_POLARITY_INV_1 = 5,
	REG_CONFIGURATION_0 = 6,
	REG_CONFIGURATION_1 = 7
} PCA9555_REGISTERS;

//----------- Master Transmitter/Receiver -------------------
#define TW_START 0x08
#define TW_REP_START 0x10

//---------------- Master Transmitter ----------------------
#define TW_MT_SLA_ACK 0x18
#define TW_MT_SLA_NACK 0x20
#define TW_MT_DATA_ACK 0x28

//---------------- Master Receiver ----------------
#define TW_MR_SLA_ACK 0x40
#define TW_MR_SLA_NACK 0x48
#define TW_MR_DATA_NACK 0x58

#define TW_STATUS_MASK 0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)

//initialize TWI clock
void twi_init(void){
	TWSR0 = 0;                          // PRESCALER_VALUE=1
	TWBR0 = TWBR0_VALUE;                // SCL_CLOCK 100KHz
}

// Read one byte from the twi device (request more data from device)
unsigned char twi_readAck(void){
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

// Read one byte from the twi device, read is followed by a stop condition
unsigned char twi_readNak(void){
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

// Issues a start condition and sends address and transfer direction.
// return 0 = device accessible, 1= failed to access device
unsigned char twi_start(unsigned char address){
	uint8_t twi_status;
	// send START condition
	TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	// wait until transmission completed
	while(!(TWCR0 & (1<<TWINT)));
	// check value of TWI Status Register.
	twi_status = TW_STATUS & 0xF8;
	if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) return 1;
	// send device address
	TWDR0 = address;
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	// wail until transmission completed and ACK/NACK has been received
	while(!(TWCR0 & (1<<TWINT)));
	// check value of TWI Status Register.
	twi_status = TW_STATUS & 0xF8;
	if ((twi_status != TW_MT_SLA_ACK) && (twi_status != TW_MR_SLA_ACK))
	{
		return 1;
	}
	return 0;
}

// Send start condition, address, transfer direction.
// Use ack polling to wait until device is ready
void twi_start_wait(unsigned char address){
	uint8_t twi_status;
	while ( 1 )
	{
		// send START condition
		TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		// wait until transmission completed
		while(!(TWCR0 & (1<<TWINT)));
		// check value of TWI Status Register.
		twi_status = TW_STATUS & 0xF8;
		if ((twi_status != TW_START) && (twi_status != TW_REP_START)) continue;
		// send device address
		TWDR0 = address;
		TWCR0 = (1<<TWINT) | (1<<TWEN);
		// wail until transmission completed
		while(!(TWCR0 & (1<<TWINT)));
		// check value of TWI Status Register.
		twi_status = TW_STATUS & 0xF8;
		if ( (twi_status == TW_MT_SLA_NACK )||(twi_status ==TW_MR_DATA_NACK) )
		{
			/* device busy, send stop condition to terminate write operation */
			TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
			// wait until stop condition is executed and bus released
			while(TWCR0 & (1<<TWSTO));
			continue;
		}
		break;
	}
}

// Send one byte to twi device, Return 0 if write successful or 1 if write failed
unsigned char twi_write( unsigned char data ){
	// send data to the previously addressed device
	TWDR0 = data;
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	// wait until transmission completed
	while(!(TWCR0 & (1<<TWINT)));
	if( (TW_STATUS & 0xF8) != TW_MT_DATA_ACK) return 1;
	return 0;
}

// Send repeated start condition, address, transfer direction
//Return: 0 device accessible
//Return: 1 failed to access device
unsigned char twi_rep_start(unsigned char address){
	return twi_start( address );
}

// Terminates the data transfer and releases the twi bus
void twi_stop(void){
	// send stop condition
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	// wait until stop condition is executed and bus released
	while(TWCR0 & (1<<TWSTO));
}

void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value){
	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	twi_write(value);
	twi_stop();
}

uint8_t PCA9555_0_read(PCA9555_REGISTERS reg){
	uint8_t ret_val;
	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	// repeated start switches to read without releasing bus
	twi_rep_start(PCA9555_0_ADDRESS + TWI_READ);
	ret_val = twi_readNak();
	twi_stop();
	return ret_val;
}

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

void code_correct(){
	// light up the leds accordingly
	PORTB = 0b00111111;
	_delay_ms(3000);
	PORTB = 0;
	
	// delay 5sec total
	_delay_ms(2000);
}

void code_wrong(){
	// light up the leds accordingly
	for(int i=0; i<5; i++){
		PORTB = 0b00111111;
		_delay_ms(500);
		PORTB = 0;
		_delay_ms(500);
	}
	
	// last second of blinking needs to check for combinations
	finish_blinking();
}

void finish_blinking(){
	uint8_t digit = 0;
	uint16_t first;
	uint16_t second;
	char result;
	char key1;
	char key2;
	
	PORTB=0b00111111;
	for(int i=0; i<10; i++){
		if(digit==0){
			// searching for the first button press
			
			first = scan_keypad_rising_edge();	// 20msec
			result = keypad_to_ascii(first);
			
			// if no button was pressed, continue looping over i
			if(result == 0){
				_delay_ms(30);	// (20+30)msec
				continue;
			}
			
			// a button was pressed, update variables accordingly
			digit++;			// we took the first button press
			key1 = result;		// save the actual key pressed in "key1"
			_delay_ms(30);		// (20+30)msec
			continue;
		}
		else{
			// first button press has been detected
			// searching for the second one
			
			second = scan_keypad_rising_edge();		// 20msec
			result = keypad_to_ascii(second);
			
			// if no button was pressed, continue looping over i
			if(result == 0){
				_delay_ms(30);	// (20+30)msec
				continue;
			}
			
			// second button was pressed, update variables accordingly
			key2 = result;	// save the actual key pressed in "key2"
			if (key1 == '1' && key2 == '4'){
				// if the two buttons pressed are correct,
				// light leds accordingly
				code_correct();
				// after code_correct() returns, this function also needs to return
				return;
			}
			else{
				// wrong combination of keys, blink leds
				code_wrong();
				// after code_wrong() returns, this function also needs to return
				return;
			}
		}
	}
	
	PORTB=0b00000000;
	for(int i=0; i<10; i++){
		if(digit==0){
			// searching for the first button press
			
			first = scan_keypad_rising_edge();	// 20msec
			result = keypad_to_ascii(first);
			
			// if no button was pressed, continue looping over i
			if(result == 0){
				_delay_ms(30);	// (20+30)msec
				continue;
			}
			
			// a button was pressed, update variables accordingly
			digit++;			// we took the first button press
			key1 = result;		// save the actual key pressed in "key1"
			_delay_ms(30);		// (20+30)msec
			continue;
		}
		else{
			// first button press has been detected
			// searching for the second one
			
			second = scan_keypad_rising_edge();		// 20msec
			result = keypad_to_ascii(second);
			
			// if no button was pressed, continue looping over i
			if(result == 0){
				_delay_ms(30);	// (20+30)msec
				continue;
			}
			
			// second button was pressed, update variables accordingly
			key2 = result;	// save the actual key pressed in "key2"
			if (key1 == '1' && key2 == '4'){
				// if the two buttons pressed are correct,
				// light leds accordingly
				code_correct();
				// after code_correct() returns, this function also needs to return
				return;
			}
			else{
				// wrong combination of keys, blink leds
				code_wrong();
				// after code_wrong() returns, this function also needs to return
				return;
			}
		}
	}
}

int main(){
	// --------------------
	// code is 14
	// --------------------
	
	// initialize twi
	twi_init();
	// set columns as input, rows as output
	PCA9555_0_write(REG_CONFIGURATION_1, 0b11110000);
	
	// portB as output
	DDRB = 0xFF;
	PORTB = 0;	// start with leds off
	
	uint16_t scan;	// 16 bit variable: if a bit is 0, a key was pressed
	char result;	// translate the scan into a button if a button was pressed
	bool maybe = false;		// set if first button press is '1'
	
	while(1){
		// wait for the first button press
		while (result == 0){
			scan = scan_keypad_rising_edge();	// detect a NEW press
			result = keypad_to_ascii(scan);
		}
		// wait for button release (button press is a 0)
		while (scan_keypad()!=0xFFFF);
		
		// if first button was '1', set maybe flag
		if (result == '1') maybe = true;
		
		if (maybe){
			// flag is set, search for a '4'
			result = 0;
			while (result == 0){
				scan = scan_keypad_rising_edge();
				result = keypad_to_ascii(scan);
			}
			// wait for button release
			while (scan_keypad()!=0xFFFF);
			
			// check if second press is '4'
			// and display accordingly
			if (result == '4') code_correct();
			else code_wrong();
		}
		else {
			// maybe flag not set
			// just wait for the second press
			// and display code_wrong
			result = 0;
			while (result == 0){
				scan = scan_keypad_rising_edge();
				result = keypad_to_ascii(scan);
			}
			// wait for button release
			while (scan_keypad()!=0xFFFF);
			code_wrong();
		}
		
		// reset result and maybe flag
		result = 0;
		maybe = false;
	}
}