#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <asm/sn/arch.h>
#include <asm/sn/pic.h>
#include <asm/pci/bridge.h>
#include <asm/sn/intr.h>
#include <asm/irq.h>

static struct resource sgipic_pic_mem_resource = {
	.name  = "SGI PIC PCI MEM",
	.start = 0,
	.end   = ~0UL,
	.flags = IORESOURCE_MEM,
};

static struct resource sgipic_pic_io_resource = {
	.name  = "SGI PIC IO MEM",
	.start = 0,
	.end   = ~0UL,
	.flags = IORESOURCE_IO,
};

static struct resource sgipic_bridge_mem_resource = {
	.name  = "SGI BRIDGE PCI MEM",
	.start = 0,
	.end   = ~0UL,
	.flags = IORESOURCE_MEM,
};

static struct resource sgipic_bridge_io_resource = {
	.name  = "SGI BRIDGE IO MEM",
	.start = 0,
	.end   = ~0UL,
	.flags = IORESOURCE_IO,
};

static struct pci_controller template_sgipic_pci_controller = {
	.pci_ops      = &bridge_pci_ops,
	.iommu        = 0,
};

static struct bridge_controller *irq_to_pic[NR_IRQS];
static int irq_to_slot[NR_IRQS];

/*
 * use these macros to get the encoded nasid and widget id
 * from the irq value
 */
#define IRQ_TO_PIC(i)		irq_to_pic[(i)]
#define SLOT_FROM_PCI_IRQ(i)		irq_to_slot[i]

/* Startup one of the (PCI ...) IRQs routes over a bridge.  */
static unsigned int startup_sgipic_irq(struct irq_data *d)
{
	struct bridge_controller *pc;
	u64 device;
	int pin, swlevel;
	cpuid_t cpu;

	pin = SLOT_FROM_PCI_IRQ(d->irq);
	pc = IRQ_TO_PIC(d->irq);

	pr_debug("bridge_startup(): irq= 0x%x  pin=%d\n", d->irq, pin);
	/*
	 * "map" irq to a swlevel greater than 6 since the first 6 bits
	 * of INT_PEND0 are taken
	 */
	swlevel = sn_hub_irq_find_level(&cpu, d->irq);

	if (pc->is_pic) {
		/* PIC p_int_addr is very different from BRIDGE/XBRIDGE */
		pc->pic->p_int_addr[pin] = (((u64)swlevel) << 48) | 0x800001800090;
		pc->pic->p_int_enable |= (1 << pin);
		pc->pic->p_int_enable |= 0x7ffffe00;	/* more stuff in int_enable */
	} else {
		pc->base->b_int_addr[pin].addr = (0x20000 | swlevel | (pc->nasid << 8));
		pc->base->b_int_enable |= (1 << pin);
		pc->base->b_int_enable |= 0x7ffffe00;	/* more stuff in int_enable */
	}

	/*
	 * Enable sending of an interrupt clear packt to the hub on a high to
	 * low transition of the interrupt pin.
	 *
	 * IRIX sets additional bits in the address which are documented as
	 * reserved in the bridge docs.
	 */
	pc->base->b_int_mode |= (1UL << pin);

	/*
	 * We assume the bridge to have a 1:1 mapping between devices
	 * (slots) and intr pins.
	 */
	device = pc->base->b_int_device;
	device &= ~(7 << (pin*3));
	device |= (pin << (pin*3));
	pc->base->b_int_device = device;

	pc->base->b_wid_tflush;

	sn_hub_irq_connect_level(cpu, swlevel);

	return 0;	/* Never anything pending.  */
}

/* Shutdown one of the (PCI ...) IRQs routes over a bridge.  */
static void shutdown_sgipic_irq(struct irq_data *d)
{
	struct bridge_controller *pc = IRQ_TO_PIC(d->irq);
	int pin, swlevel;
	cpuid_t cpu;

	pr_debug("bridge_shutdown: irq 0x%x\n", d->irq);
	pin = SLOT_FROM_PCI_IRQ(d->irq);

	/*
	 * map irq to a swlevel greater than 6 since the first 6 bits
	 * of INT_PEND0 are taken
	 */
	swlevel = sn_hub_irq_find_level(&cpu, d->irq);
	sn_hub_irq_disconnect_level(cpu, swlevel);

	if (pc->is_pic) {
		pc->pic->p_int_enable &= ~(1 << pin);
		pc->pic->p_wid_tflush;
	} else {
		pc->base->b_int_enable &= ~(1 << pin);
		pc->base->b_wid_tflush;
	}
}

static inline void enable_sgipic_irq(struct irq_data *d)
{
	cpuid_t cpu;
	int swlevel;

	swlevel = sn_hub_irq_find_level(&cpu, d->irq);	/* Criminal offence */
	sn_hub_irq_connect_level(cpu, swlevel);
}

static inline void disable_sgipic_irq(struct irq_data *d)
{
	cpuid_t cpu;
	int swlevel;

	swlevel = sn_hub_irq_find_level(&cpu, d->irq);	/* Criminal offence */
	sn_hub_irq_disconnect_level(cpu, swlevel);
}

