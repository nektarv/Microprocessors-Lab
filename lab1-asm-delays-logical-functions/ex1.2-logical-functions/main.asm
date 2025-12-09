.include "m328PBdef.inc"

.def a = r16
.def b = r17
.def c = r18
.def d = r19
.def temp = r20
.def counter = r21
.def F0 = r22
.def F1 = r23

.cseg
.org 0

start:
	; set D as output to print F0 and F1
	ser temp
	out DDRD, temp

	; initialize
	ldi a, 0x52
	ldi b, 0x42
	ldi c, 0x22
	ldi d, 0x02
	ldi counter, 0x06

loop:
	; compute F0
	mov F0, a		; F0 = a
	com F0			; F0 = nota
	and F0, b		; F0 = nota AND b
	mov temp, b		; temp = b
	com temp		; temp = notb
	and temp, d		; temp = notb AND d
	or f0, temp		; F0 = (nota AND b) OR (notb AND d)
	com f0			; F0 = not((nota AND b) OR (notb AND d))

	; print F0
	out PORTD, f0

	; compute F1
	mov f1, a		; F1 = a
	or f1, c		; F1 = a OR c
	mov temp, b		; temp = b
	or temp, d		; temp = b OR d
	and f1, temp	; F1 = (a OR c) AND (b OR d)

	; print F1
	out PORTD, f1

	; we increase b by 0x02 by subtracting -0x02
	; we do this, because there's no "addi" command
	; between a register and a constant
	; same goes for c and d
	inc a
	subi b, -0x02
	subi c, -0x03
	subi d, -0x04

	; if counter=0, we've completed 6 iterations, go to "end"
	dec counter
	brne loop

end:
	rjmp end