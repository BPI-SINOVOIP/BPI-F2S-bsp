/*
 * Core driver for the High Speed UART DMA
 *
 * Copyright (C) 2015 Intel Corporation
 * Author: Andy Shevchenko <andriy.shevchenko@linux.intel.com>
 *
 * Partially based on the bits found in drivers/tty/serial/mfd.c.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * DMA channel allocation:
 * 1. Even number chans are used for DMA Read (UART TX), odd chans for DMA
 *    Write (UART RX).
 * 2. 0/1 channel are assigned to port 0, 2/3 chan to port 1, 4/5 chan to
 *    port 3, and so on.
 */

#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "sp_cbdma.h"

#define DEVICE_NAME	"sunplus,sp7021-cbdma"

#define CB_DMA_REG_NAME	"cb_dma"

#define SP_CBDMA_BUSWIDTHS				\
	BIT(DMA_SLAVE_BUSWIDTH_UNDEFINED)	|	\
	BIT(DMA_SLAVE_BUSWIDTH_1_BYTE)		|	\
	BIT(DMA_SLAVE_BUSWIDTH_2_BYTES)		|	\
	BIT(DMA_SLAVE_BUSWIDTH_3_BYTES)		|	\
	BIT(DMA_SLAVE_BUSWIDTH_4_BYTES)

static inline void sp_cbdma_chan_disable(struct sp_cbdma_chan *sp_cbdma_ch)
{
	FUNC_DEBUG();

	//sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_CR, 0);
}

static inline void sp_cbdma_chan_enable(struct sp_cbdma_chan *sp_cbdma_ch)
{
	struct dma_slave_config *config = &sp_cbdma_ch->config;
	u32 config_reg = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO;

	FUNC_DEBUG();

	DBG_INFO("%s, %d, sp_cbdma_ch->direction:%u\n", __func__, __LINE__, sp_cbdma_ch->direction);
	DBG_INFO("%s, %d, config->direction:%u\n", __func__, __LINE__, config->direction);

	if (config->direction == DMA_MEM_TO_MEM)
		config_reg |= CBDMA_CONFIG_CP;
	else if (config->direction == DMA_MEM_TO_DEV) 
		config_reg |= CBDMA_CONFIG_RD;
	else if (config->direction == DMA_DEV_TO_MEM)
		config_reg |= CBDMA_CONFIG_WR;

	DBG_INFO("%s, %d, config_reg:0x%x\n", __func__, __LINE__, config_reg);

	sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_CONFIG, config_reg);
}

static void sp_cbdma_chan_start(struct sp_cbdma_chan *sp_cbdma_ch)
{
	struct dma_slave_config *config = &sp_cbdma_ch->config;
	struct sp_cbdma_desc *desc = sp_cbdma_ch->desc;
	u32 bsr = 0, mtsr = 0;	/* to shut the compiler up */
	//dcr = HSU_CH_DCR_CHSOE | HSU_CH_DCR_CHEI;
	unsigned int i, count;

	FUNC_DEBUG();

	DBG_INFO("%s, %d, config->direction:%u\n", __func__, __LINE__, config->direction);
	DBG_INFO("%s, %d, desc->direction:%u\n", __func__, __LINE__, desc->direction);
	if (config->direction == DMA_MEM_TO_MEM) {
		bsr = config->dst_maxburst;
		mtsr = config->src_addr_width;
	} else if (config->direction == DMA_MEM_TO_DEV) {
		bsr = config->dst_maxburst;
		mtsr = config->src_addr_width;
	} else if (config->direction == DMA_DEV_TO_MEM) {
		bsr = config->src_maxburst;
		mtsr = config->dst_addr_width;
	}

	sp_cbdma_chan_disable(sp_cbdma_ch);

#if 1
	/* Set descriptors */
	count = desc->nents - desc->active;
	for (i = 0; i < count && i < SP_CBDMA_CHAN_NR_DESC; i++) {
		if (config->direction == DMA_MEM_TO_MEM) {
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_SRC_ADR, config->src_addr);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_DES_ADR, config->dst_addr);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_LENGTH,  desc->sg[i].len);
		} else if (config->direction == DMA_MEM_TO_DEV) {
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_SRC_ADR, desc->sg[i].addr);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_DES_ADR, 0);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_LENGTH,  desc->sg[i].len);
		} else if (config->direction == DMA_DEV_TO_MEM) {
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_SRC_ADR, 0);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_DES_ADR, desc->sg[i].addr);
			sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_LENGTH,  desc->sg[i].len);
		}

		desc->active++;

		DBG_INFO("%s, %d, i:%u, src_addr:0x%x, dst_addr:0x%x\n", __func__, __LINE__, i, (int)config->src_addr, (int)config->dst_addr);
		DBG_INFO("%s, %d, i:%u, addr:0x%x, len:0x%x\n", __func__, __LINE__, i, (int)desc->sg[i].addr, desc->sg[i].len);
	}
