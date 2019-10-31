/*
 * recvdump
 * based on win32/maildump.cpp by FyberOptic
 *
 * must be run as root to set iopl and use inb/outb
 *
 * assumes codedump or datadump has been loaded on the Mailstation and is
 * running
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/limits.h>
#include <machine/sysarch.h>

#include "tribble.h"

void
usage(void)
{
	printf("usage: %s [-d] [-p port address] <code|data|mem>\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	FILE *pFile;
	unsigned int received = 0, expected = 0;
	int ch, b, codeflash = 0, dataflash = 0, mem = 0;
	char fn[14];

	while ((ch = getopt(argc, argv, "dp:")) != -1) {
		switch (ch) {
		case 'd':
			tribble_debug = 1;
			break;
		case 'p':
			tribble_port = (unsigned)strtol(optarg, NULL, 0);
			if (errno)
				err(1, "invalid port value");
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (strncmp((char *)argv[0], "code", 5) == 0) {
		if (dataflash || mem)
			usage();
		codeflash = 1;
	} else if (strncmp((char *)argv[0], "data", 5) == 0) {
		if (codeflash || mem)
			usage();
		dataflash = 1;
	} else if (strncmp((char *)argv[0], "mem", 4) == 0) {
		if (codeflash || dataflash)
			usage();
		mem = 1;
	} else
		errx(1, "unknown dump parameter: %s\n", argv[0]);

	if (codeflash) {
		expected = 1024 * 1024;
		strlcpy(fn, "codeflash.bin", sizeof(fn));
	} else if (dataflash) {
		expected = 1024 * 512;
		strlcpy(fn, "dataflash.bin", sizeof(fn));
	} else if (mem) {
		expected = (1024 * 64) - 1;
		strlcpy(fn, "mem.bin", sizeof(fn));
	} else
		usage();

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
#endif

	pFile = fopen(fn, "wb");
	if (!pFile) {
		printf("couldn't open file %s\n", fn);
		return 1;
	}

	printf("[port 0x%x] dumping to %s, run Code Dump on Mailstation...",
	    tribble_port, fn);
	fflush(stdout);

	while (received < expected) {
		if ((b = recvbyte()) == -1)
			continue;

		fputc(b & 0xff, pFile);
		fflush(pFile);

		if (received++ == 0)
			printf("\n");

		if (received % 1024 == 0 || received == expected) {
			printf("\rreceived: %07d/%07d", received, expected);
			fflush(stdout);
		}
	}
	fclose(pFile);

	printf("\n");

	return 0;
}
