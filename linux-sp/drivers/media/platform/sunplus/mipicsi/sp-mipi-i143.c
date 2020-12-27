/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the BSD Licence, GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>
#include <media/v4l2-common.h>
#include <linux/i2c.h>
#ifdef CONFIG_PM_RUNTIME_MIPI
#include <linux/pm_runtime.h>
#endif
#include "sp-mipi-i143.h"
#include "sensor_power.h"


static unsigned int allocator = 0;
module_param(allocator, uint, 0444);
MODULE_PARM_DESC(allocator, " memory allocator selection, default is 0.\n"
			     "\t    0 == dma-contig\n"
			     "\t    1 == vmalloc");

static int video_nr = -1;
module_param(video_nr, int, 0444);
MODULE_PARM_DESC(video_nr, " videoX start number, -1 is autodetect");

/* ------------------------------------------------------------------
	Constants
   ------------------------------------------------------------------*/
static const struct sp_fmt gc0310_formats[] = {
	{
		.name     = "BAYER, RAW8",
		.fourcc   = V4L2_PIX_FMT_SRGGB8,
		.width    = 640,
		.height   = 480,
		.depth    = 8,
		.walign   = 2,
		.halign   = 2,
		.mipi_lane = 1,
		.sol_sync = SYNC_RAW8,
	},
	{
		.name     = "YUYV/YUY2, YUV422",
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 640,
		.height   = 480,
		.depth    = 16,
		.walign   = 2,
		.halign   = 1,
		.mipi_lane = 1,
		.sol_sync = SYNC_YUY2,
	},
};

static const struct sp_fmt imx219_formats[] = {
	{
		.name     = "BAYER, RAW10",
		.fourcc   = V4L2_PIX_FMT_SRGGB8,
		.width    = 3280,
		.height   = 2464,
		.depth    = 8,
		.walign   = 2,
		.halign   = 2,
		.mipi_lane = 2,
		.sol_sync = SYNC_RAW10,
	},
};

static const struct sp_fmt ov5647_formats[] = {
	{
		.name     = "BAYER, RAW8",
		.fourcc   = V4L2_PIX_FMT_SRGGB8,
		.width    = 2592,
		.height   = 1944,
		.depth    = 8,
		.walign   = 2,
		.halign   = 2,
		.mipi_lane = 2,
		.sol_sync = SYNC_RAW8,
	},
};

static const struct sp_fmt ov9281_formats[] = {
	{
		.name     = "GREY, RAW10",
		.fourcc   = V4L2_PIX_FMT_GREY,
		.width    = 1280,
		.height   = 800,
		.depth    = 8,
		.walign   = 1,
		.halign   = 1,
		.mipi_lane = 2,
		.sol_sync = SYNC_RAW10,
	},
};

static const struct sp_fmt ov9281_isp_formats[] = {
	{
		.name     = "YUYV/YUY2, YUV422",        // for SunplusIT ov9281_isp
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 1280,
		.height   = 720,
		.depth    = 16,
		.walign   = 2,
		.halign   = 1,
		.mipi_lane = 4,
		.sol_sync = SYNC_YUY2,
	},
};

static const struct sp_fmt sc2310_formats[] = {
	{
		.name     = "UYVY/YUY2, YUV422",
		.fourcc   = V4L2_PIX_FMT_UYVY,
		.width    = 1920,
		.height   = 1080,
		.depth    = 16,
		.walign   = 2,
		.halign   = 1,
		.mipi_lane = 4,
		.sol_sync = SYNC_YUY2,
	},
};

static const struct sp_fmt isp_test_formats[] = {
	{
		.name     = "UYVY/YUY2, YUV422",
		.fourcc   = V4L2_PIX_FMT_UYVY,
		.width    = 1920,
		.height   = 1080,
		.depth    = 16,
		.walign   = 2,
		.halign   = 1,
		.mipi_lane = 4,
		.sol_sync = SYNC_YUY2,
	},
};

static struct sp_mipi_subdev_info sp_mipi_sub_devs[] = {
	{
		.name = "sc2310",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("sc2310", 0x30),
		},
		.formats = sc2310_formats,
		.formats_size = ARRAY_SIZE(sc2310_formats),
	},

	{
		.name = "mipi_isp_test",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("mipi_isp_test", 0x7f),
		},
		.formats = isp_test_formats,
		.formats_size = ARRAY_SIZE(isp_test_formats),
	}
};

static const struct sp_mipi_config psp_mipi_cfg = {
	.i2c_adapter_id = 1,
	.sub_devs       = sp_mipi_sub_devs,
	.num_subdevs    = ARRAY_SIZE(sp_mipi_sub_devs),
};


/* ------------------------------------------------------------------
	SP7021 function
   ------------------------------------------------------------------*/
static const struct sp_fmt *get_format(const struct sp_mipi_subdev_info *sdinfo, u32 pixel_fmt)
{
	const struct sp_fmt *formats = sdinfo->formats;
	int size = sdinfo->formats_size;
	unsigned int k;

	MIPI_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	for (k = 0; k < size; k++) {
		if (formats[k].fourcc == pixel_fmt) {
			break;
		}
	}

	if (k == size) {
		return NULL;
	}

	return &formats[k];
}

