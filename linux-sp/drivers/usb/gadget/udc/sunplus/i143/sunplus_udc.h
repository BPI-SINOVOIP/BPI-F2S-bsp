/*
 * linux/drivers/usb/gadget/udc/spnew_udc.h
 * Sunplus on-chip full/high speed USB device controllers
 *
 * Copyright (C) 2004-2007 Herbert Potzl - Arnaud Patard
 *  Additional cleanups by Ben Dooks <ben-linux@fluff.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) aby later version.
 */

#ifndef _SPNEW_UDC_H
#define _SPNEW_UDC_H

#define SPNEW_PA_USBDEV		0xBFFE8000
#define SPNEW_SZ_USBDEV		(USBD_DEBUG_PP - DEVC_VER + 4)	// 0 ??
#define IRQ_USBD		25

struct spnew_ep {
	struct list_head queue;
	unsigned long last_io;		/* jiffies timestamp*/
	struct usb_gadget *gadget;
	struct spnew_udc *dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep ep;
	u8 num;

	unsigned short fifo_size;
	u8 bEndpointAddress;
	u8 bmAttributes;
	spinlock_t lock;

	unsigned halted:1;
	unsigned already_seen:1;
	unsigned setup_stage:1;
};


/* Warning : ep0 has a fifo of 16 bytes */
/* Don't try to set 32 or 64            */
/* also testusb 14 fails with 16 bit is */
/* fine with 8                          */
#define EP0_FIFO_SIZE		8
#define EP_FIFO_SIZE		64
#define DEFAULT_POWER_STATE	0x00

#define ENTRY_SIZE		16
#define SPNEW_EP_FIFO_SIZE	512
#define EVENT_RING_SIZE		4*1024
#define EVENT_SEGMENT_COUNT	1
#define ENDPOINT_NUM		31
#define TRANSFER_RING_SIZE	512

#define CONTROL_TRANS		0
#define ISO_TRANS		0x1
#define BULK_TRANS		0x2
#define INT_TRANS		0x3
#define UDC_FULL_SPEED		0x1
#define UDC_HIGH_SPEED		0x3

#define NORMAL_TRB		1
#define SETUP_TRB		2
#define LINK_TRB		6
#define TRANS_EVENT_TRB		32
#define DEV_EVENT_TRB		48
#define SOF_EVENT_TRB		49

static const char ep0name[] = "ep0";

static const char *const ep_name[31] = {
	ep0name,			/* everyone has ep0 */
	/* SPNEW four bidirectional bulk endpoints */
	"ep1-bulk", "ep2-bulk", "ep3-bulk",
};

#define SPNEW_ENDPOINTS		ARRAY_SIZE(ep_name)

struct spnew_request {
	struct list_head queue;
	struct usb_request req;
};

enum ep0_state {
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_END_XFER,
	EP0_STALL,
};

struct spnew_udc {
	spinlock_t lock;

	struct spnew_ep ep[SPNEW_ENDPOINTS];
	int address;
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct spnew_request fifo_req;
	u8 fifo_buf[EP_FIFO_SIZE];
	u16 devstatus;

	u32 port_status;
	int ep0state;

	unsigned got_irq:1;

	unsigned req_std:1;
	unsigned req_config:1;
	unsigned req_pending:1;
	u8 vbus;
	struct dentry *regs_info;
	struct work_struct event_work;
	struct workqueue_struct *event_workqueue;
};

/* endpoint ID0 descriptor */
/* endpoint IDn descriptor */
struct udc_endpoint_desc {
	u32 ep_entry0;
	u32 ep_entry1;
	u32 ep_entry2;
	u32 ep_entry3;
};

struct trb {
	u32 entry0;
	u32 entry1;
	u32 entry2;
	u32 entry3;
};

