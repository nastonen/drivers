#include <dev/wscons/wsconsio.h>
#include <sys/ioctl.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wsfont.h"

static int do_help(void);
static int do_list(void);
static int do_set(int argc, char* argv[]);
static int do_dump(int argc, char* argv[]);
static int wr_font(struct wsdisplay_font* wsf, char* path);
static void pr_font(struct wsdisplay_font* wsf);
static int set_font(char* fname, struct wsdisplay_font* wsf, char* cdev);
static char* bord2s(int b_order);
static char* enc2s(int encoding);

static char* prog;

int
main(int argc, char* argv[])
{
	char* cmd;
	int rc = EXIT_FAILURE;

	prog = argv[0];
	if (argc == 1) {
		rc = do_help();
		exit(rc);
	}

	cmd = argv[1];
	if (strcmp(cmd, "help") == 0)
		rc = do_help();
	else if (strcmp(cmd, "list") == 0)
		rc = do_list();
	else if (strcmp(cmd, "dump") == 0)
		rc = do_dump(argc, argv);
	else if (strcmp(cmd, "set") == 0)
		rc = do_set(argc, argv);
	else
		warnx("%s: unknown command.\nTry: help\n", cmd);
	return rc;
}

static int
do_help(void)
{
	printf(
	"Available commands:\n"
	"  list:            List compiled-in fonts\n"
	"\n"
	"  dump DIR:        Dump fonts into \"DIR\" (which must exist)\n"
	"                   eg.: wsfont dump /usr/share/wscons/fonts\n"
	"\n"
	"  set FONT [DEV]:  Load & set \"FONT\" on console \"DEV\" (or current)\n"
	"                   eg.: wsfont set Go_Mono_12x23 /dev/ttyE0\n"
	"                   eg.: wsfont set Go_Mono_12x23\n"
	"\n"
	"  help:            This list\n");
	return EXIT_SUCCESS;
}

static int
do_list(void)
{
	int i, rc = EXIT_FAILURE;

	for (i = 0; wsfonts[i].fname != NULL; i++) {
		if (i == 0) {	/* print heading */
			printf("Compiled in fonts:\n");
			printf("%-22s %-16s ", "Fontname", "Int. Name");
			printf("%3s %3s %-5s ", "1ch", "Nch", "Enc");
			printf("%3s %3s %3s ", "Wid", "Hei", "Str");
			printf("%-2s %-2s %6s\n", "bO", "BO", "Size");
		}
		printf("%-22s", wsfonts[i].fname);
		pr_font(wsfonts[i].wsdfont);
	}
	if (i == 0)
		warnx("error: no fonts compiled in!");
	else
		rc = EXIT_SUCCESS;
	return rc;
}

static void
pr_font(struct wsdisplay_font* wsf)
{
	printf(" %-16s %3d %3d", wsf->name, wsf->firstchar, wsf->numchars);
	printf(" ");
	printf("%-5s", enc2s(wsf->encoding));
	printf(" %3d %3d %3d", wsf->fontwidth, wsf->fontheight, wsf->stride);
	printf(" %-2s", bord2s(wsf->bitorder));
	printf(" %-2s", bord2s(wsf->byteorder));
	printf(" %6u\n", wsf->fontheight * wsf->stride * wsf->numchars);
}

static char*
enc2s(int encoding)
{
	switch (encoding) {
	case WSDISPLAY_FONTENC_ISO: return "iso";
	case WSDISPLAY_FONTENC_IBM: return "ibm";
	case WSDISPLAY_FONTENC_PCVT: return "pcvt";
	case WSDISPLAY_FONTENC_ISO7: return "iso7";
	case WSDISPLAY_FONTENC_ISO2: return "iso2";
	case WSDISPLAY_FONTENC_KOI8_R: return "koi8r";
	default: return "?";
	}
}

static char*
bord2s(int b_order)
{
	switch (b_order) {
	case WSDISPLAY_FONTORDER_KNOWN: return "K";
	case WSDISPLAY_FONTORDER_L2R: return "LR";
	case WSDISPLAY_FONTORDER_R2L: return "RL";
	default: return "?";
	}
}