static int sp_mipi_get_register_base(struct platform_device *pdev, void **membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	MIPI_DBG("Resource name: %s\n", res_name);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (r == NULL) {
		MIPI_ERR("platform_get_resource_byname failed!\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		MIPI_ERR("devm_ioremap_resource failed!\n");
		return PTR_ERR(p);
	}

	MIPI_DBG("ioremap address = 0x%px\n", p);
	*membase = p;

	return 0;
}

static void mipi_isp_init(struct sp_mipi_device *mipi)
{
	struct mipi_isp_info *isp_info;
	char fourcc[5] = "    ";
	u8 isp_channel = ISP0_PATH;
	u8 isp_mode = ISP_NORMAL_MODE;
	u8 test_pattern = STILL_WHITE;
	u8 probe = 0;
	u16 width = mipi->cur_format->width;
	u16 height = mipi->cur_format->height;
	u8 input_format = YUV422_FORMAT;
	u8 output_format = YUV422_FORMAT_YUYV_ORDER;
	u8 scale = SCALE_DOWN_OFF;
	//u8 scale = SCALE_DOWN_FHD_HD;

	MIPI_DBG("%s, %d\n", __FUNCTION__, __LINE__);


	// Conver fourcc to string
	fourcc[0] = (char)(mipi->cur_format->fourcc & 0x000000FF);
	fourcc[1] = (char)((mipi->cur_format->fourcc & 0x0000FF00)>>8);
	fourcc[2] = (char)((mipi->cur_format->fourcc & 0x00FF0000)>>16);
	fourcc[3] = (char)((mipi->cur_format->fourcc & 0xFF000000)>>24);
	MIPI_INFO("fourcc: 0x%04x, %s\n", mipi->cur_format->fourcc, fourcc);

	// Convert Four-character-code (FOURCC) to ISP support format
	switch (mipi->cur_format->fourcc)
	{
		case V4L2_PIX_FMT_YUYV:
			MIPI_DBG("YUV422 format YUYV order input\n");
			input_format = YUV422_FORMAT_YUYV_ORDER;
			break;

		case V4L2_PIX_FMT_UYVY:
			MIPI_DBG("YUV422 format UYVY order input\n");
			input_format = YUV422_FORMAT_UYVY_ORDER;
			break;

		case V4L2_PIX_FMT_SRGGB8:
			MIPI_DBG("RAW10 format input\n");
			input_format = RAW10_FORMAT;
			break;

		default:
			MIPI_ERR("No such format! (fourcc: 0x%04x)\n", mipi->cur_format->fourcc);
			break;
	}
	MIPI_DBG("ISP input format: %d\n", input_format);

#if 0
	MIPI_INFO("MIPI ISP Test Info:\n");
	isp_mode = ISP_TEST_MODE;
	test_pattern = MOVING_HORIZONTAL_COLOR_BAR;
	probe = 0;
	width = 1920;
	height = 1080;
	input_format = RAW10_FORMAT;
	output_format = YUV422_FORMAT_UYVY_ORDER;
	scale = SCALE_DOWN_OFF;
	MIPI_INFO("test_pattern: %d, probe: %d\n", test_pattern, probe);
	MIPI_INFO("Image size: %dx%d\n", width, height);
	MIPI_INFO("input_format: %d, output_format: %d\n", input_format, output_format);
#endif

	// Prepare ISP information
	isp_info = &mipi->isp_info;
	isp_info->mipi_isp_regs = mipi->mipi_isp_regs;
	isp_info->video_device = &mipi->video_dev;
	isp_info->isp_channel = isp_channel;
	isp_info->isp_mode = isp_mode;
	isp_info->test_pattern = test_pattern;
	isp_info->probe = probe;
	isp_info->width = width;
	isp_info->height = height;
	isp_info->input_fmt = input_format;
	isp_info->output_fmt = output_format;
	isp_info->scale = scale;

	if (isp_mode == ISP_TEST_MODE) {
		isp_test_setting(isp_info);
	}
	else {
		isp_setting(isp_info);
		powerSensorDown_RAM(isp_info);
	}
}

static void csiiw_init(struct sp_mipi_device *mipi)
{
	u32 line_stride = 0;
	u32 frame_size = 0;
	u32 config2 = NO_STRIDE_EN_ENA;
	u32 width = mipi->isp_info.width;
	u32 height = mipi->isp_info.height;
	u32 reg_value = 0;

	MIPI_DBG("%s, %d\n", __FUNCTION__, __LINE__);
	MIPI_INFO("Image size: %ux%u)\n", width, height);

	// Use ISP scaler to scale down image size
	switch (mipi->isp_info.scale)
	{
		case SCALE_DOWN_OFF:
			MIPI_DBG("Scaling down is off (%ux%u)\n", width, height);
			break;
	
		case SCALE_DOWN_FHD_HD:
			width = 1280;
			height = 720;
			MIPI_DBG("Scale down FHD to HD (%ux%u)\n", width, height);
			break;

		case SCALE_DOWN_FHD_WVGA:
			width = 720;
			height = 480;
			MIPI_DBG("Scale down FHD to WVGA (%ux%u)\n", width, height);
			break;
	
		case SCALE_DOWN_FHD_VGA:
			width = 640;
			height = 480;
			MIPI_DBG("Scale down FHD to VGA (%ux%u)\n", width, height);
			break;
	
		case SCALE_DOWN_FHD_QQVGA:
			width = 160;
			height = 120;
			MIPI_DBG("Scale down FHD to QQVGA (%ux%u)\n", width, height);
			break;

		default:
			MIPI_ERR("No such scale! (scale: %d)\n", mipi->isp_info.scale);
			break;
	}
	frame_size = YLEN(height) | XLEN(width);

	switch(mipi->isp_info.output_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_YUYV_ORDER:
		case YUV422_FORMAT_UYVY_ORDER:
			// YUV422 packet data size
			// 2 pixels, 4 bytes, 32 bits
			line_stride = LINE_STRIDE((width/2)*4);

			if (((mipi->isp_info.input_fmt == YUV422_FORMAT_UYVY_ORDER) && (mipi->isp_info.output_fmt == YUV422_FORMAT_YUYV_ORDER)) ||
				((mipi->isp_info.input_fmt == YUV422_FORMAT_YUYV_ORDER) && (mipi->isp_info.output_fmt == YUV422_FORMAT_UYVY_ORDER))) {
				config2 |= YCSWAP_EN_ENA;       // YCSWAP_EN = 1 (bit 1)
			}
			break;

		case RAW8_FORMAT:
			// RAW8 packet data size
			// 1 pixels, 1 bytes, 8 bits
			line_stride = LINE_STRIDE(width);
			break;

		case RAW10_FORMAT:
			// RAW10 data size
			// 2 pixels, 4 bytes, 32 bits
			// This mode is like YUV422 format
			line_stride = LINE_STRIDE((width/2)*4);
			break;

		case RAW10_FORMAT_PACK_MODE:
			// RAW10 packet data size
			// 4 pixels, 5 bytes, 40 bits
			// ISP converts RAW10 to YUV422 and then output it to CSIIW.
			line_stride = LINE_STRIDE((width/4)*5);

			// For RAW10 format pack mode output
			config2 |= OUTPUT_MODE_ENA;         // OUTPUT_MODE = 1 (bit 8)
			break;

		default:
			MIPI_ERR("No such format! (output_fmt: %d)\n", mipi->isp_info.output_fmt);
			break;
	}
	MIPI_DBG("line_stride: 0x%08X, frame_size: 0x%08X\n", line_stride, frame_size);

	// latch mode should be enable before base address setup
	reg_value = LATCH_EN_ENA;
	writel(reg_value, &mipi->csiiw_regs->csiiw_latch_mode);        // LATCH_EN = 1
	MIPI_DBG("csiiw_latch_mode: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_latch_mode), reg_value);

	// raw8 (0x2701); raw10 (10bit two byte space:0x2731, 8bit one byte space:0x2701)
	// Disable csiiw, fs_irq and fe_irq
	reg_value = IRQ_MASK_FE_DIS|IRQ_MASK_FS_DIS|CMD_URGENT_TH(2)|CMD_QUEUE(7)|CSIIW_EN_DIS;
	writel(reg_value, &mipi->csiiw_regs->csiiw_config0);           // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
	MIPI_DBG("csiiw_config0: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_config0), reg_value);

	writel(line_stride, &mipi->csiiw_regs->csiiw_stride);           // LINE_STRIED = 0x50 ((1280 / 2) * 2 / 16 = 80)
	writel(frame_size, &mipi->csiiw_regs->csiiw_frame_size);        // YLEN = 0x320(800), XLEN = 0x500(1280) 
	writel(ADDR_OFFSET(0x100), &mipi->csiiw_regs->csiiw_frame_buf); // set offset to trigger DRAM write
	writel(config2, &mipi->csiiw_regs->csiiw_config2);              // NO_STRIDE_EN = 1 

	MIPI_DBG("csiiw_stride: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_stride), line_stride);
	MIPI_DBG("csiiw_frame_size: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_frame_size), frame_size);
	MIPI_DBG("csiiw_frame_buf: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_frame_buf), ADDR_OFFSET(0x100));
	MIPI_DBG("config2: 0x%08X(0x%08X)\n", readl(&mipi->csiiw_regs->csiiw_config2), config2);
}

irqreturn_t isp_vsync_isr(int irq, void *dev_instance)
{
	struct sp_mipi_device *mipi = dev_instance;

	ispVsyncInt(&mipi->isp_info);

	return IRQ_HANDLED;
}

static int isp_irq_init(struct sp_mipi_device *mipi)
{
	int ret;

	// Disable ISP Vsync interrupt
	ispVsyncIntCtrl(&mipi->isp_info, 0);

	mipi->vsync_irq = irq_of_parse_and_map(mipi->pdev->of_node, 2);
	ret = devm_request_irq(mipi->pdev, mipi->vsync_irq, isp_vsync_isr, 0, "isp_vsync", mipi);
	if (ret) {
		goto err_isp_vsync_irq;
	}

	MIPI_INFO("Installed isp interrupt (vsync_irq=%d).\n", mipi->vsync_irq);
	return 0;

err_isp_vsync_irq:
	MIPI_ERR("request_irq failed (%d)!\n", ret);
	return ret;
}

irqreturn_t csiiw_fs_isr(int irq, void *dev_instance)
{
	struct sp_mipi_device *mipi = dev_instance;

	if (mipi->streaming) {
	}

	return IRQ_HANDLED;
}

irqreturn_t csiiw_fe_isr(int irq, void *dev_instance)
{
	struct sp_mipi_device *mipi = dev_instance;
	struct sp_buffer *next_frm;
	int addr;

	if (mipi->skip_first_int) {
		mipi->skip_first_int = false;
		return IRQ_HANDLED;
	}

	if (mipi->streaming) {
		spin_lock(&mipi->dma_queue_lock);

		// One frame is just being captured, get the next frame-buffer
		// from the queue. If no frame-buffer is available in queue,
		// hold on to the current buffer.
		if (!list_empty(&mipi->dma_queue))
		{
			// One video frame is just being captured, if next frame
			// is available, delete the frame from queue.
			next_frm = list_entry(mipi->dma_queue.next, struct sp_buffer, list);
			list_del_init(&next_frm->list);

			// Set active-buffer to 'next frame'.
			addr = vb2_dma_contig_plane_dma_addr(&next_frm->vb.vb2_buf, 0);
			addr = addr & 0x7fffffff;                                // Mark bit31 for direct access
			writel(addr, &mipi->csiiw_regs->csiiw_base_addr);        // base address

			// Then, release current frame.
			mipi->cur_frm->vb.vb2_buf.timestamp = ktime_get_ns();
			mipi->cur_frm->vb.field = mipi->fmt.fmt.pix.field;
			mipi->cur_frm->vb.sequence = mipi->sequence++;
			vb2_buffer_done(&mipi->cur_frm->vb.vb2_buf, VB2_BUF_STATE_DONE);

			// Finally, move on.
			mipi->cur_frm = next_frm;
		}

		spin_unlock(&mipi->dma_queue_lock);
	}

	return IRQ_HANDLED;
}

static int csiiw_irq_init(struct sp_mipi_device *mipi)
{
	int ret;

	mipi->fs_irq = irq_of_parse_and_map(mipi->pdev->of_node, 0);
	ret = devm_request_irq(mipi->pdev, mipi->fs_irq, csiiw_fs_isr, 0, "csiiw_fs", mipi);
	if (ret) {
		goto err_fs_irq;
	}

	mipi->fe_irq = irq_of_parse_and_map(mipi->pdev->of_node, 1);
	ret = devm_request_irq(mipi->pdev, mipi->fe_irq, csiiw_fe_isr, 0, "csiiw_fe", mipi);
	if (ret) {
		goto err_fe_irq;
	}

	MIPI_INFO("Installed csiiw interrupts (fs_irq=%d, fe_irq=%d).\n", mipi->fs_irq , mipi->fe_irq);
	return 0;

err_fe_irq:
err_fs_irq:
	MIPI_ERR("request_irq failed (%d)!\n", ret);
	return ret;
}

static int sp_queue_setup(struct vb2_queue *vq, unsigned *nbuffers, unsigned *nplanes,
		       unsigned sizes[], struct device *alloc_devs[])
{
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vq);
	unsigned size = mipi->fmt.fmt.pix.sizeimage;

	if (*nplanes) {
		if (sizes[0] < size) {
			return -EINVAL;
		}

		size = sizes[0];
	}

	// Don't support multiplanes.
	*nplanes = 1;
	sizes[0] = size;

	if ((vq->num_buffers + *nbuffers) < MIN_BUFFERS) {
		*nbuffers = MIN_BUFFERS - vq->num_buffers;
	}

	MIPI_DBG("%s: count = %u, size = %u\n", __FUNCTION__, *nbuffers, sizes[0]);
	return 0;
}

static int sp_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = mipi->fmt.fmt.pix.sizeimage;

	vb2_set_plane_payload(vb, 0, mipi->fmt.fmt.pix.sizeimage);

	if (vb2_get_plane_payload(vb, 0) > vb2_plane_size(vb, 0)) {
		MIPI_ERR("Buffer is too small (%lu < %lu)!\n", vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vbuf->field = mipi->fmt.fmt.pix.field;

	return 0;
}

static void sp_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vb->vb2_queue);
	struct sp_buffer *buf = container_of(vbuf, struct sp_buffer, vb);
	unsigned long flags = 0;

	// Add the buffer to the DMA queue.
	spin_lock_irqsave(&mipi->dma_queue_lock, flags);
	list_add_tail(&buf->list, &mipi->dma_queue);
	MIPI_DBG("%s: list_add\n", __FUNCTION__);
	print_List(&mipi->dma_queue);
	spin_unlock_irqrestore(&mipi->dma_queue_lock, flags);
}

static int sp_start_streaming(struct vb2_queue *vq, unsigned count)
{
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vq);
	struct sp_mipi_subdev_info *sdinfo;
	struct sp_buffer *buf, *tmp;
	struct mipi_isp_reg *regs;
	unsigned long flags;
	unsigned long addr;
	int ret;
	u32 reg_value = 0;

	MIPI_DBG("%s\n", __FUNCTION__);

	if (mipi->streaming) {
		MIPI_ERR("Device has started streaming!\n");
		return -EBUSY;
	}

	// Power senor on and start video output
	powerSensorOn_RAM(&mipi->isp_info);
	videoStartMode(&mipi->isp_info);

	mipi->sequence = 0;
	sdinfo = mipi->current_subdev;
	ret = v4l2_device_call_until_err(&mipi->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 1);
	if (ret && (ret != -ENOIOCTLCMD)) {
		MIPI_ERR("streamon failed in subdevice!\n");

		list_for_each_entry_safe(buf, tmp, &mipi->dma_queue, list) {
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_QUEUED);
		}

		return ret;
	}

	regs = (struct mipi_isp_reg *)((u64)mipi->mipi_isp_regs - ISP_BASE_ADDRESS);

	/* use pixel clock, master clock and mipi decoder clock as they are  */
	writeb(0x07, &(regs->reg[0x2008]));

	/* set and clear of front sensor interface */
	writeb(0x01, &(regs->reg[0x276C]));
	writeb(0x00, &(regs->reg[0x276C]));

	/* set and clear of front i2c interface */
	writeb(0x01, &(regs->reg[0x2660]));
	writeb(0x00, &(regs->reg[0x2660]));

	/* set and clear of cdsp */
	writeb(0x01, &(regs->reg[0x21D0]));
	writeb(0x00, &(regs->reg[0x21D0]));

	spin_lock_irqsave(&mipi->dma_queue_lock, flags);

	// Get the next frame from the dma queue.
	mipi->cur_frm = list_entry(mipi->dma_queue.next, struct sp_buffer, list);

	// Remove buffer from the dma queue.
	list_del_init(&mipi->cur_frm->list);

	addr = vb2_dma_contig_plane_dma_addr(&mipi->cur_frm->vb.vb2_buf, 0);
	addr = addr & 0x7fffffff;								 // Mark bit31 for direct access

	spin_unlock_irqrestore(&mipi->dma_queue_lock, flags);

	//writel(addr, &mipi->csiiw_regs->csiiw_base_addr);
	reg_value = BASE_ADDR(addr);
	writel(reg_value, &mipi->csiiw_regs->csiiw_base_addr);
	//writel(mEXTENDED_ALIGNED(mipi->fmt.fmt.pix.bytesperline, 16), &mipi->csiiw_regs->csiiw_stride);
	reg_value = LINE_STRIDE(mEXTENDED_ALIGNED(mipi->fmt.fmt.pix.bytesperline, 16));
	writel(reg_value, &mipi->csiiw_regs->csiiw_stride);
	//writel((mipi->fmt.fmt.pix.height<<16)|mipi->fmt.fmt.pix.bytesperline, &mipi->csiiw_regs->csiiw_frame_size);
	reg_value = YLEN(mipi->fmt.fmt.pix.height) | XLEN(mipi->fmt.fmt.pix.width);
	writel(reg_value, &mipi->csiiw_regs->csiiw_frame_size);

	//writel(0x12701, &mipi->csiiw_regs->csiiw_config0);      // Enable csiiw and fe_irq
	//reg_value = readl(&mipi->csiiw_regs->csiiw_config0);
	reg_value = (IRQ_MASK_FE_ENA|IRQ_MASK_FS_ENA|CMD_URGENT_TH(2)|CMD_QUEUE(7)|CSIIW_EN_ENA);
	writel(reg_value, &mipi->csiiw_regs->csiiw_config0);    // Enable csiiw and fe_irq

	mipi->streaming = true;
	mipi->skip_first_int = true;

	// Enable ISP Vsync interrupt
	ispVsyncIntCtrl(&mipi->isp_info, 1);

	MIPI_DBG("%s: video%d, cur_frm = %px, addr = %08lx\n",
		__FUNCTION__, mipi->video_dev.num, mipi->cur_frm, addr);

	return 0;
}

static void sp_stop_streaming(struct vb2_queue *vq)
{
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vq);
	struct sp_mipi_subdev_info *sdinfo;
	struct sp_buffer *buf;
	unsigned long flags;
	int ret;
	u32 reg_value = 0;

	MIPI_DBG("%s\n", __FUNCTION__);

	if (!mipi->streaming) {
		MIPI_ERR("Device has stopped already!\n");
		return;
	}

	sdinfo = mipi->current_subdev;
	ret = v4l2_device_call_until_err(&mipi->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 0);
	if (ret && (ret != -ENOIOCTLCMD)) {
		MIPI_ERR("streamon failed in subdevice!\n");
		return;
	}

	// Disable ISP Vsync interrupt
	//ispVsyncIntCtrl(&mipi->isp_info, 0);

	// Power senor down and stop video output
	powerSensorDown_RAM(&mipi->isp_info);
	videoStopMode(&mipi->isp_info);

	// FW must mask irq to avoid unmap issue (for test code)
	//writel(0x32700, &mipi->csiiw_regs->csiiw_config0);      // Disable csiiw, fs_irq and fe_irq
	//reg_value = readl(&mipi->csiiw_regs->csiiw_config0);
	reg_value = (IRQ_MASK_FE_DIS|IRQ_MASK_FS_DIS|CMD_URGENT_TH(2)|CMD_QUEUE(7)|CSIIW_EN_DIS);
	writel(reg_value, &mipi->csiiw_regs->csiiw_config0);    // Disable csiiw, fs_irq and fe_irq

	mipi->streaming = false;

	// Release all active buffers.
	spin_lock_irqsave(&mipi->dma_queue_lock, flags);

	if (mipi->cur_frm != NULL) {
		vb2_buffer_done(&mipi->cur_frm->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	while (!list_empty(&mipi->dma_queue)) {
		buf = list_entry(mipi->dma_queue.next, struct sp_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		MIPI_DBG("video buffer #%d is done!\n", buf->vb.vb2_buf.index);
	}

	spin_unlock_irqrestore(&mipi->dma_queue_lock, flags);
}

static const struct vb2_ops sp_video_qops = {
	.queue_setup            = sp_queue_setup,
	.buf_prepare            = sp_buf_prepare,
	.buf_queue              = sp_buf_queue,
	.start_streaming        = sp_start_streaming,
	.stop_streaming         = sp_stop_streaming,
	.wait_prepare           = vb2_ops_wait_prepare,
	.wait_finish            = vb2_ops_wait_finish
};

//===================================================================================
/* ------------------------------------------------------------------
	V4L2 ioctrl operations
   ------------------------------------------------------------------*/
static int vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *vcap)
{
	struct sp_mipi_device *mipi = video_drvdata(file);

	MIPI_DBG("%s\n", __FUNCTION__);

	strlcpy(vcap->driver, "SP MIPI Driver", sizeof(vcap->driver));
	strlcpy(vcap->card, "SP MIPI Camera Card", sizeof(vcap->card));
	strlcpy(vcap->bus_info, "SP MIPI Camera BUS", sizeof(vcap->bus_info));

	// report capabilities
	vcap->device_caps = mipi->caps;
	vcap->capabilities = vcap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *fmtdesc)
{
	struct sp_mipi_device *mipi = video_drvdata(file);
	const struct sp_fmt *fmt;

	MIPI_DBG("%s: index = %d\n", __FUNCTION__, fmtdesc->index);

	if (fmtdesc->index >= mipi->current_subdev->formats_size) {
		return -EINVAL;
	}

	if (fmtdesc->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		MIPI_ERR("Invalid V4L2 buffer type!\n");
		return -EINVAL;
	}

	fmt = &mipi->current_subdev->formats[fmtdesc->index];
	strlcpy(fmtdesc->description, fmt->name, sizeof(fmtdesc->description));
	fmtdesc->pixelformat = fmt->fourcc;

	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct sp_mipi_device *mipi = video_drvdata(file);
	const struct sp_fmt *fmt;
	enum v4l2_field field;

	MIPI_DBG("%s\n", __FUNCTION__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		MIPI_ERR("Invalid V4L2 buffer type!\n");
		return -EINVAL;
	}

	fmt = get_format(mipi->current_subdev, f->fmt.pix.pixelformat);
	if (fmt == NULL) {
		return -EINVAL;
	}

	field = f->fmt.pix.field;
	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	}
	f->fmt.pix.field = field;

	v4l_bound_align_image(&f->fmt.pix.width, 48, fmt->width, fmt->walign,
			      &f->fmt.pix.height, 32, fmt->height, fmt->halign, 0);

	f->fmt.pix.bytesperline = (f->fmt.pix.width * fmt->depth) >> 3;
	f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;

	if ((fmt->fourcc == V4L2_PIX_FMT_YUYV) || (fmt->fourcc == V4L2_PIX_FMT_UYVY)) {
		f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
	} else {
		f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	}

	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct sp_mipi_device *mipi = video_drvdata(file);
	int ret;

	MIPI_DBG("%s\n", __FUNCTION__);

	if (mipi->streaming) {
		MIPI_ERR("Device has started streaming!\n");
		return -EBUSY;
	}

	ret = vidioc_try_fmt_vid_cap(file, mipi, f);
	if (ret != 0) {
		return ret;
	}

	mipi->fmt.type                 = f->type;
	mipi->fmt.fmt.pix.width        = f->fmt.pix.width;
	mipi->fmt.fmt.pix.height       = f->fmt.pix.height;
	mipi->fmt.fmt.pix.pixelformat  = f->fmt.pix.pixelformat; // from vidioc_try_fmt_vid_cap
	mipi->fmt.fmt.pix.field        = f->fmt.pix.field;
	mipi->fmt.fmt.pix.bytesperline = f->fmt.pix.bytesperline;
	mipi->fmt.fmt.pix.sizeimage    = f->fmt.pix.sizeimage;
	mipi->fmt.fmt.pix.colorspace   = f->fmt.pix.colorspace;

	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct sp_mipi_device *mipi = video_drvdata(file);

	MIPI_DBG("%s\n", __FUNCTION__);

	memcpy(f, &mipi->fmt, sizeof(*f));

	return 0;
}

static const struct v4l2_ioctl_ops sp_mipi_ioctl_ops = {
	.vidioc_querycap                = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap        = vidioc_enum_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap         = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap           = vidioc_s_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap           = vidioc_g_fmt_vid_cap,
	.vidioc_reqbufs                 = vb2_ioctl_reqbufs,
	.vidioc_querybuf                = vb2_ioctl_querybuf,
	.vidioc_create_bufs             = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf             = vb2_ioctl_prepare_buf,
	.vidioc_qbuf                    = vb2_ioctl_qbuf,
	.vidioc_dqbuf                   = vb2_ioctl_dqbuf,
	.vidioc_expbuf                  = vb2_ioctl_expbuf,
	.vidioc_streamon                = vb2_ioctl_streamon,
	.vidioc_streamoff               = vb2_ioctl_streamoff,
};

//===================================================================================
/* ------------------------------------------------------------------
	V4L2 file operations
   ------------------------------------------------------------------*/
static int sp_mipi_open(struct file *file)
{
	struct sp_mipi_device *mipi = video_drvdata(file);
	int ret;

	MIPI_DBG("%s\n", __FUNCTION__);

#ifdef CONFIG_PM_RUNTIME_MIPI
	if (pm_runtime_get_sync(mipi->pdev) < 0)
		goto out;
#endif

	mutex_lock(&mipi->lock);

	ret = v4l2_fh_open(file);
	if (ret) {
		MIPI_ERR("v4l2_fh_open failed!\n");
	}

	mutex_unlock(&mipi->lock);
	return ret;

#ifdef CONFIG_PM_RUNTIME_MIPI
out:
	pm_runtime_mark_last_busy(mipi->pdev);
	pm_runtime_put_autosuspend(mipi->pdev);
	return -ENOMEM;
#endif
}

static int sp_mipi_release(struct file *file)
{
	struct sp_mipi_device *mipi = video_drvdata(file);
	int ret;

	MIPI_DBG("%s\n", __FUNCTION__);

	mutex_lock(&mipi->lock);

	// The release helper will cleanup any on-going streaming.
	ret = _vb2_fop_release(file, NULL);

	mutex_unlock(&mipi->lock);

#ifdef CONFIG_PM_RUNTIME_MIPI
	pm_runtime_put(mipi->pdev);             // Starting count timeout.
#endif

	return ret;
}

static const struct v4l2_file_operations sp_mipi_fops = {
	.owner          = THIS_MODULE,
	.open           = sp_mipi_open,
	.release        = sp_mipi_release,
	.read           = vb2_fop_read,
	.poll           = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = vb2_fop_mmap,
};

//===================================================================================
static const struct vb2_mem_ops *const sp_mem_ops[2] = {
	&vb2_dma_contig_memops,
	&vb2_vmalloc_memops,
};

/* ------------------------------------------------------------------
	sysfs
   ------------------------------------------------------------------*/
#if (SYSFS_MIPI_CSIIW_ATTRIBUTE == 1)
static ssize_t mipi_csiiw_show (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);
	u32 reg_value;

	MIPI_INFO("%s, %d\n", __FUNCTION__, __LINE__);

	reg_value = readl(&mipi->csiiw_regs->csiiw_frame_size);
	MIPI_INFO("csiiw_frame_size: 0x%08x\n", reg_value);

	return sprintf(buf, "0x%08x\n", reg_value);
}

static ssize_t mipi_csiiw_store (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);
	u32 reg_value;

	MIPI_INFO("%s, %d\n", __FUNCTION__, __LINE__);

	reg_value = simple_strtoul(buf, NULL, 16);
	MIPI_INFO("buf: %s, reg_value: 0x%08x\n", buf, reg_value);

	writel(reg_value, &mipi->csiiw_regs->csiiw_frame_size);
	MIPI_INFO("csiiw_frame_size: 0x%08x\n", readl(&mipi->csiiw_regs->csiiw_frame_size));

	return count;
}

