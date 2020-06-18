/*
 * Generic platform ohci driver
 *
 * Copyright 2007 Michael Buesch <m@bues.ch>
 * Copyright 2011-2012 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Derived from the OCHI-SSB driver
 * Derived from the OHCI-PCI driver
 * Copyright 1999 Roman Weissgaerber
 * Copyright 2000-2002 David Brownell
 * Copyright 1999 Linus Torvalds
 * Copyright 1999 Gregory P. Smith
 *
 * Licensed under the GNU/GPL. See COPYING for details.
 */
#include <linux/platform_device.h>
#include <linux/usb/ohci_pdriver.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
//#include "ohci.h"

#ifdef CONFIG_USB_SP_UDC_HOST
#include "udc-to-host.h"
#endif

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)

struct ohci_hcd_sp {
	/* must be 1st member here for hcd_to_ehci() to work */
	struct ohci_hcd ohci;

	struct task_struct *reset_thread;
	struct notifier_block ohci_notifier;

#ifdef CONFIG_USB_SP_UDC_HOST
	struct notifier_block udc_notifier;
#endif

	u32 flag;

#define RESET_UPHY_SIGN	(1<<0)
#define RESET_HC_SIGN		(1<<1)
#define RESET_SENDER		(1<<31)
#define FASTBOOT_FG			(1<<30)
};

#elif defined CONFIG_USB_SP_UDC_HOST

struct ohci_hcd_sp {
	/* must be 1st member here for hcd_to_ehci() to work */
	struct ohci_hcd ohci;
	struct notifier_block udc_notifier;
};
#endif

#if 1				/*fix ohci msi */
/*
*twofish suggest that latency should not
*exceed 8192 clock cycles(system clock),about 24.8us
*/
s32 get_td_retry_time = 24;
#endif

static struct clk *ohci_clk[USB_PORT_NUM];

static int ohci_platform_reset(struct usb_hcd *hcd)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	struct usb_ohci_pdata *pdata = pdev->dev.platform_data;
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int err;

	if (pdata->big_endian_desc)
		ohci->flags |= OHCI_QUIRK_BE_DESC;
	if (pdata->big_endian_mmio)
		ohci->flags |= OHCI_QUIRK_BE_MMIO;
	if (pdata->no_big_frame_no)
		ohci->flags |= OHCI_QUIRK_FRAME_NO;

	ohci_hcd_init(ohci);
	err = ohci_init(ohci);

	return err;
}

static int ohci_platform_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int err;

	err = ohci_run(ohci);
	if (err < 0) {
		ohci_err(ohci, "can't start\n");
		ohci_stop(hcd);
	}

	return err;
}

u8 otg0_vbus_off = 0;
u8 otg1_vbus_off = 0;
EXPORT_SYMBOL_GPL(otg0_vbus_off);
EXPORT_SYMBOL_GPL(otg1_vbus_off);

static int ohci_update_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);

	if ((udev->descriptor.bcdDevice & 0x1) == 0x1) {
		if (pdev->id == 1)
			otg0_vbus_off = 1;
		else if (pdev->id == 2)
			otg1_vbus_off = 1;
	}

	return 0;
}

static const struct hc_driver ohci_platform_hc_driver = {
	.description = hcd_name,
	.product_desc = "Generic Platform OHCI Controller",
#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)
	.hcd_priv_size = sizeof(struct ohci_hcd_sp),
#else
	.hcd_priv_size = sizeof(struct ohci_hcd),
#endif

	.irq = ohci_irq,
	.flags = HCD_MEMORY | HCD_USB11,

	.reset = ohci_platform_reset,
	.start = ohci_platform_start,
	.stop = ohci_stop,
	.shutdown = ohci_shutdown,

	.urb_enqueue = ohci_urb_enqueue,
	.urb_dequeue = ohci_urb_dequeue,
	.endpoint_disable = ohci_endpoint_disable,

	.get_frame_number = ohci_get_frame,

	.update_device = ohci_update_device,

	.hub_status_data = ohci_hub_status_data,
	.hub_control = ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend = ohci_bus_suspend,
	.bus_resume = ohci_bus_resume,
#endif

	.start_port_reset = ohci_start_port_reset,
	.get_port_status_from_register = ohci_get_port_status_from_register,
};

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)
#include <mach/io_map.h>
#define 	RESET_UPHY(x,ret,addr)	{				\
				ret	 = ioread32(addr);		\
				ret |= (1<<(9+x))|(1<<(12+x));		\
				iowrite32(ret,addr);			\
				ret &= ~((1<<(9+x))|(1<<(12+x)));	\
				iowrite32(ret,addr);			\
			}
