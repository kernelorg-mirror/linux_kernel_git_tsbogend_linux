// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 1999, 2000 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999, 2000 Silcon Graphics, Inc.
 * Copyright (C) 2004 Christoph Hellwig.
 *	Released under GPL v2.
 *
 * Generic XTALK initialization code
 */

#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/platform_device.h>
#include <linux/platform_data/xtalk-bridge.h>
#include <asm/sn/types.h>
#include <asm/sn/klconfig.h>
#include <asm/sn/agent.h>
#include <asm/pci/bridge.h>
#include <asm/sn/pic.h>
#include <asm/xtalk/xtalk.h>


#define XBOW_WIDGET_PART_NUM	0x0
#define XXBOW_WIDGET_PART_NUM	0xd000	/* Xbow in Xbridge */
#define PIC_XBOW_WIDGET_PART_NUM 0xd100	/* Xbow(?) used in O350 (IP35) */
#define BASE_XBOW_PORT		8     /* Lowest external port */

#define PIC_BRIDGE_WIDGET_PART_NUM 0xd102 /* Xbridge(?) used in O350 (IP35) */

/* Xbow bits */
#define XBOW_REG_LINK_STAT_0        0x114
#define XBOW_REG_LINK_BLOCK_SIZE    0x40
#define XBOW_REG_LINK_ALIVE         0x80000000

struct xtalk_widget_info {
	xwidget_part_num_t partnum;
	const char *name;
	int (*probe)(nasid_t nasid, int widget, int masterwid);
};

static struct xtalk_widget_info *find_xtalk_widget_info(xwidget_part_num_t);

static int bridge_platform_create(nasid_t nasid, int widget, int masterwid)
{
	struct xtalk_bridge_platform_data *bd;
	struct platform_device *pdev;
	unsigned long offset;

	offset = NODE_OFFSET(nasid);

	// FIXME, add data about bridge via L1 infos

	bd = kzalloc(sizeof(*bd), GFP_KERNEL);
	if (!bd)
		goto no_mem;
	pdev = platform_device_alloc("xtalk-bridge", PLATFORM_DEVID_AUTO);
	if (!pdev) {
		kfree(bd);
		goto no_mem;
	}


	bd->bridge_addr = RAW_NODE_SWIN_BASE(nasid, widget);
	bd->intr_addr	= BIT_ULL(47) + 0x01800000 + PI_INT_PEND_MOD;
	bd->nasid	= nasid;
	bd->masterwid	= masterwid;

	bd->mem.name	= "Bridge PCI MEM";
	bd->mem.start	= offset + (widget << SWIN_SIZE_BITS) + BRIDGE_DEVIO0;
	bd->mem.end	= offset + (widget << SWIN_SIZE_BITS) + SWIN_SIZE - 1;
	bd->mem.flags	= IORESOURCE_MEM;
	bd->mem_offset	= offset;

	bd->io.name	= "Bridge PCI IO";
	bd->io.start	= offset + (widget << SWIN_SIZE_BITS) + BRIDGE_DEVIO0;
	bd->io.end	= offset + (widget << SWIN_SIZE_BITS) + SWIN_SIZE - 1;
	bd->io.flags	= IORESOURCE_IO;
	bd->io_offset	= offset;

	platform_device_add_data(pdev, bd, sizeof(*bd));
	platform_device_add(pdev);
	pr_info("xtalk:n%d/%x bridge widget\n", nasid, widget);
	return 0;

no_mem:
	pr_warn("xtalk:n%d/%x bridge create out of memory\n", nasid, widget);
	return -ENOMEM;
}

static int xbow_link_alive(nasid_t nasid, int link_number)
{
	u64 link_status_address;
	unsigned int link_status;

	link_status_address = (RAW_NODE_SWIN_BASE(nasid, 0) +
			       XBOW_REG_LINK_STAT_0 +
			       XBOW_REG_LINK_BLOCK_SIZE * link_number);

	link_status = readl((void *)link_status_address);

	return (link_status & XBOW_REG_LINK_ALIVE) ? 1 : 0;
}

