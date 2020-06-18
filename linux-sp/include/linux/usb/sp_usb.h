#ifndef __SP_USB_H
#define __SP_USB_H

#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/io_map.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>


#define RF_MASK_V(_mask, _val)       (((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask)        (((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask)        (((_mask) << 16) | 0)

#define USB_PORT0_ID						0
#define USB_PORT1_ID						1
#define USB_PORT_NUM						3

#define	VBUS_GPIO_CTRL_0					90
#define	VBUS_GPIO_CTRL_1					91

#define CDP_MODE_VALUE						0
#define DCP_MODE_VALUE						1
#define SDP_MODE_VALUE						2
#define UPHY1_OTP_DISC_LEVEL_OFFSET				5
#define OTP_DISC_LEVEL_TEMP					0x16
#define DISC_LEVEL_DEFAULT					0x0B
#define OTP_DISC_LEVEL_BIT					0x1F
#define GET_BC_MODE						0xFF00
#define APHY_PROBE_CTRL						0x38

#define PORT0_ENABLED						(1 << 0)
#define PORT1_ENABLED						(1 << 1)
#define POWER_SAVING_SET					(1 << 5)
#define ECO_PATH_SET						(1 << 6)
#define	UPHY_DISC_0						(1 << 2)
#define APHY_PROBE_CTRL_MASK					0x38

#define USB_RESET_OFFSET					0x5C
#define PIN_MUX_CTRL						0x8C
#define USBC_CTL_OFFSET						0x244

#define UPHY0_CTL0_OFFSET					0x248
#define UPHY0_CTL1_OFFSET					0x24C
#define UPHY0_CTL2_OFFSET					0x250
#define UPHY0_CTL3_OFFSET					0x254
#define UPHY1_CTL0_OFFSET					0x258
#define UPHY1_CTL1_OFFSET					0x25C
#define UPHY1_CTL2_OFFSET					0x260
#define UPHY1_CTL3_OFFSET					0x264

#define CLK_REG_OFFSET						0xc

#define	POWER_SAVING_OFFSET					0x4

#define DISC_LEVEL_OFFSET					0x1c
#define	ECO_PATH_OFFSET						0x24
#define	UPHY_DISC_OFFSET					0x28
#define BIT_TEST_OFFSET						0x10
#define CDP_REG_OFFSET						0x40
#define	DCP_REG_OFFSET						0x44
#define	UPHY_INTR_OFFSET					0x4c
#define APHY_PROBE_OFFSET					0x5c
#define CDP_OFFSET						0

#define	UPHY_DEBUG_SIGNAL_REG_OFFSET				0x30
#define UPHY_INTER_SIGNAL_REG_OFFSET				0xc
#define USB_OTP_REG						0x9c00af18

#define PORT_OWNERSHIP						0x00002000
#define CURRENT_CONNECT_STATUS					0x00000001
#define EHCI_CONNECT_STATUS_CHANGE				0x00000002
#define OHCI_CONNECT_STATUS_CHANGE				0x00010000

#define WAIT_TIME_AFTER_RESUME					25
#define ELAPSE_TIME_AFTER_SUSPEND				15000
#define SEND_SOF_TIME_BEFORE_SUSPEND				15000
#define SEND_SOF_TIME_BEFORE_SEND_IN_PACKET			15000


extern u32 bc_switch;
extern u32 cdp_cfg16_value;
extern u32 cdp_cfg17_value;
extern u32 dcp_cfg16_value;
extern u32 dcp_cfg17_value;
extern u32 sdp_cfg16_value;
extern u32 sdp_cfg17_value;

extern int uphy0_irq_num;
extern int uphy1_irq_num;
extern void __iomem *uphy0_base_addr;
extern void __iomem *uphy1_base_addr;
#if 0
extern u32 usb_vbus_gpio[USB_PORT_NUM];
#endif

extern u8 max_topo_level;
extern bool tid_test_flag;
extern u8 sp_port_enabled;
extern uint accessory_port_id;
extern bool enum_rx_active_flag[USB_PORT_NUM];
extern struct semaphore enum_rx_active_reset_sem[USB_PORT_NUM];
#ifdef CONFIG_USB_SUNPLUS_OTG
extern struct timer_list hnp_polling_timer;;
#endif

typedef enum {
	eHW_GPIO_FIRST_FUNC = 0,
	eHW_GPIO_FIRST_GPIO = 1,
	eHW_GPIO_FIRST_NULL
} eHW_GPIO_FIRST;

typedef enum {
	eHW_GPIO_IOP = 0,
	eHW_GPIO_RISC = 1,
	eHW_GPIO_MASTER_NULL
} eHW_GPIO_Master;

typedef enum {
	eHW_GPIO_IN = 0,
	eHW_GPIO_OUT = 1,
	eHW_GPIO_OE_NULL
} eHW_GPIO_OE;

