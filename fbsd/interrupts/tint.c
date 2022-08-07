#include <sys/types.h>
#include <machine/cpufunc.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BASE_ADDRESS	0x378

int
main(int argc, char *argv[])
{
	int fd;

	fd = open("/dev/io", O_RDWR);
	if (fd < 0)
		err(1, "open(/dev/io)");

	outb(BASE_ADDRESS, 0x00);
	outb(BASE_ADDRESS, 0xff);
	outb(BASE_ADDRESS, 0x00);

	close(fd);
	return (0);
}

