; vim:syntax=z8a:ts=8
;
; dataflash loader
;
; works as a "stage 2" loader, so type in loader.asm and then run Loader from
; the Yahoo! menu, then use sendload to upload this compiled binary, then
; use sendload to upload your actual binary to be written to the dataflash at
; DATAFLASH_PAGE
;
; Copyright (c) 2019 joshua stein <jcs@jcs.org>
;
; Permission to use, copy, modify, and distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
;
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;

	.module	dataflashloader

	; which page (Yahoo! menu slot) to save an uploaded program to
	; page 0 is 0x0000, page 1 0x4000, page 2, 0x8000, etc.
	.equ	DATAFLASH_PAGE, #0


	; when running from Loader, we are loaded at 0x8000 and use slot 4
	.equ	SLOT_ADDR,	#0x4000
	.equ	RUN_ADDR,	#0x8000
	.equ	SLOT_DEVICE,	#0x6
	.equ	SLOT_PAGE,	#0x5

	; where we'll buffer the 256 bytes we receive before writing to flash
	.equ	RAM_ADDR,	#0xd000

	.equ	SDP_LOCK,	#SLOT_ADDR + 0x040a
	.equ	SDP_UNLOCK,	#SLOT_ADDR + 0x041a

	; lpt
	.equ	CONTROL,	#0x2c
	.equ	DATA,		#0x2d
	.equ	STATUS,		#0x21

	.equ	LPT_BUSY_IN,	#0x40
	.equ	LPT_BUSY_OUT,	#0x08
	.equ	LPT_STROBE_IN,	#0x80
	.equ	LPT_STROBE_OUT,	#0x10
	.equ	LPT_TRIB_MASK,	#0x07
	.equ	LPT_DIB_MASK,	#0x03


	.area	_DATA
	.area	_HEADER (ABS)

	.org 	#RUN_ADDR

	jp	eventhandler

 	.dw	(icons)
 	.dw	(caption)
 	.dw	(dunno)
dunno:
	.db	#0
zip:
	.dw	#0
zilch:
	.dw	#0
caption:
	.dw	#0x0001
	.dw	(endcap - caption - 6)	; num of chars
	.dw	#0x0006			; offset to first char
	.ascii	"FlashLoader"		; the caption string
endcap:

icons:
	.dw	#0x0			; size icon0
	.dw	(icon0 - icons)		; offset to icon0
	.dw	#0			; size icon1
	.dw	(icon1 - icons)		; offset to icon1 (0x00b5)
icon0:
	.dw	#0x0			; icon width
	.db	#0x0			; icon height

icon1:
	.dw	#0			; icon width
	.db	#0			; icon height

