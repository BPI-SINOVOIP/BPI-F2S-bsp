/*
 * linux/drivers/usb/gadget/udc/spnew_udc.c
 *
 * Sunplus udc2.0 series on-chip full speed USB device controllers
 *
 * Copyright (C) 2004-2007 Herbert Potzl - Arnaud Patard
 *  Additional cleanups by Ben Dooks <ben-linux@fluff.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) aby later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/prefetch.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/gadget_chips.h>
#include <linux/usb/composite.h>
#include <linux/usb/sp_usb.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/unaligned.h>

#include "sunplus_udc.h"


#define DRIVER_DESC "Sunplus new USB Device Controller Gadget"
#define DRIVER_VERSION  "13 Aug 2018"
#define DRIVER_AUTHOR "yl.gu"

static const char gadget_name[] = "spnew_udc";
static const char driver_desc[] = DRIVER_DESC;

#ifdef CONFIG_GADGET_USB0
uint accessory_port_id = USB_PORT0_ID;
#else
uint accessory_port_id = USB_PORT1_ID;
#endif
module_param(accessory_port_id, uint, 0644);
EXPORT_SYMBOL_GPL(accessory_port_id);

void usb_switch(int device);

#define VUBUS_CD_POLL	(HZ / 4)
struct timer_list vbus_polling_timer;
struct timer_list sof_polling_timer;
char is_config = 0;
static u32 sof_value = 0x1000;
static u32 temp_sof_value;
static u8 bus_reset_finish_flag = false;
static u8 platform_device_handle_flag = false;
static u8 first_enter_polling_timer_flag = false;

static u32 d_time0 = VUBUS_CD_POLL * 2;
module_param(d_time0, uint, 0644);
static u32 d_time1 = 10;
module_param(d_time1, uint, 0644);

static struct spnew_udc	*the_controller;
static struct clk	*udc_clock;
static int irq_num = 0;
static void __iomem	*base_addr;
static void __iomem	*moon5_base_addr;
static struct spnew_udc memory;

#define TO_HOST 	0
#define TO_DEVICE	1

static u32 dmsg = 0x2;
#define DEBUG_DBG(fmt, arg...)		do { if (dmsg&(1<<0)) printk(KERN_DEBUG fmt,##arg); } while (0)
#define DEBUG_NOTICE(fmt, arg...)	do { if (dmsg&(1<<1)) printk(KERN_NOTICE fmt,##arg); } while (0)
#define DEBUG_INFO(fmt, arg...)		do { if (dmsg&(1<<2)) printk(KERN_INFO fmt,##arg); } while (0)
#define DEBUG_ERR(fmt, arg...)		do { printk(KERN_ERR fmt,##arg); } while (0)

void detech_start(void);

static inline u32 udc_read(u32 reg)
{
	return readb(base_addr + reg);
}

static inline void udc_write(u32 value, u32 reg)
{
	writeb(value, base_addr + reg);
}

static inline void udc_writeb(void __iomem *base, u32 value, u32 reg)
{
	writeb(value, base + reg);
}

static ssize_t show_udc_ctrl(struct device *dev,
			      struct device_attribute *attr, char *buffer)
{
	u32 result = 0;
	struct spnew_ep *ep = &memory.ep[1];

	spin_lock(&ep->lock);
	result = list_empty(&ep->queue);
	spin_unlock(&ep->lock);

	DEBUG_NOTICE("ep1 is empty:%d req\n", result);
	return snprintf(buffer, 3, "%d\n", result);
}

static ssize_t store_udc_ctrl(struct device *dev,
			       struct device_attribute *attr,
			       const char *buffer, size_t count)
{
	static char irq_i = 0;

	if (*buffer == 'd') {		/* d:switch uphy to device */
		DEBUG_NOTICE("user switch \n");
		usb_switch(TO_DEVICE);
		msleep(1);
		detech_start();
		return count;
	} else if (*buffer == 'h') {	/* h:switch uphy to host */
		DEBUG_NOTICE("user switch \n");
		usb_switch(TO_HOST);
		return count;
	} else if (*buffer == 'i') {	/* interrupt disable or enable */
		if (!irq_i) {
			disable_irq(irq_num);
			irq_i = 1;
		} else {
			enable_irq(irq_num);
			irq_i = 0;
		}
	}

	return count;
}

static DEVICE_ATTR(udc_ctrl, S_IWUSR | S_IRUSR, show_udc_ctrl, store_udc_ctrl);

/*************************** DEBUG FUNCTION ***************************/
#define DEBUG_NORMAL	1
#define DEBUG_VERBOSE	2

#if 0
static unit32_t spnew_ticks = 0;

static int dprintk(int level, const char *fmt, ...)
{
	static char printk_buf[1024];
	static long prevticks;
	static int invocation;
	va_list args;
	int len;

	if (level > USB_S3C2410_DEBUG_LEVEL)
		return 0;

	if (s3c2410_ticks != prevticks) {
		prevticks = s3c2410_ticks;
		invocation = 0;
	}

	len = scnprintf(printk_buf,
	    sizeof(printk_buf), "%11u.%02d USB: ",
	    prevticks, invocation++);

	va_start(args, fmt);
	len = vscnprintf(printk_buf+len,
	    sizeof(printk_buf-len), fmt, args);
	va_end(args);

	return printk(KERN_DEBUG "%s", printk_buf);
}
#else
static int dprintk(int level, const char *fmt, ...)
{
	return 0;
}
#endif

static inline struct spnew_ep *to_spnew_ep(struct usb_ep *ep)
{
	return container_of(ep, struct spnew_ep, ep);
}

static inline struct spnew_udc *to_spnew_udc(struct usb_gadget *gadget)
{
	return container_of(gadget, struct spnew_udc, gadget);
}

static inline struct spnew_request *to_spnew_req(struct usb_request *req)
{
	return container_of(req, struct spnew_request, req);
}

static int spnew_udc_list_empty(struct list_head *head, spinlock_t * lock)
{
	int ret;
	
	spin_lock(lock);
	ret = list_empty(head);
	spin_unlock(lock);

	return ret;
}

/******************************** I/O *********************************/

/*
 *  sp_new_udc_done
 */
static void spnew_udc_done(struct spnew_ep *ep,
	struct spnew_request *req, int status)
{
	unsigned halted = ep->halted;

	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	ep->halted = 1;
	req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static void spnew_udc_nuke(struct spnew_udc *udc,
		struct spnew_ep *ep, int status)
{
	/* Sanity check */
	if (&ep->queue == NULL)
		return;

	while (!list_empty(&ep->queue)) {
		struct spnew_request *req;
		req = list_entry(ep->queue.next, struct spnew_request,
		    		 queue);
		spnew_udc_done(ep, req, status);
	}
}

static struct usb_ep *
find_ep (struct usb_gadget *gadget, const char *name)
{
	struct usb_ep *_ep = &memory.ep[1].ep;

	list_for_each_entry (_ep, &gadget->ep_list, ep_list) {
		if (strcmp (_ep->name, name) == 0)
			return _ep;
	}

	return NULL;
}

/*------------------------------------- usb state machine
-------------------------------------------*/
static int spnew_event_ring_init(void);
static int spnew_udc_set_halt(struct usb_ep *_ep, int value);

#if 0
static int spnew_udc_get_status(struct spnew_udc *dev,
			struct usb_ctrlrequest *crq)
{
	u16 status = 0;
	u8 ep_num = crq->wIndex & 0x7F;
	struct spnew_ep *ep = &memory.ep[ep_num];

