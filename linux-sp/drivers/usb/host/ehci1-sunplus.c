#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ehci-platform.h"

extern void usb_hcd_platform_shutdown(struct platform_device *dev);

static int ehci1_sunplus_platform_probe(struct platform_device *dev){

	dev->id = 2;

	return ehci_platform_probe(dev);
}

static const struct of_device_id ehci1_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-ehci1" },
	{ }
};

MODULE_DEVICE_TABLE(of, ehci1_sunplus_dt_ids);


static struct platform_driver ehci1_hcd_sunplus_driver = {
	.probe			= ehci1_sunplus_platform_probe,
	.remove			= ehci_platform_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ehci1-sunplus",
		.of_match_table = ehci1_sunplus_dt_ids,
#ifdef CONFIG_PM
		.pm = &ehci_sunplus_pm_ops,
#endif
	}
};

/*-------------------------------------------------------------------------*/

static int __init ehci1_sunplus_init(void)
{
	if (sp_port_enabled & PORT1_ENABLED){
		printk(KERN_NOTICE "register ehci1_hcd_sunplus_driver\n");
		return platform_driver_register(&ehci1_hcd_sunplus_driver);
	} else {
		printk(KERN_NOTICE "warn,port1 not enable,not register ehci1 sunplus hcd driver\n");
		return -1;
	}
}
module_init(ehci1_sunplus_init);

static void __exit ehci1_sunplus_cleanup(void)
{
	platform_driver_unregister(&ehci1_hcd_sunplus_driver);
}
module_exit(ehci1_sunplus_cleanup);

MODULE_ALIAS("platform:ehci1-sunplus");
MODULE_LICENSE("GPL");

