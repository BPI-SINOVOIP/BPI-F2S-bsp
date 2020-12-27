#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>

/*** The code from Xilinx dma_proxy codes : Start ***/
//#include <linux/delay.h>
//#include <linux/dmaengine.h>
//#include <linux/dma-mapping.h>
//#include <linux/init.h>
//#include <linux/module.h>
#include <linux/slab.h>
/*** The code from Xilinx dma_proxy codes : End ***/


/*** The code from Xilinx dma_proxy codes : Start ***/
#include <linux/dmaengine.h>
//#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
//#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
//#include <linux/fs.h>
#include <linux/workqueue.h>
#include "sp_cbdma_proxy.h"
/*** The code from Xilinx dma_proxy codes : End ***/


#define DEVICE_NAME	"sunplus,sp7021-cbdma-proxy"

#define CB_DMA_REG_NAME	"cb_dma_proxy"

#define SP_CBDMA_BASIC_TEST
#if defined(SP_CBDMA_BASIC_TEST)
/* Unaligned test */
#define UNALIGNED_DROP_S	0	/* 0, 1, 2, 3 */
#define UNALIGNED_DROP_E	0	/* 0, 1, 2, 3 */
#define UNALIGNED_ADDR_S(X)	(X + UNALIGNED_DROP_S)
#define UNALIGNED_ADDR_E(X)	(X - UNALIGNED_DROP_E)

#define BUF_SIZE_DRAM		(PAGE_SIZE * 2)

#define PATTERN4TEST(X)		((((u32)(X)) << 24) | (((u32)(X)) << 16) | (((u32)(X)) << 8) | (((u32)(X)) << 0))
#endif



#define MIN(X, Y)		((X) < (Y) ? (X): (Y))

#define NUM_SG_IDX		32

struct cbdma_proxy_info_s {
	struct platform_device *pdev;
	struct miscdevice dev;
	struct mutex cbdma_lock;
	void __iomem *cbdma_regs;
	u32 sram_addr;
	u32 sram_size;
	void *buf_va;
	dma_addr_t dma_handle;
};
static struct cbdma_proxy_info_s *cbdma_proxy_info;


/*** The code from Xilinx dma_proxy codes : Start ***/
//#define DRIVER_NAME 		"dma_proxy"
#define CHANNEL_COUNT 		2
#define ERROR 			-1
#define NOT_LAST_CHANNEL 	0
#define LAST_CHANNEL 		1

/* The following data structure represents a single channel of DMA, transmit or receive in the case
 * when using AXI DMA.  It contains all the data to be maintained for the channel.
 */
struct sp_cbdma_proxy_channel {
	struct sp_cbdma_proxy_channel_interface *interface_p;	/* user to kernel space interface */
	dma_addr_t interface_phys_addr;

	struct device *proxy_device_p;				/* character device support */
	struct device *dma_device_p;
	dev_t dev_node;
	struct cdev cdev;
	struct class *class_p;

	struct dma_chan *channel_p;				/* dma support */
	struct completion cmp;
	dma_cookie_t cookie;
	dma_addr_t dma_handle;
	u32 direction;						/* DMA_MEM_TO_DEV or DMA_DEV_TO_MEM */
};

/*** The code from Xilinx dma_proxy codes : End ***/


atomic_t isr_cnt = ATOMIC_INIT(0);

#define CBDMA_PROXY_FUNC_DEBUG
#define CBDMA_PROXY_KDBG_INFO
#define CBDMA_PROXY_KDBG_ERR

#ifdef CBDMA_PROXY_FUNC_DEBUG
#define FUNC_DEBUG()            printk(KERN_INFO "[CBDMA_PROXY] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif

#ifdef CBDMA_PROXY_KDBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "K_CBDMA_PROXY: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef CBDMA_PROXY_KDBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "K_CBDMA_PROXY: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif

/*** The code from spi-davinci.c : Start ***/
static void sp_cbdma_proxy_rx_callback(void *data)
{
	struct sp_cbdma_proxy_channel *dma_proxy_ch = (struct sp_cbdma_proxy_channel *)data;

	FUNC_DEBUG();

	complete(&dma_proxy_ch->cmp);
}

