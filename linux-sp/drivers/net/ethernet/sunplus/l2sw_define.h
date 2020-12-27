#ifndef __L2SW_DEFINE_H__
#define __L2SW_DEFINE_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/of_address.h>
#include <linux/of_mdio.h>


//#define CONFIG_PM
#define INTERRUPT_IMMEDIATELY
//#define RX_POLLING

#ifdef CONFIG_SOC_I143
//#define ZEBU_XTOR

#ifdef ZEBU_XTOR
// mac_force_mode0[11:10]: force_gmii_en[1:0]   = 0x3 (enable force function)
// mac_force_mode0[17:16]: force_gmii_spd0[1:0] = 0x3 (1G)
// mac_force_mode0[19:18]: force_gmii_spd1[1:0] = 0x3 (1G)
// mac_force_mode0[27:26]: force_gmii_dpx[1:0]  = 0x3 (no force duplex)
#define MAC_FORCE_MODE0 0x0c0f0c00
// mac_force_mode1[17:16]: force_gmii_xfc0[1:0] = 0x3 (full duplex and tx flow control)
// mac_force_mode1[19:18]: force_gmii_xfc1[1:0] = 0x3 (full duplex and tx flow control)
// mac_force_mode1[27:26]: force_gmii_link[1:0] = 0x3 (link up)
#define MAC_FORCE_MODE1 0x0c0f0000
#endif
#endif
/**********************************************************
 * Debug Macros
 **********************************************************/
#define ETH_ERR(fmt, arg...)            printk(KERN_ERR     "[L2SW]" fmt, ##arg)
#define ETH_WARNING(fmt, arg...)        printk(KERN_WARNING "[L2SW]" fmt, ##arg)
#define ETH_NOTICE(fmt, arg...)         printk(KERN_NOTICE  "[L2SW]" fmt, ##arg)
#define ETH_INFO(fmt, arg...)           printk(KERN_INFO    "[L2SW]" fmt, ##arg)
#define ETH_DEBUG(fmt, arg...)          pr_debug("[L2SW]" fmt, ##arg)


//define MAC interrupt status bit
#define MAC_INT_DAISY_MODE_CHG          (1<<31)
#ifdef CONFIG_SOC_I143
#define MAC_INT_MEM_TEST_DONE           (1<<29)
#define MAC_INT_TCPUDP_CHKSUM_ERR       (1<<24)
#endif
#define MAC_INT_IP_CHKSUM_ERR           (1<<23)
#define MAC_INT_WDOG_TIMER1_EXP         (1<<22)
#define MAC_INT_WDOG_TIMER0_EXP         (1<<21)
#define MAC_INT_INTRUDER_ALERT          (1<<20)
#define MAC_INT_PORT_ST_CHG             (1<<19)
#define MAC_INT_BC_STORM                (1<<18)
#define MAC_INT_MUST_DROP_LAN           (1<<17)
#define MAC_INT_GLOBAL_QUE_FULL         (1<<16)
#define MAC_INT_TX_SOC_PAUSE_ON         (1<<15)
#define MAC_INT_RX_SOC_QUE_FULL         (1<<14)
#define MAC_INT_TX_LAN1_QUE_FULL        (1<<9)
#define MAC_INT_TX_LAN0_QUE_FULL        (1<<8)
#define MAC_INT_RX_L_DESCF              (1<<7)
#define MAC_INT_RX_H_DESCF              (1<<6)
#define MAC_INT_RX_DONE_L               (1<<5)
#define MAC_INT_RX_DONE_H               (1<<4)
#define MAC_INT_TX_DONE_L               (1<<3)
#define MAC_INT_TX_DONE_H               (1<<2)
#define MAC_INT_TX_DES_ERR              (1<<1)
#define MAC_INT_RX_DES_ERR              (1<<0)

