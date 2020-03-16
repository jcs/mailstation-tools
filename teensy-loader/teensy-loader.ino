/*
 * Teensy Mailstation parallel port interface
 * Copyright (c) 2020 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

const int pLED      = 13; /* as it is on the Teensy 3.2 */

const int pError    =  1; /* pin 2 (data0) on mailstation */
const int pSelect   =  2; /* pin 3 (data1) on mailstation */
const int pPaperOut =  3; /* pin 4 (data2) on mailstation */
const int pAck      =  4; /* pin 5 (data3) on mailstation */
const int pBusy     =  5; /* pin 6 (data4) on mailstation */

const int pData4    =  8; /* pin 11 (busy) on mailstation */
const int pData3    =  7; /* pin 10 (ack) on mailstation */
const int pData2    =  9; /* pin 12 (paperout) on mailstation */
const int pData1    = 10; /* pin 13 (select) on mailstation */
const int pData0    = 11; /* pin 15 (error) on mailstation */
			  /* connect teensy GND pin to mailstation pin 25 */

#define bsyin       0x40 /* 01000000 */
#define bsyout      0x08 /* 00001000 */
#define stbin       0x80 /* 10000000 */
#define stbout      0x10 /* 00010000 */
#define tribmask    0x07 /* 00000111 */
#define dibmask     0x03 /* 00000011 */

void	dumpbyte(char);
int	readstatus(void);
void	writedata(char);
int	recvtribble(void);
int	recvbyte(void);
int	sendtribble(unsigned char);
int	sendbyte(unsigned char);

void
setup(void)
{
	Serial.begin(115200);

	pinMode(pData0, OUTPUT);
	pinMode(pData1, OUTPUT);
	pinMode(pData2, OUTPUT);
	pinMode(pData3, OUTPUT);
	pinMode(pData4, OUTPUT);

	pinMode(pAck, INPUT);
	pinMode(pBusy, INPUT);
	pinMode(pPaperOut, INPUT);
	pinMode(pSelect, INPUT);
	pinMode(pError, INPUT);

	pinMode(pLED, OUTPUT);

	writedata(bsyout);
}

void
loop(void)
{
	while (Serial.available()) {
		char b = Serial.read();
		while (sendbyte(b) != 0)
			;
	}
}

void
dumpbyte(char b)
{
	Serial.print((int)b);
	Serial.print(" =");
	for (int x = 7; x >= 0; x--) {
		Serial.print(" ");
		Serial.print(!!(b & (1 << x)));
	}
	Serial.println("");
}

int
readstatus(void)
{
	return ((digitalRead(pBusy) == HIGH ? 1 : 0) << 7) +
	    ((digitalRead(pAck) == HIGH ? 1 : 0) << 6) +
	    ((digitalRead(pPaperOut) == HIGH ? 1 : 0) << 5) +
	    ((digitalRead(pSelect) == HIGH ? 1 : 0) << 4) +
	    ((digitalRead(pError) == HIGH ? 1 : 0) << 3);
}

void
writedata(char b)
{
	/* we only have 5 data pins, so mask off high 3 bits and shift */
	b = (b & ~0xe0) << 3;

	digitalWrite(pData4, (b & (1 << 7)) == 0 ? LOW : HIGH);
	digitalWrite(pData3, (b & (1 << 6)) == 0 ? LOW : HIGH);
	digitalWrite(pData2, (b & (1 << 5)) == 0 ? LOW : HIGH);
	digitalWrite(pData1, (b & (1 << 4)) == 0 ? LOW : HIGH);
	digitalWrite(pData0, (b & (1 << 3)) == 0 ? LOW : HIGH);
}

int
recvtribble(void)
{
	unsigned char b;
	int tries = 0;

	/* drop busy */
	writedata(0);

	/* wait for strobe (stbout) from other side */
	while ((readstatus() & stbin) == 0) {
		if (++tries > 500000) {
			writedata(bsyout);
			return -1;
		}
	}

	digitalWrite(pLED, HIGH);

	/* store received byte */
	b = (readstatus() >> 3) & tribmask;

	/* raise busy/ack */
	writedata(bsyout);

	/* wait for strobe to drop on other side */
	tries = 0;
	while ((readstatus() & stbin) != 0) {
		if (++tries >= 50000) {
			digitalWrite(pLED, LOW);
			return -1;
		}
	}

	digitalWrite(pLED, LOW);

	return b;
}

int
recvbyte(void)
{
	int t;
	char c;

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
	writedata(bsyout);

	/* wait for mailstation to drop busy */
	tries = 0;
	while ((readstatus() & bsyin) != 0) {
		if (++tries >= 500000) {
			ret = -1;
			goto sendtribble_out;
		}
	}

	/* send tribble */
	writedata(b & tribmask);

	/* strobe */
	writedata((b & tribmask) | stbout);

	/* wait for ack */
	tries = 0;
	while ((readstatus() & bsyin) == 0) {
		if (++tries >= 500000) {
			ret = -1;
			goto sendtribble_out;
		}
	}

	/* unstrobe */
	writedata(0);

sendtribble_out:
	/* raise busy/ack */
	writedata(bsyout);

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
