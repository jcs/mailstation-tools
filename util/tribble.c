#include <stdio.h>
#include <sys/types.h>
#include <machine/pio.h>

#include "tribble.h"

int tribble_debug = 0;

unsigned int
havetribble(void)
{
	int tries;

	/* drop busy */
	outb(DATA, 0);

	/* wait for (inverted) strobe */
	for (tries = 0; tries < 100; tries++)
		if ((inb(STATUS) & stbin) == 0)
			/*
			 * leave busy dropped, assume recvtribble() will deal
			 * with it 
			 */
			return 1;

	/* re-raise busy */
	outb(DATA, bsyout);

	return 0;
}

unsigned char
recvtribble(void)
{
	unsigned char b;

	/* drop busy */
	outb(DATA, 0);

	if (tribble_debug) {
		printf("waiting for strobe...");
		fflush(stdout);
	}

	/* wait for (inverted) strobe */
	while ((inb(STATUS) & stbin) != 0)
		;

	/* grab tribble */
	b = (inb(STATUS) >> 3) & tribmask;

	/* raise busy/ack */
	outb(DATA,bsyout);

	/* wait for (inverted) UNstrobe */
	while ((inb(STATUS) & stbin) == 0)
		;

	return b;
}

unsigned char
recvbyte(void)
{
	return recvtribble() + (recvtribble() << 3) +
	    ((recvtribble() & dibmask) << 6);
}

void
sendtribble(unsigned char b)
{
	/* raise busy */
	outb(DATA, bsyout);

	/* wait for mailstation to drop busy */
	while ((inb(STATUS) & bsyin) != 0)
		;

	/* send tribble */
	outb(DATA, b & tribmask);

	/* strobe */
	outb(DATA, (b & tribmask) | stbout);

	/* wait for ack */
	while ((inb(STATUS) & bsyin) == 0)
		;

	/* unstrobe */
	outb(DATA, 0);

	/* raise busy/ack */
	outb(DATA, bsyout);
}

void
sendbyte(unsigned char b)
{
	sendtribble(b);
	sendtribble(b >> 3);
	sendtribble(b >> 6);
}
