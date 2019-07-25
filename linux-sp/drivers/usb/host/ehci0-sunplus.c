#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ehci-platform.h"

extern void usb_hcd_platform_shutdown(struct platform_device *dev);

static int ehci0_sunplus_platform_probe(struct platform_device *dev){

	dev->id = 1;

	return ehci_platform_probe(dev);
}

static const struct of_device_id ehci0_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-ehci0" },
	{ }
};

MODULE_DEVICE_TABLE(of, ehci0_sunplus_dt_ids);


struct platform_driver ehci0_hcd_sunplus_driver = {
	.probe			= ehci0_sunplus_platform_probe,
	.remove			= ehci_platform_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ehci0-sunplus",
		.of_match_table = ehci0_sunplus_dt_ids,
#ifdef CONFIG_PM
		.pm = &ehci_sunplus_pm_ops,
#endif
	}
};

/*-------------------------------------------------------------------------*/

static int __init ehci0_sunplus_init(void)
{
	if (sp_port_enabled & PORT0_ENABLED){
		printk(KERN_NOTICE "register ehci0_hcd_sunplus_driver\n");
		return platform_driver_register(&ehci0_hcd_sunplus_driver);
	} else {
		printk(KERN_NOTICE "warn,port0 not enable,not register ehci0 sunplus hcd driver\n");
		return -1;
	}
}
module_init(ehci0_sunplus_init);

static void __exit ehci0_sunplus_cleanup(void)
{
	platform_driver_unregister(&ehci0_hcd_sunplus_driver);
}
module_exit(ehci0_sunplus_cleanup);

MODULE_ALIAS("platform:ehci0-sunplus");
MODULE_LICENSE("GPL");

