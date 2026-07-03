# Lab 1 - Programmable Delays & Logic Functions

AVR assembly exercises for the ATmega328PB, developed and simulated in Microchip Studio.

## ex1.1 - Programmable Delay Routine
Implements a `wait_x_msec` routine that produces an adjustable delay of x milliseconds, where x (1-65535) is held in the r25:r24 register pair. The timing loop uses the `sbiw` instruction to decrement the pair, and the routine is embedded in a complete program whose timing is verified with the Microchip Studio Stopwatch.

## ex1.2 - Logical Functions
Computes F0 = (A'·B + B'·D)' and F1 = (A+C)·(B+D) inside a loop that runs six times. On each pass the inputs A, B, C, D (starting at 0x52, 0x42, 0x22, 0x02) are incremented by 0x01, 0x02, 0x03 and 0x04 respectively, and the results are recorded.

## ex1.3 - LED Wagon
Simulates a wagon as a single lit bit on PORTD that travels from the LSB to the MSB and back. The bit advances roughly every 2 seconds, the direction of travel is stored in the SREG T flag, and the wagon pauses an extra second at each end. Delays reuse the routine from ex1.1.
