#include <linux/init.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
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
#ifdef CONFIG_USB_SUNPLUS_OTG
#include "../../phy/sunplus-otg.h"
#endif

#ifdef CONFIG_FIQ_GLUE
#include <asm/fiq.h>
#include <asm/fiq_glue.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#endif

#ifdef ISP_DEBUG
char *g_hw;
#include <linux/vmalloc.h>

#include "usb_device.c"
#endif

#include <linux/clk.h>
#include <linux/vmalloc.h>

#define EP1_DMA

#define ISO_DEBUG_INFO
#ifdef CONFIG_GADGET_USB0
#define USB0
#endif
//#define INT_TEST

#define USBTEST_ZERO

#define MANUAL_EP11
#define  SW_IRQ

static u32 is_ncm = 0;
#ifndef USBTEST_ZERO
struct tasklet_struct *t_task;
EXPORT_SYMBOL(t_task);
#endif
char sof_n = 0;

static u8 bus_reset_finish_flag = false;
bool ncm_initialize_finish_flag = false;
EXPORT_SYMBOL_GPL(ncm_initialize_finish_flag);

void usb_switch(int device);

#include "sunplus_udc.h"
#include "sunplus_udc_regs.h"
/*#define ISP_DEBUG*/
#define VUBUS_CD_POLL	HZ/4
struct timer_list vbus_polling_timer;
struct timer_list sof_polling_timer;
char is_config = 0;
static u32 sof_value = 0x1000;
static int irq_num = 0;
static u32 temp_sof_value;
static u8 platform_device_handle_flag = false;
static u8 first_enter_polling_timer_flag = false;

#define IRQ_USB_DEV_PORT0				45
#define IRQ_USB_DEV_PORT1				48
#define TO_HOST 						0
#define TO_DEVICE						1
#define BUS_RESET_FOR_CHIRP_DELAY		2000	/*ctrl rx singal keep the strongest time */
#define DMA_FLUSH_TIMEOUT_DELAY			300000
#define IC_VERSION_A     				0x630
#define ZERO_TEST_TAG					0x484d434e
#define ISP_DEBUG_SIZE					(0x8800 * 512)

#define FULL_SPEED_DMA_SIZE				64
#define HIGH_SPEED_DMA_SIZE				512
#define UDC_FLASH_BUFFER_SIZE 			(1024*64)
#define EPC_FIFO_SIZE 					(32)
#define	DMA_ADDR_INVALID				(~(dma_addr_t)0)
static u32 bulkep_dma_block_size = 0;
u32 *trb_c;
u32 *trb_c_start;
u32 *trb_c_end;			/*reserve 2 trb:one for remain buf less than ep_fifo_size ;next for link trb */
u32 *trb_event;
static char g_flag_ep5;
u32 ep5_use = 0;
u32 ep5_e = 0;
static u32 tog = 1;
u32 *trb_c_12;
u32 *trb_c_start_12;
u32 *trb_c_end_12;
u32 *trb_event_12;
static char g_flag_ep12;
u32 ep12_use = 0;
u32 ep12_e = 0;
static u32 tog_12 = 1;
struct sp_ep_iso {
	struct list_head queue;
	int act;
};
struct sp_ep_iso ep_iso_5;
struct sp_ep_iso ep_iso_12;
char dma_flag = 0;
char dma_flag_b = 0;
#ifdef OTG_TEST
static u32 dmsg = 0xFF;
#else
static u32 dmsg = 0x2;
#endif
u32 nth_len;
u32 dma_len_ep1;
u32 dma_len;

struct usb_cdc_ncm_nth16 {
	__le32 dwSignature;
	__le16 wHeaderLength;
	__le16 wSequence;
	__le16 wBlockLength;
	__le16 wNdpIndex;
} __attribute__ ((packed));

module_param(dmsg, uint, 0644);
static u32 intface_d = 0x0;
module_param(intface_d, uint, 0644);
u32 dma_fail = 0x0;
module_param(dma_fail, uint, 0644);
static unsigned long time_ns = 0x0;
module_param(time_ns, ulong, 0644);
static u32 d_time0 = VUBUS_CD_POLL * 2;
module_param(d_time0, uint, 0644);
static u32 d_time1 = 10;
module_param(d_time1, uint, 0644);
u32 is_vera = 0x0;
/*coverity[declared_but_not_referenced]*/
module_param(is_vera, uint, 0644);
u32 is_ean = 0;
/*coverity[declared_but_not_referenced]*/
module_param(is_ean, uint, 0644);
EXPORT_SYMBOL_GPL(is_vera);
EXPORT_SYMBOL_GPL(is_ean);
EXPORT_SYMBOL_GPL(dma_fail);
#ifdef CONFIG_USB_SUNPLUS_OTG
	#ifdef OTG_TEST
static u32 dev_otg_status = 1;
	#else
static u32 dev_otg_status = 0;
	#endif
module_param(dev_otg_status, uint, 0644);
#endif

static const char gadget_name[] = "sp_udc";
static struct semaphore ep1_ack_sem, ep1_dma_sem;
static struct semaphore ep12_bulk_out_ack_sem, ep12_bulk_out_nak_sem;
static struct semaphore ep9_ack_sem;
static struct semaphore ep11_ack_sem, ep11_dma_sem;
static struct semaphore ep11_sw_sem;

static void __iomem *base_addr;

static struct sp_udc *the_controller;
#define DEBUG_DBG(fmt, arg...)		do { if (dmsg&(1<<0)) printk(KERN_DEBUG fmt,##arg); } while (0)
#define DEBUG_NOTICE(fmt, arg...)	do { if (dmsg&(1<<1)) printk(KERN_NOTICE fmt,##arg); } while (0)
#define DEBUG_INFO(fmt, arg...)		do { if (dmsg&(1<<2)) printk(KERN_INFO fmt,##arg); } while (0)
#define DEBUG_ERR(fmt, arg...)		do { printk(KERN_ERR fmt,##arg); } while (0)
static u32 full_spd = 0;
static struct sp_udc memory;
int in_p_num;
static inline u32 udc_read(u32 reg);
static inline void udc_write(u32 value, u32 reg);
void detech_start(void);
#ifdef CONFIG_USB_SUNPLUS_OTG
extern void sp_accept_b_hnp_en_feature(struct usb_otg *otg);
#endif


static void reset_global_value(void)
{
	dma_len = 0;
	dma_flag_b = 0;
}

static void udc_clean_dcache_range(unsigned int start,unsigned int start_pa,unsigned int size){
	unsigned long oldIrq;
	/*void* vaddr = (void *)start;*/

	local_irq_save(oldIrq);
	/* flush L1 cache by range */
	/*dmac_flush_range(vaddr,vaddr+size);*/
	/* flush L2 cache by range */
	outer_flush_range(start_pa, start_pa+size);
	local_irq_restore(oldIrq);
 }

static void udc_invalidate_dcache_range(unsigned int start,unsigned int start_pa,unsigned int size){
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	/* invalidate L1 cache by range */
	/*dmac_unmap_area((void *)start,size,2);*/
	/* invalidate L2 cache by range */
	outer_inv_range(start_pa, start_pa+size);
	local_irq_restore(oldIrq);
}

static ssize_t show_udc_ctrl(struct device *dev,
			      struct device_attribute *attr, char *buffer)
{
	u32 result = 0;

	struct sp_ep *ep = &memory.ep[1];
	spin_lock(&ep->lock);
	result = list_empty(&ep->queue);
	spin_unlock(&ep->lock);

	DEBUG_NOTICE("ep1 is empty:%d req %d\n", result, in_p_num);
	DEBUG_NOTICE("dma_len = %d %d\n", dma_len, full_spd);
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
		is_ncm = 1;
		udc_write(udc_read(UDEPBPPC) | SOFT_DISC, UDEPBPPC);
	} else if (*buffer == 'o') { /*ping pong buffer auto switch*/
		is_ncm = 0;
		udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
		udc_write(udc_read(UDEPBPPC) & 0XFE, UDEPBPPC);
	} else {
		ret &= ~SUPP_SETDESC;
	}

	DEBUG_NOTICE("full_spd = %d %d \n", full_spd, (*buffer - 48));
	udc_write(ret, UDLCSET);
	return count;
}

static DEVICE_ATTR(udc_ctrl, S_IWUSR | S_IRUSR, show_udc_ctrl, store_udc_ctrl);


void PrintBlock_usb(u8 *pBuffStar, u32 uiBuffLen)
{
	u32 uiIter;
	pBuffStar = pBuffStar;
	DEBUG_NOTICE("pBuffStar=%p\n", pBuffStar);
	DEBUG_NOTICE(" %02x", pBuffStar[0]);
	for (uiIter = 1; uiIter < uiBuffLen; uiIter++) {
		if (uiIter % 16 == 0)
			DEBUG_NOTICE("\n");
		if (uiIter % 512 == 0)
			DEBUG_NOTICE("\n");
		DEBUG_NOTICE(" %02x", pBuffStar[uiIter]);
	}
	DEBUG_NOTICE("\n");
}

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

void sp_sem_init(void)
{
	sema_init(&ep1_ack_sem, 1);
	sema_init(&ep1_dma_sem, 1);
	sema_init(&ep12_bulk_out_ack_sem, 1);
	sema_init(&ep12_bulk_out_nak_sem, 1);
	sema_init(&ep9_ack_sem, 1);
	sema_init(&ep11_ack_sem, 1);
	sema_init(&ep11_dma_sem, 1);
	sema_init(&ep11_sw_sem, 1);
}

void init_ep_spin(void)
{
	int i;
	for (i = 0; i < SP_MAXENDPOINTS; i++) {
		spin_lock_init(&memory.ep[i].lock);
	}
}

static void sp_del_list(struct list_head *mylist, spinlock_t * mylock)
{
	spin_lock(mylock);
	list_del_init(mylist);
	spin_unlock(mylock);
}

static void sp_list_add(struct list_head *mylist, struct list_head *head,
			spinlock_t * mylock)
{
	spin_lock(mylock);
	list_add_tail(mylist, head);
	spin_unlock(mylock);
}

