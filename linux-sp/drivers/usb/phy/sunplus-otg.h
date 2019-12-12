#ifndef __SUNPLUS_OTG_H_
#define __SUNPLUS_OTG_H_

#include <linux/usb/phy.h>
#include "otg_fsm.h"

#define otg_debug(fmt, args...)               printk(KERN_DEBUG "#@#OTG: "fmt, ##args)
//#define otg_debug(fmt, args...)

//#define OTG_TEST

#define       CONFIG_ADP_TIMER
#define	ADP_TIMER_FREQ	(5*HZ)

#define		WORD_MODE_MASK	(0x03)
#define		OTG_20			(0x0)
#define		OTG_12			(0x01)
#define		OTG_HOST		(0x02)
#define		OTG_DEVICE		(0x03)

#define		OTG_SRP		(1 << 2)
#define		OTG_ADP		(1 << 3)
#define		OTG_SIM     (1 << 4)

#define		ENA_SRP(reg)		((reg) |=  OTG_SRP)
#define		DIS_SRP(reg)		((reg) &= ~OTG_SRP)
#define		ENA_ADP(reg)		((reg) |=  OTG_ADP)
#define		DIS_ADP(reg)		((reg) &= ~OTG_ADP)
#define		ENA_SIM(reg)		((reg) |=  OTG_SIM)
#define		DIS_SIM(reg)		((reg) &= ~OTG_SIM)

#define		B_HNP_EN_BIT			(1 << 5)
#define		B_VBUS_REQ			(1 << 4)
#define		A_CLE_ERR_BIT			(1 << 3)
#define		A_SET_B_HNP_EN_BIT	(1 << 2)
#define		A_BUS_REQ_BIT			(1 << 1)
#define		A_BUS_DROP_BIT		(1 << 0)

#define		B_HNP_EN(reg)			((reg)|= (1 << 5))
#define		B_BUS_REQ(reg)			((reg)|= (1 << 4))
#define		A_CLE_ERR(reg)			((reg)|= (1 << 3))
#define		A_SET_B_HNP_EN(reg)		((reg)|= (1 << 2))
#define		A_BUS_REQ(reg)			((reg)|= (1 << 1))
#define		A_BUS_DROP(reg)			((reg)|= (1 << 0))

#define		ID_PIN					(1 << 16)
#define		INT_MASK				(0x3ff)
#define		ADP_CHANGE_IF			(1 << 9)
#define		A_SRP_DET_IF			(1 << 8)
#define		B_AIDL_BDIS_IF			(1 << 7)
#define		A_BIDL_ADIS_IF			(1 << 6)
#define		A_AIDS_BDIS_TOUT_IF 	(1 << 5)
#define		B_SRP_FAIL_IF			(1 << 4)
#define		BDEV_CONNT_TOUT_IF 	(1 << 3)
#define		VBUS_RISE_TOUT_IF		(1 << 2)
#define		ID_CHAGE_IF			(1 << 1)
#define		OVERCURRENT_IF 		(1 << 0)

#define 	ENABLE_OTG_INT(x)		iowrite32(0x3ff, x)

extern int sp_otg_probe(struct platform_device *);
extern int sp_otg_remove(struct platform_device *);
extern int sp_otg_suspend(struct platform_device *, pm_message_t);
extern int sp_otg_resume(struct platform_device *);

struct sp_otg {
	struct usb_phy otg;

	struct sp_regs_otg __iomem *regs_otg;
	struct sp_regs_udsdi __iomem *regs_udsdi;
	struct sp_regs_moon4 __iomem *regs_moon4;

	int irq;
	int id;

	struct work_struct work;
	struct workqueue_struct *qwork;

	struct notifier_block notifier;
	struct otg_fsm fsm;

#ifdef	CONFIG_ADP_TIMER
	struct timer_list adp_timer;
#endif
};

struct sp_regs_otg {
	u32 mode_select;
	u32 otg_device_ctrl;
	u32 otg_init_en;
	u32 otg_int_st;

