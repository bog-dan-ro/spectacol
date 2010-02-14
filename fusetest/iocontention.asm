; Contention tester

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

ORG 0xc000
	
main
PROC
	call interruptsync_init

	ld hl, 0x9100		; framelength - 32768
	ld (framelen), hl

	ld hl, (basedelay)

_next  	ld de, 0xff51
	add hl, de
	ld (delay1), hl

	ld e, l
	ld d, h

	ld hl, (framelen)
	ld bc, 0x7d1e
	add hl, bc
	xor a
	sbc hl, de
	ld (delay2), hl

	call bigtest

	ld hl, (basedelay)
	inc hl
	ld (basedelay), hl

	jr _next 

ENDP

bigtest
PROC
	push de
	push hl

	im 1
	ei

	ld hl, (basedelay)
	ld a, h
	call printa
	ld a, l
	call printa
	ld hl, _colonstring
	call printstring

	di
	im 2

	ld b, 0x00
	push bc
	
	call test

	pop bc
	add a, b
	ld b, a
	push bc

	ld a, ','
	rst 0x10

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	pop bc
	add a, b
	ld b, a
	push bc		

	ld a, ','
	rst 0x10

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	pop bc
	add a, b
	ld b, a
	push bc		

	ld a, ','
	rst 0x10

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	pop bc
	add a, b
	ld b, a

	ld hl, _finalstring
	call printstring

	ld a, b
	neg
	call printa

	ld a, 0x0d
	rst 0x10

	pop hl
	pop de

	ret

_colonstring defb ': ', 0
_finalstring defb ' = ', 0

ENDP

test
PROC
	call interruptsync

	ld hl, sync_isr + 1	; 92
	ld (hl), _isr % 0x100	; 102
	inc hl			; 112
	ld (hl), _isr / 0x100	; 118

	ld hl, (delay1)		; 128
	call delay		; 144

	call test_routine

	ld hl, (delay2)
	call delay

	jp atiming

_isr	pop hl
	call printa
	ret
ENDP

test_routine
PROC
	ld a, 0xff
	in a, (0xfe)
	ret
ENDP

basedelay defw 0x37fb
delay1 defw 0000
delay2 defw 0000

INCLUDE atiming.asm
INCLUDE delay.asm
INCLUDE print.asm
INCLUDE sync.asm
INCLUDE framelength.asm

END	main
