/*
 * DMA driver for Xilinx ZynqMP DMA Engine
 *
 * Copyright (C) 2016 Xilinx, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/bitops.h>
#include <linux/dmapool.h>
#include <linux/dma/xilinx_dma.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_dma.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io-64-nonatomic-lo-hi.h>

#include "../dmaengine.h"

#define DEVICE_NAME	"sunplus,sp7021-cbdma"

#define CB_DMA_REG_NAME	"cb_dma"

/* Register Offsets */
#define SP_DMA_HW_VER				0x00
#define SP_DMA_CONFIG				0x04
#define SP_DMA_LENGTH				0x08
#define SP_DMA_SRC_ADR				0x0c
#define SP_DMA_DES_ADR				0x10
#define SP_DMA_INT_FLAG				0x14
#define SP_DMA_INT_EN				0x18
#define SP_DMA_MEMSET_VAL			0x1c
#define SP_DMA_SDRAM_SIZE_CONFIG	0x20
#define SP_DMA_ILLEGLE_RECORD		0x24
#define SP_DMA_SG_IDX				0x28
#define SP_DMA_SG_CONFIG			0x2c
#define SP_DMA_SG_LENGTH			0x30
#define SP_DMA_SG_SRC_ADR			0x34
#define SP_DMA_SG_DES_ADR			0x38
#define SP_DMA_SG_MEMSET_VAL		0x3c
#define SP_DMA_SG_EN_GO				0x40
#define SP_DMA_SG_LP_MODE			0x44
#define SP_DMA_SG_LP_SRAM_START		0x48
#define SP_DMA_SG_LP_SRAM_SIZE		0x4c
#define SP_DMA_SG_CHK_MODE			0x50
#define SP_DMA_SG_CHK_SUM			0x54
#define SP_DMA_SG_CHK_XOR			0x58
#define SP_DMA_RSV_23				0x5c
#define SP_DMA_RSV_24				0x60


#define CBDMA_CONFIG_CMD_QUE_DEPTH_1	(0x0 << 16)
#define CBDMA_CONFIG_CMD_QUE_DEPTH_2	(0x1 << 16)
#define CBDMA_CONFIG_CMD_QUE_DEPTH_4	(0x2 << 16)
#define CBDMA_CONFIG_CMD_QUE_DEPTH_8	(0x3 << 16)

#define CBDMA_CONFIG_DEFAULT	CBDMA_CONFIG_CMD_QUE_DEPTH_8
#define CBDMA_CONFIG_GO		(0x01 << 8)
#define CBDMA_CONFIG_MEMSET	(0x00 << 0)
#define CBDMA_CONFIG_WR		(0x01 << 0)
#define CBDMA_CONFIG_RD		(0x02 << 0)
#define CBDMA_CONFIG_CP		(0x03 << 0)


#define CBDMA_INT_FLAG_COPY_ADDR_OVLAP	(1 << 6)
#define CBDMA_INT_FLAG_DAM_SDRAMB_OF	(1 << 5)
#define CBDMA_INT_FLAG_DMA_SDRAMA_OF	(1 << 4)
#define CBDMA_INT_FLAG_DMA_SRAM_OF		(1 << 3)
#define CBDMA_INT_FLAG_CBSRAM_OF		(1 << 2)
#define CBDMA_INT_FLAG_ADDR_CONFLICT	(1 << 1)
#define CBDMA_INT_FLAG_DONE				(1 << 0)

#define CBDMA_SG_CFG_NOT_LAST	(0x00 << 2)
#define CBDMA_SG_CFG_LAST	(0x01 << 2)
#define CBDMA_SG_CFG_MEMSET	(0x00 << 0)
#define CBDMA_SG_CFG_WR		(0x01 << 0)
#define CBDMA_SG_CFG_RD		(0x02 << 0)
#define CBDMA_SG_CFG_CP		(0x03 << 0)
#define CBDMA_SG_EN_GO_EN	(0x01 << 31)
#define CBDMA_SG_EN_GO_GO	(0x01 << 0)
#define CBDMA_SG_LP_MODE_LP	(0x01 << 0)
#define CBDMA_SG_LP_SZ_1KB	(0 << 0)
#define CBDMA_SG_LP_SZ_2KB	(1 << 0)
#define CBDMA_SG_LP_SZ_4KB	(2 << 0)
#define CBDMA_SG_LP_SZ_8KB	(3 << 0)
#define CBDMA_SG_LP_SZ_16KB	(4 << 0)
#define CBDMA_SG_LP_SZ_32KB	(5 << 0)
#define CBDMA_SG_LP_SZ_64KB	(6 << 0)

/* Interrupt flag registers bit field definitions */
#define CBDMA_DMA_DONE				BIT(0)
#define CBDMA_ADDR_CONFLICT			BIT(1)
#define CDDMA_CBSRAM_OVFLOW			BIT(2)
#define CDDMA_DMA_SRAM_OVFLOW		BIT(3)
#define CDDMA_DMA_SDRAMA_OVFLOW		BIT(4)
#define CDDMA_DMA_SDRAMB_OVFLOW		BIT(5)
#define CDDMA_COPY_ADDR_OVLAP		BIT(6)

/* Interrupt flag registers bit field definitions */
#define CBDMA_DMA_DONE_EN			BIT(0)
#define RSV0						BIT(1)
#define CDDMA_CBSRAM_OVFLOW_EN		BIT(2)
#define CDDMA_DMA_SRAM_OVFLOW_EN	BIT(3)
#define CDDMA_DMA_SDRAMA_OVFLOW_EN	BIT(4)
#define CDDMA_DMA_SDRAMB_OVFLOW_EN	BIT(5)
#define CDDMA_COPY_ADDR_OVLAP_EN	BIT(6)



/* Register Offsets */
#define ZYNQMP_DMA_ISR			0x100
#define ZYNQMP_DMA_IMR			0x104
#define ZYNQMP_DMA_IER			0x108
#define ZYNQMP_DMA_IDS			0x10C
#define ZYNQMP_DMA_CTRL0		0x110
#define ZYNQMP_DMA_CTRL1		0x114
#define ZYNQMP_DMA_DATA_ATTR		0x120
#define ZYNQMP_DMA_DSCR_ATTR		0x124
#define ZYNQMP_DMA_SRC_DSCR_WRD0	0x128
#define ZYNQMP_DMA_SRC_DSCR_WRD1	0x12C
#define ZYNQMP_DMA_SRC_DSCR_WRD2	0x130
#define ZYNQMP_DMA_SRC_DSCR_WRD3	0x134
#define ZYNQMP_DMA_DST_DSCR_WRD0	0x138
#define ZYNQMP_DMA_DST_DSCR_WRD1	0x13C
#define ZYNQMP_DMA_DST_DSCR_WRD2	0x140
#define ZYNQMP_DMA_DST_DSCR_WRD3	0x144
#define ZYNQMP_DMA_SRC_START_LSB	0x158
#define ZYNQMP_DMA_SRC_START_MSB	0x15C
#define ZYNQMP_DMA_DST_START_LSB	0x160
#define ZYNQMP_DMA_DST_START_MSB	0x164
#define ZYNQMP_DMA_RATE_CTRL		0x18C
#define ZYNQMP_DMA_IRQ_SRC_ACCT		0x190
#define ZYNQMP_DMA_IRQ_DST_ACCT		0x194
#define ZYNQMP_DMA_CTRL2		0x200

