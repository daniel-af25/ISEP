.cseg
.org 0x0000
jmp inic
.cseg
.org 0x0046
inic:
	ldi r16,0b00000000
	out DDRA,r16
	ldi r16,0b11111111
	out DDRC,r16
	out PORTC,r16
	rjmp sw1

sw1:
	sbic PINA,0
	rjmp sw2
	ldi r16, 0b01111110
	out PORTC,r16
	rjmp sw1

sw2:
	sbic PINA,1
	rjmp sw3
	ldi r16, 0b10111101
	out PORTC,r16
	rjmp sw1

sw3:
	sbic PINA,2
	rjmp sw4
	ldi r16, 0b11011011
	out PORTC,r16
	rjmp sw1

sw4:
	sbic PINA,3
	rjmp sw6
	ldi r16, 0b11100111
	out PORTC,r16
	rjmp sw1

/*sw5:
	sbic PINA,4
	rjmp sw6
	ldi r16, 0b00000000
	out PORTC,r16
	rjmp sw1
	*/
sw6:
	sbic PINA,5
	rjmp sw1
	ldi r16, 0b11111111
	out PORTC,r16
	rjmp sw1

	