#define		REG_UPHY_RESET_OFFSET	(18)
#endif

#ifdef 	CONFIG_USB_HOST_RESET

static int ohci_reset_thread(void *arg)
{
	struct ohci_hcd_sp *sp_ohci = (struct ohci_hcd_sp *)arg;
	struct ohci_hcd *ohci = (struct ohci_hcd *)arg;
	struct usb_hcd *hcd = ohci_to_hcd(ohci);
	struct platform_device *pdev = to_platform_device(hcd->self.controller);

	u32 val;
	int i;
	//int flag = 1;

	while (1) {

		if (sp_ohci->flag & (RESET_UPHY_SIGN | RESET_HC_SIGN)) {

			while (hcd->self.devmap.devicemap[0] != 2)
				msleep(1);

			if (hcd->self.devmap.devicemap[0] > 2) {
				goto NEXT_LOOP;
			}

			for (i = 1; i < 128 / (8 * sizeof(unsigned long)); i++) {
				if (hcd->self.devmap.devicemap[i]) {
					goto NEXT_LOOP;
				}
			}

			/*hcd->irq will be set to 0 by ohci_stop() */
			if (pdev->id == 1) {
				i = SP_IRQ_OHCI_USB0;
			} else if (pdev->id == 2) {
				i = SP_IRQ_OHCI_USB1;
			}

			if (sp_ohci->flag & RESET_UPHY_SIGN) {
				reset_uphy(pdev->id - 1);
				sp_ohci->flag = RESET_SENDER;
			} else {
				sp_ohci->flag = 0;
			}

			hcd->irq = i;
			usb_remove_hcd(hcd);
			ohci_usb_reset(ohci);
			msleep(1);
			usb_add_hcd(hcd, i, IRQF_SHARED);
		}

NEXT_LOOP:
		sp_ohci->flag = 0;
		msleep(30);
	}

	return 0;
}

#elif defined(CONFIG_USB_HOST_RESET_SP)
#ifdef CONFIG_USB_LOGO_TEST
extern u32 usb_logo_test_start;
#endif

