#include <linux/init.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/usb/gadget.h>
#include <mach/io_map.h>
#include <linux/usb/composite.h>
#include <linux/usb/otg.h>
#include <linux/usb/sp_usb.h>
#include <asm/cacheflush.h>
#include "../../../../arch/arm/mm/dma.h"

#ifdef CONFIG_FIQ_GLUE
#include <asm/fiq.h>
#include <asm/fiq_glue.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#endif


#include <linux/clk.h>
#include <linux/vmalloc.h>

#define ISO_DEBUG_INFO
#ifdef CONFIG_GADGET_USB0
#define USB0
#endif

void usb_switch(int device);

#include "sunplus_udc.h"
#include "sunplus_udc_regs.h"

#define USE_DMA

static int irq_num = 0;
//static u64 sunplus_udc_dma_mask = DMA_BIT_MASK(32);

#define IRQ_USB_DEV_PORT0			45
#define IRQ_USB_DEV_PORT1			48

#define TO_HOST 				0
#define TO_DEVICE				1
#define BUS_RESET_FOR_CHIRP_DELAY		2000	/*ctrl rx singal keep the strongest time */
#define DMA_FLUSH_TIMEOUT_DELAY			300000

#define FULL_SPEED_DMA_SIZE			64
#define HIGH_SPEED_DMA_SIZE			512
#define UDC_FLASH_BUFFER_SIZE 			(1024*64)
#define	DMA_ADDR_INVALID			(~(dma_addr_t)0)

static u32 bulkep_dma_block_size = 0;

struct sp_ep_iso {
	struct list_head queue;
	int act;
};

static u32 dmsg = 1;
module_param(dmsg, uint, 0644);

u32 dma_len_ep1;
u32 dma_xferlen_ep11;
spinlock_t plock, qlock;

/*coverity[declared_but_not_referenced]*/
#ifdef CONFIG_USB_SUNPLUS_OTG
	#ifdef OTG_TEST
static u32 dev_otg_status = 1;
	#else
static u32 dev_otg_status = 0;
	#endif
module_param(dev_otg_status, uint, 0644);
#endif

static const char gadget_name[] = "sp_udc";
static void __iomem *base_addr;
static struct sp_udc *the_controller;

static int dmesg_buf_size = 3000;
module_param(dmesg_buf_size, uint, 0644);
static int dqueue = 1;
module_param(dqueue, uint, 0644);
static char *sp_kbuf = 0;

static void dprintf(const char *fmt, ...)
{
	char buf[200];
	int len;
	int x;
	char *pBuf;
	char *pos;
	char org;

    	static int i=0;
	static int bfirst = 1;

    	va_list args;

	spin_lock(&plock);

	if (sp_kbuf == 0) {
		if((sp_kbuf = kzalloc(dmesg_buf_size, GFP_KERNEL)) == NULL)
			printk(KERN_INFO "kmalloc fail\n");
	}

    	va_start(args, fmt);
	vsnprintf(buf, 200, fmt, args);
	// printk(KERN_INFO "i = %d\n", i);

	len = strlen(buf);
	if (len)
	if (i+len <= dmesg_buf_size) {
		memcpy(sp_kbuf+i, buf, len);
	}
	i += len;
	va_end(args);

	if (dqueue)
		printk(KERN_INFO "%s\n", buf);
	else if (i >= dmesg_buf_size && bfirst == 1) {
		bfirst = 0;
		i = 0;
		dmsg = 32;
		x = 0;

		printk(KERN_INFO ">>> start dump\n");
		printk(KERN_INFO "sp_kbuf: %d, i = %d\n", strlen(sp_kbuf), i);

		pBuf = sp_kbuf;
		while(1) {
			pos = strstr(pBuf, "\n");
			if (pos == NULL) break;
			org = *pos;
			*pos = 0;
			printk(KERN_INFO "%s", pBuf);
			pBuf = pos+1;
		}

		printk(KERN_INFO ">>> end dump\n");
	}

	spin_unlock(&plock);
}

#define DEBUG_INFO(fmt, arg...)		do { if (dmsg&(1<<0)) dprintf(fmt, ##arg); } while(0)
#define DEBUG_NOTICE(fmt, arg...)	do { if (dmsg&(1<<1)) dprintf(fmt, ##arg); } while(0)
#define DEBUG_DBG(fmt, arg...)		do { if (dmsg&(1<<2)) dprintf(fmt, ##arg); } while(0)
#define DEBUG_DUMP(fmt, arg...)		do { if (dmsg&(1<<3)) dprintf(fmt, ##arg); } while(0)
#define DEBUG_PRT(fmt, arg...)		do { if (dmsg&(1<<4)) dprintf(fmt, ##arg); } while(0)

static struct sp_udc memory;

static inline u32 udc_read(u32 reg);
static inline void udc_write(u32 value, u32 reg);
void detech_start(void);

#if 0
static void udc_invalidate_dcache_range(unsigned int start,unsigned int start_pa,unsigned int size){
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	/* invalidate L1 cache by range */
	/*dmac_unmap_area((void *)start,size,2);*/
	/* invalidate L2 cache by range */
	outer_inv_range(start_pa, start_pa+size);
	local_irq_restore(oldIrq);
}
#endif

static ssize_t show_udc_ctrl(struct device *dev,
				struct device_attribute *attr, char *buffer)
{
	u32 result = 0;
	struct sp_ep *ep = &memory.ep[1];

	spin_lock(&ep->lock);
	result = list_empty(&ep->queue);
	spin_unlock(&ep->lock);

	return snprintf(buffer, 3, "%d\n", result);
}

static ssize_t store_udc_ctrl(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t count)
{
	u32 ret = 0;
	static char irq_i = 0;

	ret = udc_read(UDLCSET) & SIM_MODE;
	if (*buffer == 'd') {	/*d:switch uphy to device*/
		DEBUG_NOTICE("user switch \n");
		usb_switch(TO_DEVICE);
		msleep(1);
		detech_start();

		return count;
	} else if (*buffer == 'h') {	/*h:switch uphy to host*/
		DEBUG_NOTICE("user switch \n");
		usb_switch(TO_HOST);

		return count;
	} else if (*buffer == '1') { /* support SET_DESC COMMND */
		ret |= SUPP_SETDESC;
	} else if (*buffer == 'i') { /*interrupt disable or enable*/
		if (!irq_i) {
			disable_irq(irq_num);
			irq_i = 1;
		} else {
			enable_irq(irq_num);
			irq_i = 0;
		}
	} else if (*buffer == 'n') { /*ping pong buffer not auto switch*/
		udc_write(udc_read(UDEPBPPC) | SOFT_DISC, UDEPBPPC);
	} else if (*buffer == 'o') { /*ping pong buffer auto switch*/
		udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
		udc_write(udc_read(UDEPBPPC) & 0XFE, UDEPBPPC);
	} else {
		ret &= ~SUPP_SETDESC;
	}

	udc_write(ret, UDLCSET);

	return count;
}

static DEVICE_ATTR(udc_ctrl, S_IWUSR | S_IRUSR, show_udc_ctrl, store_udc_ctrl);

static inline struct sp_ep *to_sp_ep(struct usb_ep *ep)
{
	return container_of(ep, struct sp_ep, ep);
}

static inline struct sp_udc *to_sp_udc(struct usb_gadget *gadget)
{
	return container_of(gadget, struct sp_udc, gadget);
}

static inline struct sp_request *to_sp_req(struct usb_request *req)
{
	return container_of(req, struct sp_request, req);
}

static inline u32 udc_read(u32 reg)
{
	return readl(base_addr + reg);
}

static inline void udc_write(u32 value, u32 reg)
{
	writel(value, base_addr + reg);
}

void init_ep_spin(void)
{
	int i;

	for (i = 0; i < SP_MAXENDPOINTS; i++) {
		spin_lock_init(&memory.ep[i].lock);
	}
}

static inline int sp_udc_get_ep_fifo_count(int is_pingbuf, u32 ping_c)
{
	int tmp = 0;

	if (is_pingbuf) {
		tmp = udc_read(ping_c);
	} else {
		tmp = udc_read(ping_c + 0x4);
	}

	return (tmp & 0x3ff);
}

static void sp_udc_done(struct sp_ep *ep, struct sp_request *req,
			    int status)
{
	//DEBUG_DBG(">>> %s...\n", __FUNCTION__);

	unsigned halted = ep->halted;

	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	ep->halted = 1;
	usb_gadget_giveback_request(&ep->ep, &req->req);
	ep->halted = halted;

	//DEBUG_DBG("<<< %s...\n", __FUNCTION__);
}

static void sp_udc_nuke(struct sp_udc *udc, struct sp_ep *ep,
			    int status)
{
	/* Sanity check */
	DEBUG_DBG(">>> sp_udc_nuke...\n");
	if (&ep->queue == NULL)
		return;

	while (!list_empty(&ep->queue)) {
		struct sp_request *req;
		req = list_entry(ep->queue.next, struct sp_request, queue);
		sp_udc_done(ep, req, status);
	}

	DEBUG_DBG("<<< sp_udc_nuke...\n");
}

static int sp_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct sp_ep *ep = to_sp_ep(_ep);
	u32 idx;
	u32 v = 0;
	unsigned long flags;

	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		printk(KERN_ERR "%s: inval 2\n", __func__);
		return -EINVAL;
	}

	DEBUG_DBG(">>> sp_udc_set_halt\n");

	local_irq_save(flags);

	idx = ep->bEndpointAddress & 0x7F;
	DEBUG_DBG("udc set halt ep=%x val=%x \n", idx, value);

	switch (idx) {
		case EP1:
			v = SETEP1STL;
			break;

		case EP3:
			v = SETEP3STL;
			break;

		case EP11:
			v = SETEPBSTL;
			break;

		default:
			return -EINVAL;
	}

	if ((!value) && v)
		v = v << 16;

	udc_write((udc_read(UDLCSTL) | v), UDLCSTL);
	DEBUG_DBG("udc set halt v=%xh, UDLCSTL=%x \n", v, udc_read(UDLCSTL));
	ep->halted = value ? 1 : 0;

	local_irq_restore(flags);
	DEBUG_DBG("<<< sp_udc_set_halt\n");

	return 0;
}

