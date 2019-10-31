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
#include <stdlib.h>
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

void
usage(void)
{
	printf("usage: %s [-d] [-p port address]\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct timeval tv = { 0 };
	unsigned char obuf[2];
	size_t cc = 0;
	fd_set rfds;
	pid_t child;
	int master, slave, ch, b;

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

	if (argc != 0)
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

	printf("[port 0x%x]\n", tribble_port);

	if (openpty(&master, &slave, NULL, NULL, NULL) == -1)
		errx(1, "openpty");

	child = fork();
	if (child == 0) {
		char *argp[] = { "sh", "-c", "/usr/libexec/getty", NULL };

		close(master);
		login_tty(slave);

		setenv("TERM", "vt100", 1);
		setenv("LINES", "15", 1);
		setenv("COLUMNS", "64", 1);

		execv("/bin/sh", argp);
	}

	close(STDIN_FILENO);

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(master, &rfds);
		tv.tv_usec = 10;

		if (cc == 0 && select(master + 1, &rfds, NULL, NULL, &tv)) {
			cc = read(master, obuf, 1);
			if (cc == -1 && errno == EINTR)
				continue;
			if (cc <= 0)
				break;
		}

		if (cc > 0 && (sendbyte(obuf[0]) == 0)) {
			cc--;
#if 0
			printf("sendbyte(0x%x): %c\n", obuf[0],
				(obuf[0] >= 32 && obuf[0] <= 126 ?
				obuf[0] : ' '));
#endif
		}

		if (havetribble()) {
			b = recvbyte();
			if (b < 0) {
				printf("recvbyte() failed\n");
				continue;
			}
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