static int xbow_probe(nasid_t nasid, int widget, int parent)
{
	unsigned int i;
	struct xtalk_widget_info *widget_info;
	widgetreg_t		widget_id;
	xwidget_part_num_t	partnum;

	if (widget != 0) {
		pr_err("xbow not at widget 0, ignoring\n");
	return -ENODEV;
	}

	for (i = 8; i < 16; i++) {
		/* XBow doesn't like parents talking to themselves */
		if (i == parent) {
			pr_info("xtalk: nasid %x widget %d is parent\n",
				nasid, i);
			continue;
		}

		/* Skip inactive links */
		if (!xbow_link_alive(nasid, i - 8))
			continue;

		widget_id = *(volatile widgetreg_t *)
				(RAW_NODE_SWIN_BASE(nasid, i) + WIDGET_ID);
		partnum = XWIDGET_PART_NUM(widget_id);

		widget_info = find_xtalk_widget_info(partnum);

		pr_info("xtalk: nasid %x widget %d: partnum 0x%x (%s)\n",
			nasid, i, partnum,
			widget_info ? widget_info->name : "unknown");

		if (widget_info && widget_info->probe)
			widget_info->probe(nasid, i, parent);
	}
	return 0;
}

static struct xtalk_widget_info known_widgets[] = {
	{
		.partnum = XBOW_WIDGET_PART_NUM,
		.name = "XBow",
		.probe = &xbow_probe,
	},
	{
		.partnum = XXBOW_WIDGET_PART_NUM,
		.name = "XBridge XBow",
		.probe = &xbow_probe,
	},
	{
		.partnum = PIC_XBOW_WIDGET_PART_NUM,
		.name = "PIC XBow",
		.probe = &xbow_probe,
	},
#if 0
	{
		.partnum = PIC_BRIDGE_WIDGET_PART_NUM,
		.name = "PIC bridge",
		.probe = &sgipic_probe,
	},
#endif
	{
		.partnum = BRIDGE_WIDGET_PART_NUM,
		.name = "Bridge",
		.probe = &bridge_platform_create,
	},
	{
		.partnum = XBRIDGE_WIDGET_PART_NUM,
		.name = "XBridge bridge",
		.probe = &bridge_platform_create,
	},
};

static struct xtalk_widget_info *
find_xtalk_widget_info(xwidget_part_num_t partnum)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(known_widgets); i++)
		if (known_widgets[i].partnum == partnum)
			return &known_widgets[i];

	return NULL;
}

static void xtalk_probe_node(nasid_t nasid)
{
	u64			hubreg;
	int			hubwidget;
	xwidget_part_num_t	partnum;
	widgetreg_t		widget_id;
	struct xtalk_widget_info *widget_info;

	hubreg = REMOTE_HUB_L(nasid, IIO_LLP_CSR);

	/* check whether the link is up */
	if (!(hubreg & IIO_LLP_CSR_IS_UP))
		return;

	hubwidget = REMOTE_HUB_L(nasid, IIO_WCR) & 15;
	pr_info("xtalk: nasid %x: master widget is %d\n", nasid, hubwidget);

	widget_id = *(volatile widgetreg_t *)
		       (RAW_NODE_SWIN_BASE(nasid, 0x0) + WIDGET_ID);
	partnum = XWIDGET_PART_NUM(widget_id);

	widget_info = find_xtalk_widget_info(partnum);

	pr_info("xtalk: nasid %x widget 0: partnum 0x%x (%s)\n",
		nasid, partnum, widget_info ? widget_info->name : "unknown");

	if (widget_info && widget_info->probe)
		widget_info->probe(nasid, 0, hubwidget);
}

static int __init xtalk_init(void)
{
	nasid_t nasid;

	for_each_online_node(nasid) {
		xtalk_probe_node(nasid);
	}

	return 0;
}

arch_initcall(xtalk_init);