static int sp_udc_list_empty(struct list_head *head, spinlock_t * lock)
{
	int ret;
	spin_lock(lock);
	ret = list_empty(head);
	spin_unlock(lock);
	return ret;
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

static void ep_iso_debug_dcache(u32 *event,u8 ep_num)
{
	u32 trb_use;
	u32 i = 0;

	struct iso_trb *trb = (struct iso_trb *)event;

	if(ep_num == EP5)
		trb_use = ep5_use;
	else
		trb_use = 32;
	udc_invalidate_dcache_range((u32)event,
				   __pa(event),
				   TRB_NUM * 16);
	DEBUG_DBG("event\n");
	for (i = 0; i < trb_use; i++) {
		if (i % 12 == 0)
			DEBUG_NOTICE("\n");
		DEBUG_NOTICE("%x %x,", trb[i].size_cc,
			     trb[i].cmd);
	}
	DEBUG_DBG(" event end\n");
}

static void ep_iso_done(struct sp_ep *ep, struct sp_request *req,
			int status)
{
#if 0
	req->req.status = status;
	sp_del_list(&req->queue, &ep->lock);
	if (ep->num == EP5)
		sp_list_add(&req->queue, &ep_iso_5.queue, &ep->lock);
	else
		sp_list_add(&req->queue, &ep_iso_12.queue, &ep->lock);
#endif
	unsigned halted = ep->halted;

	if (ep->num == EP1 && sp_udc_list_empty(&req->queue, &ep->lock)) {
		DEBUG_ERR("1double done ep%d\n", ep->num);
		return;
	}
	sp_del_list(&req->queue, &ep->lock);
	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	ep->halted = 1;
	if (ep->num == EP1)
		in_p_num++;
	/*DEBUG_DBG("done ep%d act=%d\n", ep->num, req->req.actual);*/
	req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static void sp_udc_done(struct sp_ep *ep, struct sp_request *req,
			    int status)
{
	unsigned halted = ep->halted;

	if (ep->num == EP1 && sp_udc_list_empty(&req->queue, &ep->lock)) {
		DEBUG_ERR("1double done ep%d\n", ep->num);
		return;
	}
	sp_del_list(&req->queue, &ep->lock);
	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	ep->halted = 1;
	if (ep->num == EP1)
		in_p_num++;
	/*DEBUG_DBG("done ep%d act=%d\n", ep->num, req->req.actual);*/
	req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static void sp_udc_nuke(struct sp_udc *udc, struct sp_ep *ep,
			    int status)
{
	/* Sanity check */
	DEBUG_DBG(">>> sp_udc_nuke...\n");
	if (ep->num == EP5 || ep->num == EP12) {
#ifndef INT_TEST
		while (!list_empty(&ep_iso_5.queue)) {
			struct sp_request *req;
			req = list_entry(ep_iso_5.queue.next, struct sp_request, queue);
			list_del_init(&req->queue);
			req->req.status = status;
			req->req.complete(&ep->ep, &req->req);
		}
#endif
		while (!list_empty(&ep_iso_12.queue)) {
			struct sp_request *req;
			req = list_entry(ep_iso_12.queue.next, struct sp_request, queue);
			list_del_init(&req->queue);
			req->req.status = status;
			req->req.complete(&ep->ep, &req->req);
		}
	}
	if (&ep->queue == NULL)
		return;

	while (!list_empty(&ep->queue)) {
		struct sp_request *req;
		req = list_entry(ep->queue.next, struct sp_request, queue);
		sp_udc_done(ep, req, status);
	}
	DEBUG_DBG("<<< sp_udc_nuke...\n");
}

static void sp_reinit_iap(struct sp_ep *ep, struct sp_request *req,
			      int status)
{
	unsigned halted = ep->halted;
	req->req.status = status;
	ep->halted = 1;
	req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static int sp_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct sp_ep *ep = to_sp_ep(_ep);
	u32 idx;
	u32 v = 0;

	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG_ERR("%s: inval 2\n", __func__);
		return -EINVAL;
	}

	DEBUG_DBG(">>>> sp_udc_set_halt\n");

	idx = ep->bEndpointAddress & 0x7F;
	DEBUG_NOTICE("udc set halt idx=%x val=%x \n", idx, value);

	switch (idx) {
	case EP1:
		v = SETEP1STL;
		break;
	case EP2:
		v = SETEP2STL;
		break;
	case EP3:
		v = SETEP3STL;
		break;
	case EP4:
		break;
	case EP5:
		break;
	case EP6:
		break;
	case EP7:
		break;
	case EP8:
		v = SETEP8STL;
		break;
	case EP9:
		v = SETEP9STL;
		break;
	case EP10:
		break;
	case EP11:
		v = SETEPBSTL;
		break;
	default:
		return -EINVAL;
	}
	if ((!value) && v)
		v = v << 16;
	DEBUG_ERR("udc set halt v=%x  \n", v);
	udc_write((udc_read(UDLCSTL) | v), UDLCSTL);
	ep->halted = value ? 1 : 0;
	return 0;
}

static int sp_udc_ep_enable(struct usb_ep *_ep,
				const struct usb_endpoint_descriptor *desc)
{
	struct sp_udc *dev;
	struct sp_ep *ep;
	u32 max;
	u32 tmp;
	u32 udll_int_en;
	u32 int_en_reg2;

	ep = to_sp_ep(_ep);

	if (!_ep || !desc || _ep->name == ep0name || desc->bDescriptorType != USB_DT_ENDPOINT){
		DEBUG_ERR("%s.%d,,%p,%p,%p,%s,%d\n",__FUNCTION__,__LINE__,
			_ep,desc,ep->desc,_ep->name,desc->bDescriptorType);
		return -EINVAL;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	udll_int_en = udc_read(UDLIE);
	int_en_reg2 = udc_read(UDNBIE);

	switch (ep->num) {
	case EP0:
		/* enable irqs */
		udll_int_en |= EP0S_IF;
		break;
	case EP1:
		udll_int_en |= EP1I_IF | EP1N_IF;
		udc_write(EP_DIR | EP_ENA | RESET_PIPO_FIFO, UDEP12C);
		break;
	case EP2:
		udll_int_en |= EP2N_IF | EP2O_IF;
		udc_write(EP_ENA /*|RESET_PIPO_FIFO */ , UDEP12C);
		/*debug for switch buff
		udc_write(0 ,UDEP2PPC);*/
		break;
	case EP3:
		udll_int_en |= EP3I_IF;
		/*udc_write((1<<0), UDEP3CTRL);*/
		break;
	case EP4:
		break;
	case EP5:
		udll_int_en |= EP5I_IF;
		udc_write(udc_read(UDCIE) | VIDEO_TRB_IF | VIDEO_ERF_IF, UDCIE);
		udc_write((1 << 0), UDEP5CTRL);
		 /*ENABLE*/ trb_c = kzalloc(ALL_TRB_SIZE, GFP_ATOMIC);	/*cmd ring */
		trb_c_start = trb_c;
		trb_c_end = trb_c + ((TRB_NUM - 2) * TRB_SIZE) / sizeof(u32);
		trb_event = kzalloc(ALL_TRB_SIZE, GFP_ATOMIC);	/*event ring */
		memset(trb_event, 0, TRB_NUM * TRB_SIZE);
		DEBUG_DBG("01 CRCR=%x s %p end %p\n", udc_read(UDVDMA_CRCR),
			  trb_c_start, trb_c_end);
		trb_c = (u32 *) (((u32) (trb_c)) & (~0x3f));
		DEBUG_DBG("01 CRCR=%x trb_c addr %lu\n", udc_read(UDVDMA_CRCR),
			  (unsigned long)__pa(trb_c));
		tog = 1;
		ep_iso_5.act = 0;
		INIT_LIST_HEAD(&ep_iso_5.queue);
		break;
	case EP6:
		break;
	case EP7:
		udll_int_en |= EP7I_IF;
		udc_write((1 << 0), UDEP7CTRL);
		 /*ENABLE*/ break;
	case EP8:
		int_en_reg2 |= EP8N_IF | EP8I_IF;
		udc_write(EP_DIR | EP_ENA | RESET_PIPO_FIFO, UDEP89C);
		break;
	case EP9:
		int_en_reg2 |= EP9N_IF | EP9O_IF;
		udc_write(EP_ENA /*|RESET_PIPO_FIFO */ , UDEP89C);
		udc_write(0, UDEP9PPC);
		break;
	case EP10:
		int_en_reg2 |= EP10N_IF | EP10I_IF;
		udc_write(EP_DIR | EP_ENA | RESET_PIPO_FIFO, UDEPABC);
		break;
	case EP11:
		int_en_reg2 |= EP11N_IF | EP11O_IF;
		udc_write(EP_ENA | RESET_PIPO_FIFO, UDEPABC);
#ifdef MANUAL_EP11
		if (!is_ncm && !is_vera)
			udc_write(0, UDEPBPPC);
#endif
		break;
	case EP12:
		udc_write(udc_read(UDCIE) | EPC_TRB_IF | EPC_ERF_IF, UDCIE);
		udll_int_en |= EPC_FAIL_IF | EPC_OFLOW_IF | EPC_SUCC_IF | EPC_DERR_IF;
		udc_write(EPC_FAIL_IF | EPC_OFLOW_IF | EPC_SUCC_IF | EPC_DERR_IF, UDLIF);
		udc_write(0x11, UDEPCS);
		udc_write((1 << 0), UDEPCCTRL);
		 /*ENABLE*/ trb_c_12 = kzalloc(ALL_TRB_SIZE, GFP_ATOMIC);	/*cmd ring */
		trb_c_start_12 = trb_c_12;
		trb_c_end_12 = trb_c_12 + ((TRB_NUM - 2) * TRB_SIZE) / sizeof(u32);
		trb_event_12 = kzalloc(ALL_TRB_SIZE, GFP_ATOMIC);	/*event ring */
		DEBUG_DBG("EPc CRCR=%x s %p end %p\n", udc_read(UDEPCDMA_CRCR),
			  trb_c_start_12, trb_c_end_12);
		trb_c_12 = (u32 *) (((u32) (trb_c_12)) & (~0x3f));
		tog_12 = 1;
		ep_iso_12.act = 0;
		INIT_LIST_HEAD(&ep_iso_12.queue);
		break;
	default:
		return -EINVAL;
	}

	udc_write(udll_int_en, UDLIE);
	udc_write(int_en_reg2, UDNBIE);
	DEBUG_DBG("ep_enable UDLIE=%x\n", udc_read(UDLIE));
	/* print some debug message */
	tmp = desc->bEndpointAddress;
	DEBUG_NOTICE("enable %s(%d) ep%x%s-blk max %02x\n", _ep->name, ep->num,
		     tmp, desc->bEndpointAddress & USB_DIR_IN ? "in" : "out",
		     max);
	sp_udc_set_halt(_ep, 0);

	return 0;
}

static int sp_udc_ep_disable(struct usb_ep *_ep)
{
	struct sp_ep *ep = to_sp_ep(_ep);
	u32 int_udll_ie;
	u32 int_udn_ie;

	DEBUG_DBG(">>> sp_udc_ep_disable...\n");

	if (!_ep || !ep->desc) {
		DEBUG_ERR("%s not enabled\n", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	DEBUG_DBG("ep_disable: %s\n", _ep->name);
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
	case EP2:
		int_udll_ie &= ~(EP2O_IF);
		break;
	case EP3:
		int_udll_ie &= ~(EP3I_IF);
		break;
	case EP5:
		int_udll_ie &= ~(EP5I_IF);
		DEBUG_DBG("event will stop %x\n", udc_read(UDVDMA_CRCR));
		udc_write(2, UDVDMA_CRCR);	/*stop ring*/
		g_flag_ep5 = 1;
#ifdef ISO_DEBUG_INFO
		ep_iso_debug_dcache(trb_event,EP5);
#endif
		ep5_use = 0;
		ep5_e = 0;
		kfree(trb_c_start);
		kfree(trb_event);
		break;
	case EP7:
		int_udll_ie &= ~(EP7I_IF);
		break;
	case EP8:
		int_udn_ie &= ~(EP8I_IF);
		break;
	case EP9:
		int_udn_ie &= ~(EP9O_IF);
		break;
	case EP10:
		break;
	case EP11:
		int_udn_ie &= ~(EP11O_IF);

		if (udc_read(UDEPBDMACS) & DMA_EN) {
			dma_len = 0;
			dma_flag_b = 0;
			udc_write(udc_read(UDEPBDMACS) | DMA_FLUSH, UDEPBDMACS);
			while (!(udc_read(UDEPBDMACS) & DMA_FLUSHEND)) {
				DEBUG_DBG("wait dma 11 flush\n");
			}
		}
		break;
	case EP12:
		udc_write(2, UDEPCDMA_CRCR);	/*stop ring*/
		g_flag_ep12 = 1;
		ep12_use = 0;
		ep12_e = 0;
#ifdef ISO_DEBUG_INFO
		ep_iso_debug_dcache(trb_event_12,EP12);
#endif
		kfree(trb_c_start_12);
		kfree(trb_event_12);
		break;
	default:
		return -EINVAL;
	}

	udc_write(int_udll_ie, UDLIE);
	udc_write(int_udn_ie, UDNBIE);

	DEBUG_NOTICE("%s disabled\n", _ep->name);
	DEBUG_DBG("<<< sp_udc_ep_disable...\n");
	return 0;
}

static struct usb_request *sp_udc_alloc_request(struct usb_ep *_ep,
						    gfp_t mem_flags)
{
	struct sp_request *req;

	if (!_ep)
		return NULL;

	req = kzalloc(sizeof(struct sp_request), mem_flags);
	if (!req)
		return NULL;

	req->req.dma = DMA_ADDR_INVALID;
	INIT_LIST_HEAD(&req->queue);
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
		dma_flag = 0;
		udc_write(udc_read(UDEP2DMACS) | DMA_FLUSH, UDEP2DMACS);
		while (!(udc_read(UDEP2DMACS) & DMA_FLUSHEND)) {
			tmp++;
			if (tmp > DMA_FLUSH_TIMEOUT_DELAY) {
				DEBUG_INFO("##");
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
	DEBUG_INFO(">>> vbusInt_handle...\n");
	/*host present */
	if (udc_read(UDCCS) & VBUS) {
		/*IF soft discconect ->force connect */
#if 0
		if (udc_read(UDLCSET) & SOFT_DISC)
			udc_write(udc_read(UDLCSET) & 0xFE, UDLCSET);
#endif
	} else {		/*host absent */
		/*soft connect ->force disconnect */
		if (!(udc_read(UDLCSET) & SOFT_DISC))
			udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
		clearHwState_UDC();
	}

	DEBUG_INFO("<<< vbusInt_handle...\n");
	return 0;
}

static int sp_udc_readep0s_fifo_crq(struct usb_ctrlrequest *crq)
{
	unsigned char *outbuf = (unsigned char *)crq;

	DEBUG_INFO("read ep0 fifi crq ,len= %d\n", udc_read(UDEP0DC));
	memcpy((unsigned char *)outbuf, (char *)(UDEP0SDP + base_addr), 4);
	mb();
	memcpy((unsigned char *)(outbuf + 4), (char *)(UDEP0SDP + base_addr), 4);

	return 8;
}

static int sp_udc_write_ep0_fifo(struct sp_ep *ep,
				     struct sp_request *req);
static int sp_udc_read_ep0_fifo(struct sp_ep *ep,
				    struct sp_request *req);

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
	udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);

	return 0;
}

static void sp_udc_handle_ep0s_idle(struct sp_udc *dev,
					struct sp_ep *ep,
					struct usb_ctrlrequest *crq, u32 ep0csr)
{
	int len;
	int ret;
#ifdef CONFIG_USB_SUNPLUS_OTG
	struct usb_phy *otg_phy;
#endif

	DEBUG_INFO(">>>sp_udc_handle_ep0s_idle...\n");
	/* start control request? */
	sp_udc_nuke(dev, ep, -EPROTO);

	len = sp_udc_readep0s_fifo_crq(crq);
	DEBUG_INFO("len  = %d\n", len);
	if (len != sizeof(*crq)) {
		DEBUG_ERR("setup begin: fifo READ ERROR"
			  " wanted %d bytes got %d. Stalling out...\n",
			  sizeof(*crq), len);
		udc_write((udc_read(UDLCSTL) | SETEP0STL), UDLCSTL);	/* error send stall;*/
		return;
	}

	DEBUG_DBG("bRequest = %x bRequestType %x, %x,%x, wLength = %d\n",
		  crq->bRequest, crq->bRequestType, crq->wValue, crq->wIndex,
		  crq->wLength);

	/* cope with automagic for some standard requests. */
	dev->req_std = (crq->bRequestType & USB_TYPE_MASK)
	    == USB_TYPE_STANDARD;
	dev->req_config = 0;
	dev->req_pending = 1;

	switch (crq->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		DEBUG_INFO("start get descriptor after bus reset\n");
		bus_reset_finish_flag = true;
		{
			u32 DescType = ((crq->wValue) >> 8);
			if (DescType == 0x1) {
				if (udc_read(UDLCSET) & CURR_SPEED) {
					DEBUG_DBG("DESCRIPTOR SPeed = USB_SPEED_FULL\n");
					dev->gadget.speed = USB_SPEED_FULL;
					bulkep_dma_block_size = FULL_SPEED_DMA_SIZE;
				} else {
					DEBUG_DBG("DESCRIPTOR SPeed = USB_SPEED_HIGH\n");
					dev->gadget.speed = USB_SPEED_HIGH;
					bulkep_dma_block_size = HIGH_SPEED_DMA_SIZE;
				}
			}
		}
		break;
	case USB_REQ_SET_CONFIGURATION:
		DEBUG_INFO(" ******* USB_REQ_SET_CONFIGURATION ... \n");
		dev->req_config = 1;
		udc_write(udc_read(UDLCADDR) | (crq->wValue) << 16, UDLCADDR);
		is_config = 1;
		break;

	case USB_REQ_SET_INTERFACE:
		DEBUG_INFO("***** USB_REQ_SET_INTERFACE **** \n");
		dev->req_config = 1;
		if (intface_d) {
			crq->wValue = 0;	/*interface namber */
			crq->wIndex = 0;	/*altersetting */
		}
#ifdef CONFIG_USB_DEVICE_LOSE_PACKET_AFTER_SET_INTERFACE_WORKAROUND
		udc_write(udc_read(UDLCADDR) | (1 << 18), UDLCADDR);
#endif
		break;

	case USB_REQ_SET_ADDRESS:
		DEBUG_INFO("USB_REQ_SET_ADDRESS ... \n");
		break;

	case USB_REQ_GET_STATUS:
		DEBUG_INFO("USB_REQ_GET_STATUS ... \n");
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
		break;

	case USB_REQ_SET_FEATURE:
#ifdef CONFIG_USB_SUNPLUS_OTG
		if((0 == crq->bRequestType) && (3 == crq->wValue) && (0 == crq->wIndex) && (0 == crq->wLength)){
			DEBUG_DBG("set hnp featrue\n");

	#ifdef CONFIG_GADGET_USB0
			otg_phy = usb_get_transceiver_sp(0);
	#else
			otg_phy = usb_get_transceiver_sp(1);
	#endif

			if (!otg_phy) {
				DEBUG_NOTICE("Get otg control fail\n");
			} else {
				sp_accept_b_hnp_en_feature(otg_phy->otg);
			}
		}
#endif

		break;

	default:
		break;
	}

	if (crq->bRequestType & USB_DIR_IN) {
		dev->ep0state = EP0_IN_DATA_PHASE;
	} else {
		dev->ep0state = EP0_OUT_DATA_PHASE;
		/*udc_write(CLR_EP0_OUT_VLD, UDEP0CS);*/
		DEBUG_NOTICE("ep0 fifo %x\n", udc_read(UDEP0CS));
		udc_write(0, UDEP0CS);
	}
	/* deliver the request to the gadget driver */
	ret = dev->driver->setup(&dev->gadget, crq);	/*android_setup composite_setup*/

	if (crq->bRequest == USB_REQ_SET_ADDRESS) {

		ret = 0;
		dev->ep0state = EP0_IDLE;
	}

	if (ret < 0) {
		if (dev->req_config) {
			DEBUG_ERR("config change %02x fail %d?\n",
				  crq->bRequest, ret);
			return;
		}

		if (ret == -EOPNOTSUPP)
			DEBUG_ERR("Operation not supported\n");
		else
			DEBUG_ERR("dev->driver->setup failed. (%d)\n", ret);

		udelay(5);
		udc_write(0x1, UDLCSTL);	/*set ep0 stall*/
		dev->ep0state = EP0_IDLE;
	} else if (dev->req_pending) {
		DEBUG_INFO("dev->req_pending... what now?\n");
		dev->req_pending = 0;
		/*MSC reset command */
		if (crq->bRequest == 0xff) {
			/*ep1SetHalt = 0;
			   ep2SetHalt = 0;*/
		}
	}

	DEBUG_INFO("ep0state *** %s\n", ep0states[dev->ep0state]);
}

static void sp_udc_handle_ep0s(struct sp_udc *dev)
{
	u32 ep0csr;
	struct sp_ep *ep = &dev->ep[0];
	struct usb_ctrlrequest crq;
	struct usb_composite_dev *cdev = get_gadget_data(&dev->gadget);
	struct usb_request *req_g = NULL;
	struct sp_request *req = NULL;

	if (!cdev) {
		DEBUG_DBG("cdev invalid\n");
		return;
	}
	req_g = cdev->req;
	req = to_sp_req(req_g);
	if (!req) {
		DEBUG_DBG("req invalid\n");
		return;
	}
	ep0csr = udc_read(UDEP0CS);

	switch (dev->ep0state) {
	case EP0_IDLE:
		DEBUG_DBG("EP0_IDLE_PHASE ... what now?\n");
		sp_udc_handle_ep0s_idle(dev, ep, &crq, ep0csr);
		break;

	case EP0_IN_DATA_PHASE:
		DEBUG_DBG("EP0_IN_DATA_PHASE ... what now?\n");
		if (req->req.length != req->req.actual) {
			if(1 == sp_udc_write_ep0_fifo(ep, req)){
				udc_write(udc_read(UDLIE) & (~EP0I_IF), UDLIE);
				udc_write(udc_read(UDEP0CS) & (~EP_DIR), UDEP0CS);
				ep->dev->ep0state = EP0_IDLE;
				DEBUG_DBG("ep0 in done\n");
				sp_udc_done(ep, req, 0);
			}
		} else {
			udc_write(udc_read(UDLIE) & (~EP0I_IF), UDLIE);
			udc_write(udc_read(UDEP0CS) & (~EP_DIR), UDEP0CS);
			ep->dev->ep0state = EP0_IDLE;
			DEBUG_DBG("ep0 in done\n");
			sp_udc_done(ep, req, 0);
		}
		break;
	case EP0_OUT_DATA_PHASE:
		DEBUG_DBG("EP0_OUT_DATA_PHASE *** what now?\n");
		if (req->req.length != req->req.actual) {
			if(1 == sp_udc_read_ep0_fifo(ep, req)){
				udc_write(udc_read(UDLIE) & (~EP0O_IF), UDLIE);
				udc_write(udc_read(UDEP0CS) | EP_DIR, UDEP0CS);
				ep->dev->ep0state = EP0_IDLE;
				sp_udc_done(ep, req, 0);
				DEBUG_NOTICE("****ep0 out done\n");
			}
		} else {
			udc_write(udc_read(UDLIE) & (~EP0O_IF), UDLIE);
			udc_write(udc_read(UDEP0CS) | EP_DIR, UDEP0CS);
			ep->dev->ep0state = EP0_IDLE;
			sp_udc_done(ep, req, 0);
			DEBUG_NOTICE("****ep0 out done\n");
		}
		break;

	case EP0_END_XFER:
		DEBUG_INFO("EP0_END_XFER ... what now?\n");
		dev->ep0state = EP0_IDLE;
		break;

	case EP0_STALL:
		DEBUG_INFO("EP0_STALL ... what now?\n");
		dev->ep0state = EP0_IDLE;
		break;
	}
}

static inline int sp_udc_read_fifo_packet(int fifo, u8 * buf, int length,
					      int regoffset)
{
	int n = 0;
	int m = 0;
	int i = 0;
	char testbuf[4];

	n = length / 4;
	m = length % 4;
	writel(0xF, base_addr + regoffset);
	for (i = 0; i < n; i++) {
		*((u32 *) (buf + (i * 4))) =
		    *(u32 *) (base_addr + fifo);
	}
	if (m > 0) {
		udc_write(((1 << m) - 1), regoffset);
		memset(testbuf, 0, 4);
		memcpy((char *)testbuf, (char *)(base_addr + fifo), 4);
		memcpy((char *)(buf + (i * 4)), (char *)testbuf, m);
	}
	return length;
}

static inline int sp_udc_write_packet(int fifo, struct sp_request *req,
					  unsigned max, int offset)
{
	unsigned len = min(req->req.length - req->req.actual, max);
	u8 *buf = req->req.buf + req->req.actual;
	int n = 0;
	int m = 0;
	int i = 0;

	n = len / 4;
	m = len % 4;

	if (n > 0) {
		udc_write(0xF, offset);
		for (i = 0; i < n; i++) {
			*(u32 *) (base_addr + fifo) =
			    *((u32 *) (buf + (i * 4)));
		}
	}

	if (m > 0) {
		udc_write(((1 << m) - 1), offset);
		memcpy((char *)(base_addr + fifo), (char *)(buf + n * 4), 4);
	}

	return len;
}

static inline int sp_udc_read_fifo0_packet(int fifo, u8 * buf, int length,
					       int regoffset)
{
	int n = 0;
	int m = 0;
	int i = 0;
	char testbuf[4];

	n = length / 4;
	m = length % 4;

	writel(0xF, base_addr + regoffset);
	for (i = 0; i < n; i++) {
		*((u32 *) (buf + (i * 4))) =
		    *(u32 *) (base_addr + fifo);
	}
	if (m > 0) {
		udc_write(((1 << m) - 1), regoffset);
		memset(testbuf, 0, 4);
		memcpy((char *)testbuf, (char *)(base_addr + fifo), 4);
		memcpy((char *)(buf + (i * 4)), (char *)testbuf, m);
	}

	return length;
}

static int sp_udc_write_ep0_fifo(struct sp_ep *ep,
				     struct sp_request *req)
{
	unsigned count;
	int is_last;
	u32 idx;

	idx = ep->bEndpointAddress & 0x7F;
	if (idx != 0) {
		DEBUG_ERR("write ep0 idx error\n");
		return -1;
	}

	udc_write(EP0_DIR | CLR_EP0_OUT_VLD, UDEP0CS);
	count = sp_udc_write_packet(UDEP0DP, req, ep->ep.maxpacket, UDEP0VB);
	udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
	udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
	req->req.actual += count;
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length == req->req.actual && !req->req.zero)
		is_last = 1;
	else
		is_last = 0;
	DEBUG_DBG("Written ep%d %d.%d of %d b [last %d,z %d]\n", idx, count,
		  req->req.actual, req->req.length, is_last, req->req.zero);

	return is_last;
}

static int sp_udc_read_ep0_fifo(struct sp_ep *ep,
				    struct sp_request *req)
{
	u8 *buf;
	unsigned bufferspace, count;
	u32 idx;
	u8 ep0_len = 64;
	unsigned avail;
	int is_last;

	if (!req->req.length)
		return 1;
	idx = ep->bEndpointAddress & 0x7F;
	buf = req->req.buf + req->req.actual;
	bufferspace = req->req.length - req->req.actual;
	if (!bufferspace) {
		DEBUG_ERR("%s: buffer full!\n", __func__);
		return -1;
	}
	DEBUG_DBG("\tread before %x\n", udc_read(UDEP0CS));
	udc_write(udc_read(UDEP0CS) & (~EP0_DIR), UDEP0CS);	/*read direction*/

	ep0_len = udc_read(UDEP0DC);
	DEBUG_DBG("******** maxpacketsize is %d length:%d data in ep0 buff:%d\n",
	     ep->ep.maxpacket, req->req.length, ep0_len);
	if (ep0_len > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = ep0_len;
	if (req->req.length > 0) {
		count = sp_udc_read_fifo0_packet(UDEP0DP, buf, avail, UDEP0VB);
		udc_write(udc_read(UDEP0CS) | CLR_EP0_OUT_VLD, UDEP0CS);
		DEBUG_DBG("\t%x\n", udc_read(UDEP0CS));
		req->req.actual += count;
		if (count != ep->ep.maxpacket)
			is_last = 1;
		else if (req->req.length == req->req.actual && !req->req.zero)
			is_last = 1;
		else
			is_last = 0;
	} else {
		is_last = 0;
		count = 0;
		req->req.actual = 0;
	}
	PrintBlock_usb(buf, 4);

	if (!req->req.actual && !count)
		is_last = 0;
	/* Only ep0 debug messages are interesting */
	if (idx == 0) {
		DEBUG_DBG("Read ep%d read:%d actual:%d length:%d[last %d,z %d]\n",
		     idx, count, req->req.actual, req->req.length, is_last,
		     req->req.zero);
	}
	if (is_last) {
		udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
		udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
	}
	return is_last;
}

#ifdef EP2_ENABLE
static int sp_udc_ep2_bulkout_pio(struct sp_ep *ep,
				      struct sp_request *req,
				      int is_pingbuf)
{
	u8 *buf;
	u32 count, avail;
	int is_last = 0;

	DEBUG_INFO(">>>%x %s...%p len=%d actual=%d\n", udc_read(UDEP2FS),
		   __FUNCTION__, req->req.buf, req->req.length,
		   req->req.actual);

	buf = req->req.buf + req->req.actual;
	count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEP2PIC);
	if (!count) {
		if ((udc_read(UDEP2FS) & 0x6) == 0x6) {
			udc_write(udc_read(UDEP12C) | CLR_EP_OVLD, UDEP12C);
			DEBUG_DBG("clear ep2 0\n");
		}
		DEBUG_DBG("rx count == 0 is_last = %d %x\n", is_last,
			  udc_read(UDEP2FS));
		if (req->req.actual) {
			DEBUG_DBG("rx 0 len packet end req!\n");
#ifndef LEN_PARSE		/*not define for adb */
			is_last = 1;
			sp_udc_done(ep, req, 0);
#endif
		}
		return is_last;
	}

	if (count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = count;

	/*read data form ping or pong buffer according to UDEP12PPC[2] */
	sp_udc_read_fifo_packet(UDEP2FDP, buf, avail, UDEP2VB);
	DEBUG_DBG("read after %x %x :", udc_read(UDEP2FS), udc_read(UDEP12C));
	DEBUG_DBG("%2x %2x %2x %2x\n", buf[0], buf[1], buf[2], buf[3]);
	udc_write(udc_read(UDEP12C) | CLR_EP_OVLD, UDEP12C);

	if (req->req.actual == 0) {
		if (is_ean)
			nth_len = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7]) + 8;
		else
			nth_len = ((struct usb_cdc_ncm_nth16 *)buf)->wBlockLength;
		DEBUG_DBG("\t\t nth_len=%d\n", nth_len);
	}
	req->req.actual += avail;

	if (count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (count != avail) {
			req->req.status = -EOVERFLOW;
			avail -= count;
			DEBUG_NOTICE("[%s:%d], overflow [%d]\n", __FUNCTION__,
				     __LINE__, avail);
		}
	} else if (req->req.length <= req->req.actual
		   || avail < ep->ep.maxpacket)
		is_last = 1;
	else {
		is_last = 0;
	}
	DEBUG_DBG("len=%d act=%d count=%d is_last=%d\n", req->req.length,
		  req->req.actual, count, is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
	}

	return is_last;
}
#endif
static int sp_udc_ep9_bulkout_pio(struct sp_ep *ep,
				      struct sp_request *req,
				      int is_pingbuf)
{
	u8 *buf;
	u32 count;
	u32 avail;
	int is_last = 0;

	DEBUG_INFO(">>>ep9_bulkout_pio...%p len=%d actual=%d\n", req->req.buf,
		   req->req.length, req->req.actual);

	buf = req->req.buf + req->req.actual;
	count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEP9PIC);
	if (!count) {
		if ((udc_read(UDEP9FS) & 0x6) == 0x6)
			udc_write(udc_read(UDEP89C) | CLR_EP_OVLD, UDEP89C);
		DEBUG_DBG("rx count == 0 is_last = %d\n", is_last);
		if (req->req.actual) {
			DEBUG_DBG("rx 0 len packet end req!\n");
#ifndef LEN_PARSE		/*not define for adb */
			is_last = 1;
			sp_udc_done(ep, req, 0);
#endif
		}
		return is_last;
	}

	if (count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = count;

	/*read data form ping or pong buffer according to UDEP12PPC[2] */
	sp_udc_read_fifo_packet(UDEP9FDP, buf, avail, UDEP9VB);
/*   DEBUG_DBG("read after %x %x :",udc_read(UDEP9FS),udc_read(UDEP89C));
      DEBUG_DBG("%2x %2x %2x %2x\n",buf[0],buf[1],buf[2],buf[3]);*/
	udc_write(udc_read(UDEP89C) | CLR_EP_OVLD, UDEP89C);

	req->req.actual += avail;
	if (count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (count != avail) {
			req->req.status = -EOVERFLOW;
			avail -= count;
			DEBUG_NOTICE("[%s:%d], overflow [%d]\n", __FUNCTION__,
				     __LINE__, avail);
		}
	} else if (req->req.length <= req->req.actual
		   || avail < ep->ep.maxpacket)
		is_last = 1;
	else {
		is_last = 0;
	}
	DEBUG_DBG("len=%d act=%d count=%d is_last=%d\n", req->req.length,
		  req->req.actual, count, is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
	}

	return is_last;
}

static int sp_udc_ep11_bulkout_pio(struct sp_ep *ep,
				       struct sp_request *req,
				       int is_pingbuf)
{
	u8 *buf;
	u32 count;
	u32 avail;
	int is_last = 0;

	DEBUG_INFO(">>>%x %s...%p len=%d actual=%d\n", udc_read(UDEPBFS),
		   __FUNCTION__, req->req.buf, req->req.length,
		   req->req.actual);

	buf = req->req.buf + req->req.actual;
	count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEPBPIC);
	if (!count) {
		if ((udc_read(UDEPBFS) & 0x6) == 0x6) {
			udc_write(udc_read(UDEPABC) | CLR_EP_OVLD, UDEPABC);
			DEBUG_DBG("clear ep11 0\n");
		}
		DEBUG_DBG("rx count == 0 is_last = %d %x\n", is_last,
			  udc_read(UDEPBFS));
		if (req->req.actual) {
			DEBUG_DBG("rx 0 len packet end req!\n");
#ifndef LEN_PARSE		/*not define for adb */
			is_last = 1;
			sp_udc_done(ep, req, 0);
#endif
		}
		return is_last;
	}

	if (count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = count;

	/*read data form ping or pong buffer according to UDEP12PPC[2] */
	DEBUG_DBG("count=%d avail=%d\n", count, avail);
	sp_udc_read_fifo_packet(UDEPBFDP, buf, avail, UDEPBVB);
	DEBUG_DBG("read after %x %x :", udc_read(UDEPBFS), udc_read(UDEPABC));
	DEBUG_DBG("count=%d avail=%d\n", count, avail);
	udc_write(udc_read(UDEPABC) | CLR_EP_OVLD, UDEPABC);

	if (req->req.actual == 0) {
		if (is_ean)
			nth_len = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7]) + 8;
		else
			nth_len = ((struct usb_cdc_ncm_nth16 *)buf)->wBlockLength;
		DEBUG_DBG("\t\t nth_len=%d\n", nth_len);
	}
#if defined(USBTEST_ZERO) || defined(USB_MASS_STORAGE)
	//nth_len = req->req.length;
#endif
	req->req.actual += avail;

	if (count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (count != avail) {
			req->req.status = -EOVERFLOW;
			avail -= count;
			DEBUG_NOTICE("[%s:%d], overflow [%d]\n", __FUNCTION__,
				     __LINE__, avail);
		}
	} else if (req->req.length <= req->req.actual
		   || avail < ep->ep.maxpacket || nth_len == req->req.actual)
		is_last = 1;
	else {
		is_last = 0;
	}
	DEBUG_DBG("len=%d act=%d count=%d is_last=%d\n", req->req.length,
		  req->req.actual, count, is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
	}

	return is_last;
}

