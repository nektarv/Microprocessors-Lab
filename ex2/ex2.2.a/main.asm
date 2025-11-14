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
	; r26 holds the current number in the first 5 bits
	; we move it to another register and perform
	; a left logical shift, so we can print the
	; current number in PC1-PC5
	mov temp, r26
	lsl temp
	out PORTC, temp

	ldi r24, low(DEL_NU)
	ldi r25, high(DEL_NU)
	rcall delay_mS

	inc r26
	cpi r26, 32	; count up to 31
	breq loop1	; if r26-32=0, branch to loop1 to clear it
	rjmp loop2	; keep counting

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