	switch (crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			break;
		case USB_RECIP_DEVICE:
			status = dev->devstatus;
			break;
		case USB_RECIP_ENDPOINT:
#if 1
			if (ep_num >= SPNEW_ENDPOINTS || crq->wLength > 2)
#else
			if (ep_num > 30 || crq->wLength >2)
#endif
				return 1;

			status = ep->halted;
			break;
		default:
			return 1;
	}

#if 1
	return status;
#else
	return 0;
#endif
}
#endif

static int check_trb_status(struct trb *t_trb)
{
	u32 trb_status;
	int ret = 0;

	trb_status = ETRB_CC(t_trb->entry2);

	switch(trb_status) {
		case INVALID_TRB:
			printk(KERN_ERR "invalid trb\n");
			break;
		case SUCCESS_TRB:
			ret = SUCCESS_TRB;
			break;
		case DATA_BUF_ERR:
			printk(KERN_ERR "data buf err\n");
			break;
		case TRANS_ERR:
			printk(KERN_ERR "trans err\n");
			break;
		case TRB_ERR:
			printk(KERN_ERR "trb err\n");
			break;
		case RESOURCE_ERR:
			printk(KERN_ERR "resource err\n");
			break;
		case SHORT_PACKET:
			printk(KERN_ERR "short packet\n");
			ret = SHORT_PACKET;
			break;
		case EVENT_RING_FULL:
			printk(KERN_ERR "event ring full\n");
			ret = EVENT_RING_FULL;
			break;
		case STOPPED:
			printk(KERN_ERR "sw stopped\n");
			break;
		case UDC_RESET:
			printk(KERN_ERR "udc reset\n");
			ret = UDC_RESET;
			break;
		case UDC_SUSPEND:
			printk(KERN_ERR "udc suspend\n");
			ret = UDC_SUSPEND;
			break;
		case UDC_RESUME:
			printk(KERN_ERR "udc resume\n");
			ret = UDC_RESUME;
			break;
	}

	if (ret)
		return ret;
	else
		return -1;
}

static void process_trans_event(struct trb *t_trb)
{
	struct trb *trans_trb;
	struct spnew_ep *ep;
	struct spnew_request *req;
	u32 ep_num = 0;
	u32 er_trans_len = 0;
	u32 tr_trans_len = 0;
	u8 *data_buf;	// u8 ??

	trans_trb = (struct trb *)(u64)t_trb->entry0;
	ep_num = ETRB_EID(t_trb->entry3);
	er_trans_len = ETRB_LEN(t_trb->entry2);	// residual number of bytes not transferred
	tr_trans_len = TRB_TLEN(trans_trb->entry2);

	ep = &memory.ep[ep_num];
	data_buf = (u8 *)(u64)trans_trb->entry0;

	if (spnew_udc_list_empty(&ep->queue, &ep->lock)) {
		printk(KERN_ERR "\tep%d req is NULL\t!\n", ep_num);

		return;
	} else {
		req = list_entry(ep->queue.next, struct spnew_request, queue);

		if (trans_trb->entry3 & TRB_IDT) { /*find req done*/
			if (req->req.buf == data_buf)
				spnew_udc_done(ep, req, 0);
			else
				printk(KERN_ERR "it's not corresponding req\n");
		} else {
			if (req->req.length == (u16)tr_trans_len) // normal transfer, how to detect??
				spnew_udc_done(ep, req, 0);
			else
				printk(KERN_ERR "it's not corresponding req\n");

			printk(KERN_ERR "no data, transfer_len:%d\n", er_trans_len);
		}
	}

	return;
}

static void process_setup(struct spnew_udc *dev, struct trb *setup_trb)
{
	struct usb_ctrlrequest crq;
	struct spnew_ep *ep = &dev->ep[0];
	struct usb_composite_dev *cdev = get_gadget_data(&dev->gadget);
	struct usb_request *req_g = NULL;
	struct spnew_request *req = NULL;
	struct udc_endpoint_desc *ep0_desc;
	u32 ep_entry0 = 0;
	int ret;

	ep0_desc = spnew_device_desc;
	ep_entry0 = ep0_desc->ep_entry0;

	if (!cdev) {
		printk("cdev invalid\n");
		return;
	}

	req_g = cdev->req;
	req = to_spnew_req(req_g);
	if (!req) {
		printk("req invalid\n");
		return;
	}
	spnew_udc_nuke(dev, ep, -EPROTO);

	crq.bRequestType = setup_trb->entry0 & 0xFF;
	crq.bRequest = (setup_trb->entry0 >> 8) & 0xFF;
	crq.wValue = setup_trb->entry0 >> 16;
	crq.wIndex = setup_trb->entry1 & 0xFFFF;
	crq.wLength = setup_trb->entry1 >> 16;

	printk(KERN_DEBUG "bRequest = %x bRequestType %x, %x, %x, wLength = %d\n",
	  crq.bRequest, crq.bRequestType, crq.wValue, crq.wIndex, crq.wLength);

	/* cope with automatic for some standard request */
	dev->req_std = (crq.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD;
	dev->req_config = 0;
	dev->req_pending = 1;

	switch (crq.bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			printk("start get descriptor after bus reset\n");
			bus_reset_finish_flag = true;

			{
				u32 DescType = ((crq.wValue) >> 8);
				
				if (DescType == 0x01) {
					if (((ep_entry0 >> 16) & UDC_FULL_SPEED) == UDC_FULL_SPEED) {  // UDC_FULL_SPEED, USB_FULL_SPEED ??
						printk("DESCRIPTOR Speed = USB_FULL_SPEED\n");
						dev->gadget.speed = USB_SPEED_FULL;
					} else if (((ep_entry0 >> 16) & UDC_HIGH_SPEED) == UDC_HIGH_SPEED) {
						printk("DESCRIPTOR Speed = USB_HIGH_SPEED\n");
						dev->gadget.speed = USB_SPEED_HIGH;
					}
				}
			}
			break;
		case USB_REQ_SET_CONFIGURATION:
			printk(" ******* USB_REQ_SET_CONFIGURATION ******* \n");
			dev->req_config = 1; /* fill ep desc CFGS */
			is_config = 1;
			break;
		case USB_REQ_SET_INTERFACE:
			printk(" ******* USB_REQ_SET_INTERFACE ******* \n");
			dev->req_config = 1; /* fill ep desc INFS */
			break;
		case USB_REQ_SET_ADDRESS:
			printk(" ******* USB_REQ_SET_ADDRESS ******* \n");
			break;
		case USB_REQ_GET_STATUS:
			printk(" ******* USB_REQ_GET_STATUS ******* \n");
			break;
		case USB_REQ_CLEAR_FEATURE:
			break;
		case USB_REQ_SET_FEATURE:
			break;
		default:
			break;
	}

	if (crq.bRequestType & USB_DIR_IN) 
		dev->ep0state = EP0_IN_DATA_PHASE;
	else
		dev->ep0state = EP0_OUT_DATA_PHASE;

	/* deliver the request to the gadget driver */
	ret = dev->driver->setup(&dev->gadget, &crq);	/*android_setup composite_setup*/

	if (crq.bRequest == USB_REQ_SET_ADDRESS) {
		ret = 0;
		dev->ep0state = EP0_IDLE;
	}

	if (ret < 0) {
		if (dev->req_config) {
			printk(KERN_ERR "config change %02x fail %d?\n", crq.bRequest, ret);
			return;
		}

		if (ret == -EOPNOTSUPP)
			printk(KERN_ERR "Operation not support\n");
		else
			printk(KERN_ERR "dev->driver->setup failed. (%d)\n", ret);

		udc_write(udc_read(EP0_CS)|EP_STALL, EP0_CS);    /*set ep0 stall*/
		dev->ep0state = EP0_IDLE;
	} else if (dev->req_pending) {
		printk("dev->req_pending...what now?\n");
		dev->req_pending = 0;
		/*MSC reset command*/
		if (crq.bRequest == 0xff) {
			/*ep1SetHalt = 0;
			ep2SetHalt = 0;*/
		}
	}

	return;
}

static int handle_event_ring(struct spnew_udc *dev);
static void handle_event(struct work_struct *work)
{
	int ret;

	if (!gSpnew_udc)
		printk("no udc device\n");

	do {
		ret = handle_event_ring(gSpnew_udc);
	} while (ret != -EINVAL);

	printk("work_event ok\n");
	return;
}

static int handle_event_ring(struct spnew_udc *dev)
{
	struct trb *pevent;
	u32 trb_cc = 0;
	u32 trb_type = 0;
	u32 erdp_reg = 0;
	int ret;

	pevent = (struct trb *)(u64)(udc_read(DEVC_ERDP) & ERDP_MASK);
	trb_cc = pevent->entry3 & ETRB_C;
	if (trb_cc != event_ccs) {	/*invalid event trb*/
		printk(KERN_ERR "invalid event trb\n");
		return -EINVAL;
	}

	ret = check_trb_status(pevent);	/*handle err situation*/
	if (ret < 0) {
		pevent++;
		erdp_reg = udc_read(DEVC_ERDP) & 0xF;
		pevent = (struct trb *)((u64)pevent | (u64)erdp_reg);
		udc_write((u32)((u64)pevent), DEVC_ERDP);

		return -1;
	}

	trb_type = ETRB_TYPE(pevent->entry3);
	switch (trb_type) {
		case SETUP_TRB:
			process_setup(dev, pevent);
			break;
		case LINK_TRB:
			pevent = (struct trb*)(u64)pevent->entry0;

			if (pevent->entry3 & LTRB_C)
				pevent->entry3  &= (~LTRB_C);
			else
				pevent->entry3 |= LTRB_C;

			erdp_reg = udc_read(DEVC_ERDP) & 0xF;
			pevent = (struct trb*)((u64)pevent | (u64)erdp_reg);
			udc_write((u32)((u64)pevent), DEVC_ERDP);
			event_ccs = (event_ccs == 1) ? 0 : 1;	/*Toggle CCS*/
			break;
		case TRANS_EVENT_TRB:
			process_trans_event(pevent);
			break;
		case DEV_EVENT_TRB:
			if (ret == UDC_RESET) {
				is_config = 0;

				if (dev->driver && dev->driver->disconnect)
					dev->driver->disconnect(&dev->gadget);	/*jump to android_disconnect*/
			} else if (ret == UDC_SUSPEND) {
				dev->driver->suspend(&dev->gadget);
			} else {
				printk("\tNOT support\n");
			}
			break;
		case SOF_EVENT_TRB:
			break;
	}

	if (trb_type != LINK_TRB) {
		pevent++;
		erdp_reg = udc_read(DEVC_ERDP) & 0xF;
		pevent = (struct trb *)((u64)pevent | (u64) erdp_reg);
		udc_write((u32)((u64)pevent), DEVC_ERDP);
	}

	return 0;
}

//#include <mach/regs-irq.h>

/*
 *  spnew_udc_irq - interrupt handler
 */
static irqreturn_t spnew_udc_irq(int dummy, void * _dev)
{
	struct spnew_udc *dev = _dev;
	unsigned long flags;
	u32 int_reg;

	spin_lock_irqsave(&dev->lock, flags);

	int_reg = udc_read(DEVC_STS);

	if (int_reg & VBUS_CI) {
		udc_write(udc_read(DEVC_STS) & (~VBUS_CI), DEVC_STS);
		if (udc_read(DEVC_STS) & VBUS) {
			printk(KERN_NOTICE "attach device\n");
			//udc_write(udc_read(DEVC_CS) | UDC_RUN, DEVC_CS); /*enable udc controller*/
		} else {
			printk(KERN_NOTICE "detach device\n");
			udc_write(udc_read(DEVC_CS) & (~UDC_RUN), DEVC_CS); /*disable udc controller*/
		}
	}

	if (int_reg & EINT) {
		printk(KERN_NOTICE "event irq\n");
		queue_work(dev->event_workqueue, &dev->event_work);
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return IRQ_HANDLED;
}

/*--------------------------------- spnew_ep_ops ---------------------------------*/
static void fill_trans_trb(struct trb *t_trb, struct usb_request *req, u8 is_in)
{
	t_trb->entry0 = (u32)((u64)req->buf);
	t_trb->entry2 = req->length;

	if (t_trb->entry3 & TRB_C)
		t_trb->entry3 &= (~TRB_C);
	else
		t_trb->entry3 |= TRB_C;    /*toggle cycle bit*/

	if (is_in) {	// set TRB_IDT ??
		t_trb->entry3 |= (TRB_ISP | TRB_IOC | TRB_DIR | (NORMAL_TRB << 10));
	} else {
		t_trb->entry3 |= (TRB_ISP | TRB_IOC | (NORMAL_TRB << 10));
		t_trb->entry3 &= (~TRB_DIR);
	}
}

static void fill_link_trb(struct trb *t_trb, struct trb *ring)
{
	t_trb->entry0 = (u32)((u64)ring);
	t_trb->entry3 |= (LTRB_TC | LTRB_C | LTRB_IOC | (LINK_TRB << 10));
}

static int skip_link_trb(struct trb *t_trb, struct udc_endpoint_desc *desc)
{
	int ret = 0;

	t_trb->entry3 |= LTRB_TC;
	t_trb = (struct trb *)((u64)t_trb->entry0);

	//t_trb < 0x80000000 ??
	if (((u64)t_trb == (u64)DESC_TRDP(desc->ep_entry3)) || ((u64)t_trb < 0x80000000)) {
		printk(KERN_ERR "trans ring is full\n");
		ret = -1;
	}

	if (t_trb->entry3 & LTRB_C)
		t_trb->entry3 &= (~LTRB_C);
	else
		t_trb->entry3 |= LTRB_C;

	return ret;
}

static int check_trb_type(struct trb *t_trb)
{
	u32 idx;
	int trb_type = -1;

	idx = (u32)(LTRB_TYPE((u64)t_trb));
	switch(idx) {
		case 1:
			trb_type = NORMAL_TRB;
			break;
		case 2:
			trb_type = SETUP_TRB;
			break;
		case 6:
			trb_type = LINK_TRB;
			break;
		case 32:
			trb_type = TRANS_EVENT_TRB;
			break;
		case 48:
			trb_type = DEV_EVENT_TRB;
			break;
		case 49:
			trb_type = SOF_EVENT_TRB;
			break;
	}

	return trb_type;
}

static int fill_trans_ring(struct spnew_ep *ep, struct spnew_request *req, u8 is_in)
{
	int ret = 0;
	struct udc_endpoint_desc *desc;

	desc = spnew_device_desc;
	desc += ep->num;

	ret = check_trb_type(pep_enqueue[ep->num]);
	if (ret != LINK_TRB) {
		fill_trans_trb(pep_enqueue[ep->num], &req->req, is_in);
		pep_enqueue[ep->num]++;
	}else if (ret < 0) {
		printk(KERN_ERR "wrong trb type\n");
		ret = -EINVAL;
	} else {
		/*check dequeue pointer position*/
		ret = skip_link_trb(pep_enqueue[ep->num], desc);
		if (ret < 0) {
			printk(KERN_ERR "ep%d transfer ring is wrong\n", ep->num);
			ret = -EINVAL;
		} else {
			fill_trans_trb(pep_enqueue[ep->num], &req->req, is_in);
			pep_enqueue[ep->num]++;
		}
	}

	return ret;
}

/*
 *  spnew_udc_ep_enable
 */
static int spnew_udc_ep_enable(struct usb_ep *_ep,
         const struct usb_endpoint_descriptor *desc)
{
	struct spnew_udc *dev;
	struct spnew_ep *ep;
	struct trb *end_trb;
	struct udc_endpoint_desc *pep_desc;
	u32 max, tmp;
	unsigned long flags;
	u32 idx;

	ep = to_spnew_ep(_ep);

	if (!_ep || !desc || ep->desc || _ep->name == ep0name ||
	    desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = usb_endpoint_maxp(desc) & 0x1fff;

	local_irq_save(flags);
	_ep->maxpacket = max & 0x7FF;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;
	idx = ep->bEndpointAddress & 0x7F;

	/* malloc transfer ring */
	ep_transfer_ring[idx] = (struct trb *)kmalloc(TRANSFER_RING_SIZE + 15,
	                                              GFP_KERNEL);

	if (!ep_transfer_ring[idx]) {
		printk(KERN_NOTICE "malloc ep0 transfer ring fail\n");
		return -1;
	}

	if (((u64)ep_transfer_ring[idx] & ~0xF) != 0)
		ep_transfer_ring[idx] = (struct trb *)(((u64)ep_transfer_ring[idx] & ~0xF) + 0x10);

	end_trb = ep_transfer_ring[idx];
	end_trb += (TRANSFER_RING_SIZE / ENTRY_SIZE - 1);

	fill_link_trb(end_trb, ep_transfer_ring[idx]);
	pep_desc = spnew_device_desc;
	pep_desc += idx;

	pep_desc->ep_entry1 = (idx | (ep->bmAttributes << 4));
	pep_desc->ep_entry2 = (_ep->maxpacket | (1 << 30));
	pep_desc->ep_entry3 = (u32)((u64)ep_transfer_ring[idx]);

	/* enable irqs */
	udc_write(udc_read(DEVC_CS) | (1 << idx), DEVC_CS);  /* reload ep desc */
	udc_write(RDP_EN | EP_EN, EP_CS(idx));

	/* print some debug message */
	tmp = desc->bEndpointAddress;
	dprintk(DEBUG_NORMAL, "enable %s(%d) ep%x%s-blk max %02x\n",
	   _ep->name, ep->num, tmp,
	   desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

	local_irq_restore(flags);
	spnew_udc_set_halt(_ep, 0);

	return 0;
}

/*
 *  spnew_udc_ep_disable
 */
static int spnew_udc_ep_disable(struct usb_ep *_ep)
{
	struct spnew_ep *ep = to_spnew_ep(_ep);
	struct udc_endpoint_desc *pep_desc;
	unsigned long flags;
	u32 int_en_reg;
	u32 idx;

	if (!_ep || !ep->desc) {
		dprintk(DEBUG_NORMAL, "%s not enabled\n", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	local_irq_save(flags);
	dprintk(DEBUG_NORMAL, "ep disabled: %s\n", ep->ep.name);

	ep->desc = NULL;
	ep->ep.desc = NULL;
	ep->halted = 1;
	idx = ep->bEndpointAddress & 0x7F;

	spnew_udc_nuke(ep->dev, ep, -ESHUTDOWN);

	/* disable irqs */
	pep_desc = spnew_device_desc;
	pep_desc += idx;
	memset(pep_desc, 0, ENTRY_SIZE);

	int_en_reg = udc_read(EP_CS(idx));
	int_en_reg &= (~EP_EN);
	udc_write(RDP_EN | int_en_reg, EP_CS(idx));

	if (ep_transfer_ring[idx])
		kfree(ep_transfer_ring[idx]);

	local_irq_restore(flags);
	dprintk(DEBUG_NORMAL, "ep disabled: %s\n", ep->ep.name);

	return 0;
}

/*
 *  spnew_udc_alloc_request
 */
static struct usb_request *
spnew_udc_alloc_request(struct usb_ep *_ep, gfp_t mem_flags)
{
	struct spnew_ep *ep = to_spnew_ep(_ep);
	struct spnew_request *req;

	dprintk(DEBUG_VERBOSE, "%s(%p, %d)\n", __func__, _ep, mem_flags);

	if (!_ep)
		return NULL;

	if (_ep->name == ep0name) {
		ep->bEndpointAddress = 0;
		ep->halted = 0;
	}

	req = kzalloc(sizeof(struct spnew_request), mem_flags);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

/*
 *  spnew_udc_free_request
 */
static void
spnew_udc_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct spnew_ep *ep = to_spnew_ep(_ep);
	struct spnew_request *req = to_spnew_req(_req);

	dprintk(DEBUG_VERBOSE, "%s(%p, %p)\n", __func__, _ep, _req);

	if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
		return;

	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

/*
 *  spnew_udc_queue
 */
static int spnew_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
				gfp_t gfp_flags)
{
	struct spnew_request *req = to_spnew_req(_req);
	struct spnew_ep *ep = to_spnew_ep(_ep);
	struct spnew_udc *dev;
	unsigned long flags;
	int ret = 0;
	u8 is_in;

	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_VERBOSE, "%s: invalid arge\n", __func__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN))
		return -EINVAL;

	local_irq_save(flags);

	if (unlikely(!_req || !_req->complete ||
	    !_req->buf || !list_empty(&req->queue))) {
		if (!_req)
			dprintk(DEBUG_NORMAL, "%s: 1 X X X\n", __func__);
		else {
			dprintk(DEBUG_NORMAL, "%s: 0 %01d %01d %01d\n", 
			  __func__, !_req->complete, !_req->buf,
			  !list_empty(&req->queue));
		}

		local_irq_restore(flags);
		return -EINVAL;
	}

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	dprintk(DEBUG_VERBOSE, "%s: ep%x len %d\n", 
	   __func__, ep->bEndpointAddress, _req->length);

	/* kickstart this i/o queue? */
	if (/*list_empty(&ep->queue) &&*/!ep->halted) {
		if (ep->bEndpointAddress == 0 /*ep0*/) {
			switch (dev->ep0state) {
				case EP0_IN_DATA_PHASE:
					is_in = 1;
					break;
				case EP0_OUT_DATA_PHASE:
					is_in = 0;
					break;
				default:
					local_irq_restore(flags);
					return -EL2HLT;
			}

			ret = fill_trans_ring(ep, req, is_in);
			if (ret < 0) {
				printk(KERN_ERR "fill ep0 trans_ring fail, is_in:%d\n", is_in);
				return ret;
			}

			pep_enqueue[0]->entry3 |= TRB_IOC;    /* trigger interrupt */
			dev->ep0state = EP0_IDLE;
			req = NULL;
		} else if ((ep->bEndpointAddress & USB_DIR_IN) != 0) {
			is_in = 1;
			ret = fill_trans_ring(ep, req, is_in);
			if (ret < 0) {
				printk(KERN_ERR "fill ep%d in trans_ring fail\n", ep->num);
				return ret;
			}
			req = NULL;
		} else {
			is_in = 0;
			ret = fill_trans_ring(ep, req, is_in);
			if (ret < 0) {
				printk(KERN_ERR "fill ep%d out trans_ring fail\n", ep->num);
				return ret;
			}
			req = NULL;
		}
	}

	/* pio or dma irq handler advances the queue */
	if (likely(ret != 0))
		list_add_tail(&req->queue, &ep->queue);

	local_irq_restore(flags);
	dprintk(DEBUG_VERBOSE, "%s: ok\n", __func__);

	return 0;
}

/*
 *  spnew_udc_dequeue
 */
static int spnew_udc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct spnew_ep *ep = to_spnew_ep(_ep);
	int retval = -EINVAL;
	unsigned long flags;
	struct spnew_request *req = NULL;

	dprintk(DEBUG_VERBOSE, "%s:(%p, %p)\n", __func__, _ep, _req);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	local_irq_save(flags);

	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req) {
			list_del_init(&req->queue);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		dprintk(DEBUG_VERBOSE,
		  "dequeued req %p from %s, len %d buf %p\n", 
	          req, _ep->name, _req->length, _req->buf);

		spnew_udc_done(ep, req, -ECONNRESET);
	}

	local_irq_restore(flags);

	return retval;
}

/*
 *  spnew_udc_set_halt
 */
static int spnew_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct spnew_ep *ep = to_spnew_ep(_ep);
	unsigned long flags;
	u32 idx;

	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "%s: inval 2\n", __func__);
		return -EINVAL;
	}