#ifdef EP1_DMA
static int sp_udc_ep1_bulkin_dma(struct sp_ep *ep,
				     struct sp_request *req)
{
	u8 *buf;
	int dma_xferlen;
	int actual_length = 0;

	DEBUG_DBG(">>>%s\n", __FUNCTION__);
	if (req->req.dma == DMA_ADDR_INVALID) {
		req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, req->req.buf, dma_len_ep1, (ep->bEndpointAddress & USB_DIR_IN)
				   ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
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
	DEBUG_DBG("req->req.actual = %d %x\n", req->req.actual,
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
#endif
static int sp_udc_ep12_bulkout_dma(struct sp_ep *ep,
				       struct sp_request *req);
#ifdef EP2_DMA
static int sp_udc_ep2_bulkout_pio_with_dma(struct sp_ep *ep,
					       struct sp_request *req,
					       int is_pingbuf)
{
	u8 *buf;
	u32 count;
	u32 avail;
	int is_last = 0;
	struct usb_cdc_ncm_nth16 *pnth_header;

	DEBUG_INFO(">>>%x sp3502_udc_ep2_bulkout_pio...%p len=%d actual=%d\n",
		   udc_read(UDEP2FS), req->req.buf, req->req.length,
		   req->req.actual);
	buf = req->req.buf + req->req.actual;
	count = sp_udc_get_ep_fifo_count(is_pingbuf, UDEP2PIC);
	if (!count) {
		if ((udc_read(UDEP2FS) & 0x6) == 0x6) {
			udc_write(udc_read(UDEP12C) | CLR_EP_OVLD, UDEP12C);
			DEBUG_DBG("clear ep2 0\n");
		}
		DEBUG_DBG("rx count == 0 is_last = %d\n", is_last);
		if (req->req.actual) {
			DEBUG_DBG("rx 0 len packet end req!\n");
		}
		return is_last;
	}

	if (count > ep->ep.maxpacket)
		avail = ep->ep.maxpacket;
	else
		avail = count;

	sp_udc_read_fifo_packet(UDEP2FDP, buf, avail, UDEP2VB);
	udc_write(udc_read(UDEP12C) | CLR_EP_OVLD, UDEP12C);
	if (req->req.actual == 0) {
		pnth_header = (struct usb_cdc_ncm_nth16 *)buf;
		if (is_ean)
			nth_len = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7]) + 8;
		else
			nth_len = ((struct usb_cdc_ncm_nth16 *)buf)->wBlockLength;
		DEBUG_DBG("\t\t nth_len=%d\n", nth_len);
	}
#ifdef USBTEST_ZERO
	/*nth_len = req->req.length;*/
	nth_len = 1536;
	req->req.length = 1536;
#endif

#ifndef USBTEST_ZERO
	if (!req->req.actual) {
		if (!is_ean && *(u32 *) buf != ZERO_TEST_TAG) {
			DEBUG_NOTICE("note wei\n");
			return 0;
		}
	}
#endif

	req->req.actual += avail;
	DEBUG_DBG("ep2 fifo status %x act=%d", udc_read(UDEP2FS),
		  req->req.actual);
	if (count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (count != avail) {
			req->req.status = -EOVERFLOW;
			avail -= count;
			DEBUG_ERR("[%s:%d], overflow [%d]\n", __FUNCTION__,
				  __LINE__, avail);
		}
	} else if (req->req.length <= req->req.actual
		   || avail < ep->ep.maxpacket)
		is_last = 1;
	else {
		is_last = 0;
		if (nth_len == req->req.actual) {
			is_last = 1;
		}
	}
	DEBUG_DBG("len=%d nth_len=%d act=%d count=%d is_last=%d\n",
		  req->req.length, nth_len, req->req.actual, count, is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
		return 1;
	}

	dma_len = nth_len - bulkep_dma_block_size - (nth_len % bulkep_dma_block_size);
	DEBUG_DBG("dma_len=%d %d\n", dma_len, bulkep_dma_block_size);
	if (dma_len / bulkep_dma_block_size) {
		if (!sp_udc_ep12_bulkout_dma(ep, req))
			return -1;
		if (req->req.length == req->req.actual
		    || (req->req.actual == nth_len)) {
			/*PrintBlock_usb(req->req.buf,req->req.actual);*/
			sp_udc_done(ep, req, 0);
			return 1;
		}
	}

	return is_last;
}
#endif
static int sp_udc_ep11_bulkout_dma(struct sp_ep *ep,
				       struct sp_request *req);

static int sp_udc_ep11_bulkout_pio_with_dma(struct sp_ep *ep,
						struct sp_request *req,
						int is_pingbuf)
{
	u8 *buf;
	u32 count;
	u32 avail;
	u32 count2;
	int is_last = 0;

	DEBUG_INFO(">>>%x %s...%p len=%d actual=%d\n", udc_read(UDEPBFS),
		   __FUNCTION__, req->req.buf, req->req.length,
		   req->req.actual);
	buf = req->req.buf + req->req.actual;
	{
		count = sp_udc_get_ep_fifo_count((udc_read(UDEPBPPC) & CURR_BUFF), UDEPBPIC);
		count2 = sp_udc_get_ep_fifo_count(!(udc_read(UDEPBPPC) & CURR_BUFF), UDEPBPIC);
		if (!count) {
			if ((udc_read(UDEPBFS) & 0x6) == 0x6) {
				udc_write(udc_read(UDEPABC) | CLR_EP_OVLD, UDEPABC);
				DEBUG_DBG("clear ep11 0\n");
			}
			DEBUG_DBG("rx count == 0 is_last = %d\n", is_last);
			DEBUG_DBG("is_ncm = %d \n", is_ncm);
			if (req->req.actual) {
				DEBUG_DBG("rx 0 len packet end req!\n");
			}
			return is_last;
		}

		if (count > ep->ep.maxpacket)
			avail = ep->ep.maxpacket;
		else
			avail = count;

		sp_udc_read_fifo_packet(UDEPBFDP, buf, avail, UDEPBVB);
		udc_write(udc_read(UDEPABC) | CLR_EP_OVLD, UDEPABC);
	}
	if (req->req.actual == 0) {
		if (is_ean)
			nth_len = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7]) + 8;
		else
			nth_len = ((struct usb_cdc_ncm_nth16 *)buf)->wBlockLength;
		if (nth_len == 16384)
			DEBUG_NOTICE("nth = 16k\n");
		DEBUG_DBG("\t\t count %d nth_len=%d count2 %d %d\n", count,
			  nth_len, count2, is_pingbuf);
	}
#ifdef USBTEST_ZERO
	nth_len = req->req.length;
#endif

#ifndef USBTEST_ZERO
	if (0) {
		if (!is_ean && *(u32 *) buf != ZERO_TEST_TAG) {
			/*udc_write(udc_read(UDEPBPPC) | 1, UDEPBPPC);
			dma_fail = 1;*/
			PrintBlock_usb(buf, avail);
			DEBUG_ERR("**********note wei\n");
			return 0;
		}
	}
#endif

	req->req.actual += avail;
	DEBUG_DBG("ep11 fifo status %x act=%d", udc_read(UDEPBFS),
		  req->req.actual);
	if (count < ep->ep.maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (count != avail) {
			req->req.status = -EOVERFLOW;
			avail -= count;
			DEBUG_ERR("[%s:%d], overflow [%d]\n", __FUNCTION__,
				  __LINE__, avail);
		}
	} else if (req->req.length <= req->req.actual
		   || avail < ep->ep.maxpacket)
		is_last = 1;
	else {
		is_last = 0;
		if (nth_len == req->req.actual) {
			is_last = 1;
		}
	}
	DEBUG_DBG("len=%d nth_len=%d act=%d count=%d is_last=%d\n",
		  req->req.length, nth_len, req->req.actual, count, is_last);
	if (is_last) {
		sp_udc_done(ep, req, 0);
		return 1;
	}

	dma_len = nth_len - bulkep_dma_block_size - (nth_len % bulkep_dma_block_size);
	DEBUG_DBG("dma_len=%d %d\n", dma_len, bulkep_dma_block_size);
	if (dma_len / bulkep_dma_block_size) {
		if (!sp_udc_ep11_bulkout_dma(ep, req))
			return -1;
		if (req->req.length == req->req.actual
		    || (req->req.actual == nth_len)) {
			sp_udc_done(ep, req, 0);
			return 1;
		}
	}

	return is_last;
}

#ifdef EP1_DMA
static int sp_ep1_bulkin_dma(struct sp_ep *ep,
				 struct sp_request *req)
{
	u32 count;
	int is_last = 0;

	DEBUG_DBG(">>>%s\n", __FUNCTION__);
	if (dma_len_ep1) {
		return 0;
	}

	udc_write(udc_read(UDLIE) & (~EP1I_IF), UDLIE);

	if (req->req.actual)
		goto _TX_BULK_IN_DATA;
	/* DMA Mode */
	dma_len_ep1 = req->req.length - (req->req.length % bulkep_dma_block_size);
	if (dma_len_ep1 == bulkep_dma_block_size) {
		dma_len_ep1 = 0;
		goto _TX_BULK_IN_DATA;
	}
	if (dma_len_ep1) {
		DEBUG_DBG("ep1 bulk in dma mode,zero=%d\n", req->req.zero);
		if (!sp_udc_ep1_bulkin_dma(ep, req))
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
			udc_write(udc_read(UDLIE) & (~EP1I_IF), UDLIE);

			udc_write(EP1N_IF, UDLIF);
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
	udc_write(EP1N_IF, UDLIF);
	if (!(udc_read(UDEP12FS) & 0x1)) {
		count = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
	} else {
		DEBUG_DBG("A?? %x\n", udc_read(UDEP12FS));
		if (!(udc_read(UDEP12FS) & 0x1))
			goto _TX_BULK_IN_DATA;
		return is_last;
	}
	/*if(count < ep->ep.maxpacket){
	   udc_write(EP1I_IF, UDLIF);
	   } */
	udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
	req->req.actual += count;
	DEBUG_DBG("ep1 buf=%p count=%d len=%d\n", req->req.buf, count,
		  req->req.length);
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length == req->req.actual && !req->req.zero)
		is_last = 1;
	else
		is_last = 0;

done_dma:
	if (is_last) {
		sp_udc_done(ep, req, 0);
	} else if (!(udc_read(UDEP12FS) & 0x1))
		goto _TX_BULK_IN_DATA;

	udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
	DEBUG_DBG(" <<<%s...\n", __FUNCTION__);
	return is_last;
}
#endif
static int sp_ep1_bulkin(struct sp_ep *ep, struct sp_request *req)
{
	u32 count;
	int is_last = 0;
	int is_ping;

	DEBUG_DBG(">>>%s\n", __FUNCTION__);
	udc_write(udc_read(UDLIE) & (~EP1I_IF), UDLIE);

	/* PIO Mode */
_TX_BULK_IN_DATA:
	udc_write(EP1N_IF, UDLIF);
	if (!(udc_read(UDEP12FS) & EP_IVLD)) {
		is_ping = udc_read(UDEP12PPC) & CURR_BUFF;
		count = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
	} else {
		DEBUG_DBG("1?? %x %d\n", udc_read(UDEP12FS), sp_udc_get_ep_fifo_count(udc_read(UDEP12PPC) & CURR_BUFF, UDEP12PIC));
		if ((!(udc_read(UDEP12FS) & 0x1)) && !(sp_udc_get_ep_fifo_count(udc_read(UDEP12PPC) & CURR_BUFF, UDEP12PIC))) {
			DEBUG_DBG("note ep1_bulkin...\n");
			goto _TX_BULK_IN_DATA;
		}
		udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
		return is_last;
	}
	udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
	req->req.actual += count;
	DEBUG_DBG("ep1 act=%d count=%d len=%d %d ep:%x\n", req->req.actual,
		  count, req->req.length, udc_read(UDEP12FS),
		  udc_read(UDEP2FS));
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length == req->req.actual && !req->req.zero)
		is_last = 1;
	else
		is_last = 0;

	if (is_last) {
		sp_udc_done(ep, req, 0);
	} else
	    if (!sp_udc_get_ep_fifo_count
		(udc_read(UDEP12PPC) & CURR_BUFF, UDEP12PIC))
		goto _TX_BULK_IN_DATA;

	udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
	DEBUG_DBG(" <<< %s...\n", __FUNCTION__);
	return is_last;
}

