#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <linux/usb/hcd.h>

#include <linux/platform_device.h>
#include <linux/usb/sp_usb.h>
#include <linux/usb/otg.h>

#include "otg-sunplus.h"
#ifdef CONFIG_USB_GADGET_SUNPLUS
#include "../core/otg_whitelist.h"
#endif

#define DRIVER_NAME		"sp-otg"

#ifdef CONFIG_USB_GADGET_SUNPLUS
extern void detech_start(void);
extern void udc_otg_ctrl(void);
#endif
extern void phy0_otg_ctrl(void);
extern void phy1_otg_ctrl(void);

extern struct sp_otg *sp_otg0_host;
extern struct sp_otg *sp_otg1_host;
extern u8 otg0_vbus_off;
extern u8 otg1_vbus_off;

static struct usb_phy *phy_sp[2] = {NULL, NULL};

struct usb_phy *usb_get_transceiver_sp(int bus_num)
{
	if (phy_sp[bus_num]){
		return phy_sp[bus_num];
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(usb_get_transceiver_sp);

int usb_set_transceiver_sp(struct usb_phy *x, int bus_num)
{
	//dump_stack();
	if (phy_sp[bus_num] && x){
		return -EBUSY;
	}
	phy_sp[bus_num]= x;
	return 0;
}
EXPORT_SYMBOL(usb_set_transceiver_sp);

const char *otg_state_string(enum usb_otg_state state)
{
	switch (state) {
	case OTG_STATE_A_IDLE:
		return "a_idle";
	case OTG_STATE_A_WAIT_VRISE:
		return "a_wait_vrise";
	case OTG_STATE_A_WAIT_BCON:
		return "a_wait_bcon";
	case OTG_STATE_A_HOST:
		return "a_host";
	case OTG_STATE_A_SUSPEND:
		return "a_suspend";
	case OTG_STATE_A_PERIPHERAL:
		return "a_peripheral";
	case OTG_STATE_A_WAIT_VFALL:
		return "a_wait_vfall";
	case OTG_STATE_A_VBUS_ERR:
		return "a_vbus_err";
	case OTG_STATE_B_IDLE:
		return "b_idle";
	case OTG_STATE_B_SRP_INIT:
		return "b_srp_init";
	case OTG_STATE_B_PERIPHERAL:
		return "b_peripheral";
	case OTG_STATE_B_WAIT_ACON:
		return "b_wait_acon";
	case OTG_STATE_B_HOST:
		return "b_host";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(otg_state_string);

static int sp_start_srp(struct usb_otg *otg)
{
	return 0;
}

void dump_debug_register(struct usb_otg *otg)
{
	struct sp_otg *otg_host = (struct sp_otg *)container_of(otg->usb_phy, struct sp_otg, otg);

	printk(KERN_DEBUG "%s.%d,otg_debug:%x,%p\n",__FUNCTION__,__LINE__,
				readl(&otg_host->regs_otg->otg_debug_reg),&(otg_host->regs_otg->otg_debug_reg));
}
EXPORT_SYMBOL(dump_debug_register);

#ifdef CONFIG_USB_GADGET_SUNPLUS
void sp_accept_b_hnp_en_feature(struct usb_otg *otg)
{
	u32 val;

	struct sp_otg *otg_host = (struct sp_otg *)container_of(otg->usb_phy, struct sp_otg, otg);

	val = readl(&otg_host->regs_otg->otg_device_ctrl);
	val |= B_HNP_EN_BIT;
	val |= B_VBUS_REQ;
	writel(val, &otg_host->regs_otg->otg_device_ctrl);

	udc_otg_ctrl();

	if (otg_host->id == 1)
		phy0_otg_ctrl();
	else if (otg_host->id == 2)
		phy1_otg_ctrl();
}

static int sp_start_hnp(struct usb_otg *otg)
{
	struct sp_otg *otg_host = (struct sp_otg *)container_of(otg->usb_phy,
						struct sp_otg, otg);
	u32 ret;

	ret = readl(&otg_host->regs_otg->otg_device_ctrl);
	otg_debug("%s.%d,otg_debug:%x,%p\n",__FUNCTION__,__LINE__,
				readl(&otg_host->regs_otg->otg_debug_reg),&(otg_host->regs_otg->otg_debug_reg));
	ret |= A_SET_B_HNP_EN_BIT;
	ret &= ~A_BUS_REQ_BIT;
	writel(ret, &otg_host->regs_otg->otg_device_ctrl);
	otg_debug("%s.%d,otg_debug:%x,%p\n",__FUNCTION__,__LINE__,
				readl(&otg_host->regs_otg->otg_debug_reg),&(otg_host->regs_otg->otg_debug_reg));

	otg_host->otg.otg->state = OTG_STATE_A_PERIPHERAL;
	mdelay(100);
	detech_start();

	otg_debug("start hnp\n");

	return 0;
}
#endif

static int sp_set_vbus(struct usb_otg *otg, bool enabled)
{
	struct sp_otg *otg_host = (struct sp_otg *)container_of(otg->usb_phy,
						struct sp_otg, otg);
	u32 ret;
	u32 id_pin;

	ret = readl(&otg_host->regs_otg->otg_device_ctrl);

	if (enabled) {
		otg_debug("set vbus\n");
		ret |= A_BUS_REQ_BIT;
	} else {
		otg_debug("drop vbus\n");
		ret |= A_BUS_DROP_BIT;

#ifdef	CONFIG_ADP_TIMER
		mod_timer(&otg_host->adp_timer, ADP_TIMER_FREQ + jiffies);
#endif

		id_pin = readl(&otg_host->regs_otg->otg_int_st);
		if ((id_pin & ID_PIN) == 0)
			otg_host->otg.otg->state = OTG_STATE_A_WAIT_VFALL;
	}

	writel(ret, &otg_host->regs_otg->otg_device_ctrl);

	return 0;
}

int sp_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	struct sp_otg *otg_host = (struct sp_otg *)container_of(otg->usb_phy, struct sp_otg, otg);

	otg_host->otg.otg->host = host;
	otg->host = host;

	if (host) {
		otg->host->otg_port = otg_host->id;
		otg->host->is_b_host = otg_host->fsm.id;
	}

	return 0;
}

#ifdef CONFIG_USB_GADGET_SUNPLUS
int sp_set_peripheral(struct usb_otg *otg, struct usb_gadget *gadget)
{
	otg->gadget = gadget;

	return 0;
}
#endif

int sp_phy_read(struct usb_phy *x, u32 reg)
{
	return readl((u32 *)reg);
}

int sp_phy_write(struct usb_phy *x, u32 val, u32 reg)
{
	writel(val, (u32 *)reg);

	return 0;
}

struct usb_phy_io_ops sp_phy_ios = {
	.read = sp_phy_read,
	.write = sp_phy_write,
};

#ifdef CONFIG_USB_GADGET_SUNPLUS
static int hnp_polling_watchdog(void *arg)
{
	struct sp_otg *otg_host = (struct sp_otg *)arg;
	struct usb_device *udev = NULL;
	struct usb_hcd *hcd;
	struct usb_phy *otg_phy;
	static bool find_child = false;
	static int targeted;
	u32 val;
	bool host_req_flag = false;
	u32 *otg_status;
	int ret;

 	otg_status = kmalloc(4, GFP_KERNEL);
	if (!otg_status)
		return -ENOMEM;

	while(!kthread_should_stop()) {
		val = readl(&otg_host->regs_otg->otg_debug_reg);
		val &= 0x0f;

		if (val == SP_OTG_STATE_A_HOST) {
 			if (find_child == false) {
				msleep(2000);

				udev = usb_hub_find_child(otg_host->otg.otg->host->root_hub, 1);
				if ((!udev) ||
				    (udev->config->interface[0]->altsetting->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE))
					continue;

				targeted = is_targeted(udev);
				hcd  = bus_to_hcd(udev->bus);
				find_child = true;

				if ((IS_ENABLED(CONFIG_USB_OTG_WHITELIST) && hcd->tpl_support && targeted) ||
			    	    (!IS_ENABLED(CONFIG_USB_OTG_WHITELIST) || !hcd->tpl_support)) {
			    	    	if (IS_ENABLED(CONFIG_USB_OTG_WHITELIST) && hcd->tpl_support && targeted)
						otg_debug("TPL is supported\n");
					else
						otg_debug("TPL is not supported\n");

			    		otg_debug("start HNP polling\n");
				}
			}

			if ((IS_ENABLED(CONFIG_USB_OTG_WHITELIST) && hcd->tpl_support && targeted) ||
			    (!IS_ENABLED(CONFIG_USB_OTG_WHITELIST) || !hcd->tpl_support)) {
				ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
									  0, 0x80, 0, 0xf000, otg_status, 4, 8000);
				if(ret < 0) {
					otg_debug("polling otg status fail,ret:%d\n", ret);
					return -1;
				} else {
					host_req_flag = *otg_status & 0x1;
					if(host_req_flag) {
						otg_phy = usb_get_transceiver_sp(udev->bus->busnum-1);
						if(!otg_phy){
							otg_debug("Get otg control fail(busnum:%d)!\n", udev->bus->busnum);
							return 1;
						}

						ret = usb_control_msg(udev,
						usb_sndctrlpipe(udev, 0),
						USB_REQ_SET_FEATURE, 0,
						USB_DEVICE_B_HNP_ENABLE,
						0, NULL, 0,
						USB_CTRL_SET_TIMEOUT);

						if (ret < 0) {
							/*
							 * OTG MESSAGE: report errors here,
							 * customize to match your product.
							 */
							otg_debug("can't set HNP mode: %d\n",ret);
						}

						otg_start_hnp(otg_phy->otg);
						msleep(1);
#ifdef CONFIG_USB_GADGET_SUNPLUS
						detech_start();
#endif
						return 0;
					} else {
					  	msleep(1000);
					}
				}
			}
		}
		else {
			find_child = false;
			msleep(1);
		}
	}

	return 0;
}
#endif

static void sp_otg_work(struct work_struct *work)
{
	struct sp_otg *otg_host;
	u32 val;

	otg_host = (struct sp_otg *)container_of(work, struct sp_otg,
						work);

	switch (otg_host->otg.otg->state) {
	case OTG_STATE_UNDEFINED:
	case OTG_STATE_B_IDLE:
		break;
	case OTG_STATE_B_SRP_INIT:
		break;
	case OTG_STATE_B_PERIPHERAL:
		break;
	case OTG_STATE_B_WAIT_ACON:
		break;
	case OTG_STATE_B_HOST:
		break;
	case OTG_STATE_A_IDLE:
		otg_debug("FSM: OTG_STATE_A_IDLE\n");

		if (otg_host->fsm.id == 0) {

			val = readl(&otg_host->regs_otg->otg_device_ctrl);
			val |= A_BUS_REQ_BIT;
			writel(val, &otg_host->regs_otg->otg_device_ctrl);
		}

		otg_host->otg.otg->state = OTG_STATE_A_WAIT_VRISE;
		break;
	case OTG_STATE_A_WAIT_VRISE:
		break;
	case OTG_STATE_A_WAIT_BCON:
		break;
	case OTG_STATE_A_HOST:
		break;
	case OTG_STATE_A_SUSPEND:
		break;
	case OTG_STATE_A_PERIPHERAL:
		break;
	case OTG_STATE_A_VBUS_ERR:
		otg_debug("FSM: OTG_STATE_A_VBUS_ERR\n");

		if (otg_host->fsm.id == 0) {
			msleep(100);
			val = readl(&otg_host->regs_otg->otg_device_ctrl);
			val |= A_BUS_REQ_BIT;
			writel(val, &otg_host->regs_otg->otg_device_ctrl);
		}
		otg_host->otg.otg->state = OTG_STATE_A_WAIT_VRISE;
		break;
	case OTG_STATE_A_WAIT_VFALL:
		break;
	default:
		break;
	}
}

/* host/client notify transceiver when event affects HNP state */
void sp_otg_update_transceiver(struct sp_otg *otg_host)
{
	if (unlikely(!otg_host || !otg_host->qwork))
		return;

	queue_work(otg_host->qwork, &otg_host->work);
}
EXPORT_SYMBOL(sp_otg_update_transceiver);

static void otg_hw_init(struct sp_otg *otg_host)
{
	u32 val;

	/* Set adp charge precision */
	writel(0x3f, &otg_host->regs_otg->adp_chng_precision);

	/* Set adp dis-charge time */
	writel(0xfff, &otg_host->regs_otg->adp_chrg_time);

	/* Set wait power rise timer */
	writel(0x1ffff, &otg_host->regs_otg->a_wait_vrise_tmr);

	/* Set session end timer */
	writel(0x20040, &otg_host->regs_otg->a_wait_vfall_tmr);

	/* Set wait b-connect time  */
	writel(0x1fffff, &otg_host->regs_otg->a_wait_bcon_tmr);

	/* Enbale ADP&SRP  */
	val = readl(&otg_host->regs_otg->otg_int_st);
	if (val & ID_PIN)
		writel(~OTG_SIM & (OTG_SRP | OTG_20),
			  &otg_host->regs_otg->mode_select);
	else
		writel(~OTG_SIM & (OTG_ADP | OTG_SRP | OTG_20),
			  &otg_host->regs_otg->mode_select);
}

static void otg_hsm_init(struct sp_otg *otg_host)
{
	int val32;

	val32 = readl(&otg_host->regs_otg->otg_int_st);

	/* set init state */
	if (val32 & ID_PIN) {
		otg_debug("Init is B device\n");
		otg_host->fsm.id = 1;
		otg_host->otg.otg->default_a = 0;
		otg_host->otg.otg->state = OTG_STATE_B_IDLE;
	} else {
		otg_debug("Init is A device\n");
		otg_host->fsm.id = 0;
		otg_host->otg.otg->default_a = 1;
		otg_host->otg.otg->state = OTG_STATE_A_IDLE;
	}

	otg_host->fsm.drv_vbus = 0;
	otg_host->fsm.loc_conn = 0;
	otg_host->fsm.loc_sof = 0;

	/* defautly power the bus */
	otg_host->fsm.a_bus_req = 0;
	otg_host->fsm.a_bus_drop = 0;
	/* defautly don't request bus as B device */
	otg_host->fsm.b_bus_req = 0;
	/* no system error */
	otg_host->fsm.a_clr_err = 0;
}

int sp_notifier_call(struct notifier_block *self, unsigned long action,
			   void *dev)
{
	struct sp_otg *otg_host = (struct sp_otg *)container_of(self, struct sp_otg, notifier);

	struct usb_device *udev = (struct usb_device *)dev;
	u32 val;

	otg_debug("%p %p\n", udev->bus, otg_host->otg.otg->host);

	if (udev->bus != otg_host->otg.otg->host)
		return 0;

	if (udev->parent != udev->bus->root_hub)
		return 0;

	if ((action == USB_DEVICE_REMOVE)
	    && (otg_host->otg.otg->state != OTG_STATE_A_PERIPHERAL)) {
		otg_debug(" usb device remove\n");
		val = readl(&otg_host->regs_otg->otg_device_ctrl);
		val |= A_BUS_DROP_BIT;
		writel(val, &otg_host->regs_otg->otg_device_ctrl);

#ifdef	CONFIG_ADP_TIMER
		mod_timer(&otg_host->adp_timer, ADP_TIMER_FREQ + jiffies);
#endif
	}

	return 0;
}

#ifdef	CONFIG_ADP_TIMER
static void adp_watchdog0(struct timer_list *t)
{
	u32 val;

	if (otg0_vbus_off == 1) {
		otg0_vbus_off = 0;

		val = readl(&sp_otg0_host->regs_otg->mode_select);
		val |= OTG_ADP;
		writel(val, &sp_otg0_host->regs_otg->mode_select);

		otg_debug("adp timer (otg0) %d\n", sp_otg0_host->irq);
	}
}

static void adp_watchdog1(struct timer_list *t)
{
	u32 val;

	if (otg1_vbus_off == 1) {
		otg1_vbus_off = 0;

		val = readl(&sp_otg1_host->regs_otg->mode_select);
		val |= OTG_ADP;
		writel(val, &sp_otg1_host->regs_otg->mode_select);

		otg_debug("adp timer (otg0) %d\n", sp_otg1_host->irq);
	}
}
#endif

static irqreturn_t otg_irq(int irq, void *dev_priv)
{
	struct sp_otg *otg_host = (struct sp_otg *)dev_priv;
	u32 int_status;
	u32 val = 0;

//	struct timespec	t0;
//	getnstimeofday(&t0);

	if (otg_host->id == 1)
		otg_debug("otg0 irq in\n");
	else if (otg_host->id == 2)
		otg_debug("otg1 irq in\n");

	/* clear interrupt status */
	int_status = readl(&otg_host->regs_otg->otg_int_st);
	writel((int_status & INT_MASK), &otg_host->regs_otg->otg_int_st);

	if (int_status & ADP_CHANGE_IF) {
		otg_debug("ADP_CHANGE_IF %d %x %x\n", irq,
			  readl(&otg_host->regs_otg->otg_debug_reg)
			  , readl(&otg_host->regs_otg->adp_debug_reg));
		otg_host->fsm.adp_change = 1;
		val = readl(&otg_host->regs_otg->otg_device_ctrl);
		val |= A_BUS_REQ_BIT;
		writel(val, &otg_host->regs_otg->otg_device_ctrl);

		//otg_debug("adp in %09ld.%09ld\n", t0.tv_sec, t0.tv_nsec);

		//otg_host->fsm.id = (int_status&ID_PIN) ? 1 : 0;
		//otg_host->otg.state = OTG_STATE_A_IDLE;
	}

	if (int_status & A_BIDL_ADIS_IF) {
		otg_debug("A_BIDL_ADIS_IF\n");

		val = readl(&otg_host->regs_otg->otg_device_ctrl);
		val |= A_BUS_DROP_BIT;
		writel(val, &otg_host->regs_otg->otg_device_ctrl);
	}

	if (int_status & A_SRP_DET_IF) {
		otg_debug("A_SRP_DET_IF\n");
		otg_host->fsm.a_srp_det = 1;
	}

	if (int_status & B_AIDL_BDIS_IF) {
		otg_debug("B_AIDL_BDIS_IF\n");

		if (otg_host->id == 1)
			val = RF_MASK_V_CLR(1 << 4);
		else if (otg_host->id == 2)
			val = RF_MASK_V_CLR(1 << 12);

		writel(val, &otg_host->regs_moon4->mo4_usbc_ctl);
	}

	if (int_status & A_AIDS_BDIS_TOUT_IF) {
		otg_debug("A_AIDS_BDIS_TOUT_IF\n");
		otg_host->fsm.a_aidl_bdis_tmout = 1;
	}

	if (int_status & B_SRP_FAIL_IF) {
		otg_debug("B_SRP_FAIL_IF id %x\n", int_status);
	}

	if (int_status & BDEV_CONNT_TOUT_IF) {
		otg_debug("BDEV_CONNT_TOUT_IF\n");

		val = readl(&otg_host->regs_otg->otg_device_ctrl);
		val |= A_BUS_DROP_BIT;
		writel(val, &otg_host->regs_otg->otg_device_ctrl);

		if (((otg_host->id == 1) && (otg0_vbus_off == 1)) ||
		    ((otg_host->id == 2) && (otg1_vbus_off == 1))) {
			val = readl(&otg_host->regs_otg->mode_select);
			val &= ~OTG_ADP;
			writel(val, &otg_host->regs_otg->mode_select);
		}

#ifdef	CONFIG_ADP_TIMER
		mod_timer(&otg_host->adp_timer, ADP_TIMER_FREQ + jiffies);
#endif
		otg_host->fsm.a_wait_bcon_tmout = 1;

		//otg_debug("adp in %09ld.%09ld\n", t0.tv_sec, t0.tv_nsec);
	}

	if (int_status & VBUS_RISE_TOUT_IF) {
		otg_debug("VBUS_RISE_TOUT_IF\n");
		otg_host->fsm.a_wait_vrise_tmout = 1;
	}

	if (int_status & ID_CHAGE_IF) {
		otg_debug("ID_CHAGE_IF\n");

		otg_host->fsm.id = (int_status & ID_PIN) ? 1 : 0;
		if ((int_status & ID_PIN) == 0) {
			writel(~OTG_SIM & (OTG_ADP | OTG_SRP | OTG_20),
				  &otg_host->regs_otg->mode_select);
#ifdef	CONFIG_ADP_TIMER
			mod_timer(&otg_host->adp_timer,
				  ADP_TIMER_FREQ + jiffies);
#endif
		} else {
			writel(~OTG_SIM & (OTG_SRP | OTG_20),
				  &otg_host->regs_otg->mode_select);
		}
	}

	if (int_status & OVERCURRENT_IF) {
		otg_debug("OVERCURRENT_IF\n");

		val = readl(&otg_host->regs_otg->otg_device_ctrl);
		val &= ~A_CLE_ERR_BIT;
		val |= A_BUS_DROP_BIT;
		writel(val, &otg_host->regs_otg->otg_device_ctrl);

		//otg_debug("oct in %09ld.%09ld\n", t0.tv_sec, t0.tv_nsec);

#ifdef	CONFIG_ADP_TIMER
		mod_timer(&otg_host->adp_timer, (HZ/10) + jiffies);
#endif

		//otg_host->fsm.id = (int_status&ID_PIN) ? 1 : 0;
		//otg_host->otg.state = OTG_STATE_A_VBUS_ERR;
	}

	if (otg_host->id == 1)
		otg_debug("otg0 irq out\n");
	else if (otg_host->id == 2)
		otg_debug("otg1 irq out\n");

	sp_otg_update_transceiver(otg_host);

	return IRQ_HANDLED;
}

int sp_otg_probe(struct platform_device *dev)
{
	struct sp_otg *otg_host;
	struct resource *res_mem;
	int ret;

	if ((sp_port0_enabled & PORT0_ENABLED) && (sp_otg0_host == NULL))
			dev->id = 1;
	else if ((sp_port1_enabled & PORT1_ENABLED) && (sp_otg1_host == NULL))
			dev->id = 2;

	otg_host = kzalloc(sizeof(struct sp_otg), GFP_KERNEL);
	if (!otg_host) {
		printk(KERN_NOTICE "Alloc mem for otg host fail\n");
		return -ENOMEM;
	}

	if (dev->id == 1) {
		sp_otg0_host = otg_host;
		otg_host->id = 1;
	}
	else if (dev->id == 2) {
		sp_otg1_host = otg_host;
		otg_host->id = 2;
	}

	otg_host->otg.otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
	if (!otg_host->otg.otg) {
		kfree(otg_host);
		printk(KERN_NOTICE "Alloc mem for otg fail\n");
		return -ENOMEM;
	}

	otg_host->irq = platform_get_irq(dev, 0);
	if (otg_host->irq < 0) {
		pr_err("otg no irq provieded\n");
		ret = otg_host->irq;
		goto release_mem;
	}
	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		pr_err("otg no memory recourse provieded\n");
		ret = -ENXIO;
		goto release_mem;
	}

	if (!request_mem_region
	    (res_mem->start, resource_size(res_mem), DRIVER_NAME)) {
		pr_err("Otg controller already in use\n");
		ret = -EBUSY;
		goto release_mem;
	}

	otg_host->regs_otg =
	    (struct sp_regs_otg *)ioremap_nocache(res_mem->start,
						    resource_size(res_mem));
	if (!otg_host->regs_otg) {
		ret = -EBUSY;
		goto err_release_region;
	}

	otg_debug("@@@ otg reg %d %d irq %d %x\n", res_mem->start,
		  resource_size(res_mem), otg_host->irq,
		  readl(&otg_host->regs_otg->otg_int_st));

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 1);
	if (!res_mem) {
		pr_err("otg no memory recourse provieded\n");
		ret = -ENXIO;
		goto release_mem;
	}

