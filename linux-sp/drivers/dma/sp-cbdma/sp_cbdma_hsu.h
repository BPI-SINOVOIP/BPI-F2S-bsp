/*
 * Driver for the High Speed UART DMA
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * Partially based on the bits found in drivers/tty/serial/mfd.c.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SP_CBDMA_H__
#define __SP_CBDMA_H__

#include <linux/spinlock.h>
#include <linux/dma/hsu.h>

//#include <linux/module.h>
//#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
//#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
//#include <linux/delay.h>
//#include <linux/dma-mapping.h>
#include <linux/atomic.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>

#include "../virt-dma.h"

struct sp_cbdma_reg {
	volatile unsigned int hw_ver;
	volatile unsigned int config;
	volatile unsigned int length;
	volatile unsigned int src_adr;
	volatile unsigned int des_adr;
	volatile unsigned int int_flag;
	volatile unsigned int int_en;
	volatile unsigned int memset_val;
	volatile unsigned int sdram_size_config;
	volatile unsigned int illegle_record;
	volatile unsigned int sg_idx;
	volatile unsigned int sg_cfg;
	volatile unsigned int sg_length;
	volatile unsigned int sg_src_adr;
	volatile unsigned int sg_des_adr;
	volatile unsigned int sg_memset_val;
	volatile unsigned int sg_en_go;
	volatile unsigned int sg_lp_mode;
	volatile unsigned int sg_lp_sram_start;
	volatile unsigned int sg_lp_sram_size;
	volatile unsigned int sg_chk_mode;
	volatile unsigned int sg_chk_sum;
	volatile unsigned int sg_chk_xor;
	volatile unsigned int rsv_23_31[9];
};

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

#define CBDMA_CONFIG_DEFAULT	0x00030000
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

#define SP_CBDMA_CHAN_NR_DESC	1


#if 1 // Remove HSU register and other definitions
// Blank
#else
#define HSU_CH_SR		0x00			/* channel status */
#define HSU_CH_CR		0x04			/* channel control */
#define HSU_CH_DCR		0x08			/* descriptor control */
#define HSU_CH_BSR		0x10			/* FIFO buffer size */
#define HSU_CH_MTSR		0x14			/* minimum transfer size */
#define HSU_CH_DxSAR(x)		(0x20 + 8 * (x))	/* desc start addr */
#define HSU_CH_DxTSR(x)		(0x24 + 8 * (x))	/* desc transfer size */
#define HSU_CH_D0SAR		0x20			/* desc 0 start addr */
#define HSU_CH_D0TSR		0x24			/* desc 0 transfer size */
#define HSU_CH_D1SAR		0x28
#define HSU_CH_D1TSR		0x2c
#define HSU_CH_D2SAR		0x30
#define HSU_CH_D2TSR		0x34
#define HSU_CH_D3SAR		0x38
#define HSU_CH_D3TSR		0x3c

#define HSU_DMA_CHAN_NR_DESC	4
//#define HSU_DMA_CHAN_LENGTH	0x40

/* Bits in HSU_CH_SR */
#define HSU_CH_SR_DESCTO(x)	BIT(8 + (x))
#define HSU_CH_SR_DESCTO_ANY	(BIT(11) | BIT(10) | BIT(9) | BIT(8))
#define HSU_CH_SR_CHE		BIT(15)
#define HSU_CH_SR_DESCE(x)	BIT(16 + (x))
#define HSU_CH_SR_DESCE_ANY	(BIT(19) | BIT(18) | BIT(17) | BIT(16))
#define HSU_CH_SR_CDESC_ANY	(BIT(31) | BIT(30))

/* Bits in HSU_CH_CR */
#define HSU_CH_CR_CHA		BIT(0)
#define HSU_CH_CR_CHD		BIT(1)

/* Bits in HSU_CH_DCR */
#define HSU_CH_DCR_DESCA(x)	BIT(0 + (x))
#define HSU_CH_DCR_CHSOD(x)	BIT(8 + (x))
#define HSU_CH_DCR_CHSOTO	BIT(14)
#define HSU_CH_DCR_CHSOE	BIT(15)
#define HSU_CH_DCR_CHDI(x)	BIT(16 + (x))
#define HSU_CH_DCR_CHEI		BIT(23)
#define HSU_CH_DCR_CHTOI(x)	BIT(24 + (x))

/* Bits in HSU_CH_DxTSR */
//#define HSU_CH_DxTSR_MASK	GENMASK(15, 0)
//#define HSU_CH_DxTSR_TSR(x)	((x) & HSU_CH_DxTSR_MASK)
#endif

struct sp_cbdma_sg {
	dma_addr_t addr;
	unsigned int len;
};

struct sp_cbdma_desc {
	struct virt_dma_desc vdesc;
	enum dma_transfer_direction direction;
	struct sp_cbdma_sg *sg;
	unsigned int nents;
	size_t length;
	unsigned int active;
	enum dma_status status;
};

static inline struct sp_cbdma_desc *to_sp_cbdma_desc(struct virt_dma_desc *vdesc)
{
	return container_of(vdesc, struct sp_cbdma_desc, vdesc);
}

struct sp_cbdma_chan {
	struct virt_dma_chan vchan;

	void __iomem *reg;

	/* hardware configuration */
	enum dma_transfer_direction direction;
	struct dma_slave_config config;

	struct sp_cbdma_desc *desc;
};

struct sp_cbdma {
	struct dma_device		dma;

	/* channels */
	struct sp_cbdma_chan		*chan;
	unsigned short			nr_channels;
};

static inline struct sp_cbdma *to_sp_cbdma(struct dma_device *ddev)
{
	return container_of(ddev, struct sp_cbdma, dma);
}

/**
 * struct hsu_dma_chip - representation of HSU DMA hardware
 * @dev:		 struct device of the DMA controller
 * @irq:		 irq line
 * @regs:		 memory mapped I/O space
 * @length:		 I/O space length
 * @offset:		 offset of the I/O space where registers are located
 * @hsu:		 struct hsu_dma that is filed by ->probe()
 * @pdata:		 platform data for the DMA controller if provided
 */
struct sp_cbdma_info_s {
	struct device			*dev;
	int						irq;
	void __iomem			*regs;
	u32 					sram_addr;
	u32 					sram_size;
	unsigned int			length;
	unsigned int			offset;
	struct sp_cbdma			*sp_cbdma;

};

static struct sp_cbdma_info_s *sp_cbdma_info;


static inline struct sp_cbdma_chan *to_sp_cbdma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct sp_cbdma_chan, vchan.chan);
}

static inline u32 sp_cbdma_chan_readl(struct sp_cbdma_chan *sp_cbdma_ch, int offset)
{
	return readl(sp_cbdma_ch->reg + offset);
}

static inline void sp_cbdma_chan_writel(struct sp_cbdma_chan *sp_cbdma_ch, int offset,
				   u32 value)
{
	writel(value, sp_cbdma_ch->reg + offset);
}


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

#endif /* __SP_CBDMA_H__ */
