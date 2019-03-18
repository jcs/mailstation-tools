/*
 * recvdump
 * based on win32/maildump.cpp by FyberOptic
 *
 * usage: recvdump [-data | -code]
 *
 * must be run as root to set iopl and use inb/outb
 *
 * assumes parallel port is at PORTADDRESS and codedump or datadump has been
 * loaded on the MailStation and is running
 */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/sysarch.h>
#include <machine/pio.h>

#define PORTADDRESS 0x378

#define DATA PORTADDRESS+0
#define STATUS PORTADDRESS+1
#define CONTROL PORTADDRESS+2

#define bsyin 0x40
#define bsyout 0x08
#define stbin 0x80
#define stbout 0x10
#define tribmask 0x07
#define dibmask 0x03

unsigned char
recvtribble(void)
{
	unsigned char mytribble;

	/* drop busy/ack */
	outb(DATA, 0);

	/* wait for (inverted) strobe */
	while ((inb(STATUS) & stbin) != 0)
		;

	/* grab tribble */
	mytribble = (inb(STATUS) >> 3) & tribmask;

	/* raise busy/ack */
	outb(DATA,bsyout);

	/* wait for (inverted) UNstrobe */
	while ((inb(STATUS) & stbin) == 0)
		;

	return mytribble;
}

int
main(int argc, char *argv[])
{
	FILE *pFile;
	unsigned int received = 0, expected = 0;
	unsigned char b;
	char fn[14];
	int codeflash = 0, dataflash = 0;
	int x;

	for (x = 1; x < argc; x++) {
		if (strncmp((char *)argv[x], "-code", 5) == 0)
			codeflash = 1;
		else if (strncmp((char *)argv[x], "-data", 5) == 0)
			dataflash = 1;
		else
			printf("unknown parameter: %s\n", argv[x]);
	}

	if (codeflash == dataflash) {
		printf("usage: %s [-code | -data]\n", argv[0]);
		return 1;
	}

	if (codeflash) {
		expected = 1024 * 1024;
		strlcpy(fn, "codeflash.bin", sizeof(fn));
	} else if (dataflash) {
		expected = 1024 * 512;
		strlcpy(fn, "dataflash.bin", sizeof(fn));
	}

	if (geteuid() != 0)
		errx(1, "must be run as root");

#ifdef __OpenBSD__
	if (amd64_iopl(1) != 0)
		errx(1, "amd64_iopl failed (is machdep.allowaperture=1?)");
#endif

	pFile = fopen(fn, "wb");
	if (!pFile) {
		printf("couldn't open file %s\n", fn);
		return -1;
	}

	printf("dumping to %s, run Code Dump on MailStation now...", fn);
	fflush(stdout);

	while (received < expected) {
		b = recvtribble() + (recvtribble() << 3) +
		    ((recvtribble() & dibmask) << 6);

		if (received == 0)
			printf("\n");

		fputc(b, pFile);
		received++;

		if (received % 1024 == 0 || received == expected) {
			printf("\rreceived: %07d/%07d", received, expected);
			fflush(stdout);
		}
	}
	fclose(pFile);

	printf("\n");

	return 0;
}