/* Interrupt registers bit field definitions */
#define ZYNQMP_DMA_DONE			BIT(10)
#define ZYNQMP_DMA_AXI_WR_DATA		BIT(9)
#define ZYNQMP_DMA_AXI_RD_DATA		BIT(8)
#define ZYNQMP_DMA_AXI_RD_DST_DSCR	BIT(7)
#define ZYNQMP_DMA_AXI_RD_SRC_DSCR	BIT(6)
#define ZYNQMP_DMA_IRQ_DST_ACCT_ERR	BIT(5)
#define ZYNQMP_DMA_IRQ_SRC_ACCT_ERR	BIT(4)
#define ZYNQMP_DMA_BYTE_CNT_OVRFL	BIT(3)
#define ZYNQMP_DMA_DST_DSCR_DONE	BIT(2)
#define ZYNQMP_DMA_INV_APB		BIT(0)

/* Control 0 register bit field definitions */
#define ZYNQMP_DMA_OVR_FETCH		BIT(7)
#define ZYNQMP_DMA_POINT_TYPE_SG	BIT(6)
#define ZYNQMP_DMA_RATE_CTRL_EN		BIT(3)

/* Control 1 register bit field definitions */
#define ZYNQMP_DMA_SRC_ISSUE		GENMASK(4, 0)

/* Data Attribute register bit field definitions */
#define ZYNQMP_DMA_ARBURST		GENMASK(27, 26)
#define ZYNQMP_DMA_ARCACHE		GENMASK(25, 22)
#define ZYNQMP_DMA_ARCACHE_OFST		22
#define ZYNQMP_DMA_ARQOS		GENMASK(21, 18)
#define ZYNQMP_DMA_ARQOS_OFST		18
#define ZYNQMP_DMA_ARLEN		GENMASK(17, 14)
#define ZYNQMP_DMA_ARLEN_OFST		14
#define ZYNQMP_DMA_AWBURST		GENMASK(13, 12)
#define ZYNQMP_DMA_AWCACHE		GENMASK(11, 8)
#define ZYNQMP_DMA_AWCACHE_OFST		8
#define ZYNQMP_DMA_AWQOS		GENMASK(7, 4)
#define ZYNQMP_DMA_AWQOS_OFST		4
#define ZYNQMP_DMA_AWLEN		GENMASK(3, 0)
#define ZYNQMP_DMA_AWLEN_OFST		0

/* Descriptor Attribute register bit field definitions */
#define ZYNQMP_DMA_AXCOHRNT		BIT(8)
#define ZYNQMP_DMA_AXCACHE		GENMASK(7, 4)
#define ZYNQMP_DMA_AXCACHE_OFST		4
#define ZYNQMP_DMA_AXQOS		GENMASK(3, 0)
#define ZYNQMP_DMA_AXQOS_OFST		0

/* Control register 2 bit field definitions */
#define ZYNQMP_DMA_ENABLE		BIT(0)

/* Buffer Descriptor definitions */
#define ZYNQMP_DMA_DESC_CTRL_STOP	0x10
#define ZYNQMP_DMA_DESC_CTRL_COMP_INT	0x4
#define ZYNQMP_DMA_DESC_CTRL_SIZE_256	0x2
#define ZYNQMP_DMA_DESC_CTRL_COHRNT	0x1

/* Interrupt Mask specific definitions */
#define ZYNQMP_DMA_INT_ERR	(ZYNQMP_DMA_AXI_RD_DATA | \
				ZYNQMP_DMA_AXI_WR_DATA | \
				ZYNQMP_DMA_AXI_RD_DST_DSCR | \
				ZYNQMP_DMA_AXI_RD_SRC_DSCR | \
				ZYNQMP_DMA_INV_APB)
#define ZYNQMP_DMA_INT_OVRFL	(ZYNQMP_DMA_BYTE_CNT_OVRFL | \
				ZYNQMP_DMA_IRQ_SRC_ACCT_ERR | \
				ZYNQMP_DMA_IRQ_DST_ACCT_ERR)
#define ZYNQMP_DMA_INT_DONE	(ZYNQMP_DMA_DONE | ZYNQMP_DMA_DST_DSCR_DONE)
#define ZYNQMP_DMA_INT_EN_DEFAULT_MASK	(ZYNQMP_DMA_INT_DONE | \
					ZYNQMP_DMA_INT_ERR | \
					ZYNQMP_DMA_INT_OVRFL | \
					ZYNQMP_DMA_DST_DSCR_DONE)

/* Max number of descriptors per channel */
#define ZYNQMP_DMA_NUM_DESCS	32

/* Max transfer size per descriptor */
#define ZYNQMP_DMA_MAX_TRANS_LEN	0x40000000

/* Reset values for data attributes */
#define ZYNQMP_DMA_AXCACHE_VAL		0xF
#define ZYNQMP_DMA_ARLEN_RST_VAL	0xF
#define ZYNQMP_DMA_AWLEN_RST_VAL	0xF

#define ZYNQMP_DMA_SRC_ISSUE_RST_VAL	0x1F

#define ZYNQMP_DMA_IDS_DEFAULT_MASK	0xFFF

/* Bus width in bits */
#define ZYNQMP_DMA_BUS_WIDTH_64		64
#define ZYNQMP_DMA_BUS_WIDTH_128	128

#define ZYNQMP_DMA_DESC_SIZE(chan)	(chan->desc_size)

#define to_chan(chan)		container_of(chan, struct sp_cbdma_chan, \
					     common)
#define tx_to_desc(tx)		container_of(tx, struct sp_cbdma_desc_sw, \
					     async_tx)

/* Debug message definition */
#define CBDMA_FUNC_DEBUG
#define CBDMA_KDBG_INFO
#define CBDMA_KDBG_ERR

#ifdef CBDMA_FUNC_DEBUG
#define FUNC_DEBUG()    printk(KERN_INFO "[CBDMA] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif

#ifdef CBDMA_KDBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "K_CBDMA: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef CBDMA_KDBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "K_CBDMA: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif


/**
 * struct sp_cbdma_desc_ll - Hw linked list descriptor
 * @addr: Buffer address
 * @size: Size of the buffer
 * @ctrl: Control word
 * @nxtdscraddr: Next descriptor base address
 * @rsvd: Reserved field and for Hw internal use.
 */