static void sp_cbdma_proxy_tx_callback(void *data)
{
	struct sp_cbdma_proxy_channel *dma_proxy_ch = (struct sp_cbdma_proxy_channel *)data;

	FUNC_DEBUG();

	complete(&dma_proxy_ch->cmp);
}

/*** The code from spi-davinci.c : End ***/

static int sp_cbdma_proxy_transfer(struct sp_cbdma_proxy_channel *dma_proxy_ch)
{
	/*** The code from Xilinx dma_proxy codes : Start ***/
	dma_cap_mask_t mask;
	/*** The code from Xilinx dma_proxy codes : End ***/

	FUNC_DEBUG();

	/*** The code from Xilinx dma_proxy codes : Start ***/
	/* Zero out the capability mask then initialize it for a slave channel that is
	 * private.
	 */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);

	/* Request the DMA channel from the DMA engine and then use the device from
	 * the channel for the proxy channel also.
	 */
	DBG_INFO("%s, %d, Reguest DMA channel\n", __func__, __LINE__);
	dma_proxy_ch->channel_p = dma_request_channel(mask, NULL, NULL);
	DBG_INFO("%s, %d, channel_p 0x%lx\n", __func__, __LINE__, (unsigned long)dma_proxy_ch->channel_p);
	if (!dma_proxy_ch->channel_p) {
		dev_err(dma_proxy_ch->dma_device_p, "DMA channel request error\n");
		//return ERROR;
		goto error;
	}
	dma_proxy_ch->dma_device_p = &dma_proxy_ch->channel_p->dev->device;
	dev_info(dma_proxy_ch->dma_device_p, "It's a CBDMA channel which we got\n");

	/* Initialize the character device for the dma proxy channel
	 */
#if 0
	rc = cdevice_init(pchannel_p, name);
	if (rc) {
		return rc;
	}
#endif

	/* Allocate memory for the proxy channel interface for the channel as either
	 * cached or non-cache depending on input parameter. Use the managed
	 * device memory when possible but right now there's a bug that's not understood
	 * when using devm_kzalloc rather than kzalloc, so stay with kzalloc.
	 */
#if 0
	if (0) {
		dma_proxy_ch.interface_p = (struct dma_proxy_channel_interface *)
			kzalloc(sizeof(struct dma_proxy_channel_interface),
					GFP_KERNEL);
		printk(KERN_INFO "Allocating cached memory at 0x%08X\n",
			   (unsigned int)dma_proxy_ch.interface_p);
	} else {

		/* Step 1, set dma memory allocation for the channel so that all of memory
		 * is available for allocation, and then allocate the uncached memory (DMA)
		 * for the channel interface
		 */
		dma_set_coherent_mask(dma_proxy_ch.proxy_device_p, 0xFFFFFFFF);
		dma_proxy_ch.interface_p = (struct dma_proxy_channel_interface *)
			dmam_alloc_coherent(dma_proxy_ch.proxy_device_p,
								sizeof(struct dma_proxy_channel_interface),
								&dma_proxy_ch.interface_phys_addr, GFP_KERNEL);
		printk(KERN_INFO "Allocating uncached memory at 0x%08X\n",
			   (unsigned int)dma_proxy_ch.interface_p);
	}
	if (!dma_proxy_ch.interface_p) {
		dev_err(dma_proxy_ch.dma_device_p, "DMA allocation error\n");
		//return ERROR;
	}