/* frame number */
#define GET_FRNUM(x)		((x & 0x3FFF) >> 3)
/* endpoint n (n != 0) descriptor */
#define DESC_TRDP(x)		((x) & 0xFFFFFFF0)
/* transfer ring normal TRB */
#define TRB_PTR(x)		(x)			/* entry0 */
#define TRB_TLEN(x)		((x) & (0x0001FFFF))	/* entry2 */
#define TRB_C			(1 << 0)		/* entry3 */
#define TRB_ISP			(1 << 2)
#define TRB_IOC			(1 << 5)
#define TRB_IDT			(1 << 6)
#define TRB_BEI			(1 << 9)
#define TRB_DIR			(1 << 16)
/* ISO */
#define TRB_FID(x)		((x >> 20) & 0x000007FF)
#define TRB_SIA			(1 << 31)
/* transfer ring Link TRB */
#define LTRB_PTR(x)		((x >> 4) & 0x0FFFFFFF)	/* entry0 */
#define LTRB_C			(1 << 0)		/* entry3 */
#define LTRB_TC			(1 << 1)
#define LTRB_IOC		(1 << 5)
#define LTRB_TYPE(x)		((x >> 10) & 0x3F)
/* Device controller event TRB */
#define ETRB_CC(x)		((x >> 24) & 0xFF)	/* entry2 */
#define ETRB_C			(1 << 0)		/* entry3 */
#define ETRB_TYPE(x)		((x >> 10) & 0x3F)
/* Setup stage event TRB */
#define ETRB_SDL(x)		(x)			/* entry0:wValue,bRequest,bmRequestType */
#define ETRB_SDH(x)		(x)			/* entry1:wLength,wIndex */
#define ETRB_ENUM(x)		((x >> 16) & 0x0F)	/* entry3 */
/* Transfer event TRB*/
#define ETRB_TRBP(x)		(x)			/* entry0 */
#define ETRB_LEN(x)		(x & 0xFFFFFF)		/* entry2 */
#define ETRB_EID(x)		((x >> 16) & 0x1F)	/* entry3 */
/* SOF event TRB */
#define ETRB_FNUM(x)		(x & 0x1FFFF)		/* entry2 */
#define ETRB_SLEN(x)		((x >> 11) & 0x1FFF)

struct ers_table {
	u32 rsba;
	u32 rfu1;
	u32 rssz;
	u32 rfu2;
};

#define ERDP_MASK		0xFFFFFFF0
/* completion codes */
#define INVALID_TRB		0
#define SUCCESS_TRB		1
#define DATA_BUF_ERR		2
#define TRANS_ERR		4
#define TRB_ERR			5
#define RESOURCE_ERR		7
#define SHORT_PACKET		13
#define EVENT_RING_FULL		21
#define STOPPED			26
#define UDC_RESET		192
#define UDC_SUSPEND		193
#define UDC_RESUME		194

/**/
#define DEVC_VER		0x00
#define DEVC_MMR		0x04
#define DEVC_PARAM		0x10
#define GL_CS			0x20

#define DEVC_IMAN		0x80
#define DEVC_INTR_ENABLE	(1 << 1)
#define DEVC_IMOD		0x84
#define DEVC_ERSTSZ		0x88
#define DEVC_ERSTBA		0x90
#define DEVC_ERDP		0x98
#define DEVD_ADDR		0xA0
#define DEVC_CTRL		0xA8
#define DEVC_STS		0xAC
#define VBUS			(1 << 16)
#define EINT			(1 << 1)
#define VBUS_CI			(1 << 0)
#define DEVC_DTOGL		0xF8
#define DEVC_DTOGH		0xFC

#define DEVC_CS			0x100
#define UDC_RUN			(1 << 31)
#define EP0_CS			0x104
#define EP_STALL		(1 << 2)
#define RDP_EN			(1 << 1)
#define EP_EN			(1 << 0)
#define EP1_CS			0x108

#define MAC_FSM			0x180
#define DEV_EP0_FSM		0x184
#define EP1_FSM			0x188

#define USBD_CFG		0x200
#define USBD_INF		0x204
#define EP1_INF			0x208

#define USBD_FRNUM_ADDR		0x280
#define USBD_DEBUG_PP		0x284

#define EP_CS(idx)		(EP1_CS + (idx - 1) * 4)

static u32 event_ccs = 1;
static struct spnew_udc *gSpnew_udc = NULL;
static struct trb *event_ring_start = NULL;
static struct trb *ep_transfer_ring[SPNEW_ENDPOINTS] = {NULL};
static struct ers_table *event_ring_seg_table = NULL;
static struct udc_endpoint_desc *spnew_device_desc = NULL;
static struct trb *pep_enqueue[SPNEW_ENDPOINTS] = {NULL};
#endif