struct sp_cbdma_desc_ll {
	u64 addr;
	u32 size;
	u32 ctrl;
	u64 nxtdscraddr;
	u64 rsvd;
}; __aligned(64)

/**
 * struct sp_cbdma_desc_sw - Per Transaction structure
 * @src: Source address for simple mode dma
 * @dst: Destination address for simple mode dma
 * @len: Transfer length for simple mode dma
 * @node: Node in the channel descriptor list
 * @tx_list: List head for the current transfer
 * @async_tx: Async transaction descriptor
 * @src_v: Virtual address of the src descriptor
 * @src_p: Physical address of the src descriptor
 * @dst_v: Virtual address of the dst descriptor
 * @dst_p: Physical address of the dst descriptor
 */
struct sp_cbdma_desc_sw {
	u64 src;
	u64 dst;
	u32 len;
	struct list_head node;
	struct list_head tx_list;
	struct dma_async_tx_descriptor async_tx;
	struct sp_cbdma_desc_ll *src_v;
	dma_addr_t src_p;
	struct sp_cbdma_desc_ll *dst_v;
	dma_addr_t dst_p;
};

/**
 * struct sp_cbdma_chan - Driver specific DMA channel structure
 * @sodev: Driver specific device structure
 * @regs: Control registers offset
 * @lock: Descriptor operation lock
 * @pending_list: Descriptors waiting
 * @free_list: Descriptors free
 * @active_list: Descriptors active
 * @sw_desc_pool: SW descriptor pool
 * @done_list: Complete descriptors
 * @common: DMA common channel
 * @desc_pool_v: Statically allocated descriptor base
 * @desc_pool_p: Physical allocated descriptor base
 * @desc_free_cnt: Descriptor available count
 * @dev: The dma device
 * @irq: Channel IRQ
 * @is_dmacoherent: Tells whether dma operations are coherent or not
 * @tasklet: Cleanup work after irq
 * @idle : Channel status;
 * @desc_size: Size of the low level descriptor
 * @err: Channel has errors
 * @bus_width: Bus width
 * @src_burst_len: Source burst length
 * @dst_burst_len: Dest burst length
 * @clk_main: Pointer to main clock
 * @clk_apb: Pointer to apb clock
 */
struct sp_cbdma_chan {
	struct sp_cbdma_device *spdev;
	void __iomem *regs;
	spinlock_t lock;
	struct list_head pending_list;
	struct list_head free_list;
	struct list_head active_list;
	struct sp_cbdma_desc_sw *sw_desc_pool;
	struct list_head done_list;
	struct dma_chan common;
	void *desc_pool_v;
	dma_addr_t desc_pool_p;
	u32 desc_free_cnt;
	struct device *dev;
	int irq;
	bool is_dmacoherent;
	struct tasklet_struct tasklet;
	bool idle;
	u32 desc_size;
	bool err;
	u32 bus_width;
	u32 src_burst_len;
	u32 dst_burst_len;
	struct clk *clk_main;
	struct clk *clk_apb;
	u32 sram_addr;
	u32 sram_size;
};

/**
 * struct sp_cbdma_device - DMA device structure
 * @dev: Device Structure
 * @common: DMA device structure
 * @chan: Driver specific DMA channel
 */
struct sp_cbdma_device {
	struct device *dev;
	struct dma_device common;
	struct sp_cbdma_chan *chan;
};

static inline void zynqmp_dma_writeq(struct sp_cbdma_chan *chan, u32 reg,
				     u64 value)
{
	FUNC_DEBUG();

	lo_hi_writeq(value, chan->regs + reg);
}

/**
 * zynqmp_dma_update_desc_to_ctrlr - Updates descriptor to the controller
 * @chan: ZynqMP DMA DMA channel pointer
 * @desc: Transaction descriptor pointer
 */
static void zynqmp_dma_update_desc_to_ctrlr(struct sp_cbdma_chan *chan,
				      struct sp_cbdma_desc_sw *desc)
{
	dma_addr_t addr;

	FUNC_DEBUG();

	addr = desc->src_p;
	zynqmp_dma_writeq(chan, ZYNQMP_DMA_SRC_START_LSB, addr);
	addr = desc->dst_p;
	zynqmp_dma_writeq(chan, ZYNQMP_DMA_DST_START_LSB, addr);
}

/**
 * zynqmp_dma_desc_config_eod - Mark the descriptor as end descriptor
 * @chan: ZynqMP DMA channel pointer
 * @desc: Hw descriptor pointer
 */
static void zynqmp_dma_desc_config_eod(struct sp_cbdma_chan *chan,
				       void *desc)
{
	struct sp_cbdma_desc_ll *hw = (struct sp_cbdma_desc_ll *)desc;

	FUNC_DEBUG();

	hw->ctrl |= ZYNQMP_DMA_DESC_CTRL_STOP;
	hw++;
	hw->ctrl |= ZYNQMP_DMA_DESC_CTRL_COMP_INT | ZYNQMP_DMA_DESC_CTRL_STOP;
}

/**
 * zynqmp_dma_config_sg_ll_desc - Configure the linked list descriptor
 * @chan: ZynqMP DMA channel pointer
 * @sdesc: Hw descriptor pointer
 * @src: Source buffer address
 * @dst: Destination buffer address
 * @len: Transfer length
 * @prev: Previous hw descriptor pointer
 */
static void zynqmp_dma_config_sg_ll_desc(struct sp_cbdma_chan *chan,
				   struct sp_cbdma_desc_ll *sdesc,
				   dma_addr_t src, dma_addr_t dst, size_t len,
				   struct sp_cbdma_desc_ll *prev)
{
	struct sp_cbdma_desc_ll *ddesc = sdesc + 1;

	FUNC_DEBUG();

	sdesc->size = ddesc->size = len;
	sdesc->addr = src;
	ddesc->addr = dst;

	sdesc->ctrl = ddesc->ctrl = ZYNQMP_DMA_DESC_CTRL_SIZE_256;
	if (chan->is_dmacoherent) {
		sdesc->ctrl |= ZYNQMP_DMA_DESC_CTRL_COHRNT;
		ddesc->ctrl |= ZYNQMP_DMA_DESC_CTRL_COHRNT;
	}

	if (prev) {
		dma_addr_t addr = chan->desc_pool_p +
			    ((uintptr_t)sdesc - (uintptr_t)chan->desc_pool_v);
		ddesc = prev + 1;
		prev->nxtdscraddr = addr;
		ddesc->nxtdscraddr = addr + ZYNQMP_DMA_DESC_SIZE(chan);
	}
}

/**
 * sp_cbdma_init - Initialize the channel
 * @chan: ZynqMP DMA channel pointer
 */
