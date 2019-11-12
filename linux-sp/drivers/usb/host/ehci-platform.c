/*
 * Generic platform ehci driver
 *
 * Copyright 2007 Steven Brown <sbrown@cortland.com>
 * Copyright 2010-2012 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Derived from the ohci-ssb driver
 * Copyright 2007 Michael Buesch <m@bues.ch>
 *
 * Derived from the EHCI-PCI driver
 * Copyright (c) 2000-2004 by David Brownell
 *
 * Derived from the ohci-pci driver
 * Copyright 1999 Roman Weissgaerber
 * Copyright 2000-2002 David Brownell
 * Copyright 1999 Linus Torvalds
 * Copyright 1999 Gregory P. Smith
 *
 * Licensed under the GNU/GPL. See COPYING for details.
 */
#include <linux/platform_device.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/kthread.h>
#include <linux/usb/otg.h>
#include <mach/io_map.h>
#include <linux/clk.h>
#include <linux/usb/sp_usb.h>
#include "../phy/sunplus-otg.h"

struct clk *ehci_clk[USB_PORT_NUM];
static int ehci_platform_reset(struct usb_hcd *hcd)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	struct usb_ehci_pdata *pdata = pdev->dev.platform_data;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int port_num = pdev->id - 1;
	int retval;

	hcd->has_tt = pdata->has_tt;
	ehci->has_synopsys_hc_bug = pdata->has_synopsys_hc_bug;
	ehci->big_endian_desc = pdata->big_endian_desc;
	ehci->big_endian_mmio = pdata->big_endian_mmio;

	ehci->caps = hcd->regs + pdata->caps_offset;
	retval = ehci_setup(hcd);
	if (retval)
		return retval;

	if (pdata->port_power_on)
		ehci_port_power(ehci, port_num, 1);
	if (pdata->port_power_off)
		ehci_port_power(ehci, port_num, 0);

	return 0;
}

static const struct hc_driver ehci_platform_hc_driver = {
	.description = hcd_name,
	.product_desc = "Generic Platform EHCI Controller",
#ifdef	CONFIG_USB_HOST_RESET_SP
	.hcd_priv_size = sizeof(struct ehci_hcd_sp),
#else
	.hcd_priv_size = sizeof(struct ehci_hcd),
#endif

	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2 | HCD_BH,

	.reset = ehci_platform_reset,
	.start = ehci_run,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

#ifdef CONFIG_USB_LOGO_TEST
	/*
	 * usb logo test support
	 */
	.usb_logo_test = ehci_usb_logo_test,
#endif

	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,
	.endpoint_reset = ehci_endpoint_reset,

	.get_frame_number = ehci_get_frame,

	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
#if defined(CONFIG_PM)
	.bus_suspend = ehci_bus_suspend,
	.bus_resume = ehci_bus_resume,
#endif
	.relinquish_port = ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,

	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
	.get_port_status_from_register = ehci_get_port_status_from_register,
};

#ifdef	CONFIG_USB_HOST_RESET_SP
#define 	RESET_UPHY(x,ret,addr)	{				\
				ret	 = readl(addr);		\
				ret |= (1<<(9+x))|(1<<(12+x));		\
				writel(ret,addr);			\
				ret &= ~((1<<(9+x))|(1<<(12+x)));	\
				writel(ret,addr);			\
			}
#define		REG_UPHY_RESET_OFFSET	(18)
#endif

#ifdef	CONFIG_USB_HOST_RESET_SP
extern void reset_usb_powerx(struct usb_hcd *hcd, int delayms);
#ifdef CONFIG_USB_LOGO_TEST
extern u32 usb_logo_test_start;
#endif

