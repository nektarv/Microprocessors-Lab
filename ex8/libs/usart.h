#ifndef _USART_
#define _USART_

#include "utils.h"
#include "lcd.h"

#define UBRR 103

void usart_init(unsigned int ubrr){
	UCSR0A=0;
	
	// enable receiving and transmiting data
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	
	// set BAUD rate
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	
	// set charachter size to 8 bits and asynchronous mode
	UCSR0C = (3 << UCSZ10);
	
	return;
}

void usart_transmit(uint8_t data){
	// make transmit register ready to receive data
	while(!(UCSR0A & (1 << UDRE0)));
	
	// write the data to the register
	UDR0 = data;
}

uint8_t usart_receive(){
	// set RXC1 to let the device know that there are data ready to be read
	while(!(UCSR0A & (1 << RXC0)));
	
	// return the received data
	return UDR0;
}

void esp_send_command(const char* string){
	usart_transmit('E');
	usart_transmit('S');
	usart_transmit('P');
	usart_transmit(':');
	
	for(int i=0; string[i]!='\0'; i++){
		usart_transmit(string[i]);
	}
	
	usart_transmit('\n');
}

void esp_receive_answer(char* esp_answer){
	esp_answer[0] = '\0';	// reset the array
	int index = 0;
	char c;
	
	while(1){
		c = usart_receive();
		if (c == '\n') break;
		esp_answer[index++] = c;
		if (index >= 64) break;
	}
	
	esp_answer[index] = '\0';
}

void esp_print_response(char cmd_count, char* esp_answer){
	lcd_data(cmd_count);
	lcd_data('.');
	
	if(strstr(esp_answer, "Success") != NULL){
		lcd_data('S');
		lcd_data('u');
		lcd_data('c');
		lcd_data('c');
		lcd_data('e');
		lcd_data('s');
		lcd_data('s');
	}
	else{
		lcd_data('F');
		lcd_data('a');
		lcd_data('i');
		lcd_data('l');
	}
}

#endif /*USART*/