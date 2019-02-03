;
; Based off of spew.asm from Cyrano Jones on the mailstation Yahoo Group,
; modified to send the 512kb of dataflash instead of 1mb of codeflash.
;
; Originally written to be assembled with AS80, converted to SDCC ASZ80 syntax.
;

	.module	datadump

	.area	_DATA
	.area	_HEADER (ABS)
	.org	0x4000			; This is *always* #4000, regardless of
					; what page you use.

	jp	eventhandler

	.dw	(icon)			; icon location (in this page)
	.dw	(caption)
	.dw	(dunno)

dunno:
	.db	#0
zip:
	.db	#0
zilch:
	.db	#0
caption:
	.dw	#0x0001			; ?????
	.dw	(endcap - caption - 6)	; end of chars
	.dw	#0006			; offset to first char
	.ascii	"Data Dump"		; the caption string
endcap:

icon:

	.equ	bsendbyte, #0x802D	; raises busy & sends byte.
					; We use the existing sendbyte from
					; original update code. This means
					; codeflash page #01 needs to be banked
					; in to slot8000 before calling bsendbyte.

	.equ	done, #0x0000		; Gotta go somewhere when done, we reboot.
					; Mailstation will call eventhandler 3
					; or 4 times when you select the new
					; application, and we only want to exec
					; once, so we do not return at end, we
					; reboot after first "event".

eventhandler:
	ld	bc, #0x2000		; b=count=32 pages, c=currentpage=0

pgloop:
	ld	hl, #0x8000		; start at begining of each page

byteloop:
	ld	a, #03			; set slot8000device = dataflash
	out	(8), a

	ld	a, c			; set slot8000page to currentpage
	out	(7), a

	ld	a, (hl)			; get byte[i]

	push	hl
	push	bc

	ld	h, a			; h is byte

	ld	a, #00			; set slot8000device = codeflash
	out	(8), a
	ld	a, #1			; bank bsendbyte into slot8000
	out	(7), a
	call	bsendbyte		; send byte(H)

	pop	bc
	pop	hl

	inc	hl			; i++   (next byte)
	ld	a, h
	cp	#0xC0
	jr	nz, byteloop		; jump if i < #C000

	inc	c			; currentpage++  (next page)
	djnz	pgloop

	jp	done