static int ohci_reset_thread(void *arg)
{
	struct ohci_hcd *ohci = (struct ohci_hcd *)arg;
	struct ohci_hcd_sp *sp_ohci = (struct ohci_hcd_sp *)arg;
	struct usb_hcd *hcd = ohci_to_hcd(ohci);
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	u32 flag;
	int i;
	int irq_num = 0;
#ifndef CONFIG_USB_SUNPLUS_OTG
	void __iomem *reg_addr;
#endif

	do {

NEXT_LOOP:
		msleep(1);
		wait_event_interruptible(hcd->reset_queue,
					 ((sp_ohci->flag &
					   (RESET_UPHY_SIGN | RESET_HC_SIGN)) !=
					  0) || kthread_should_stop());

		if (kthread_should_stop())
			break;

#ifdef CONFIG_USB_LOGO_TEST
		if (usb_logo_test_start == 1) {
			sp_ohci->flag = 0;
			continue;
		}
#endif

		/*besure nothing on hcd bus */
		for (i = 1; i < 128 / (8 * sizeof(unsigned long)); i++) {
			if (hcd->self.devmap.devicemap[i]) {
				sp_ohci->flag = 0;
				goto NEXT_LOOP;
			}
		}

		printk("### %s  devicemap=%lx +\n", __FUNCTION__, hcd->self.devmap.devicemap[0]);
		if (hcd->self.devmap.devicemap[0] == 2) {
			irq_num = hcd->irq;
			flag = sp_ohci->flag;
			/*Clear flag  */
			clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
			clear_bit(HCD_FLAG_DEAD, &hcd->flags);

			disable_irq(hcd->irq);
			usb_remove_hcd(hcd);

#ifndef CONFIG_USB_SUNPLUS_OTG
			if (flag & RESET_UPHY_SIGN) {
				reg_addr = (pdev->id - 1) ? uphy1_base_addr : uphy0_base_addr;
				hcd->uphy_disconnect_level[pdev->id - 1] = readl(reg_addr + DISC_LEVEL_OFFSET);
				reset_uphy(pdev->id - 1);
				reinit_uphy(pdev->id - 1);
				writel(hcd->uphy_disconnect_level[pdev->id - 1], reg_addr + DISC_LEVEL_OFFSET);
				/*tell ehci reset controllor */
				sp_ohci->flag = RESET_SENDER;
			}
#endif
			msleep(100);

			usb_add_hcd(hcd, irq_num, IRQF_DISABLED | IRQF_SHARED);

			sp_ohci->flag &= ~RESET_HC_SIGN;

#ifdef CONFIG_USB_OHCI_SPHE8700_TD_FIX
			/*clear td fix flag */
			clr_td_fix_flag();
#endif

			if (enum_rx_active_flag[pdev->id - 1]) {
				printk(KERN_NOTICE
				       "rx-active,ohci reset finish\n");
				up(&enum_rx_active_reset_sem[pdev->id - 1]);
			}
		} else {
			sp_ohci->flag = 0;
		}
	} while (!kthread_should_stop());

	return 0;
}

#endif

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)

static int ohci_notifier_call(struct notifier_block *self,
			      unsigned long action, void *dev)
{
	struct usb_device *udev = (struct usb_device *)dev;

	struct usb_hcd *hcd_e;
	struct platform_device *pdev_e;
	u32 *ptr;

	struct ohci_hcd_sp *sp_ohci = container_of((void *)self,
						   struct ohci_hcd_sp,
						   ohci_notifier);
	struct ohci_hcd *ohci = (struct ohci_hcd *)sp_ohci;
	struct usb_hcd *hcd_o = ohci_to_hcd(ohci);
	struct platform_device *pdev_o =
	    to_platform_device(hcd_o->self.controller);

	//printk("notifier ohci\n");

	if (action == USB_DEVICE_ADD) {

		if (!udev->parent) {	//roothub add

			hcd_e = bus_to_hcd(udev->bus);
			pdev_e = to_platform_device(hcd_e->self.controller);
			ptr = (u32 *) ((u8 *) hcd_e->hcd_priv +
				       hcd_e->driver->hcd_priv_size -
				       sizeof(u32)
			    );
			//printk("notifier ohci %p 0x%x\n",ptr,*ptr);

			//EHCI&OHCI on one port
			if ((pdev_e != pdev_o) && (pdev_e->id == pdev_o->id)) {
				if (ptr && (*ptr & RESET_SENDER)) {
					sp_ohci->flag =
					    RESET_HC_SIGN & (~RESET_UPHY_SIGN);
					*ptr &= ~RESET_SENDER;
#ifdef CONFIG_USB_HOST_RESET_SP
					wake_up_interruptible(&hcd_o->
							      reset_queue);
#endif
				}
			}
		}
	}

	return 0;
}

#endif

#ifdef CONFIG_USB_SP_UDC_HOST