static struct irq_chip sgipic_irq_type = {
	.name		= "sgipic",
	.irq_startup	= startup_sgipic_irq,
	.irq_shutdown	= shutdown_sgipic_irq,
	.irq_mask	= disable_sgipic_irq,
	.irq_unmask	= enable_sgipic_irq,
};

void register_sgipic_irq(unsigned int irq)
{
	irq_set_chip_and_handler(irq, &sgipic_irq_type, handle_level_irq);
}

int request_sgipic_irq(struct bridge_controller *pc)
{
	int irq = allocate_irqno();
	int swlevel, cpu;

	if (irq < 0)
		return irq;

	/*
	 * "map" irq to a swlevel greater than 6 since the first 6 bits
	 * of INT_PEND0 are taken
	 */
	cpu = pc->irq_cpu;
	swlevel = sn_hub_irq_alloc_level(cpu, irq);
	if (unlikely(swlevel < 0)) {
		free_irqno(irq);

		return -EAGAIN;
	}

	/* Make sure it's not already pending when we connect it. */
	sn_hub_irq_clear_level(cpu, swlevel);

	sn_hub_irq_connect_level(cpu, swlevel);

	register_sgipic_irq(irq);

	return irq;
}

static int sgipic_probe_one_bridge(nasid_t nasid,
				   xwidgetnum_t widget_id,
				   xwidgetnum_t masterwid,
				   volatile struct pic *pic)
{
	unsigned long offset = NODE_OFFSET(nasid);
	struct bridge_controller *pc;
	int slot;

	pci_set_flags(PCI_PROBE_ONLY);

	ioport_resource.end = ~0UL; /* Kludge? */

	pc = kzalloc(sizeof(struct bridge_controller), GFP_KERNEL);

	pc->pc = template_sgipic_pci_controller;
	pc->mem = sgipic_pic_mem_resource;
	pc->mem.start = ((u64)pic) + 0x100000;
	pc->mem.end = ((u64)pic) + 0x7fffff;
	pc->pc.mem_offset = offset;
	pc->pc.mem_resource = &pc->mem;
	pc->io = sgipic_pic_io_resource;
	pc->io.start = ((u64)pic) + 0x100000;
	pc->io.end = ((u64)pic) + 0x7fffff;
	pc->pc.io_offset = offset;
	pc->pc.io_resource = &pc->io;

	pc->irq_cpu = smp_processor_id();
	pc->nasid = nasid;

	pc->is_pic = 1;

	pc->baddr = (u64)masterwid << 60 | PCI64_ATTR_BAR | PCI64_ATTR_SWAP;

	/*
	 * Clear all pending interrupts.
	 */
	pic->p_int_rst_stat = 0x7f; //BRIDGE_IRR_ALL_CLR;

	/*
	 * Until otherwise set up, assume all interrupts are from slot 0
	 */
	pic->p_int_device = 0x0;

	/*
	 * Configure PIO access (big windows?)
	 */
	pic->p_wid_control |= 0xc00000;
#ifdef CONFIG_PAGE_SIZE_4KB
	pic->p_wid_control &= ~0x200000; //~BRIDGE_CTRL_PAGE_SIZE;
#else /* 16kB or larger */
	pic->p_wid_control |= 0x200000; //BRIDGE_CTRL_PAGE_SIZE;
#endif

	/*
	 * Cribbed from IP27 XBRIDGE logic, apparently a mix of IRQ
	 * and DMA configuration.
	 */
	pic->p_wid_int_lower = 0x800001800090 | (((u64)masterwid) << 48);
	pic->p_dir_map = (masterwid << 20);	/* DMA */
	pic->p_int_enable = 0;

	for (slot = 0; slot < 4; slot++) {
		pic->p_device[slot] |= BRIDGE_DEV_SWAP_DIR;
		pc->pci_int[slot] = -1;
	}

	pic->p_wid_tflush; /* Wait for PIO to complete */

	pc->pic = pic;

	register_pci_controller(&pc->pc);

	return 0;
}

int sgipic_probe(nasid_t nasid, int widget_id, int masterwid)
{
	volatile struct pic *pic;

	/* Bridge 0 */
	pic = (struct pic *) RAW_NODE_SWIN_BASE(nasid, widget_id);
	sgipic_probe_one_bridge(nasid, widget_id, masterwid, pic);

	/* Bridge 1 */
	pic = (struct pic *) (RAW_NODE_SWIN_BASE(nasid, widget_id) +
			      0x800000);
	sgipic_probe_one_bridge(nasid, widget_id, masterwid, pic);

	return 0;
}