static void sp_cbdma_init(struct sp_cbdma_chan *chan)
{
	u32 val;

	FUNC_DEBUG();

#if 1
	val = CBDMA_CONFIG_DEFAULT;
	writel(val, chan->regs + SP_DMA_CONFIG);

	/* Clearing the interrupt account rgisters */
	val = 0;
	writel(val, chan->regs + SP_DMA_INT_EN);
	val = 0x0000003F;
	writel(val, chan->regs + SP_DMA_INT_FLAG);
	val = CBDMA_DMA_DONE_EN;
	writel(val, chan->regs + SP_DMA_INT_EN);
#else
	writel(ZYNQMP_DMA_IDS_DEFAULT_MASK, chan->regs + ZYNQMP_DMA_IDS);
	val = readl(chan->regs + ZYNQMP_DMA_ISR);
	writel(val, chan->regs + ZYNQMP_DMA_ISR);

	if (chan->is_dmacoherent) {
		val = ZYNQMP_DMA_AXCOHRNT;
		val = (val & ~ZYNQMP_DMA_AXCACHE) |
			(ZYNQMP_DMA_AXCACHE_VAL << ZYNQMP_DMA_AXCACHE_OFST);
		writel(val, chan->regs + ZYNQMP_DMA_DSCR_ATTR);
	}

	val = readl(chan->regs + ZYNQMP_DMA_DATA_ATTR);
	if (chan->is_dmacoherent) {
		val = (val & ~ZYNQMP_DMA_ARCACHE) |
			(ZYNQMP_DMA_AXCACHE_VAL << ZYNQMP_DMA_ARCACHE_OFST);
		val = (val & ~ZYNQMP_DMA_AWCACHE) |
			(ZYNQMP_DMA_AXCACHE_VAL << ZYNQMP_DMA_AWCACHE_OFST);
	}
	writel(val, chan->regs + ZYNQMP_DMA_DATA_ATTR);

	/* Clearing the interrupt account rgisters */
	val = readl(chan->regs + ZYNQMP_DMA_IRQ_SRC_ACCT);
	val = readl(chan->regs + ZYNQMP_DMA_IRQ_DST_ACCT);
#endif

	chan->idle = true;
}

/**
 * zynqmp_dma_tx_submit - Submit DMA transaction
 * @tx: Async transaction descriptor pointer
 *
 * Return: cookie value
 */
static dma_cookie_t zynqmp_dma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct sp_cbdma_chan *chan = to_chan(tx->chan);
	struct sp_cbdma_desc_sw *desc, *new;
	dma_cookie_t cookie;

	FUNC_DEBUG();

	new = tx_to_desc(tx);
	spin_lock_bh(&chan->lock);
	cookie = dma_cookie_assign(tx);

	if (!list_empty(&chan->pending_list)) {
		desc = list_last_entry(&chan->pending_list,
				     struct sp_cbdma_desc_sw, node);
		if (!list_empty(&desc->tx_list))
			desc = list_last_entry(&desc->tx_list,
					       struct sp_cbdma_desc_sw, node);
		desc->src_v->nxtdscraddr = new->src_p;
		desc->src_v->ctrl &= ~ZYNQMP_DMA_DESC_CTRL_STOP;
		desc->dst_v->nxtdscraddr = new->dst_p;
		desc->dst_v->ctrl &= ~ZYNQMP_DMA_DESC_CTRL_STOP;
	}

	list_add_tail(&new->node, &chan->pending_list);
	spin_unlock_bh(&chan->lock);

	return cookie;
}

/**
 * zynqmp_dma_get_descriptor - Get the sw descriptor from the pool
 * @chan: ZynqMP DMA channel pointer
 *
 * Return: The sw descriptor
 */
static struct sp_cbdma_desc_sw *
zynqmp_dma_get_descriptor(struct sp_cbdma_chan *chan)
{
	struct sp_cbdma_desc_sw *desc;

	FUNC_DEBUG();

	spin_lock_bh(&chan->lock);
	desc = list_first_entry(&chan->free_list,
				struct sp_cbdma_desc_sw, node);
	list_del(&desc->node);
	spin_unlock_bh(&chan->lock);

	INIT_LIST_HEAD(&desc->tx_list);
	/* Clear the src and dst descriptor memory */
	memset((void *)desc->src_v, 0, ZYNQMP_DMA_DESC_SIZE(chan));
	memset((void *)desc->dst_v, 0, ZYNQMP_DMA_DESC_SIZE(chan));

	return desc;
}

/**
 * zynqmp_dma_free_descriptor - Issue pending transactions
 * @chan: ZynqMP DMA channel pointer
 * @sdesc: Transaction descriptor pointer
 */
static void zynqmp_dma_free_descriptor(struct sp_cbdma_chan *chan,
				 struct sp_cbdma_desc_sw *sdesc)
{
	struct sp_cbdma_desc_sw *child, *next;

	FUNC_DEBUG();

	chan->desc_free_cnt++;
	list_add_tail(&sdesc->node, &chan->free_list);
	list_for_each_entry_safe(child, next, &sdesc->tx_list, node) {
		chan->desc_free_cnt++;
		list_move_tail(&child->node, &chan->free_list);
	}
}

/**
 * zynqmp_dma_free_desc_list - Free descriptors list
 * @chan: ZynqMP DMA channel pointer
 * @list: List to parse and delete the descriptor
 */
static void zynqmp_dma_free_desc_list(struct sp_cbdma_chan *chan,
				      struct list_head *list)
{
	struct sp_cbdma_desc_sw *desc, *next;

	FUNC_DEBUG();

	list_for_each_entry_safe(desc, next, list, node)
		zynqmp_dma_free_descriptor(chan, desc);
}

/**
 * zynqmp_dma_alloc_chan_resources - Allocate channel resources
 * @dchan: DMA channel
 *
 * Return: Number of descriptors on success and failure value on error
 */
