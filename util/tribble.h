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
	unsigned char b;

	/* drop busy/ack */
	outb(DATA, 0);

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
	outb(DATA, b & tribmask | stbout);

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
