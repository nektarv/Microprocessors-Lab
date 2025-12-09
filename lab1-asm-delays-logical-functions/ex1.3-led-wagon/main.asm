; program that simulates a wagon moving through
; the leds of port D, stalling approximately 
; 2sec at each stop and 3sec at each edge

; T flag is manipulated at "last_1s"

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
	ldi leds, 0x01
	out PORTD, leds

	; we're on the edge, delay 3sec (1st iteration of program)
	rcall delay_2s
	rcall delay_1s
	bst leds, 0	; T=leds(0)=1 when moving left

main:	
	ldi temp, 0x07	; counter

	; we branch to the "move_left" routine if T=1
	; otherwise, we move on to "move_right" (T=0)
	brts move_left

move_right:
    lsr leds		; light the next led by shifting the "1" accordingly
	out PORTD, leds	; print
	rcall delay_2s	; delay 2s
	dec temp		; decrease counter
	brne move_right

	; if temp=0, we reached the opposite edge
	rcall delay_1s	; delay another 1s
	rjmp main

move_left:
    lsl leds		; light the next led by shifting the "1" accordingly
	out PORTD, leds	; print
	rcall delay_2s	; delay 2s
	dec temp		; decrease counter
	brne move_left

	; if temp=0, we reached the opposite edge
	rcall delay_1s	; delay another 1s
	rjmp main

; ----------------------------------------------
; delay routines from ex1.1
; ----------------------------------------------
delay_2s:
	ldi r24, LOW(2000)	; 2000 msec
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
	ldi r24, LOW(1000)	;  1000 msec
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

	; exclusive or between the SREG values and 0b01000000
	; will flip the T flag and leave other bits unchanged
	;-----------------
	; 0eorX = X
	; 1eorX = not X
	;-----------------

	ldi temp, 0b01000000
	in r18, SREG
	eor r18, temp
	out SREG, r18
	ret