	local_irq_save(flags);

	idx = ep->bEndpointAddress & 0x7F;

	if (value)
		udc_write(udc_read(EP_CS(idx)) | EP_STALL, EP_CS(idx)); /* ep[idx] stall */

	ep->halted = value ? 1 : 0;
	local_irq_restore(flags);

	return 0;
}


static const struct usb_ep_ops spnew_ep_ops = {
	.enable	       = spnew_udc_ep_enable,
	.disable       = spnew_udc_ep_disable,

	.alloc_request = spnew_udc_alloc_request,
	.free_request  = spnew_udc_free_request,

	.queue	       = spnew_udc_queue,
	.dequeue       = spnew_udc_dequeue,

	.set_halt      = spnew_udc_set_halt,
};


/*------------------------------------- usb gadget ops
-------------------------------------------*/

/*
 *  spnew_udc_get_frame
 */
static int spnew_udc_get_frame(struct usb_gadget *_gadget)
{
	int tmp;

	dprintk(DEBUG_VERBOSE, "%s:()\n", __func__);

	tmp = GET_FRNUM(udc_read(USBD_FRNUM_ADDR));
	return tmp;
}

/*
 *  spnew_udc_wakeup
 */
static int spnew_udc_wakeup(struct usb_gadget *_gadget)
{
	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);
	return 0;
}

