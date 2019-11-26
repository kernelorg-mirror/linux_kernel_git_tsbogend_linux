// SPDX-License-Identifier: GPL-2.0-only

#include <linux/pci.h>

#include <asm/sn/ioc3.h>

static void ip29_fixup_phy(struct pci_dev *dev)
{
	int nasid = pcibus_to_node(dev->bus);
	u32 sid;

	if (nasid != 1)
		return; /* only needed on second module */

	/* enable ethernet PHY on IP29 systemboard */
	pci_read_config_dword(dev, PCI_SUBSYSTEM_VENDOR_ID, &sid);
	if (sid == (PCI_VENDOR_ID_SGI | (IOC3_SUBSYS_IP29_SYSBOARD) << 16))
		REMOTE_HUB_S(nasid, MD_LED0, 0x09);
}

DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_SGI, PCI_DEVICE_ID_SGI_IOC3,
			ip29_fixup_phy);
