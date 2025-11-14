#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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

void print_names() {
	lcd_clear_display();
	lcd_data('f');
	lcd_data('n');
	lcd_data('1');
	lcd_command(0xC0);
	lcd_data('l');
	lcd_data('n');
	lcd_data('1');
	_delay_ms(2000);
	
	lcd_clear_display();
	lcd_data('f');
	lcd_data('n');
	lcd_data('2');
	lcd_command(0xC0);
	lcd_data('l');
	lcd_data('n');
	lcd_data('2');
	_delay_ms(2000);
}

int main() {
	// initialize twi
	twi_init();
	// clear configuration bits to make PORT0 output
	PCA9555_0_write(REG_CONFIGURATION_0, 0x00);
	
	// initialize lcd and print
	lcd_init();
	while(1){
		print_names();
	}
}