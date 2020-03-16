/*
 * sendload
 *
 * must be run as root to set iopl and use inb/outb
 *
 * assumes Loader has been loaded on the Mailstation and is running
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tribble.h"

static int serial_fd = -1;
static int serial_speed = B115200;

void
usage(void)
{
	errx(1, "usage: %s [-dr] [-p lpt address | serial device] "
	    "[-s serial speed] <file to send>", getprogname());
}

int
sendbyte_shim(char b)
{
	if (serial_fd >= 0)
		return (write(serial_fd, &b, 1) != 1);
	else
		return sendbyte(b);
}

void
setupserial(speed_t speed)
{
	struct termios tty;

	if (tcgetattr(serial_fd, &tty) < 0)
		err(1, "tcgetattr");

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag |= (CLOCAL | CREAD);	/* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;			/* 8-bit characters */
	tty.c_cflag &= ~PARENB;			/* no parity bit */
	tty.c_cflag &= ~CSTOPB;			/* only need 1 stop bit */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(serial_fd, TCSANOW, &tty) != 0)
		err(1, "tcsetattr");
}

int
main(int argc, char *argv[])
{
	FILE *pFile;
	struct stat sb;
	unsigned int sent = 0, size = 0, raw = 0;
	int ch;
	char *fn, *serial_dev = NULL;

	while ((ch = getopt(argc, argv, "dp:rs:")) != -1) {
		switch (ch) {
		case 'd':
			tribble_debug = 1;
			break;
		case 'p':
			if (optarg[0] == '/') {
				if ((serial_dev = strdup(optarg)) == NULL)
					err(1, "strdup");
				serial_fd = open(serial_dev,
				    O_RDWR | O_NOCTTY | O_SYNC);
				if (serial_fd < 0)
					err(1, "can't open %s", optarg);
			} else {
				tribble_port = (unsigned)strtol(optarg, NULL,
				    0);
				if (errno)
					err(1, "invalid port value");
			}
			break;
		case 'r':
			raw = 1;
			break;
		case 's':
			serial_speed = (unsigned)strtol(optarg, NULL, 0);
			if (errno)
				err(1, "invalid serial port speed value");
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (serial_fd >= 0)
		setupserial(serial_speed);
	else
		checkio();

	fn = argv[0];
	pFile = fopen(fn, "rb");
	if (!pFile)
		err(1, "open: %s", fn);

	if (fstat(fileno(pFile), &sb) != 0)
		err(1, "fstat: %s", fn);

	/* we're never going to send huge files */
	size = (unsigned int)sb.st_size;

	if (serial_fd >= 0)
		printf("[%s]", serial_dev);
	else
		printf("[lpt port 0x%x]", tribble_port);

	printf(" sending %s (%d bytes)...", fn, size);
	fflush(stdout);

	/* loader expects two bytes, the low and then high of the file size */
	if (!raw) {
		if (sendbyte_shim(size & 0xff) != 0)
			errx(1, "sendbyte failed");
		if (sendbyte_shim((size >> 8) & 0xff) != 0)
			errx(1, "sendbyte failed");
	}

	while (sent < size) {
		if (sendbyte_shim(fgetc(pFile)) != 0)
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
