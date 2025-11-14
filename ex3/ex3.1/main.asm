.include "m328pbdef.inc"

.equ f = 16
.equ delay = f*5

.def temp = r16
.def DC_VALUE = r17
.def index = r18

.org 0x0
	rjmp init

table:
	.db 5, 20, 36, 51, 66, 82, 97, 112, 128, 143, 158, 173, 189, 204, 219, 235, 250, 0

init:
	; stack pointer
	ldi temp, LOW(RAMEND)
	out SPL, temp
	ldi temp, HIGH(RAMEND)
	out SPH, temp

	; PB1 output, PB4,PB5 input
	ldi temp, 0b00000010
	out DDRB, temp
	ldi temp, 0b00110000
	out PORTB, temp

	; Fpwm = Fclk/N*(1+TOP)
	; where Fpwm=62500Hz, Fclk=16MHz, TOP=255
	; which means that N=1 (prescaler)

	; 1 only for CS10 -> prescaler=1
	; 1 for WGM12, WGM10 -> fast PWM, 8-bit
	; 1 for COM1A1 -> non-inverting mode
	ldi temp, (1<<COM1A1)|(1<<WGM10)
	sts TCCR1A, temp
	ldi temp, (1<<WGM12)|(1<<CS10)
	sts TCCR1B, temp

	; load base address of table into Z register
	; multiply by 2 because ldi and lpm expect a byte
	; address, whereas the label "table" is refers 
	; to a word address (2 bytes)
	ldi ZH, HIGH(table*2)
	ldi ZL, LOW(table*2)

	ldi index, 8	; start at middle element (9th)
	add ZL, index
	clr temp
	adc ZH, temp	; ZH + temp + C
	lpm DC_VALUE, Z

	; set OCR1A at 50% duty cycle
	clr temp
	sts OCR1AH, temp
	sts OCR1AL, DC_VALUE

; ------------------------------

main:
	; check if either pin is pressed
	sbis PINB, 4
	rjmp increase
	sbis PINB, 5
	rjmp decrease

	rjmp main

; ------------------------------

increase:
	; check if we've reached 98%
	cpi index, 16
	breq main

	debouncing_inc:
		ldi r24, LOW(delay)
		ldi r25, HIGH(delay)
		rcall delay_mS

		sbis PINB, 4
		rjmp debouncing_inc

	; update DC_VALUE
	inc index
	adiw ZL, 1
	lpm DC_VALUE, Z

	; update OCR1A
	sts OCR1AL, DC_VALUE

	rjmp main

; ------------------------------

decrease:
	; check if we've reached 2%
	tst index
	breq main

	debouncing_dec:
		ldi r24, LOW(delay)
		ldi r25, HIGH(delay)
		rcall delay_mS

		sbis PINB, 5
		rjmp debouncing_dec

	; update DC_VALUE
	dec index
	sbiw ZL, 1
	lpm DC_VALUE, Z

	; update OCR1A
	sts OCR1AL, DC_VALUE

	rjmp main

; ------------------------------

; delay routine
delay_mS:
    ldi r23, 249
loop_inn:
    dec r23
    nop
    brne loop_inn
    sbiw r24, 1
    brne delay_mS
    ret