static int ehci_reset_thread(void *arg)
{
	struct ehci_hcd *ehci = (struct ehci_hcd *)arg;
	struct ehci_hcd_sp *sp_ehci = (struct ehci_hcd_sp *)arg;
	struct usb_hcd *hcd = ehci_to_hcd(ehci);
#ifndef CONFIG_USB_SUNPLUS_OTG
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
#endif

	int i;
	u32 flag;
#ifndef CONFIG_USB_SUNPLUS_OTG
	void __iomem *reg_addr;
#endif

	do {

NEXT_LOOP:

		msleep(10);
		wait_event_interruptible(hcd->reset_queue,
					 ((sp_ehci->flag & (RESET_UPHY_SIGN |
							    RESET_HC_SIGN)) !=
					  0)
					 || kthread_should_stop());

		if (kthread_should_stop()) {
			printk(KERN_DEBUG "%s is stoped!\n", __func__);
			break;
		}
#ifdef CONFIG_USB_LOGO_TEST
		if (usb_logo_test_start == 1) {
			sp_ehci->flag = 0;
			continue;
		}
#endif

		/*besure nothing on hcd bus */
		for (i = 1; i < 128 / (8 * sizeof(unsigned long)); i++) {

			if (unlikely(sp_ehci->flag & RESET_HC_DEAD))
				break;

			if (hcd->self.devmap.devicemap[i]) {
				sp_ehci->flag = 0;
				goto NEXT_LOOP;
			}
		}

		if ((hcd->self.devmap.devicemap[0] == 2)
		    || unlikely(sp_ehci->flag & RESET_HC_DEAD)) {

			flag = sp_ehci->flag;
			clear_bit(HCD_FLAG_DEAD, &hcd->flags);

			usb_remove_hcd(hcd);

#ifndef CONFIG_USB_SUNPLUS_OTG
			if (flag & RESET_UPHY_SIGN) {
				reg_addr = (pdev->id - 1) ? uphy1_base_addr : uphy0_base_addr;
				hcd->uphy_disconnect_level[pdev->id - 1] = readl(reg_addr + DISC_LEVEL_OFFSET);
				reset_uphy(pdev->id - 1);
				reinit_uphy(pdev->id - 1);
				writel(hcd->uphy_disconnect_level[pdev->id - 1], reg_addr + DISC_LEVEL_OFFSET);
				/*tell ohci reset controllor */
				sp_ehci->flag = RESET_SENDER;
			}
#endif

			msleep(100);

			usb_add_hcd(hcd, hcd->irq, IRQF_DISABLED | IRQF_SHARED);

			sp_ehci->flag &= ~RESET_HC_SIGN;
		} else {
			sp_ehci->flag = 0;
		}
	} while (!kthread_should_stop());
	return 0;
}
#endif

