#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>

#define INB inb

#ifdef __OpenBSD__
#include <machine/sysarch.h>
#include <machine/pio.h>
#define OUTB outb

#elif defined(__linux__)
#include <sys/io.h>
#define OUTB(port, byte) outb(byte, port)

#else
void
OUTB(int port, int byte)
{
	errx(1, "cannot write to parallel port");
}
int
inb(int port)
{
	errx(1, "cannot read from parallel port");
}
#endif

#include "tribble.h"

unsigned int tribble_port = PORTADDRESS;

int tribble_debug = 0;

void
checkio(void)
{
	if (geteuid() != 0)
		errx(1, "must be run as root");

#ifdef __OpenBSD__
#ifdef __amd64__
	if (amd64_iopl(1) != 0)
		errx(1, "amd64_iopl failed (is machdep.allowaperture=1?)");
#elif defined(__i386__)
	if (i386_iopl(1) != 0)
		errx(1, "i386_iopl failed (is machdep.allowaperture=1?)");
#endif
#elif defined(__linux__)
	iopl(3);
#endif
}

unsigned int
havetribble(void)
{
	int tries;

	/* drop busy */
	OUTB(tribble_port + DATA, 0);

	/* wait for (inverted) strobe */
	for (tries = 0; tries < 1024; tries++)
		if ((INB(tribble_port + STATUS) & stbin) == 0)
			/*
			 * leave busy dropped, assume recvtribble() will deal
			 * with it
			 */
			return 1;

	/* re-raise busy */
	OUTB(tribble_port + DATA, bsyout);

	return 0;
}

int
recvtribble(void)
{
	unsigned char b;
	unsigned int tries;

	/* drop busy */
	OUTB(tribble_port + DATA, 0);

	/* wait for (inverted) strobe */
	tries = 0;
	while ((INB(tribble_port + STATUS) & stbin) != 0) {
		if (++tries >= 500000) {
			/* raise busy/ack */
			OUTB(tribble_port + DATA, bsyout);
			return -1;
		}
	}

	/* grab tribble */
	b = (INB(tribble_port + STATUS) >> 3) & tribmask;

	/* raise busy/ack */
	OUTB(tribble_port + DATA, bsyout);

	/* wait for (inverted) UNstrobe */
	tries = 0;
	while ((INB(tribble_port + STATUS) & stbin) == 0) {
		if (++tries >= 500000)
			return -1;
	}

	return b;
}

int
recvbyte(void)
{
	char c, t;

	if ((t = recvtribble()) == -1)
		return -1;
	c = t;

	if ((t = recvtribble()) == -1)
		return -1;
	c += (t << 3);

	if ((t = recvtribble()) == -1)
		return -1;
	c += ((t & dibmask) << 6);

	return c;
}

int
sendtribble(unsigned char b)
{
	unsigned int tries;
	int ret = 0;

	/* raise busy */
	OUTB(tribble_port + DATA, bsyout);

	/* wait for mailstation to drop busy */
	tries = 0;
	while ((INB(tribble_port + STATUS) & bsyin) != 0) {
		if (++tries >= 500000) {
			ret = 1;
			goto sendtribble_out;
		}
	}

	/* send tribble */
	OUTB(tribble_port + DATA, b & tribmask);

	/* strobe */
	OUTB(tribble_port + DATA, (b & tribmask) | stbout);

	/* wait for ack */
	tries = 0;
	while ((INB(tribble_port + STATUS) & bsyin) == 0) {
		if (++tries >= 500000) {
			ret = 1;
			goto sendtribble_out;
		}
	}

	/* unstrobe */
	OUTB(tribble_port + DATA, 0);

sendtribble_out:
	/* raise busy/ack */
	OUTB(tribble_port + DATA, bsyout);

	return ret;
}

int
sendbyte(unsigned char b)
{
	if (sendtribble(b) != 0)
		return 1;
	if (sendtribble(b >> 3) != 0)
		return 2;
	if (sendtribble(b >> 6) != 0)
		return 3;

	return 0;
}