/*
 *  spnew_udc_set_selfpowered
 */
static int spnew_udc_set_selfpowered(struct usb_gadget *gadget, int value)
{
	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);
	return 0;
}

static void spnew_udc_disable(struct spnew_udc *dev);
static void spnew_udc_enable(struct spnew_udc *dev);

static int spnew_udc_set_pullup(struct spnew_udc *udc, int is_on)
{
	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);

	if (is_on)
		spnew_udc_enable(udc);
	else {
		if (udc->gadget.speed != USB_SPEED_UNKNOWN) {
			if (udc->driver && udc->driver->disconnect)
				udc->driver->disconnect(&udc->gadget);
		}
		spnew_udc_disable(udc);
	}

	return 0;
}

static int spnew_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{
	struct spnew_udc *udc = to_spnew_udc(gadget);

	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);

	udc->vbus = (is_active != 0);
	spnew_udc_set_pullup(udc, is_active);

	return 0;
}

static int spnew_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	struct spnew_udc *udc = to_spnew_udc(gadget);

	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);

	spnew_udc_set_pullup(udc, is_on ? 0 : 1);

	return 0;
}

static int spnew_vbus_draw(struct usb_gadget *gadget, unsigned ma)
{
	dprintk(DEBUG_NORMAL, "%s:()\n", __func__);

	return -ENOTSUPP;
}