#else
	sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_DCR, 0);
	sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_BSR, bsr);
	sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_MTSR, mtsr);

	/* Set descriptors */
	count = desc->nents - desc->active;
	for (i = 0; i < count && i < HSU_DMA_CHAN_NR_DESC; i++) {
		sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_DxSAR(i), desc->sg[i].addr);
		sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_DxTSR(i), desc->sg[i].len);

		/* Prepare value for DCR */
		dcr |= HSU_CH_DCR_DESCA(i);
		dcr |= HSU_CH_DCR_CHTOI(i);	/* timeout bit, see HSU Errata 1 */

		desc->active++;
	}
	/* Only for the last descriptor in the chain */
	dcr |= HSU_CH_DCR_CHSOD(count - 1);
	dcr |= HSU_CH_DCR_CHDI(count - 1);

	sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_DCR, dcr);
#endif

	sp_cbdma_chan_enable(sp_cbdma_ch);
}

static void sp_cbdma_stop_channel(struct sp_cbdma_chan *sp_cbdma_ch)
{
	FUNC_DEBUG();

	sp_cbdma_chan_disable(sp_cbdma_ch);
	//sp_cbdma_chan_writel(sp_cbdma_ch, HSU_CH_DCR, 0);
}

static void sp_cbdma_start_channel(struct sp_cbdma_chan *hsuc)
{
	FUNC_DEBUG();

	sp_cbdma_chan_start(hsuc);
}

static void sp_cbdma_start_transfer(struct sp_cbdma_chan *sp_cbdma_ch)
{
	struct virt_dma_desc *vdesc;

	FUNC_DEBUG();

	/* Get the next descriptor */
	vdesc = vchan_next_desc(&sp_cbdma_ch->vchan);
	if (!vdesc) {
		sp_cbdma_ch->desc = NULL;
		return;
	}

	list_del(&vdesc->node);
	sp_cbdma_ch->desc = to_sp_cbdma_desc(vdesc);

	DBG_INFO("%s, %d, sp_cbdma_ch->desc: 0x%x\n", __func__, __LINE__, (int)sp_cbdma_ch->desc);

	/* Start the channel with a new descriptor */
	sp_cbdma_start_channel(sp_cbdma_ch);
}

#if 1 // Remove sp_cbdma_get_status() function.
	//Blank
#else
/*
 *      sp_cbdma_get_status() - get DMA channel status
 *      @chip: HSUART DMA chip
 *      @nr: DMA channel number
 *      @status: pointer for DMA Channel Status Register value
 *
 *      Description:
 *      The function reads and clears the DMA Channel Status Register, checks
 *      if it was a timeout interrupt and returns a corresponding value.
 *
 *      Caller should provide a valid pointer for the DMA Channel Status
 *      Register value that will be returned in @status.
 *
 *      Return:
 *      1 for DMA timeout status, 0 for other DMA status, or error code for
 *      invalid parameters or no interrupt pending.
 */
