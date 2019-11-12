#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ohci-platform.h"

extern void usb_hcd_platform_shutdown(struct platform_device *dev);

static int ohci0_sunplus_platform_probe(struct platform_device *dev){

	dev->id = 1;

	return ohci_platform_probe(dev);
}


static const struct of_device_id ohci0_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-ohci0" },
	{ }
};

MODULE_DEVICE_TABLE(of, ohci0_sunplus_dt_ids);

static struct platform_driver ohci0_hcd_sunplus_driver = {
	.probe			= ohci0_sunplus_platform_probe,
	.remove			= ohci_platform_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ohci0-sunplus",
		.of_match_table = ohci0_sunplus_dt_ids,
#ifdef CONFIG_PM
		.pm = &ohci_sunplus_pm_ops,
#endif
	}
};

/*-------------------------------------------------------------------------*/

static int __init ohci0_sunplus_init(void)
{
	if (sp_port_enabled & PORT0_ENABLED){
		printk(KERN_NOTICE "register ohci0_hcd_sunplus_driver\n");
		return platform_driver_register(&ohci0_hcd_sunplus_driver);
	} else {
		printk(KERN_NOTICE "warn,port0 not enable,not register ohci0 sunplus hcd driver\n");
		return -1;
	}
}
module_init(ohci0_sunplus_init);

static void __exit ohci0_sunplus_cleanup(void)
{
	platform_driver_unregister(&ohci0_hcd_sunplus_driver);
}
module_exit(ohci0_sunplus_cleanup);

MODULE_ALIAS("platform:ohci0-sunplus");
MODULE_LICENSE("GPL");