	otg_host->regs_moon4 =
	    (struct sp_regs_moon4 *)ioremap_nocache(res_mem->start,
						    resource_size(res_mem));
	if (!otg_host->regs_moon4) {
		ret = -EBUSY;
		goto err_release_region;
	}

	otg_host->qwork = create_singlethread_workqueue(DRIVER_NAME);
	if (!otg_host->qwork) {
		dev_dbg(&dev->dev, "cannot create workqueue %s\n", DRIVER_NAME);
		ret = -ENOMEM;
		goto err_ioumap;
	}
	INIT_WORK(&otg_host->work, sp_otg_work);

	//otg_host->notifier.notifier_call = sp_notifier_call;
	//usb_register_notify(&otg_host->notifier);

	otg_host->otg.otg->set_host = sp_set_host;
	otg_host->otg.otg->set_vbus = sp_set_vbus;
#ifdef CONFIG_USB_GADGET_SUNPLUS
	otg_host->otg.otg->set_peripheral = sp_set_peripheral;
	otg_host->otg.otg->start_hnp = sp_start_hnp;
#endif
	otg_host->otg.otg->start_srp = sp_start_srp;
	otg_host->otg.otg->usb_phy = &otg_host->otg;

	otg_host->otg.io_priv = otg_host->regs_otg;
	otg_host->otg.io_ops = &sp_phy_ios;

#ifdef	CONFIG_ADP_TIMER
	if (dev->id == 1)
		timer_setup(&otg_host->adp_timer, adp_watchdog0, 0);
	else if (dev->id == 2)
		timer_setup(&otg_host->adp_timer, adp_watchdog1, 0);
#endif

