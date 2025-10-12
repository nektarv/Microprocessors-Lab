; subroutine that delays "delay" msec for "f_cpu" CPU frequency
; between break point 1 and break point 2, we have 4 extra cycles
; from the nop and rcall commands

.include "m328PBdef.inc"

.cseg
.org 0

.equ delay = 1000
.equ f_cpu = 16000000
.equ iterations = (f_cpu / 4000) - 3

init:
	ldi r26, LOW(RAMEND)
	out SPL, r26
	ldi r26, HIGH(RAMEND)
	out SPH, r26

	ldi r24, LOW(delay)
	ldi r25, HIGH(delay)
	nop ; break point 1
	rcall wait_x_msec
	nop ; break point 2

wait_x_msec:
	ldi r26, LOW(iterations) ; 1
	ldi r27, HIGH(iterations) ; 1
	loop:
		sbiw r26, 1 ; 2
		brne loop ; 2/1
	sbiw r24, 1 ; 2
	breq last ; 1/2
	nop
	nop
	nop ; 3
	nop
	nop
	nop ; 6
	brne wait_x_msec ; 2/1

last:
	nop
	nop
	nop
	ret ; 4