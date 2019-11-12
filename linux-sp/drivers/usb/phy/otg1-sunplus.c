#include <linux/module.h>

#include <linux/platform_device.h>
#include "sunplus-otg.h"
#include <linux/usb/sp_usb.h>

struct sp_otg *sp_otg1_host = NULL;
EXPORT_SYMBOL(sp_otg1_host);

static const struct of_device_id otg1_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sunplus-q628-usb-otg1" },
	{ }
};
MODULE_DEVICE_TABLE(of, otg1_sunplus_dt_ids);

static struct platform_driver sunplus_usb_otg1_driver = {
	.probe		= sp_otg_probe,
	.remove		= sp_otg_remove,
	.suspend	= sp_otg_suspend,
	.resume		= sp_otg_resume,
	.driver		= {
		.name	= "sunplus-usb-otg1",
		.of_match_table = otg1_sunplus_dt_ids,
	},
};

static int __init usb_otg1_sunplus_init(void)
{
	if (sp_port_enabled & PORT1_ENABLED) {
		printk(KERN_NOTICE "register sunplus_usb_otg1_driver\n");
		return platform_driver_register(&sunplus_usb_otg1_driver);	
	} else {
		printk(KERN_NOTICE "otg1 not enabled\n");
		return 0;
	}
}
fs_initcall(usb_otg1_sunplus_init);

static void __exit usb_otg1_sunplus_exit(void)
{
	if (sp_port_enabled & PORT1_ENABLED) {
		printk(KERN_NOTICE "unregister sunplus_usb_otg1_driver\n");
		platform_driver_unregister(&sunplus_usb_otg1_driver);
	} else {
		printk(KERN_NOTICE "otg1 not enabled\n");
		return;
	}
}
module_exit(usb_otg1_sunplus_exit);

MODULE_ALIAS("sunplus_usb_otg1");
MODULE_AUTHOR("qiang.deng");
MODULE_LICENSE("GPL");
