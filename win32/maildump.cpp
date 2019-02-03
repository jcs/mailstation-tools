/*
 *	Maildump.cpp
 *	------------
 *
 *	This should dump your Mailstation's codeflash or dataflash, in
 *	combination with the codedump.asm or datadump.asm app for the
 *	Mailstation, and a laplink cable of course.
 *
 *	Basic use would be as follows:
 *	1.) connect laplink cable to MS and PC
 *	2.) turn on MS
 *	3.) run "maildump /code" on PC
 *	4.) run codedump on MS
 *
 *	The MS should be on first to ensure parallel port signals are
 *	reset, otherwise your dump could be bad.  If you think you
 *	did get a bad dump, it wouldn't hurt to turn the MS off and
 *	on again, waiting for the main menu to show up, just to make
 *	sure it resets itself before you start listening for data.
 *
 *	A dump of the codeflash should be 1,048,576 bytes (1MB), and named
 *	"codeflash.bin", which is automatically overwritten everytime you run
 *	the program.
 *
 *	A dump of the dataflash should be 524,288 bytes (512Kb), and named
 *	"dataflash.bin", which is automatically overwritten everytime you run
 *	the program.
 *
 *	This is based around Cyrano Jones' unit_tribbles in Pascal,
 *	but written to work under Windows (tested under 98 and XP) thanks
 *	to inpout32.dll, which doesn't require a separate driver
 *	to interface with I/O ports.  It's pretty useful!
 *
 *	- FyberOptic (fyberoptic@...)
 *
 */

#include <windows.h>
#include <stdio.h>
#include <math.h>

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

typedef short (_stdcall *inpfuncPtr)(short portaddr);
typedef void (_stdcall *oupfuncPtr)(short portaddr, short datum);

inpfuncPtr inp32fp;
oupfuncPtr oup32fp;

short
Inp32(short portaddr)
{
	return (inp32fp)(portaddr);
}

void
Out32(short portaddr, short datum)
{
	(oup32fp)(portaddr,datum);
}

int
InitIOLibrary(void)
{
	HINSTANCE hLib;

	hLib = LoadLibrary("inpout32.dll");

	if (hLib == NULL) {
		fprintf(stderr,"LoadLibrary Failed.\n");
		return -1;
	}

	inp32fp = (inpfuncPtr)GetProcAddress(hLib, "Inp32");

	if (inp32fp == NULL) {
		fprintf(stderr,"GetProcAddress for Inp32 Failed.\n");
		return -1;
	}

	oup32fp = (oupfuncPtr)GetProcAddress(hLib, "Out32");

	if (oup32fp == NULL) {
		fprintf(stderr,"GetProcAddress for Oup32 Failed.\n");
		return -1;
	}
}

unsigned char
recvtribble(void)
{
	unsigned char mytribble;

	// drop busy/ack
	Out32(DATA,0);

	// wait for (inverted) strobe
	while ((Inp32(STATUS) & stbin) != 0)
		;

	// grab tribble
	mytribble = (Inp32(STATUS) >> 3) & tribmask;

	// raise busy/ack
	Out32(DATA,bsyout);

	// wait for (inverted) UNstrobe
	while ((Inp32(STATUS) & stbin) == 0)
		;

	return mytribble;
}

int
main(int argc, void **argv)
{
	FILE *pFile;
	unsigned int received = 0, expected = 0;
	unsigned char t1, t2, t3, b;
	char fn[14];
	int codeflash = 0, dataflash = 0;
	int x;

	for (x = 1; x < argc; x++) {
		if (strcmp((char *)argv[x], "/code") == 0)
			codeflash = 1;
		else if (strcmp((char *)argv[x], "/data") == 0)
			dataflash = 1;
		else
			printf("unknown parameter: %s\n", argv[x]);
	}

	if (codeflash == dataflash) {
		printf("usage: %s [/code | /data]\n", argv[0]);
		return 1;
	}

	if (codeflash) {
		expected = 1024 * 1024;
		strncpy(fn, "codeflash.bin", sizeof(fn));
	} else if (dataflash) {
		expected = 1024 * 512;
		strncpy(fn, "dataflash.bin", sizeof(fn));
	}

	if (!InitIOLibrary()) {
		printf("Failed to initialize port I/O library\n");
		return -1;
	}
	
	pFile = fopen(fn, "wb");
	if (!pFile) {
		printf("couldn't open file %s\n", fn);
		return -1;
	}

	printf("waiting to dump to %s...", fn);

	while (received < expected)	{
		t1 = recvtribble();
		t2 = recvtribble();
		t3 = recvtribble();

		b = t1 + (t2 << 3) + ((t3 & dibmask) << 6);

		if (received == 0)
			printf("\n");

		if (0) {
			printf("[%08d] received 0x%x, 0x%x, 0x%x = 0x%x\r\n", received, t1, t2, t3, b);
		}

		fputc(b, pFile);
		received++;

		if (received % 1024 == 0 || received == expected)
			printf("\rreceived: %07d/%07d", received, expected);
	}
	fclose(pFile);

	printf("\n");

	return 0;
}

