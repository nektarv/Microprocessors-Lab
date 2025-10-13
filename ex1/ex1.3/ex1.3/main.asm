.include "m328PBdef.inc"

.cseg
.org 0

.def temp = r16
.def leds = r17

.equ f_cpu = 16000000
.equ iterations = (f_cpu / 4000) - 3	; adjust "3" to best fit cycles in delays

init:
	; reg pairs for delays
	ldi r24, LOW(2000)
	ldi r25, HIGH(2000)
	ldi r26, LOW(1000)
	ldi r27, HIGH(1000)

	; set D as output
	ser temp
	out DDRD, temp

	; light up PD0
	ldi leds, 0x01
	bst leds, 0	; T=1 when moving to the left
	out PORTD, leds

	; we're on the edge, delay 3sec (1st iteration of program)
	; 1st delay will be inaccurate
	rcall delay_2s
	rcall delay_1s

main:	
	ldi temp, 0x07	; counter
	brts move_left	; if T=1, move to the left

move_right:
	lsr leds
	out PORTD, leds
	rcall delay_2s
	dec temp
	brne move_right
	; temp=0, reached the opposite edge
	rcall delay_1s
	rjmp main

move_left:
	lsl leds
	out PORTD, leds
	rcall delay_2s
	dec temp
	brne move_left
	; temp=0, reached the opposite edge
	rcall delay_1s
	rjmp main

delay_2s:
	ldi r28, LOW(iterations)
	ldi r29, HIGH(iterations)
	loop:
		sbiw r28, 1
		brne loop
	sbiw r24, 1
	breq last_2s
	; nop's
	brne delay_2s
last_2s:
	; nop's
	ret

delay_1s:
	ldi r28, LOW(iterations)
	ldi r29, HIGH(iterations)
	small_loop:
		sbiw r28, 1
		brne small_loop
	sbiw r26, 1
	breq last_1s
	; nop's
	brne delay_1s
last_1s:
	; reached edge, complement T flag
	ldi temp, 0b01000000
	in r18, SREG
	eor r18, temp
	out SREG, r18
	; nop's
	ret