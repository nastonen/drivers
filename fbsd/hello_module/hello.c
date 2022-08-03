

/*
 * Skeleton KLD
 * Inspired by FreeBSD man pages
 */

#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

static int
hello_modevent(module_t mod __unused, int event, void *arg __unused)
{
	int err = 0;

	switch (event) {
	case MOD_LOAD:
		uprintf("Hello, world!\n");
		break;
	case MOD_UNLOAD:
		uprintf("Good-bye, world!\n");
		break;
	default:
		err = EOPNOTSUPP;
		break;
	}

	return (err);
}

static moduledata_t hello_mod = {
 	"hello",
	hello_modevent,
	NULL
};

DECLARE_MODULE(hello, hello_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);