static int sp_ep8_bulkin(struct sp_ep *ep, struct sp_request *req)
{
	u32 count;
	int is_last = 0;
	int is_ping;

	DEBUG_DBG(">>> sp_ep8_bulkin...\n");
	udc_write(udc_read(UDNBIE) & (~EP8I_IF), UDNBIE);

	/* PIO Mode */
_TX_BULK_IN8_DATA:
	/*udc_write(EP8N_IF, UDNBIE);*/
	if (!(udc_read(UDEP89FS) & EP_IVLD)) {
		is_ping = udc_read(UDEP89PPC) & CURR_BUFF;
		count = sp_udc_write_packet(UDEP89FDP, req, ep->ep.maxpacket, UDEP89VB);
	} else {
		DEBUG_DBG("8?? %x %d\n", udc_read(UDEP89FS), sp_udc_get_ep_fifo_count(udc_read(UDEP89PPC) & CURR_BUFF, UDEP89PIC));
		if ((!(udc_read(UDEP89FS) & 0x1)) && !(sp_udc_get_ep_fifo_count(udc_read(UDEP89PPC) & CURR_BUFF, UDEP89PIC))) {
			DEBUG_DBG(">>> note \n");
			goto _TX_BULK_IN8_DATA;
		}
		return is_last;
	}
	udc_write(udc_read(UDEP89C) | SET_EP_IVLD, UDEP89C);
	req->req.actual += count;
	DEBUG_DBG("ep8 act=%d count=%d len=%d %x\n", req->req.actual, count,
		  req->req.length, udc_read(UDEP89FS));
	if (count != ep->ep.maxpacket)
		is_last = 1;
	else if (req->req.length == req->req.actual && !req->req.zero)
		is_last = 1;
	else
		is_last = 0;

	if (is_last) {
		sp_udc_done(ep, req, 0);
	} else if ((!(udc_read(UDEP89FS) & 0x1))
		   && (is_ping != (udc_read(UDEP89PPC) & CURR_BUFF)))
		goto _TX_BULK_IN8_DATA;

	udc_write(udc_read(UDNBIE) | EP8I_IF, UDNBIE);
	DEBUG_DBG(" <<< sp_ep8_bulkin...\n");
	return is_last;
}

static int sp_udc_ep12_bulkout_dma(struct sp_ep *ep,
				       struct sp_request *req)
{
	u8 *buf;
	int actual_length = 0;
	int cur_length = dma_len;
	int dma_xferlen;
	int is_error = 0;
	unsigned long t;

	udc_write(udc_read(UDCIE) & (~EP2_DMA_IF), UDCIE);
	dma_flag = 2;
	DEBUG_DBG(">>> sp_udc_ep12_bulkout_dma...\n");
	DEBUG_DBG("12length=%d 12actual=%d\n", req->req.length,
		  req->req.actual);

	if (req->req.dma == DMA_ADDR_INVALID) {
		req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, (u8 *) req->req.buf + req->req.actual /*+ bulkep_dma_block_size */ ,
				   cur_length + 512,	/*512 for debug dma*/
				   (ep->bEndpointAddress & USB_DIR_IN)
				   ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}
	/*if(!(udc_read(UDEP2DMACS) & DMA_FLUSHEND))*/
	DEBUG_DBG("%x cur_length=%d actual_length=%d\n", udc_read(UDEP2DMACS),
		  cur_length, actual_length);
	while (actual_length < cur_length) {
		/*sp_udc_dma_int_en(USB_DMA_EP2_BULK_OUT, 1);*/
		buf = (u8 *) (req->req.dma /*+ req->req.actual - bulkep_dma_block_size */  + actual_length);
		dma_xferlen = min(cur_length - actual_length, (int)UDC_FLASH_BUFFER_SIZE);
		DEBUG_DBG("bulkout dma_xferlen = %d dma_addr %p req buf %p %x\n", dma_xferlen, buf, (u8 *) req->req.buf, udc_read(UDEP2DMACS));
		udc_write(udc_read(UDEP2DMACS) | DMA_COUNT_ALIGN | DMA_WRITE | dma_xferlen, UDEP2DMACS);
		udc_write((u32) buf, UDEP2DMADA);
		/*udc_write(udc_read(UDCIE) | CIE_DMA_IE , UDCIE);*/
		udc_write(udc_read(UDEP2DMACS) | DMA_EN, UDEP2DMACS);
		/*clear flag */
		/*udc_write(EP12C_ENABLE_BULK, UDC_EP12C_OFST);*/
		DEBUG_DBG("************Wait DMA Finish***************** %x\n",
			  udc_read(UDEP2DMACS));

		t = jiffies;
		if ((udc_read(UDEP2DMACS) & DMA_EN)
		    || (!(udc_read(UDCIF) & EP2_DMA_IF))
		    || (udc_read(UDEP2DMACS) & 0x3fffff)) {
			udc_write(udc_read(UDCIE) | EP2_DMA_IF, UDCIE);
			if (!(udc_read(UDEP2DMACS) & DMA_EN))
				DEBUG_ERR("dma%d staus %x\n", ep->num,
					  udc_read(UDEP2DMACS));
			return 0;
		}
		udc_write(EP2_DMA_IF, UDCIF);
		while ((udc_read(UDEP2DMACS) & DMA_EN) != 0) {
			if (time_after(jiffies, t + 10 * HZ)) {	/*超时处理*/
				req->req.status = -ECONNRESET;
				sp_udc_done(ep, req, -ECONNRESET);
				is_error = 1;
				DEBUG_ERR("cur_length=%d req->req.actual=%d\n",
					  cur_length, req->req.actual);
				DEBUG_ERR("[%s:%d] DMA Error [0x%x]\n",
					  __FUNCTION__, __LINE__,
					  udc_read(UDEP2DMACS));
				break;
			}
			/*if((udc_read(UDEP2FS)&0x22) == 0x20)
			      udc_write(udc_read(UDEP2PPC) | SWITCH_BUFF ,UDEP2PPC);*/
		}
		DEBUG_DBG("EP2_DMA_IF = %x\n", udc_read(UDCIF) & EP2_DMA_IF);
		udc_write(EP2_DMA_IF, UDCIF);
		DEBUG_DBG("DMA_EN disable\n");
		actual_length += dma_xferlen;
		DEBUG_DBG("disable\n");
	}

	DEBUG_DBG("cur_length=%d actual_length=%d\n", cur_length,
		  actual_length);
	req->req.actual += actual_length;
	cur_length -= actual_length;
	DEBUG_DBG("cur_length=%d req->req.actual=%d\n", cur_length,
		  req->req.actual);

	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma,
				 cur_length, (ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}
	DEBUG_DBG("<<< sp_udc_ep12_bulkout_dma...\n");

	DEBUG_DBG("DD %x\n", udc_read(UDEP2DMACS));
	dma_flag = 0;
	dma_len = 0;
	return 1;
}

static int sp_udc_ep11_bulkout_dma(struct sp_ep *ep,
				       struct sp_request *req)
{
	u8 *buf;
	int actual_length = 0;
	int cur_length = dma_len;
	int dma_xferlen;
	int is_error = 0;
	unsigned long t;

	udc_write(udc_read(UDCIE) & (~EPB_DMA_IF), UDCIE);
	dma_flag_b = 11;
	DEBUG_DBG("11length=%d 11actual=%d\n", req->req.length,
		  req->req.actual);
	if (req->req.dma == DMA_ADDR_INVALID) {
		req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, (u8 *) req->req.buf + req->req.actual ,
				   cur_length,
				   (ep->bEndpointAddress & USB_DIR_IN)
				   ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}

	DEBUG_DBG("cur_length=%d actual_length=%d\n", cur_length,
		  actual_length);
	while (actual_length < cur_length) {
		buf = (u8 *) (req->req.dma + actual_length);
		dma_xferlen = min(cur_length - actual_length, (int)UDC_FLASH_BUFFER_SIZE);
		/*DEBUG_DBG("bulkout dma_xferlen = %d %d dma_addr %p req buf %p %x\n", dma_xferlen,nth_len, buf, (u8 *) req->req.buf, udc_read(UDEPBDMACS));
		DEBUG_DBG("bulkout dma_xferlen = %d dma_addr %p req buf %p %x\n", dma_xferlen, buf, (u8 *) req->req.buf, udc_read(UDEPBDMACS));
		udc_write(udc_read(UDEPBDMACS) | DMA_COUNT_ALIGN | DMA_WRITE | dma_xferlen, UDEPBDMACS);*/
		udc_write((u32) buf, UDEPBDMADA);
		if ((udc_read(UDEPBDMACS) & 0xff000000) == 0x58000000)
			DEBUG_DBG("dma --staus 0x%x\n", udc_read(UDEPBDMACS));
		/*udc_write(udc_read(UDEPBDMACS) | DMA_EN, UDEPBDMACS);*/
		udc_write((udc_read(UDEPBDMACS) & (~DMA_COUNT_MASK)) |
			  DMA_COUNT_ALIGN | DMA_WRITE | dma_xferlen | DMA_EN,
			  UDEPBDMACS);
		/*clear flag */
		/*verB DMA bug*/
		if (!down_trylock(&ep11_sw_sem)) {
			if ((udc_read(UDEPBFS) & 0x22) == 0x20)
				udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
			up(&ep11_sw_sem);
		}
		DEBUG_DBG("dma staus %x\n", udc_read(UDEPBDMACS));
		DEBUG_DBG("************Wait DMA Finish***************** %x\n",
			  udc_read(UDEPBDMACS));
		t = jiffies;
#ifdef SW_IRQ
		if (!is_ncm && ((udc_read(UDEPBDMACS) & DMA_EN)
				|| (!(udc_read(UDCIF) & EPB_DMA_IF))
				|| (udc_read(UDEPBDMACS) & 0x3fffff))) {
			udc_write(udc_read(UDCIE) | EPB_DMA_IF, UDCIE);
#ifdef MANUAL_EP11
			udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
			/*if((udc_read(UDEPBFS)&0x22) == 0x20){
			   udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF ,UDEPBPPC);
			   } */
#endif
			if (!(udc_read(UDEPBDMACS) & DMA_EN))
				DEBUG_ERR("dma%d staus %x\n", ep->num,
					  udc_read(UDEPBDMACS));
			return 0;
		}
#endif
		if (is_ncm && ((udc_read(UDEPBDMACS) & DMA_EN)
			       || (!(udc_read(UDCIF) & EPB_DMA_IF))
			       || (udc_read(UDEPBDMACS) & 0x3fffff))) {
			udc_write(udc_read(UDCIE) | EPB_DMA_IF, UDCIE);

			if (!(udc_read(UDEPBDMACS) & DMA_EN))
				DEBUG_DBG("dma%d staus %x\n", ep->num,
					  udc_read(UDEPBDMACS));
			return 0;
		}
		while ((udc_read(UDEPBDMACS) & DMA_EN) != 0) {
			if (time_after(jiffies, t + 10 * HZ)) {	/*超时处理*/
				req->req.status = -ECONNRESET;
				sp_udc_done(ep, req, -ECONNRESET);
				is_error = 1;
				DEBUG_ERR("cur_length=%d req->req.actual=%d\n",
					  cur_length, req->req.actual);
				DEBUG_ERR("[%s:%d] DMA Error [0x%x]\n",
					  __FUNCTION__, __LINE__,
					  udc_read(UDEPBDMACS));
				break;
			}
		}
		DEBUG_DBG("EP11_DMA_IF = %x\n", udc_read(UDCIF) & EPB_DMA_IF);
		udc_write(EPB_DMA_IF, UDCIF);
		actual_length += dma_xferlen;
	}

	req->req.actual += actual_length;
	cur_length -= actual_length;

	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma,
				 cur_length, (ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}
	DEBUG_DBG("DD %x\n", udc_read(UDEPBDMACS));
	dma_flag_b = 0;
	dma_len = 0;
	return 1;
}

static int sp_ep2_bulkout_dma(struct sp_ep *ep,
				  struct sp_request *req)
{
	int is_pingbuf = 0;
	int ret = 0;
	udc_write(udc_read(UDLIE) & (~EP2O_IF), UDLIE);
	udc_write(udc_read(UDLIE) & (~EP2N_IF), UDLIE);

	nth_len = req->req.length;
	dma_len = nth_len;
	if (!sp_udc_ep12_bulkout_dma(ep, req))
		return 0;
	dma_flag = 0;
	sp_udc_done(ep, req, 0);
	is_pingbuf = 0;
	ret = 0;

	udc_write(udc_read(UDLIE) | EP2O_IF, UDLIE);
	udc_write(udc_read(UDLIE) | EP2N_IF, UDLIE);
	DEBUG_DBG("<<<%s...\n", __FUNCTION__);
	return 1;
}

static int sp_ep11_bulkout_dma(struct sp_ep *ep,
				   struct sp_request *req)
{
	int is_pingbuf = 0;
	int ret = 0;
	udc_write(udc_read(UDNBIE) & (~EP11O_IF), UDNBIE);
	udc_write(udc_read(UDNBIE) & (~EP11N_IF), UDNBIE);

_RX_BULKOUT_DATA_AGAIN:
#ifdef MANUAL_EP11
	if (!is_ncm && !(udc_read(UDEPBFS) & 0x22)) {
		DEBUG_DBG("ep11 fifo not valid %x\n", udc_read(UDEPBFS));
		udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
		udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
		return 0;
	}
	if (!is_ncm && !down_trylock(&ep11_sw_sem)) {
		/*if (!sp_udc_get_ep_fifo_count((udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0, UDEPBPIC) && !(udc_read(UDEPBFS) & 0x2))
		      udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);*/
		if ((udc_read(UDEPBFS) & 0x22) == 0x20) {
			udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
		}
		up(&ep11_sw_sem);
	}
#endif
	is_pingbuf = (udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0;
	if (req->req.actual == 0
	    || ((nth_len - req->req.actual) < bulkep_dma_block_size)) {
		ret = sp_udc_ep11_bulkout_pio_with_dma(ep, req, is_pingbuf);
		if (ret == 0) {
			if (udc_read(UDEPBFS) & 0x22 && !dma_len)
				goto _RX_BULKOUT_DATA_AGAIN;
			else if (dma_len) {
				DEBUG_DBG("bb dma=%d\n", dma_len);
				return 0;
			}
			udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);

			DEBUG_DBG("<<< sp3502_ep11_bulkout @#$ ...\n");
			return 0;
		} else if (ret == -1) {
			return 0;
		}
	} else {
		DEBUG_DBG("why len=%d act=%d dma_len=%d count=%d\n", nth_len,
			  req->req.actual, dma_len,
			  sp_udc_get_ep_fifo_count(is_pingbuf, UDEPBPIC));
		return 0;
	}
	udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
	udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
	DEBUG_DBG("<<<%s...\n", __FUNCTION__);
	return 1;
}

static int sp_ep9_bulkout_not_auto(struct sp_ep *ep,
				       struct sp_request *req)
{
	int is_pingbuf = 0;

	udc_write(udc_read(UDNBIE) & (~EP9O_IF), UDNBIE);
	udc_write(udc_read(UDNBIE) & (~EP9N_IF), UDNBIE);

_RX_BULKOUT_DATA_AGAIN_9:
	if ((!sp_udc_get_ep_fifo_count(0, UDEP9PIC)
	     && !sp_udc_get_ep_fifo_count(1, UDEP9PIC))
	    && !(udc_read(UDEP9FS) & 0x22)) {
		DEBUG_NOTICE("ep2 fifo not valid\n");
		udc_write(udc_read(UDNBIE) | EP9O_IF, UDNBIE);
		udc_write(udc_read(UDNBIE) | EP9N_IF, UDNBIE);
		return 0;
	}
	is_pingbuf = (udc_read(UDEP9PPC) & CURR_BUFF) ? 1 : 0;
	if (!sp_udc_get_ep_fifo_count(is_pingbuf, UDEP9PIC)
	    && !(udc_read(UDEP9FS) & 0x2))
		udc_write(udc_read(UDEP9PPC) | SWITCH_BUFF, UDEP9PPC);
	if (sp_udc_ep9_bulkout_pio(ep, req, is_pingbuf) == 0) {

		DEBUG_INFO(">>>>9 _RX_BULKOUT_DATA_AGAIN <<<<\n");
		if (udc_read(UDEP9FS) & 0x22)
			goto _RX_BULKOUT_DATA_AGAIN_9;
		udc_write(udc_read(UDNBIE) | EP9O_IF, UDNBIE);
		udc_write(udc_read(UDNBIE) | EP9N_IF, UDNBIE);
		DEBUG_DBG("<<< %s RET 0\n", __FUNCTION__);
		return 0;
	}
	udc_write(udc_read(UDNBIE) | EP9O_IF, UDNBIE);
	udc_write(udc_read(UDNBIE) | EP9N_IF, UDNBIE);
	DEBUG_DBG("<<< %s...\n", __FUNCTION__);
	return 1;
}

