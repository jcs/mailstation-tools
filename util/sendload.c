/*
 * sendload
 *
 * must be run as root to set iopl and use inb/outb
 *
 * assumes Loader has been loaded on the Mailstation and is running
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/sysarch.h>
#include <machine/pio.h>

#include "tribble.h"

void
usage(void)
{
	printf("usage: %s [-dr] [-p port address] <file to send>\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	FILE *pFile;
	struct stat sb;
	unsigned int sent = 0, size = 0, raw = 0;
	int ch;
	char *fn;

	while ((ch = getopt(argc, argv, "dp:r")) != -1) {
		switch (ch) {
		case 'd':
			tribble_debug = 1;
			break;
		case 'p':
			tribble_port = (unsigned)strtol(optarg, NULL, 0);
			if (errno)
				err(1, "invalid port value");
			break;
		case 'r':
			raw = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
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

	fn = argv[0];
	pFile = fopen(fn, "rb");
	if (!pFile)
		err(1, "open: %s", fn);

	if (fstat(fileno(pFile), &sb) != 0)
		err(1, "fstat: %s", fn);

	/* we're never going to send huge files */
	size = (unsigned int)sb.st_size;

	printf("[port 0x%x] sending %s (%d bytes)...", tribble_port, fn, size);
	fflush(stdout);

	/* loader expects two bytes, the low and then high of the file size */
	if (!raw) {
		if (sendbyte(size & 0xff) != 0)
			errx(1, "sendbyte failed");
		if (sendbyte((size >> 8) & 0xff) != 0)
			errx(1, "sendbyte failed");
	}

	while (sent < size) {
		if (sendbyte(fgetc(pFile)) != 0)
			errx(1, "sendbyte failed at %d/%d", sent, size);

		if (sent++ == 0)
			printf("\n");

		if (sent % (raw ? 64 : 1024) == 0 || sent == size) {
			printf("\rsent: %07d/%07d", sent, size);
			fflush(stdout);
		}
	}
	fclose(pFile);

	printf("\n");

	return 0;
}
