;
; Based off of spew.asm from Cyrano Jones on the mailstation Yahoo Group,
; modified to send 64kb of memory.
;
; Originally written to be assembled with AS80, converted to SDCC ASZ80 syntax.
;

	.module	memdump

	.area	_DATA
	.area	_HEADER (ABS)
	.org	0x4000			; This is *always* #4000, regardless of
					; what page you use.

	jp	eventhandler

	.dw	(icons)			; icon location (in this page)
	.dw	(caption)
	.dw	(dunno)
dunno:
	.db	#0
xpos:
	.dw	#0
ypos:
	.dw	#0
caption:
	.dw	#0x0001			; ?????
	.dw	(endcap - caption - 6)	; end of chars
	.dw	#0x0006			; offset to first char
	.ascii	"Mem Dump"		; the caption string
endcap:

icons:
	.dw	#0		; size icon0
	.dw	(icon0 - icons)	; offset to icon0
	.dw	#0		; size icon1
	.dw	(icon1 - icons)	; offset to icon1 (0x00b5)
icon0:
	.dw	#0		; icon width
	.db	#0		; icon height
icon1:
	.dw	#0		; icon width
	.db	#0		; icon height


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
	ld	a, #0			; set slot8000device = codeflash
	out	(8), a
	ld	a, #1			; bank bsendbyte into slot8000
	out	(7), a

	ld	de, #0x0000
byteloop:
	ld	a, (de)
	ld	h, a
	push	de
	call	bsendbyte		; send byte(H)
	pop	de

	ld	a, d
	cp	#0xff
	jr	nz, incde
	ld	a, e
	cp	#0xff
	jr	nz, incde
	jp	done
incde:
	inc	de
	jr	byteloop