static int sp_ep11_bulkout(struct sp_ep *ep, struct sp_request *req)
{
	int is_pingbuf = 0;

	udc_write(udc_read(UDNBIE) & (~EP11O_IF), UDNBIE);
	udc_write(udc_read(UDNBIE) & (~EP11N_IF), UDNBIE);

_RX_BULKOUT_DATA_AGAIN_11:
	if (!(udc_read(UDEPBFS) & 0x2)) {
		DEBUG_DBG("ep11 fifo not valid\n");
#ifdef CONFIG_USB_DEVICE_EP11_NOT_AUTO_SWITCH_WORKAROUND
		if ((udc_read(UDEPBFS) & 0x22) == 0x20) {
			udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
		} else
#endif
		{
			udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
			return 0;
		}
	}
	is_pingbuf = (udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0;
	if (sp_udc_ep11_bulkout_pio(ep, req, is_pingbuf) == 0) {

		DEBUG_INFO(">>>>11 _RX_BULKOUT_DATA_AGAIN <<<<\n");
		if (udc_read(UDEPBFS) & 0x22)
			goto _RX_BULKOUT_DATA_AGAIN_11;
		udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
		udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
		DEBUG_DBG("<<< %s***\n", __FUNCTION__);
		return 0;
	}
	udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
	udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
	DEBUG_DBG("<<< %s...\n", __FUNCTION__);
	return 1;
}

static int sp_ep7_iso_in(struct sp_ep *ep, struct sp_request *req)
{
	u32 *trb;
	struct iso_trb *trb_s;
	DEBUG_DBG("ep7 iso %d %d\n", req->req.length, req->req.actual);
	/*allocate the memory for all rings */
	trb = kzalloc(ALL_TRB_SIZE, GFP_ATOMIC);	/*don't forget kfree */
	DEBUG_DBG("00 UDADMA_CRCR=%x %p\n", udc_read(UDADMA_CRCR), trb);
	trb = (u32 *) (((u32) (trb)) & (~0x3f));
	udc_write(2, UDADMA_CRCR);
	DEBUG_DBG("01 UDADMA_CRCR=%x %p\n", udc_read(UDADMA_CRCR), trb);
	udc_write((((u32) (trb)) & RCS_0), UDADMA_CRCR);
	udc_write((u32) trb, UDADMA_ERBAR);
	udc_write(((u32) trb) + 4, UDADMA_ERDPR);

	trb_s = (struct iso_trb *)trb;
	/* TODO:should be dma addr*/
	if (req->req.dma == DMA_ADDR_INVALID) {
		req->req.dma = dma_map_single(ep->dev->gadget.dev.parent, (u8 *) req->req.buf, req->req.length, (ep->bEndpointAddress & USB_DIR_IN)
				   ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}
	trb_s->ptr = req->req.dma;	/*NOTE:should DMA addr */
	trb_s->size_cc = 16;
	trb_s->cmd = RCS_0 | TRB_NORMAL | TRB_IN;

	udc_write(DMA_EN | 1, UDADMA_RCSR);
	DEBUG_DBG("UDADMA_RTR will trig %x\n", udc_read(UDADMA_RTR));

	udc_write(1, UDADMA_RTR);
	return 0;
}

static void fill_trb(struct iso_trb *trb, u32 buf, u32 size, u32 flag)
{
	trb->ptr = __pa(buf);
	trb->size_cc = size;
	trb->cmd = flag;
}

int fifo_size_ep5 = 32;
u32 cnt;
u8 *d_p;
u8 mo = 0;
#define CRP_MASK	(0xffffffc0)
#define CRP_MASK_	(0xfc0)

static int sp_ep5_iso_in(struct sp_ep *ep, struct sp_request *req)
{
	int i = 0;
	int count = 0;
	u32 toggle;
	struct sp_request *req_5;
	int crcr_i;
	struct iso_trb *trb_s;

	if (!g_flag_ep5) {	/*debug*/
		g_flag_ep5++;
		DEBUG_DBG("gadget audio debug\n");
		return 0;
	}
	/*udc_write(DMA_EN, UDVDMA_RCSR);*/
	DEBUG_DBG("ep5 iso %d %d\n", req->req.length, req->req.actual);
	if (g_flag_ep5 == 1) {
		cnt = 0;
		udc_write((((u32) (__pa(trb_c))) | RCS_1), UDVDMA_CRCR);
		DEBUG_DBG("02 CRCR=%x trb_event %lu\n", udc_read(UDVDMA_CRCR),
			  (unsigned long)__pa(trb_event));
		udc_write(__pa(trb_event), UDVDMA_ERBAR);
		udc_write(__pa(trb_event + 4 * (TRB_NUM - 1)), UDVDMA_ERDPR);
		g_flag_ep5++;
	}
#ifdef ISO_DEBUG_INFO

	if (!req->req.actual) {
		u32 *p = (u32 *) req->req.buf;
		for (i = 0; i < req->req.length / 4; i++) {
			*p++ = cnt++;
		}
	}
	udc_clean_dcache_range((u32)req->req.buf,
			      (u32)__pa(req->req.buf),
			      req->req.length);

#endif

ep5_start:
	trb_s = (struct iso_trb *)trb_c;
	toggle = ((trb_s->cmd) & RCS_1) ? 0 : RCS_1;
	i = ((u32 *) trb_s - trb_c_start) / 4;
	count = ep5_use;
	crcr_i = (udc_read(UDVDMA_CRCR) & CRP_MASK_) / TRB_SIZE;
	if (crcr_i <= i)
		ep5_use = i - crcr_i;
	else
		ep5_use = (TRB_NUM - 0) - (crcr_i - i);
	DEBUG_DBG("before i %d  used %d %d\n", i, ep5_use, crcr_i);
	if (ep5_use == 0 && (udc_read(UDVDMA_CRCR) & CRR))
		ep5_use = (TRB_NUM - 0);
	else if (g_flag_ep5 == 2 && ep5_use == (TRB_NUM - 0)
		 && !(udc_read(UDVDMA_CRCR) & CRR))
		ep5_use = 0;

	count = count - ep5_use;

	if (sp_udc_list_empty(&ep_iso_5.queue, &memory.ep[5].lock)) {
		req_5 = NULL;
		DEBUG_DBG("ep5 queue iso is NULL!!!!!!!!!!!!\n");
	} else
		req_5 = list_entry(ep_iso_5.queue.next, struct sp_request, queue);
	ep_iso_5.act += fifo_size_ep5 * count;
	if (req_5 && ep_iso_5.act >= req_5->req.length) {
		ep_iso_5.act -= req_5->req.length;
		req_5->req.status = -EINPROGRESS;
		sp_udc_done(&memory.ep[5], req_5, 0);

	}

	if (ep5_use >= (TRB_NUM - 0)) {
		DEBUG_DBG("not fill trb ep5_use=%d %p %x\n", ep5_use, trb_s,
			  udc_read(UDVDMA_CRCR));
		goto fill_done0;
	}
	DEBUG_DBG("toggle=%d ep5_use %d i%d\n", toggle, ep5_use, i);
	while (i < (TRB_NUM - 0) && ep5_use < (TRB_NUM - 0) && req->req.actual < req->req.length) {
		trb_s->rfu = 1;
		count = min((int)(req->req.length - req->req.actual), fifo_size_ep5);
		if (i % 16 == 14)
			fill_trb(trb_s, (u32) (req->req.buf + req->req.actual),
				 count, toggle | TRB_NORMAL | TRB_IN | TRB_IOC);
		else
			fill_trb(trb_s, (u32) (req->req.buf + req->req.actual),
				 count, toggle | TRB_NORMAL | TRB_IN);
		trb_s++;
		req->req.actual += count;
		i++;
		ep5_use++;
	}
	DEBUG_DBG("i %d  used %d\n", i, ep5_use);

	trb_c = (u32 *) trb_s;
	if (i == (TRB_NUM - 0)) {
		trb_c = trb_c_start;
		toggle |= TRB_TC;
		trb_s->rfu = 1;
		fill_trb(trb_s, (u32) trb_c, 0, toggle | TRB_LINK | TRB_IN);
		DEBUG_DBG("trb i=%d toggle %d\n", i + 1, toggle);
	}
	if (trb_c == trb_c_start && ep5_use < (TRB_NUM - 0)
	    && req->req.actual < req->req.length) {
		goto ep5_start;
	}
	DEBUG_DBG("trb_c %p  trb_c_start %p\n", trb_c, trb_c_start);

fill_done0:
	udc_clean_dcache_range((u32)trb_c_start, __pa(trb_c_start),
			      ALL_TRB_SIZE);
	if (udc_read(UDVDMA_CRCR) & CRR)
		goto fill_done;
	/*if (g_flag_ep5 == 2){
		DEBUG_DBG("fill trb num\n");*/
	g_flag_ep5++;
	udc_write(DMA_EN | (TRB_NUM - 0), UDVDMA_RCSR);
	DEBUG_DBG("UDADMA_RTR will trig %x\n", udc_read(UDVDMA_RTR));

	udc_write(1, UDVDMA_RTR);
	DEBUG_DBG("%x\n", udc_read(UDVDMA_RTR));
	DEBUG_DBG("%x\n", udc_read(UDVDMA_RTR));
	DEBUG_DBG("hw %x %x %x %x \n", udc_read(0xb0), udc_read(0xb4),
		  udc_read(0xb8), udc_read(0xbc));
	DEBUG_DBG("UDADMA_RTR after trig %x CRCR %x\n", udc_read(UDVDMA_RTR),
		  udc_read(UDVDMA_CRCR));
	DEBUG_DBG("act may %x len %x \n", req->req.actual, req->req.length);
fill_done:
	if (req->req.actual >= req->req.length) {
		DEBUG_DBG("ep5 done=1\n");
		ep_iso_done(ep, req, 1);
		return 1;
	} else {
		DEBUG_DBG("ep5 done=0\n");
		return 0;
	}
}

static int sp_epc_iso_out(struct sp_ep *ep, struct sp_request *req)
{
	int i = 0;
	int count = 0;
	u32 toggle;
	struct iso_trb *trb_s;

	DEBUG_DBG("epc iso l %d a %d\n", req->req.length, req->req.actual);
	if (!g_flag_ep12) {	/*debug*/
		g_flag_ep12++;
		DEBUG_NOTICE("epc gadget debug\n");
		return 0;
	}
#ifdef ISO_DEBUG_INFO
	if (!req->req.actual) {
		memset(req->req.buf, 0xfa, req->req.length);
		udc_clean_dcache_range((u32)req->req.buf,
				      __pa(req->req.buf), req->req.length);
	}
#endif
	if (g_flag_ep12 == 1) {
		udc_write((((u32) (__pa(trb_c_12))) | RCS_1), UDEPCDMA_CRCR);
		DEBUG_DBG("02 CRCR=%x trb_event %lu\n", udc_read(UDEPCDMA_CRCR),
			  (unsigned long)__pa(trb_event_12));
		udc_write(__pa(trb_event_12), UDEPCDMA_ERBAR);
		udc_write(__pa(trb_event_12), UDEPCDMA_ERDPR);
		g_flag_ep12++;
	}
ep12_start:
	trb_s = (struct iso_trb *)trb_c_12;
	toggle = ((trb_s->cmd) & RCS_1) ? 0 : RCS_1;
	i = ((u32 *) trb_s - trb_c_start_12) / 4;
	ep12_use = (((u32) trb_s + 0x1000 - (udc_read(UDEPCDMA_CRCR) & CRP_MASK)) / TRB_SIZE) % TRB_NUM;
	if (!ep12_use && (udc_read(UDEPCDMA_CRCR) & CRR))
		ep12_use = 256;
	if (ep12_use >= 255) {
		DEBUG_DBG("not fill trb ep12_use=%d %p %x\n", ep12_use, trb_s,
			  udc_read(UDEPCDMA_CRCR));
		goto fill_done0;
	}
	DEBUG_DBG("i %d toggle=%d used %d\n", i, toggle, ep12_use);
	while (i < 255 && ep12_use < 255 && req->req.actual < req->req.length) {
		trb_s->rfu = 1;
		count = min((int)(req->req.length - req->req.actual), (int)EPC_FIFO_SIZE);
		if (i % 16 == 14)
			fill_trb(trb_s, (u32) (req->req.buf + req->req.actual),
				 count, toggle | TRB_NORMAL | TRB_IOC);
		else
			fill_trb(trb_s, (u32) (req->req.buf + req->req.actual),
				 count, toggle | TRB_NORMAL);
		trb_s++;
		req->req.actual += count;
		i++;
		ep12_use++;
	}
	DEBUG_DBG("i %d  used %d\n", i, ep12_use);
	trb_c_12 = (u32 *) trb_s;
	if (i == 255) {
		trb_c_12 = trb_c_start_12;
		toggle |= TRB_TC;
		trb_s->rfu = 1;
		fill_trb(trb_s, (u32) trb_c_12, 0, toggle | TRB_LINK);
		DEBUG_DBG("i=%d toggle %d\n", i + 1, toggle);
	}
	if (trb_c_12 == trb_c_start_12 && ep12_use < 255
	    && req->req.actual < req->req.length) {
		goto ep12_start;
	}
	DEBUG_DBG("trb_c %p  trb_c_start %p\n", trb_c_12, trb_c_start_12);
fill_done0:
	udc_clean_dcache_range((u32)trb_c_start_12,
			      __pa(trb_c_start_12), ALL_TRB_SIZE);
	if (udc_read(UDEPCDMA_CRCR) & CRR)
		goto fill_done;
	udc_write(DMA_EN | (TRB_NUM - 1), UDEPCDMA_RCSR);
	DEBUG_DBG("UDADMA_RTR will trig %x\n", udc_read(UDEPCDMA_RTR));
	DEBUG_DBG("UDADMA_RTR after trig %x CRCR %x\n", udc_read(UDEPCDMA_RTR),
		  	udc_read(UDEPCDMA_CRCR));
	DEBUG_DBG("act may %x len %x \n", req->req.actual, req->req.length);
fill_done:
	if (req->req.actual == req->req.length) {
		DEBUG_DBG("epC done=1\n");
		ep_iso_done(ep, req, 1);
		return 1;
	} else {
		DEBUG_DBG("epC done=0\n");
		return 0;
	}
}

#if 0
static void event_ring_show(void)
{
	struct iso_trb *trb_event;

	trb_event = (struct iso_trb *)udc_read(UDVDMA_ERDPR);
	DEBUG_DBG("trb %x\n", *((u32 *) (__va(trb_event))));
}
#endif

#ifdef INT_TEST
static void iso_complete(struct usb_ep *ep, struct usb_request *req);
#endif
static int sp_ep3_int_in(struct sp_ep *ep, struct sp_request *req)
{
	int count;
#ifdef INT_TEST
	static int vary_len = 0;
	vary_len++;
	if (vary_len == 65)
		vary_len = 1;
	ep->ep.maxpacket = vary_len;	/* int test */
#endif
	count = sp_udc_write_packet(UDEP3DATA, req, ep->ep.maxpacket, UDEP3VB);
	udc_write((1 << 0) | (1 << 3), UDEP3CTRL);
	req->req.actual += count;
	if (req->req.actual == req->req.length)
#ifdef INT_TEST
		iso_complete(&ep->ep, &req->req);
#else
		sp_udc_done(ep, req, 0);
#endif
	return 1;
}

static void ep1_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[1];
	struct sp_request *req;
	int ret = 0;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);
	if (dma_len_ep1)
		return;
	do {
		if (down_trylock(&ep1_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__,
				  __LINE__, udc_read(UDEP12FS));
			udc_write(0, UDEP1INAKCN);
			return;
		}
		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			up(&ep1_ack_sem);
			DEBUG_DBG( "null 1 unlock >>\n");
			req = NULL;
			DEBUG_DBG("ep1 req is NULL!\n");
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);

		if ((udc_read(UDEP12FS) & 0x1) != 0x1 && req) {
#ifdef EP1_DMA
			if (is_vera)
				ret = sp_ep1_bulkin(ep, req);
			else
				ret = sp_ep1_bulkin_dma(ep, req);
#else
			ret = sp_ep1_bulkin(ep, req);
#endif
		}
		up(&ep1_ack_sem);
		DEBUG_DBG("1 unlock >>\n");
	} while (ret && ((udc_read(UDEP12FS) & 0x1) != 0x1));

	DEBUG_DBG("<<< %s\n", __FUNCTION__);
	return;
}

#ifdef CONFIG_USB_SUNPLUS_EP1
static void sp_udc_ep1_work(void)
{
	struct sp_ep *ep = &memory.ep[1];
	struct sp_request *req;
	int ret = 0;

	DEBUG_DBG(">>> %s\n", __FUNCTION__);
	do {
		if (down_trylock(&ep1_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__,
				  __LINE__, udc_read(UDEP12FS));
			udc_write(0, UDEP1INAKCN);
			return;
		}

		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			up(&ep1_ack_sem);
			DEBUG_DBG("null 1 unlock >>\n");
			req = NULL;
			DEBUG_DBG("ep1 req is NULL!\n");
			/*udc_write(udc_read(UDLIE)|EP1I_IF, UDLIE);*/
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);

		if (((udc_read(UDEP12FS) & 0x1) != 0x1) && req) {
#ifdef EP1_DMA
			if (is_vera)
				ret = sp_ep1_bulkin(ep, req);
			else
				ret = sp_ep1_bulkin_dma(ep, req);
#else
			ret = sp_ep1_bulkin(ep, req);
#endif
		}
		up(&ep1_ack_sem);
		DEBUG_DBG("1 unlock >>\n");
	} while (ret && ((udc_read(UDEP12FS) & 0x1) != 0x1));

	DEBUG_DBG("<<< %s\n", __FUNCTION__);
	return;
}

#endif

static void ep2_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[2];
	struct sp_request *req;
	int ret = 0;

	if (dma_flag)
		return;
	do {
		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			req = NULL;
			DEBUG_DBG("\tep2 req is NULL\t!\n");
			udc_write(udc_read(UDLIE) | EP2O_IF, UDLIE);
			udc_write(udc_read(UDLIE) | EP2N_IF, UDLIE);
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);

		DEBUG_DBG("length=0x%x actual=0x%x\n",req->req.length,req->req.actual);
		DEBUG_DBG("ep2 lock >>>\t!\n");
		if (down_trylock(&ep12_bulk_out_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__,
				  __LINE__, udc_read(UDEP2FS));
			return;
		}
		ret = sp_ep2_bulkout_dma(ep, req);	/*test dma use*/
		up(&ep12_bulk_out_ack_sem);
		DEBUG_DBG("\t<<< ep2 unlock! ret=%d\n", ret);
	} while (ret && (udc_read(UDEP2FS) & 0x22));
	return;
}

static void ep9_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[9];
	struct sp_request *req;
	int ret = 0;

	do {
		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			req = NULL;
			DEBUG_DBG("\tep9 req is NULL\t!\n");
			udc_write(udc_read(UDNBIE) | EP9O_IF, UDNBIE);
			udc_write(udc_read(UDNBIE) | EP9N_IF, UDNBIE);
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);

		DEBUG_DBG("length=0x%x actual=0x%x\n",req->req.length,req->req.actual);
		DEBUG_DBG("ep9 lock >>>\t!\n");
		if (down_trylock(&ep9_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__,
				  __LINE__, udc_read(UDEP9FS));
			return;
		}
		ret = sp_ep9_bulkout_not_auto(ep, req);
		up(&ep9_ack_sem);
		DEBUG_DBG("\t<<< ep9 unlock! ret=%d fs %x\n", ret,
			  udc_read(UDEP9FS));
	} while (ret && (udc_read(UDEP9FS) & 0x22));
	return;
}

static void ep11_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[11];
	struct sp_request *req;
	int ret = 0;

	if (dma_flag_b)
		return;
	do {
		DEBUG_DBG("ep11 lock >>>\t!\n");
		if (down_trylock(&ep11_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__, __LINE__, udc_read(UDEPBFS));
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
			udc_write(0, UDEPBONAKCN);
			return;
		}
		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			req = NULL;
			up(&ep11_ack_sem);
			DEBUG_DBG("\tep11 req is NULL\t!\n");
			udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);
		if (dma_fail) {
			ret = sp_ep11_bulkout(ep, req);
		} else {
			ret = sp_ep11_bulkout_dma(ep, req);
		}

		up(&ep11_ack_sem);
		DEBUG_DBG("\t<<< ep11 unlock! ret=%d\n", ret);
	} while (ret && (udc_read(UDEPBFS) & 0x22));
	return;
}

static void sp_udc_ep9_work(struct work_struct *work)
{
	struct sp_udc *udc = the_controller;

	if(udc){
		ep9_handle(udc);
	}

	return;
}



#ifdef CONFIG_USB_SUNPLUS_EP11
static void sp_udc_ep11_work(void)
{
	struct sp_ep *ep = &memory.ep[11];
	struct sp_request *req;
	int ret = 0;

	if (dma_flag_b) {
#ifdef MANUAL_EP11
		if (!is_ncm && !is_vera) {
			if (!down_trylock(&ep11_sw_sem)) {

				if ((udc_read(UDEPBFS) & 0x22) == 0x20) {
					if ((udc_read(UDEPBDMACS) & (UDC_FLASH_BUFFER_SIZE - 1)) > 0)	/*verB DMA bug*/
						udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
				} else {
					/*udc_write(1 ,UDEPBONAKCN);*/
				}
				up(&ep11_sw_sem);
			}
		}
#endif
		return;
	}
	do {
		DEBUG_DBG("ep11 lock >>>\t!\n");
		if (down_trylock(&ep11_ack_sem)) {
			DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__, __LINE__, udc_read(UDEPBFS));
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
			udc_write(0, UDEPBONAKCN);
			return;
		}
		if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
			req = NULL;
			up(&ep11_ack_sem);
			DEBUG_DBG("\tep11 req is NULL\t!\n");
			udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
			udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);
			return;
		} else
			req = list_entry(ep->queue.next, struct sp_request, queue);


		if (dma_fail) {
			ret = sp_ep11_bulkout(ep, req);
		} else {
			ret = sp_ep11_bulkout_dma(ep, req);
		}
		up(&ep11_ack_sem);
		DEBUG_DBG("\t<<< ep11 unlock! ret=%d\n", ret);
	} while (ret && (udc_read(UDEPBFS) & 0x22));
	return;
}
#endif
static void debug_ep_out(void)
{
	struct sp_ep *ep = &memory.ep[9];
	struct sp_request *req;
	int ret = 0;

	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("\tep9 req is NULL\t!\n");
		udc_write(udc_read(UDNBIE) | EP9O_IF, UDNBIE);
		udc_write(udc_read(UDNBIE) | EP9N_IF, UDNBIE);
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);

	DEBUG_DBG("ep9 lock >>>\t!\n");
	if (down_trylock(&ep9_ack_sem)) {
		DEBUG_DBG("[%s:%d] error [0x%x]\n", __FUNCTION__, __LINE__,
			  udc_read(UDEP9FS));
		return;
	}
	ret = (udc_read(UDEP9PPC) & CURR_BUFF) ? 1 : 0;
	DEBUG_DBG("ep out debug %d %x %x\n", ret,
		  sp_udc_get_ep_fifo_count(0, UDEP9PIC),
		  sp_udc_get_ep_fifo_count(1, UDEP9PIC));
	up(&ep9_ack_sem);
	DEBUG_DBG("\t<<< ep9 unlock! ret=%d fs %x\n", ret, udc_read(UDEP9FS));
	return;
}

