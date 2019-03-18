/*
 * sendload
 *
 * must be run as root to set iopl and use inb/outb
 *
 * assumes parallel port is at PORTADDRESS and Loader has been loaded on the
 * Mailstation and is running
 */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/sysarch.h>
#include <machine/pio.h>

#include "tribble.h"

int
main(int argc, char *argv[])
{
	FILE *pFile;
	struct stat sb;
	unsigned int sent = 0, size = 0;
	char *fn;

	if (argc != 2) {
		printf("usage: %s <binary file to send>\n", argv[0]);
		return 1;
	}

	if (geteuid() != 0)
		errx(1, "must be run as root");

#ifdef __OpenBSD__
	if (amd64_iopl(1) != 0)
		errx(1, "amd64_iopl failed (is machdep.allowaperture=1?)");
#endif

	fn = argv[1];
	pFile = fopen(fn, "rb");
	if (!pFile)
		err(1, "open: %s", fn);

	if (fstat(fileno(pFile), &sb) != 0)
		err(1, "fstat: %s", fn);

	/* we're never going to send huge files */
	size = (unsigned int)sb.st_size;

	printf("sending %s (%d bytes)...", fn, size);
	fflush(stdout);

	/* loader expects two bytes, the low and then high of the file size */
	sendbyte(size & 0xff);
	sendbyte((size >> 8) & 0xff);

	while (sent < size) {
		sendbyte(fgetc(pFile));

		if (sent++ == 0)
			printf("\n");

		if (sent % 1024 == 0 || sent == size) {
			printf("\rsent: %07d/%07d", sent, size);
			fflush(stdout);
		}
	}
	fclose(pFile);

	printf("\n");

	return 0;
}