typedef enum {
	eHW_GPIO_STS_LOW = 0,
	eHW_GPIO_STS_HIGH = 1,
	eHW_GPIO_STS_NULL
} eHW_IO_STS;

#if 0
#define	ENABLE_VBUS_POWER(port) gpio_set_value(usb_vbus_gpio[port], eHW_GPIO_STS_LOW)

#define	DISABLE_VBUS_POWER(port) gpio_set_value(usb_vbus_gpio[port], eHW_GPIO_STS_HIGH)
#else
#define	ENABLE_VBUS_POWER(port)

#define	DISABLE_VBUS_POWER(port)
#endif


static inline void uphy_force_disc(int en, int port)
{
	void __iomem *reg_addr;
	u32 uphy_val;

	if (port > USB_PORT1_ID)
		printk("-- err port num %d\n", port);

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	uphy_val = readl(reg_addr + UPHY_DISC_OFFSET);
	if (en) {
		uphy_val |= UPHY_DISC_0;
	} else {
		uphy_val &= ~UPHY_DISC_0;
	}

	writel(uphy_val, reg_addr + UPHY_DISC_OFFSET);
}

#define Reset_Usb_Power(port) do {	\
	DISABLE_VBUS_POWER(port);	\
	uphy_force_disc(1, (port));	\
	msleep(500);			\
	uphy_force_disc(0, (port));	\
	ENABLE_VBUS_POWER(port);	\
} while (0)

static inline int get_uphy_swing(int port)
{
	int uphy_ctl_offset = port ? UPHY1_CTL2_OFFSET : UPHY0_CTL2_OFFSET;
	void __iomem *reg_addr = (void __iomem *)B_SYSTEM_BASE;
	u32 val;

	val = readl(reg_addr + uphy_ctl_offset);
	return (val >> 8) & 0xFF;

	return 0;
}

static inline int set_uphy_swing(u32 swing, int port)
{
	int uphy_ctl_offset = port ? UPHY1_CTL2_OFFSET : UPHY0_CTL2_OFFSET;
	void __iomem *reg_addr = (void __iomem *)B_SYSTEM_BASE;

	writel(RF_MASK_V_CLR(0x3F << 8), reg_addr + uphy_ctl_offset);
	writel(RF_MASK_V_SET((swing & 0x3F) << 8), reg_addr + uphy_ctl_offset);
	writel(RF_MASK_V_SET(1 << 15), reg_addr + uphy_ctl_offset);

	return 0;
}

static inline int get_disconnect_level(int port)
{
	void __iomem *reg_addr;
	u32 val;

	if (port > USB_PORT1_ID) {
		return -1;
	}

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	val = readl(reg_addr + DISC_LEVEL_OFFSET);

	return val & 0x1F;
}

static inline int set_disconnect_level(u32 disc_level, int port)
{
	void __iomem *reg_addr;
	u32 val;

	if (port > USB_PORT1_ID) {
		return -1;
	}

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	val = readl(reg_addr + DISC_LEVEL_OFFSET);
	val = (val & ~0x1F) | disc_level;
	writel(val, reg_addr + DISC_LEVEL_OFFSET);

	return 0;
}

/*
 * return: 0 = device, 1 = host
 */
static inline int get_uphy_owner(int port)
{
	void __iomem *reg_addr = (void __iomem *)B_SYSTEM_BASE;
	u32 val;

	val = readl(reg_addr + USBC_CTL_OFFSET);

	return (val & (1 << (port * 8 + 5))) ? 1 : 0;
}

static inline void reset_uphy(int port){
	void __iomem *reg_addr = (void __iomem *)B_SYSTEM_BASE;

	writel(RF_MASK_V_SET(1 << (13 + port)), reg_addr + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << (13 + port)), reg_addr + USB_RESET_OFFSET);
	writel(RF_MASK_V_SET(1 << (10 + port)), reg_addr + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << (10 + port)), reg_addr + USB_RESET_OFFSET);
}

static inline void reinit_uphy(int port)
{
	void __iomem *reg_addr;
	u32 val;

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;

	val = readl(reg_addr + ECO_PATH_OFFSET);
	val &= ~(ECO_PATH_SET);
	writel(val, reg_addr + ECO_PATH_OFFSET);
	val = readl(reg_addr + POWER_SAVING_OFFSET);
	val &= ~(POWER_SAVING_SET);
	writel(val, reg_addr + POWER_SAVING_OFFSET);

#ifdef CONFIG_USB_BC
	writel(0x19, reg_addr + CDP_REG_OFFSET);
	writel(0x92, reg_addr + DCP_REG_OFFSET);
	writel(0x17, reg_addr + UPHY_INTER_SIGNAL_REG_OFFSET);
#endif
}

#endif	/* __SP_USB_H */