static int zynqmp_dma_alloc_chan_resources(struct dma_chan *dchan)
{
	struct sp_cbdma_chan *chan = to_chan(dchan);
	struct sp_cbdma_desc_sw *desc;
	int i;

	FUNC_DEBUG();

	chan->sw_desc_pool = kzalloc(sizeof(*desc) * ZYNQMP_DMA_NUM_DESCS,
				     GFP_KERNEL);
	if (!chan->sw_desc_pool)
		return -ENOMEM;

	chan->idle = true;
	chan->desc_free_cnt = ZYNQMP_DMA_NUM_DESCS;

	INIT_LIST_HEAD(&chan->free_list);

	for (i = 0; i < ZYNQMP_DMA_NUM_DESCS; i++) {
		desc = chan->sw_desc_pool + i;
		dma_async_tx_descriptor_init(&desc->async_tx, &chan->common);
		desc->async_tx.tx_submit = zynqmp_dma_tx_submit;
		list_add_tail(&desc->node, &chan->free_list);
	}

	chan->desc_pool_v = dma_zalloc_coherent(chan->dev,
				(2 * chan->desc_size * ZYNQMP_DMA_NUM_DESCS),
				&chan->desc_pool_p, GFP_KERNEL);
	if (!chan->desc_pool_v)
		return -ENOMEM;

	for (i = 0; i < ZYNQMP_DMA_NUM_DESCS; i++) {
		desc = chan->sw_desc_pool + i;
		desc->src_v = (struct sp_cbdma_desc_ll *) (chan->desc_pool_v +
					(i * ZYNQMP_DMA_DESC_SIZE(chan) * 2));
		desc->dst_v = (struct sp_cbdma_desc_ll *) (desc->src_v + 1);
		desc->src_p = chan->desc_pool_p +
				(i * ZYNQMP_DMA_DESC_SIZE(chan) * 2);
		desc->dst_p = desc->src_p + ZYNQMP_DMA_DESC_SIZE(chan);
	}

	return ZYNQMP_DMA_NUM_DESCS;
}

/**
 * zynqmp_dma_start - Start DMA channel
 * @chan: ZynqMP DMA channel pointer
 */
static void zynqmp_dma_start(struct sp_cbdma_chan *chan)
{
	FUNC_DEBUG();

	writel(ZYNQMP_DMA_INT_EN_DEFAULT_MASK, chan->regs + ZYNQMP_DMA_IER);
	chan->idle = false;
	writel(ZYNQMP_DMA_ENABLE, chan->regs + ZYNQMP_DMA_CTRL2);
}

/**
 * zynqmp_dma_handle_ovfl_int - Process the overflow interrupt
 * @chan: ZynqMP DMA channel pointer
 * @status: Interrupt status value
 */
static void zynqmp_dma_handle_ovfl_int(struct sp_cbdma_chan *chan, u32 status)
{
	u32 val;

	FUNC_DEBUG();

	if (status & ZYNQMP_DMA_IRQ_DST_ACCT_ERR)
		val = readl(chan->regs + ZYNQMP_DMA_IRQ_DST_ACCT);
	if (status & ZYNQMP_DMA_IRQ_SRC_ACCT_ERR)
		val = readl(chan->regs + ZYNQMP_DMA_IRQ_SRC_ACCT);
}

static void zynqmp_dma_config(struct sp_cbdma_chan *chan)
{
	u32 val;

	FUNC_DEBUG();

	val = readl(chan->regs + ZYNQMP_DMA_CTRL0);
	val |= ZYNQMP_DMA_POINT_TYPE_SG;
	writel(val, chan->regs + ZYNQMP_DMA_CTRL0);

	val = readl(chan->regs + ZYNQMP_DMA_DATA_ATTR);
	val = (val & ~ZYNQMP_DMA_ARLEN) |
		(chan->src_burst_len << ZYNQMP_DMA_ARLEN_OFST);
	val = (val & ~ZYNQMP_DMA_AWLEN) |
		(chan->dst_burst_len << ZYNQMP_DMA_AWLEN_OFST);
	writel(val, chan->regs + ZYNQMP_DMA_DATA_ATTR);
}

/**
 * zynqmp_dma_device_config - Zynqmp dma device configuration
 * @dchan: DMA channel
 * @config: DMA device config
 */
static int zynqmp_dma_device_config(struct dma_chan *dchan,
				    struct dma_slave_config *config)
{
	struct sp_cbdma_chan *chan = to_chan(dchan);

	FUNC_DEBUG();

	chan->src_burst_len = config->src_maxburst;
	chan->dst_burst_len = config->dst_maxburst;

	return 0;
}

/**
 * zynqmp_dma_start_transfer - Initiate the new transfer
 * @chan: ZynqMP DMA channel pointer
 */
static void zynqmp_dma_start_transfer(struct sp_cbdma_chan *chan)
{
	struct sp_cbdma_desc_sw *desc;

	FUNC_DEBUG();

	if (!chan->idle)
		return;

	zynqmp_dma_config(chan);

	desc = list_first_entry_or_null(&chan->pending_list,
					struct sp_cbdma_desc_sw, node);
	if (!desc)
		return;

	list_splice_tail_init(&chan->pending_list, &chan->active_list);
	zynqmp_dma_update_desc_to_ctrlr(chan, desc);
	zynqmp_dma_start(chan);
}


/**
 * zynqmp_dma_chan_desc_cleanup - Cleanup the completed descriptors
 * @chan: ZynqMP DMA channel
 */
static void zynqmp_dma_chan_desc_cleanup(struct sp_cbdma_chan *chan)
{
	struct sp_cbdma_desc_sw *desc, *next;

	FUNC_DEBUG();

	list_for_each_entry_safe(desc, next, &chan->done_list, node) {
		dma_async_tx_callback callback;
		void *callback_param;

		list_del(&desc->node);

		callback = desc->async_tx.callback;
		callback_param = desc->async_tx.callback_param;
		if (callback) {
			spin_unlock(&chan->lock);
			callback(callback_param);
			spin_lock(&chan->lock);
		}

		/* Run any dependencies, then free the descriptor */
		zynqmp_dma_free_descriptor(chan, desc);
	}
}

/**
 * zynqmp_dma_complete_descriptor - Mark the active descriptor as complete
 * @chan: ZynqMP DMA channel pointer
 */
static void zynqmp_dma_complete_descriptor(struct sp_cbdma_chan *chan)
{
	struct sp_cbdma_desc_sw *desc;

	FUNC_DEBUG();

	desc = list_first_entry_or_null(&chan->active_list,
					struct sp_cbdma_desc_sw, node);
	if (!desc)
		return;
	list_del(&desc->node);
	dma_cookie_complete(&desc->async_tx);
	list_add_tail(&desc->node, &chan->done_list);
}

/**
 * zynqmp_dma_issue_pending - Issue pending transactions
 * @dchan: DMA channel pointer
 */
static void zynqmp_dma_issue_pending(struct dma_chan *dchan)
{
	struct sp_cbdma_chan *chan = to_chan(dchan);

	FUNC_DEBUG();

	spin_lock_bh(&chan->lock);
	zynqmp_dma_start_transfer(chan);
	spin_unlock_bh(&chan->lock);
}

/**
 * zynqmp_dma_free_descriptors - Free channel descriptors
 * @dchan: DMA channel pointer
 */
static void zynqmp_dma_free_descriptors(struct sp_cbdma_chan *chan)
{
	FUNC_DEBUG();

	zynqmp_dma_free_desc_list(chan, &chan->active_list);
	zynqmp_dma_free_desc_list(chan, &chan->pending_list);
	zynqmp_dma_free_desc_list(chan, &chan->done_list);
}