	usb_set_transceiver_sp(&otg_host->otg, dev->id - 1);

	otg_hw_init(otg_host);
	otg_hsm_init(otg_host);

	if (request_irq
	    (otg_host->irq, otg_irq, IRQF_SHARED, DRIVER_NAME, otg_host) != 0) {
		printk(KERN_NOTICE "OTG: Request irq fail\n");
		goto err_ioumap;
	}

#ifdef CONFIG_USB_GADGET_SUNPLUS
	#ifdef CONFIG_GADGET_USB0
 	if (otg_host->id == 1) {
		sp_otg0_host->hnp_polling_timer = kthread_create(hnp_polling_watchdog, sp_otg0_host, "hnp_polling");
		wake_up_process(sp_otg0_host->hnp_polling_timer);
 	}
	#else
 	if (otg_host->id == 2) {
		sp_otg1_host->hnp_polling_timer = kthread_create(hnp_polling_watchdog, sp_otg1_host, "hnp_polling");
		wake_up_process(sp_otg1_host->hnp_polling_timer);
 	}
	#endif
#endif

	ENABLE_OTG_INT(&otg_host->regs_otg->otg_init_en);

	platform_set_drvdata(dev, otg_host);

	if (otg_host->fsm.id == 0) {
		sp_otg_update_transceiver(otg_host);
	}

