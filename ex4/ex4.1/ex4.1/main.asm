.include "m328PBdef.inc"

; initialize delay routine parameters
.equ f_cpu = 16000000
.equ iterations = (f_cpu / 4000) - 3

.equ PD0=0 
.equ PD1=1 
.equ PD2=2 
.equ PD3=3 
.equ PD4=4 
.equ PD5=5 
.equ PD6=6 
.equ PD7=7

.def temp = r16

.org 0x00
	rjmp init
.org 0x2A
	rjmp adc_isr

adc_isr:
	; save temp's value for when we return from interrupt to main
	push temp

	rcall lcd_clear_display

	; load ADC reading into Z
	lds ZL, ADCL
	lds ZH, ADCH

	; we will multiply by 500 and divide by 1024
	; this will give us Vin*100

	; in order to multiply by 500,
	; we need to do a 16bit x 16bit multiplication
	; let's say we have AB and CD (where each letter is 8bits)
	; then, we have AB = A*256 + B, CD = C*256 + D
	; when we multiply AB*CD, we get:
	; AB*CD = B*D + B*C*256 + A*D*256 + A*C*256^2

	; final result will be stored in r19:r18
	rcall multiply	; Vin*100

	; r19:r18 now hold Vin*100
	; if the result is 250, we need to print 2.50
	; which means that we need to count the 100s, 10s and 1s
	; in the number we calculated
	rcall count_digits

	; print on lcd screen
	ldi temp, 48	; numbers are offset by 48 on the lcd screen

	; r20 holds the integer part
	mov r24, r20
	add r24, temp
	rcall lcd_data

	ldi r24, '.'
	rcall lcd_data

	; r21 holds the decimal tens part
	mov r24, r21
	add r24, temp
	rcall lcd_data

	; r22 holds the decimal units part
	mov r24, r22
	add r24, temp
	rcall lcd_data

	ldi r24, 'V'
	rcall lcd_data

	; 1sec delay
	ldi r24, low(1000)
	ldi r25, high(1000)
	rcall wait_msec

	; restore temp and return
	pop temp
	reti

init:
	; stack pointer
	ldi temp, LOW(RAMEND)
	out SPL, temp
	ldi temp, HIGH(RAMEND)
	out SPH, temp

	; setup D as output (lcd needs this)
	ser temp
	out DDRD, temp

	; setup ADC3
	cbi DDRC, 3
	cbi PORTC, 3

	; setup ADC registers
	ldi temp, (1<<REFS0)|(1<<MUX1)|(1<<MUX0)
	sts ADMUX, temp
	ldi temp, (1<<ADEN)|(1<<ADIE)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2)
	sts ADCSRA, temp

	; initialize lcd
	rcall lcd_init

	; enable global interrupts
	sei

main:
	; enable conversion
    lds temp, ADCSRA
    ori temp, (1 << ADSC)
    sts ADCSRA, temp
    rjmp main

; routine to multiply ADC measurement by 500 and divide by 1024
multiply:
	clr temp
	; multiplication result is stored in r21:r20:r19:r18
	clr r18
	clr r19
	clr r20
	clr r21

	; multiply by 500
	ldi r22, low(500)
	ldi r23, high(500)

	; low byte of adc with low byte of 500
	mul ZL, r22	; stored in r1:r0
	movw r18, r0

	; low byte of adc with high byte of 500
	mul ZL, r23
	add r19, r0
	adc r20, r1
	adc r21, temp

	; high byte of adc with low byte of 500
	mul ZH, r22
	add r19, r0
	adc r20, r1
	adc r21, temp

	; high byte of adc with high byte of 500
	mul ZH, r23
	add r20, r0
	adc r21, r1

	ldi temp, 10	; divide by 1024 -> shift right 10 times

divide:
	; shift 1 bit to the right accross 4 registers
	lsr r21
	ror r20
	ror r19
	ror r18

	; do it 10 times
	dec temp
	tst temp
	brne divide

	ret

; routine to translate Vin*100 into Vin
count_digits:
	movw YL, r18	; move the word in a reg pair that works with sbiw/adiw
	clr r20		; integer part counter
	clr r21		; decimal tens part counter
	clr r22		; decimal units part counter
integer_part:
	sbiw YL, 50	; sbiw only works with constants up to 63
	sbiw YL, 50
	brmi end_integer_part	; if result is negative, integer part is over
	inc r20
	rjmp integer_part
end_integer_part:
	adiw YL, 50	; restore last 100
	adiw YL, 50
decimal_tens_part:
	sbiw YL, 10
	brmi end_tens_part		; if result is negative, decimal tens part is over
	inc r21
	rjmp decimal_tens_part
end_tens_part:
	adiw YL, 10	; restore last 10
decimal_units_part:
	sbiw YL, 1
	brmi end_counting		; if result is negative, decimal units part is over
	inc r22
	rjmp decimal_units_part
end_counting:
	adiw YL, 1	; restore last 1
	ret

; ----------------------------------------------------------------
; routines from lab guide or previous exercises
; ----------------------------------------------------------------

write_2_nibbles:
	; preserve r24
	push r24
	
	; read current state
	in r25, PIND

	andi r25, 0x0F	; preserve control bits
	andi r24, 0xF0	; keep 4 higher data bits
	add r24, r25	; combine
	out PORTD, r24

	; send pulse
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3

	; restore r24 and reverse bits
	pop r24
	swap r24

	andi r24, 0xF0	; keep 4 higher (initially lower) data bits
	add r24, r25	; combine with data bits
	out PORTD, r24

	; send pulse
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3

	ret

; display data
lcd_data:
	; RS=1
	sbi PORTD, PD2
	rcall write_2_nibbles
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec
	ret

; command the screen
lcd_command:
	; RS=0
	cbi PORTD, PD2
	rcall write_2_nibbles
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec
	ret

; clear display
lcd_clear_display:
	; load instruction code and call lcd_command
	ldi r24, 0x01
	rcall lcd_command

	ldi r24, low(5)
	ldi r25, high(5)
	rcall wait_msec
	ret

lcd_init:
	ldi r24, low(200)
	ldi r25, high(200)
	rcall wait_msec

	; 8-bit mode (times) 3
	ldi r24, 0x30
	out PORTD, r24
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec
	ldi r24, 0x30
	out PORTD, r24
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec
	ldi r24, 0x30
	out PORTD, r24
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec

	; 4-bit mode
	ldi r24, 0x20
	out PORTD, r24
	sbi PORTD, PD3
	nop
	nop
	cbi PORTD, PD3
	ldi r24, 250
	ldi r25, 0
	rcall wait_usec

	; 2 lines
	ldi r24, 0x28
	rcall lcd_command

	; turn on display, clear cursor
	ldi r24, 0x0C
	rcall lcd_command
	rcall lcd_clear_display

	; move cursor right after each character
	ldi r24, 0x06
	rcall lcd_command

	ret

; delay routines
wait_msec:
	ldi r26, LOW(iterations)
	ldi r27, HIGH(iterations)
	inner_loop:
		sbiw r26, 1
		brne inner_loop
	sbiw r24, 1
	breq last
	nop
	nop
	nop
	nop
	nop
	nop
	brne wait_msec
last:
	nop
	ret

wait_usec:
	sbiw r24, 1
	call delay_8cycles
	brne wait_usec
	ret
delay_8cycles:
	nop
	nop
	nop
	nop
	ret