/**
 * zynqmp_dma_free_chan_resources - Free channel resources
 * @dchan: DMA channel pointer
 */
static void zynqmp_dma_free_chan_resources(struct dma_chan *dchan)
{
	struct sp_cbdma_chan *chan = to_chan(dchan);

	FUNC_DEBUG();

	spin_lock_bh(&chan->lock);
	zynqmp_dma_free_descriptors(chan);
	spin_unlock_bh(&chan->lock);
	dma_free_coherent(chan->dev,
		(2 * ZYNQMP_DMA_DESC_SIZE(chan) * ZYNQMP_DMA_NUM_DESCS),
		chan->desc_pool_v, chan->desc_pool_p);
	kfree(chan->sw_desc_pool);
}

/**
 * zynqmp_dma_reset - Reset the channel
 * @chan: ZynqMP DMA channel pointer
 */
static void zynqmp_dma_reset(struct sp_cbdma_chan *chan)
{
	FUNC_DEBUG();

	writel(ZYNQMP_DMA_IDS_DEFAULT_MASK, chan->regs + ZYNQMP_DMA_IDS);

	zynqmp_dma_complete_descriptor(chan);
	zynqmp_dma_chan_desc_cleanup(chan);
	zynqmp_dma_free_descriptors(chan);
	sp_cbdma_init(chan);
}

/**
 * zynqmp_dma_irq_handler - ZynqMP DMA Interrupt handler
 * @irq: IRQ number
 * @data: Pointer to the ZynqMP DMA channel structure
 *
 * Return: IRQ_HANDLED/IRQ_NONE
 */
static irqreturn_t zynqmp_dma_irq_handler(int irq, void *data)
{
	struct sp_cbdma_chan *chan = (struct sp_cbdma_chan *)data;
	u32 isr, imr, status;
	irqreturn_t ret = IRQ_NONE;

	FUNC_DEBUG();

	isr = readl(chan->regs + ZYNQMP_DMA_ISR);
	imr = readl(chan->regs + ZYNQMP_DMA_IMR);
	status = isr & ~imr;

	writel(isr, chan->regs + ZYNQMP_DMA_ISR);
	if (status & ZYNQMP_DMA_INT_DONE) {
		tasklet_schedule(&chan->tasklet);
		ret = IRQ_HANDLED;
	}

	if (status & ZYNQMP_DMA_DONE)
		chan->idle = true;

	if (status & ZYNQMP_DMA_INT_ERR) {
		chan->err = true;
		tasklet_schedule(&chan->tasklet);
		dev_err(chan->dev, "Channel %p has errors\n", chan);
		ret = IRQ_HANDLED;
	}

	if (status & ZYNQMP_DMA_INT_OVRFL) {
		zynqmp_dma_handle_ovfl_int(chan, status);
		dev_info(chan->dev, "Channel %p overflow interrupt\n", chan);
		ret = IRQ_HANDLED;
	}

	return ret;
}

/**
 * zynqmp_dma_do_tasklet - Schedule completion tasklet
 * @data: Pointer to the ZynqMP DMA channel structure
 */
static void zynqmp_dma_do_tasklet(unsigned long data)
{
	struct sp_cbdma_chan *chan = (struct sp_cbdma_chan *)data;
	u32 count;

	FUNC_DEBUG();

	spin_lock(&chan->lock);

	if (chan->err) {
		zynqmp_dma_reset(chan);
		chan->err = false;
		goto unlock;
	}

	count = readl(chan->regs + ZYNQMP_DMA_IRQ_DST_ACCT);

	while (count) {
		zynqmp_dma_complete_descriptor(chan);
		zynqmp_dma_chan_desc_cleanup(chan);
		count--;
	}

	if (chan->idle)
		zynqmp_dma_start_transfer(chan);

unlock:
	spin_unlock(&chan->lock);
}

/**
 * zynqmp_dma_device_terminate_all - Aborts all transfers on a channel
 * @dchan: DMA channel pointer
 *
 * Return: Always '0'
 */
static int zynqmp_dma_device_terminate_all(struct dma_chan *dchan)
{
	struct sp_cbdma_chan *chan = to_chan(dchan);

	FUNC_DEBUG();

	spin_lock_bh(&chan->lock);
	writel(ZYNQMP_DMA_IDS_DEFAULT_MASK, chan->regs + ZYNQMP_DMA_IDS);
	zynqmp_dma_free_descriptors(chan);
	spin_unlock_bh(&chan->lock);

	return 0;
}

/**
 * zynqmp_dma_prep_memcpy - prepare descriptors for memcpy transaction
 * @dchan: DMA channel
 * @dma_dst: Destination buffer address
 * @dma_src: Source buffer address
 * @len: Transfer length
 * @flags: transfer ack flags
 *
 * Return: Async transaction descriptor on success and NULL on failure
 */
static struct dma_async_tx_descriptor *zynqmp_dma_prep_memcpy(
				struct dma_chan *dchan, dma_addr_t dma_dst,
				dma_addr_t dma_src, size_t len, ulong flags)
{
	struct sp_cbdma_chan *chan;
	struct sp_cbdma_desc_sw *new, *first = NULL;
	void *desc = NULL, *prev = NULL;
	size_t copy;
	u32 desc_cnt;

	FUNC_DEBUG();

	chan = to_chan(dchan);

	if (len > ZYNQMP_DMA_MAX_TRANS_LEN)
		return NULL;

	desc_cnt = DIV_ROUND_UP(len, ZYNQMP_DMA_MAX_TRANS_LEN);

	spin_lock_bh(&chan->lock);
	if (desc_cnt > chan->desc_free_cnt) {
		spin_unlock_bh(&chan->lock);
		dev_dbg(chan->dev, "chan %p descs are not available\n", chan);
		return NULL;
	}
	chan->desc_free_cnt = chan->desc_free_cnt - desc_cnt;
	spin_unlock_bh(&chan->lock);

	do {
		/* Allocate and populate the descriptor */
		new = zynqmp_dma_get_descriptor(chan);

		copy = min_t(size_t, len, ZYNQMP_DMA_MAX_TRANS_LEN);
		desc = (struct sp_cbdma_desc_ll *)new->src_v;
		zynqmp_dma_config_sg_ll_desc(chan, desc, dma_src,
					     dma_dst, copy, prev);
		prev = desc;
		len -= copy;
		dma_src += copy;
		dma_dst += copy;
		if (!first)
			first = new;
		else
			list_add_tail(&new->node, &first->tx_list);
	} while (len);

	zynqmp_dma_desc_config_eod(chan, desc);
	async_tx_ack(&first->async_tx);
	first->async_tx.flags = flags;
	return &first->async_tx;
}

