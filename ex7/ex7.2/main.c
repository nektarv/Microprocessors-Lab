#define F_CPU 16000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<avr/cpufunc.h>
#include<stdio.h>
#include<stdbool.h>

#define PCA9555_0_ADDRESS 0x40
#define TWI_READ    1
#define TWI_WRITE   0
#define SCL_CLOCK  100000L

#define TWBR0_VALUE ((F_CPU/SCL_CLOCK)-16)/2

#define PD0 0
#define PD1 1
#define PD2 2
#define PD3 3

volatile uint8_t state = 0;

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

#define TW_STATUS_MASK  0b11111000
#define TW_STATUS (TWSR0 & TW_STATUS_MASK)

#define clear(register, bit) (register &= ~(1 << bit))
#define set(register, bit) (register |= (1 << bit))

//initialize TWI clock
void twi_init(void)
{
	TWSR0 = 0;
	TWBR0 = TWBR0_VALUE;
}

// Read one byte from the twi device (request more data from device)
unsigned char twi_readAck(void)
{
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

//Read one byte from the twi device, read is followed by a stop condition
unsigned char twi_readNak(void)
{
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR0 & (1<<TWINT)));
	return TWDR0;
}

// Issues a start condition and sends address and transfer direction.
// return 0 = device accessible, 1= failed to access device
unsigned char twi_start(unsigned char address){
	
	uint8_t   twi_status;
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
	if ( (twi_status != TW_MT_SLA_ACK) && (twi_status != TW_MR_SLA_ACK) )
	{
		return 1;
	}
	
	return 0;
	
}

// Send start condition, address, transfer direction.
// Use ack polling to wait until device is ready
void twi_start_wait(unsigned char address)
{
	uint8_t   twi_status;
	
	while ( 1 )
	{
		// send START condition
		TWCR0 = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		
		// wait until transmission completed
		while(!(TWCR0 & (1<<TWINT)));
		
		// check value of TWI Status Register.
		twi_status = TW_STATUS & 0xF8;
		if ( (twi_status != TW_START) && (twi_status != TW_REP_START)) continue;
		
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

//  Send one byte to twi device,  Return 0 if write successful or 1 if write failed
unsigned char twi_write( unsigned char data )
{
	// send data to the previously addressed device
	TWDR0 = data;
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	// wait until transmission completed
	while(!(TWCR0 & (1<<TWINT)));
	if( (TW_STATUS & 0xF8) != TW_MT_DATA_ACK) return 1;
	return 0;
}

// Send repeated start condition, address, transfer direction
//Return:  0 device accessible
//1 failed to access device
unsigned char twi_rep_start(unsigned char address)
{
	return twi_start( address );
}

// Terminates the data transfer and releases the twi bus
void twi_stop(void)
{
	// send stop condition
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	// wait until stop condition is executed and bus released
	while(TWCR0 & (1<<TWSTO));
}

void PCA9555_0_write(PCA9555_REGISTERS reg, uint8_t value)
{
	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	twi_write(value);
	twi_stop();
}

uint8_t PCA9555_0_read(PCA9555_REGISTERS reg)
{
	uint8_t ret_val;
	twi_start_wait(PCA9555_0_ADDRESS + TWI_WRITE);
	twi_write(reg);
	twi_rep_start(PCA9555_0_ADDRESS + TWI_READ);
	ret_val = twi_readNak();
	twi_stop();
	return ret_val;
}

// ------ LCD functions using PCA ------
void write_2_nibbles(uint8_t data)
{
	uint8_t temp;
	uint8_t state;

	temp = PCA9555_0_read(REG_OUTPUT_0) & 0x0F;
	state = temp | (data & 0xF0);
	PCA9555_0_write(REG_OUTPUT_0, state);

	state |= (1 << PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_ms(1);
	state &= ~(1 << PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);

	temp = PCA9555_0_read(REG_OUTPUT_0) & 0x0F;
	state = temp | ((data << 4) & 0xF0);
	PCA9555_0_write(REG_OUTPUT_0, state);

	state |= (1 << PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_ms(1);
	state &= ~(1 << PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
}

void lcd_command(uint8_t command){
	state &= ~(1<<PD2);
	PCA9555_0_write(REG_OUTPUT_0, state);
	write_2_nibbles(command);
	_delay_us(250);
}

void lcd_data(uint8_t data){
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
	uint8_t state;
	
	for (int i=0; i<3; i++){
		state = 0x30;
		PCA9555_0_write(REG_OUTPUT_0, state);
		state |= (1<<PD3);
		PCA9555_0_write(REG_OUTPUT_0, state);
		_delay_ms(1);
		state &= ~(1<<PD3);
		PCA9555_0_write(REG_OUTPUT_0, state);
		_delay_us(250);
	}
	
	state = 0x20;
	PCA9555_0_write(REG_OUTPUT_0, state);
	state |= (1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_ms(1);
	state &= ~(1<<PD3);
	PCA9555_0_write(REG_OUTPUT_0, state);
	_delay_us(250);
	
	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_clear_display();
	lcd_command(0x06);
}

// ------ 7.1 functions ------

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

int main(){
	// initialize twi
	twi_init();
	// initialize lcd
	lcd_init();

	while(1){
		// reset cursor
		lcd_command(0x80);
		
		// take measurement
		int16_t temp = read_temp();
		
		// check if thermometer is missing
		if(temp == 0x8000) {
			char string[] = "NO DEVICE";
			for(int i=0; string[i]; i++) lcd_data(string[i]);
		}
		else {
			float celsius = temp * 0.0625;
			
			// buffer that will hold measurement with sign and comma
			char buf[8];  // sign(1), integer part(3), comma(1), decimal part(3)
			
			// format buf to display the sign of "celsius"
			// and 3 decimal digits
			sprintf(buf, "%+.3f", celsius);
			for(int i=0; buf[i]; i++){
				lcd_data(buf[i]);
			}
			
			// display celsius symbol
			lcd_data(0b11011111);
			lcd_data('C');
		}
	}
}