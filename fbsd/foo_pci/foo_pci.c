#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

struct foo_pci_softc {
	device_t	device;
	struct cdev	*cdev;
};

static d_open_t		foo_pci_open;
static d_close_t	foo_pci_close;
static d_read_t		foo_pci_read;
static d_write_t	foo_pci_write;

static struct cdevsw foo_pci_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	foo_pci_open,
	.d_close = 	foo_pci_close,
	.d_read = 	foo_pci_read,
	.d_write =	foo_pci_write,
	.d_name =	"foo_pci"
};

static devclass_t foo_pci_devclass;

static int
foo_pci_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	struct foo_pci_softc *sc;

	sc = dev->si_drv1;
	device_printf(sc->device, "opened successfully\n");
	return (0);
}

static int
foo_pci_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	struct foo_pci_softc *sc;

	sc = dev->si_drv1;
	device_printf(sc->device, "closed\n");
	return (0);
}

static int
foo_pci_read(struct cdev *dev, struct uio *uio, int ioflag)
{
	struct foo_pci_softc *sc;

	sc = dev->si_drv1;
	device_printf(sc->device, "read request = %luB\n", uio->uio_resid);
	return (0);
}

static int
foo_pci_write(struct cdev *dev, struct uio *uio, int ioflag)
{
	struct foo_pci_softc *sc;

	sc = dev->si_drv1;
	device_printf(sc->device, "write request = %luB\n", uio->uio_resid);
	return (0);
}

static struct _pcsid {
	uint32_t	type;
	const char	*desc;
} pci_ids[] = {
	{ 0x1234abcd, "RED PCI Widget" },
	{ 0x4321fedc, "BLU PCI Widget" },
	{ 0x00000000, NULL }
};

static int
foo_pci_probe(device_t dev)
{
	uint32_t type = pci_get_devid(dev);
	struct _pcsid *ep = pci_ids;

	while (ep->type && ep->type != type)
		ep++;

	if (ep->desc) {
		device_set_desc(dev, ep->desc);
		return (BUS_PROBE_DEFAULT);
	}

	return (ENXIO);
}

static int
foo_pci_attach(device_t dev)
{
	struct foo_pci_softc *sc = device_get_softc(dev);
	int unit = device_get_unit(dev);

	sc->device = dev;
	sc->cdev = make_dev(&foo_pci_cdevsw, unit, UID_ROOT, GID_WHEEL,
	    0600, "foo_pci%d", unit);
	sc->cdev->si_drv1 = sc;

	return (0);
}

static int
foo_pci_detach(device_t dev)
{
	struct foo_pci_softc *sc = device_get_softc(dev);

	destroy_dev(sc->cdev);
	return (0);
}

static device_method_t foo_pci_methods[] = {
	/* Device interface. */
	DEVMETHOD(device_probe,		foo_pci_probe),
	DEVMETHOD(device_attach,	foo_pci_attach),
	DEVMETHOD(device_detach,	foo_pci_detach),
	{ 0, 0 }
};

static driver_t foo_pci_driver = {
	"foo_pci",
	foo_pci_methods,
	sizeof(struct foo_pci_softc)
};

DRIVER_MODULE(foo_pci, pci, foo_pci_driver, foo_pci_devclass, 0, 0);