static int udc_notifier_call(struct notifier_block *self,
			     unsigned long action, void *dev)
{
	struct ohci_hcd_sp *sp_ohci = container_of((void *)self,
						   struct ohci_hcd_sp,
						   udc_notifier);
	struct ohci_hcd *ohci = (struct ohci_hcd *)sp_ohci;
	struct usb_hcd *hcd = ohci_to_hcd(ohci);

	struct udc_hcd_platform_device *udc_dev =
	    (struct udc_hcd_platform_device *)dev;

	//u32 *grop1 = (u32*)VA_IOB_ADDR(1*32*4);
	//u32 ret;

	//printk("udc notifier %lx %x %p\n",action,hcd->rh_registered,hcd);

	if (hcd->rh_registered == 1)
		//usb_remove_hcd(hcd);

		if (action == USB_DEVICE_ADD) {
			//if(hcd->rh_registered == 1)
			//      usb_remove_hcd(hcd);
			//ret = ioread32(grop1 + 10);
			//ret &= ~(1<<4);
			//iowrite32(ret,grop1 + 10);
			while (HC_IS_RUNNING(hcd->state))
				msleep(1);
			//SET_TO_UDC_OHCI(udc_dev->state);
			SET_TO_UDC(udc_dev->state);
		}

	if (action == USB_DEVICE_REMOVE) {

		//if(hcd->rh_registered == 1)
		//      usb_remove_hcd(hcd);

		//ret = ioread32(grop1 + 10);
		//ret |= (1<<4);
		//iowrite32(ret,grop1 + 10);

		//usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
	}

	return 0;
}

#endif

#if 1				/*fix ohci msi */
static ssize_t show_get_td_retry_time(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "get get_td_retry_time\n");

	return sprintf(buf, "%d\n", get_td_retry_time);
}

static ssize_t store_get_td_retry_time(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	printk(KERN_DEBUG "set get_td_retry_time\n");
	if (kstrtouint(buf, 0, &get_td_retry_time) == 0) {
		printk(KERN_DEBUG "%d\n", get_td_retry_time);
	} else {
		printk(KERN_NOTICE "set get_td_retry_time fail\n");
	}

	return count;
}

static DEVICE_ATTR(get_td_retry_time, S_IWUSR | S_IRUSR,
		   show_get_td_retry_time, store_get_td_retry_time);
#endif

static struct usb_ohci_pdata usb_ohci_pdata = {
};

int ohci_platform_probe(struct platform_device *dev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	int irq;
	int err = -ENOMEM;

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP) || defined(CONFIG_USB_SP_UDC_HOST)
	struct ohci_hcd_sp *ohci_sp;
#endif

	//BUG_ON(!dev->dev.platform_data);

	if (usb_disabled())
		return -ENODEV;

	dev->dev.platform_data = &usb_ohci_pdata;

	/*enable usb controller clock*/
	ohci_clk[dev->id - 1] = devm_clk_get(&dev->dev, NULL);
	if (IS_ERR(ohci_clk[dev->id - 1])) {
		pr_err("not found clk source\n");
		return PTR_ERR(ohci_clk[dev->id - 1]);
	}
	clk_prepare(ohci_clk[dev->id - 1]);
	clk_enable(ohci_clk[dev->id - 1]);

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		pr_err("no irq provieded");
		return irq;
	}
	printk("ohci_id:%d,irq:%d\n",dev->id,irq);

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		pr_err("no memory recourse provieded");
		return -ENXIO;
	}

	hcd = usb_create_hcd(&ohci_platform_hc_driver, &dev->dev,
			     dev_name(&dev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res_mem->start;
	hcd->rsrc_len = resource_size(res_mem);

#ifdef	CONFIG_USB_USE_PLATFORM_RESOURCE
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
#ifdef CONFIG_USB_HOST_NOT_FINISH_QTD_WHEN_DISC_WORKAROUND
	tasklet_init(&hcd->host_irq_tasklet, ohci_irq_tasklet, (unsigned long)hcd);
#endif
	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err)
		goto err_iounmap;

	platform_set_drvdata(dev, hcd);

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP) || defined(CONFIG_USB_SP_UDC_HOST)
	ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
#endif