static int sp_udc_ep_enable(struct usb_ep *_ep,
				const struct usb_endpoint_descriptor *desc)
{
	struct sp_udc *dev;
	struct sp_ep *ep;
	unsigned long flags;
	u32 max;
	u32 linker_int_en;
	u32 bulk_int_en;

	DEBUG_INFO(">>> %s\n", __FUNCTION__);

	ep = to_sp_ep(_ep);

	if (!_ep || !desc || _ep->name == ep0name || desc->bDescriptorType != USB_DT_ENDPOINT) {
		printk(KERN_ERR "%s.%d,,%p,%p,%p,%s,%d\n",__FUNCTION__,__LINE__,
			_ep,desc,ep->desc,_ep->name,desc->bDescriptorType);

		return -EINVAL;
	}

	dev = ep->dev;
	DEBUG_INFO("dev->driver = %xh, dev->gadget.speed = %d\n", dev->driver, dev->gadget.speed);
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	local_irq_save(flags);

	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	linker_int_en   = udc_read(UDLIE);
	bulk_int_en     = udc_read(UDNBIE);

	// udc_write(udc_read(UDEPBPPC) & 0xfe, UDEPBPPC);
	DEBUG_INFO("ep->num = %d\n", ep->num);

	switch (ep->num) {
		case EP0:
			linker_int_en |= EP0S_IF;
			break;

		case EP1:
			linker_int_en |= EP1I_IF;
			udc_write(EP_DIR | EP_ENA | RESET_PIPO_FIFO, UDEP12C);
			break;

		case EP3:
			linker_int_en |= EP3I_IF;
			break;

		case EP11:
			bulk_int_en |= EP11O_IF;
			udc_write(EP_ENA | RESET_PIPO_FIFO, UDEPABC);
			break;

		default:
			return -EINVAL;
	}

	udc_write(linker_int_en, UDLIE);
	udc_write(bulk_int_en, UDNBIE);
	
	/* print some debug message */
	DEBUG_INFO("enable %s(%d) ep%x%s-blk max %02x\n", _ep->name, ep->num,
		     desc->bEndpointAddress, desc->bEndpointAddress & USB_DIR_IN ? "in" : "out",
		     max);

	local_irq_restore(flags);
	sp_udc_set_halt(_ep, 0);
	DEBUG_INFO("<<< %s\n", __FUNCTION__);

	return 0;
}