static DEVICE_ATTR(csiiw, S_IWUSR|S_IRUGO, mipi_csiiw_show, mipi_csiiw_store);
#endif

#if (SYSFS_MIPI_ISP_ATTRIBUTE == 1)
static ssize_t mipi_isp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	MIPI_INFO("%s, %d\n", __FUNCTION__, __LINE__);
	return sprintf(buf, "MIPI ISP attritube\n");
};


/* Definitions for RREG and WREG command */
#define ISP_ATTR_MAX_INST_LENGTH    15
#define ISP_ATTR_RREG_INST_LENGTH   9   // without null characters
#define ISP_ATTR_WREG_INST_LENGTH   12  // without null characters

#define ISP_ATTR_COMMAND_LENGTH     4
#define ISP_ATTR_ADDR_OFFSET        5
#define ISP_ATTR_ADDR_LENGTH        4
#define ISP_ATTR_VALUE_OFFSET       10
#define ISP_ATTR_VALUE_LENGTH       2

static ssize_t mipi_isp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);
	const char *pbuf, *ptr;
	char inst[ISP_ATTR_MAX_INST_LENGTH], str[ISP_ATTR_COMMAND_LENGTH+1];
	u64 base_addr = (u64)mipi->mipi_isp_regs, cur_addr;
	u32 cur = 0, inst_len = 0;
	u32 addr, value;
	u32 i, j;

	MIPI_INFO("%s, %d\n", __FUNCTION__, __LINE__);
	//MIPI_INFO("buf: %s, count: %d\n", buf, count);

	MIPI_INFO("mipi_isp_regs = 0x%px\n", mipi->mipi_isp_regs);
	MIPI_INFO("base_adr = 0x%016llx\n", base_addr);

	/* Parse string to do read/write instructions */
	for (pbuf = buf; cur < count; ) {
		MIPI_INFO("cur: %d, count: %ld\n", cur, count);

		// Get read/write instruction
		MIPI_INFO("Searching instruction starts...\n");
		for (i = 0, j = 0, inst_len = 0, ptr = pbuf; i < ISP_ATTR_MAX_INST_LENGTH; i++, ptr++)
		{
			if ((*ptr >= 0x20) && (*ptr <= 0x7F)) {
				// ASCII printable characters (character code 32-127)
				inst[i] = *ptr;
			} else {
				// ASCII control characters (character code 0-31)
				// The extended ASCII codes (character code 128-255)
				j++;                                        // Count the number of non-printable character
				inst[i] = '\0'; 							// Replace it with null character '\0'
				inst_len = i + 1;
			}

			// If line-feed character '\n' can't be found
			if ((i == (ISP_ATTR_MAX_INST_LENGTH-1)) && (inst_len == 0)) {
				MIPI_INFO("Can't find Line Feed character!\n");
				inst[ISP_ATTR_MAX_INST_LENGTH-1] = '\0';    // Put null character'\0' behind the string
				inst_len = ISP_ATTR_MAX_INST_LENGTH;
				j = 1;
			}

			//MIPI_INFO("i: %d, *ptr: %c (0x%02x)\n", i, *ptr, *ptr);
			//MIPI_INFO("i: %d, inst[i]: %c (0x%02x)\n", i, inst[i], inst[i]);
		}
		MIPI_INFO("inst: %s\n", inst);
		MIPI_INFO("inst_len: %d, j: %d\n", inst_len, j);

		// If instructions are separated by space bar
		if ((strncmp(inst, "rreg", ISP_ATTR_COMMAND_LENGTH) == 0) && ((inst_len-j) > ISP_ATTR_RREG_INST_LENGTH)) {
			inst[ISP_ATTR_RREG_INST_LENGTH] = '\0';       // Put null character'\0' behind the string
			inst_len = ISP_ATTR_RREG_INST_LENGTH + 1;
			j = 1;

			MIPI_INFO("read instruction with space bar\n");
			MIPI_INFO("inst: %s\n", inst);
			MIPI_INFO("inst_len: %d, j: %d\n", inst_len, j);
		} else if ((strncmp(inst, "wreg", ISP_ATTR_COMMAND_LENGTH) == 0) && ((inst_len-j) > ISP_ATTR_WREG_INST_LENGTH)) {
			inst[ISP_ATTR_WREG_INST_LENGTH] = '\0';       // Put null character'\0' behind the string
			inst_len = ISP_ATTR_WREG_INST_LENGTH + 1;
			j = 1;

			MIPI_INFO("write instruction with space bar\n");
			MIPI_INFO("inst: %s\n", inst);
			MIPI_INFO("inst_len: %d, j: %d\n", inst_len, j);
		}
		MIPI_INFO("Searching instruction ends.\n");

		// Execute read/write instruction
		if (strncmp(inst, "rreg" ,ISP_ATTR_COMMAND_LENGTH) == 0) {
			MIPI_INFO("Read ISP register command\n");

			// Get ISP register address
			strncpy(str, inst+ISP_ATTR_ADDR_OFFSET, ISP_ATTR_ADDR_LENGTH);
			str[4] = '\0';                              // Put null character'\0' behind the string
			addr = simple_strtoul(str, NULL, 16);

			// Read ISP register value
			cur_addr = base_addr + (addr - 0x2000);
			value = readb((void __iomem *)cur_addr);

			MIPI_INFO("cur_addr = 0x%016llx\n", cur_addr);
			MIPI_INFO("Read ISP reg, addr 0x%04x = 0x%02x\n", addr, value);
		}
		else if (strncmp(inst, "wreg", ISP_ATTR_COMMAND_LENGTH) == 0) {
			MIPI_INFO("Write ISP register command\n");

			// Get ISP register address
			strncpy(str, inst+ISP_ATTR_ADDR_OFFSET, ISP_ATTR_ADDR_LENGTH);
			str[4] = '\0';                              // Put null character'\0' behind the string
			addr = simple_strtoul(str, NULL, 16);

			// Get ISP register setting value
			strncpy(str, inst+ISP_ATTR_VALUE_OFFSET, ISP_ATTR_VALUE_LENGTH);
			str[2] = '\0';                              // Put null character'\0' behind the string
			value = simple_strtoul(str, NULL, 16);

			// Write ISP register with the setting value
			cur_addr = base_addr + (addr - 0x2000);
			writeb(value, (void __iomem *)cur_addr);

			MIPI_INFO("cur_addr = 0x%016llx\n", cur_addr);
			MIPI_INFO("Write ISP reg, addr 0x%04x = 0x%02x\n", addr, value);
		}
		else {
			strncpy(str, inst, 4);
			str[4] = '\0';                              // Put null character'\0' behind the string
			MIPI_ERR("No this command: %s\n", str);
			break;
		}

		// Increase curren parsed length and buffer pointer
		cur = cur + inst_len;
		pbuf = pbuf + inst_len;
	}

	return count;
};

