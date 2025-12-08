#include "utils.h"
#include "pca9555.h"
#include "usart.h"
#include "lcd.h"
#include "adc.h"
#include "ds18bs20.h"
#include "keypad.h"

// state variable for lcd functions
volatile uint8_t state = 0;

// is set when patient calls for nurse (4)
// is cleared when nurse resolves (#)
volatile bool nurse_call = false;
// is cleared when we have an active nurse call
// is set when nurse resolves (#)
volatile bool nurse_response = true;

void initialization();
void nurse_call_status();
const char* patient_status(float temp, float press);
void display_patient_measurements(float temp, float pressure);
void patient_report(float temp, float press);
void send_payload(float temp, float press);

int main(){
	initialization();
	// answer will receive ESP's answers
	char answer[64];
	
	// restart ESP
	// esp_send_command("restart");
	
	// connect to ESP
	esp_send_command("connect");
	esp_receive_answer(answer);
	esp_print_response('1', answer);
	patient_friendly_delay(1500);
	
	// send url to ESP
	esp_send_command("url:\"http://192.168.1.250:5000/data\"");
	esp_receive_answer(answer);
	esp_print_response('2', answer);
	patient_friendly_delay(1500);
	
	// start displaying patient status
	while(1){
		// take temperature
		float real_temp = read_temp() * 0.0625;
		float patient_temp = real_temp + 12.0;
		// take pressure
		float patient_press = read_pressure();

		// display patient report
		patient_report(patient_temp, patient_press);
		patient_friendly_delay(3000);
	
		// send payload
		send_payload(patient_temp, patient_press);	// calls esp_send_command() at the end
		esp_receive_answer(answer);
		esp_print_response('3', answer);
		patient_friendly_delay(1500);
		
		// transmit
		esp_send_command("transmit");
		esp_receive_answer(answer);
		lcd_data('4');
		lcd_data('.');
		for(int i=0; answer[i]!='\0'; i++){
			if(answer[i] == '\r') continue;
			lcd_data(answer[i]);
		}
		patient_friendly_delay(1500);
	}
}

void initialization(){
	twi_init();
	_delay_ms(100);
	
	PCA9555_0_write(REG_CONFIGURATION_1, 0b11110000);
	_delay_ms(100);
	
	one_wire_reset();
	_delay_ms(100);
	
	lcd_init();
	_delay_ms(100);
	
	usart_init(UBRR);
	_delay_ms(100);
	
	ADMUX = (1<<REFS0)|(0<<MUX0);
	ADCSRA = (1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	_delay_ms(100);
	
	lcd_clear_display();	// preserving mental health
	
	return;
}

void patient_friendly_delay(int delay){
	// delay that allows for the keypad to be pressed
	// during payload or transmit parts
	
	int iter = delay / 10;
	for(int i=0; i<iter; i++){
		_delay_ms(10);
		keypad_sanity_check();
	}
	
	// this delay is always called after printing
	// clear lcd here for a more readable main
	lcd_clear_display();
}

void keypad_sanity_check(){
	// created to check whether keypad is pressed
	// during payload or transmit parts
	
	if(!nurse_call){
		if (keypad_to_ascii(scan_keypad()) == '4'){
			nurse_call = true;
			nurse_response = false;
		}
	}
	
	if(nurse_call){
		if (keypad_to_ascii(scan_keypad()) == '#'){
			nurse_call = false;
			nurse_response = true;
		}
	}	
}

void nurse_call_status(){
	// if we have an active nurse call, return
	if (!nurse_response) return;
	
	// if we don't have an active nurse call
	// check if the patient requests for a nurse
	// set flags accordingly
	if (keypad_to_ascii(scan_keypad()) == '4'){
		nurse_call = true;
		nurse_response = false;
	}
}

const char* patient_status(float temp, float press){
	// returns appropriate patient status
	if (nurse_call) return "NURSE CALL";
	if (press<4.0 || press>12.0) return "CHECK PRESSURE";
	if (temp<34.0 || temp>37.0) return "CHECK TEMP";
	return "OK";
}

void display_patient_measurements(float temp, float pressure){
	// print temperature
	char temp_buf[7];
	sprintf(temp_buf, "%.1f", temp);
	for(int i=0; temp_buf[i]; i++) lcd_data(temp_buf[i]);
	lcd_data(0b11011111);
	lcd_data('C');
	
	// leave empty space in between
	lcd_data(' ');
	
	// print pressure
	char press_buf[7];
	sprintf(press_buf, "%.1f", pressure);
	for(int i=0; press_buf[i]; i++) lcd_data(press_buf[i]);
	lcd_data('c');
	lcd_data('m');
	lcd_data('H');
	lcd_data('2');
	lcd_data('O');
}

void patient_report(float temp, float press){
	// update patient status
	nurse_call_status();
	const char* status = patient_status(temp, press);
	
	// display temperature and pressure
	display_patient_measurements(temp, press);
	// new line
	lcd_command(0xC0);
	// display patient status
	for(int i=0; status[i]!='\0'; i++) lcd_data(status[i]);
	
	// if '#' is pressed, reset nurse_call flags
	if(keypad_to_ascii(scan_keypad()) == '#'){
		nurse_response = true;
		nurse_call = false;
	}
}

void send_payload(float patient_temp, float patient_press){
	// payload params
	const char* payload_status = patient_status(patient_temp, patient_press);
	int payload_temp_integer = (int)patient_temp;
	int payload_temp_decimal = (int)((patient_temp - payload_temp_integer) * 100);
	int payload_press_integer = (int)patient_press;
	int payload_press_decimal = (int)((patient_press - payload_press_integer) * 100);
	
	// build payload
	char payload[256] = {0};
	snprintf(payload, sizeof(payload),
			"payload:[{\"name\":\"team\",\"value\":\"14\"},"
			"{\"name\":\"temperature\",\"value\":\"%d.%d\"},"
			"{\"name\":\"pressure\",\"value\":\"%d.%d\"},"
			"{\"name\":\"status\",\"value\":\"%s\"}]",
			payload_temp_integer, payload_temp_decimal,
			payload_press_integer, payload_press_decimal,
			payload_status);
	
	// send payload
	esp_send_command(payload);
}