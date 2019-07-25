/*
 * Sunplus RTC ISR test driver
 *
 * Copyright (C) 2018 Sunplus Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This code is used for testing ISR wiring only.
 * Because RISC(s) is/are power-off, there is no way to wakeup system from RTC's ISR.
 * It must be done through IOP.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_irq.h>

struct sp_rtc_reg {
	volatile unsigned int rsv00;
	volatile unsigned int rsv01;
	volatile unsigned int rsv02;
	volatile unsigned int rsv03;
	volatile unsigned int rsv04;
	volatile unsigned int rsv05;
	volatile unsigned int rsv06;
	volatile unsigned int rsv07;
	volatile unsigned int rsv08;
	volatile unsigned int rsv09;
	volatile unsigned int rsv10;
	volatile unsigned int rsv11;
	volatile unsigned int rsv12;
	volatile unsigned int rsv13;
	volatile unsigned int rsv14;
	volatile unsigned int rsv15;
	volatile unsigned int rtc_ctrl;
	volatile unsigned int rtc_timer_out;
	volatile unsigned int rtc_divider;
	volatile unsigned int rtc_timer_set;
	volatile unsigned int rtc_alarm_set;
	volatile unsigned int rtc_user_data;
	volatile unsigned int rtc_reset_record;
	volatile unsigned int rtc_battery_ctrl;
	volatile unsigned int rtc_trim_ctrl;
	volatile unsigned int rsv25;
	volatile unsigned int rsv26;
	volatile unsigned int rsv27;
	volatile unsigned int rsv28;
	volatile unsigned int rsv29;
	volatile unsigned int rsv30;
	volatile unsigned int rsv31;
};

static const struct platform_device_id sp_rtc_isr_tst_devtypes[] = {
	{
		.name = "sp_rtc_isr_tst",
	}, {
		/* sentinel */
	}
};

static const struct of_device_id sp_rtc_isr_tst_dt_ids[] = {
	{
		.compatible = "sunplus,sp-rtc-isr-tst",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, sp_rtc_isr_tst_dt_ids);

static irqreturn_t sp_rtc_isr_tst_irq(int irq, void *args)
{
	printk(KERN_INFO "%s\n", __func__);
	return IRQ_HANDLED;
}

static int sp_rtc_isr_tst_probe(struct platform_device *pdev)
{
	struct resource *res_mem;
	const struct of_device_id *match;
	int ret, num_irq;
	int irq;
	struct sp_rtc_reg *sp_rtc_ptr;
	void __iomem *membase;

	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);

	if (pdev->dev.of_node) {
		match = of_match_node(sp_rtc_isr_tst_dt_ids, pdev->dev.of_node);
		if (match == NULL) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
			return -ENODEV;
		}
		num_irq = of_irq_count(pdev->dev.of_node);
		if (num_irq != 1) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		}
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res_mem)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(res_mem);
	}

	membase = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(membase)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(membase);
	}

	sp_rtc_ptr = (struct sp_rtc_reg *)(membase);
	irq = platform_get_irq(pdev, 0);
	ret = request_irq(irq, sp_rtc_isr_tst_irq, 0, NULL, NULL);

	sp_rtc_ptr->rtc_timer_set = 0x12345678;
	sp_rtc_ptr->rtc_alarm_set = sp_rtc_ptr->rtc_timer_set + 1;

	sp_rtc_ptr->rtc_user_data = (0xFFFF << 16) | 0xabcd;
	sp_rtc_ptr->rtc_ctrl = (0x003F << 16) | 0x0017;

	return 0;

}

static struct platform_driver rtc_isr_test_driver = {
	.driver		= {
		.name		= "sp_rtc_isr_tst",
		.of_match_table	= of_match_ptr(sp_rtc_isr_tst_dt_ids),
	},
	.id_table	= sp_rtc_isr_tst_devtypes,
	.probe		= sp_rtc_isr_tst_probe,
};
module_platform_driver(rtc_isr_test_driver);

MODULE_DESCRIPTION("RTC ISR test driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sp_rtc_isr_tst");