static int sp_udc_ep_disable(struct usb_ep *_ep)
{
	struct sp_ep *ep = to_sp_ep(_ep);
	u32 int_udll_ie;
	u32 int_udn_ie;
	unsigned long flags;

	DEBUG_DBG(">>> sp_udc_ep_disable...\n");

	if (!_ep || !ep->desc) {
		DEBUG_DBG("%s not enabled\n", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	DEBUG_DBG("ep_disable: %s\n", _ep->name);

	local_irq_save(flags);

	ep->desc = NULL;
	ep->halted = 1;

	sp_udc_nuke(ep->dev, ep, -ESHUTDOWN);

	/* disable irqs */
	int_udll_ie = udc_read(UDLIE);
	int_udn_ie = udc_read(UDNBIE);
	switch (ep->num) {
		case EP0:
			int_udll_ie &= ~(EP0I_IF | EP0O_IF | EP0S_IF);
			break;

		case EP1:
			int_udll_ie &= ~(EP1I_IF);

			if (udc_read(UEP12DMACS) & DMA_EN) {
				dma_len_ep1 = 0;
				udc_write(udc_read(UEP12DMACS) | DMA_FLUSH, UEP12DMACS);

				while (!(udc_read(UEP12DMACS) & DMA_FLUSHEND)) {
					DEBUG_DBG("wait dma 1 flush\n");
				}
			}

			dma_len_ep1 = 0;
			break;

		case EP3:
			int_udll_ie &= ~(EP3I_IF);
			break;

		case EP11:
			int_udn_ie &= ~(EP11O_IF);

			if (udc_read(UDEPBDMACS) & DMA_EN) {
				dma_xferlen_ep11 = 0;
				udc_write(udc_read(UDEPBDMACS) | DMA_FLUSH, UDEPBDMACS);

				while (!(udc_read(UDEPBDMACS) & DMA_FLUSHEND)) {
					DEBUG_DBG("wait dma 11 flush\n");
				}
			}

			dma_xferlen_ep11 = 0;
			break;

		default:
			return -EINVAL;
	}

	udc_write(int_udll_ie, UDLIE);
	udc_write(int_udn_ie, UDNBIE);

	local_irq_restore(flags);

	DEBUG_DBG("%s disabled\n", _ep->name);
	DEBUG_DBG("<<< sp_udc_ep_disable...\n");

	return 0;
}

static struct usb_request *sp_udc_alloc_request(struct usb_ep *_ep,
						    gfp_t mem_flags)
{
	struct sp_request *req;
	DEBUG_INFO(">>> sp_udc_alloc_request...\n");

	if (!_ep)
		return NULL;

	req = kzalloc(sizeof(struct sp_request), mem_flags);
	if (!req)
		return NULL;

	req->req.dma = DMA_ADDR_INVALID;
	INIT_LIST_HEAD(&req->queue);
	DEBUG_INFO("<<< sp_udc_alloc_request...\n");

	return &req->req;
}

static void sp_udc_free_request(struct usb_ep *_ep,
				struct usb_request *_req)
{
	struct sp_ep *ep = to_sp_ep(_ep);
	struct sp_request *req = to_sp_req(_req);

	DEBUG_DBG("%s free rquest\n", _ep->name);

	if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
		return;

	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

static void clearHwState_UDC(void)
{
	/*INFO: we don't disable udc interrupt when we are clear udc hw state,
	 * 1.since when we are clearing, we are in ISR , will not the same interrupt reentry problem.
	 * 2.after we finish clearing , we will go into udc ISR again, if there are interrupts occur while we are clearing ,we want to catch them
	 *  immediately
	 */
	/*===== check udc DMA state, and flush it =======*/
	int tmp = 0;
	if (udc_read(UDEP2DMACS) & DMA_EN) {
		udc_write(udc_read(UDEP2DMACS) | DMA_FLUSH, UDEP2DMACS);
		while (!(udc_read(UDEP2DMACS) & DMA_FLUSHEND)) {
			tmp++;
			if (tmp > DMA_FLUSH_TIMEOUT_DELAY) {
				DEBUG_DBG("##");
				tmp = 0;
			}
		}
	}

	/*Disable Interrupt */
	/*Clear linker layer Interrupt source */
	udc_write(0xefffffff, UDLIF);
	/*EP0 control status*/
	udc_write(CLR_EP0_OUT_VLD, UDEP0CS);	/*clear ep0 out vld=1, clear set ep0 in vld=0, set ctl dir to OUT direction=0*/
	udc_write(0x0, UDEP0CS);

	/*udc_write(CLR_EP_OVLD| RESET_PIPO_FIFO ,UDEP1SCS);*/
	udc_write(CLR_EP_OVLD | RESET_PIPO_FIFO, UDEP12C);

	/*Clear Stall Status */
	udc_write((CLREPBSTL | CLREPASTL | CLREP9STL | CLREP8STL | CLREP3STL |
		   CLREP2STL | CLREP1STL | CLREP0STL), UDLCSTL);
}

static int vbusInt_handle(void)
{
	DEBUG_DBG(">>> vbusInt_handle... UDCCS=%xh, UDLCSET=%xh\n", udc_read(UDCCS), udc_read(UDLCSET));

	/*host present */
	if (udc_read(UDCCS) & VBUS) {
		/*IF soft discconect ->force connect */
		if (udc_read(UDLCSET) & SOFT_DISC)
			udc_write(udc_read(UDLCSET) & 0xFE, UDLCSET);
	} else {		/*host absent */
		/*soft connect ->force disconnect */
		if (!(udc_read(UDLCSET) & SOFT_DISC))
			udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
		// clearHwState_UDC();
	}

	DEBUG_DBG("<<< vbusInt_handle...\n");
	return 0;
}

static int sp_udc_readep0_fifo_crq(struct usb_ctrlrequest *crq)
{
	unsigned char *outbuf = (unsigned char *)crq;

	DEBUG_DBG("read ep0 fifi crq ,len= %d\n", udc_read(UDEP0DC));
	memcpy((unsigned char *)outbuf, (char *)(UDEP0SDP + base_addr), 4);
	mb();
	memcpy((unsigned char *)(outbuf + 4), (char *)(UDEP0SDP + base_addr), 4);

	return 8;
}

static int sp_udc_get_status(struct sp_udc *dev,
				 struct usb_ctrlrequest *crq)
{
	u32 status = 0;
	u8 ep_num = crq->wIndex & 0x7F;
	struct sp_ep *ep = &memory.ep[ep_num];

	switch (crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			break;

		case USB_RECIP_DEVICE:
			status = dev->devstatus;
			break;

		case USB_RECIP_ENDPOINT:
			if (ep_num > 14 || crq->wLength > 2)
				return 1;

			status = ep->halted;
			break;

		default:
			return 1;
	}

	udc_write(EP0_DIR | CLR_EP0_OUT_VLD, UDEP0CS);
	udc_write(((1 << 2) - 1), UDEP0VB);
	memcpy((char *)(base_addr + UDEP0DP), (char *)(&status), 4);
	udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
	udc_write(EP0_DIR | SET_EP0_IN_VLD, UDEP0CS);

	return 0;
}

static void sp_udc_handle_ep0_idle(struct sp_udc *dev,
					struct sp_ep *ep,
					struct usb_ctrlrequest *crq, u32 ep0csr, int cf)
{
	int len;
	int ret, state;

	DEBUG_DBG(">>> sp_udc_handle_ep0_idle...\n");
	/* start control request? */
	sp_udc_nuke(dev, ep, -EPROTO);

	len = sp_udc_readep0_fifo_crq(crq);
	DEBUG_DBG("len  = %d\n", len);

	if (len != sizeof(*crq)) {
		printk(KERN_ERR "setup begin: fifo READ ERROR"
			  " wanted %d bytes got %d. Stalling out...\n",
			  sizeof(*crq), len);

		udc_write((udc_read(UDLCSTL) | SETEP0STL), UDLCSTL);	/* error send stall;*/
		return;
	}

	DEBUG_DBG("bRequestType = %02xh, bRequest = %02xh, wValue = %04xh, wIndex = %04xh, wLength = %04xh\n",
			crq->bRequestType, crq->bRequest, crq->wValue, crq->wIndex, crq->wLength);

	/***********************************
	* bRequestType
	Bit 7 : 資料傳輸方向
	0b = Host-to-device
	1b = Device-to-host
	Bit 6 ...
	Bit 5 : type
	00b = Standard
	01b = Class
	10b = Vendor
	11b = Reserved
	Bit 4 ...
	Bit 0 : recipient
	00000b = Device
	00001b = Interface
	00010b = Endpoint
	00011b = Other
	00100b to 11111b = Reserved
	************************************/

	/***********************************
	* wValue, high-byte
	1 = Device
	2 = Configuration
	3 = String
	4 = Interface
	5 = Endpoint
	6 = Device Qualifier
	7 = Other Speed Configuration
	8 - Interface Power
	9 - On-The-Go (OTG)
	21 = HID (Human interface device)
	22 = Report
	23 = Physical
	************************************/

	/* cope with automagic for some standard requests. */
	dev->req_std = (crq->bRequestType & USB_TYPE_MASK)== USB_TYPE_STANDARD;
	dev->req_config = 0;
	dev->req_pending = 1;
	state = dev->gadget.state;

	switch (crq->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			DEBUG_DBG(" ******* USB_REQ_GET_DESCRIPTOR ... \n");
			DEBUG_DBG("start get descriptor after bus reset\n");
			{
				u32 DescType = ((crq->wValue) >> 8);
				if (DescType == 0x1) {
					if (udc_read(UDLCSET) & CURR_SPEED) {
						DEBUG_INFO("DESCRIPTOR SPeed = USB_SPEED_FULL\n");
						dev->gadget.speed = USB_SPEED_FULL;
						bulkep_dma_block_size = FULL_SPEED_DMA_SIZE;
					} else {
						DEBUG_INFO("DESCRIPTOR SPeed = USB_SPEED_HIGH\n");
						dev->gadget.speed = USB_SPEED_HIGH;
						bulkep_dma_block_size = HIGH_SPEED_DMA_SIZE;
					}
				}
			}
			break;

		case USB_REQ_SET_CONFIGURATION:
			DEBUG_DBG(" ******* USB_REQ_SET_CONFIGURATION ... \n");
			DEBUG_DBG("dev->gadget.state = %d \n", dev->gadget.state);
			dev->req_config = 1;
			udc_write(udc_read(UDLCADDR) | (crq->wValue) << 16, UDLCADDR);
			break;

		case USB_REQ_SET_INTERFACE:
			DEBUG_DBG("***** USB_REQ_SET_INTERFACE **** \n");
			dev->req_config = 1;
			break;

		case USB_REQ_SET_ADDRESS:
			DEBUG_DBG("USB_REQ_SET_ADDRESS ... \n");
			return;

		case USB_REQ_GET_STATUS:
			DEBUG_DBG("USB_REQ_GET_STATUS ... \n");

#ifdef CONFIG_USB_SUNPLUS_OTG
			if((0x80 == crq->bRequestType) && (0 == crq->wValue) && (0xf000 == crq->wIndex) && (4 == crq->wLength)){
				u32 status = 0;
				status = dev_otg_status;
				udc_write(EP0_DIR | CLR_EP0_OUT_VLD, UDEP0CS);
				udc_write(((1 << 2) - 1), UDEP0VB);
				DEBUG_DBG("get otg status,%d,%d\n",dev_otg_status,status);
				memcpy((char *)(base_addr + UDEP0DP), (char *)(&status), 4);
				udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
				udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
				return;
			}
#endif

			if (dev->req_std) {
				if (!sp_udc_get_status(dev, crq)) {
					return;
				}
			}
			break;

		case USB_REQ_CLEAR_FEATURE:
			DEBUG_DBG(">>> USB_REQ_CLEAR_FEATURE ... \n");
			break;

		case USB_REQ_SET_FEATURE:
			DEBUG_DBG("USB_REQ_SET_FEATURE ... \n");
			break;

		default:
			break;
	}

	if (crq->bRequestType & USB_DIR_IN) {
		dev->ep0state = EP0_IN_DATA_PHASE;
	} else {
		dev->ep0state = EP0_OUT_DATA_PHASE;
		/*udc_write(CLR_EP0_OUT_VLD, UDEP0CS);*/
		// DEBUG_NOTICE("ep0 fifo %x\n", udc_read(UDEP0CS));
		udc_write(0, UDEP0CS);
	}

	if (!dev->driver)
		return;

	/* deliver the request to the gadget driver */
	ret = dev->driver->setup(&dev->gadget, crq);	/*android_setup composite_setup*/
	DEBUG_DBG("dev->driver->setup = %x\n", ret);

	if (ret < 0) {
		if (dev->req_config) {
			printk(KERN_ERR "config change %02x fail %d?\n",
				  crq->bRequest, ret);
			return;
		}

		if (ret == -EOPNOTSUPP)
			printk(KERN_ERR "Operation not supported\n");
		else
			printk(KERN_ERR "dev->driver->setup failed. (%d)\n", ret);

		udelay(5);
		udc_write(0x1, UDLCSTL);	/*set ep0 stall*/
		dev->ep0state = EP0_IDLE;
	} else if (dev->req_pending) {
		DEBUG_DBG("dev->req_pending... what now?\n");
		dev->req_pending = 0;
		/*MSC reset command */
		if (crq->bRequest == 0xff) {
			/*ep1SetHalt = 0;
			   ep2SetHalt = 0;*/
		}
	}

	DEBUG_DBG("ep0state *** %s, Request = %d, RequestType = %d, from = %d\n", ep0states[dev->ep0state], crq->bRequest, crq->bRequestType, cf);
	DEBUG_DBG("<<< sp_udc_handle_ep0_idle...\n");
}

static inline int sp_udc_write_packet_with_buf(int fifo, u8 *buf,
					  unsigned len, int reg)
{
	int n = 0;
	int m = 0;
	int i = 0;

	n = len / 4;
	m = len % 4;

	if (n > 0) {
		udc_write(0xf, reg);
		for (i = 0; i < n; i++) {
			*(u32 *) (base_addr + fifo) = *((u32 *) (buf + (i * 4)));
		}
	}

	if (m > 0) {
		udc_write(((1 << m) - 1), reg);
		*(u32 *) (base_addr + fifo) = *((u32 *) (buf + (i * 4)));
	}

	return len;
}

static inline int sp_udc_write_packet(int fifo, struct sp_request *req,
					  unsigned max, int offset)
{
	unsigned len = min(req->req.length - req->req.actual, max);

	u8 *buf = req->req.buf + req->req.actual;

	return sp_udc_write_packet_with_buf(fifo, buf, len, offset);
}

static int sp_udc_write_ep0_fifo(struct sp_ep *ep,
				     struct sp_request *req)
{
	unsigned count;
	int is_last;

	udc_write(EP0_DIR | CLR_EP0_OUT_VLD, UDEP0CS);
	count = sp_udc_write_packet(UDEP0DP, req, ep->ep.maxpacket, UDEP0VB);
	udc_write(EP0_DIR | SET_EP0_IN_VLD, UDEP0CS);

	req->req.actual += count;
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length != req->req.actual || req->req.zero)
		is_last = 0;
	else
		is_last = 1;

	// DEBUG_INFO("write ep0: count=%d, actual=%d, length=%d, last=%d, zero=%d\n", count, req->req.actual, req->req.length, is_last, req->req.zero);

	if (is_last) {
		int cc = 0;

		while(udc_read(UDEP0CS)&EP0_IVLD) {
			udelay(5);
			if (cc++>1000) break;
		}

		sp_udc_done(ep, req, 0);
		udc_write(udc_read(UDLIE) & (~EP0I_IF), UDLIE);
		udc_write(udc_read(UDEP0CS) & (~EP0_DIR), UDEP0CS);
	}
	else {
		udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
	}

	return is_last;
}

static inline int sp_udc_read_fifo_packet(int fifo, u8 * buf, int length,
					      int reg)
{
	int n = 0;
	int m = 0;
	int i = 0;

	n = length / 4;
	m = length % 4;

	udc_write(0xf, reg);

	for (i = 0; i < n; i++) {
		*((u32 *) (buf + (i * 4))) = *(u32 *) (base_addr + fifo);
	}

	if (m > 0) {
		udc_write(((1 << m) - 1), reg);
		*((u32 *) (buf + (i * 4))) = *(u32 *) (base_addr + fifo);
	}

	return length;
}

static int sp_udc_read_ep0_fifo(struct sp_ep *ep,
				struct sp_request *req)
{
	u8 *buf;
	unsigned count;
	u8 ep0_len;
	int is_last;

	if (!req->req.length) {
		DEBUG_DBG("sp_udc_read_ep0_fifo: length = 0");
		// udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
		// udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
		// udc_write(udc_read(UDLIE) & (~EP0O_IF), UDLIE);
		// sp_udc_done(ep, req, 0);
		is_last = 1;
	}
	else {
		udc_write(udc_read(UDLIE) | EP0O_IF, UDLIE);

		buf = req->req.buf + req->req.actual;
		udc_write(udc_read(UDEP0CS) & (~EP0_DIR), UDEP0CS);	/*read direction*/

		ep0_len = udc_read(UDEP0DC);

		if (ep0_len > ep->ep.maxpacket)
			ep0_len = ep->ep.maxpacket;

		count = sp_udc_read_fifo_packet(UDEP0DP, buf, ep0_len, UDEP0VB);
		udc_write(udc_read(UDEP0CS) | CLR_EP0_OUT_VLD, UDEP0CS);

		req->req.actual += count;

		if (count != ep->ep.maxpacket)
			is_last = 1;
		else if (req->req.length != req->req.actual || req->req.zero)
			is_last = 0;
		else
			is_last = 1;

		DEBUG_DBG("read ep0: count=%d, actual=%d, length=%d, maxpacket=%d, last=%d, zero=%d\n", count, req->req.actual, req->req.length, ep->ep.maxpacket, is_last, req->req.zero);
	}
	if (is_last) {
		DEBUG_DBG("sp_udc_read_ep0_fifo: is_last = 1");
		udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
		udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
		udc_write(udc_read(UDLIE) & (~EP0O_IF), UDLIE);
		sp_udc_done(ep, req, 0);
	}

	return is_last;
}

static int sp_udc_handle_ep0_proc(struct sp_ep *ep, struct sp_request *req, int cf)
{
	struct usb_ctrlrequest crq;
	u32 ep0csr;
	struct sp_udc *dev = ep->dev;
	int bOk = 1;
	
	ep0csr = udc_read(UDEP0CS);

	switch (dev->ep0state) {
		case EP0_IDLE:
			DEBUG_DBG("EP0_IDLE_PHASE ... what now?\n");
			sp_udc_handle_ep0_idle(dev, ep, &crq, ep0csr, cf);
			break;

		case EP0_IN_DATA_PHASE:
			DEBUG_DBG("EP0_IN_DATA_PHASE ... what now?\n");
			if(sp_udc_write_ep0_fifo(ep, req)){
				ep->dev->ep0state = EP0_IDLE;
				DEBUG_DBG("ep0 in0 done\n");
			} else bOk = 0;
			break;

		case EP0_OUT_DATA_PHASE:
			DEBUG_DBG("EP0_OUT_DATA_PHASE *** what now?\n");
			if(sp_udc_read_ep0_fifo(ep, req)){
				ep->dev->ep0state = EP0_IDLE;
				DEBUG_DBG("ep0 out1 done\n");
			}
			else bOk = 0;
			break;
	}

	return bOk;
}

static void sp_udc_handle_ep0(struct sp_udc *dev)
{
	struct sp_ep *ep = &dev->ep[0];
	struct usb_composite_dev *cdev = get_gadget_data(&dev->gadget);
	struct usb_request *req_g = NULL;
	struct sp_request *req = NULL;

	DEBUG_DBG(">>> sp_udc_handle_ep0 ...\n");

	if (!cdev) {
		printk(KERN_ERR "cdev invalid\n");
		return;
	}

	req_g = cdev->req;
	req = to_sp_req(req_g);

	if (!req) {
		printk(KERN_ERR "req invalid\n");
		return;
	}

	sp_udc_handle_ep0_proc(ep, req, 1);
	DEBUG_DBG("<<< sp_udc_handle_ep0 \n");
}

static void sp_print_hex_dump_bytes(const char *prefix_str, int prefix_type,
			  const void *buf, size_t len)
{
	if (dmsg & (1 << 3))
		print_hex_dump(KERN_NOTICE, prefix_str, prefix_type, 16, 1,
				buf, len, true);
}

#ifdef USE_DMA
static int sp_udc_ep11_bulkout_pio(struct sp_ep *ep,
					struct sp_request *req);

static int sp_udc_ep11_bulkout_dma(struct sp_ep *ep,
					struct sp_request *req)
{
	u8 *buf;
	int actual_length = 0;
	int cur_length = req->req.length;
	int dma_xferlen;
	unsigned long t;
	int ret = 0;

	DEBUG_INFO(">>> %s...\n", __FUNCTION__);

	if (dma_xferlen_ep11 == 0) {
		if (cur_length <= bulkep_dma_block_size) {
			ret = sp_udc_ep11_bulkout_pio(ep, req);
			return ret;
		}

		udc_write(udc_read(UDNBIE) & (~EP11O_IF), UDNBIE);

		DEBUG_INFO("req.length=%d req.actual=%d, req->req.dma = %xh UDCIF = %xh\n",
				req->req.length, req->req.actual, req->req.dma, udc_read(UDCIF));

		if (req->req.dma == DMA_ADDR_INVALID) {
			req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, (u8 *)req->req.buf, cur_length, DMA_FROM_DEVICE);
			if (dma_mapping_error(ep->dev->gadget.dev.parent, req->req.dma)) {
				DEBUG_INFO("dma_mapping_error");
				return 1;
			} 
		}

		while (actual_length < cur_length) {
			buf = (u8 *) (req->req.dma + actual_length);
			dma_xferlen = min(cur_length - actual_length, (int)UDC_FLASH_BUFFER_SIZE);
			dma_xferlen_ep11 = dma_xferlen;

			udc_write((u32) buf, UDEPBDMADA);
			udc_write((udc_read(UDEPBDMACS) & (~DMA_COUNT_MASK)) |
				  DMA_COUNT_ALIGN | DMA_WRITE | dma_xferlen | DMA_EN,
				  UDEPBDMACS);

			/*verB DMA bug*/
			if ((udc_read(UDEPBFS) & 0x22) == 0x20)
				udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);

			DEBUG_INFO("cur_len=%d actual_len=%d req.dma=%xh dma_len=%d UDEPBDMACS = %xh UDEPBFS = %xh UDCIF = %xh\n", cur_length, actual_length, req->req.dma, dma_xferlen, udc_read(UDEPBDMACS), udc_read(UDEPBFS), udc_read(UDCIF));

			t = jiffies;

			// if (((udc_read(UDEPBDMACS) & DMA_EN)
			// 	       || (!(udc_read(UDCIF) & EPB_DMA_IF))
			// 	       || (udc_read(UDEPBDMACS) & 0x3fffff))) {
			// 	udc_write(udc_read(UDCIE) | EPB_DMA_IF, UDCIE);
			// 	return 0;
			// }

			while ((udc_read(UDEPBDMACS) & DMA_EN) != 0) {
				udc_write(udc_read(UDCIE) | EPB_DMA_IF, UDCIE);
				DEBUG_INFO(">>cur_len=%d actual_len=%d req.dma=%xh dma_len=%d UDEPBDMACS = %xh UDCIE = %xh UDCIF = %xh\n",
						cur_length, actual_length, req->req.dma, dma_xferlen, udc_read(UDEPBDMACS), udc_read(UDCIE), udc_read(UDCIF));

				if (time_after(jiffies, t + 10 * HZ)) {
					DEBUG_INFO("dma error: UDEPBDMACS = %xh\n", udc_read(UDEPBDMACS));
					break;
				}
			}

			DEBUG_INFO("UDCIF = %xh\n", udc_read(UDCIF));

			actual_length += dma_xferlen;
		}

		cur_length -= actual_length;

		if (req->req.dma != DMA_ADDR_INVALID) {
			dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma,
						req->req.length, DMA_FROM_DEVICE);

			req->req.dma = DMA_ADDR_INVALID;
		}

		udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);

		DEBUG_INFO("UDEPBDMACS = %x\n", udc_read(UDEPBDMACS));
		DEBUG_INFO("<<< %s...\n", __FUNCTION__);

		return 1;
	} else {
		DEBUG_INFO("<<< %s...\n", __FUNCTION__);

		return ret;
	}
}
#endif