static DEVICE_ATTR(isp, S_IWUSR|S_IRUGO, mipi_isp_show, mipi_isp_store);
#endif

static struct attribute *mipi_attributes[] = {
#if (SYSFS_MIPI_CSIIW_ATTRIBUTE == 1)
	&dev_attr_csiiw.attr,
#endif
#if (SYSFS_MIPI_ISP_ATTRIBUTE == 1)
	&dev_attr_isp.attr,
#endif
	NULL,
};

static struct attribute_group mipi_attribute_group = {
	.attrs = mipi_attributes,
};

/* ------------------------------------------------------------------
	SP-MIPI driver probe
   ------------------------------------------------------------------*/
static int sp_mipi_probe(struct platform_device *pdev)
{
	struct sp_mipi_device *mipi;
	struct video_device *vfd;
	struct device *dev = &pdev->dev;
	struct vb2_queue *q;
	struct sp_mipi_config *sp_mipi_cfg;
	struct sp_mipi_subdev_info *sdinfo;
	struct i2c_adapter *i2c_adap;
	struct v4l2_subdev *subdev;
	struct sp_subdev_sensor_data *sensor_data;
	struct resource *r;
	void __iomem *p;
	const struct sp_fmt *cur_fmt;
	int num_subdevs = 0;
	int ret, i;

	MIPI_INFO("SP MIPI driver is probed\n");

	// Allocate memory for 'sp_mipi_device'.
	mipi = kzalloc(sizeof(struct sp_mipi_device), GFP_KERNEL);
	if (!mipi) {
		MIPI_ERR("Failed to allocate memory!\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mipi->pdev = &pdev->dev;
	mipi->caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;

	// Set the driver data in platform device.
	platform_set_drvdata(pdev, mipi);

	// Get and set 'mipi_isp' register base.
	ret = sp_mipi_get_register_base(pdev, (void**)&mipi->mipi_isp_regs, MIPI_ISP_REG_NAME);
	if (ret) {
		return ret;
	}

	// Get and set 'csi_iw' register base.
	ret = sp_mipi_get_register_base(pdev, (void**)&mipi->csiiw_regs, CSIIW_REG_NAME);
	if (ret) {
		return ret;
	}

	// Get and set 'moon5' register base.
	MIPI_DBG("Resource name: %s\n", MOON5_REG_NAME);
	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, MOON5_REG_NAME);
	if (r == NULL) {
		MIPI_ERR("Fail to get moon5 register resource\n");
		return -ENODEV;
	}

	p = devm_ioremap(&pdev->dev, r->start, (r->end - r->start + 1));
	if (IS_ERR(p)) {
		MIPI_ERR("Fail to remap moon5 register resource\n");
		return PTR_ERR(p);
	}
	MIPI_DBG("ioremap addr : 0x%px!!\n", p);
	mipi->moon5_regs = (struct moon5_reg *)p;

	MIPI_INFO("mipi_isp_regs = 0x%px\n", mipi->mipi_isp_regs);
	MIPI_INFO("csiiw_regs = 0x%px\n", mipi->csiiw_regs);
	MIPI_INFO("moon5_regs = 0x%px\n", mipi->moon5_regs);

	// Get clock resource 'clkc_csiiw'.
	mipi->clkc_csiiw = devm_clk_get(dev, "clkc_csiiw");
	if (IS_ERR(mipi->clkc_csiiw)) {
		ret = PTR_ERR(mipi->clkc_csiiw);
		MIPI_ERR("Failed to retrieve clock resource \'clkc_csiiw\'!\n");
		goto err_get_clkc_csiiw;
	}

	// Get reset controller resource 'rstc_csiiw'.
	mipi->rstc_csiiw = devm_reset_control_get(&pdev->dev, "rstc_csiiw");
	if (IS_ERR(mipi->rstc_csiiw)) {
		ret = PTR_ERR(mipi->rstc_csiiw);
		MIPI_ERR("Failed to retrieve reset controller 'rstc_csiiw\'!\n");
		goto err_get_rstc_csiiw;
	}

	// Get cam_gpio0.
	mipi->cam_gpio0 = devm_gpiod_get(&pdev->dev, "cam_gpio0", GPIOD_OUT_HIGH);
	if (!IS_ERR(mipi->cam_gpio0)) {
		MIPI_INFO("cam_gpio0 is at G_MX[%d].\n", desc_to_gpio(mipi->cam_gpio0));
		gpiod_set_value(mipi->cam_gpio0,1);
	}

	// Get cam_gpio1.
	mipi->cam_gpio1 = devm_gpiod_get(&pdev->dev, "cam_gpio1", GPIOD_OUT_HIGH);
	if (!IS_ERR(mipi->cam_gpio1)) {
		MIPI_INFO("cam_gpio1 is at G_MX[%d].\n", desc_to_gpio(mipi->cam_gpio1));
		gpiod_set_value(mipi->cam_gpio1,1);
	}

	// Get 'i2c-id' property from device tree.
	ret = of_property_read_u32(pdev->dev.of_node, "i2c-id", &mipi->i2c_id);
	if (ret) {
		MIPI_ERR("Failed to retrieve \'i2c-id\'!\n");
		goto err_get_i2c_id;
	}

	// Enable 'csiiw' clock.
	ret = clk_prepare_enable(mipi->clkc_csiiw);
	if (ret) {
		MIPI_ERR("Failed to enable \'csiiw\' clock!\n");
		goto err_en_clkc_csiiw;
	}

	// De-assert 'csiiw' reset controller.
	ret = reset_control_deassert(mipi->rstc_csiiw);
	if (ret) {
		MIPI_ERR("Failed to deassert 'csiiw' reset controller!\n");
		goto err_deassert_rstc_csiiw;
	}

	// Register V4L2 device.
	ret = v4l2_device_register(&pdev->dev, &mipi->v4l2_dev);
	if (ret) {
		MIPI_ERR("Unable to register V4L2 device!\n");
		goto err_v4l2_register;
	}
	MIPI_INFO("Registered V4L2 device.\n");

	// Initialize locks.
	spin_lock_init(&mipi->dma_queue_lock);
	mutex_init(&mipi->lock);

	if (allocator >= ARRAY_SIZE(sp_mem_ops)) {
		allocator = 0;
	}
	if (allocator == 0) {
		dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	}

	// Start creating the vb2 queues.
	q = &mipi->buffer_queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = mipi;
	q->buf_struct_size = sizeof(struct sp_buffer);
	q->ops = &sp_video_qops;
	q->mem_ops = sp_mem_ops[allocator];
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = MIN_BUFFERS;
	q->lock = &mipi->lock;
	q->dev = mipi->v4l2_dev.dev;

	ret = vb2_queue_init(q);
	if (ret) {
		MIPI_ERR("Failed to initialize vb2 queue!\n");
		goto err_vb2_queue_init;
	}

	// Initialize dma queues.
	INIT_LIST_HEAD(&mipi->dma_queue);

	// Initialize field of video device.
	vfd = &mipi->video_dev;
	vfd->fops        = &sp_mipi_fops;
	vfd->ioctl_ops   = &sp_mipi_ioctl_ops;
	vfd->device_caps = mipi->caps;
	vfd->release     = video_device_release_empty;
	vfd->v4l2_dev    = &mipi->v4l2_dev;
	vfd->queue       = &mipi->buffer_queue;
	//vfd->tvnorms     = 0;
	strlcpy(vfd->name, MIPI_CSI_RX_NAME, sizeof(vfd->name));
	vfd->lock       = &mipi->lock;  // protect all fops and v4l2 ioctls.

	video_set_drvdata(vfd, mipi);

	// Register video device.
	ret = video_register_device(&mipi->video_dev, VFL_TYPE_GRABBER, video_nr);
	if (ret) {
		MIPI_ERR("Unable to register video device!\n");
		ret = -ENODEV;
		goto err_video_register;
	}
	MIPI_INFO("Registered video device \'/dev/video%d\'.\n", vfd->num);

	// Get i2c_info for sub-device.
	sp_mipi_cfg = kmalloc(sizeof(*sp_mipi_cfg), GFP_KERNEL);
	if (!sp_mipi_cfg) {
		MIPI_ERR("Failed to allocate memory for \'sp_mipi_cfg\'!\n");
		ret = -ENOMEM;
		goto err_alloc_mipi_cfg;
	}
	memcpy (sp_mipi_cfg, &psp_mipi_cfg, sizeof(*sp_mipi_cfg));
	sp_mipi_cfg->i2c_adapter_id = mipi->i2c_id;
	num_subdevs = sp_mipi_cfg->num_subdevs;
	mipi->cfg = sp_mipi_cfg;

	// Get i2c adapter.
	i2c_adap = i2c_get_adapter(sp_mipi_cfg->i2c_adapter_id);
	if (!i2c_adap) {
		MIPI_ERR("Failed to get i2c adapter #%d!\n", sp_mipi_cfg->i2c_adapter_id);
		ret = -ENODEV;
		goto err_i2c_get_adapter;
	}
	mipi->i2c_adap = i2c_adap;
	MIPI_INFO("Got i2c adapter #%d.\n", sp_mipi_cfg->i2c_adapter_id);

	for (i = 0; i < num_subdevs; i++) {
		sdinfo = &sp_mipi_cfg->sub_devs[i];

		// Load up the subdevice.
		subdev = v4l2_i2c_new_subdev_board(&mipi->v4l2_dev,
						    i2c_adap,
						    &sdinfo->board_info,
						    NULL);

		if (subdev) {
			subdev->grp_id = sdinfo->grp_id;
			MIPI_INFO("Registered V4L2 subdevice \'%s\'.\n", sdinfo->name);
			break;
		}
	}
	if (i == num_subdevs) {
		MIPI_ERR("Failed to register V4L2 subdevice!\n");
		ret = -ENXIO;
		goto err_subdev_register;
	}

	// Set current sub device.
	mipi->current_subdev = &sp_mipi_cfg->sub_devs[i];
	mipi->v4l2_dev.ctrl_handler = subdev->ctrl_handler;
	sensor_data = v4l2_get_subdev_hostdata(subdev);
	mipi->cur_mode = sensor_data->mode;

	cur_fmt = get_format(mipi->current_subdev, sensor_data->fourcc);
	if (cur_fmt == NULL) {
		MIPI_ERR("Failed to check sensor format!\n");
		goto err_get_format;
	}
	mipi->cur_format = cur_fmt;

	// Check MO5_CFG0 register if ISP APB interval mode is 4T
	MIPI_INFO("mo5_cfg_0: 0x%08x\n", readl(&(mipi->moon5_regs->mo5_cfg_0)));

	// Initialize MIPI ISP
	mipi_isp_init(mipi);

	// Initialize CSIIW
	csiiw_init(mipi);

	ret = csiiw_irq_init(mipi);
	if (ret) {
		goto err_csiiw_irq_init;
	}

	ret = isp_irq_init(mipi);
	if (ret) {
		goto err_isp_irq_init;
	}

#if (SYSFS_MIPI_CSIIW_ATTRIBUTE == 1) || (SYSFS_MIPI_ISP_ATTRIBUTE == 1)
	/* Add the device attribute group into sysfs */
	ret = sysfs_create_group(&pdev->dev.kobj, &mipi_attribute_group);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to create sysfs files: %d\n", ret);
		goto err_sysfs_create_group;
	}
#endif

	// Initialize video format (V4L2_format).
	mipi->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	mipi->fmt.fmt.pix.width = cur_fmt->width;
	mipi->fmt.fmt.pix.height = cur_fmt->height;
	mipi->fmt.fmt.pix.pixelformat = cur_fmt->fourcc;
	mipi->fmt.fmt.pix.field = V4L2_FIELD_NONE;
	mipi->fmt.fmt.pix.bytesperline = (mipi->fmt.fmt.pix.width * cur_fmt->depth) >> 3;
	mipi->fmt.fmt.pix.sizeimage = mipi->fmt.fmt.pix.height * mipi->fmt.fmt.pix.bytesperline;
	mipi->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
	mipi->fmt.fmt.pix.priv = 0;

#ifdef CONFIG_PM_RUNTIME_MIPI
	pm_runtime_set_autosuspend_delay(&pdev->dev,5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif

	MIPI_INFO("SP MIPI driver is installed!\n");
	return 0;

err_sysfs_create_group:
err_isp_irq_init:
err_csiiw_irq_init:
err_get_format:
err_subdev_register:
	i2c_put_adapter(i2c_adap);

err_i2c_get_adapter:
	kfree(sp_mipi_cfg);

err_alloc_mipi_cfg:
	video_unregister_device(&mipi->video_dev);

err_video_register:
err_vb2_queue_init:
	v4l2_device_unregister(&mipi->v4l2_dev);

err_v4l2_register:
err_deassert_rstc_csiiw:
	clk_disable_unprepare(mipi->clkc_csiiw);

err_en_clkc_csiiw:
err_get_i2c_id:
err_get_rstc_csiiw:
err_get_clkc_csiiw:
err_alloc:
	kfree(mipi);
	return ret;
}

static int sp_mipi_remove(struct platform_device *pdev)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);

	MIPI_INFO("SP MIPI driver is removing\n");

