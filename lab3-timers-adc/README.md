# Lab 3 - Timers, PWM & ADC

Fast-PWM and ADC exercises for the ATmega328PB, driving an LED and reading analog voltages.

## ex3.1 - PWM LED Dimming
Initialises Timer1 in 8-bit Fast PWM mode to generate a 62500 Hz waveform on PB1 whose duty cycle sets an LED's brightness. Duty cycle starts at 50% and steps up/down by 6% with buttons PB4/PB5, clamped between 2% and 98%. The OCR1A values for each duty-cycle step are precomputed and stored in a table so nothing is calculated at runtime.

## ex3.2 - ADC Average
Reimplements the PWM control in C and, every 100 ms, reads the DC voltage on the PB1_PWM analog filter with 10-bit precision. Sixteen successive ADC samples are averaged and the result lights one of the PORTD LEDs according to the measured band.