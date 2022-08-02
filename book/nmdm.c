#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/eventhandler.h>
#include <sys/limits.h>
#include <sys/serial.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>
#include <sys/lock.h>
#include <sys/mutex.h>

MALLOC_DEFINE(M_NMDM, "nullmodem", "nullmodem data structures");

struct nmdm_part {
	struct tty		*np_tty;
	struct nmdm_part	*np_other;
	struct task		np_task;
	struct callout		np_callout;
	int			np_dcd;
	int			np_rate;
	u_long			np_quota;
	int			np_credits;
	u_long			np_accumulator;

#define QS 8			/* Quota shift. */
};

struct nmdm_softc {
	struct nmdm_part	ns_partA;
	struct nmdm_part	ns_partB;
	struct mtx		ns_mtx;
};

static tsw_outwakeup_t		nmdm_outwakeup;
static tsw_inwakeup_t		nmdm_inwakeup;
static tsw_param_t		nmdm_param;
static tsw_modem_t		nmdm_modem;

static struct ttydevsw nmdm_class = {
	.tsw_flags =		TF_NOPREFIX,
	.tsw_outwakeup =	nmdm_outwakeup,
	.tsw_inwakeup =		nmdm_inwakeup,
	.tsw_param = 		nmdm_param,
	.tsw_modem =		nmdm_modem
};

static int nmdm_count = 0;

static void“struct nmdm_softc *ns;
{
	atomic_add_int(&nmdm_count, 1);

	ns = malloc(sizeof(*ns), M_NMDM, M_WAITOK | M_ZERO);
	mtx_init(&ns->ns_mtx, "nmdm", NULL, MTX_DEF);

	/* Connect the pairs together. */
	ns->ns_partA.np_other = &ns->ns_partB;
	TASK_INIT(&ns->ns_partA.np_task, 0, nmdm_task_tty, &ns->ns_partA);
	callout_init_mtx(&ns->ns_partA.np_callout, &ns->ns_mtx, 0);

	ns->ns_partB.np_other = &ns->ns_partA;
	TASK_INIT(&ns->ns_partB.np_task, 0, nmdm_task_tty, &ns->ns_partB);
	callout_init_mtx(&ns->ns_partB.np_callout, &ns->ns_mtx, 0);

	/* Create device nodes. */
	ns->ns_partA.np_tty = tty_alloc_mutex(&nmdm_class, &ns->ns_partA,
	    &ns->ns_mtx);
	tty_makedev(ns->ns_partA.np_tty, NULL, "nmdm%luA", unit);

	ns->ns_partB.np_tty = tty_alloc_mutex(&nmdm_class, &ns->ns_partB,
	    &ns->ns_mtx);
	tty_makedev(ns->ns_partB.np_tty, NULL, "nmdm%luB", unit);

	return (ns);
}

static void
nmdm_task_tty(void *arg, int pending __unused)
{
...
}

static struct nmdm_softc *
nmdm_alloc(unsigned long unit)
{

}

static void
nmdm_clone(void *arg, struct ucred *cred, char *name, int len,
    struct cdev **dev)
{
	unsigned long unit;
	char *end;
	struct nmdm_softc *ns;

	if (*dev != NULL)
		return;
	if (strncmp(name, "nmdm", 4) != 0)
		return;

	/* Device name must be "nmdm%lu%c", where %c is "A" or "B". */
	name += 4;
	unit = strtoul(name, &end, 10);
	if (unit == ULONG_MAX || name == end)
		return;
	if ((end[0] != 'A' && end[0] != 'B') || end[1] !=  '\0')
		return;

	ns = nmdm_alloc(unit);

	if (end[0] == 'A')
		*dev = ns->ns_partA.np_tty->t_dev;
	else
		*dev = ns->ns_partB.np_tty->t_dev;
}

static void
nmdm_outwakeup(struct tty *tp)
{
...
}

static void
nmdm_inwakeup(struct tty *tp)
{
...
}

static int
bits_per_char(struct termios *t)
{
...
}

static int
nmdm_param(struct tty *tp, struct termios *t)
{
...
}

static int
nmdm_modem(struct tty *tp, int sigon, int sigoff)
{
...
}

static int
nmdm_modevent(module_t mod __unused, int event, void *arg __unused)
{
	static eventhandler_tag tag;

	switch (event) {
	case MOD_LOAD:
		tag = EVENTHANDLER_REGISTER(dev_clone, nmdm_clone, 0, 1000);
		if (tag == NULL)
			return (ENOMEM);
		break;
	case MOD_SHUTDOWN:
		break;
	case MOD_UNLOAD:
		if (nmdm_count != 0)
			return (EBUSY);
		EVENTHANDLER_DEREGISTER(dev_clone, tag);
		break;
	default:
		return (EOPNOTSUPP);
	}

	return (0);
}

DEV_MODULE(nmdm, nmdm_modevent, NULL);