#ifdef CONFIG_SOC_SP7021
#define MAC_INT_RX                      (MAC_INT_RX_DONE_H | MAC_INT_RX_DONE_L | MAC_INT_RX_DES_ERR)
#define MAC_INT_TX                      (MAC_INT_TX_DONE_L | MAC_INT_TX_DONE_H | MAC_INT_TX_DES_ERR)
#define MAC_INT_MASK_DEF                (MAC_INT_DAISY_MODE_CHG | MAC_INT_IP_CHKSUM_ERR | MAC_INT_WDOG_TIMER1_EXP | \
					MAC_INT_WDOG_TIMER0_EXP | MAC_INT_INTRUDER_ALERT | MAC_INT_BC_STORM | \
					MAC_INT_MUST_DROP_LAN | MAC_INT_GLOBAL_QUE_FULL | MAC_INT_TX_SOC_PAUSE_ON | \
					MAC_INT_RX_SOC_QUE_FULL | MAC_INT_TX_LAN1_QUE_FULL | MAC_INT_TX_LAN0_QUE_FULL | \
					MAC_INT_RX_L_DESCF | MAC_INT_RX_H_DESCF)
#else
// Note that due to a hardware bug, TCPUDP_APPEND_ERROR cannot be masked. Must handle it.
#define MAC_INT_RX                      (MAC_INT_RX_DONE_L | MAC_INT_RX_DONE_H | MAC_INT_RX_L_DESCF | MAC_INT_RX_H_DESCF)
#define MAC_INT_RX_MASK_DEF             (0)
#define MAC_INT_TX                      (MAC_INT_TCPUDP_CHKSUM_ERR | MAC_INT_PORT_ST_CHG | MAC_INT_TX_DONE_L | \
					MAC_INT_TX_DONE_H | MAC_INT_TX_DES_ERR | MAC_INT_RX_DES_ERR)
#define MAC_INT_TX_MASK_DEF             (MAC_INT_DAISY_MODE_CHG | MAC_INT_MEM_TEST_DONE | \
					MAC_INT_TCPUDP_CHKSUM_ERR | MAC_INT_IP_CHKSUM_ERR | \
					MAC_INT_WDOG_TIMER1_EXP | MAC_INT_WDOG_TIMER0_EXP | MAC_INT_INTRUDER_ALERT | \
					MAC_INT_BC_STORM | MAC_INT_MUST_DROP_LAN | MAC_INT_GLOBAL_QUE_FULL | \
					MAC_INT_TX_SOC_PAUSE_ON | MAC_INT_RX_SOC_QUE_FULL | MAC_INT_TX_LAN1_QUE_FULL | \
					MAC_INT_TX_LAN0_QUE_FULL)
#define MAC_INT_MASK_DEF                (MAC_INT_RX_MASK_DEF | MAC_INT_TX_MASK_DEF)
#endif

/*define port ability*/
#ifdef CONFIG_SOC_SP7021
#define PORT_ABILITY_LINK_ST_P1         (1<<25)
#define PORT_ABILITY_LINK_ST_P0         (1<<24)
#else
#define PORT_ABILITY_LINK_ST_P1         (1<<28)
#define PORT_ABILITY_LINK_ST_P0         (1<<27)
#endif


/*define PHY command bit*/
#define PHY_WT_DATA_MASK                0xffff0000
#define PHY_RD_CMD                      0x00004000
#define PHY_WT_CMD                      0x00002000
#define PHY_REG_MASK                    0x00001f00
#define PHY_ADR_MASK                    0x0000001f

/*define PHY status bit*/
#define PHY_RD_DATA_MASK                0xffff0000
#define PHY_RD_RDY                      (1<<1)
#define PHY_WT_DONE                     (1<<0)

/*define other register bit*/
#define RX_MAX_LEN_MASK                 0x00011000
#define ROUTE_MODE_MASK                 0x00000060
#define POK_INT_THS_MASK                0x000E0000
#define VLAN_TH_MASK                    0x00000007


/*define tx descriptor bit*/
#define OWN_BIT                         (1<<31)
#define FS_BIT                          (1<<25)
#define LS_BIT                          (1<<24)
#define LEN_MASK                        0x000007FF
#define PKTSP_MASK                      0x00007000
#define PKTSP_PORT1                     0x00001000
#define TO_VLAN_MASK                    0x0003F000
#define TO_VLAN_GROUP1                  0x00002000

#define EOR_BIT                         (1<<31)
#ifdef CONFIG_SOC_I143
#define IP_CHKSUM_APPEND                (1<<30)
#define TCP_UDP_CHKSUM_APPEND           (1<<29)
#endif


/*define rx descriptor bit*/
#define ERR_CODE                        (0xf<<26)
#ifdef CONFIG_SOC_SP7021
#define RX_TCP_UDP_CHKSUM_BIT           (1<<23)
#define RX_IP_CHKSUM_BIT                (1<<18)