static struct usb_ep *spnew_match_ep(struct usb_gadget *gadget,
		struct usb_endpoint_descriptor *desc,
		struct usb_ss_ep_comp_descriptor *ep_comp)
{
	struct usb_ep *ep = NULL;
	u8 type;

	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	if (gadget_is_i143(gadget)) {
		DEBUG_NOTICE("wei ep config\n");

		if (type == USB_ENDPOINT_XFER_BULK) {
			ep = find_ep(gadget,
				(USB_DIR_IN & desc->bEndpointAddress) ? "ep1-bulk" : "ep3-bulk");
		}
	}

	return ep;
}

static int spnew_udc_start(struct usb_gadget *gadget,
			struct usb_gadget_driver *driver);
static int spnew_udc_stop(struct usb_gadget *gadget);

static const struct usb_gadget_ops spnew_ops = {
	.get_frame       	= spnew_udc_get_frame,
	.wakeup          	= spnew_udc_wakeup,
	.set_selfpowered 	= spnew_udc_set_selfpowered,
	.pullup          	= spnew_udc_pullup,
	.vbus_session    	= spnew_udc_vbus_session,
	.vbus_draw       	= spnew_vbus_draw,
	.udc_start       	= spnew_udc_start,
	.udc_stop       	= spnew_udc_stop,
	.match_ep	 	= spnew_match_ep,
};

