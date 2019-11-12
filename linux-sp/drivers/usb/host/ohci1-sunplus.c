#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ohci-platform.h"

extern void usb_hcd_platform_shutdown(struct platform_device *dev);

static int ohci1_sunplus_platform_probe(struct platform_device *dev){

	dev->id = 2;

	return ohci_platform_probe(dev);
}


static const struct of_device_id ohci1_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-ohci1" },
	{ }
};

MODULE_DEVICE_TABLE(of, ohci1_sunplus_dt_ids);

static struct platform_driver ohci1_hcd_sunplus_driver = {
	.probe			= ohci1_sunplus_platform_probe,
	.remove			= ohci_platform_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ohci1-sunplus",
		.of_match_table = ohci1_sunplus_dt_ids,
#ifdef CONFIG_PM
		.pm = &ohci_sunplus_pm_ops,
#endif
	}
};

/*-------------------------------------------------------------------------*/

static int __init ohci1_sunplus_init(void)
{
	if (sp_port_enabled & PORT1_ENABLED){
		printk(KERN_NOTICE "register ohci1_hcd_sunplus_driver\n");
		return platform_driver_register(&ohci1_hcd_sunplus_driver);	
	} else {
		printk(KERN_NOTICE "warn,port1 not enable,not register ohci1 sunplus hcd driver\n");
		return -1;
	}
}
module_init(ohci1_sunplus_init);

static void __exit ohci1_sunplus_cleanup(void)
{
	platform_driver_unregister(&ohci1_hcd_sunplus_driver);
}
module_exit(ohci1_sunplus_cleanup);

MODULE_ALIAS("platform:ohci1-sunplus");
MODULE_LICENSE("GPL");