/**
 * zynqmp_dma_prep_slave_sg - prepare descriptors for a memory sg transaction
 * @dchan: DMA channel
 * @dst_sg: Destination scatter list
 * @dst_sg_len: Number of entries in destination scatter list
 * @src_sg: Source scatter list
 * @src_sg_len: Number of entries in source scatter list
 * @flags: transfer ack flags
 *
 * Return: Async transaction descriptor on success and NULL on failure
 */
static struct dma_async_tx_descriptor *zynqmp_dma_prep_sg(
			struct dma_chan *dchan, struct scatterlist *dst_sg,
			unsigned int dst_sg_len, struct scatterlist *src_sg,
			unsigned int src_sg_len, unsigned long flags)
{
	struct sp_cbdma_desc_sw *new, *first = NULL;
	struct sp_cbdma_chan *chan = to_chan(dchan);
	void *desc = NULL, *prev = NULL;
	size_t len, dst_avail, src_avail;
	dma_addr_t dma_dst, dma_src;
	u32 desc_cnt = 0, i;
	struct scatterlist *sg;

	FUNC_DEBUG();

	for_each_sg(src_sg, sg, src_sg_len, i)
		desc_cnt += DIV_ROUND_UP(sg_dma_len(sg),
					 ZYNQMP_DMA_MAX_TRANS_LEN);

	spin_lock_bh(&chan->lock);
	if (desc_cnt > chan->desc_free_cnt) {
		spin_unlock_bh(&chan->lock);
		dev_dbg(chan->dev, "chan %p descs are not available\n", chan);
		return NULL;
	}
	chan->desc_free_cnt = chan->desc_free_cnt - desc_cnt;
	spin_unlock_bh(&chan->lock);

	dst_avail = sg_dma_len(dst_sg);
	src_avail = sg_dma_len(src_sg);

	/* Run until we are out of scatterlist entries */
	while (true) {
		/* Allocate and populate the descriptor */
		new = zynqmp_dma_get_descriptor(chan);
		desc = (struct sp_cbdma_desc_ll *)new->src_v;
		len = min_t(size_t, src_avail, dst_avail);
		len = min_t(size_t, len, ZYNQMP_DMA_MAX_TRANS_LEN);
		if (len == 0)
			goto fetch;
		dma_dst = sg_dma_address(dst_sg) + sg_dma_len(dst_sg) -
			dst_avail;
		dma_src = sg_dma_address(src_sg) + sg_dma_len(src_sg) -
			src_avail;

		zynqmp_dma_config_sg_ll_desc(chan, desc, dma_src, dma_dst,
					     len, prev);
		prev = desc;
		dst_avail -= len;
		src_avail -= len;

		if (!first)
			first = new;
		else
			list_add_tail(&new->node, &first->tx_list);
fetch:
		/* Fetch the next dst scatterlist entry */
		if (dst_avail == 0) {
			if (dst_sg_len == 0)
				break;
			dst_sg = sg_next(dst_sg);
			if (dst_sg == NULL)
				break;
			dst_sg_len--;
			dst_avail = sg_dma_len(dst_sg);
		}
		/* Fetch the next src scatterlist entry */
		if (src_avail == 0) {
			if (src_sg_len == 0)
				break;
			src_sg = sg_next(src_sg);
			if (src_sg == NULL)
				break;
			src_sg_len--;
			src_avail = sg_dma_len(src_sg);
		}
	}

	zynqmp_dma_desc_config_eod(chan, desc);
	first->async_tx.flags = flags;
	return &first->async_tx;
}

/**
 * sp_cbdma_chan_remove - Channel remove function
 * @chan: ZynqMP DMA channel pointer
 */
static void sp_cbdma_chan_remove(struct sp_cbdma_chan *chan)
{
	FUNC_DEBUG();

	if (!chan)
		return;

	devm_free_irq(chan->spdev->dev, chan->irq, chan);
	tasklet_kill(&chan->tasklet);
	list_del(&chan->common.device_node);
	clk_disable_unprepare(chan->clk_apb);
	clk_disable_unprepare(chan->clk_main);
}

/**
 * sp_cbdma_chan_probe - Per Channel Probing
 * @zdev: Driver specific device structure
 * @pdev: Pointer to the platform_device structure
 *
 * Return: '0' on success and failure value on error
 */
static int sp_cbdma_chan_probe(struct sp_cbdma_device *spdev,
			   struct platform_device *pdev)
{
	struct sp_cbdma_chan *chan;
	struct resource *res;
	struct device_node *node = pdev->dev.of_node;
	int err;

	FUNC_DEBUG();

	chan = devm_kzalloc(spdev->dev, sizeof(*chan), GFP_KERNEL);
	if (!chan)
		return -ENOMEM;
	chan->dev = spdev->dev;
	chan->spdev = spdev;

#if 1
	/* Get CBDMA register source */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, CB_DMA_REG_NAME);
	DBG_INFO("%s, %d, res:%p\n", __func__, __LINE__, res);
	chan->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chan->regs)) {
		DBG_ERR("%s devm_ioremap_resource fail\n",CB_DMA_REG_NAME);
		return PTR_ERR(chan->regs);
	}
	DBG_INFO("%s, %d, regs:%p\n", __func__, __LINE__, chan->regs);

	if (((u32)(res->start)) == 0x9C000D00) {
		/* CBDMA0 */
		chan->sram_addr = 0x9E800000;
		chan->sram_size = 40 << 10;
	} else {
		/* CBDMA1 */
		chan->sram_addr = 0x9E820000;
		chan->sram_size = 4 << 10;
	}
	DBG_INFO("%s, %d, SRAM:0x%x bytes @ 0x%x\n", __func__, __LINE__, chan->sram_size, chan->sram_addr);

	DBG_INFO("%s, %d, hw_ver:0x%x\n", __func__, __LINE__, readl(chan->regs + SP_DMA_HW_VER));

#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chan->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chan->regs))
		return PTR_ERR(chan->regs);
#endif

	chan->bus_width = ZYNQMP_DMA_BUS_WIDTH_64;
	chan->dst_burst_len = ZYNQMP_DMA_AWLEN_RST_VAL;
	chan->src_burst_len = ZYNQMP_DMA_ARLEN_RST_VAL;
#if 0 // CCHo: Disable this detection temporarily.
	err = of_property_read_u32(node, "xlnx,bus-width", &chan->bus_width);
	if (err < 0) {
		dev_err(&pdev->dev, "missing xlnx,bus-width property\n");
		return err;
	}