int sp_cbdma_get_status(struct sp_cbdma_chip *chip, unsigned short nr,
		       u32 *status)
{
	struct sp_cbdma_chan *hsuc;
	unsigned long flags;
	u32 sr;

	/* Sanity check */
	if (nr >= chip->sp_cbdma->nr_channels)
		return -EINVAL;

	hsuc = &chip->sp_cbdma->chan[nr];

	/*
	 * No matter what situation, need read clear the IRQ status
	 * There is a bug, see Errata 5, HSD 2900918
	 */
	spin_lock_irqsave(&hsuc->vchan.lock, flags);
	sr = sp_cbdma_chan_readl(hsuc, HSU_CH_SR);
	spin_unlock_irqrestore(&hsuc->vchan.lock, flags);

	/* Check if any interrupt is pending */
	sr &= ~(HSU_CH_SR_DESCE_ANY | HSU_CH_SR_CDESC_ANY);
	if (!sr)
		return -EIO;

	/* Timeout IRQ, need wait some time, see Errata 2 */
	if (sr & HSU_CH_SR_DESCTO_ANY)
		udelay(2);

	/*
	 * At this point, at least one of Descriptor Time Out, Channel Error
	 * or Descriptor Done bits must be set. Clear the Descriptor Time Out
	 * bits and if sr is still non-zero, it must be channel error or
	 * descriptor done which are higher priority than timeout and handled
	 * in sp_cbdma_do_irq(). Else, it must be a timeout.
	 */
	sr &= ~HSU_CH_SR_DESCTO_ANY;

	*status = sr;

	return sr ? 0 : 1;
}
EXPORT_SYMBOL_GPL(sp_cbdma_get_status);
#endif

#if 1 // Remove sp_cbdma_do_irq() function.
	//Blank
#else

/*
 *      sp_cbdma_do_irq() - DMA interrupt handler
 *      @chip: HSUART DMA chip
 *      @nr: DMA channel number
 *      @status: Channel Status Register value
 *
 *      Description:
 *      This function handles Channel Error and Descriptor Done interrupts.
 *      This function should be called after determining that the DMA interrupt
 *      is not a normal timeout interrupt, ie. sp_cbdma_get_status() returned 0.
 *
 *      Return:
 *      0 for invalid channel number, 1 otherwise.
 */
int sp_cbdma_do_irq(struct sp_cbdma_chip *chip, unsigned short nr, u32 status)
{
	struct sp_cbdma_chan *hsuc;
	struct sp_cbdma_desc *desc;
	unsigned long flags;

	/* Sanity check */
	if (nr >= chip->sp_cbdma->nr_channels)
		return 0;

	hsuc = &chip->sp_cbdma->chan[nr];

	spin_lock_irqsave(&hsuc->vchan.lock, flags);
	desc = hsuc->desc;
	if (desc) {
		if (status & HSU_CH_SR_CHE) {
			desc->status = DMA_ERROR;
		} else if (desc->active < desc->nents) {
			sp_cbdma_start_channel(hsuc);
		} else {
			vchan_cookie_complete(&desc->vdesc);
			desc->status = DMA_COMPLETE;
			sp_cbdma_start_transfer(hsuc);
		}
	}
	spin_unlock_irqrestore(&hsuc->vchan.lock, flags);

	return 1;
}
EXPORT_SYMBOL_GPL(sp_cbdma_do_irq);
#endif

static struct sp_cbdma_desc *sp_cbdma_alloc_desc(unsigned int nents)
{
	struct sp_cbdma_desc *desc;

	FUNC_DEBUG();

	desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
	if (!desc)
		return NULL;

	desc->sg = kcalloc(nents, sizeof(*desc->sg), GFP_NOWAIT);
	if (!desc->sg) {
		kfree(desc);
		return NULL;
	}

	DBG_INFO("%s, %d, desc: %p, desc->sg: %p\n", __func__, __LINE__, desc, desc->sg);

	return desc;
}

static void sp_cbdma_desc_free(struct virt_dma_desc *vdesc)
{
	struct sp_cbdma_desc *desc = to_sp_cbdma_desc(vdesc);

	FUNC_DEBUG();

	DBG_INFO("%s, %d, desc: %p, desc->sg: %p\n", __func__, __LINE__, desc, desc->sg);

	kfree(desc->sg);
	kfree(desc);
}

