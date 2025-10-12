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
	ldi a, 0x52
	ldi b, 0x42
	ldi c, 0x22
	ldi d, 0x02
	ldi counter, 0x06

loop:
	mov F0, a
	com F0
	and F0, b
	mov temp, b
	com temp
	and temp, d
	or f0, temp
	com f0

	mov f1, a
	or f1, c
	mov temp, b
	or temp, d
	and f1, temp

	inc a
	subi b, -0x02
	subi c, -0x03
	subi d, -0x04

	dec counter
	brne loop

end:
	rjmp end