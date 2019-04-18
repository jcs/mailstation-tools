#
# ASM code requires SDCC and its ASZ80 assembler
# http://sdcc.sourceforge.net/
# and hex2bin
# https://sourceforge.net/projects/hex2bin/files/hex2bin/
#

ASZ80	?= sdasz80 -l
SDCC	?= sdcc -mz80

OBJ	?= obj/

CFLAGS	+= -Wall

IOPL_LIB=

OS	!= uname

.if ${OS} == "OpenBSD"
.if ${MACHINE_ARCH} == "amd64"
IOPL_LIB=-lamd64
.elif ${MACHINE_ARCH} == "i386"
IOPL_LIB=-li386
.endif
.endif

all: objdir loader.bin codedump.bin datadump.bin memdump.bin \
	recvdump sendload tribble_getty

objdir:
	@mkdir -p ${OBJ}

clean:
	rm -f *.{map,bin,ihx,lst,rel,sym,lk,noi} recvdump sendload \
		tribble_getty

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

memdump.rel: memdump.asm
	$(ASZ80) -o $@ $>

memdump.ihx: memdump.rel
	$(SDCC) --no-std-crt0 -o $@ $>

memdump.bin: memdump.ihx
	hex2bin $> >/dev/null

# datadump/codedump receiver
recvdump: util/recvdump.c util/tribble.c
	$(CC) $(CFLAGS) -o $@ $> $(IOPL_LIB)

# program loader
sendload: util/sendload.c util/tribble.c
	$(CC) $(CFLAGS) -o $@ $> $(IOPL_LIB)

# tribble getty
tribble_getty: util/tribble_getty.c util/tribble.c
	$(CC) $(CFLAGS) -o $@ $> $(IOPL_LIB) -lutil
