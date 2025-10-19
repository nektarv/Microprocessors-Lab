.include "m328PBdef.inc"

.org 0x0
rjmp init
.org 0x2
rjmp ISR0

.equ FOSC_MHZ = 1				; frequency
.equ DEL_mS = 1000				; delay in msec
.equ DEL_NU = FOSC_MHZ * DEL_mS	; delay_mS routine: 1000*DEL_NU + 6 cycles

.def temp = r16
.def counter = r17

init:
	; stack pointer
	ldi r24, LOW(RAMEND)
	out SPL, r24
	ldi r24, HIGH(RAMEND)
	out SPH, r24

	; interrupt on falling edge of INT0
	ldi r24, (1 << ISC01)
	sts EICRA, r24

	; enable INT0
	ldi r24, (1 << INT0)
	out EIMSK, r24

	; PORTC as output
	ser r26
	out DDRC, r26

	; PD2 ready
	cbi DDRD, 2
	sbi PORTD, 2

	; enable interrupts
	sei

; counting routine
loop1:
	clr r26
loop2:
	mov temp, r26
	lsl temp
	out PORTC, temp

	ldi r24, low(DEL_NU)
	ldi r25, high(DEL_NU)
	rcall delay_mS

	inc r26

	cpi r26, 32
	breq loop1
	rjmp loop2

; delay routine
delay_mS:
	ldi r23, 249
loop:
	dec r23
	nop
	brne loop

	sbiw r24, 1
	brne delay_mS

	ret

ISR0:
	push r18
	
	; place PORTB in r18 and keep only PB1-PB4
	in r18, PINB
	andi r18, 0b00011110

	rcall count

	out PORTC, r18

	pop r18
	reti

; amount of PINB buttons pressed
count:
	clr counter

	; increase counter only if PBx was pressed
	sbrc r18, 1
	inc counter
	sbrc r18, 2
	inc counter
	sbrc r18, 3
	inc counter
	sbrc r18, 4
	inc counter

	; -------------------
	; counter = 0
	; r18 = 0000
	; -------------------
	; counter = 1
	; r18 = 0001
	; -------------------
	; counter = 2
	; r18 = 0011
	; -------------------
	; counter = 3
	; r18 = 0111
	; -------------------
	; counter = 4
	; r18 = 1111
	; -------------------

; prep r18 to be printed
	clr r18
mask:
	; if counter=0, the mask is ready
	tst counter
	breq ready
	; if counter>0, decrease it
	dec counter
	; add another "1" to r18
	lsl r18		; shift LSB to the left
	ori r18, 1	; add 1 at new LSB
	rjmp mask
ready:
	ret