#if (SYSFS_MIPI_CSIIW_ATTRIBUTE == 1) || (SYSFS_MIPI_ISP_ATTRIBUTE == 1)
	/* Remove the device attribute group from sysfs */
	sysfs_remove_group(&pdev->dev.kobj, &mipi_attribute_group);
#endif

	i2c_put_adapter(mipi->i2c_adap);
	kfree(mipi->cfg);

	video_unregister_device(&mipi->video_dev);
	v4l2_device_unregister(&mipi->v4l2_dev);

	clk_disable_unprepare(mipi->clkc_csiiw);

	kfree(mipi);

#ifdef CONFIG_PM_RUNTIME_MIPI
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
#endif

	MIPI_INFO("SP MIPI driver is removed!\n");
	return 0;
}

static int sp_mipi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);

	MIPI_INFO("MIPI suspend.\n");

	// Disable 'csiiw' clock.
	clk_disable(mipi->clkc_csiiw);

	return 0;
}

static int sp_mipi_resume(struct platform_device *pdev)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);
	int ret;

	MIPI_INFO("MIPI resume.\n");

	// Enable 'isp' clock.
	ret = clk_prepare_enable(mipi->clkc_csiiw);
	if (ret) {
		MIPI_ERR("Failed to enable \'csiiw\' clock!\n");
	}

	return 0;
}