static int
do_dump(int argc, char* argv[])
{
	int i, rc = EXIT_FAILURE;
	char* dir, *p;

	if (argc != 3) {
		warnx("Usage: %s dump DIR", prog);
		return rc;
	}
	dir = argv[2];
	if (*dir == '\0') {
		warnx("error: empty directory name");
		return rc;
	}

	p = dir + strlen(dir);
	while (--p >= dir && *p == '/')
		*p = '\0';
	rc = EXIT_SUCCESS;
	for (i = 0; wsfonts[i].fname != NULL; i++) {
		char path[PATH_MAX];
		snprintf(path, sizeof path, "%s/%s.fnt", dir, wsfonts[i].fname);
		rc = wr_font(wsfonts[i].wsdfont, path);
		if (rc != EXIT_SUCCESS)
			break;
	}

	return rc;
}

static int
wr_font(struct wsdisplay_font* wsf, char* path)
{
	char buf[4 + 64];
	int fd, n, rc = EXIT_FAILURE;

	if ((fd = open(path, O_WRONLY | O_CREAT, 0644)) == -1) {
		warn("%s: open failed", path);
		return rc;
	}
	memset(buf, 0, sizeof buf);
	memcpy(buf, "WSFT", 4);
	strcpy(buf + 4, wsf->name);
	n = sizeof buf;
	if (write(fd, buf, n) != n) {
		warn("%s: write failed", path);
		unlink(path);
		goto out;
	}
	n = htole32(wsf->firstchar); write(fd, &n, 4);
	n = htole32(wsf->numchars); write(fd, &n, 4);
	n = htole32(wsf->encoding); write(fd, &n, 4);
	n = htole32(wsf->fontwidth); write(fd, &n, 4);
	n = htole32(wsf->fontheight); write(fd, &n, 4);
	n = htole32(wsf->stride); write(fd, &n, 4);
	n = htole32(wsf->bitorder); write(fd, &n, 4);
	n = htole32(wsf->byteorder); write(fd, &n, 4);
	n =  wsf->numchars * wsf->fontheight * wsf->stride;
	if (write(fd, wsf->data, n) != n) {
		warn("%s: write failed", path);
		unlink(path);
	} else
		rc = EXIT_SUCCESS;
out:
	close(fd);
	return rc;
}

static int
do_set(int argc, char* argv[])
{
	char* cdev, *fname;
	int i, rc = EXIT_FAILURE;

	if (argc < 3) {
		warnx("Usage: %s set fontname console_device", prog);
		return rc;
	}

	fname = argv[2];
	if (argc == 4)
		cdev = argv[3];
	else {		/* set font on current tty */
		cdev = ttyname(STDIN_FILENO);
		if (cdev == NULL) {
			warn("ttyname failed");
			return rc;
		}
	}
	for (i = 0; wsfonts[i].fname != NULL; i++) {
		if (strcmp(wsfonts[i].fname, fname) == 0) {
			rc = set_font(fname, wsfonts[i].wsdfont, cdev);
			break;
		}
	}

	return rc;
}

static int
set_font(char* fname, struct wsdisplay_font* wsf, char* cdev)
{
	int fd, rc = EXIT_FAILURE;
	char* cmd = NULL;

	if ((fd = open("/dev/wsfont", O_RDWR)) < 0) {
		warn("/dev/wsfont: open failed");
		return rc;
	}
	wsf->name = fname;	/* VT220 has multiple fonts with same name */
	if (ioctl(fd, WSDISPLAYIO_LDFONT, wsf) < 0) {
		if (errno == EEXIST)
			warnx("%s: font already loaded", wsf->name);
		else {
			warn("ioctl WSDISPLAYIO_LDFONT failed");
			return rc;
		}
	}
	close(fd);
	if (asprintf(&cmd, "/sbin/wsconsctl -f \"%s\" -dw font=\"%s\"",
	    cdev, wsf->name) == -1) {
		warn("asprintf failed");
		return rc;
	}
	if (system(cmd) != 0)
		warn("warning: %s: cmd failed", cmd);
	else
		rc = EXIT_SUCCESS;
	free(cmd);

	return rc;
}
