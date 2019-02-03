#
# Requires SDCC and its ASZ80 assembler
# http://sdcc.sourceforge.net/
#
# Requires hex2bin
# https://sourceforge.net/projects/hex2bin/files/hex2bin/
#

ASZ80	?= sdasz80 -l
SDCC	?= sdcc -mz80

OBJ	?= obj/

all: loader.bin codedump.bin datadump.bin

clean:
	rm -f *.{map,bin,ihx,lst,rel,sym,lk,noi}

# parallel loader
loader.rel: loader.asm
	$(ASZ80) -o $@ $>

loader.ihx: loader.rel
	$(SDCC) --no-std-crt0 -o $@ $>

loader.bin: loader.ihx
	hex2bin $> >/dev/null


# parallel dumpers, codeflash and dataflash
codedump.rel: codedump.asm
	$(ASZ80) -o $@ $>

codedump.ihx: codedump.rel
	$(SDCC) --no-std-crt0 -o $@ $>

codedump.bin: codedump.ihx
	hex2bin $> >/dev/null

datadump.rel: datadump.asm
	$(ASZ80) -o $@ $>

datadump.ihx: datadump.rel
	$(SDCC) --no-std-crt0 -o $@ $>

datadump.bin: datadump.ihx
	hex2bin $> >/dev/null