static void sp_udc_bulkout_pio(u8 *buf, u32 avail)
{
	sp_udc_read_fifo_packet(UDEPBFDP, buf, avail, UDEPBVB);
	udc_write(udc_read(UDEPABC) | CLR_EP_OVLD, UDEPABC);
}

static int sp_udc_ep11_bulkout_pio(struct sp_ep *ep,
				       struct sp_request *req)
{
	u8 *buf;
	u32 count;
	u32 avail;
	int is_last ;
	int is_pingbuf;
	int pre_is_pingbuf;
	int delay_count;

	DEBUG_DBG(">>> %s UDEPBFS = %xh\n", __FUNCTION__, udc_read(UDEPBFS));

	if (down_trylock(&ep->wsem)) {
		return 0;
	}
	DEBUG_DBG("1.req.length=%d req.actual=%d req->req.dma = %xh UDEPBFS = %xh \n", req->req.length, req->req.actual, req->req.dma, udc_read(UDEPBFS));

	is_last = 0;
	delay_count = 0;
	is_pingbuf = (udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0;

	do {
		pre_is_pingbuf = is_pingbuf;
		count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEPBPIC);
		if (!count) {
			// udelay(2);
			// if (delay_count++<20) continue;
			up(&ep->wsem);

			return 1;
		}

		buf = req->req.buf + req->req.actual;

		if (count > ep->ep.maxpacket)
			avail = ep->ep.maxpacket;
		else
			avail = count;
		
		sp_udc_bulkout_pio(buf, avail);

		// if (req->req.length/bulkep_dma_block_size)
		// 	sp_udc_ep11_bulkout_dma(ep, req);

		req->req.actual += avail;

		if (count < ep->ep.maxpacket || req->req.length <= req->req.actual) {
			is_last = 1;
		}

		DEBUG_DBG("2.req.length=%d req.actual=%d UDEPBFS = %xh UDEPBPPC = %xh UDEPBPOC = %xh  UDEPBPIC = %xh avail=%d is_last=%d\n",
				req->req.length, req->req.actual, udc_read(UDEPBFS), udc_read(UDEPBPPC), udc_read(UDEPBPOC), udc_read(UDEPBPIC), avail, is_last);

		if (is_last) {
			break;
		}

out_fifo_retry:
		if ((udc_read(UDEPBFS)&0x22) == 0) {
			udelay(1);
			if (delay_count++ < 20) goto out_fifo_retry;
			delay_count = 0;
		}

out_fifo_controllable:
		is_pingbuf = (udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0;

		if (is_pingbuf == pre_is_pingbuf)
			goto out_fifo_controllable;
	} while(1);

	DEBUG_DBG("3.req.length=%d req.actual=%d UDEPBFS = %xh count=%d is_last=%d\n",
			req->req.length, req->req.actual, udc_read(UDEPBFS), count, is_last);

	if (req->req.actual >= 320)
		sp_print_hex_dump_bytes("30->20-", DUMP_PREFIX_OFFSET, req->req.buf, 320);
	else
		sp_print_hex_dump_bytes("30->20-", DUMP_PREFIX_OFFSET, req->req.buf, req->req.actual);

	sp_udc_done(ep, req, 0);
	up(&ep->wsem);
	DEBUG_DBG("<<< %s\n", __FUNCTION__);

	return is_last;
}

static int sp_udc_ep1_bulkin_pio(struct sp_ep *ep, struct sp_request *req)
{
	u32 count, w_count;
	int is_pingbuf;
	int is_last;
	int delay_count;
	int pre_is_pingbuf;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);

