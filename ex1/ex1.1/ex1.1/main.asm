; subroutine that delays "delay" msec for "f_cpu" CPU frequency
; x=delay ranges from 0 to 65535

; if we want the routine to delay for x msec, we leave the program as is

; if we want two routine calls to be x msec apart (including the lighting of the leds),
; we decrease the iterations by 1 and add 4 nop commands in the outer cycle

.include "m328PBdef.inc"

.cseg
.org 0

.equ delay = 100
.equ f_cpu = 1000000
.equ iterations = (f_cpu / 4000) - 3

init:
	; initialize stack pointer to start
	; at the end address of RAM
	ldi r26, LOW(RAMEND)
	out SPL, r26
	ldi r26, HIGH(RAMEND)
	out SPH, r26

	; set D as output
	ser r16
    out DDRD, r16
	out PORTD, r16

main:
	rcall wait_x_msec

	; light up D leds for visualization
	com r16
	out PORTD, r16
	rjmp main

; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------
; reminder: iterations = (f_cpu / 4000) - 3
; ---------------------------------------------------------------------------
; we want each outer loop to delay for 1msec, which means
; it has to add up to f_cpu/1000 cycles
; by the end of the routine, we will have spent delay*(f_cpu/1000) cycles
; ---------------------------------------------------------------------------
; the outer loop is executed "delay" times
; ---------------------------------------------------------------------------
; for the first "delay-1" iterations, the inner loops combined add
; (delay-1)*(4*(iterations-1)+3) cycles which is (delay-1)*(f_cpu/1000 - 13) total cycles

; we need an extra 13 cycles per iteration for a total of f_cpu/1000.
; from each outer loop, we gain 2 cycles from the ldi commands,
; 2 cycles from sbiw, 1 from breq, 6 from nop's and 2 from brne
; which adds up to 13 per outer loop
; we've shown that each of the (delay-1) first iterations add f_cpu/1000 cycles
; ---------------------------------------------------------------------------
; for the last iteration of the outer loop, the breq command branches to the "last"
; routine (r25:r24=0), which means that for this iteration we have:
; 4*(iterations-1)+3 cycles from the inner loop, which is f_cpu/1000 - 13
; in the outer loop, we gain 2 cycles from the ldi commands, 2 from sbiw and 2 from breq
; additionally, when we enter the "last" routine, we add an extra 5 cycles from nop and ret

; we have reached f_cpu/1000 - 2 cycles
; the two "missing" cycles come from when we initially called "wait_x_msec", 
; when we loaded the delay in the r25:r24 pair
; ---------------------------------------------------------------------------
; ## CONCLUSION ## 
; the routine delays exactly delay*(f_cpu/1000) cycles
; for f_cpu=16MHz, that is 16000*delay, or 16000*x, cycles
; which is equivalent to x msec
; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------

wait_x_msec:
	; load x in the r25:r24 reg pair
	ldi r24, LOW(delay)	; 1 clock cycle
	ldi r25, HIGH(delay)
outer:
	; load iterations in the r27:r26 reg pair
	ldi r26, LOW(iterations)
	ldi r27, HIGH(iterations)
	inner:
		sbiw r26, 1 ; 2 cycles
		brne inner ; 2 cycles if we branch, 1 if we don't
	sbiw r24, 1
	breq last	; if this is the last msec, branch
	nop	; 1 cycle
	nop
	nop ; 3
	nop
	nop
	nop ; 6
	brne outer	; if this isn't the last msec, do it again
last:
	nop
	ret ; 4