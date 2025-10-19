.include "m328PBdef.inc"

.equ FOSC_MHZ = 16				; frequency
.equ DEL_mS = 1000				; delay in msec
.equ DEL_NU = FOSC_MHZ * DEL_mS	; delay_mS routine: 1000*DEL_NU + 6 cycles

.def temp = r16

init:
	; stack pointer
	ldi r24, LOW(RAMEND)
	out SPL, r24
	ldi r24, HIGH(RAMEND)
	out SPH, r24

	; PORTC as output
	ser r26
	out DDRC, r26

; counting routine
loop1:
	clr r26
loop2:
	; shift r26 so it displays on PC1-PC5
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