	if (down_trylock(&ep->wsem)) {
		return 0;
	}

	is_last = 0;
	delay_count = 0;

	DEBUG_DBG("1.req.actual = %d req.length=%d req->req.dma=%xh UDEP12FS = %xh\n",
		              req->req.actual, req->req.length, req->req.dma, udc_read(UDEP12FS));

	is_pingbuf = (udc_read(UDEP12PPC) & CURR_BUFF) ? 1 : 0;

	while(1) {
		pre_is_pingbuf = is_pingbuf;
		count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEP12PIC);

		w_count = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
		udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
		req->req.actual += w_count;

		if (w_count != ep->ep.maxpacket)
			is_last = 1;
		else if (req->req.length == req->req.actual && !req->req.zero)
			is_last = 1;
		else
			is_last = 0;

		DEBUG_DBG("2.req.length=%d req.actual=%d UDEP12FS = %xh UDEP12PPC = %xh UDEP12POC = %xh  UDEP12PIC = %xh count=%d  w_count = %d is_last=%d\n",
			       req->req.length, req->req.actual, udc_read(UDEP12FS), udc_read(UDEP12PPC), udc_read(UDEP12POC), udc_read(UDEP12PIC), count, w_count, is_last);

		if (is_last) break;

in_fifo_controllable:
		is_pingbuf = (udc_read(UDEP12PPC) & CURR_BUFF) ? 1 : 0;

		if (is_pingbuf == pre_is_pingbuf)
			goto in_fifo_controllable;
	}
	sp_udc_done(ep, req, 0);

	up(&ep->wsem);
	DEBUG_DBG("3.req.actual = %d, count = %d, req.length=%d, UDEP12FS = %xh, is_last = %d\n",
			req->req.actual, count, req->req.length, udc_read(UDEP12FS), is_last);

	if (req->req.actual >= 142)
		sp_print_hex_dump_bytes("20->30-", DUMP_PREFIX_OFFSET, req->req.buf, 142);
	else
		sp_print_hex_dump_bytes("20->30-", DUMP_PREFIX_OFFSET, req->req.buf, req->req.actual);

	DEBUG_DBG("<<< %s...\n", __FUNCTION__);

	return is_last;
}

#ifdef USE_DMA
static int sp_ep1_bulkin_dma(struct sp_ep *ep, struct sp_request *req)
{
	u8 *buf;
	int dma_xferlen;
	int actual_length = 0;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);

	if (req->req.dma == DMA_ADDR_INVALID) {
		req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, req->req.buf, dma_len_ep1,
			(ep->bEndpointAddress & USB_DIR_IN) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}

	buf = (u8 *) (req->req.dma + req->req.actual + actual_length);
	dma_xferlen = min(dma_len_ep1 - req->req.actual - actual_length, (unsigned)UDC_FLASH_BUFFER_SIZE);
	DEBUG_DBG("%p dma_xferlen = %d %d\n", buf, dma_xferlen, dma_len_ep1);
	actual_length = dma_xferlen;

	if (dma_xferlen > 4096)
		DEBUG_NOTICE("dma in len err %d\n", dma_xferlen);

	/*udc_write(dma_xferlen, UEP12DMACS);*/
	udc_write(dma_xferlen | DMA_COUNT_ALIGN, UEP12DMACS);
	udc_write((u32) buf, UEP12DMADA);
	udc_write(udc_read(UEP12DMACS) | DMA_EN, UEP12DMACS);

	if (udc_read(UEP12DMACS) & DMA_EN) {
		udc_write(udc_read(UDLIE) | EP1_DMA_IF, UDLIE);
		return 0;
	}

	udc_write(EP1_DMA_IF, UDLIF);

	req->req.actual += actual_length;
	DEBUG_DBG("req->req.actual = %d UEP12DMACS = %xh\n", req->req.actual,
		  udc_read(UEP12DMACS));

	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma,
				 dma_len_ep1,
				 (ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}

	dma_len_ep1 = 0;
	DEBUG_DBG("<<< %s\n", __FUNCTION__);

	return 1;
}

static int sp_udc_ep1_bulkin_dma(struct sp_ep *ep,
				 struct sp_request *req)
{
	u32 count;
	int is_last = 0;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);

	if ((req->req.actual) || (req->req.length == 0))
		goto _TX_BULK_IN_DATA;

	/* DMA Mode */
	dma_len_ep1 = req->req.length - (req->req.length % bulkep_dma_block_size);

	if (dma_len_ep1 == bulkep_dma_block_size) {
		dma_len_ep1 = 0;
		goto _TX_BULK_IN_DATA;
	}

	if (dma_len_ep1) {
		DEBUG_DBG("ep1 bulk in dma mode,zero=%d\n", req->req.zero);

		udc_write(udc_read(UDLIE) & (~EP1I_IF), UDLIE);

		if (!sp_ep1_bulkin_dma(ep, req))
			return 0;
		/*else if(req->req.length == req->req.actual){
		   is_last = 1;
		   goto done_dma;} */
		else if (req->req.length == req->req.actual && !req->req.zero) {
			is_last = 1;
			goto done_dma;
		} else if (udc_read(UDEP12FS) & 0x1) {
			DEBUG_DBG("ep1 dma->pio wait write!\n");
			goto done_dma;
		} else {
			//udc_write(EP1N_IF, UDLIF);
			count = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
			udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
			req->req.actual += count;
			sp_udc_done(ep, req, 0);
			DEBUG_DBG("ep1 dma->pio write!\n");
			udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);

			return 1;
		}
	}

_TX_BULK_IN_DATA:
	udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
	is_last = sp_udc_ep1_bulkin_pio(ep, req);

	return is_last;

done_dma:
	DEBUG_DBG("is_last = %d\n", is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
	}

	udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
	DEBUG_DBG("<<< %s...\n", __FUNCTION__);

	return is_last;
}
#endif

static int sp_udc_int_in(struct sp_ep *ep, struct sp_request *req)
{
	int count;

	DEBUG_DBG(">>> %s...\n", __FUNCTION__);

	count = sp_udc_write_packet(UDEP3DATA, req, ep->ep.maxpacket, UDEP3VB);
	udc_write((1 << 0) | (1 << 3), UDEP3CTRL);
	req->req.actual += count;

	DEBUG_INFO("write ep3, count=%d, actual=%d, length=%d, zero=%d\n", count, req->req.actual, req->req.length, req->req.zero);

	if (req->req.actual == req->req.length) {
		DEBUG_DBG("write ep3, sp_udc_done\n");
		sp_udc_done(ep, req, 0);
	}

	DEBUG_DBG("<<< %s...\n", __FUNCTION__);
	return 1;
}

static int sp_udc_ep1_bulkin(struct sp_ep *ep, struct sp_request *req)
{
	int ret;

#ifdef USE_DMA
	if (dma_len_ep1 == 0)
		ret = sp_udc_ep1_bulkin_dma(ep, req);
#else
	ret = sp_udc_ep1_bulkin_pio(ep, req);
#endif

	return ret;
}

static int sp_udc_ep11_bulkout(struct sp_ep *ep, struct sp_request *req)
{
	int ret = 0;

#ifdef USE_DMA
	ret = sp_udc_ep11_bulkout_dma(ep, req);
#else
	ret = sp_udc_ep11_bulkout_pio(ep, req);
#endif

	return ret;
}

static int sp_udc_handle_ep(struct sp_ep *ep, struct sp_request *req)
{
	int ret = 0;
	int idx = ep->bEndpointAddress & 0x7F;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);

	if (!req) {
		int empty = list_empty(&ep->queue);
		if (empty) {
			req = NULL;
			ret = 1;
			DEBUG_DBG("idx = %d, req is NULL\n", idx);
		} else {
			req = list_entry(ep->queue.next, struct sp_request, queue);
		}
	}

	if (req) {	
		switch (idx) {
			case EP1:
				if ((udc_read(UDEP12FS) & 0x1) == 0)
					ret = sp_udc_ep1_bulkin(ep, req);

				break;
			case EP3:
				ret = sp_udc_int_in(ep, req);

				break;
			case EP11:
				if (udc_read(UDEPBFS) & 0x22)
					ret = sp_udc_ep11_bulkout(ep, req);

				break;
		}
	}

	DEBUG_DBG("<<< %s ret = %d\n", __FUNCTION__, ret);
	return ret;
}

