#ifndef _SUNPLUS_UDC_H
#define _SUNPLUS_UDC_H

#include <linux/wakelock.h>

#define SP_MAXENDPOINTS      13
#define EP0_FIFO_SIZE		 64	/*control Endpoint */
#define EP12_FIFO_SIZE64	 64	/*full speed */
#define EP_FIFO_SIZE		 64
#define CONFIG_USB_SUNPLUS_EP11
//#define CONFIG_USB_SUNPLUS_EP1
#define CONFIG_USB_SUNPLUS_EP2
#undef CONFIG_FIQ_GLUE

static const char ep0name[] = "ep0";

enum ep0_state {
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_END_XFER,
	EP0_STALL,
};

static const char *ep0states[] = {
	"EP0_IDLE",
	"EP0_IN_DATA_PHASE",
	"EP0_OUT_DATA_PHASE",
	"EP0_END_XFER",
	"EP0_STALL",
};

struct sp_ep {
	struct list_head queue;
	unsigned long last_io;	/* jiffies timestamp */
	struct usb_gadget *gadget;
	struct sp_udc *dev;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep ep;
	u8 num;

	unsigned short fifo_size;
	u8 bEndpointAddress;
	u8 bmAttributes;

	unsigned halted:1;
	unsigned already_seen:1;
	unsigned setup_stage:1;
	spinlock_t lock;
};

struct sp_request {
	struct list_head queue;	/* ep's requests */
	struct usb_request req;
	u8 *cmd_trb;
};

struct sp_udc {
	spinlock_t lock;

	struct sp_ep ep[SP_MAXENDPOINTS];
	int address;
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct sp_request fifo_req;

	u8 fifo_buf[EP_FIFO_SIZE];
	u16 devstatus;

	u32 port_status;
	int ep0state;

#ifdef CONFIG_USB_SUNPLUS_OTG

#define		ID_PIN					(1 << 16)
#define		OTG_INT_ST_REG			(3)

	int bus_num;
	int suspend_sta;

	struct work_struct work_otg;
	struct workqueue_struct *qwork_otg;
#endif
#ifdef CONFIG_USB_SUNPLUS_EP1
	struct work_struct work_ep1;
	struct workqueue_struct *qwork_ep1;
#endif
#ifdef CONFIG_USB_SUNPLUS_EP2
	struct work_struct work_ep2;
	struct workqueue_struct *qwork_ep2;
#endif
#ifdef CONFIG_USB_SUNPLUS_EP11
	struct work_struct work_ep11;
	struct workqueue_struct *qwork_ep11;
	struct work_struct work_ep11_dma;
	struct workqueue_struct *qwork_ep11_dma;
#endif

	unsigned got_irq:1;

	unsigned req_std:1;
	unsigned req_config:1;
	unsigned req_pending:1;
	u8 vbus;
	struct dentry *regs_info;
	struct tasklet_struct ep1bulkin_task;
	struct tasklet_struct ep2bulkout_task;
	struct tasklet_struct epabulkin_task;
	struct tasklet_struct epabulkin_dma_task;
	struct tasklet_struct ep8bulkin_task;
	struct tasklet_struct ep9bulkout_task;
	struct wake_lock wake_lock;
#ifdef CONFIG_FIQ_GLUE
	struct fiq_glue_handler handler;
#endif

	struct work_struct work_ep3;
	struct workqueue_struct *qwork_ep3;
	struct work_struct work_ep9;
	struct workqueue_struct *qwork_ep9;
	struct work_struct work_configure;
	struct clk *clk;
};

extern int Q571_get_platform(void);
/*for ISO DMA*/
#define ALIGN_64	64
#define TRB_NUM		256
#define TRB_SIZE	16
#define ALL_TRB_SIZE	(ALIGN_64+TRB_NUM*TRB_SIZE)
#define E_TRB_SIZE	1024
#define RCS_0		0
#define RCS_1		1
#define TRB_CC		(1 << 24)

#define TRB_TC		(1 << 1)
#define TRB_IOC		(1 << 5)
#define TRB_NORMAL	(1 << 10)
#define TRB_LINK	(3 << 11)
#define TRB_IN		(1 << 16)

struct iso_trb {
	u32 ptr;
	u32 rfu;
	u32 size_cc;
	u32 cmd;
};
/* udc endpoint*/
#define EP0			0
#define EP1			1
#define EP2			2
#define EP3			3
#define EP4			4
#define EP5			5
#define EP6			6
#define EP7			7
#define EP8			8
#define EP9			9
#define EP10		10
#define EP11		11
#define EP12		12
#endif
