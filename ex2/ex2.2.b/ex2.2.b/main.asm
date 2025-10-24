.include "m328PBdef.inc"

.org 0x0
rjmp init
.org 0x2
rjmp ISR0

.equ FOSC_MHZ = 16				; frequency
.equ DEL_mS = 1000				; delay in msec
.equ DEL_NU = FOSC_MHZ * DEL_mS	; delay_mS routine: 1000*DEL_NU + 6 cycles
.equ five_msec = FOSC_MHZ * 5	; 5msec delay for debouncing

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

    ; PD2 ready (input and enabled pull-up)
    cbi DDRD, 2
    sbi PORTD, 2

    ; enable interrupts
    sei

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

; interrupt routine
ISR0:
	; save the registers we use in the stack
    push r18
    push r24
	push r25
debouncing:
	; clear INTF0 by writing 1 to EIFR
	ldi r24, (1 << INTF0)
	out EIFR, r24

	; delay 5msec
	ldi r24, LOW(five_msec)
	ldi r25, HIGH(five_msec)
	rcall delay_mS

	; check if EIFR has been reset due to bouncing
	sbic EIFR, INT0
	rjmp debouncing

	; if PD2 is still pressed, loop back
	sbic PIND, 2
	rjmp debouncing

	; ------------------------

    ; place PORTB in r18 and keep only PB1-PB4
    in r18, PINB
    andi r18, 0b00011110

    rcall count	; count how many pins from PB1 to PB4 were pressed

    out PORTC, r18	; print

	; pop the registers from the stack
	pop r25
	pop r24
    pop r18
    reti

; amount of PINB buttons pressed
count:
    clr counter

    ; increment counter only if PBx was pressed
	; skip incrementing the counter if bit was set (reverse logic)
    sbrs r18, 1
    inc counter
    sbrs r18, 2
    inc counter
    sbrs r18, 3
    inc counter
    sbrs r18, 4
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
prep_output:
	; we shift r18's bits to the left and
	; add an "1" to the LSB by performing 
	; an OR operation with 0b1 until
	; the counter reaches 0

    ; if counter=0, r18 is ready to be printed
    tst counter
    breq ready
    ; if counter>0, decrease it
    dec counter
    ; add another "1" to r18
    lsl r18       ; shift LSB (and all bits) to the left
    ori r18, 1    ; place an "1" at new LSB
    rjmp prep_output
ready:
    ret