static struct dma_async_tx_descriptor *sp_cbdma_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	struct sp_cbdma_desc *desc;
	struct scatterlist *sg;
	unsigned int i;

	FUNC_DEBUG();

	DBG_INFO("%s, %d, sgl: 0x%x, sg->page_link:0x%lx, sg->offset: 0x%x\n", __func__, __LINE__,
		(u32)sgl, sgl->page_link, sgl->offset);
	DBG_INFO("%s, %d, sgl: 0x%x, sg->length: 0x%x, sg->dma_address: 0x%x\n", __func__, __LINE__,
		(u32)sgl, sgl->length, (u32)sgl->dma_address);

	desc = sp_cbdma_alloc_desc(sg_len);
	if (!desc)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i) {
		desc->sg[i].addr = sg_dma_address(sg);
		desc->sg[i].len = sg_dma_len(sg);

		desc->length += sg_dma_len(sg);

		DBG_INFO("%s, %d, i: %u, sg: 0x%x, desc->sg[i].addr:0x%x\n", __func__, __LINE__,
			i, (u32)sg, (u32)desc->sg[i].addr);
		DBG_INFO("%s, %d, i: %u, desc->sg[i].len: 0x%x, desc->length: 0x%x\n", __func__, __LINE__,
			i, desc->sg[i].len, desc->length);
	}

	desc->nents = sg_len;
	desc->direction = direction;
	/* desc->active = 0 by kzalloc */
	desc->status = DMA_IN_PROGRESS;

	return vchan_tx_prep(&sp_cbdma_ch->vchan, &desc->vdesc, flags);
}

static void sp_cbdma_issue_pending(struct dma_chan *chan)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	unsigned long flags;

	FUNC_DEBUG();

	DBG_INFO("%s, %d, vchan_issue_pending():%u, sp_cbdma_ch->desc: 0x%x\n", __func__, __LINE__, vchan_issue_pending(&sp_cbdma_ch->vchan), (int)sp_cbdma_ch->desc);

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);
	if (vchan_issue_pending(&sp_cbdma_ch->vchan) && !sp_cbdma_ch->desc)
		sp_cbdma_start_transfer(sp_cbdma_ch);
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);
}

static size_t sp_cbdma_active_desc_size(struct sp_cbdma_chan *sp_cbdma_ch)
{
	struct sp_cbdma_desc *desc = sp_cbdma_ch->desc;
	size_t bytes = 0;
	int i;

	FUNC_DEBUG();

	for (i = desc->active; i < desc->nents; i++)
		bytes += desc->sg[i].len;

#if 1 // CCHo: It's ablut HSU hardware descriptor.
	// Blank
#else
	i = HSU_DMA_CHAN_NR_DESC - 1;
	do {
		bytes += sp_cbdma_chan_readl(sp_cbdma_ch, HSU_CH_DxTSR(i));
	} while (--i >= 0);
#endif

	return bytes;
}

static enum dma_status sp_cbdma_tx_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	struct virt_dma_desc *vdesc;
	enum dma_status status;
	size_t bytes;
	unsigned long flags;

	FUNC_DEBUG();

	DBG_INFO("%s, %d, desc: %p, cookie: %u, state: %p\n", __func__, __LINE__, chan, cookie, state);
	DBG_INFO("%s, %d, last: 0x%x, used: 0x%x, residue: 0x%x\n", __func__, __LINE__, state->last, state->used, state->residue);

	status = dma_cookie_status(chan, cookie, state);
	DBG_INFO("%s, %d, status: %u\n", __func__, __LINE__, status);
	if (status == DMA_COMPLETE)
		return status;

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);
	vdesc = vchan_find_desc(&sp_cbdma_ch->vchan, cookie);
	if (sp_cbdma_ch->desc && cookie == sp_cbdma_ch->desc->vdesc.tx.cookie) {
		bytes = sp_cbdma_active_desc_size(sp_cbdma_ch);
		dma_set_residue(state, bytes);
		status = sp_cbdma_ch->desc->status;
	} else if (vdesc) {
		bytes = to_sp_cbdma_desc(vdesc)->length;
		dma_set_residue(state, bytes);
	}
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);

	return status;
}

static int sp_cbdma_slave_config(struct dma_chan *chan,
				struct dma_slave_config *config)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);

	FUNC_DEBUG();

	/* Check if chan will be configured for slave transfers */
	if (!is_slave_direction(config->direction))
		return -EINVAL;

	memcpy(&sp_cbdma_ch->config, config, sizeof(sp_cbdma_ch->config));

	return 0;
}