static void ep1_dma_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[1];
	struct sp_request *req;
	int ret;
	if (down_trylock(&ep1_dma_sem)) {
		DEBUG_ERR("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return;
	}
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		up(&ep1_dma_sem);
		DEBUG_ERR("\tep1_dma req is NULL\t!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	if (req->req.actual != 0) {
		DEBUG_ERR("WHY ep1\n");
		if (req->req.actual != req->req.length) {
			up(&ep1_dma_sem);
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
			ep1_handle(dev);
		}
		DEBUG_DBG("ep1 dma: %d\n", udc_read(UDEP12FS));
		up(&ep1_dma_sem);
		return;
	}
	if (!(udc_read(UDEP12FS) & 0x1)) {
		udc_write(EP1N_IF, UDLIF);
		ret = sp_udc_write_packet(UDEP12FDP, req, ep->ep.maxpacket, UDEP12VB);
		udc_write(udc_read(UDEP12C) | SET_EP_IVLD, UDEP12C);
		req->req.actual += ret;
		sp_udc_done(ep, req, 0);
		DEBUG_DBG("DMA->write fifo by pio count=%d!\n", ret);
	} else {
		DEBUG_DBG("\twait DMA->write fifo by pio!\n\n");
	}
	dma_len_ep1 = 0;
	up(&ep1_dma_sem);
	udc_write(udc_read(UDLIE) | EP1I_IF | EP1N_IF, UDLIE);
	return;
}

static void ep2_dma_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[2];
	struct sp_request *req;

	if (down_trylock(&ep12_bulk_out_nak_sem)) {
		DEBUG_ERR("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return;
	}
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("\tep2_dma req is NULL\t!\n");
		up(&ep12_bulk_out_nak_sem);
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	if (nth_len - req->req.actual < bulkep_dma_block_size) {
		up(&ep12_bulk_out_nak_sem);
		return;
	}
	if (!req->req.actual)
		DEBUG_ERR("why act=0?\n");
	req->req.actual += dma_len;
	dma_len = 0;
	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma, (req->req.length - req->req.actual), (ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}

	udc_write(udc_read(UDCIE) & (~EP2_DMA_IF), UDCIE);
	if (req->req.length == req->req.actual || req->req.actual == nth_len) {
		sp_udc_done(ep, req, 0);
	}
	dma_flag = 0;
	up(&ep12_bulk_out_nak_sem);
	udc_write(udc_read(UDLIE) | EP2O_IF, UDLIE);
	udc_write(udc_read(UDLIE) | EP2N_IF, UDLIE);

	if (udc_read(UDEP2FS) & 0x22) {
		ep2_handle(dev);
	}
	DEBUG_DBG("DMA finished!...\n");
	return;
}

#if 0
static void ep11_dma_work(struct work_struct *work)
{
	struct sp_ep *ep = &memory.ep[11];
	struct sp_request *req;

	if (down_trylock(&ep11_dma_sem)) {
		DEBUG_ERR("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return;
	}
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("\tep11_dma req is NULL\t!\n");
		up(&ep11_dma_sem);
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	if (nth_len - req->req.actual < bulkep_dma_block_size) {
		up(&ep11_dma_sem);
		return;
	}
	if (!req->req.actual)
		DEBUG_ERR("why act=0?\n");

	req->req.actual += dma_len;
	dma_len = 0;
	dma_flag_b = 0;
	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma,
				(req->req.length - req->req.actual),
				(ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		req->req.dma = DMA_ADDR_INVALID;
	}

	udc_write(udc_read(UDCIE) & (~EPB_DMA_IF), UDCIE);
	if (req->req.length == req->req.actual || req->req.actual == nth_len) {
		sp_udc_done(ep, req, 0);
	}

	up(&ep11_dma_sem);
	udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
	udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);

	if (udc_read(UDEPBFS) & 0x22) {
		ep11_handle(NULL);
	}
	DEBUG_DBG("DMA 11 finished!...\n");
	return;
}
#endif

static void ep11_dma_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[11];
	struct sp_request *req;

	DEBUG_INFO("<<< %s\n", __FUNCTION__);
	if (down_trylock(&ep11_dma_sem)) {
		DEBUG_ERR("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return;
	}
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("\tep11_dma req is NULL\t!\n");
		up(&ep11_dma_sem);
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	if (nth_len - req->req.actual < bulkep_dma_block_size) {
		up(&ep11_dma_sem);
		return;
	}
	if (!req->req.actual)
		DEBUG_ERR("why act=0?\n");

	req->req.actual += dma_len;
	if (req->req.dma != DMA_ADDR_INVALID) {
		dma_unmap_single(ep->dev->gadget.dev.parent, req->req.dma, (req->req.length - req->req.actual), (ep->bEndpointAddress & USB_DIR_IN)
				 ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		req->req.dma = DMA_ADDR_INVALID;
	}

	udc_write(udc_read(UDCIE) & (~EPB_DMA_IF), UDCIE);
	if (req->req.length == req->req.actual || req->req.actual == nth_len) {
		sp_udc_done(ep, req, 0);
	}
	dma_len = 0;
	dma_flag_b = 0;
	up(&ep11_dma_sem);
	udc_write(udc_read(UDNBIE) | EP11O_IF, UDNBIE);
	udc_write(udc_read(UDNBIE) | EP11N_IF, UDNBIE);

	if (udc_read(UDEPBFS) & 0x22) {
#ifdef CONFIG_USB_SUNPLUS_EP11
		sp_udc_ep11_work();
#else
		ep11_handle(dev);
#endif
	}
	DEBUG_INFO(">>> %s\n", __FUNCTION__);
	return;
}

static void ep3_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[3];
	struct sp_request *req;
	if (list_empty(&ep->queue)) {
		req = NULL;
		DEBUG_DBG("ep3 req is NULL!!!!!!!!!!!!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	DEBUG_DBG("length=0x%x actual=0x%x \n", req->req.length,
		  req->req.actual);
	sp_ep3_int_in(ep, req);
	return;
}

static void sp_udc_ep3_work(struct work_struct *work)
{
	struct sp_udc *udc = the_controller;

	if(udc){
		ep3_handle(udc);
	}

	return;
}

static void ep5_event_handle(void)
{
	int i = 0;
	struct iso_trb *trb = (struct iso_trb *)trb_event;
	struct sp_request *req;

	if (sp_udc_list_empty(&ep_iso_5.queue, &memory.ep[5].lock)) {
		req = NULL;
		DEBUG_DBG("ep5 queue iso is NULL!!!!!!!!!!!!\n");
	} else
		req = list_entry(ep_iso_5.queue.next, struct sp_request, queue);
	udc_invalidate_dcache_range((u32)trb_event, __pa(trb_event), TRB_NUM * 16);
	do {
		if ((trb[ep5_e].cmd & RCS_1) == tog) {
			ep5_e++;
			i++;
		} else {
			DEBUG_DBG("%d not %x %x\n", ep5_e, tog,
				  (trb[ep5_e].cmd & RCS_1));
		}

		if (ep5_e == (TRB_NUM - 0)) {
			ep5_e = 0;
			if (i)
				tog = tog ? 0 : 1;
		}
	} while (((trb[ep5_e].cmd & RCS_1) == tog) && i < 91);
	DEBUG_DBG("e%d i%d trb event tog %x C %x\n", ep5_e, i, tog,
		  (trb[ep5_e].cmd & RCS_1));

	if (ep5_e)
		udc_write(__pa(trb_event + ep5_e * 4), UDVDMA_ERDPR);
}

static void ep5_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[5];
	struct sp_request *req;

	ep5_event_handle();

	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("ep5 req is NULL!!!!!!!!!!!!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	DEBUG_DBG("ep5_handle length=0x%x actual=0x%x \n", req->req.length,
		  req->req.actual);
	if (sp_udc_list_empty(&ep_iso_5.queue, &memory.ep[5].lock)) {
		DEBUG_DBG("test ep5 queue iso is NULL!!!!!!!!!!!!\n");
	} else
		DEBUG_DBG("test ep5 queue iso\n");
	sp_ep5_iso_in(ep, req);
	return;
}

static void epc_event_handle(void)
{
	struct iso_trb *trb = (struct iso_trb *)trb_event_12;
	struct sp_request *req;
	int i = 0;
	if (list_empty(&ep_iso_12.queue)) {
		req = NULL;
		DEBUG_DBG("epc queue iso is NULL!!!!!!!!!!!!\n");
	} else
		req = list_entry(ep_iso_12.queue.next, struct sp_request, queue);
	udc_invalidate_dcache_range((u32)trb_event_12, __pa(trb_event_12), TRB_NUM * 16);
	do {
		if ((trb[ep12_e].cmd & RCS_1) == tog_12) {
			ep12_e++;
			i++;
			ep_iso_12.act += EPC_FIFO_SIZE;
			if (req && ep_iso_12.act >= req->req.length) {
				ep_iso_12.act = 0;
				req->req.status = -EINPROGRESS;
				udc_invalidate_dcache_range((u32)req->
							   req.buf,
							   __pa(req->req.buf),
							   req->req.length);
				PrintBlock_usb(req->req.buf, req->req.length);
				/*PrintBlock_usb(req->req.buf+512*7, 512);*/
				sp_udc_done(&memory.ep[12], req, 0);
			}
		} else {
			DEBUG_DBG("epC %d not %x %x a %d\n", ep12_e, tog_12,
				  (trb[ep12_e].cmd & RCS_1), ep_iso_12.act);
			if (req)
				DEBUG_DBG("epC buf %p\n", req->req.buf);
		}

		if (ep12_e == TRB_NUM - 1) {
			ep12_e = 0;
			if (i)
				tog_12 = tog_12 ? 0 : 1;
		}
	} while ((trb[ep12_e].cmd & RCS_1) == tog_12);
	DEBUG_DBG("epc e%d trb event tog %x C %x\n", ep12_e, tog_12,
		  (trb[ep12_e].cmd & RCS_1));
	if (ep12_e)
		udc_write(__pa(trb_event_12 + ep12_e * 4), UDEPCDMA_ERDPR);
}

static void epc_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[12];
	struct sp_request *req;
	epc_event_handle();
	if (list_empty(&ep->queue)) {
		req = NULL;
		DEBUG_DBG("epc req is NULL!!!!!!!!!!!!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	DEBUG_DBG("length=0x%x actual=0x%x \n", req->req.length,
		  req->req.actual);
	/*udc_invalidate_dcache_range((u32)req->req.buf, __pa(req->req.buf), req->req.length);
	PrintBlock_usb(req->req.buf,512);*/
	sp_epc_iso_out(ep, req);
	return;
}

#ifdef INT_TEST
static void iso_complete(struct usb_ep *ep, struct usb_request *req)
{
	DEBUG_NOTICE("iso_complete\n");
}