#ifdef USE_DMA
static void ep1_dma_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[1];
	struct sp_request *req;
	int ret;

	if (list_empty(&ep->queue)) {
		req = NULL;
		printk(KERN_ERR "ep1_dma req is NULL\t!\n");

		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);

	if (req->req.actual != 0) {
		printk(KERN_ERR "WHY ep1\n");
		if (req->req.actual != req->req.length) {
			return;
		}
	}

	req->req.actual += dma_len_ep1;
	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma, dma_len_ep1,
				(ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		req->req.dma = DMA_ADDR_INVALID;
	}

	if (req->req.length == req->req.actual && !req->req.zero) {
		sp_udc_done(ep, req, 0);
		dma_len_ep1 = 0;

		if (!(udc_read(UDEP12FS) & 0x1)){
			sp_udc_handle_ep(ep, NULL);
		}
		DEBUG_DBG("ep1 dma: %d\n", udc_read(UDEP12FS));

		return;
	}

	if (!(udc_read(UDEP12FS) & 0x1)) {
		//udc_write(EP1N_IF, UDLIF);
		ret = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
		udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
		req->req.actual += ret;
		sp_udc_done(ep, req, 0);

		DEBUG_DBG("DMA->write fifo by pio count=%d!\n", ret);
	} else {
		DEBUG_DBG("wait DMA->write fifo by pio!\n\n");
	}

	dma_len_ep1 = 0;
	udc_write(udc_read(UDLIE) | EP1I_IF , UDLIE);
}

static void ep11_dma_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[11];
	struct sp_request *req;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);

	if (list_empty(&ep->queue)) {
		req = NULL;
		DEBUG_DBG("ep11_dma req is NULL\t!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);

	//if (!req->req.actual)
	//	printk(KERN_ERR "why act=0?\n");

	req->req.actual += dma_xferlen_ep11;

	if (req->req.length == req->req.actual) {
		dma_xferlen_ep11 = 0;

		if (req->req.dma != DMA_ADDR_INVALID) {
			dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma, req->req.length,
						(ep->bEndpointAddress & USB_DIR_IN) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
			req->req.dma = DMA_ADDR_INVALID;
		}

		udc_write(udc_read(UDCIE) & (~EPB_DMA_IF), UDCIE);
		sp_udc_done(ep, req, 0);
	}

	DEBUG_DBG("<<< %s\n", __FUNCTION__);
	return;
}
#endif

static void sp_sendto_workqueue(struct sp_udc *dev, int n)
{
	spin_lock(&qlock);
	dev->wq[n].sw.ep_num = n;
	queue_work(dev->wq[n].queue, &dev->wq[n].sw.work);
	spin_unlock(&qlock);
}

static irqreturn_t sp_udc_irq(int irq, void *_dev)
{
	/*u32 udc_epnb_irq_en;*/
	u32 irq_en1_flags;
	u32 irq_en2_flags;
	unsigned long flags;

	struct sp_udc *dev = (struct sp_udc *)_dev;

	spin_lock_irqsave(&dev->lock, flags);

	irq_en1_flags = udc_read(UDCIF) & udc_read(UDCIE);
	irq_en2_flags = udc_read(UDLIE) & udc_read(UDLIF);

	if (irq_en2_flags & RESET_RELEASE_IF) {
		udc_write(RESET_RELEASE_IF, UDLIF);
		DEBUG_DBG("reset end irq\n");
	}

	if (irq_en2_flags & RESET_IF) {
		DEBUG_DBG("reset irq\n");
		/* two kind of reset :
		 * - reset start -> pwr reg = 8
		 * - reset end   -> pwr reg = 0
		 **/
		dev->gadget.speed = USB_SPEED_UNKNOWN;
		dev->address = 0;

		udc_write((udc_read(UDLCSET) | 8) & 0xFE, UDLCSET);
		udc_write(RESET_IF, UDLIF);
		/*Allow LNK to suspend PHY */
		udc_write(udc_read(UDCCS) & (~UPHY_CLK_CSS), UDCCS);
		clearHwState_UDC();

		dev->ep0state = EP0_IDLE;
		dev->gadget.speed = USB_SPEED_FULL;

		spin_unlock_irqrestore(&dev->lock, flags);

		return IRQ_HANDLED;
	}

	/* force disconnect interrupt */
	if (irq_en1_flags & VBUS_IF) {
		DEBUG_DBG("vbus_irq[%xh]\n",udc_read(UDLCSET));
		udc_write(VBUS_IF, UDCIF);
		vbusInt_handle();
	}

	if (irq_en2_flags & SUS_IF) {
		DEBUG_DBG(">>> IRQ: Suspend \n");
		DEBUG_DBG("clear  Suspend Event\n");
		udc_write(SUS_IF, UDLIF);

		if (dev->gadget.speed != USB_SPEED_UNKNOWN
				&& dev->driver
				&& dev->driver->suspend)
			dev->driver->suspend(&dev->gadget);

		dev->gadget.speed = USB_SPEED_UNKNOWN;
		dev->address = 0;

		DEBUG_DBG("<<< IRQ: Suspend \n");
	}

	if (irq_en2_flags & EP0S_IF) {
		udc_write(EP0S_IF, UDLIF);
		DEBUG_DBG("IRQ:EP0S_IF %d, dev->ep0state = %d\n", udc_read(UDEP0CS),dev->ep0state);
		if ((udc_read(UDEP0CS) & (EP0_OVLD | EP0_OUT_EMPTY)) ==
		    (EP0_OVLD | EP0_OUT_EMPTY)) {
			udc_write(udc_read(UDEP0CS) | CLR_EP0_OUT_VLD, UDEP0CS);
		}

		if (dev->ep0state == EP0_IDLE)
			sp_udc_handle_ep0(dev);
	}

	if ((irq_en2_flags & EP0I_IF)) {
		DEBUG_DBG("IRQ:EP0I_IF %d, dev->ep0state = %d\n", udc_read(UDEP0CS),dev->ep0state);
		udc_write(EP0I_IF, UDLIF);

		if (dev->ep0state != EP0_IDLE)
			sp_udc_handle_ep0(dev);
	}

	if ((irq_en2_flags & EP0O_IF)) {
		DEBUG_DBG("IRQ:EP0O_IF %d, dev->ep0state = %d\n", udc_read(UDEP0CS), dev->ep0state);
		udc_write(EP0O_IF, UDLIF);

		if (dev->ep0state != EP0_IDLE)
			sp_udc_handle_ep0(dev);
	}

#ifdef USE_DMA
	if (irq_en2_flags & EP1_DMA_IF) {	/*dma finish*/
		DEBUG_DBG("IRQ:UDC ep1 DMA\n");
		udc_write(EP1_DMA_IF, UDLIF);

		if (dma_len_ep1)
			ep1_dma_handle(dev);
	}
#endif

	if (irq_en2_flags & EP1I_IF) {
		DEBUG_DBG("IRQ:ep1 in %xh\n", udc_read(UDCIE));
		udc_write(EP1I_IF, UDLIF);
		// udc_write(udc_read(UDLIE) & (~EP1I_IF), UDLIE);
		// if ((udc_read(UDEP12FS) & 0x1)==0)
		sp_sendto_workqueue(dev, 1);
	}

	if (irq_en2_flags & EP3I_IF) {
		DEBUG_DBG("IRQ:ep3 in int\n");
		udc_write(EP3I_IF, UDLIF);
		sp_udc_handle_ep(&dev->ep[3], NULL);
	}

#ifdef USE_DMA
	if (irq_en1_flags & EPB_DMA_IF) {
		DEBUG_DBG("IRQ:UDC ep11 DMA\n");
		mdelay(1);
		udc_write(EPB_DMA_IF, UDCIF);
		ep11_dma_handle(dev);
		sp_sendto_workqueue(dev, 11);
	}
#endif

	if (udc_read(UDNBIF) & EP11O_IF) {
		udc_write(EP11O_IF, UDNBIF);
		udc_write(udc_read(UDNBIE) & (~EP11O_IF), UDNBIE);
		DEBUG_DBG("IRQ:ep11 out %xh %xh state=%d\n", udc_read(UDNBIE), udc_read(UDEPBFS), dev->gadget.state);

		// if (udc_read(UDEPBFS) & 0x22)
		sp_sendto_workqueue(dev, 11);
	}

	if (udc_read(UDNBIF) & SOF_IF) {
		udc_write(SOF_IF, UDNBIF);
		udc_write(udc_read(UDNBIE) | (SOF_IF), UDNBIE);
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	return IRQ_HANDLED;
}

static int sp_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
			    gfp_t gfp_flags)
{
	struct sp_udc *dev;
	unsigned long flags;
	int idx;
	struct sp_request *req = to_sp_req(_req);
	struct sp_ep *ep = to_sp_ep(_ep);

	DEBUG_DBG(">>> %s..\n", __FUNCTION__);

	if (unlikely(!_ep || (!ep->ep.desc && ep->ep.name != ep0name))) {
		printk(KERN_NOTICE "%s: invalid args\n", __func__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver
			|| dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		return -ESHUTDOWN;
	}

	local_irq_save(flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	idx = ep->bEndpointAddress & 0x7F;
	if (list_empty(&ep->queue) ) 
		if (idx == EP0 && sp_udc_handle_ep0_proc(ep, req, 0)) req = NULL;

	DEBUG_DBG("req = %x, ep=%d, req_config=%d \n", req, idx, dev->req_config);
	if (likely(req)) {
		list_add_tail(&req->queue, &ep->queue);
		if (idx && idx != EP3) sp_sendto_workqueue(dev, idx);
	}
	
	local_irq_restore(flags);
	DEBUG_DBG("<<< %s..\n", __FUNCTION__);

	return 0;
}

static int sp_udc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct sp_ep *ep;
	struct sp_udc *udc;
	struct sp_request *req = NULL;
	int retval = -EINVAL;

	DEBUG_DBG(">>> sp_udc_dequeue...\n");
	DEBUG_DBG("%s dequeue\n", _ep->name);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	ep = to_sp_ep(_ep);
	udc = to_sp_udc(ep->gadget);
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req) {
			list_del_init(&req->queue);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		DEBUG_DBG("dequeued req %p from %s, len %d buf %p\n", req,
			   _ep->name, _req->length, _req->buf);
		sp_udc_done(ep, req, -ECONNRESET);
	}

	DEBUG_DBG("<<< sp_udc_dequeue...retval = %d\n", retval);
	return retval;
}

/*
return	0 : disconnect	1 : connect
*/
static int sp_vbus_detect(void)
{
	return 1;
}

static void sp_udc_enable(struct sp_udc *udc)
{
	/*
	   usb device interrupt enable
	   ---force usb bus disconnect enable
	   ---force usb bus connect interrupt enable
	   ---vbus interrupt enable
	 */
	DEBUG_DBG(">>> sp_udc_enable\n");
	/* usb device controller interrupt flag */
	udc_write(udc_read(UDCIF) & 0xFFFF, UDCIF);
	/* usb device link layer interrupt flag */
	udc_write(0xefffffff, UDLIF);

	udc_write(VBUS_IF, UDCIE);
	udc_write(EP0S_IF | RESET_IF | RESET_RELEASE_IF, UDLIE);

	if (sp_vbus_detect()) {
		udc_write(udc_read(UDLIE) | SUS_IF, UDLIE);
		udelay(200);
		udc_write(udc_read(UDCCS) | UPHY_CLK_CSS, UDCCS);	/*PREVENT SUSP */
		udelay(200);
		/*Force to Connect */
	}

	DEBUG_DBG("func:%s line:%d\n", __FUNCTION__, __LINE__);
	DEBUG_DBG("<<< sp_udc_enable\n");
}

static int sp_udc_get_frame(struct usb_gadget *_gadget)
{
	u32 sof_value;

	DEBUG_NOTICE(">>> sp_udc_get_frame...\n");
	sof_value = udc_read(UDFRNUM);
	DEBUG_NOTICE("<<< sp_udc_get_frame...\n");

	return sof_value;
}

static int sp_udc_wakeup(struct usb_gadget *_gadget)
{
	DEBUG_DBG(">>> sp_udc_wakeup...\n");
	DEBUG_DBG("<<< sp_udc_wakeup...\n");

	return 0;
}

static int sp_udc_set_selfpowered(struct usb_gadget *_gadget,
				      int is_selfpowered)
{
	DEBUG_DBG(">>> sp_udc_set_selfpowered...\n");
	DEBUG_DBG("<<< sp_udc_set_selfpowered...\n");

	return 0;
}

static int sp_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	DEBUG_NOTICE(">>> sp_udc_pullup...\n");

	if (is_on) {
		DEBUG_NOTICE("Force to Connect\n");
		/*Force to Connect */
		udc_write((udc_read(UDLCSET) | 8) & 0xFE, UDLCSET);
	} else {
		DEBUG_NOTICE("Force to Disconnect\n");
		/*Force to Disconnect */
		udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
	}

	DEBUG_NOTICE("<<< sp_udc_pullup...\n");

	return 0;
}