#endif
	if (chan->bus_width != ZYNQMP_DMA_BUS_WIDTH_64 &&
	    chan->bus_width != ZYNQMP_DMA_BUS_WIDTH_128) {
		dev_err(spdev->dev, "invalid bus-width value");
		return -EINVAL;
	}

	chan->is_dmacoherent =  of_property_read_bool(node, "dma-coherent");
	spdev->chan = chan;
	DBG_INFO("%s, %d, chan:%p, chan->is_dmacoherent: %u\n", __func__, __LINE__, chan, chan->is_dmacoherent);
	tasklet_init(&chan->tasklet, zynqmp_dma_do_tasklet, (ulong)chan);
	spin_lock_init(&chan->lock);
	INIT_LIST_HEAD(&chan->active_list);
	INIT_LIST_HEAD(&chan->pending_list);
	INIT_LIST_HEAD(&chan->done_list);
	INIT_LIST_HEAD(&chan->free_list);

	dma_cookie_init(&chan->common);
	chan->common.device = &spdev->common;
	list_add_tail(&chan->common.device_node, &spdev->common.channels);

	sp_cbdma_init(chan);
#if 1
	chan->irq = platform_get_irq(pdev, 0);
	if (chan->irq < 0) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return -ENXIO;
	}
	err = devm_request_irq(&pdev->dev, chan->irq, zynqmp_dma_irq_handler, 0, "sp-cbdma", chan);
	if (err) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return err;
	}
	DBG_INFO("%s, %d, irq: %d, %s\n", __func__, __LINE__, chan->irq, "sp-cbdma");

#else
	chan->irq = platform_get_irq(pdev, 0);
	if (chan->irq < 0)
		return -ENXIO;
	err = devm_request_irq(&pdev->dev, chan->irq, zynqmp_dma_irq_handler, 0,
			       "zynqmp-dma", chan);
	if (err)
		return err;

	chan->clk_main = devm_clk_get(&pdev->dev, "clk_main");
	if (IS_ERR(chan->clk_main)) {
		dev_err(&pdev->dev, "main clock not found.\n");
		return PTR_ERR(chan->clk_main);
	}

	chan->clk_apb = devm_clk_get(&pdev->dev, "clk_apb");
	if (IS_ERR(chan->clk_apb)) {
		dev_err(&pdev->dev, "apb clock not found.\n");
		return PTR_ERR(chan->clk_apb);
	}

	err = clk_prepare_enable(chan->clk_main);
	if (err) {
		dev_err(&pdev->dev, "Unable to enable main clock.\n");
		return err;
	}

	err = clk_prepare_enable(chan->clk_apb);
	if (err) {
		clk_disable_unprepare(chan->clk_main);
		dev_err(&pdev->dev, "Unable to enable apb clock.\n");
		return err;
	}
#endif

	chan->desc_size = sizeof(struct sp_cbdma_desc_ll);
	chan->idle = true;
	DBG_INFO("%s, %d, chan->desc_size: %d\n", __func__, __LINE__, chan->desc_size);
	return 0;
}

/**
 * of_sp_cbdma_xlate - Translation function
 * @dma_spec: Pointer to DMA specifier as found in the device tree
 * @ofdma: Pointer to DMA controller data
 *
 * Return: DMA channel pointer on success and NULL on error
 */
static struct dma_chan *of_sp_cbdma_xlate(struct of_phandle_args *dma_spec,
					    struct of_dma *ofdma)
{
	struct sp_cbdma_device *spdev = ofdma->of_dma_data;

	FUNC_DEBUG();

	return dma_get_slave_channel(&spdev->chan->common);
}

/**
 * sp_cbdma_probe - Driver probe function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: '0' on success and failure value on error
 */
static int sp_cbdma_probe(struct platform_device *pdev)
{
	struct sp_cbdma_device *spdev;
	struct dma_device *p;
	int ret;

	FUNC_DEBUG();

	spdev = devm_kzalloc(&pdev->dev, sizeof(*spdev), GFP_KERNEL);
	if (!spdev)
		return -ENOMEM;

	spdev->dev = &pdev->dev;
	INIT_LIST_HEAD(&spdev->common.channels);

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(44));
#if 1
	dma_cap_set(DMA_SLAVE, spdev->common.cap_mask);
	dma_cap_set(DMA_PRIVATE, spdev->common.cap_mask);
#else
	dma_cap_set(DMA_SG, spdev->common.cap_mask);
	dma_cap_set(DMA_MEMCPY, spdev->common.cap_mask);
#endif

	p = &spdev->common;
	p->device_prep_dma_sg = zynqmp_dma_prep_sg;
	p->device_prep_dma_memcpy = zynqmp_dma_prep_memcpy;
	p->device_terminate_all = zynqmp_dma_device_terminate_all;
	p->device_issue_pending = zynqmp_dma_issue_pending;
	p->device_alloc_chan_resources = zynqmp_dma_alloc_chan_resources;
	p->device_free_chan_resources = zynqmp_dma_free_chan_resources;
	p->device_tx_status = dma_cookie_status;
	p->device_config = zynqmp_dma_device_config;
	p->dev = &pdev->dev;

	platform_set_drvdata(pdev, spdev);

	ret = sp_cbdma_chan_probe(spdev, pdev);
	if (ret) {
		dev_err(&pdev->dev, "Probing channel failed\n");
		goto free_chan_resources;
	}

	p->dst_addr_widths = BIT(spdev->chan->bus_width / 8);
	p->src_addr_widths = BIT(spdev->chan->bus_width / 8);

	ret = dma_async_device_register(&spdev->common);
	DBG_INFO("%s, %d, ret %d\n", __func__, __LINE__, ret);
	if (ret) {
		DBG_INFO("%s, %d, dma_async_device_register() fail\n", __func__, __LINE__);
		return ret;
	}

	ret = of_dma_controller_register(pdev->dev.of_node, of_sp_cbdma_xlate, spdev);
	DBG_INFO("%s, %d, ret %d\n", __func__, __LINE__, ret);
	if (ret) {
		dev_err(&pdev->dev, "Unable to register DMA to DT\n");
		dma_async_device_unregister(&spdev->common);
		goto free_chan_resources;
	}

	dev_info(&pdev->dev, "SP CBDMA driver Probe success\n");

	return 0;

free_chan_resources:
	sp_cbdma_chan_remove(spdev->chan);
	return ret;
}

/**
 * sp_cbdma_remove - Driver remove function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: Always '0'
 */
static int sp_cbdma_remove(struct platform_device *pdev)
{
	struct sp_cbdma_device *spdev = platform_get_drvdata(pdev);

	FUNC_DEBUG();

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&spdev->common);

	sp_cbdma_chan_remove(spdev->chan);

	return 0;
}

static const struct of_device_id sp_cbdma_of_match[] = {
	{ .compatible = "sunplus,sp7021-cbdma" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sp_cbdma_of_match);

static struct platform_driver sp_cbdma_driver = {
	.driver = {
		.name	= DEVICE_NAME,
		.of_match_table = of_match_ptr(sp_cbdma_of_match),
	},
		.probe = sp_cbdma_probe,
		.remove = sp_cbdma_remove,
};

module_platform_driver(sp_cbdma_driver);


/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus CBDMA Driver");
