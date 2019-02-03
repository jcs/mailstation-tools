/*
 *	Maildump.cpp
 *	------------
 *
 *	This should dump your Mailstation's codeflash, in combination
 *	with Cyrano Jones' "spew.txt" app for the Mailstation, and
 *	a laplink cable of course.
 *
 *	Basic use would be as follows:
 *	1.) connect laplink cable to MS and PC
 *	2.) turn on MS
 *	3.) run maildump on PC
 *	4.) run spew on MS
 *
 *	The MS should be on first to ensure parallel port signals are
 *	reset, otherwise your dump could be bad.  If you think you
 *	did get a bad dump, it wouldn't hurt to turn the MS off and
 *	on again, waiting for the main menu to show up, just to make
 *	sure it resets itself before you start listening for data.
 *
 *	The dump should be 1,048,576 bytes (1MB), and named "ms.bin",
 *	which is automatically overwritten everytime you run the
 *	program.
 *
 *	This is based around Cyrano Jones' unit_tribbles in Pascal,
 *	but written to work under Windows (tested under XP) thanks
 *	to inpout32.dll, which doesn't require a separate driver
 *	to interface with I/O ports.  It's pretty useful!
 *
 *	Spew.txt can be gotten from the Yahoo Mailstation group's file
 *	section:
 *	http://tech.groups.yahoo.com/group/mailstation/files/Mailbug/
 *
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

short  Inp32 (short portaddr)
{
	return (inp32fp)(portaddr);
}

void  Out32 (short portaddr, short datum)
{
	(oup32fp)(portaddr,datum);
}


int InitIOLibrary()
{
	HINSTANCE hLib;

	hLib = LoadLibrary("inpout32.dll");

	if (hLib == NULL) {
		fprintf(stderr,"LoadLibrary Failed.\n");
		return -1;
	}

	inp32fp = (inpfuncPtr) GetProcAddress(hLib, "Inp32");

	if (inp32fp == NULL) {
		fprintf(stderr,"GetProcAddress for Inp32 Failed.\n");
		return -1;
	}

	oup32fp = (oupfuncPtr) GetProcAddress(hLib, "Out32");

	if (oup32fp == NULL) {
		fprintf(stderr,"GetProcAddress for Oup32 Failed.\n");
		return -1;
	}
}



unsigned char recvtribble()
{
	Out32(DATA,0);  // drop busy/ack
	while ((Inp32(STATUS) & stbin) != 0) {}	 // wait for (inverted) strobe
	unsigned char mytribble = (Inp32(STATUS) >> 3) & tribmask;       // grab tribble
	Out32(DATA,bsyout);	     // raise busy/ack
	while ((Inp32(STATUS) & stbin) == 0) {}	 // wait for (inverted) UNstrobe

	return mytribble;
}



int main(int ARGC, void **ARGV)
{

	if (!InitIOLibrary()) { printf("Failed to initialize port I/O library\n"); return -1; }

	FILE * pFile;

	pFile = fopen ("ms.bin" , "wb");
	if (!pFile)
	{
		printf("Couldn't open file\n");
		return -1;
	}

	unsigned char bytereceived;
	int totalbytesreceived = 0;

	printf("Waiting...");

	while (totalbytesreceived < (1024*1024))	// Fetch 1MB
	{
		bytereceived = recvtribble() + (recvtribble() << 3) + ((recvtribble() & dibmask) << 6);
		fputc ( bytereceived, pFile );
		totalbytesreceived++;
		printf("\rReceived: %d bytes (%d%%)	     ",
			totalbytesreceived, (int)floor(((float)totalbytesreceived / (1024*1024))*100));
	}
	fclose (pFile);

	printf("\n");
	return 0;

}