/*------------------------------------- gadget driver
handling--------------------------------------*/

/*
 *  spnew_udc_disable
 */
static void spnew_udc_disable(struct spnew_udc *dev)
{
	printk(KERN_NOTICE "%s()\n", __func__);

	/* Disable all interrupts */
	udc_write(0x00, DEVC_IMAN);

	/* Disable udc controller */
	udc_write(udc_read(DEVC_CS) & (~UDC_RUN), DEVC_CS);

	/* Clear the interrupt registers */

	/* Set speed to unknown */
	dev->gadget.speed = USB_SPEED_UNKNOWN;
}

/*
 *  spnew_udc_reinit
 */
static void spnew_udc_reinit(struct spnew_udc *dev)
{
	u32 i;

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0state = EP0_IDLE;

	for(i = 0; i < SPNEW_ENDPOINTS; i++) {
		struct spnew_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->dev = dev;
		ep->desc = NULL;
		ep->ep.desc = NULL;
		ep->halted = 0;
		INIT_LIST_HEAD(&ep->queue);
	}
}

/*
 *  spnew_udc_enable
 */
static void spnew_udc_enable(struct spnew_udc *dev)
{
	dprintk(DEBUG_NORMAL, "spnew_udc_enable called\n");

	/* dev->gadget.speed = USB_SPEED_UNKNOWN; */
	dev->gadget.speed = USB_SPEED_FULL;

	/* Enable interrupt */
	udc_write(DEVC_INTR_ENABLE, DEVC_IMAN);

	/* Enable udc controller */
	udc_write(udc_read(DEVC_CS) | UDC_RUN, DEVC_CS);
}

static int spnew_udc_start(struct usb_gadget *gadget,
			struct usb_gadget_driver *driver)
{
	struct spnew_udc *udc = the_controller;
#if 0
	int retval;
#endif

	dprintk(DEBUG_NORMAL, "%s:() '%s'\n", __func__, driver->driver.name);

	/* Sanity checks */
	if (!udc)
		return -ENODEV;

	if (udc->driver)
		return -EBUSY;

	if (!driver->setup || driver->max_speed < USB_SPEED_FULL) {
		printk(KERN_ERR "Invalid driver: setup %p speed %d\n",
		  			driver->setup, driver->max_speed);
		return -EINVAL;
	}

#if defined(MODULE)
	if (!driver->unbind) {
		printk(KERN_ERR "Invalid driver: no unbind method\n");
		return -EINVAL;;
	}
#endif

	/* Hook the driver */
	udc->driver = driver;
	udc->gadget.dev.driver = &driver->driver;

#if 0
	/* Bind the driver */
	if ((retval = device_add(&udc->gadget.dev)) != 0) {
		printk(KERN_ERR "Error in device_add() : %d\n", retval);
		goto register_error;
	}

	dprintk(DEBUG_NORMAL, "binding gadget driver '%s'\n", driver->driver.name);

	if ((retval = bind(&udc->gadget)) != 0) {
		device_del(&udc->gadget.dev);
		goto register_error;
	}
#endif

	/*Enable udc */
	spnew_udc_enable(udc);

	return 0;

#if 0
register_error:
	udc->driver = NULL;
	udc->gadget.dev.driver = NULL;
	return retval;
#endif
}

static int spnew_udc_stop(struct usb_gadget *gadget)
{
	struct spnew_udc *udc = the_controller;

	if (!udc)
		return -ENODEV;

	if (!udc->driver || !udc->driver->unbind)
		return -EINVAL;

	dprintk(DEBUG_NORMAL, "usb_gadget_unregister_driver() '%s'\n", udc->driver->driver.name);

	/* report disconnect */
	if (udc->driver->disconnect)
		udc->driver->disconnect(gadget);

	udc->driver->unbind(gadget);

	device_del(&gadget->dev);
	udc->driver = NULL;

	/* Disable udc */
	spnew_udc_disable(udc);

	return 0;
}

/*---------------------------------------------------------------------------------
*/
static struct spnew_udc memory = {
	.gadget = {
		.ops			= &spnew_ops,
		.ep0			= &memory.ep[0].ep,
		.name			= gadget_name,
		.dev = {
			.init_name 	= "gadget",
		},
	},

	/* control endpoint */
	.ep[0] = {
		.num			= 0,
		.ep = {
			.name		= ep0name,
			.ops		= &spnew_ep_ops,
			.maxpacket	= EP0_FIFO_SIZE,
		},
		.dev			= &memory,
	},