int bridge_probe(nasid_t nasid, int widget_id, int masterwid)
{
	unsigned long offset = NODE_OFFSET(nasid);
	struct bridge_controller *bc;
	bridge_t *bridge;
	int slot;

	pci_set_flags(PCI_PROBE_ONLY);

	ioport_resource.end = ~0UL; /* Kludge? */

	bc = kzalloc(sizeof(struct bridge_controller), GFP_KERNEL);

	bc->pc = template_sgipic_pci_controller;
	bc->mem = sgipic_bridge_mem_resource;
	bc->mem.start = (widget_id << 24) + 0x100000;
	bc->mem.end = (widget_id << 24) + 0xffffff;
	bc->pc.mem_offset = offset;
	bc->pc.mem_resource = &bc->mem;
	bc->io = sgipic_bridge_io_resource;
	bc->io.start = (widget_id << 24) + 0x100000;
	bc->io.end = (widget_id << 24) + 0xffffff;
	bc->pc.io_offset = offset;
	bc->pc.io_resource = &bc->io;

	bc->irq_cpu = smp_processor_id();
	//bc->widget_id = widget_id; /* Where is this used? */
	bc->nasid = nasid;

	bc->is_pic = 0;

	bc->baddr = (u64)masterwid << 60 | PCI64_ATTR_BAR | PCI64_ATTR_SWAP;

	/*
	 * point to this bridge
	 */
	bridge = (bridge_t *) RAW_NODE_SWIN_BASE(nasid, widget_id);

	/*
	 * Clear all pending interrupts.
	 */
	bridge->b_int_rst_stat = BRIDGE_IRR_ALL_CLR;

	/*
	 * Until otherwise set up, assume all interrupts are from slot 0
	 */
	bridge->b_int_device = 0x0;

	/*
	 * swap pio's to pci mem and io space (big windows)
	 */
	bridge->b_wid_control |= BRIDGE_CTRL_IO_SWAP |
				 BRIDGE_CTRL_MEM_SWAP;
#ifdef CONFIG_PAGE_SIZE_4KB
	bridge->b_wid_control &= ~BRIDGE_CTRL_PAGE_SIZE;
#else /* 16kB or larger */
	bridge->b_wid_control |= BRIDGE_CTRL_PAGE_SIZE;
#endif

	/*
	 * Hmm...  IRIX sets additional bits in the address which
	 * are documented as reserved in the bridge docs.
	 */
	bridge->b_wid_int_upper = 0x8000 | (masterwid << 16);
	bridge->b_wid_int_lower = 0x01800090;	/* PI_INT_PEND_MOD off*/
	bridge->b_dir_map = (masterwid << 20);	/* DMA */
	bridge->b_int_enable = 0;

	for (slot = 0; slot < 8; slot++) {
		bridge->b_device[slot].reg |= BRIDGE_DEV_SWAP_DIR;
		bc->pci_int[slot] = -1;
	}
	bridge->b_wid_tflush;	  /* wait until Bridge PIO complete */

	bc->base = bridge;

	register_pci_controller(&bc->pc);

	return 0;
}

int pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	return 0;
}

static inline struct pci_dev *bridge_root_dev(struct pci_dev *dev)
{
	while (dev->bus->parent) {
		/* Move up the chain of bridges. */
		dev = dev->bus->self;
	}

	return dev;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	struct bridge_controller *pc = dev->bus->sysdata;
	struct pci_dev *rdev = bridge_root_dev(dev);
	int slot;
	int irq;

	/*
	 * The main (only?) thing we need to do here is set up an IRQ
	 * mapping for the device.
	 */

	if (pc->is_pic) {
		slot = PCI_SLOT(rdev->devfn)-1;
	} else {
		slot = PCI_SLOT(rdev->devfn);
	}

	irq = pc->pci_int[slot];
	if (irq == -1) {
		irq = request_sgipic_irq(pc);
		if (irq < 0)
			return irq;

		pc->pci_int[slot] = irq;
	}

	irq_to_pic[irq] = pc;
	irq_to_slot[irq] = slot;

	dev->irq = irq;

	return 0;
}

/*
 * Device might live on a subordinate PCI bus.	XXX Walk up the chain of buses
 * to find the slot number in sense of the bridge device register.
 * XXX This also means multiple devices might rely on conflicting bridge
 * settings.
 */

static inline void pci_disable_swapping(struct pci_dev *dev)
{
	struct bridge_controller *bc = BRIDGE_CONTROLLER(dev->bus);
	bridge_t *bridge = bc->base;
	int slot = PCI_SLOT(dev->devfn);

	/* Turn off byte swapping */
	bridge->b_device[slot].reg &= ~BRIDGE_DEV_SWAP_DIR;
	bridge->b_widget.w_tflush;	/* Flush */
}

static inline void pci_enable_swapping(struct pci_dev *dev)
{
	struct bridge_controller *bc = BRIDGE_CONTROLLER(dev->bus);
	bridge_t *bridge = bc->base;
	int slot = PCI_SLOT(dev->devfn);

	/* Turn on byte swapping */
	bridge->b_device[slot].reg |= BRIDGE_DEV_SWAP_DIR;
	bridge->b_widget.w_tflush;	/* Flush */
}

static void pci_fixup_ioc3(struct pci_dev *d)
{
	pci_disable_swapping(d);
}

DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_SGI, PCI_DEVICE_ID_SGI_IOC3,
	pci_fixup_ioc3);