eventhandler:
	push	ix
	ld	ix, #0
	add	ix, sp
	push	bc
	push	de
	push	hl
	ld	hl, #-8			; stack bytes for local storage
	add	hl, sp
	ld	sp, hl
	in	a, (#SLOT_DEVICE)
 	ld	-3(ix), a		; stack[-3] = slot 8 device
	in	a, (#SLOT_PAGE)
 	ld	-4(ix), a		; stack[-4] = slot 8 device
	ld	a, #3			; slot 8 device = dataflash
	out	(#SLOT_DEVICE), a
	xor	a			; slot 8 page = 0
	out	(#SLOT_PAGE), a
	ld	hl, #SLOT_ADDR
	ld	-5(ix), h
	ld	-6(ix), l		; stack[-5,-6] = data flash location
get_size:
	call	getbyte			; low byte of total bytes to download
	ld	-8(ix), a
	call	getbyte			; high byte of total bytes to download
	ld	-7(ix), a
	cp	#0x40			; we can't write more than 0x3fff
	jr	c, size_ok
size_too_big:
	jp	0x0000
size_ok:
	di				; prevent things from touching RAM
	call	sdp
	ld	a, (#SDP_UNLOCK)
read_chunk_into_ram:
	; read 256 bytes at a time into ram, erase the target flash sector,
	; then program it with those bytes
	ld	b, -7(ix)
	ld	c, -8(ix)
	ld	hl, #RAM_ADDR
ingest_loop:
	call	getbyte
	ld	(hl), a
	inc	hl
	dec	bc
	ld	a, l
	cp	#0
	jr	z, done_ingesting	; on 256-byte boundary
	ld	a, b
	cp	#0
	jr	nz, ingest_loop		; bc != 0, keep reading input
	ld	a, c
	cp	#0
	jr	nz, ingest_loop		; bc != 0, keep reading input
done_ingesting:
	ld	-7(ix), b		; update bytes remaining to fetch on
	ld	-8(ix), c		;  next iteration
move_into_flash:
	ld	a, #3			; slot 8 device = dataflash
	out	(#SLOT_DEVICE), a
	ld	a, #DATAFLASH_PAGE
	out	(#SLOT_PAGE), a
	ld	de, #RAM_ADDR
	ld	h, -5(ix)		; data flash write location
	ld	l, -6(ix)
sector_erase:
	ld	(hl), #0x20		; 28SF040 Sector-Erase Setup
	ld	(hl), #0xd0		; 28SF040 Execute
sector_erase_wait:
	ld	a, (hl)			; wait until End-of-Write
	ld	b, a
	ld	a, (hl)
	cp	b
	jr	nz, sector_erase_wait
byte_program_loop:
	ld	a, (de)
	ld	(hl), #0x10		; 28SF040 Byte-Program Setup
	ld	(hl), a			; 28SF040 Execute
byte_program:
	ld	a, (hl)
	ld	b, a
	ld	a, (hl)			; End-of-Write by reading it
	cp	b
	jr	nz, byte_program	; read until writing succeeds
	inc	hl			; next flash byte
	inc	de			; next RAM byte
	ld	a, l
	cp	#0
	jr	nz, byte_program_loop
sector_done:
	ld	-5(ix), h		; update data flash write location
	ld	-6(ix), l
	ld	a, -7(ix)
	cp	#0
	jp	nz, read_chunk_into_ram	; more data to transfer
	ld	a, -8(ix)
	cp	#0
	jp	nz, read_chunk_into_ram	; more data to transfer
	; all done
flash_out:
	ld	a, #3			; slot 8 device = dataflash
	out	(#SLOT_DEVICE), a
	xor	a			; slot 8 page = 0
	out	(#SLOT_PAGE), a
	call	sdp
	ld	a, (#SDP_LOCK)
bail:
 	ld	a, -3(ix)		; restore slot 8 device
	out	(#SLOT_DEVICE), a
 	ld	a, -4(ix)		; restore slot 8 page
	out	(#SLOT_PAGE), a
	ld	hl, #8			; remove stack bytes
	add	hl, sp
	ld	sp, hl
	pop	hl
	pop	de
	pop	bc
	ld	sp, ix
	pop	ix
	ei
	jp	0x0000


sdp:
	ld	a, (#SLOT_ADDR + 0x1823) ; 28SF040 Software Data Protection
	ld	a, (#SLOT_ADDR + 0x1820)
	ld	a, (#SLOT_ADDR + 0x1822)
	ld	a, (#SLOT_ADDR + 0x0418)
	ld	a, (#SLOT_ADDR + 0x041b)
	ld	a, (#SLOT_ADDR + 0x0419)
	ret
	; caller needs to read final SDP_LOCK or SDP_UNLOCK address


; loop until we get a byte, return it in a
getbyte:
	push	hl
getbyte_loop:
	call	_lptrecv
	ld	a, h
	cp	#0
	jp	nz, getbyte_loop
	ld	a, l
	pop	hl
	ret


; receive a tribble byte from host, return h=1 l=0 on error, else h=0, l=(byte)
lptrecv_tribble:
	push	bc
	ld	hl, #0			; h will contain error, l result
	xor	a
	out	(DATA), a		; drop busy/ack, wait for strobe
	ld	b, #0xff		; try a bunch before bailing
wait_for_strobe:
	in	a, (STATUS)
	and	#LPT_STROBE_IN		; inb(STATUS) & stbin
	jr	nz, got_strobe
	djnz	wait_for_strobe
strobe_failed:
	ld	h, #1
	ld	l, #0
	jr	lptrecv_tribble_out
got_strobe:
	in	a, (STATUS)
	sra	a
	sra	a
	sra	a
	and	#LPT_TRIB_MASK		; & tribmask
	ld	l, a
	ld	a, #LPT_BUSY_OUT	; raise busy/ack
	out	(DATA), a
	ld	b, #0xff		; retry 255 times
wait_for_unstrobe:
	in	a, (STATUS)
	and	#LPT_STROBE_IN		; inb(STATUS) & stbin
	jr	z, lptrecv_tribble_out
	djnz	wait_for_unstrobe
	; if we never get unstrobe, that's ok
lptrecv_tribble_out:
	ld	a, #LPT_BUSY_OUT	; raise busy/ack
	out	(DATA), a
	pop	bc
	ret


; unsigned char lptrecv(void)
; receive a full byte from host in three parts
; return h=1 l=0 on error, otherwise h=0, l=(byte)
_lptrecv::
	push	bc
	call	lptrecv_tribble
	ld	a, h
	cp	#1
	jr	z, lptrecv_err		; bail early if h has an error
	ld	b, l
	call	lptrecv_tribble
	ld	a, h
	cp	#1
	jr	z, lptrecv_err
	ld	a, l
	sla	a
	sla	a
	sla	a
	add	b
	ld	b, a			; += tribble << 3
	call	lptrecv_tribble
	ld	a, h
	cp	#1
	jr	z, lptrecv_err
	ld	a, l
	and	#LPT_DIB_MASK		; dibmask
	sla	a
	sla	a
	sla	a
	sla	a
	sla	a
	sla	a
	add	b			; += (tribble & dibmask) << 6
	ld	h, #0
	ld	l, a
lptrecv_out:
	pop	bc
	ret
lptrecv_err:
	pop	bc
	ld	hl, #0x0100
	ret
