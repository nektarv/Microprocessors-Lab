# Lab 8 - UART & IoT Patient Monitoring (ESP8266)

A C program for the ATmega328PB that turns the board into an IoT patient-monitoring node, talking to an ESP8266 Wi-Fi module over UART. It is built on driver headers reused from earlier labs (`libs/`): TWI/PCA9555, LCD, ADC, DS18B20 1-wire, keypad, and USART.

On startup the node sends the ESP the connect and url commands (setting the endpoint to http://192.168.1.250:5000/data), reads each reply, and reports progress on the LCD (1.Success/1.Fail, then 2.Success/2.Fail). It then loops continuously:

- Reads the DS18B20 temperature and adds a fixed 12 °C offset so the reading sits near a realistic patient's body temperature.
- Reads a potentiometer on the ADC and scales it to a 0-20 cmH₂O central-venous-pressure reading.
- Derives a status field: `NURSE CALL` when the patient presses keypad key "4" (cleared when a nurse presses "#"), otherwise `CHECK PRESSURE` if pressure is outside 4-12 cmH₂O, `CHECK TEMP` if temperature is outside 34-37 °C, or `OK`. The nurse call takes priority over the threshold checks.

The temperature and pressure are shown on the first LCD line and the status on the second. The measurements and status are packed into a JSON payload (with the lab team number) and sent via the payload and transmit commands; the payload step reports 3.Success/3.Fail and the server's response is printed as "4. *server response*" before the cycle repeats. Keypad presses are polled during the inter-step delays so a nurse call can be registered at any time.