	u32 a_wait_vrise_tmr;
	u32 b_send_srp_tmr;	//5
	u32 b_se0_srp_tmr;
	u32 b_data_pls_tmr;
	u32 b_srp_fail_tmr;
	u32 b_svld_bcon_tmr;	//9
	u32 b_aidl_bdis_tmr;
	u32 a_bdis_acon_tmr;
	u32 ldis_dschg_tmr;
	u32 a_bcon_sdb_tmr;
	u32 a_bcon_ldb_tmr;
	u32 a_adp_prb_tmr;	//15
	u32 b_adp_prb_tmr;
	u32 b_adp_detach_tmr;
	u32 adp_chng_precision;
	u32 a_wait_vfall_tmr;
	u32 a_wait_bcon_tmr;
	u32 a_aidl_bdis_tmr;	//21
	u32 b_ase0_brst_tmr;
	u32 a_bidl_adis_tmr;
	u32 adp_chrg_time;
	u32 vbus_pules_time;
	u32 a_bcon_sdb_win;	//26
	u32 otg_debug_reg;
	u32 adp_debug_reg;
};

struct sp_regs_udsdi {
	u32 udccs;
};

struct sp_regs_moon4 {
	u32 mo4_pllsp_ctl_0;  //0x200
	u32 mo4_pllsp_ctl_1;  //0x204
	u32 mo4_pllsp_ctl_2;  //0x208
	u32 mo4_pllsp_ctl_3;  //0x20c
	u32 mo4_pllsp_ctl_4;  //0x210
	u32 mo4_pllsp_ctl_5;  //0x214
	u32 mo4_pllsp_ctl_6;  //0x218
	u32 mo4_plla_ctl_0;   //0x21c
	u32 mo4_plla_ctl_1;   //0x220
	u32 mo4_plla_ctl_2;   //0x224
	u32 mo4_plla_ctl_3;   //0x228
	u32 mo4_plla_ctl_4;   //0x22c
	u32 mo4_plle_ctl;     //0x230
	u32 mo4_pllf_ctl;     //0x234
	u32 mo4_plltv_ctl_0;  //0x238
	u32 mo4_plltv_ctl_1;  //0x23c
	u32 mo4_plltv_ctl_2;  //0x240
	u32 mo4_usbc_ctl;     //0x244
	u32 mo4_uphy0_ctl_0;  //0x248
	u32 mo4_uphy0_ctl_1;
	u32 mo4_uphy0_ctl_2;
	u32 mo4_uphy0_ctl_3;
	u32 mo4_uphy1_ctl_0;
	u32 mo4_uphy1_ctl_1;
	u32 mo4_uphy1_ctl_2;
	u32 mo4_uphy1_ctl_3;
	u32 mo4_pllsys;
	u32 mo_clk_sel0;
	u32 mo_probe_sel;
	u32 mo4_misc_ctl_0;
	u32 mo4_uphy0_sts;
	u32 otp_sp;
};

/* Context: can sleep */
static inline int
otg_start_hnp(struct usb_otg *otg)
{
	if (otg && otg->start_hnp)
		return otg->start_hnp(otg);

	return -ENOTSUPP;
}

/* Context: can sleep */
static inline int
otg_set_vbus(struct usb_otg *otg, bool enabled)
{
	if (otg && otg->set_vbus)
		return otg->set_vbus(otg, enabled);

	return -ENOTSUPP;
}

/* for HCDs */
static inline int
otg_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	if (otg && otg->set_host)
		return otg->set_host(otg, host);

	return -ENOTSUPP;
}

/* for usb peripheral controller drivers */

/* Context: can sleep */
static inline int
otg_set_peripheral(struct usb_otg *otg, struct usb_gadget *periph)
{
	if (otg && otg->set_peripheral)
		return otg->set_peripheral(otg, periph);

	return -ENOTSUPP;
}

static inline int
otg_start_srp(struct usb_otg *otg)
{
	if (otg && otg->start_srp)
		return otg->start_srp(otg);

	return -ENOTSUPP;
}

/* for OTG controller drivers (and maybe other stuff) */
extern int usb_bus_start_enum(struct usb_bus *bus, unsigned port_num);

/**
 * usb_get_dr_mode - Get dual role mode for given device
 * @dev: Pointer to the given device
 *
 * The function gets phy interface string from property 'dr_mode',
 * and returns the correspondig enum usb_dr_mode
 */
extern enum usb_dr_mode usb_get_dr_mode(struct device *dev);

void sp_otg_update_transceiver(struct sp_otg *);

struct usb_phy *usb_get_transceiver_sp(int bus_num);


#endif
