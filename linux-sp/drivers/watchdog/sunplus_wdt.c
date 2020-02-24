/*
 *      sunplus Watchdog Driver
 *
 *      Copyright (c) 2019 Xiantao Hu
 *                   
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *      Based on sunxi_wdt.c
 *      (c) Copyright 2010 Novell, Inc.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>

#define WDT_CTRL 				0x30
#define WDT_CNT 				0x34

#define WDT_STOP				0x3877
#define WDT_RESUME				0x4A4B
#define WDT_CLRIRQ				0x7482
#define WDT_UNLOCK        		0xAB00
#define WDT_LOCK         		0xAB01
#define WDT_CONMAX        		0xDEAF

#define WDT_MAX_TIMEOUT		10
#define WDT_MIN_TIMEOUT		1

#define DRV_NAME		"sunplus-wdt"
#define DRV_VERSION		"1.0"

static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout;

struct sunplus_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem *wdt_base;
};

static int sunplus_wdt_restart(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct sunplus_wdt_dev *sunplus_wdt = watchdog_get_drvdata(wdt_dev);
    void __iomem *wdt_base;
	wdt_base = sunplus_wdt->wdt_base;
	writel(WDT_STOP, wdt_base + WDT_CTRL);
	writel(WDT_UNLOCK, wdt_base + WDT_CTRL);
	writel(0x0001, wdt_base + WDT_CNT);
	writel(WDT_RESUME, wdt_base + WDT_CTRL);

	return 0;
}

static int sunplus_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct sunplus_wdt_dev *sunplus_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunplus_wdt->wdt_base;
	writel(WDT_CONMAX, wdt_base + WDT_CTRL);
	return 0;
}

static int sunplus_wdt_set_timeout(struct watchdog_device *wdt_dev,
		unsigned int timeout)
{
	wdt_dev->timeout = timeout;
	sunplus_wdt_ping(wdt_dev);
	return 0;
}

static int sunplus_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct sunplus_wdt_dev *sunplus_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = sunplus_wdt->wdt_base;

	writel(WDT_STOP, wdt_base + WDT_CTRL);
	return 0;
}

static int sunplus_wdt_start(struct watchdog_device *wdt_dev)
{
	struct sunplus_wdt_dev *sunplus_wdt = watchdog_get_drvdata(wdt_dev);
    void __iomem *wdt_base;
	int ret;

	wdt_base = sunplus_wdt->wdt_base;

	ret = sunplus_wdt_set_timeout(wdt_dev, wdt_dev->timeout);
	if (ret < 0)
		return ret;

	sunplus_wdt_ping(wdt_dev);
	writel(WDT_RESUME, wdt_base + WDT_CTRL);
	return 0;
}

static const struct watchdog_info sunplus_wdt_info = {
	.identity	= DRV_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops sunplus_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= sunplus_wdt_start,
	.stop		= sunplus_wdt_stop,
	.ping		= sunplus_wdt_ping,
	.set_timeout	= sunplus_wdt_set_timeout,
	.restart	= sunplus_wdt_restart,
};

static int sunplus_wdt_probe(struct platform_device *pdev)
{
	struct sunplus_wdt_dev *sunplus_wdt;
    struct resource *wdt_res;
	int err;

	sunplus_wdt = devm_kzalloc(&pdev->dev, sizeof(*sunplus_wdt), GFP_KERNEL);
	if (!sunplus_wdt)
		return -EINVAL;

	platform_set_drvdata(pdev, sunplus_wdt);

	wdt_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sunplus_wdt->wdt_base = devm_ioremap_resource(&pdev->dev, wdt_res);
	if (IS_ERR(sunplus_wdt->wdt_base))
		return PTR_ERR(sunplus_wdt->wdt_base);

	sunplus_wdt->wdt_dev.info = &sunplus_wdt_info;
	sunplus_wdt->wdt_dev.ops = &sunplus_wdt_ops;
	sunplus_wdt->wdt_dev.timeout = WDT_MAX_TIMEOUT;
	sunplus_wdt->wdt_dev.max_timeout = WDT_MAX_TIMEOUT;
	sunplus_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	sunplus_wdt->wdt_dev.parent = &pdev->dev;

	watchdog_init_timeout(&sunplus_wdt->wdt_dev, timeout, &pdev->dev);
	watchdog_set_nowayout(&sunplus_wdt->wdt_dev, nowayout);
	watchdog_set_restart_priority(&sunplus_wdt->wdt_dev, 128);

	watchdog_set_drvdata(&sunplus_wdt->wdt_dev, sunplus_wdt);
	
	sunplus_wdt_stop(&sunplus_wdt->wdt_dev);
	err = watchdog_register_device(&sunplus_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	dev_info(&pdev->dev, "Watchdog enabled (timeout=%d sec, nowayout=%d)\n",
			sunplus_wdt->wdt_dev.timeout, nowayout);

	return 0;
}

static void sunplus_wdt_shutdown(struct platform_device *pdev)
{
	struct sunplus_wdt_dev *sunplus_wdt = platform_get_drvdata(pdev);

	if (watchdog_active(&sunplus_wdt->wdt_dev))
		sunplus_wdt_stop(&sunplus_wdt->wdt_dev);
}

static int sunplus_wdt_remove(struct platform_device *pdev)
{
	struct sunplus_wdt_dev *sunplus_wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&sunplus_wdt->wdt_dev);

	return 0;
}

static const struct of_device_id sunplus_wdt_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-wdt" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunplus_wdt_dt_ids);

static struct platform_driver sunplus_wdt_driver = {
	.probe		= sunplus_wdt_probe,
	.remove		= sunplus_wdt_remove,
	.shutdown	= sunplus_wdt_shutdown,
	.driver		= {
		.name		= DRV_NAME,
		.of_match_table	= sunplus_wdt_dt_ids,
	},
};

module_platform_driver(sunplus_wdt_driver);

module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog heartbeat in seconds");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xiantao Hu <wjxianjian@126.com>");
MODULE_DESCRIPTION("Sunplus WatchDog Timer Driver");
MODULE_VERSION(DRV_VERSION);