static int sp_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{
	DEBUG_DBG(">>> sp_udc_vbus_session...\n");
	DEBUG_DBG("<<< sp_udc_vbus_session...\n");

	return 0;
}

static int sp_vbus_draw(struct usb_gadget *_gadget, unsigned ma)
{
	DEBUG_DBG(">>> sp_vbus_draw...\n");
	DEBUG_DBG("<<< sp_vbus_draw...\n");

	return 0;
}

static int sp_udc_start(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct sp_udc *udc = the_controller;

	DEBUG_NOTICE(">>> sp_udc_start...\n");
	/* Sanity checks */
	if (!udc)
		return -ENODEV;
	if (udc->driver)
		return -EBUSY;

	if (!driver->bind || !driver->setup || driver->max_speed < USB_SPEED_FULL) {
		printk(KERN_ERR "Invalid driver: bind %p setup %p speed %d\n", driver->bind,
			  driver->setup, driver->max_speed);

		return -EINVAL;
	}

	/* Hook the driver */
	udc->driver = driver;
	sp_udc_enable(udc);

	DEBUG_NOTICE("<<< sp_udc_start...\n");

	return 0;
}

static int sp_udc_stop(struct usb_gadget *gadget)
{
	struct sp_udc *udc = the_controller;

	if (!udc)
		return -ENODEV;

	DEBUG_NOTICE(">>> sp_udc_stop...\n");

	// if (!driver || driver != udc->driver || !driver->unbind)
	// 	return -EINVAL;

	/* report disconnect */
	if (udc->driver->disconnect)
		udc->driver->disconnect(&udc->gadget);

	udc->driver->unbind(&udc->gadget);

	device_del(&udc->gadget.dev);
	udc->driver = NULL;

	/* Disable udc
	sp_udc_disable(udc);  TODO: sp_udc_disable*/
	DEBUG_NOTICE("<<< sp_udc_stop...\n");

	return 0;
}

static struct usb_ep *find_ep(struct usb_gadget *gadget, const char *name)
{
	struct usb_ep	*ep;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		if (0 == strcmp (ep->name, name))
			return ep;
	}

	return NULL;
}

static struct usb_ep *sp_match_ep(struct usb_gadget *_gadget,
					struct usb_endpoint_descriptor *desc,
					struct usb_ss_ep_comp_descriptor *ep_comp)
{
	struct usb_ep *ep;
	u8 type;

	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	desc->bInterval = 5;
	ep = NULL;

	if (USB_ENDPOINT_XFER_BULK == type) {
		ep = find_ep(_gadget, (USB_DIR_IN & desc->bEndpointAddress) ?
			"ep1in-bulk" : "ep11out-bulk");
	}
	else if (USB_ENDPOINT_XFER_INT == type) {
		ep = find_ep(_gadget, "ep3in-int");
	}
	else if (USB_ENDPOINT_XFER_ISOC == type) {
		ep = find_ep(_gadget, (USB_DIR_IN & desc->bEndpointAddress) ?
			"ep5-iso" : "ep12-iso");
	}

	return ep;
}

static const struct usb_gadget_ops sp_ops = {
	.get_frame 		= sp_udc_get_frame,
	.wakeup 		= sp_udc_wakeup,
	.set_selfpowered 	= sp_udc_set_selfpowered,
	.pullup 		= sp_udc_pullup,
	.vbus_session 		= sp_udc_vbus_session,
	.vbus_draw 		= sp_vbus_draw,
	.udc_start 		= sp_udc_start,
	.udc_stop 		= sp_udc_stop,
	.match_ep 		= sp_match_ep,
};

static const struct usb_ep_ops sp_ep_ops = {
	.enable 		= sp_udc_ep_enable,
	.disable 		= sp_udc_ep_disable,

	.alloc_request 		= sp_udc_alloc_request,
	.free_request 		= sp_udc_free_request,

	.queue 			= sp_udc_queue,
	.dequeue 		= sp_udc_dequeue,

	.set_halt 		= sp_udc_set_halt,
};

static struct sp_udc memory = {
	.gadget = {
			.max_speed = USB_SPEED_HIGH,
			.ops = &sp_ops,
			.ep0 = &memory.ep[0].ep,
			.name = gadget_name,
			.dev = {
				.init_name = "gadget",
			},
		   },

	/* control endpoint */
	.ep[0] = {
		  .num = 0,
		  .ep = {
			 .name = ep0name,
			 .ops = &sp_ep_ops,
			 .maxpacket = EP0_FIFO_SIZE,
			 },
		  .dev = &memory,
		  },

