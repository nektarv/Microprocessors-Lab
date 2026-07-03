# Lab 4 - 2×16 Character LCD

ADC-to-LCD exercises for the ATmega328PB using a 2×16 alphanumeric LCD in 4-bit mode.

## ex4.1 - ADC Voltage Meter
Starts an ADC conversion every second on analog input A3, reading the result inside the ADC conversion-complete interrupt service routine. The value is converted to a voltage (VREF = 5V) and printed on the LCD to two decimal places, starting at the first character of the first line.

## ex4.3 - CO Sensor
Reads a CO sensor on analog input A3 every 100 ms and shows the gas level on LEDs PB0-PB5.
When the concentration exceeds 75 ppm the LCD shows GAS DETECTED and the level LEDs blink while once it drops back below 75 ppm the LEDs stop blinking and the LCD shows CLEAR.