	/* first group of endpoints */
	.ep[1] = {
		.num			= 1,
		.ep = {
			.name		= "ep1-bulk",
			.ops		= &spnew_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev			= &memory,
		.fifo_size		= EP_FIFO_SIZE,
		.bEndpointAddress	= 1,
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[2] = {
		.num			= 2,
		.ep = {
			.name		= "ep2-bulk",
			.ops		= &spnew_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev			= &memory,
		.fifo_size		= EP_FIFO_SIZE,
		.bEndpointAddress	= 2,
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[3] = {
		.num			= 3,
		.ep = {
			.name		= "ep3-bulk",
			.ops		= &spnew_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev			= &memory,
		.fifo_size		= EP_FIFO_SIZE,
		.bEndpointAddress	= 3,
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},
};

/*
 *  probe - binds to the platform device
 */
static void fill_ep0_desc(struct udc_endpoint_desc *desc)
{
	desc->ep_entry0 |= (UDC_HIGH_SPEED << 16);
	desc->ep_entry1 = 0;
	desc->ep_entry2 = EP_FIFO_SIZE | (1 << 30);
	desc->ep_entry3 = (u32)((u64)ep_transfer_ring[0] | (1 << 0));

	return;
}

static int spnew_ep_desc_init(void)
{
	struct udc_endpoint_desc *ep0_desc;
	struct trb *end_trb;

	ep_transfer_ring[0] = (struct trb *)
		              kzalloc(TRANSFER_RING_SIZE + 15, GFP_KERNEL);

	if (!ep_transfer_ring[0]) {
		printk(KERN_NOTICE "malloc ep0 transfer ring fail\n");
		return -1;
	}

	if (((u64)ep_transfer_ring[0] & 0xF) != 0)
		ep_transfer_ring[0] = (struct trb *)(((u64)ep_transfer_ring[0] & ~0xF) + 0x10);

	end_trb = ep_transfer_ring[0];
	end_trb += (TRANSFER_RING_SIZE / ENTRY_SIZE - 1);
	fill_link_trb(end_trb, ep_transfer_ring[0]);

	spnew_device_desc = (struct udc_endpoint_desc *)
		            kzalloc((ENTRY_SIZE * ENDPOINT_NUM) + 31, GFP_KERNEL);
	if (!spnew_device_desc) {
		printk(KERN_NOTICE "malloc device desc fail\n");
		goto err1;
	}

	pep_enqueue[0] = ep_transfer_ring[0];

	if (((u64)spnew_device_desc & 0x1F) != 0)
		spnew_device_desc = (struct udc_endpoint_desc *)
					(((u64)spnew_device_desc & ~0x1F) + 0x20);

	ep0_desc = spnew_device_desc;
	fill_ep0_desc(ep0_desc);
	udc_write((u32)((u64)spnew_device_desc), DEVD_ADDR);	/* SETUP DCBA */
	udc_write(udc_read(DEVC_CS) | (1 << 0), DEVC_CS);	/* reload ep0 desc */
	udc_write(RDP_EN | EP_EN, EP0_CS);			/* enable ep0 */

	return 0;

err1:
	kfree(ep_transfer_ring[0]);

	return -1;
}

static int spnew_event_ring_init(void)
{
	struct ers_table *er_seg;
	struct trb *end_trb;
	int ret;

	event_ring_start = (struct trb *)kzalloc(EVENT_RING_SIZE + 16 + 63, GFP_KERNEL);
	if (!event_ring_start) {
		printk(KERN_NOTICE "malloc event ring fail\n");
		return -1;
	}

	event_ring_seg_table = (struct ers_table *)
		               kzalloc((EVENT_SEGMENT_COUNT * ENTRY_SIZE) + 63, GFP_KERNEL);
	if (!event_ring_seg_table) {
		printk(KERN_NOTICE "malloc event ring fail\n");
		goto malloc_fail1;
	}

	if (((u64)event_ring_seg_table & ~0x3f) == 0)
		er_seg = event_ring_seg_table;
	else
		er_seg = (struct ers_table *)(((u64)event_ring_seg_table & ~0x3f) + 0x40);

	if (((u64)event_ring_start & ~0x3f) == 0)
		er_seg->rsba = (u32)((u64)event_ring_start);
	else
		er_seg->rsba = (u32)(((u64)event_ring_start & ~0x3f) + 0x40);

	er_seg->rssz = EVENT_RING_SIZE / ENTRY_SIZE;	/* the number of TRBs */

	end_trb = (struct trb *)&er_seg->rsba;
	end_trb += (EVENT_RING_SIZE / ENTRY_SIZE);
	fill_link_trb(end_trb, (struct trb *)&er_seg->rsba);

	udc_write(EVENT_SEGMENT_COUNT, DEVC_ERSTSZ);
	udc_write(er_seg->rsba, DEVC_ERDP);
	udc_write((u32)((u64)er_seg), DEVC_ERSTBA); /* enable event ring */
	udc_write(4000, DEVC_IMOD); /* interrupt interval */
	udc_write(DEVC_INTR_ENABLE, DEVC_IMAN); /* enable interrupt */

	/* alloc ep0 transfer ring */
	ret = spnew_ep_desc_init();
	if (ret < 0) {
		printk(KERN_NOTICE "malloc spnew_ep_desc_init fail\n");
		goto malloc_fail2;
	}

	//udc_write(udc_read(DEVC_CS) | UDC_RUN, DEVC_CS); /* enable udc controller */

	return 0;

malloc_fail2:
	kfree(event_ring_seg_table);
malloc_fail1:
	kfree(event_ring_start);

	return -1;
}

static int spnew_udc_probe(struct platform_device *pdev)
{
	struct spnew_udc *udc = &memory;
	struct device *dev = &pdev->dev;
	struct resource *res0, *res1;
	int retval;

	dev_dbg(dev, "%s()\n", __func__);

	/*enable usb controller clock*/
	udc_clock = devm_clk_get(dev, NULL);
	if (IS_ERR(udc_clock)) {
		pr_err("not found clk source\n");
		return PTR_ERR(udc_clock);
	}

	clk_prepare(udc_clock);
	clk_enable(udc_clock);
	mdelay(10);

	dev_dbg(dev, "got and enabled clocks\n");

	if (strncmp(pdev->name, "spnew", 5) == 0) {
		dev_info(dev, "spnew: increasing FIFO to 512 bytes\n");
		memory.ep[1].fifo_size = SPNEW_EP_FIFO_SIZE;
		memory.ep[2].fifo_size = SPNEW_EP_FIFO_SIZE;
		memory.ep[3].fifo_size = SPNEW_EP_FIFO_SIZE;
		memory.ep[4].fifo_size = SPNEW_EP_FIFO_SIZE;
	}

	spin_lock_init(&udc->lock);

	res0 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = ioremap(res0->start, resource_size(res0));
	if (!base_addr) {
		retval = -ENOMEM;
		goto err_mem0;
	}

	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	moon5_base_addr = ioremap(res1->start, resource_size(res1));
	if (!moon5_base_addr) {
		retval = -ENOMEM;
		goto err_mem1;
	}

	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = &pdev->dev;
	udc->gadget.dev.dma_mask = pdev->dev.dma_mask;

	the_controller = udc;
	platform_set_drvdata(pdev, udc);

	spnew_udc_disable(udc);
	spnew_udc_reinit(udc);
	spnew_event_ring_init();

	/* irq setup after old hardware state is cleaned up */
	irq_num = platform_get_irq(pdev, 0);
	if (!irq_num) {
		dev_err(dev, "Not enough platform resources.\n");
		retval = -ENODEV;
		goto err_map;
	}

	retval = request_irq(irq_num, spnew_udc_irq, IRQF_SHARED, gadget_name, udc);
	if (retval != 0) {
		dev_err(dev, "cannot get irq %d, err %d\n", irq_num, retval);
		retval = -EBUSY;
		goto err_int;
	}

	gSpnew_udc = udc;
	dev_dbg(dev, "got irq %d\n", irq_num);

	/* event ring initialized */
	retval = usb_add_gadget_udc(dev, &udc->gadget);
	if (retval)
		goto err_int;

	udc->event_workqueue = create_singlethread_workqueue("spnew-udc-event");
	if (!udc->event_workqueue) {
		printk("cannot creat workqueue spnew-udc-event\n");
		retval = -ENOMEM;
		goto err_int;
	}
	INIT_WORK(&udc->event_work, handle_event);

	device_create_file(&pdev->dev, &dev_attr_udc_ctrl);

	dev_dbg(dev, "probe ok\n");

	return 0;

err_int:
	if(irq_num)
		free_irq(irq_num, udc);
err_map:
	iounmap(moon5_base_addr);
	iounmap(base_addr);
err_mem1:
	release_mem_region(res1->start, resource_size(res1));
err_mem0:
	release_mem_region(res0->start, resource_size(res0));

	return retval;
}

/*
 *  spnew_udc_remove
 */
static int spnew_udc_remove(struct platform_device *pdev)
{
	struct spnew_udc *udc = platform_get_drvdata(pdev);
	struct resource *res;

	dev_dbg(&pdev->dev, "%s()\n", __func__);

	usb_del_gadget_udc(&udc->gadget);
	if(udc->driver)
		return -EBUSY;

	free_irq(IRQ_USBD, udc);

	iounmap(base_addr);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res) {
		release_mem_region(res->start, resource_size(res));
	}

	iounmap(moon5_base_addr);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(res) {
		release_mem_region(res->start, resource_size(res));
	}

	platform_set_drvdata(pdev, NULL);

	if (!IS_ERR(udc_clock) && udc_clock != NULL) {
		clk_disable(udc_clock);
		clk_put(udc_clock);
		udc_clock = NULL;
	}

	dev_dbg(&pdev->dev, "%s: remove ok\n", __func__);
	return 0;
}

#define spnew_udc_suspend NULL
#define spnew_udc_resume  NULL

static int udc_init_c(void)
{
	spnew_udc_enable(NULL);
	udc_write(udc_read(DEVC_IMAN) & (~DEVC_INTR_ENABLE), DEVC_IMAN);

	return 0;
}

void spnew_udc_state_polling(struct timer_list *t)
{
	if (!platform_device_handle_flag
		|| !bus_reset_finish_flag){
		DEBUG_DBG("finish_flag: %d,handle_flag: %d\n",
			bus_reset_finish_flag, platform_device_handle_flag);
		return ;
	}
	if (sof_value == GET_FRNUM(udc_read(USBD_FRNUM_ADDR)) /*&& sof_value */ ) {
		DEBUG_NOTICE("SOF %x\n", GET_FRNUM(udc_read(USBD_FRNUM_ADDR)));
		sof_value = 0x1000;
		/* discnnect */
		usb_switch(TO_HOST);
	} else {
		sof_value = GET_FRNUM(udc_read(USBD_FRNUM_ADDR));
		if (is_config) {
			mod_timer(&vbus_polling_timer,
				  jiffies + d_time1);
		} else {
			mod_timer(&vbus_polling_timer,
				  jiffies + d_time0);
		}
	}
}

void sp_sof_state_polling(struct timer_list *t)
{
	if (!platform_device_handle_flag
		|| bus_reset_finish_flag){
		DEBUG_DBG("finish: %d,handle: %d\n",
			bus_reset_finish_flag,platform_device_handle_flag);
		return ;
	}

	if (first_enter_polling_timer_flag) {
		temp_sof_value = GET_FRNUM(udc_read(USBD_FRNUM_ADDR));
		first_enter_polling_timer_flag = false;
		mod_timer(&sof_polling_timer,
			  jiffies + 3 * HZ / 2);
	} else {
		if (temp_sof_value == GET_FRNUM(udc_read(USBD_FRNUM_ADDR))) {
			DEBUG_NOTICE("SOFX %x\n",
				     GET_FRNUM(udc_read(USBD_FRNUM_ADDR)));
			/* discnnect */
			usb_switch(TO_HOST);
		} else {
			temp_sof_value = GET_FRNUM(udc_read(USBD_FRNUM_ADDR));
			mod_timer(&sof_polling_timer,
				  jiffies + 3 * HZ / 2);
		}
	}
}

void usb_switch(int device)
{
	if (device) {
		if (accessory_port_id == 0) {
			udc_writeb(moon5_base_addr, RF_MASK_V_SET(1 << 4), USBC_CTL_OFFSET);
			udc_writeb(moon5_base_addr, RF_MASK_V_CLR(1 << 5), USBC_CTL_OFFSET);
		} else if (accessory_port_id == 1){
			udc_writeb(moon5_base_addr, RF_MASK_V_SET(1 << 12), USBC_CTL_OFFSET);
			udc_writeb(moon5_base_addr, RF_MASK_V_CLR(1 << 13), USBC_CTL_OFFSET);
		}

		DEBUG_ERR("host to device\n");
	} else {
		udc_write(udc_read(DEVC_CS) & (~UDC_RUN), DEVC_CS);

		if (is_config == 1)
			is_config = 0;

		if (accessory_port_id == 0) {
			udc_writeb(moon5_base_addr, RF_MASK_V_SET(1 << 5), USBC_CTL_OFFSET);
		} else if (accessory_port_id == 1){
			udc_writeb(moon5_base_addr, RF_MASK_V_SET(1 << 13), USBC_CTL_OFFSET);
		}

		bus_reset_finish_flag = false;
		platform_device_handle_flag = false;

		DEBUG_ERR("device to host!\n");
	}
}
EXPORT_SYMBOL(usb_switch);

void detech_start(void)
{
	udc_init_c();

	platform_device_handle_flag = true;
	first_enter_polling_timer_flag = true;

	DEBUG_ERR("detech_start......\n");
}
EXPORT_SYMBOL(detech_start);

static const struct of_device_id spnew_udc0_ids[] = {
	{ .compatible = "sunplus,i143-usb-udc0" },
	{ }
};
MODULE_DEVICE_TABLE(of, spnew_udc0_ids);

static const struct of_device_id spnew_udc1_ids[] = {
	{ .compatible = "sunplus,i143-usb-udc1", },
	{ }
};
MODULE_DEVICE_TABLE(of, spnew_udc1_ids);

static struct platform_driver spnew_udc0_driver = {
	.driver	  = {
		.name  = "spnew-usbgadget0",
		.owner = THIS_MODULE,
		.of_match_table = spnew_udc0_ids,
	},
	.probe 	  = spnew_udc_probe,
	.remove   = spnew_udc_remove,
	.suspend  = spnew_udc_suspend,
	.resume   = spnew_udc_resume,
};

static struct platform_driver spnew_udc1_driver = {
	.driver	  = {
		.name  = "spnew-usbgadget1",
		.owner = THIS_MODULE,
		.of_match_table = spnew_udc1_ids,
	},
	.probe 	  = spnew_udc_probe,
	.remove   = spnew_udc_remove,
	.suspend  = spnew_udc_suspend,
	.resume   = spnew_udc_resume,
};

static int __init udc_init(void)
{
	int retval = 0;

	dprintk(DEBUG_NORMAL, "%s: version %s\n", gadget_name, DRIVER_VERSION);

	if (accessory_port_id == USB_PORT0_ID) {
		printk(KERN_NOTICE "register sunplus_udc0_driver\n");
		retval = platform_driver_register(&spnew_udc0_driver);
	} else if (accessory_port_id == USB_PORT1_ID) {
		printk(KERN_NOTICE "register sunplus_udc1_driver\n");
		retval = platform_driver_register(&spnew_udc1_driver);
	}

	timer_setup(&vbus_polling_timer, spnew_udc_state_polling, 0);
	vbus_polling_timer.expires = jiffies - HZ;

	timer_setup(&sof_polling_timer, sp_sof_state_polling, 0);
	sof_polling_timer.expires = jiffies + 3 * HZ / 2;

	return retval;
}

static void __exit udc_exit(void)
{
	if (accessory_port_id == USB_PORT0_ID) {
		platform_driver_unregister(&spnew_udc0_driver);
	} else if (accessory_port_id == USB_PORT1_ID) {
		platform_driver_unregister(&spnew_udc1_driver);
	}
}

module_init(udc_init);
module_exit(udc_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