#endif
	/*** The code from Xilinx dma_proxy codes : End ***/


	/*** The code from spi-davinci.c : Start ***/
{
	struct dma_slave_config dma_rx_conf = {
		.direction = DMA_DEV_TO_MEM,
		.src_addr = dma_proxy_ch->dma_handle,
		.src_addr_width = 4,
		.src_maxburst = 1,
	};
	struct dma_slave_config dma_tx_conf = {
		.direction = DMA_MEM_TO_DEV,
		.dst_addr = dma_proxy_ch->dma_handle,
		.dst_addr_width = 4,
		.dst_maxburst = 1,
	};
	struct dma_async_tx_descriptor *desc;


#if 1 // Use dmaengine_prep_slave_single
#else
	DBG_INFO("%s, %d, Set SG info\n", __func__, __LINE__);
	if (dir == DMA_DEV_TO_MEM)
		sg.dma_address = src_adr;
	else if (dir == DMA_MEM_TO_DEV)
		sg.dma_address = des_adr;
	else
		DBG_INFO("%s, %d, errror!!\n", __func__, __LINE__);
	sg.length = length;
	sg.offset = 0;
	sg.page_link = 0;
#endif

	DBG_INFO("%s, %d, Configure DMA channel\n", __func__, __LINE__);
	if (dma_proxy_ch->direction == DMA_DEV_TO_MEM)
		dmaengine_slave_config(dma_proxy_ch->channel_p, &dma_rx_conf);
	else if (dma_proxy_ch->direction == DMA_MEM_TO_DEV)
		dmaengine_slave_config(dma_proxy_ch->channel_p, &dma_tx_conf);
	else
		DBG_INFO("%s, %d, errror!!\n", __func__, __LINE__);

	DBG_INFO("%s, %d, Prepare slave sg\n", __func__, __LINE__);
#if 1 // Use dmaengine_prep_slave_single
	/* Create a buffer (channel)  descriptor for the buffer since only a
	 * single buffer is being used for this transfer
	 */
	desc = dmaengine_prep_slave_single(dma_proxy_ch->channel_p, dma_proxy_ch->dma_handle,
										dma_proxy_ch->interface_p->length,
										dma_proxy_ch->direction,
										(DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
#else
	desc = dmaengine_prep_slave_sg(dma_proxy_ch->channel_p,
			&sg, 1, dir, (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
#endif
	if (!desc)
	{
		DBG_INFO("%s, %d, dmaengine_prep_slave_sg fail\n", __func__, __LINE__);
		goto error;
	}

	DBG_INFO("%s, %d, Init completion structure\n", __func__, __LINE__);
	init_completion(&dma_proxy_ch->cmp);

	if (dma_proxy_ch->direction == DMA_DEV_TO_MEM)
	{
		desc->callback = sp_cbdma_proxy_rx_callback;
		desc->callback_param = (void *)dma_proxy_ch;
	}
	else if (dma_proxy_ch->direction == DMA_MEM_TO_DEV)
	{
		desc->callback = sp_cbdma_proxy_tx_callback;
		desc->callback_param = (void *)dma_proxy_ch;
	}
	else
		DBG_INFO("%s, %d, errror!!\n", __func__, __LINE__);

	DBG_INFO("%s, %d, Submit DMA transaction into queue\n", __func__, __LINE__);
	dmaengine_submit(desc);

	DBG_INFO("%s, %d, Issue DMA transaction\n", __func__, __LINE__);
	dma_async_issue_pending(dma_proxy_ch->channel_p);

	/* Wait for the transfer to complete */
	DBG_INFO("%s, %d, Wait for DMA transaction complete\n", __func__, __LINE__);	
	if (wait_for_completion_timeout(&dma_proxy_ch->cmp, HZ) == 0)
	{
		DBG_INFO("%s, %d, DMA transaction timeout!!\n", __func__, __LINE__);	
	}
	else
	{
		DBG_INFO("%s, %d, DMA transaction done!!\n", __func__, __LINE__);	
	}

}
	/*** The code from spi-davinci.c : End ***/

error:
	/*** The code from Xilinx dma_proxy codes : Start ***/
	DBG_INFO("%s, %d, Release DMA channel\n", __func__, __LINE__);
	dma_release_channel(dma_proxy_ch->channel_p);
	/*** The code from Xilinx dma_proxy codes : End ***/

	return 0;
}

void dump_data(u8 *ptr, u32 size)
{
	u32 i, addr_begin;
	int length;
	char buffer[256];

	FUNC_DEBUG();

	addr_begin = (u32)(ptr);
	for (i = 0; i < size; i++) {
		if ((i & 0x0F) == 0x00) {
			length = sprintf(buffer, "%08x: ", i + addr_begin);
		}
		length += sprintf(&buffer[length], "%02x ", *ptr);
		ptr++;

		if ((i & 0x0F) == 0x0F) {
			printk(KERN_INFO "%s\n", buffer);
		}
	}
	printk(KERN_INFO "\n");
}


#if defined(SP_CBDMA_BASIC_TEST)
static void sp_cbdma_proxy_tst_basic(void)
{
	/*** The code from Xilinx dma_proxy codes : Start ***/
	struct sp_cbdma_proxy_channel dma_proxy_ch;
	/*** The code from Xilinx dma_proxy codes : End ***/

	int j, val;
	u32 *u32_ptr, val_u32, test_size;
	u32 *sram_ptr;

	FUNC_DEBUG();

#if 1
{	
	u32 map_direction;
		/* Cached buffers need to be handled before starting the transfer so that
		 * any cached data is pushed to memory.
		 */
		if (dma_proxy_ch.direction == DMA_MEM_TO_DEV)
			map_direction = DMA_TO_DEVICE;
		else
			map_direction = DMA_FROM_DEVICE;
		pchannel_p->dma_handle = dma_map_single(pchannel_p->dma_device_p,
												interface_p->buffer,
												interface_p->length,
												map_direction);
}
#else
	printk(KERN_INFO "R/W test\n");

	sram_ptr = (u32 *)ioremap(cbdma_proxy_info->sram_addr, cbdma_proxy_info->sram_size);
	BUG_ON(!sram_ptr);

	u32_ptr = (u32 *)(cbdma_proxy_info->buf_va);
	val_u32 = (u32)(u32_ptr);
	test_size = MIN(cbdma_proxy_info->sram_size, BUF_SIZE_DRAM) >> 1;

	printk("u32_ptr=0x%x, sram_ptr=0x%x, val_u32=0x%x, test_size=0x%x\n", (int)u32_ptr, (int)sram_ptr, val_u32, test_size);
	dump_data(cbdma_proxy_info->buf_va, 0x20);
	for (j = 0 ; j < (test_size >> 2); j++) {
		/* Fill (test_size) bytes of data to DRAM */
		*u32_ptr = val_u32;
		u32_ptr++;
		val_u32 += 4;
	}
	dump_data(cbdma_proxy_info->buf_va, 0x20);

	val = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
	dma_proxy_ch.dma_handle = cbdma_proxy_info->dma_handle;
	dma_proxy_ch.direction = DMA_DEV_TO_MEM;
	dma_proxy_ch.interface_p->length = val;
	sp_cbdma_proxy_transfer(&dma_proxy_ch);

	dump_data((u8 *)(sram_ptr), 0x20);
	u32_ptr = sram_ptr;
	val_u32 = (u32)(cbdma_proxy_info->buf_va);
	for (j = 0 ; j < (test_size >> 2); j++) {
		/* Compare (test_size) bytes of data in DRAM */
		BUG_ON(*u32_ptr != val_u32);
		u32_ptr++;
		val_u32 += 4;
	}
	printk(KERN_INFO "Data in SRAM: OK\n");

	iounmap(sram_ptr);

	val = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
	dma_proxy_ch.dma_handle = (((u32)(cbdma_proxy_info->dma_handle)) + test_size);
	dma_proxy_ch.direction = DMA_MEM_TO_DEV;
	dma_proxy_ch.interface_p->length = val;
	sp_cbdma_proxy_transfer(&dma_proxy_ch);

	dump_data(cbdma_proxy_info->buf_va + test_size, 0x20);

	u32_ptr = (u32 *)(cbdma_proxy_info->buf_va + test_size);
	val_u32 = (u32)(cbdma_proxy_info->buf_va);
	for (j = 0 ; j < (test_size >> 2); j++) {
		/* Compare (test_size) bytes of data in DRAM */
		BUG_ON(*u32_ptr != val_u32);
		u32_ptr++;
		val_u32 += 4;
	}
	printk(KERN_INFO "R/W test: OK\n\n");
#endif
	printk(KERN_INFO "%s(), end\n", __func__);
}
#endif // #if defined(SP_CBDMA_BASIC_TEST)


static int sp_cbdma_proxy_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *reg_base;
	int ret = 0;

	FUNC_DEBUG();

	/* Get a memory to store infos */
	cbdma_proxy_info = (struct cbdma_proxy_info_s *)devm_kzalloc(&pdev->dev, sizeof(struct cbdma_proxy_info_s), GFP_KERNEL);
	DBG_INFO("cbdma_proxy_info:0x%lx\n", (unsigned long)cbdma_proxy_info);
	if (cbdma_proxy_info == NULL)
	{
		printk("cbdma_proxy_info malloc fail\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}

	/* Init */
	mutex_init(&cbdma_proxy_info->cbdma_lock);

	/* Register CBDMA device to kernel */
	cbdma_proxy_info->dev.name = "sp_cbdma_proxy";
	cbdma_proxy_info->dev.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&cbdma_proxy_info->dev);
	DBG_INFO("ret:%u\n", ret);
	if (ret != 0) {	
		DBG_ERR("sp_cbdma_proxy device register fail\n");
		goto fail_regdev;
	}


	/* Get CBDMA register source */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, CB_DMA_REG_NAME);
	DBG_INFO("res:0x%lx\n", (unsigned long)res);
	if (res) {
		reg_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(reg_base)) {
			DBG_ERR("%s devm_ioremap_resource fail\n",CB_DMA_REG_NAME);
		}
	}
	DBG_INFO("reg_base 0x%x\n",(unsigned int)reg_base);

	if (((u32)(res->start)) == 0x9C000D00) {
		/* CBDMA0 */
		cbdma_proxy_info->sram_addr = 0x9E800000;
		cbdma_proxy_info->sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		cbdma_proxy_info->sram_addr = 0x9E820000;
		cbdma_proxy_info->sram_size = 4 << 10;

	}
	DBG_INFO("%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, cbdma_proxy_info->sram_size, cbdma_proxy_info->sram_addr);


#if defined(SP_CBDMA_BASIC_TEST)
	/* Allocate uncached memory for test */
	cbdma_proxy_info->buf_va = dma_alloc_coherent(&(pdev->dev), BUF_SIZE_DRAM, &(cbdma_proxy_info->dma_handle), GFP_KERNEL);
	DBG_INFO("buf_va=0x%p, BUF_SIZE_DRAM=%lu\n", cbdma_proxy_info->buf_va, BUF_SIZE_DRAM);
	if (cbdma_proxy_info->buf_va == NULL) {
		DBG_ERR("%s, %d, Can't allocation buffer for cbdma_proxy_info\n", __func__, __LINE__);
		/* Skip error handling here */
		ret = -ENOMEM;
	}
	printk(KERN_INFO "DMA buffer for cbdma_proxy_info, VA: 0x%p, PA: 0x%x\n", cbdma_proxy_info->buf_va, (u32)(cbdma_proxy_info->dma_handle));

	sp_cbdma_proxy_tst_basic();
#endif

fail_regdev:
	mutex_destroy(&cbdma_proxy_info->cbdma_lock);
fail_kmalloc:
	return ret;
}

static int sp_cbdma_proxy_remove(struct platform_device *pdev)
{
	FUNC_DEBUG();
	return 0;
}

static const struct of_device_id sp_cbdma_proxy_of_match[] = {
	{ .compatible = "sunplus,sp7021-cbdma-proxy" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_cbdma_proxy_of_match);

static struct platform_driver sp_cbdma_proxy_driver = {
	.probe = sp_cbdma_proxy_probe,
	.remove = sp_cbdma_proxy_remove,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_cbdma_proxy_of_match),
	}
};

module_platform_driver(sp_cbdma_proxy_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus CBDMA Proxy Driver");
MODULE_LICENSE("GPL");