#define ISO_LEN 16384
static struct usb_request *alloc_ep_req(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		req->length = ISO_LEN;
		req->buf = kmalloc(ISO_LEN, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static struct sp_request *new_req(struct sp_ep *ep)
{
	struct usb_request *req;
	unsigned i;
	u8 *buf;
	req = alloc_ep_req(&ep->ep);
	req->complete = iso_complete;

	buf = req->buf;
	for (i = 0; i < req->length; i++)
		*buf++ = (u8) (i % 63);
	return to_sp_req(req);
}
#endif
#ifdef INT_TEST
static void ep3_handle_test(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[3];
	struct sp_request *req;
	if (list_empty(&ep->queue)) {
		DEBUG_ERR("ep3 req is NULL ! create req ! !!!!!!\n");
		req = new_req(ep);
		list_add_tail(&req->queue, &ep->queue);
	}
	req = list_entry(ep->queue.next, struct sp_request, queue);
	DEBUG_DBG("%x length=0x%x actual=0x%x \n", udc_read(UDEP3CTRL),
		  req->req.length, req->req.actual);
	sp_ep3_int_in(ep, req);
	return;
}
#endif

static void ep8_handle(struct sp_udc *dev)
{
	struct sp_ep *ep = &memory.ep[8];
	struct sp_request *req;
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		req = NULL;
		DEBUG_DBG("ep8 req is NULL!\n");
		return;
	} else
		req = list_entry(ep->queue.next, struct sp_request, queue);
	if (udc_read(UDEP89FS) != 0x11)
		sp_ep8_bulkin(ep, req);
	return;
}

static irqreturn_t sp_udc_irq(int irq, void *_dev)
{
	u32 udc_irq_flags;
	u32 udc_irq_en1;
	u32	udc_irq_en2;
	u32 udc_epnb_irq_flags;
	/*u32 udc_epnb_irq_en;*/
	u32 irq_en1_flags;
	u32 irq_en2_flags;
	unsigned long flags;
	struct sp_udc *dev = (struct sp_udc *)_dev;

	spin_lock_irqsave(&dev->lock, flags);

	udc_irq_flags = udc_read(UDCIF);
	udc_irq_en1 = udc_read(UDCIE);
	udc_irq_en2 = udc_read(UDLIF);

	udc_epnb_irq_flags = udc_read(UDNBIF);
	/*udc_epnb_irq_en = udc_read(UDNBIE);*/

	/*DEBUG_DBG(">irq IF:0x%x IE:0x%x udc IF:0x%x IE=0x%x\n",
		udc_read(UDLIF), udc_read(UDLIE), udc_read(UDCIF), udc_read(UDCIE));*/
	irq_en1_flags = udc_irq_flags & udc_irq_en1;
	irq_en2_flags = udc_read(UDLIE) & udc_irq_en2;
	if (irq_en2_flags & RESET_RELEASE_IF) {
		udc_write(RESET_RELEASE_IF, UDLIF);
		DEBUG_NOTICE("reset end irq\n");
	}
	/* force disconnect interrupt */
	if (irq_en1_flags & VBUS_IF) {
		DEBUG_DBG("vbus_irq[%x]\n",udc_read(UDLCSET));
		udc_write(VBUS_IF, UDCIF);
		vbusInt_handle();
	}
	if (irq_en1_flags & VIDEO_TRB_IF) {
		DEBUG_DBG("IRQ:VIDEO_TRB_IF\n");
		udc_write(VIDEO_TRB_IF, UDCIF);
		DEBUG_DBG("CRCR %x base %x dequeue %x\n", udc_read(UDVDMA_CRCR),
			  udc_read(UDVDMA_ERBAR), udc_read(UDVDMA_ERDPR));
		ep5_handle(dev);
	} else if (irq_en1_flags & VIDEO_ERF_IF) {
		DEBUG_DBG("IRQ:ep5 event ring full\n");
		udc_write(VIDEO_ERF_IF, UDCIF);
		DEBUG_DBG("CRCR %x base %x  dequeue %x\n",
			  udc_read(UDVDMA_CRCR), udc_read(UDVDMA_ERBAR),
			  udc_read(UDVDMA_ERDPR));
	}
	if (irq_en2_flags & EPC_OFLOW_IF) {
		udc_write(1, UDEPCDMA_RTR);
		udc_write(EPC_OFLOW_IF, UDLIF);
		DEBUG_DBG("EPC_OFLOW_IF CRCR %x %x %x\n",
			  udc_read(UDEPCDMA_CRCR), udc_read(UDEPCDMA_RCSR),
			  udc_read(UDEPCDMA_RTR));
		udc_write(udc_read(UDLIE) & (~EPC_OFLOW_IF), UDLIE);
	} else if (irq_en2_flags & EPC_SUCC_IF) {
		udc_write(EPC_SUCC_IF, UDLIF);
		udc_write(udc_read(UDLIE) & (~EPC_SUCC_IF), UDLIE);
		DEBUG_DBG("EPC_SUCC_IF CRCR %x %x %x\n",
			  udc_read(UDEPCDMA_CRCR), udc_read(UDEPCDMA_RCSR),
			  udc_read(UDEPCDMA_RTR));
		epc_handle(dev);
	}			/*else */
	if (irq_en1_flags & EPC_TRB_IF) {
		DEBUG_DBG("IRQ:EPC_TRB_IF\n");
		udc_write(EPC_TRB_IF, UDCIF);
		DEBUG_DBG("CRCR %x %x %x\n", udc_read(UDEPCDMA_CRCR),
			  udc_read(UDEPCDMA_RCSR), udc_read(UDEPCDMA_RTR));
		epc_handle(dev);
	} else if (irq_en1_flags & EPC_ERF_IF) {
		DEBUG_DBG("IRQ:EPC event ring full\n");
		udc_write(EPC_ERF_IF, UDCIF);
		DEBUG_DBG("CRCR %x %x %x\n", udc_read(UDEPCDMA_CRCR),
			  udc_read(UDEPCDMA_RCSR), udc_read(UDEPCDMA_RTR));
	}
	if (irq_en2_flags & SUS_IF) {
		DEBUG_NOTICE("IRQ:UDC Suspend Event\n");
		udc_write(SUS_IF, UDLIF);
#ifdef CONFIG_USB_SUNPLUS_OTG
		if(is_config){
			//dev->suspend_sta++;
			//queue_work(dev->qwork_otg, &dev->work_otg);
		}
#endif

		if (dev->driver){
			if (dev->driver->suspend){
				dev->driver->suspend(&dev->gadget);
			}
			if (dev->driver->disconnect){
				dev->driver->disconnect(&dev->gadget);
			}
		}
	}

	if (irq_en2_flags & RESET_IF) {
		DEBUG_NOTICE("reset irq\n");
#ifdef CONFIG_USB_MULTIPLE_RESET_PROBLEM_WORKAROUND
		//udelay(BUS_RESET_FOR_CHIRP_DELAY);
		//udc_write(udc_read(UEP12DMACS) & (~RX_STEP7), UEP12DMACS);	/*control rx signal */
#endif
		/* two kind of reset :
		 * - reset start -> pwr reg = 8
		 * - reset end   -> pwr reg = 0
		 **/
		g_flag_ep5 = 0;
		udc_write((udc_read(UDLCSET) | 8) & 0xFE, UDLCSET);
		udc_write(RESET_IF, UDLIF);
		/*Allow LNK to suspend PHY */
		udc_write(udc_read(UDCCS) & (~UPHY_CLK_CSS), UDCCS);
		clearHwState_UDC();
		is_config = 0;
		in_p_num = 0;
	}

	if (irq_en2_flags & EP0S_IF) {
		udc_write(EP0S_IF, UDLIF);
		DEBUG_NOTICE("ep0 setup int\n");
		if ((udc_read(UDEP0CS) & (EP0_OVLD | EP0_OUT_EMPTY)) ==
		    (EP0_OVLD | EP0_OUT_EMPTY)) {
			udc_write(udc_read(UDEP0CS) | CLR_EP0_OUT_VLD, UDEP0CS);
			/*DEBUG_ERR("why EP0_OVLD|EP0_OUT_EMPTY\n");*/
		}
		while (dev->ep0state != EP0_IDLE){
			DEBUG_DBG("now,dev->ep0state:%d\n",dev->ep0state);
			sp_udc_handle_ep0s(dev);
		}
		sp_udc_handle_ep0s(dev);
	}

	if ((irq_en2_flags & EP0I_IF)) {
		DEBUG_NOTICE("IRQ:UDLC_EP0I_IE\n");
		udc_write(EP0I_IF, UDLIF);
		if (dev->ep0state != EP0_IDLE){
			sp_udc_handle_ep0s(dev);
		}
		else
			udc_write(udc_read(UDEP0CS) & (~EP_DIR), UDEP0CS);
	}
	if ((irq_en2_flags & EP0O_IF)) {
		DEBUG_NOTICE("IRQ:UDLC_EP0O_IE maybe fix\n");
		udc_write(EP0O_IF, UDLIF);
		if (dev->ep0state == EP0_OUT_DATA_PHASE){
			sp_udc_handle_ep0s(dev);
		}
	}
	if (irq_en1_flags & EPB_DMA_IF) {
		DEBUG_DBG("IRQ:UDC ep11 DMA\n");
		udc_write(EPB_DMA_IF, UDCIF);
		if ( /*dma_len&& */ 1)
			ep11_dma_handle(dev);
		/*queue_work(dev->qwork_ep11_dma, &dev->work_ep11_dma);*/

	}else if (irq_en2_flags & EP1_DMA_IF) {	/*dma finish*/
		DEBUG_DBG("IRQ:UDC ep1 DMA\n");
		udc_write(EP1_DMA_IF, UDLIF);
		if (dma_len_ep1)
			ep1_dma_handle(dev);
	}
	if (irq_en1_flags & EP2_DMA_IF) {
		DEBUG_DBG("IRQ:UDC ep2 DMA\n");
		udc_write(EP2_DMA_IF, UDCIF);
		 /*	if ( dma_len)*/
		ep2_dma_handle(dev);
	} else if (irq_en2_flags & EP2O_IF) {
		DEBUG_DBG("IRQ:ep2 out int\n");
		udc_write(EP2O_IF, UDLIF);
		if (udc_read(UDEP2FS) & 0x22)
			ep2_handle(dev);
	} else if (irq_en2_flags & EP1I_IF) {
		DEBUG_DBG("IRQ:ep1 in int\n");
		udc_write(EP1I_IF, UDLIF);
		if (dma_len_ep1) {
			udc_write(udc_read(UDLIE) | (EP1I_IF), UDLIE);
		} else if (!(udc_read(UDEP12FS) & 0x1)) {
#ifdef CONFIG_USB_SUNPLUS_EP1
			sp_udc_ep1_work();
#else
			ep1_handle(dev);
#endif
		}
	} else if (irq_en2_flags & EP3I_IF) {
		DEBUG_DBG("IRQ:ep3 in int\n");
		udc_write(EP3I_IF, UDLIF);
#ifdef INT_TEST
		ep3_handle_test(dev);
#else
		queue_work(dev->qwork_ep3, &dev->work_ep3);
#endif
	} else if (irq_en2_flags & EP7I_IF) {
		DEBUG_DBG("IRQ:ep7 in int\n");
		udc_write(EP7I_IF, UDLIF);

	} else if (irq_en2_flags & EP7_DMA_IF) {
		DEBUG_DBG("IRQ:ep7 DMA in int\n");
		udc_write(EP7_DMA_IF, UDLIF);

	} else if (irq_en2_flags & EP5I_IF) {
		DEBUG_DBG("IRQ:ep5 in int %x %x %x\n", udc_read(UDVDMA_CRCR),
			  udc_read(UDVDMA_RCSR), udc_read(UDVDMA_RTR));
		udc_write(EP5I_IF, UDLIF);
		ep5_handle(dev);
	} else if (irq_en2_flags & EP5_DMA_IF) {
		DEBUG_DBG("IRQ:ep5 DMA in int\n");
		udc_write(EP5_DMA_IF, UDLIF);
	}
	if (irq_en2_flags & EP1N_IF) {
		DEBUG_DBG("IRQ:ep1 in NAK %x\n", udc_read(UDEP12FS));
		udc_write(EP1N_IF, UDLIF);
		udc_write(udc_read(UDLIE) & (~EP1N_IF), UDLIE);

		if (dma_len_ep1) {
			DEBUG_DBG("IRQ:ep1 dma_len_ep1=%d\n", dma_len_ep1);
			udc_write(udc_read(UDLIE) | (EP1I_IF), UDLIE);
		} else if (!(udc_read(UDEP12FS) & 0x1)) {
			DEBUG_DBG("IRQ:ep1 hand\n");
#ifdef CONFIG_USB_SUNPLUS_EP1
			sp_udc_ep1_work();
#else
			ep1_handle(dev);
#endif
		}
	} else if (irq_en2_flags & EP2N_IF) {
		DEBUG_DBG("IRQ:ep2 out NAK %x\n", udc_read(UDEP2FS));
		udc_write(EP2N_IF, UDLIF);
	} else if (udc_epnb_irq_flags & EP8N_IF) {
		DEBUG_DBG("IRQ:ep8 in NAK %x\n", udc_read(UDEP89FS));
		udc_write(EP8N_IF, UDNBIF);
		if (is_config == 1) {
			is_config = 2;
			DEBUG_NOTICE("will create /dev/iap!\n");
			sp_reinit_iap(&memory.ep[9],
					   list_entry((&memory.ep[9])->queue.next,
					   struct sp_request,queue), 0xFF);
		}
	} else if (udc_epnb_irq_flags & EP9N_IF) {
		DEBUG_DBG("IRQ:ep9 out NAK %x\n", udc_read(UDEP9FS));
		udc_write(EP9N_IF, UDNBIF);

	}

	if (udc_epnb_irq_flags & EP8I_IF) {
		DEBUG_DBG("IRQ:ep8 in  %x\n", udc_read(UDEP89FS));
		udc_write(EP8I_IF, UDNBIF);
		ep8_handle(dev);
	}
	if (udc_epnb_irq_flags & EP9O_IF) {
		DEBUG_DBG("IRQ:ep9 OUT %x\n", udc_read(UDEP9FS));
		udc_write(EP9O_IF, UDNBIF);
		if (udc_read(UDEP9FS) & 0x22){
			queue_work(dev->qwork_ep9, &dev->work_ep9);
		}
		else if (udc_read(UDEP9FS) == 0x55)
			debug_ep_out();
	} else if (udc_epnb_irq_flags & EP11O_IF) {
		DEBUG_DBG("IRQ:ep11 out %x\n", udc_read(UDEPBFS));
		udc_write(EP11O_IF, UDNBIF);

		udc_write(udc_read(UDNBIE) & (~EP11O_IF), UDNBIE);
#ifdef CONFIG_USB_SUNPLUS_EP11
		if (dma_len) {
#ifdef MANUAL_EP11
			if (!is_ncm && !is_vera) {
				udc_write(udc_read(UDNBIE) | (EP11O_IF), UDNBIE);
				if (!down_trylock(&ep11_sw_sem)) {

					if ((udc_read(UDEPBFS) & 0x22) == 0x20) {
						DEBUG_DBG("IRQ:sw pipo\n");
						if ((udc_read(UDEPBDMACS) & (UDC_FLASH_BUFFER_SIZE - 1)) > 0)	/*verB DMA bug*/
							udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
					} else {
						udc_write(1, UDEPBONAKCN);
					}
					up(&ep11_sw_sem);
				}
			} else if (is_ncm) {
				sp_udc_ep11_work();
			}
#endif
		} else {
			sp_udc_ep11_work();
		}
#else
		if (udc_read(UDEPBFS) & 0x22)
			ep11_handle(dev);
#endif
	} else if (udc_epnb_irq_flags & EP11N_IF) {
		DEBUG_DBG("IRQ:ep11 nak %x\n", udc_read(UDEPBFS));
		udc_write(EP11N_IF, UDNBIF);

		udc_write(udc_read(UDNBIE) & (~EP11N_IF), UDNBIE);
#ifdef CONFIG_USB_SUNPLUS_EP11
		if (dma_len) {
			udc_write(udc_read(UDNBIE) | (EP11N_IF), UDNBIE);
#ifdef MANUAL_EP11
			if (!is_ncm && !is_vera) {
				if (!down_trylock(&ep11_sw_sem)) {

					if ((udc_read(UDEPBFS) & 0x22) == 0x20) {
						if ((udc_read(UDEPBDMACS) & (UDC_FLASH_BUFFER_SIZE - 1)) > 0)	/*verB DMA bug*/
							udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
					} else {

						udc_write(1, UDEPBONAKCN);
					}
					up(&ep11_sw_sem);
				}
			} else if (is_ncm) {
				sp_udc_ep11_work();
			}
#endif
		} else {
			sp_udc_ep11_work();
		}
#else
		if (udc_read(UDEPBFS) & 0x22)
			ep11_handle(dev);
#endif
	}
	if (udc_read(UDNBIF) & SOF_IF) {
#ifdef MANUAL_EP11
		if (!is_ncm && dma_len && !down_trylock(&ep11_sw_sem)) {
			if ((udc_read(UDEPBFS) & 0x22) == 0x20 ||
				(sp_udc_get_ep_fifo_count((udc_read(UDEPBPPC) & CURR_BUFF) ? 1 : 0, UDEPBPIC) < 512 &&
				sp_udc_get_ep_fifo_count((udc_read(UDEPBPPC) & CURR_BUFF) ? 0 : 1, UDEPBPIC) == 512)) {
				if ((udc_read(UDEPBDMACS) & (UDC_FLASH_BUFFER_SIZE - 1)) > 0)	/*verB DMA bug*/
					udc_write(udc_read(UDEPBPPC) | SWITCH_BUFF, UDEPBPPC);
			}
			up(&ep11_sw_sem);
		}
#endif
/*		DEBUG_DBG("IRQ:sof\n");*/
		udc_write(SOF_IF, UDNBIF);
		udc_write(udc_read(UDNBIE) | (SOF_IF), UDNBIE);
#ifndef USBTEST_ZERO
		if (t_task && sof_n++ % 2 && ncm_initialize_finish_flag){
			tasklet_schedule(t_task);
		}
#endif
	}
	spin_unlock_irqrestore(&dev->lock, flags);
/*	DEBUG_DBG("\t<irq...nIF=%x\n", udc_read(UDNBIF));*/
	return IRQ_HANDLED;
}

static int sp_udc_queue(struct usb_ep *_ep, struct usb_request *_req,
			    gfp_t gfp_flags)
{
	struct sp_request *req = to_sp_req(_req);
	struct sp_ep *ep = to_sp_ep(_ep);
	struct sp_udc *dev;
	int ret = -1;
	int queue_ret = 0;
	u32 ep_csr = 0;
	unsigned long flags;

	DEBUG_INFO(">>> sp_udc_queue...\n");
	if (ep->num == EP9 || ep->num == EP8 || ep->num == EP3) {
		dev = ep->dev;
		local_irq_save(flags);
		goto iap_addr;
	}
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG_ERR("%s: invalid args %d %d %d\n", __func__,
			  (!_ep) ? 1 : 0, (!ep->desc) ? 1 : 0,
			  (ep->ep.name != ep0name) ? 1 : 0);
		return -EINVAL;
	}

	dev = ep->dev;

	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DEBUG_ERR("speed unknow\n");
		return -ESHUTDOWN;
	}
	local_irq_save(flags);
	if (unlikely(!_req || !_req->complete || !_req->buf
	     || !sp_udc_list_empty(&req->queue, &ep->lock))) {
		if (!_req)
			DEBUG_ERR("%s: 1 X X X\n", __func__);
		else {
			DEBUG_ERR("%s: 0 %01d %01d %01d\n", __func__,
				  !_req->complete, !_req->buf,
				  !sp_udc_list_empty(&req->queue,
							 &ep->lock));
		}
		local_irq_restore(flags);
		return -EINVAL;
	}

iap_addr:

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* kickstart this i/o queue? */
	if (sp_udc_list_empty(&ep->queue, &ep->lock)) {
		if (ep->bEndpointAddress == EP0) {
			DEBUG_INFO("ep0 enqueue\n");

			ep_csr = udc_read(UDEP0CS);
			if ((udc_read(UDEP0CS) & (EP0_OVLD | EP0_OUT_EMPTY)) == (EP0_OVLD | EP0_OUT_EMPTY))
				udc_write(udc_read(UDEP0CS) | CLR_EP0_OUT_VLD,UDEP0CS);
			DEBUG_DBG("dev->ep0state %d %x\n", dev->ep0state,
				  udc_read(UDEP0CS));
			switch (dev->ep0state) {
			case EP0_IN_DATA_PHASE:
				if (sp_udc_write_ep0_fifo(ep, req)) {
					ret = 0;
				}
				break;

			case EP0_OUT_DATA_PHASE:
				if (!_req->length /*&& (!dev->req_std) */ ) {	/*NEED MODIFY */
					udc_write(udc_read(UDLIE) | EP0I_IF, UDLIE);
					udc_write(SET_EP0_IN_VLD | EP0_DIR, UDEP0CS);
					DEBUG_DBG("ack 0 length => ep0 state=== %x\n", udc_read(UDEP0CS));
				} else {
					udc_write(udc_read(UDLIE) | EP0O_IF, UDLIE);
					/*udc_write(CLR_EP0_OUT_VLD, UDEP0CS);*/
				}
				if ((!_req->length)
				    || ((udc_read(UDEP0CS) & EP0_OVLD)
					&& sp_udc_read_ep0_fifo(ep, req))) {
					dev->ep0state = EP0_IDLE;
					ret = 0;
				}
				DEBUG_DBG("ep0 state => %x\n",
					  udc_read(UDEP0CS));
				break;

			default:
				DEBUG_ERR(" ***** ep0 failed, dev->ep0state = %d \n",
				     dev->ep0state);
				queue_ret = -EL2HLT;
				goto __QUEUE_BREAK;
			}

			if (ret == 0)
				req = NULL;
		} else if ((ep->bEndpointAddress & USB_DIR_IN) != 0) {
			/*DEBUG_NOTICE("ep in status^^^^^^^^^^^^<<< %x\n",udc_read(UDC_LLCFS_OFST));*/
			switch (ep->bEndpointAddress & 0x7F) {
			case EP1:
				DEBUG_INFO("enqueue, ep1 [%d] fifo_status:%x\n",
					   req->req.length, udc_read(UDEP12FS));
				break;
			case EP3:
				udc_write(udc_read(UDLIE) | EP3I_IF, UDLIE);

				DEBUG_INFO("enqueue, ep3 in [%d] %x\n",
					   req->req.length,
					   udc_read(UDEP3CTRL));
				ep_csr = udc_read(UDEP3CTRL);
				if ((!(ep_csr & EP3_VLD))) {
					if (sp_ep3_int_in(ep, req)) {
						req = NULL;
					}
				} else {
					DEBUG_ERR("[%s][%d] Why Invalid??\n",
						  __FUNCTION__, __LINE__);
					queue_ret = -EINVAL;
					goto __QUEUE_BREAK;
				}
				break;

			case EP5:
				DEBUG_INFO("enqueue, ep5 in [%d] %x\n",
					   req->req.length,
					   udc_read(UDEP5CTRL));
				ep_csr = udc_read(UDEP5CTRL);
				if ((!(ep_csr & EP3_VLD)) && g_flag_ep5 < 2) {
					if (sp_ep5_iso_in(ep, req)) {
						req = NULL;
					}
				} else {
					DEBUG_ERR("[%s][%d] Why Invalid??\n",
						  __FUNCTION__, __LINE__);
					queue_ret = -EINVAL;
					goto __QUEUE_BREAK;
				}
				break;
			case EP7:
				udc_write(udc_read(UDLIE) | EP7I_IF |
					  EP7_DMA_IF, UDLIE);

				DEBUG_INFO("enqueue, ep7 in [%d] %x\n",
					   req->req.length,
					   udc_read(UDEP7CTRL));
				ep_csr = udc_read(UDEP7CTRL);
				if ((!(ep_csr & EP3_VLD))) {
					if (sp_ep7_iso_in(ep, req)) {
						req = NULL;
					}
				} else {
					DEBUG_ERR("[%s][%d] Why Invalid??\n",
						  __FUNCTION__, __LINE__);
					queue_ret = -EINVAL;
					goto __QUEUE_BREAK;
				}
				break;
			case EP8:
				DEBUG_DBG("enqueue, ep8 bulk in [%d]\n",
					  req->req.length);
				if (((udc_read(UDEP89FS) & 0x11) != 0x11)
				    && sp_ep8_bulkin(ep, req))
					req = NULL;
				break;
			case EP10:
				/*DEBUG_INFO("enqueue, epa bulk in %x buff%d\n",
						udc_read(UDC_EPABFS_OFST),(!(udc_read(UDC_EPABPPC_OFST)>>2)));*/
				break;
			default:
				DEBUG_ERR("[%s][%d] Why Invalid??\n",
					  __FUNCTION__, __LINE__);
				queue_ret = -EINVAL;
				goto __QUEUE_BREAK;
			}
		} else {
			/*DEBUG_NOTICE("ep out status^^^^^^^^^^^^<<< %x\n",udc_read(UDC_LLCFS_OFST));*/
			if (ep->bEndpointAddress != EP2
			    && ep->bEndpointAddress != EP11
			    && ep->bEndpointAddress != EP9
			    && ep->bEndpointAddress != EP12
			    && ep->bEndpointAddress != EP5) {
				DEBUG_ERR("ep%d not complete \n",
					  ep->bEndpointAddress);
				queue_ret = -EINVAL;
				goto __QUEUE_BREAK;
			} else if (ep->bEndpointAddress == EP9) {
				DEBUG_DBG("enqueue, bulk out ep9 %x buf%d\n",
					  udc_read(UDEP9FS),
					  (!(udc_read(UDEP9PPC) >> 2)));
				/*if(udc_read(UDC_EP89FS_OFST)&0x22 &&
				   sp_ep9_bulkout(ep, req))
				   req == NULL; */
			} else if (ep->bEndpointAddress == EP11) {
				DEBUG_DBG("enqueue, bulk out ep11 %x buf%d\n",
					  udc_read(UDEPBFS),
					  (!(udc_read(UDEPBPPC) >> 2)));
			} else if (ep->bEndpointAddress == EP12) {
				if (sp_epc_iso_out(ep, req)) {
					req = NULL;
				}
			}
		}
	}
	if (likely(req)) {
		sp_list_add(&req->queue, &ep->queue, &ep->lock);
	}
	local_irq_restore(flags);
	if (dma_flag == 0 && (((ep->bEndpointAddress) & 0xf) == EP2)) {
		if (udc_read(UDEP2FS) & 0x22)
			ep2_handle(NULL);
	}
	if (dma_flag_b == 0 && (((ep->bEndpointAddress) & 0xf) == EP11)) {
		if (udc_read(UDEPBFS) & 0x22)
			ep11_handle(NULL);
	}
	if (dma_len_ep1) {
		udc_write(udc_read(UDLIE) | EP1_DMA_IF, UDLIE);
	} else if (((ep->bEndpointAddress) & 0xf) == EP1) {
		if (likely(req)) {
			if (req->req.actual) {
				udc_write(udc_read(UDLIE) | EP1I_IF, UDLIE);
				udc_write(udc_read(UDLIE) | EP1N_IF, UDLIE);
			}
			if ((udc_read(UDEP12FS) & 0x1) != 0x1) {

			}
		}
	}

	DEBUG_INFO("<<<%s..\n", __FUNCTION__);

	return 0;

__QUEUE_BREAK:
	if (likely(req))
		list_add_tail(&req->queue, &ep->queue);
	DEBUG_INFO("<<<%s...\n", __FUNCTION__);
	local_irq_restore(flags);
	if(queue_ret < 0)
		DEBUG_ERR("%s  error:%d\n",__FUNCTION__,queue_ret);

	return 0;
}

static int sp_udc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct sp_ep *ep;
	struct sp_udc *udc;
	struct sp_request *req = NULL;
	int retval = -EINVAL;

	DEBUG_INFO(">>> sp_udc_dequeue...\n");
	DEBUG_INFO("%s dequeue\n", _ep->name);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	ep = to_sp_ep(_ep);
	udc = to_sp_udc(ep->gadget);
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req) {
			sp_del_list(&req->queue, &ep->lock);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		DEBUG_INFO("dequeued req %p from %s, len %d buf %p\n", req,
			   _ep->name, _req->length, _req->buf);

		sp_udc_done(ep, req, -ECONNRESET);
	}
	DEBUG_INFO("<<< sp_udc_dequeue...retval = %d\n", retval);
	return retval;
}