static int sp_cbdma_pause(struct dma_chan *chan)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	unsigned long flags;

	FUNC_DEBUG();

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);
	if (sp_cbdma_ch->desc && sp_cbdma_ch->desc->status == DMA_IN_PROGRESS) {
		sp_cbdma_chan_disable(sp_cbdma_ch);
		sp_cbdma_ch->desc->status = DMA_PAUSED;
	}
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);

	return 0;
}

static int sp_cbdma_resume(struct dma_chan *chan)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	unsigned long flags;

	FUNC_DEBUG();

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);
	if (sp_cbdma_ch->desc && sp_cbdma_ch->desc->status == DMA_PAUSED) {
		sp_cbdma_ch->desc->status = DMA_IN_PROGRESS;
		sp_cbdma_chan_enable(sp_cbdma_ch);
	}
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);

	return 0;
}

static int sp_cbdma_terminate_all(struct dma_chan *chan)
{
	struct sp_cbdma_chan *sp_cbdma_ch = to_sp_cbdma_chan(chan);
	unsigned long flags;
	LIST_HEAD(head);

	FUNC_DEBUG();

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);

	sp_cbdma_stop_channel(sp_cbdma_ch);
	if (sp_cbdma_ch->desc) {
		sp_cbdma_desc_free(&sp_cbdma_ch->desc->vdesc);
		sp_cbdma_ch->desc = NULL;
	}

	vchan_get_all_descriptors(&sp_cbdma_ch->vchan, &head);
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);
	vchan_dma_desc_free_list(&sp_cbdma_ch->vchan, &head);

	return 0;
}

static void sp_cbdma_free_chan_resources(struct dma_chan *chan)
{
	FUNC_DEBUG();

	vchan_free_chan_resources(to_virt_chan(chan));
}

/**
 * sp_cbdma_irq_handler - SP CBDMA Interrupt handler
 * @irq: IRQ number
 * @data: Pointer to the SP DMA info structure
 *
 * Return: IRQ_HANDLED/IRQ_NONE
 */
static irqreturn_t sp_cbdma_irq_handler(int irq, void *data)

{
	struct sp_cbdma_info_s *ptr = (struct sp_cbdma_info_s *)data;
	struct sp_cbdma_chan *sp_cbdma_ch = (struct sp_cbdma_chan *)ptr->sp_cbdma->chan;
	struct sp_cbdma_desc *desc;
	unsigned long flags;
	u32 int_flag, int_en, status;
	irqreturn_t ret = IRQ_NONE;

	FUNC_DEBUG();

	//cbdma_reg_ptr = (struct sp_cbdma_reg *)ptr->cbdma_regs;
	//int_flag = readl(&cbdma_reg_ptr->int_flag);
	int_flag = sp_cbdma_chan_readl(sp_cbdma_ch, SP_DMA_INT_FLAG);
	int_en = sp_cbdma_chan_readl(sp_cbdma_ch, SP_DMA_INT_EN);
	status = int_flag & int_en;

	DBG_INFO("%s, %d, int_flag: 0x%08x, int_en: 0x%08x\n", __func__, __LINE__, int_flag, int_en);

	sp_cbdma_chan_writel(sp_cbdma_ch, SP_DMA_INT_FLAG, int_flag);

	if (status & CBDMA_INT_FLAG_DONE) {
		DBG_INFO("%s, %d, Channel %p transfer done\n", __func__, __LINE__, sp_cbdma_ch);
		dev_info(ptr->dev, "Channel %p transfer done\n", sp_cbdma_ch);
		ret = IRQ_HANDLED;
	}

	if (status & CBDMA_INT_FLAG_DMA_SRAM_OF) {
		//zynqmp_dma_handle_ovfl_int(chan, status);
		DBG_INFO("%s, %d, Channel %p overflow interrupt\n", __func__, __LINE__, sp_cbdma_ch);
		dev_info(ptr->dev, "Channel %p overflow interrupt\n", sp_cbdma_ch);
		ret = IRQ_HANDLED;
	}

#if 1

#if 0
	/* Sanity check */
	if (nr >= chip->hsu->nr_channels)
		return 0;

	hsuc = &chip->hsu->chan[nr];
#endif

	spin_lock_irqsave(&sp_cbdma_ch->vchan.lock, flags);
	desc = sp_cbdma_ch->desc;
	if (desc) {
		DBG_INFO("%s, %d, desc->active: %u, desc->nents: %u, \n", __func__, __LINE__, desc->active, desc->nents);
		if (status & (~CBDMA_INT_FLAG_DONE)) {
			desc->status = DMA_ERROR;
		} else if (desc->active < desc->nents) {
			sp_cbdma_start_channel(sp_cbdma_ch);
		} else {
			vchan_cookie_complete(&desc->vdesc);
			desc->status = DMA_COMPLETE;
			sp_cbdma_start_transfer(sp_cbdma_ch);
		}
	}
	spin_unlock_irqrestore(&sp_cbdma_ch->vchan.lock, flags);
	DBG_INFO("%s, %d, desc: %p, desc->status: %u, \n", __func__, __LINE__, desc, desc->status);
#else
	struct hsu_dma_chan *hsuc;
	struct hsu_dma_desc *desc;
	unsigned long flags;

	/* Sanity check */
	if (nr >= chip->hsu->nr_channels)
		return 0;

	hsuc = &chip->hsu->chan[nr];

	spin_lock_irqsave(&hsuc->vchan.lock, flags);
	desc = hsuc->desc;
	if (desc) {
		if (status & HSU_CH_SR_CHE) {
			desc->status = DMA_ERROR;
		} else if (desc->active < desc->nents) {
			hsu_dma_start_channel(hsuc);
		} else {
			vchan_cookie_complete(&desc->vdesc);
			desc->status = DMA_COMPLETE;
			hsu_dma_start_transfer(hsuc);
		}
	}
	spin_unlock_irqrestore(&hsuc->vchan.lock, flags);
#endif

	return ret;
}

