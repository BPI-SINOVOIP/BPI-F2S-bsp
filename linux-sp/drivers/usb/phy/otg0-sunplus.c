#include <linux/module.h>

#include <linux/platform_device.h>
#include "sunplus-otg.h"
#include <linux/usb/sp_usb.h>

struct sp_otg *sp_otg0_host = NULL;
EXPORT_SYMBOL(sp_otg0_host);

static const struct of_device_id otg0_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sunplus-q628-usb-otg0" },
	{ }
};

MODULE_DEVICE_TABLE(of, otg0_sunplus_dt_ids);

static struct platform_driver sunplus_usb_otg0_driver = {
	.probe		= sp_otg_probe,
	.remove		= sp_otg_remove,
	.suspend	= sp_otg_suspend,
	.resume		= sp_otg_resume,
	.driver		= {
		.name	= "sunplus-usb-otg0",
		.of_match_table = otg0_sunplus_dt_ids,
	},
};

static int __init usb_otg0_sunplus_init(void)
{
	if (sp_port_enabled & PORT0_ENABLED) {
		printk(KERN_NOTICE "register sunplus_usb_otg0_driver\n");
		return platform_driver_register(&sunplus_usb_otg0_driver);	
	} else {
		printk(KERN_NOTICE "otg0 not enabled\n");
		return 0;
	}
}
fs_initcall(usb_otg0_sunplus_init);

static void __exit usb_otg0_sunplus_exit(void)
{
	if (sp_port_enabled & PORT0_ENABLED) {
		printk(KERN_NOTICE "unregister sunplus_usb_otg0_driver\n");
		platform_driver_unregister(&sunplus_usb_otg0_driver);
	} else {
		printk(KERN_NOTICE "otg0 not enabled\n");
		return;
	}
}
module_exit(usb_otg0_sunplus_exit);

MODULE_ALIAS("sunplus_usb_otg0");
MODULE_AUTHOR("qiang.deng");
MODULE_LICENSE("GPL");