#ifdef CONFIG_USB_SP_UDC_HOST

	/*udc/host uphy notifier */
	if (dev->id == 3) {
		ohci_sp->udc_notifier.notifier_call = udc_notifier_call;
		udc_register_notify(&ohci_sp->udc_notifier);
	}
#endif

#if 1				/*fix ohci msi */
	device_create_file(&dev->dev, &dev_attr_get_td_retry_time);
#endif

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)
	//struct ohci_hcd_sp *ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
	ohci_sp->ohci_notifier.notifier_call = ohci_notifier_call;
	usb_register_notify(&ohci_sp->ohci_notifier);

	ohci_sp->flag = 0;
	printk(KERN_DEBUG "flag *** %d %d %px\n", sizeof(struct ohci_hcd_sp),
	       hcd->driver->hcd_priv_size, &ohci_sp->flag);

#ifdef 	CONFIG_USB_HOST_RESET
	ohci_sp->reset_thread = kthread_create(ohci_reset_thread,
					       hcd_to_ohci(hcd),
					       "ohci_reset_polling");
#elif defined(CONFIG_USB_HOST_RESET_SP)

	init_waitqueue_head(&hcd->reset_queue);
	hcd->ptr_flag = &ohci_sp->flag;

	ohci_sp->reset_thread = kthread_create(ohci_reset_thread,
					       hcd_to_ohci(hcd),
					       "ohci_reset_wait_event");
#endif
	if (IS_ERR(ohci_sp->reset_thread)) {
		pr_err("Create OHCI(%d) reset thread fail!\n", dev->id);
		return err;
	}

	/* Tell the thread to start working */
	wake_up_process(ohci_sp->reset_thread);

#endif
	return err;
err_iounmap:
#ifdef	CONFIG_USB_USE_PLATFORM_RESOURCE
	iounmap(hcd->regs);
err_release_region:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err_put_hcd:
#endif
	usb_put_hcd(hcd);
	return err;
}
EXPORT_SYMBOL_GPL(ohci_platform_probe);

int ohci_platform_remove(struct platform_device *dev)
{
	struct usb_hcd *hcd = platform_get_drvdata(dev);

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)|| defined(CONFIG_USB_SP_UDC_HOST)
	struct ohci_hcd_sp *ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
#endif

#if 1				/*fix ohci msi */
	device_remove_file(&dev->dev, &dev_attr_get_td_retry_time);
#endif

#ifdef CONFIG_USB_SP_UDC_HOST
	if (dev->id == 3)
		udc_unregister_notify(&ohci_sp->udc_notifier);
#endif

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_HOST_RESET_SP)
	kthread_stop(ohci_sp->reset_thread);
	usb_unregister_notify(&ohci_sp->ohci_notifier);
#endif

	usb_remove_hcd(hcd);

#ifdef	CONFIG_USB_USE_PLATFORM_RESOURCE
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
#endif
	usb_put_hcd(hcd);
	platform_set_drvdata(dev, NULL);

	/*disable usb controller clock*/
	clk_disable(ohci_clk[dev->id - 1]);

	return 0;
}
EXPORT_SYMBOL_GPL(ohci_platform_remove);

#ifdef CONFIG_PM
static int ohci_sunplus_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);
	int rc; 

	printk("%s.%d\n",__FUNCTION__, __LINE__);
	rc = ohci_suspend(hcd, do_wakeup);
	if (rc)
		return rc;

	/*disable usb controller clock*/
	clk_disable(ohci_clk[dev->id - 1]);

	return 0;
}

static int ohci_sunplus_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd			= dev_get_drvdata(dev);

	printk("%s.%d\n",__FUNCTION__, __LINE__);
	/*enable usb controller clock*/
	clk_prepare(ohci_clk[dev->id - 1]);
	clk_enable(ohci_clk[dev->id - 1]);
	
	ohci_resume(hcd, false);

	return 0;
}

struct dev_pm_ops ohci_sunplus_pm_ops = {
	.suspend = ohci_sunplus_drv_suspend,
	.resume = ohci_sunplus_drv_resume,
};
#endif
