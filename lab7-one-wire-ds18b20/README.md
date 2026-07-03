# Lab 7 - DS18B20 Temperature Sensor (1-Wire)

C exercises for the ATmega328PB reading the DS1820/DS18B20 temperature sensor over the 1-wire protocol on PD4.

## ex7.1 - DS18B20 Functions
Mirrors the supplied assembly 1-wire routines in C to reset the bus, skip ROM (0xCC), start a conversion (0x44), wait for completion, and read the scratchpad (0xBE). It returns the raw 16-bit two's-complement temperature in the r25:r24 pair, or 0x8000 when no sensor is connected.

## ex7.2 - Temperature Measurement
Uses the ex7.1 routine to show the temperature in °C on the LCD as a signed three-digit value (-55°C to +125°C), printing "No Device" when no sensor is present.