#ifdef	CONFIG_USB_HOST_RESET_SP
static int ehci_notifier_call(struct notifier_block *self,
			      unsigned long action, void *dev)
{
	struct usb_device *udev = (struct usb_device *)dev;
	struct usb_hcd *hcd_o;
	struct platform_device *pdev_o;
	u32 *ptr;

	struct ehci_hcd_sp *sp_ehci = container_of((void *)self,
						   struct ehci_hcd_sp,
						   ehci_notifier);
	struct ehci_hcd *ehci = (struct ehci_hcd *)sp_ehci;
	struct usb_hcd *hcd_e = ehci_to_hcd(ehci);
	struct platform_device *pdev_e =
	    to_platform_device(hcd_e->self.controller);

	if (action == USB_DEVICE_ADD) {
		if (!udev->parent) {	// roothub add
			hcd_o = bus_to_hcd(udev->bus);
			pdev_o = to_platform_device(hcd_o->self.controller);
			ptr = (u32 *) ((u8 *) hcd_o->hcd_priv +
				       hcd_o->driver->hcd_priv_size -
				       sizeof(u32)
			    );

			//EHCI&OHCI on one port
			if ((pdev_e != pdev_o) && (pdev_e->id == pdev_o->id)) {
				if (ptr && (*ptr & RESET_SENDER)) {
					sp_ehci->flag =
					    RESET_HC_SIGN & (~RESET_UPHY_SIGN);
					*ptr &= ~RESET_SENDER;
					wake_up_interruptible(&hcd_e->
							      reset_queue);
				}
			}
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_SWITCH_USB_ROLE

#define USB_UPHY_REG	(3)

static ssize_t show_usb_role(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	//struct platform_device *pdev = to_platform_device(dev);

	return 0;
}

static ssize_t store_usb_role(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	//struct usb_hcd *hcd = (struct usb_hcd *)platform_get_drvdata(pdev);
	//struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	u32 switch_usb_role;
	volatile u32 *grop1 = (u32 *) VA_IOB_ADDR(2 * 32 * 4);
	u32 ret;

	/* 0:host , 1:device */
	if (kstrtouint(buf, 0, &switch_usb_role) == 0) {
		//printk("usb rold -> %x\n", switch_usb_role);
		ret = ioread32(grop1 + USB_UPHY_REG);

		ret |= (1 << ((pdev->id - 1) * 8 + 4));
		if (switch_usb_role & 0x1) {
			ret &= ~(1 << ((pdev->id - 1) * 8 + 5));
			ret |= (1 << ((pdev->id - 1) * 8 + 6));
		} else
			ret |= (3 << ((pdev->id - 1) * 8 + 5));

		//printk("usb ret -> %x\n", ret);
		iowrite32(ret, grop1 + USB_UPHY_REG);
	}
#endif
	return count;
}

static DEVICE_ATTR(usb_role_switch,
		   S_IWUSR | S_IRUSR, show_usb_role, store_usb_role);

/* switch usb speed ( fs/hs ) */
static ssize_t show_usb_speed(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = (struct usb_hcd *)platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	u32 result;

	result = ehci_readl(ehci, &ehci->regs->configured_flag);

	return sprintf(buf, "%d\n", result);
}

#define MAX_PORT_NUM  					2
#define POWER_RESET_TIME 				500
#define ENABLE_FORCE_HOST_DISCONNECT    1
#define DISABLE_FORCE_HOST_DISCONNECT   0
static ssize_t store_usb_speed(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = (struct usb_hcd *)platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int port_num = pdev->id - 1;

	u32 usb_host_owner;

	/* 0:full speed 1:high speed */
	if (kstrtouint(buf, 0, &usb_host_owner) == 0) {
		if (port_num < MAX_PORT_NUM) {
			DISABLE_VBUS_POWER(port_num);
			uphy_force_disc(ENABLE_FORCE_HOST_DISCONNECT, port_num);
			msleep(POWER_RESET_TIME);
			ehci_writel(ehci,
				    FLAG_CF & usb_host_owner,
				    &ehci->regs->configured_flag);
			uphy_force_disc(DISABLE_FORCE_HOST_DISCONNECT,
					port_num);
			ENABLE_VBUS_POWER(port_num);
		} else if (MAX_PORT_NUM == port_num) {
			printk(KERN_NOTICE
			       "warning,port 2 is not supported to power reset\n");
			ehci_writel(ehci,
				    FLAG_CF & usb_host_owner,
				    &ehci->regs->configured_flag);
		} else {
			printk(KERN_NOTICE "error port num:%d\n", port_num);
		}
	}

	return count;
}

static DEVICE_ATTR(usb_speed_switch,
		   S_IWUSR | S_IRUSR, show_usb_speed, store_usb_speed);

static ssize_t show_usb_swing(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);

	return sprintf(buf, "%d\n", get_uphy_swing(pdev->id - 1));
}

static ssize_t store_usb_swing(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	//struct usb_hcd *hcd = (struct usb_hcd *)platform_get_drvdata(pdev);
	//struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	u32 swing_val;

	if (kstrtouint(buf, 0, &swing_val) == 0) {
		set_uphy_swing(swing_val, pdev->id - 1);
	}

	return count;
}

static DEVICE_ATTR(usb_uphy_swing,
		   S_IWUSR | S_IRUSR, show_usb_swing, store_usb_swing);

static ssize_t show_usb_disconnect_level(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	u32 disc_level;

	disc_level = get_disconnect_level(pdev->id - 1);
	printk(KERN_DEBUG
	       "port:%d,get disconnect level:0x%x\n", pdev->id - 1, disc_level);

	return sprintf(buf, "0x%x\n", disc_level);
}

static ssize_t store_usb_disconnect_level(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	u32 disc_level;

	if (kstrtouint(buf, 0, &disc_level) == 0) {
		printk(KERN_DEBUG
		       "port:%d,set disconnect level:0x%x\n",
		       pdev->id - 1, disc_level);
		set_disconnect_level(disc_level, pdev->id - 1);
	}

	return count;
}

static DEVICE_ATTR(usb_disconnect_level,
		   S_IWUSR | S_IRUSR, show_usb_disconnect_level,
		   store_usb_disconnect_level);

static ssize_t store_usb_ctrl_reset(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = (struct usb_hcd *)platform_get_drvdata(pdev);
	/*struct ehci_hcd *ehci = hcd_to_ehci(hcd); */
	u32 val = 1;

	if (kstrtouint(buf, 0, &val)) {
		return 1;
	}

	if (NULL == hcd) {
		printk(KERN_NOTICE "store_usb_ctrl_reset: usb controller invalid\n");
		return count;
	}
	printk(KERN_DEBUG "Will reset usb controller val=%d\n", val);
#ifdef CONFIG_USB_HOST_RESET_SP
	if (val == 1) {
		printk(KERN_DEBUG "%s wake usb ctrl\n", __FUNCTION__);
		*(hcd->ptr_flag) |= (RESET_HC_DEAD | RESET_UPHY_SIGN);
		wake_up_interruptible(&hcd->reset_queue);
	} else if (val > 500) {
		printk(KERN_DEBUG "power reset %d ms \n", val);
		reset_usb_powerx(hcd, val);
	}
#endif

	return count;
}

static DEVICE_ATTR(usb_ctrl_reset, S_IWUSR, NULL, store_usb_ctrl_reset);

#endif

static struct usb_ehci_pdata usb_ehci_pdata = {
};

int ehci_platform_probe(struct platform_device *dev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	int irq;
	int err = -ENOMEM;
#ifdef CONFIG_USB_SUNPLUS_OTG
	struct usb_phy *otg_phy;
#endif
#ifdef CONFIG_USB_HOST_RESET_SP
	struct ehci_hcd_sp *ehci_sp;
#endif

	//BUG_ON(!dev->dev.platform_data);

	if (usb_disabled())
		return -ENODEV;

	dev->dev.platform_data = &usb_ehci_pdata;

	/*enable usb controller clock*/
	ehci_clk[dev->id - 1] = devm_clk_get(&dev->dev, NULL);
	if (IS_ERR(ehci_clk[dev->id - 1])) {
		pr_err("not found clk source\n");
		return PTR_ERR(ehci_clk[dev->id - 1]);
	}
	clk_prepare(ehci_clk[dev->id - 1]);
	clk_enable(ehci_clk[dev->id - 1]);

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		pr_err("no irq provieded,ret:%d\n",irq);
		return irq;
	}
	printk(KERN_DEBUG "ehci_id:%d,irq:%d\n",dev->id,irq);

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		pr_err("no memory recourse provieded");
		return -ENXIO;
	}

	hcd = usb_create_hcd(&ehci_platform_hc_driver, &dev->dev,
			     dev_name(&dev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res_mem->start;
	hcd->rsrc_len = resource_size(res_mem);

#ifdef CONFIG_USB_USE_PLATFORM_RESOURCE
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_err("controller already in use");
		err = -EBUSY;
		goto err_put_hcd;
	}

	hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs)
		goto err_release_region;
#else
	hcd->regs = (void *)res_mem->start;
#endif
	tasklet_init(&hcd->host_irq_tasklet, ehci_irq_tasklet, (unsigned long)hcd);
	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	printk(KERN_DEBUG "hcd_irq:%d,%d\n",hcd->irq,irq);
	if (err)
		goto err_iounmap;

	platform_set_drvdata(dev, hcd);
/****************************************************/
#ifdef CONFIG_USB_HOST_RESET_SP
	ehci_sp = (struct ehci_hcd_sp *)hcd_to_ehci(hcd);
#endif

#ifdef CONFIG_USB_SUNPLUS_OTG
	if (dev->id < 3) {
		otg_phy = usb_get_transceiver_sp(dev->id - 1);
		if(otg_phy){
			err = otg_set_host(otg_phy->otg, &hcd->self);
			if (err < 0) {
				dev_err(&dev->dev,
					"unable to register with transceiver\n");
				goto err_iounmap;
			}
		}
		hcd->self.otg_port = 1;
	}
#endif

#ifdef CONFIG_SWITCH_USB_ROLE
	if (dev->id < 3) {
		device_create_file(&dev->dev, &dev_attr_usb_role_switch);
		device_create_file(&dev->dev, &dev_attr_usb_ctrl_reset);
	}

	device_create_file(&dev->dev, &dev_attr_usb_speed_switch);
	device_create_file(&dev->dev, &dev_attr_usb_uphy_swing);
	device_create_file(&dev->dev, &dev_attr_usb_disconnect_level);
#endif

#ifdef CONFIG_USB_HOST_RESET_SP
	ehci_sp->flag = 0;

	printk(KERN_DEBUG "flag ***%px %px %d %d %px\n", hcd, hcd->hcd_priv,
	       sizeof(struct ehci_hcd_sp), hcd->driver->hcd_priv_size,
	       &ehci_sp->flag);

	/* ohci reset uphy notifier */
	ehci_sp->ehci_notifier.notifier_call = ehci_notifier_call;
	usb_register_notify(&ehci_sp->ehci_notifier);

	init_waitqueue_head(&hcd->reset_queue);
	hcd->ptr_flag = &ehci_sp->flag;

	ehci_sp->reset_thread = kthread_create(ehci_reset_thread,
					       hcd_to_ehci(hcd),
					       "ehci_reset_wait_event");

	if (IS_ERR(ehci_sp->reset_thread)) {
		pr_err("Create EHCI(%d) reset thread fail!\n", dev->id);
		return err;
	}

	/* Tell the thread to start working */
	wake_up_process(ehci_sp->reset_thread);

#endif

	return err;

err_iounmap:
#ifdef CONFIG_USB_USE_PLATFORM_RESOURCE
	iounmap(hcd->regs);
err_release_region:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err_put_hcd:
#endif
	usb_put_hcd(hcd);
	return err;
}
EXPORT_SYMBOL_GPL(ehci_platform_probe);