/**
 * sp_cbdma_probe - Driver probe function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: '0' on success and failure value on error
 */
int sp_cbdma_probe(struct platform_device *pdev)
{
	struct sp_cbdma *sp_cbdma;
	struct resource *res;
	unsigned short i;
	int ret;

	FUNC_DEBUG();

	/* Get a memory to store infos */
	sp_cbdma_info = (struct sp_cbdma_info_s *)devm_kzalloc(&pdev->dev, sizeof(struct sp_cbdma_info_s), GFP_KERNEL);
	sp_cbdma_info->dev = &pdev->dev;
	dev_info(sp_cbdma_info->dev, "Start to probe CBDMA...\n");
	DBG_INFO("%s, %d, sp_cbdma_info:0x%lx\n", __func__, __LINE__, (unsigned long)sp_cbdma_info);

	sp_cbdma = devm_kzalloc(sp_cbdma_info->dev, sizeof(*sp_cbdma), GFP_KERNEL);
	if (!sp_cbdma_info)
		return -ENOMEM;
	sp_cbdma_info->sp_cbdma = sp_cbdma;

	/* Calculate nr_channels from the IO space length */
	//sp_cbdma->nr_channels = (chip->length - chip->offset) / HSU_DMA_CHAN_LENGTH;
	sp_cbdma->nr_channels = 1;

	sp_cbdma->chan = devm_kcalloc(sp_cbdma_info->dev, sp_cbdma->nr_channels,
				 sizeof(*sp_cbdma->chan), GFP_KERNEL);
	if (!sp_cbdma->chan)
		return -ENOMEM;

	INIT_LIST_HEAD(&sp_cbdma->dma.channels);
	for (i = 0; i < sp_cbdma->nr_channels; i++) {
		struct sp_cbdma_chan *sp_cbdma_ch = &sp_cbdma->chan[i];

		sp_cbdma_ch->vchan.desc_free = sp_cbdma_desc_free;
		vchan_init(&sp_cbdma_ch->vchan, &sp_cbdma->dma);

		sp_cbdma_ch->direction = (i & 0x1) ? DMA_DEV_TO_MEM : DMA_MEM_TO_DEV;
		//sp_cbdma_ch->reg = addr + i * HSU_DMA_CHAN_LENGTH;
		sp_cbdma_ch->reg = sp_cbdma_info->regs;
	}

	dma_cap_set(DMA_SLAVE, sp_cbdma->dma.cap_mask);
	dma_cap_set(DMA_PRIVATE, sp_cbdma->dma.cap_mask);

	sp_cbdma->dma.device_free_chan_resources = sp_cbdma_free_chan_resources;

	sp_cbdma->dma.device_prep_slave_sg = sp_cbdma_prep_slave_sg;

	sp_cbdma->dma.device_issue_pending = sp_cbdma_issue_pending;
	sp_cbdma->dma.device_tx_status = sp_cbdma_tx_status;

	sp_cbdma->dma.device_config = sp_cbdma_slave_config;
	sp_cbdma->dma.device_pause = sp_cbdma_pause;
	sp_cbdma->dma.device_resume = sp_cbdma_resume;
	sp_cbdma->dma.device_terminate_all = sp_cbdma_terminate_all;

	sp_cbdma->dma.src_addr_widths = SP_CBDMA_BUSWIDTHS;
	sp_cbdma->dma.dst_addr_widths = SP_CBDMA_BUSWIDTHS;
	sp_cbdma->dma.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	sp_cbdma->dma.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;

	sp_cbdma->dma.dev = sp_cbdma_info->dev;

	//dma_set_max_seg_size(sp_cbdma->dma.dev, HSU_CH_DxTSR_MASK);

	platform_set_drvdata(pdev, sp_cbdma_info);

	/* Get CBDMA register source */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, CB_DMA_REG_NAME);
	DBG_INFO("%s, %d, res:0x%lx\n", __func__, __LINE__, (unsigned long)res);
	if (res) {
		sp_cbdma_info->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(sp_cbdma_info->regs)) {
			DBG_ERR("%s devm_ioremap_resource fail\n",CB_DMA_REG_NAME);
		}
	}
	DBG_INFO("%s, %d, regs 0x%x\n", __func__, __LINE__, (unsigned int)sp_cbdma_info->regs);
	sp_cbdma->chan->reg = sp_cbdma_info->regs;


	sp_cbdma_info->irq = platform_get_irq(pdev, 0);
	if (sp_cbdma_info->irq < 0) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return -ENXIO;
	}
	ret = devm_request_irq(&pdev->dev, sp_cbdma_info->irq, sp_cbdma_irq_handler, 0, "sp-cbdma", sp_cbdma_info);
	if (ret) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	DBG_INFO("%s, %d, irq: %d, %s\n", __func__, __LINE__, sp_cbdma_info->irq, "sp-cbdma");

	if (((u32)(res->start)) == 0x9C000D00) {
		/* CBDMA0 */
		sp_cbdma_info->sram_addr = 0x9E800000;
		sp_cbdma_info->sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		sp_cbdma_info->sram_addr = 0x9E820000;
		sp_cbdma_info->sram_size = 4 << 10;

	}
	DBG_INFO("%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, sp_cbdma_info->sram_size, sp_cbdma_info->sram_addr);

	DBG_INFO("hw_ver:0x%x\n", sp_cbdma_chan_readl(sp_cbdma->chan, SP_DMA_HW_VER));

	ret = dma_async_device_register(&sp_cbdma->dma);
	if (ret)
		return ret;

	dev_info(sp_cbdma_info->dev, "Found CBDMA, %d channels\n", sp_cbdma->nr_channels);
	return 0;
}

/**
 * sp_cbdma_remove - Driver remove function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: Always '0'
 */
int sp_cbdma_remove(struct platform_device *pdev)
{
	//struct sp_cbdma *hsu = chip->sp_cbdma;
	struct sp_cbdma *sp_cbdma = sp_cbdma_info->sp_cbdma;
	unsigned short i;

	FUNC_DEBUG();

	dma_async_device_unregister(&sp_cbdma->dma);

	for (i = 0; i < sp_cbdma->nr_channels; i++) {
		struct sp_cbdma_chan *sp_cbdma_ch = &sp_cbdma->chan[i];

		tasklet_kill(&sp_cbdma_ch->vchan.task);
	}

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

