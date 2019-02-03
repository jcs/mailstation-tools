; NOTE: You can't dump the rom after mboot or sboot is flashed,
;       because most of the rom is gone by then.
;
; This is code to be poked into dataflash on unit with yahoo
; features active.
; HEY, it seems to work on units *without* yahoo feature, too!!!!
; (4.05E for instance.)
; It's purpose is to dump the coderom out of the parallel port,
; using the tribble code in page #01.
;
; *** One catch to using this is that it requires mailbug
; *** version > 0.0.3, with the "inhale" function, to catch
; *** the data.  And that version is not quite ready yet.
; *** But you do not need it to enter this app into your
; *** mailstation.  In fact, if you want to dump your rom,
; *** you *have to* enter this in hex on the mailstation
; *** keyboard.  That is because in order for mailbug to
; *** write anything to dataflash, mbug needs to be loaded,
; *** and loading mbug requires flashing sboot or mboot, and
; *** flashing anything (to codeflash) will erase your rom.
; ***   You have to do this BEFORE INSTALLING SBOOT!

; STEP 1)
; This is data from dataflash page #08, sector #00 (which
; translates to a raw address of 020000), of a mailstation
; with two apps:
;
; 020000  02 00 02 00 01 00 01 5e   1c 18 01 01 01 20 a7 19
;
; This is the key to enabling added apps!!!
; At least first byte above is needed, it seems to be count
; of apps (2 in this case).  You need to poke it into dataflash
; with hex editor at raw address 020000.
;
; Modify the first byte to match number of apps, including
; new one.  If your mailstation already has app(s), and you
; want to overwite one of them, you can leave this data as is,
; and skip this step.  I don't know what the rest are, but those
; 16 bytes are the only data in dataflash pg #08, sector #00.
;
; If you inc that first byte, without doing steps 2 or 3, you
; should enable blank (or giberish) icons in the menu (under
; yahoo, or extras).
;
; *** Enter Test mode with vulcan nerve pinch while booting:
;     <func><shift><t>, or <func><size><t> or on the newer
;     models, <func><q><a> followed by "qa781206" without
;     the quotes, of course.
;
; *** Enter Hex Viewer mode with <shift><f5>.
;
; *** Enable Hex Edit mode by entering "G710304x<enter>" without
;     quotes of course.  This is a "go" to a nonexising sector,
;     and it will leave viewer on last sector of dataflash.
;
; *** Edit dataflash page #08 as described above. G020000 will
;     get you to the right sector. <ctrl><s> will enter edit mode
;     for current sector.  Edit the data.  <ctrl><s> will leave
;     edit mode and save to dataflash.  You need to leave edit
;     mode before moving to next sector.


; STEP 2}
; This is the start of our new app.  If your mailstation has
; no existing "apps", then you would poke this into page #00
; of dataflash (raw address 000000).
;
; If you already have say 2 existing apps, they are probably
; in dataflash pages #00 & #01 respectively.  In that case you
; would put this in page #02 of dataflash (008000).
;
; When using dataflash page #00, this goes at raw address 000000.
; When using dataflash page #01, this goes at raw address 004000.
; When using dataflash page #02, this goes at raw address 008000.
;
; (iow, in raw addresses, first app is at 000000, second at 004000,
; and third at 008000.)
;
; The JP instruction is the only thing in this part that is
; absolutely necessary, if you can live with a captionless
; menu entry.  The icon is gonna be blank anyways.
;
; *** Use hex editor to "go" to your address (G000000 or G004000
;     or G008000).  This address corresponds to #4000 in the
;     following app, no matter what page you are using.
;
; *** Enter edit mode, enter at least the bytes for the JP of
;     the following code, and save the sector.  Goto step 3.

	.module	spew

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
	.dw	(endcap - caption - 6)	; num of chars
	.dw	#0006			; offset to first char
	.ascii	"Spew"			; the caption string
endcap:

icon:
; I haven't figured out the icon format yet, but it's not needed.
; Icon data would go here, followed by app data (news, weather,
; or tv).
; Just leave this data as it is, and we will skip ahead to the last
; sector in this same page.


; STEP 3)
; I don't think it matters exactly where this next part
; sits in dataflash, as long as the JP in step two above can
; find it.  We will put it at the end of the page, 'coz that's
; where the mailstation apps put it.
;
; If using dataflash page #00, this goes at raw address 003f00.
; If using dataflash page #01, this goes at raw address 007f00.
; If using dataflash page #02, this goes at raw address 00bf00.
;
; *** Use "go" command with appropriate address matching address
;     used in step 2.
;
; *** Edit mode, enter the following 37 bytes of code, save.
;
; *** Quit Hex Editor with <back> key.
;
; *** Leave Test mode with <q> key.
;
; *** The mailstation will reboot, pick "no clear", and then
;     check if you have a new app listed in your menu under
;     "yahoo" or in "extras".


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
; This is the "event handler" for our new app.  It doesn't
; really handle any events, it just spews the rom contents
; over the laplink.

	xor	a		; Set slot8000device = codeflash
	out	(8), a

	ld	bc, #0x4000	; b=count=64 pages, c=currentpage=0

pgloop:
; for count = 64 downto 1 do

	ld	hl, #0x8000	; start at begining of each page

byteloop:
; for i=#8000 to #BFFF do

	ld	a, c		; bank currentpage into slot8000
	out	(7), a

	ld	a, (hl)		; get byte[i]

	push	hl
	push	bc

	ld	h, a		; h is byte

	ld	a, #1		; bank bsendbyte into slot8000
	out	(7), a
	call	bsendbyte	; send byte(H)

	pop	bc
	pop	hl

	inc	hl		; i++   (next byte)
	ld	a, h
	cp	#0xC0
	jr	nz, byteloop	; jump if i < #C000

	inc	c		; currentpage++  (next page)
	djnz	pgloop

	jp	done