	/* first group of endpoints */
	.ep[1] = {
		  .num = 1,
		  .ep = {
			 .name = "ep1in-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = 512,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_IN | EP1,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[2] = {
		  .num = 2,
		  .ep = {
			 .name = "ep2out-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_OUT | EP2,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[3] = {
		  .num = 3,
		  .ep = {
			 .name = "ep3in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = 64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_IN | EP3,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,
		  },

	.ep[4] = {
		  .num = 4,
		  .ep = {
			 .name = "ep4in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = 64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_IN | EP4,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,
		  },

	.ep[5] = {
		  .num = 5,
		  .ep = {
			 .name = "ep5-iso",
			 .ops = &sp_ep_ops,
			 .maxpacket = 1024 * 3,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_OUT | EP5,
		  .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		  },

	.ep[6] = {
		  .num = 6,
		  .ep = {
			 .name = "ep6in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = 64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_IN | EP6,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,
		  },

	.ep[7] = {
		  .num = 7,
		  .ep = {
			 .name = "ep7-iso",
			 .ops = &sp_ep_ops,
			 .maxpacket = FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_OUT | EP7,
		  .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		  },

	.ep[8] = {
		  .num = 8,
		  .ep = {
			 .name = "ep8in-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_IN | EP8,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[9] = {
		  .num = 9,
		  .ep = {
			 .name = "ep9out-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = 64,
		  .bEndpointAddress = USB_DIR_OUT | EP9,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[10] = {
		   .num = 10,
		   .ep = {
			  .name = "ep10in-bulk",
			  .ops = &sp_ep_ops,
			  .maxpacket = FIFO_SIZE64,
			  },
		   .dev = &memory,
		   .fifo_size = 64,
		   .bEndpointAddress = USB_DIR_IN | EP10,
		   .bmAttributes = USB_ENDPOINT_XFER_BULK,
		   },

	.ep[11] = {
		   .num = 11,
		   .ep = {
			  .name = "ep11out-bulk",
			  .ops = &sp_ep_ops,
			  .maxpacket = 512,
			  },
		   .dev = &memory,
		   .fifo_size = 512,
		   .bEndpointAddress = USB_DIR_OUT | EP11,
		   .bmAttributes = USB_ENDPOINT_XFER_BULK,
		   },

	.ep[12] = {
		   .num = 12,
		   .ep = {
			  .name = "ep12-iso",
			  .ops = &sp_ep_ops,
			  .maxpacket = 1024,
			  },
		   .dev = &memory,
		   .fifo_size = 64,
		   .bEndpointAddress = USB_DIR_OUT | EP12,
		   .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		   },
};

static void sp_udc_reinit(struct sp_udc *udc)
{
	u32 i = 0;

	DEBUG_DBG(">>> sp_udc_reinit\n");
	/* device/ep0 records init */
	INIT_LIST_HEAD(&udc->gadget.ep_list);
	INIT_LIST_HEAD(&udc->gadget.ep0->ep_list);
	udc->ep0state = EP0_IDLE;

	for (i = 0; i < SP_MAXENDPOINTS; i++) {
		struct sp_ep *ep = &udc->ep[i];

		if (i != 0){
			list_add_tail(&ep->ep.ep_list, &udc->gadget.ep_list);
		}

		ep->dev = udc;
		ep->desc = NULL;
		ep->halted = 0;
		INIT_LIST_HEAD(&ep->queue);
		usb_ep_set_maxpacket_limit(&ep->ep, ep->ep.maxpacket);
	}

	DEBUG_DBG("<<< sp_udc_reinit\n");
}

#ifdef CONFIG_FIQ_GLUE
static int fiq_isr(int fiq, void *data)
{
	sp_udc_irq(irq_num,&memory);

	return IRQ_HANDLED;
}

static void fiq_handler(struct fiq_glue_handler *h, void *regs, void *svc_sp)
{
	void __iomem *cpu_base = gic_base(GIC_CPU_BASE);
	u32 irqstat, irqnr;

	irqstat = readl_relaxed(cpu_base + GIC_CPU_HIGHPRI);
	irqnr = irqstat & ~0x1c00;

	if (irqnr == irq_num) {
		readl_relaxed(cpu_base + GIC_CPU_INTACK);
		fiq_isr(irqnr, h);
		writel_relaxed(irqstat, cpu_base + GIC_CPU_EOI);
	}
}
static irqreturn_t udcThreadHandler(int irq, void *dev_id)
{
	DEBUG_DBG("<DSR>\n");
	return IRQ_HANDLED;
}
#endif
/********** PLATFORM DRIVER & DEVICE *******************/

static void sp_udc_work(struct work_struct *work)
{
	struct sp_work* sw = container_of(work, struct sp_work, work);
	struct sp_ep *ep = &memory.ep[sw->ep_num];
	
	DEBUG_DBG(">>> sp_udc_work");
	DEBUG_DBG("sw->ep_num = %d", sw->ep_num);

	local_irq_save(ep->flags);

	sp_udc_handle_ep(ep, NULL);

	local_irq_restore(ep->flags);
	
	DEBUG_DBG("<<< sp_udc_work");
}

static int sp_init_work(int ep_num, char *desc, void (*func)(struct work_struct*))
{
	int ret = 0;
	struct sp_udc *udc = &memory;

	sema_init(&udc->ep[ep_num].wsem, 1);

	udc->wq[ep_num].queue = create_singlethread_workqueue(desc);
	
	if (!udc->wq[ep_num].queue) {
		printk(KERN_ERR "cannot create workqueue %s\n", desc);
		ret = -ENOMEM;
	}

	INIT_WORK(&udc->wq[ep_num].sw.work, func);

	return ret;
}

static int sp_udc_probe(struct platform_device *pdev)
{
	struct sp_udc *udc = &memory;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	static u64 rsrc_start, rsrc_len;

	/*enable usb controller clock*/
	udc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(udc->clk)) {
		pr_err("not found clk source\n");
		return PTR_ERR(udc->clk);
	}
	clk_prepare(udc->clk);
	clk_enable(udc->clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq_num = platform_get_irq(pdev, 0);
	if (!res || !irq_num) {
		printk(KERN_ERR "Not enough platform resources.\n");
		ret = -ENODEV;
		goto error;
	}

	rsrc_start = res->start;
	rsrc_len = resource_size(res);
	DEBUG_DBG("udc-line:%d,%lld,%lld,irq:%d\n", __LINE__, rsrc_start, rsrc_len, irq_num);
	base_addr = ioremap(rsrc_start, rsrc_len);

	if (!base_addr) {
		ret = -ENOMEM;
		goto err_mem;
	}

	udc->gadget.dev.parent = dev;
	udc->gadget.dev.dma_mask = dev->dma_mask;

	ret = sp_init_work(11, "sp-udc-ep11", sp_udc_work);
	if (ret != 0) goto err_map;

	ret = sp_init_work(1, "sp-udc-ep1", sp_udc_work);
	if (ret != 0) goto err_map;

	the_controller = udc;
	sp_udc_reinit(udc);
	
	ret = request_irq(irq_num, sp_udc_irq, 0, gadget_name, udc);

	if (ret != 0) {
		printk(KERN_ERR "cannot get irq %i, err %d\n", irq_num, ret);
		ret = -EBUSY;
		goto err_map;
	}

	platform_set_drvdata(pdev, udc);
	udc->vbus = 0;
	spin_lock_init(&udc->lock);
	spin_lock_init(&plock);
	spin_lock_init(&qlock);

	init_ep_spin();
	pdev->id = 1;

	DEBUG_DBG("udc-line:%d\n", __LINE__);
	ret = usb_add_gadget_udc(&pdev->dev, &udc->gadget);
	if (ret) {
		goto err_add_udc;
	}
	DEBUG_DBG("probe sp udc ok %x\n", udc_read(VERSION));

	device_create_file(&pdev->dev, &dev_attr_udc_ctrl);

	return 0;
err_add_udc:
	if(irq_num)
		free_irq(irq_num, udc);
err_map:
	DEBUG_DBG("probe sp udc fail\n");
	return ret;
err_mem:
	release_mem_region(rsrc_start, rsrc_len);
error:
	printk(KERN_ERR "probe sp udc fail\n");
	return ret;

}

static int sp_udc_remove(struct platform_device *pdev)
{
	struct sp_udc *udc = platform_get_drvdata(pdev);

	DEBUG_NOTICE("<<< sp_udc remove\n");
	
	if (udc->driver)
		return -EBUSY;

	device_remove_file(&pdev->dev, &dev_attr_udc_ctrl);
	usb_del_gadget_udc(&udc->gadget);

	if(irq_num)
		free_irq(irq_num, udc);

	if(base_addr)
		iounmap(base_addr);

	platform_set_drvdata(pdev, NULL);

	/*disable usb controller clock*/
	clk_disable(udc->clk);
	DEBUG_NOTICE("<<< sp_udc remove\n");

	return 0;
}

#ifdef CONFIG_PM
static int sp_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
	DEBUG_DBG(">>> sp_udc_suspend\n");
	DEBUG_DBG("<<< sp_udc_suspend\n");

	return 0;
}

static int sp_udc_resume(struct platform_device *pdev)
{
	sp_udc_enable(NULL);

	udc_write(udc_read(UDNBIE) | EP8N_IF | EP8I_IF, UDNBIE);
	udc_write(EP_ENA | EP_DIR, UDEP89C);
	udc_write(udc_read(UDNBIE) | EP9N_IF | EP9O_IF, UDNBIE);

	return 0;
}
#else
#define sp_udc_suspend	NULL
#define sp_udc_resume	NULL
#endif

#define CONFIG_USB_HOST_RESET_SP 1

#ifdef CONFIG_USB_HOST_RESET_SP
static int udc_init_c(void)
{
	sp_udc_enable(NULL);

	/*iap debug */
	udc_write(udc_read(UDNBIE) | EP8N_IF | EP8I_IF, UDNBIE);
	udc_write(EP_ENA | EP_DIR, UDEP89C);
	udc_write(udc_read(UDNBIE) | EP9N_IF | EP9O_IF, UDNBIE);

	/*pullup */
	udc_write((udc_read(UDLCSET) | 8) & 0xFE, UDLCSET);
	udc_write(udc_read(UDCIE) & (~VBUS_IF), UDCIE);
	udc_write(udc_read(UDCIE) & (~EPC_TRB_IF), UDCIE);
	udc_write(udc_read(UDCIE) & (~EPC_ERF_IF), UDCIE);

	return 0;
}
#endif

void usb_switch(int device)
{
	void __iomem *regs = (void __iomem *)B_SYSTEM_BASE;

	if (device) {
		writel(RF_MASK_V_SET(1 << 4), regs + USBC_CTL_OFFSET);
		writel(RF_MASK_V_CLR(1 << 5), regs + USBC_CTL_OFFSET);
		DEBUG_NOTICE("host to device\n");
	} else {
		udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
		writel(RF_MASK_V_SET(1 << 5), regs + USBC_CTL_OFFSET);
		DEBUG_NOTICE("device to host!\n");
	}
}

void ctrl_rx_squelch(void)/*Controlling squelch signal to slove the uphy bad signal problem*/
{
	udc_write(udc_read(UEP12DMACS) | RX_STEP7, UEP12DMACS);	/*control rx signal */

	DEBUG_DBG("ctrl_rx_squelch UEP12DMACS: %x\n",udc_read(UEP12DMACS));
}

EXPORT_SYMBOL(usb_switch);
EXPORT_SYMBOL(ctrl_rx_squelch);

void udc_otg_ctrl(void)
{
	u32 val;

	val = USB_CLK_EN | UPHY_CLK_CSS;
	udc_write(val, UDCCS);
}
EXPORT_SYMBOL(udc_otg_ctrl);

void detech_start(void)
{
#ifdef	CONFIG_USB_HOST_RESET_SP
	udc_init_c();
#endif
	DEBUG_NOTICE("detech_start......\n");
}
EXPORT_SYMBOL(detech_start);

static const struct of_device_id sunplus_udc0_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-udc0" },
	{ }
};
MODULE_DEVICE_TABLE(of, sunplus_udc0_ids);

static const struct of_device_id sunplus_udc1_ids[] = {
	{ .compatible = "sunplus,sp7021-usb-udc1", },
	{ }
};
MODULE_DEVICE_TABLE(of, sunplus_udc1_ids);

static struct platform_driver sunplus_driver_udc = {
	.driver		= {
		.name	= "sunplus-udc0",
		.of_match_table = sunplus_udc0_ids,
	},
	.probe		= sp_udc_probe,
	.remove		= sp_udc_remove,
	.suspend	= sp_udc_suspend,
	.resume		= sp_udc_resume,
};

static int __init udc_init(void)
{
	int ret;
	DEBUG_INFO("register sunplus_driver_udc\n");
	ret = platform_driver_register(&sunplus_driver_udc);

	return ret;
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&sunplus_driver_udc);
}

MODULE_LICENSE("GPL");

module_init(udc_init);
module_exit(udc_exit);