	return 0;

err_ioumap:
	iounmap(otg_host->regs_otg);
err_release_region:
	release_mem_region(res_mem->start, resource_size(res_mem));
release_mem:
	kfree(otg_host);

	return ret;
}
EXPORT_SYMBOL_GPL(sp_otg_probe);

int sp_otg_remove(struct platform_device *dev)
{
	struct resource *res_mem;
	struct sp_otg *otg_host = platform_get_drvdata(dev);
#ifdef CONFIG_USB_GADGET_SUNPLUS
	int err = 0;
#endif

	if (otg_host->qwork) {
		flush_workqueue(otg_host->qwork);
		destroy_workqueue(otg_host->qwork);
	}
#ifdef	CONFIG_ADP_TIMER
	del_timer_sync(&otg_host->adp_timer);
#endif

#ifdef CONFIG_USB_GADGET_SUNPLUS
	#ifdef CONFIG_GADGET_USB0
	if (sp_otg0_host->hnp_polling_timer) {
		err = kthread_stop(sp_otg0_host->hnp_polling_timer);
		if (err) {
			otg_debug("kthread_stop failed: %d\n", err);
		} else {
			sp_otg0_host->hnp_polling_timer = NULL;
		}
	}
	#else
	if (sp_otg1_host->hnp_polling_timer) {
		err = kthread_stop(sp_otg1_host->hnp_polling_timer);
		if (err) {
			otg_debug("kthread_stop failed: %d\n", err);
		} else {
			sp_otg1_host->hnp_polling_timer = NULL;
		}
	}
	#endif
#endif

	free_irq(otg_host->irq, otg_host);
	iounmap(otg_host->regs_otg);

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		pr_err("otg  release get recourse fail\n");
		goto free_mem;
	}

	release_mem_region(res_mem->start, resource_size(res_mem));

free_mem:
	kfree(otg_host->otg.otg);
	kfree(otg_host);

	return 0;
}
EXPORT_SYMBOL_GPL(sp_otg_remove);

int sp_otg_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sp_otg_suspend);

int sp_otg_resume(struct platform_device *dev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sp_otg_resume);