#define OWC_BIT                         (1<<31)
#define TXOK_BIT                        (1<<26)
#define LNKF_BIT                        (1<<25)
#define BUR_BIT                         (1<<22)
#define TWDE_BIT                        (1<<20)
#define CC_MASK                         0x000f0000
#define TBE_MASK                        0x00070000
#else
#define RX_TCP_UDP_CHKSUM_FAIL          (1<<23)
#define RX_IP_CHKSUM_FAIL               (1<<18)
#if 0
#define OWC_BIT                         (1<<30)
#define BUR_BIT                         (1<<29)
#define LNKF_BIT                        (1<<28)
#define TWDE_BIT                        (1<<27)
#define CC_MASK                         (0xf<<16)
#endif
#endif

// Address table search
#define MAC_ADDR_LOOKUP_IDLE            (1<<2)
#define MAC_SEARCH_NEXT_ADDR            (1<<1)
#define MAC_BEGIN_SEARCH_ADDR           (1<<0)

// Address table search
#define MAC_HASK_LOOKUP_ADDR_MASK       (0x3ff<<22)
#define MAC_AT_TABLE_END                (1<<1)
#define MAC_AT_DATA_READY               (1<<0)


#define MAC_PHY_ADDR                    0x01    /* define by hardware */

/*config descriptor*/
#define TX_DESC_NUM                     16
#define MAC_GUARD_DESC_NUM              2
#ifdef CONFIG_SOC_SP7021
#define RX_QUEUE0_DESC_NUM              16
#define RX_QUEUE1_DESC_NUM              16
#else
#define RX_QUEUE0_DESC_NUM              64
#define RX_QUEUE1_DESC_NUM              64
#endif
#define TX_DESC_QUEUE_NUM               1
#define RX_DESC_QUEUE_NUM               2

#define MAC_TX_BUFF_SIZE                1536
#define MAC_RX_LEN_MAX                  2047

#define DESC_ALIGN_BYTE                 32
#define RX_OFFSET                       0
#define TX_OFFSET                       0

#define ETHERNET_MAC_ADDR_LEN           6


struct mac_desc {
	volatile u32 cmd1;
	volatile u32 cmd2;
	volatile u32 addr1;
	volatile u32 addr2;
};

struct skb_info {
	struct sk_buff *skb;
	u32 mapping;
	u32 len;
};

struct l2sw_common {
	struct net_device *net_dev;
	struct platform_device *pdev;
	int dual_nic;
	int sa_learning;

	void *desc_base;
	dma_addr_t desc_dma;
	s32 desc_size;
	struct clk *clk;
	struct reset_control *rstc;
#ifdef CONFIG_SOC_SP7021
	int irq;
#else
	int irq[4];
#endif

	struct mac_desc *rx_desc[RX_DESC_QUEUE_NUM];
	struct skb_info *rx_skb_info[RX_DESC_QUEUE_NUM];
	volatile u32 rx_pos[RX_DESC_QUEUE_NUM];
	u32 rx_desc_num[RX_DESC_QUEUE_NUM];
	u32 rx_desc_buff_size;

	struct mac_desc *tx_desc;
	struct skb_info tx_temp_skb_info[TX_DESC_NUM];
	volatile u32 tx_done_pos;
	volatile u32 tx_pos;
	volatile u32 tx_desc_full;

	struct mii_bus *mii_bus;
	struct phy_device *phy_dev;

#ifndef INTERRUPT_IMMEDIATELY
	struct tasklet_struct rx_tasklet;
	struct tasklet_struct tx_tasklet;
#endif

	spinlock_t lock;
	spinlock_t ioctl_lock;
	struct mutex store_mode;

#ifdef RX_POLLING
	struct napi_struct napi;
#endif

	struct device_node *phy1_node;
	struct device_node *phy2_node;
	u8 phy1_addr;
	u8 phy2_addr;

	u8 enable;
};

struct l2sw_mac {
	struct platform_device *pdev;
	struct net_device *net_dev;
	struct l2sw_common *comm;
	struct net_device *next_netdev;

	struct net_device_stats dev_stats;

	u8 mac_addr[ETHERNET_MAC_ADDR_LEN];

	u8 lan_port;
	u8 to_vlan;
	u8 cpu_port;
	u8 vlan_id;
};


#endif

