# Lab 2 - External Interrupts

AVR exercises using the ATmega328PB external interrupts and timers.

## ex2.2.a - Counting
Implements a counter running 0-31 displayed in binary on LEDs PC5-PC1, with roughly 1000msec between successive counts.

## ex2.2.b - INT0
Modifies the INT0 (PD2) interrupt service routine so that, when triggered, it lights as many PORTC LEDs as there are pressed buttons among PB4-PB1, starting from the LSB.

## ex2.3 - Light Switch
Controls a lamp (LED PB3) that turns on when PD3 is pressed (INT1) and switches off after 4 seconds, unless another press refreshes the 4-second window. Each refresh briefly lights LEDs PB5-PB0 for 1 second. Implemented in C.