static void sp_udc_enable(struct sp_udc *udc);

static int sp_udc_get_frame(struct usb_gadget *_gadget)
{
	u32 sof_value;

	sof_value = udc_read(UDFRNUM);

	return sof_value;
}

static int sp_udc_wakeup(struct usb_gadget *_gadget)
{
	return -EOPNOTSUPP;
}

static int sp_udc_set_selfpowered(struct usb_gadget *_gadget,
				      int is_selfpowered)
{
	return -EOPNOTSUPP;
}

static int sp_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	if (is_on) {
		DEBUG_NOTICE("Force to Connect\n");
		/*Force to Connect */
		udc_write((udc_read(UDLCSET) | 8) & 0xFE, UDLCSET);
	} else {
		DEBUG_NOTICE("Force to Disconnect\n");
		/*Force to Disconnect */
		sp_udc_nuke(NULL, &memory.ep[9], 0);
		udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
	}
	DEBUG_NOTICE("<<< sp_udc_pullup...\n");

	return 0;
}

static int sp_udc_vbus_session(struct usb_gadget *gadget, int is_active)
{
	return -EOPNOTSUPP;
}

static int sp_vbus_draw(struct usb_gadget *_gadget, unsigned ma)
{
	return -EOPNOTSUPP;
}

static int sp_udc_start(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct sp_udc *udc = the_controller;
	int ret;

	DEBUG_NOTICE(">>> sp_udc_start...\n");
	/* Sanity checks */
	if (!udc)
		return -ENODEV;
	if (udc->driver)
		return -EBUSY;

	if (!driver->bind || !driver->setup || driver->max_speed < USB_SPEED_FULL) {
		DEBUG_ERR("Invalid driver: bind %p setup %p speed %d\n", driver->bind,
			  driver->setup, driver->max_speed);
		return -EINVAL;
	}

	/* Hook the driver */
	udc->driver = driver;
	udc->gadget.dev.driver = &driver->driver;

	if ((ret = device_add(&udc->gadget.dev)) != 0) {
		DEBUG_ERR("Error in device_add() : %d\n", ret);
		goto register_error;
	}

	if ((ret = driver->bind(&udc->gadget, driver)) != 0) {
		device_del(&udc->gadget.dev);
		goto register_error;
	}

	sp_udc_enable(udc);

	DEBUG_NOTICE("<<< sp_udc_start...\n");
	return 0;

register_error:
	DEBUG_ERR("[%s][%d] fail[%d]\n", __FUNCTION__, __LINE__, ret);
	udc->driver = NULL;
	udc->gadget.dev.driver = NULL;
	return ret;
}

static int sp_udc_stop(struct usb_gadget_driver *driver){
	struct sp_udc *udc = the_controller;

	DEBUG_NOTICE(">>> sp_udc_stop...\n");
	if (!udc)
		return -ENODEV;

	if (!driver || driver != udc->driver || !driver->unbind)
		return -EINVAL;

	/* report disconnect */
	if (driver->disconnect)
		driver->disconnect(&udc->gadget);

	driver->unbind(&udc->gadget);

	device_del(&udc->gadget.dev);
	udc->driver = NULL;

	/* Disable udc
	sp_udc_disable(udc);  TODO: sp_udc_disable*/

	return 0;
}

static const struct usb_gadget_ops sp_ops = {
	.get_frame = sp_udc_get_frame,
	.wakeup = sp_udc_wakeup,
	.set_selfpowered = sp_udc_set_selfpowered,
	.pullup = sp_udc_pullup,
	.vbus_session = sp_udc_vbus_session,
	.vbus_draw = sp_vbus_draw,
	.udc_start = sp_udc_start,
	.udc_stop = sp_udc_stop,
};

static const struct usb_ep_ops sp_ep_ops = {
	.enable = sp_udc_ep_enable,
	.disable = sp_udc_ep_disable,

	.alloc_request = sp_udc_alloc_request,
	.free_request = sp_udc_free_request,

	.queue = sp_udc_queue,
	.dequeue = sp_udc_dequeue,

	.set_halt = sp_udc_set_halt,
};

/*
return	0 : disconnect	1 : connect
*/
static int sp_vbus_detect(void)
{
	return 1;
}

static struct sp_udc memory = {
	.gadget = {
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
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_IN | EP1,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },
	.ep[2] = {
		  .num = 2,
		  .ep = {
			 .name = "ep2out-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_OUT | EP2,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[3] = {
		  .num = 3,
		  .ep = {
			 .name = "ep3in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_IN | EP3,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,
		  },
	.ep[4] = {
		  .num = 4,
		  .ep = {
			 .name = "ep4in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
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
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_OUT | EP5,
		  .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		  },

	.ep[6] = {
		  .num = 6,
		  .ep = {
			 .name = "ep6in-int",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_IN | EP6,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,
		  },

	.ep[7] = {
		  .num = 7,
		  .ep = {
			 .name = "ep7-iso",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_OUT | EP7,
		  .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		  },

	.ep[8] = {
		  .num = 8,
		  .ep = {
			 .name = "ep8in-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_IN | EP8,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[9] = {
		  .num = 9,
		  .ep = {
			 .name = "ep9out-bulk",
			 .ops = &sp_ep_ops,
			 .maxpacket = EP12_FIFO_SIZE64,
			 },
		  .dev = &memory,
		  .fifo_size = EP_FIFO_SIZE,
		  .bEndpointAddress = USB_DIR_OUT | EP9,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,
		  },

	.ep[10] = {
		   .num = 10,
		   .ep = {
			  .name = "ep10in-bulk",
			  .ops = &sp_ep_ops,
			  .maxpacket = EP12_FIFO_SIZE64,
			  },
		   .dev = &memory,
		   .fifo_size = EP_FIFO_SIZE,
		   .bEndpointAddress = USB_DIR_IN | EP10,
		   .bmAttributes = USB_ENDPOINT_XFER_BULK,
		   },

	.ep[11] = {
		   .num = 11,
		   .ep = {
			  .name = "ep11out-bulk",
			  .ops = &sp_ep_ops,
			  .maxpacket = EP12_FIFO_SIZE64,
			  },
		   .dev = &memory,
		   .fifo_size = EP_FIFO_SIZE,
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
		   .fifo_size = EP_FIFO_SIZE,
		   .bEndpointAddress = USB_DIR_OUT | EP12,
		   .bmAttributes = USB_ENDPOINT_XFER_ISOC,
		   },
};

static void sp_udc_enable(struct sp_udc *udc)
{
	/*
	   usb device interrupt enable
	   ---force usb bus disconnect enable
	   ---force usb bus connect interrupt enable
	   ---vbus interrupt enable
	 */

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
	DEBUG_NOTICE("func:%s line:%d\n", __FUNCTION__, __LINE__);
}

static void sp_udc_reinit(struct sp_udc *udc)
{
	u32 i = 0;

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
	}
}

#if 0
#ifdef CONFIG_USB_SUNPLUS_OTG

static void sp_udc_otg_work(struct work_struct *work)
{
	struct usb_phy *otg_phy;
	struct sp_udc *udc =
	    (struct sp_udc *)container_of(work, struct sp_udc,
					      work_otg);
	int frame_a;
	int frame_b;
	int otg_id;

	DEBUG_NOTICE("%s in\n", __func__);

	if (udc->suspend_sta == 1) {
		DEBUG_ERR("B frist in\n");
	}

	udc->suspend_sta = 0;

	otg_phy = usb_get_transceiver_sunplus(udc->bus_num - 1);
	if (!otg_phy) {
		DEBUG_ERR("Get otg control fail(busnum:%d)!\n", udc->bus_num);
		return;
	}

	/* check device is disconnect ? */
	frame_a = udc->gadget.ops->get_frame(&udc->gadget);
	msleep(500);
	frame_b = udc->gadget.ops->get_frame(&udc->gadget);

	if (frame_a == frame_b) {

		DEBUG_NOTICE("B device is disconnect %d %d\n", frame_a,
			     frame_b);
		otg_set_vbus(otg_phy->otg, 0);

		/* id pin is still low */
		if (otg_phy->io_ops->read && otg_phy->io_ops->write) {
			otg_id = otg_phy->io_ops->read(otg_phy, (u32) (((u32 *) otg_phy->io_priv) + OTG_INT_ST_REG));
			if ((otg_id & ~ID_PIN) == 0) {
				msleep(10);
				/*otg_set_vbus(otg_phy->otg, 1);*/
			}
		}
		DEBUG_NOTICE("disc apple %d\n", is_config);
		if (is_config) {
			DEBUG_NOTICE("disc apple\n");
			sp_udc_nuke(memory.ep[8].dev, &memory.ep[8], -ESHUTDOWN);
			sp_reinit_iap(&memory.ep[9], list_entry((&memory.ep[9])->queue.next, struct sp_request, queue), -ESHUTDOWN);
		}
	}
}
#endif
#endif

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

static int sp_udc_probe(struct platform_device *pdev)
{
	struct sp_udc *udc = &memory;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	static u64 rsrc_start, rsrc_len;

#ifdef CONFIG_USB_SUNPLUS_OTG
	struct usb_phy *otg_phy;
#endif

	/*enable usb controller clock*/
	udc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(udc->clk)) {
		pr_err("not found clk source\n");
		return PTR_ERR(udc->clk);
	}
	clk_prepare(udc->clk);
	clk_enable(udc->clk);

#ifdef CONFIG_GADGET_USB0
	pdev->id = 1;
#else
	pdev->id = 2;
#endif

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq_num = platform_get_irq(pdev, 0);
	if (!res || !irq_num) {
		DEBUG_ERR("Not enough platform resources.\n");
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
#ifdef ISP_DEBUG
	int sp3502_udc_probe(void);
	sp3502_udc_probe();
#endif
	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = dev;
	udc->gadget.dev.dma_mask = dev->dma_mask;
	udc->qwork_ep3 = create_singlethread_workqueue("sp-udc-ep3");
	if (!udc->qwork_ep3) {
		DEBUG_ERR("cannot create workqueue sp-udc-ep3\n");
		ret = -ENOMEM;
	}
	INIT_WORK(&udc->work_ep3, sp_udc_ep3_work);

	udc->qwork_ep9 = create_singlethread_workqueue("sp-udc-ep9");
	if (!udc->qwork_ep9) {
		DEBUG_ERR("cannot create workqueue sp-udc-ep9\n");
		ret = -ENOMEM;
	}
	INIT_WORK(&udc->work_ep9, sp_udc_ep9_work);
	the_controller = udc;
	platform_set_drvdata(pdev, udc);
	sp_udc_reinit(udc);
#ifdef CONFIG_FIQ_GLUE
	udc->handler.isr = fiq_isr;
	ret = fiq_glue_register_handler(&udc->handler);
	if (ret != 0)
		DEBUG_NOTICE("udc fiq fail\n");

	ret = request_threaded_irq(irq_num, 0, udcThreadHandler,
	                           IRQF_DISABLED , gadget_name, udc);/*udcThreadHandler*/

#else
	ret = request_irq(irq_num, sp_udc_irq, IRQF_DISABLED, gadget_name, udc);
#endif
	if (ret != 0) {
		DEBUG_ERR("cannot get irq %i, err %d\n", irq_num, ret);
		ret = -EBUSY;
		goto err_map;
	}
	platform_set_drvdata(pdev, udc);
	udc->vbus = 0;

	spin_lock_init(&udc->lock);
	init_ep_spin();
	sp_sem_init();

#ifdef CONFIG_USB_SUNPLUS_OTG
	udc->bus_num = pdev->id;
	otg_phy = usb_get_transceiver_sp(pdev->id - 1);
	ret = otg_set_peripheral(otg_phy->otg, &udc->gadget);
	if (ret < 0) {
		goto err_add_udc;
	}
#endif

	DEBUG_ERR("udc-line:%d\n", __LINE__);
	ret = usb_add_gadget_udc(&pdev->dev, &udc->gadget);
	if (ret) {
		goto err_add_udc;
	}

	DEBUG_ERR("probe sp udc ok %x\n", udc_read(VERSION));
	/*iap debug */
	udc_write(udc_read(UDNBIE) | EP8N_IF | EP8I_IF, UDNBIE);
	udc_write(EP_ENA | EP_DIR, UDEP89C);
	udc_write(udc_read(UDNBIE) | EP9N_IF | EP9O_IF, UDNBIE);

	device_create_file(&pdev->dev, &dev_attr_udc_ctrl);

	return 0;
err_add_udc:
	if(irq_num)
		free_irq(irq_num, udc);
err_map:
	DEBUG_INFO("probe sp udc fail\n");
	return ret;
err_mem:
	release_mem_region(rsrc_start, rsrc_len);
error:
	DEBUG_ERR("probe sp udc fail\n");
	return ret;

}

static int sp_udc_remove(struct platform_device *pdev)
{
	struct sp_udc *udc = platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &dev_attr_udc_ctrl);
	usb_del_gadget_udc(&udc->gadget);
	if (udc->driver)
		return -EBUSY;

	if(irq_num)
		free_irq(irq_num, udc);

	if (udc->qwork_ep9)
		destroy_workqueue(udc->qwork_ep9);
	if (udc->qwork_ep3)
		destroy_workqueue(udc->qwork_ep3);

	if(base_addr)
		iounmap(base_addr);

	platform_set_drvdata(pdev, NULL);
	/*disable usb controller clock*/
	clk_disable(udc->clk);
	DEBUG_INFO("sp_udc remove ok\n");

	return 0;
}

#ifdef CONFIG_PM
static int sp_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
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

void sp_udc_state_polling(struct timer_list *t)
{
	if (!platform_device_handle_flag
		|| !bus_reset_finish_flag){
		DEBUG_DBG("finish_flag: %d,handle_flag: %d\n",
			bus_reset_finish_flag,platform_device_handle_flag);
		return ;
	}
	if (sof_value == udc_read(UDFRNUM) /*&& sof_value */ ) {
		DEBUG_NOTICE("SOF %x\n", udc_read(UDFRNUM));
		sof_value = 0x1000;
		/*discnnect */
		usb_switch(TO_HOST);
	} else {
		sof_value = udc_read(UDFRNUM);
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
		temp_sof_value = udc_read(UDFRNUM);
		first_enter_polling_timer_flag = false;
		mod_timer(&sof_polling_timer,
			  jiffies + 3 * HZ / 2);
	} else {
		if (temp_sof_value == udc_read(UDFRNUM)) {
			DEBUG_NOTICE("SOFX %x\n",
				     udc_read(UDFRNUM));
			/*discnnect */
					usb_switch(TO_HOST);
		} else {
			temp_sof_value = udc_read(UDFRNUM);
			mod_timer(&sof_polling_timer,
				  jiffies + 3 * HZ / 2);
		}
	}
}

void usb_switch(int device)
{
	void __iomem *regs = (void __iomem *)B_SYSTEM_BASE;

	if (device) {
#if 1
		if (accessory_port_id == 0) {
				writel(RF_MASK_V_SET(1 << 4), regs + USBC_CTL_OFFSET);
				writel(RF_MASK_V_CLR(1 << 5), regs + USBC_CTL_OFFSET);
		} else {
				writel(RF_MASK_V_SET(1 << 12), regs + USBC_CTL_OFFSET);
				writel(RF_MASK_V_CLR(1 << 13), regs + USBC_CTL_OFFSET);
		}
#else
		value = readl(USBC_CTL);
		if (accessory_port_id == 0) {
			value &= ~(7 << 4);
			value |= (1u << 6) | (0u << 5) | (1u << 4);
		} else {
			value &= ~(7 << 12);
			value |= (1u << 14) | (0u << 13) | (1u << 12);
		}
		writel(value, USBC_CTL);
#endif
		DEBUG_ERR("host to device\n");
	} else {
		udc_write(udc_read(UDLCSET) | SOFT_DISC, UDLCSET);
		if (is_config) {
			is_config = 0;
			sp_udc_nuke(memory.ep[8].dev, &memory.ep[8],
					-ESHUTDOWN);
			/*sp_reinit_iap(&memory.ep[9],
						list_entry((&memory.ep[9])->queue.next,
						struct sp_request, queue), -ESHUTDOWN);*/
		}
		if (accessory_port_id == 0) {
			writel(RF_MASK_V_SET(1 << 5), regs + USBC_CTL_OFFSET);
		} else {
			writel(RF_MASK_V_SET(1 << 13), regs + USBC_CTL_OFFSET);
		}
		bus_reset_finish_flag = false;
		platform_device_handle_flag = false;
		DEBUG_ERR("device to host!\n");
	}
}

void ctrl_rx_squelch(void)/*Controlling squelch signal to slove the uphy bad signal problem*/
{
	udc_write(udc_read(UEP12DMACS) | RX_STEP7, UEP12DMACS);	/*control rx signal */

	DEBUG_NOTICE("ctrl_rx_squelch UEP12DMACS: %x\n",udc_read(UEP12DMACS));
}

EXPORT_SYMBOL(usb_switch);
EXPORT_SYMBOL(ctrl_rx_squelch);

void detech_start(void)
{
#ifdef	CONFIG_USB_HOST_RESET_SP
	udc_init_c();
#endif
	platform_device_handle_flag = true;
	first_enter_polling_timer_flag = true;
	reset_global_value();
	/*mod_timer(&vbus_polling_timer, jiffies + d_time0);*/
	/*mod_timer(&sof_polling_timer, jiffies + HZ / 10);*/
	DEBUG_ERR("detech_start......\n");
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

static struct platform_driver sunplus_udc0_driver = {
	.driver		= {
		.name	= "sunplus-usbgadget0",
		.of_match_table = sunplus_udc0_ids,
	},
	.probe		= sp_udc_probe,
	.remove		= sp_udc_remove,
	.suspend	= sp_udc_suspend,
	.resume		= sp_udc_resume,
};

static struct platform_driver sunplus_udc1_driver = {
	.driver		= {
		.name	= "sunplus-usbgadget1",
		.of_match_table = sunplus_udc1_ids,
	},
	.probe		= sp_udc_probe,
	.remove		= sp_udc_remove,
	.suspend	= sp_udc_suspend,
	.resume		= sp_udc_resume,
};

static int __init udc_init(void)
{
	int retval;
	
#ifdef ISP_DEBUG
	g_hw = vmalloc(ISP_DEBUG_SIZE);
	DEBUG_ERR("%x\n", g_hw);
#endif
	DEBUG_NOTICE("udc_init\n");

	if (accessory_port_id == USB_PORT0_ID) {
		printk(KERN_NOTICE "register sunplus_udc0_driver\n");
		retval = platform_driver_register(&sunplus_udc0_driver);
	} else {
		printk(KERN_NOTICE "register sunplus_udc1_driver\n");
		retval = platform_driver_register(&sunplus_udc1_driver);
		
	}
	if (retval)
		goto err;

	timer_setup(&vbus_polling_timer, sp_udc_state_polling, 0);
	vbus_polling_timer.expires = jiffies - HZ;
	/*add_timer(&vbus_polling_timer); */

	timer_setup(&sof_polling_timer, sp_sof_state_polling, 0);
	sof_polling_timer.expires = jiffies + 3 * HZ / 2;

	/*switch usbX phy to host for CarPlay ,move to ehci/ohci reset thread*/
	/*usb_switch(1); */
#if 0
	if (readl((void *)MO_STAMP) == IC_VERSION_A) {
		is_vera = 1;
		dma_fail = 1;
		DEBUG_ERR("IC VerA\n");
	}
#endif

	return 0;

err:
	return retval;
}

static void __exit udc_exit(void)
{
	if (accessory_port_id == USB_PORT0_ID) {
		platform_driver_unregister(&sunplus_udc0_driver);
	} else {
		platform_driver_unregister(&sunplus_udc1_driver);
	}
}

MODULE_LICENSE("GPL");

module_init(udc_init);
module_exit(udc_exit);