int ehci_platform_remove(struct platform_device *dev)
{
	struct usb_hcd *hcd = platform_get_drvdata(dev);

#ifdef CONFIG_USB_HOST_RESET_SP
	struct ehci_hcd_sp *ehci_sp = (struct ehci_hcd_sp *)hcd_to_ehci(hcd);
#endif

#ifdef CONFIG_SWITCH_USB_ROLE
	if (dev->id < 3) {
		device_remove_file(&dev->dev, &dev_attr_usb_role_switch);
		device_remove_file(&dev->dev, &dev_attr_usb_ctrl_reset);
	}

	device_remove_file(&dev->dev, &dev_attr_usb_speed_switch);
	device_remove_file(&dev->dev, &dev_attr_usb_uphy_swing);
	device_remove_file(&dev->dev, &dev_attr_usb_disconnect_level);
#endif

#ifdef	CONFIG_USB_HOST_RESET_SP
	kthread_stop(ehci_sp->reset_thread);
	usb_unregister_notify(&ehci_sp->ehci_notifier);
#endif

	usb_remove_hcd(hcd);

#ifdef CONFIG_USB_USE_PLATFORM_RESOURCE
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
#endif

	usb_put_hcd(hcd);
	platform_set_drvdata(dev, NULL);

	/*disable usb controller clock*/
	clk_disable(ehci_clk[dev->id - 1]);

	return 0;
}
EXPORT_SYMBOL_GPL(ehci_platform_remove);

#ifdef CONFIG_PM
static int ehci_sunplus_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);
	int rc;

	printk(KERN_DEBUG "%s.%d\n",__FUNCTION__, __LINE__);
	rc = ehci_suspend(hcd, do_wakeup);
	if (rc)
		return rc;

	/*disable usb controller clock*/
	clk_disable(ehci_clk[dev->id - 1]);
	
	return 0;
}

static int ehci_sunplus_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	printk(KERN_DEBUG "%s.%d\n",__FUNCTION__, __LINE__);
	/*enable usb controller clock*/
	clk_prepare(ehci_clk[dev->id - 1]);
	clk_enable(ehci_clk[dev->id - 1]);

	ehci_resume(hcd, false);

	return 0;
}

struct dev_pm_ops ehci_sunplus_pm_ops = {
	.suspend = ehci_sunplus_drv_suspend,
	.resume = ehci_sunplus_drv_resume,
};
#endif

