; before running, make sure to comment out the relevant
; parts in the sections that refer to the LEDs 
; lighting up on either HIGH or LOW

.include "m328PBdef.inc"

.cseg
.org 0

.def temp = r16
.def leds = r17

.equ f_cpu = 16000000
.equ iterations = (f_cpu / 4000) - 3

init:
	; set D as output
	ser temp
	out DDRD, temp

	; light up PD0
	; if LED lights on HIGH
	ldi leds, 0x01
	; if LED lights on LOW
	ldi leds, 0xFE

	out PORTD, leds

	; we're on the edge, delay 3sec (1st iteration of program)
	rcall delay_2s
	rcall delay_1s
	bst leds, 0	; T=leds(0) when moving left

main:	
	ldi temp, 0x07	; counter
	; if LED lights on HIGH
	brts move_left	; if T=1, move to the left
	; if LED lights on LOW
	brtc move_left	; if T=0, move to the left

move_right:
	; if LED lights on HIGH
    lsr leds
	; if LED lights on LOW
	sec
	ror leds

	out PORTD, leds
	rcall delay_2s
	dec temp
	brne move_right
	; temp=0, reached the opposite edge
	rcall delay_1s
	rjmp main

move_left:
	; if LED lights on HIGH
    lsl leds
	; if LED lights on LOW
	sec
	rol leds

	out PORTD, leds
	rcall delay_2s
	dec temp
	brne move_left
	; temp=0, reached the opposite edge
	rcall delay_1s
	rjmp main

delay_2s:
	ldi r24, LOW(2000)
	ldi r25, HIGH(2000)
outer_2s:
	ldi r26, LOW(iterations)
	ldi r27, HIGH(iterations)
	inner_2s:
		sbiw r26, 1
		brne inner_2s
	sbiw r24, 1
	breq last_2s
	nop
	nop
	nop
	nop
	nop
	nop
	brne outer_2s
last_2s:
	ret

delay_1s:
	ldi r24, LOW(1000)
	ldi r25, HIGH(1000)
outer_1s:
	ldi r26, LOW(iterations)
	ldi r27, HIGH(iterations)
	inner_1s:
		sbiw r26, 1
		brne inner_1s
	sbiw r24, 1
	breq last_1s
	nop
	nop
	nop
	nop
	nop
	nop
	brne outer_1s
last_1s:
	; reached edge, complement T flag
	ldi temp, 0b01000000
	in r18, SREG
	eor r18, temp
	out SREG, r18
	ret