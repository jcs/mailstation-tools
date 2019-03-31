/*
 * tribble_getty
 *
 * must be run as root to set iopl and use inb/outb
 *
 * interfaces getty(8) to the parallel port tribble routines for sending and
 * receiving bytes
 *
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <util.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/sysarch.h>
#include <machine/pio.h>

#include "tribble.h"

int
main(int argc, char *argv[])
{
	struct timeval tv = { 0 };
	char obuf[BUFSIZ];
	size_t cc, off;
	fd_set rfds;
	pid_t child;
	int master, slave;
	char b;

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

	if (openpty(&master, &slave, NULL, NULL, NULL) == -1)
		errx(1, "openpty");

	child = fork();
	if (child == 0) {
		char *argp[] = { "sh", "-c", NULL, NULL };

		close(master);
		login_tty(slave);

		argp[2] = "/usr/libexec/getty";
		execv("/bin/sh", argp);
	}

	close(STDIN_FILENO);

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(master, &rfds);
		tv.tv_usec = 10;

		if (select(master + 1, &rfds, NULL, NULL, &tv)) {
			cc = read(master, obuf, sizeof(obuf));
			if (cc == -1 && errno == EINTR)
				continue;
			if (cc <= 0)
				break;

			for (off = 0; off < cc; off++) {
#if 0
				printf("sendbyte(0x%x): %c\n", obuf[off],
					(obuf[off] >= 32 && obuf[off] <= 126 ?
					obuf[off] : ' '));
#endif
				sendbyte(obuf[off]);
			}
		}

		if (havetribble()) {
			b = recvbyte();
#if 0
			printf("recvbyte() = 0x%x: %c\n", b,
				(b >= 32 && b <= 126 ? b : ' '));
#endif
			write(master, &b, 1);
		}
	}

	close(master);
	close(slave);

	return 0;
}