#ifdef CONFIG_PM_RUNTIME_MIPI
static int sp_mipi_runtime_suspend(struct device *dev)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);

	MIPI_INFO("MIPI runtime suspend.\n");

	// Disable 'csiiw' clock.
	clk_disable(mipi->clkc_isp);

	return 0;
}

static int sp_mipi_runtime_resume(struct device *dev)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);
	int ret;

	MIPI_INFO("MIPI runtime resume.\n");

	// Enable 'isp' clock.
	ret = clk_prepare_enable(mipi->clkc_isp);
	if (ret) {
		MIPI_ERR("Failed to enable \'isp\' clock!\n");
	}

	return 0;
}

static const struct dev_pm_ops sp7021_mipicsi_rx_pm_ops = {
	.runtime_suspend = sp_mipi_runtime_suspend,
	.runtime_resume  = sp_mipi_runtime_resume,
};
#endif

static const struct of_device_id sp_mipicsi_rx_of_match[] = {
	{ .compatible = "sunplus,i143-mipicsi-rx", },
	{}
};

static struct platform_driver sp_mipicsi_rx_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = MIPI_CSI_RX_NAME,
		.of_match_table = sp_mipicsi_rx_of_match,
#ifdef CONFIG_PM_RUNTIME_MIPI
		.pm = &sp7021_mipicsi_rx_pm_ops,
#endif
	},
	.probe = sp_mipi_probe,
	.remove = sp_mipi_remove,
	.suspend = sp_mipi_suspend,
	.resume = sp_mipi_resume,
};

module_platform_driver(sp_mipicsi_rx_driver);

MODULE_DESCRIPTION("Sunplus MIPI/CSI-RX driver");
MODULE_LICENSE("GPL");
