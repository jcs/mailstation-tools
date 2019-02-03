;
; This loader app is based around the Yahoo app model used in spew.asm, by Cyrano Jones.
; The original comments have been removed/replaced.  The code itself of course is also
; replaced to change the functionality, which is to recieve a dynamically specified
; number of bytes.  That's all it does.  The screen doesn't even update, in order to
; make the binary as small as possible.
;
; The first two bytes sent from the host make up a 16-bit value of the total number of
; data bytes to follow.  The first byte is the low byte, the second is the high.  After
; those, the actual data is transferred into ram page 1 in slot8000.  The total byte
; count is decremented, and when zero, it jumps to 0x8000 to run the transferred code.
;
; This was written to be assembled using AS80, which can be found at:
;  http://www.kingswood-consulting.co.uk/assemblers/
;
;
; - FyberOptic (fyberoptic@gmail.com)
;



brecvbyte equ #8027			; Firmware function in codeflash page 1.  Attempts
					; to receive a byte.  Upon returning, if a = 0, it
					; timed out or failed.  Otherwise the l register
					; holds the received byte.


	org	#4000			; Apps always start at 0x4000 no matter what
					; dataflash page they load from.

	jp	eventhandler		; Jump to the start of our code.

	defw	icon			; The following is data about the app
	defw	caption			; itself, most of which we won't even
	defw	dunno			; worry about.

dunno:
	defb	#00
zip:
	defw	#0000
zilch:
	defw	#0000

caption:
	defw	#0001
	defw	endcap-caption-6
	defw	#0006
	defm	"Loader"		; The app icon label.
endcap:

icon:					; Except no icon for us!



;----------------------------------------------------------
; Now for the actual code
;----------------------------------------------------------

getbyte:
	push	bc		; Preserve BC, HL
	push	hl

	xor	a		; Put codeflash page 1 into slot8000.
	out	(#08), a
	inc	a
	out	(#07), a

getbyte2:
	call	brecvbyte	; Try to fetch a byte.
	or	a		; If we didn't get one, try again.
	jr	z, getbyte2

	ld	a, l		; Load received byte into A register

	pop	hl		; Restore BC, HL
	pop	bc
	ret


;------

eventhandler:

	call	getbyte		; Get low byte of total bytes to download
	ld	l, a

	call	getbyte		; Get high byte of total bytes to download
	ld	h, a


	ld	bc, #8000	; Destination address
nextcodebyte:
	call	getbyte		; Fetch a byte of data

	ld	d, a		; Preserve A

	ld	a, #1		; Put ram page 1 into slot8000
	out	(#08), a
	out	(#07), a

	ld	a, d		; Restore A

	ld	(bc), a		; Load incoming byte to ram.
	inc	bc		; Inc ram location.

	dec	hl		; Dec bytes to be received.

	xor	a		; Check if hl = 0; get another byte if not
	or	h
	jr	nz, nextcodebyte
	xor	a
	or	l
	jr	nz, nextcodebyte


	jp	#8000		; When done, jump to code!

