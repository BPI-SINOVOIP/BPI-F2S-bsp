/*
 * This is a driver for the ohci controller found in Sunplus Gemini SoC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <usb.h>

#include "ohci.h"


struct sp_ohci {
	ohci_t ohci;
};

static int ohci_sunplus_probe(struct udevice *dev)
{
	struct ohci_regs *regs = (struct ohci_regs *)devfdt_get_addr(dev);

	printf("%s.%d, dev_name:%s,port_num:%d\n",__FUNCTION__, __LINE__, dev->name, dev->seq);

	return ohci_register(dev, regs);
}

static int ohci_sunplus_remove(struct udevice *dev)
{
	printf("%s.%d, dev_name:%s,port_num:%d\n",__FUNCTION__, __LINE__, dev->name, dev->seq);

	return ohci_deregister(dev);
}


static const struct udevice_id ohci_sunplus_ids[] = {
#if defined(CONFIG_ARCH_PENTAGRAM) && !defined(CONFIG_TARGET_PENTAGRAM_I143_C)
	{ .compatible = "sunplus,sp7021-usb-ohci0" },
	{ .compatible = "sunplus,sp7021-usb-ohci1" },
#elif defined(CONFIG_TARGET_PENTAGRAM_I143_P) || defined(CONFIG_TARGET_PENTAGRAM_I143_C)
	{ .compatible = "sunplus,sunplus-i143-usb-ohci0" },
	{ .compatible = "sunplus,sunplus-i143-usb-ohci1" },
#endif
	{ }
};

U_BOOT_DRIVER(ohci_sunplus) = {
	.name	 = "ohci_sunplus",
	.id	 = UCLASS_USB,
	.of_match = ohci_sunplus_ids,
	.probe = ohci_sunplus_probe,
	.remove = ohci_sunplus_remove,
	.ops	 = &ohci_usb_ops,
	.priv_auto_alloc_size = sizeof(struct sp_ohci),
	.flags  = DM_FLAG_ALLOC_PRIV_DMA,
};
