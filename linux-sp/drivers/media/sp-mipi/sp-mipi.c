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
#include <mach/io_map.h>
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
#include "sp-mipi.h"


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

static struct sp_mipi_subdev_info sp_mipi_sub_devs[] = {
	{
		.name = "gc0310",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("gc0310", 0x21),
		},
		.formats = gc0310_formats,
		.formats_size = ARRAY_SIZE(gc0310_formats),
	},

	{
		.name = "imx219",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("imx219", 0x10),
		},
		.formats = imx219_formats,
		.formats_size = ARRAY_SIZE(imx219_formats),
	},

	{
		.name = "ov5647",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("ov5647", 0x36),
		},
		.formats = ov5647_formats,
		.formats_size = ARRAY_SIZE(ov5647_formats),
	},

	{
		.name = "ov9281",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("ov9281", 0x60),
		},
		.formats = ov9281_formats,
		.formats_size = ARRAY_SIZE(ov9281_formats),
	},
/*
	{
		.name = "ov9281",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("ov9281", 0x10),
		},
		.formats = ov9281_formats,
		.formats_size = ARRAY_SIZE(ov9281_formats),
	},
*/
	{
		.name = "ov9281_isp",
		.grp_id = 0,
		.board_info = {
			I2C_BOARD_INFO("ov9281_isp", 0x60),
		},
		.formats = ov9281_isp_formats,
		.formats_size = ARRAY_SIZE(ov9281_isp_formats),
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

	DBG_INFO("Resource name: %s\n", res_name);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (r == NULL) {
		MIP_ERR("platform_get_resource_byname failed!\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		MIP_ERR("devm_ioremap_resource failed!\n");
		return PTR_ERR(p);
	}

	DBG_INFO("ioremap address = 0x%08x\n", (unsigned int)p);
	*membase = p;

	return 0;
}

static void mipicsi_init(struct sp_mipi_device *mipi)
{
	u32 val, val2;

	val = 0x8104;   // Normal mode, MSB first, Auto

	switch (mipi->cur_format->mipi_lane) {
	default:
	case 1: // 1 lane
		val |= 0;
		val2 = 0x11;
		break;
	case 2: // 2 lanes
		val |= (1<<20);
		val2 = 0x13;
		break;
	case 4: // 4 lanes
		val |= (2<<20);
		val2 = 0x1f;
		break;
	}

	switch (mipi->cur_format->sol_sync) {
	default:
	case SYNC_RAW8:         // 8 bits
	case SYNC_YUY2:
		val |= (2<<16); break;
	case SYNC_RAW10:        // 10 bits
		val |= (1<<16); break;
	}

	writel(val, &mipi->mipicsi_regs->mipicsi_mix_cfg);
	writel(mipi->cur_format->sol_sync, &mipi->mipicsi_regs->mipicsi_sof_sol_syncword);
	writel(val2, &mipi->mipicsi_regs->mipi_analog_cfg2);
	writel(0x110, &mipi->mipicsi_regs->mipicsi_ecc_cfg);
	writel(0x1000, &mipi->mipicsi_regs->mipi_analog_cfg1);
	writel(0x1001, &mipi->mipicsi_regs->mipi_analog_cfg1);
	udelay(1);
	writel(0x1000, &mipi->mipicsi_regs->mipi_analog_cfg1);
	writel(0x1, &mipi->mipicsi_regs->mipicsi_enable);               // Enable MIPICSI
}

static void csiiw_init(struct sp_mipi_device *mipi)
{
	writel(0x1, &mipi->csiiw_regs->csiiw_latch_mode);               // latch mode should be enable before base address setup

	writel(0x500, &mipi->csiiw_regs->csiiw_stride);
	writel(0x3200500, &mipi->csiiw_regs->csiiw_frame_size);

	writel(0x00000100, &mipi->csiiw_regs->csiiw_frame_buf);         // set offset to trigger DRAM write

	//raw8 (0x2701); raw10 (10bit two byte space:0x2731, 8bit one byte space:0x2701)
	writel(0x32700, &mipi->csiiw_regs->csiiw_config0);              // Disable csiiw, fs_irq and fe_irq
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

	MIP_INFO("Installed csiiw interrupts (fs_irq=%d, fe_irq=%d).\n", mipi->fs_irq , mipi->fe_irq);
	return 0;

err_fe_irq:
err_fs_irq:
	MIP_ERR("request_irq failed (%d)!\n", ret);
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

	DBG_INFO("%s: count = %u, size = %u\n", __FUNCTION__, *nbuffers, sizes[0]);
	return 0;
}

static int sp_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = mipi->fmt.fmt.pix.sizeimage;

	vb2_set_plane_payload(vb, 0, mipi->fmt.fmt.pix.sizeimage);

	if (vb2_get_plane_payload(vb, 0) > vb2_plane_size(vb, 0)) {
		MIP_ERR("Buffer is too small (%lu < %lu)!\n", vb2_plane_size(vb, 0), size);
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
	DBG_INFO("%s: list_add\n", __FUNCTION__);
	print_List(&mipi->dma_queue);
	spin_unlock_irqrestore(&mipi->dma_queue_lock, flags);
}

static int sp_start_streaming(struct vb2_queue *vq, unsigned count)
{
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vq);
	struct sp_mipi_subdev_info *sdinfo;
	struct sp_buffer *buf, *tmp;
	unsigned long flags;
	unsigned long addr;
	u32 analog_cfg1;
	int ret;

	DBG_INFO("%s\n", __FUNCTION__);

	if (mipi->streaming) {
		MIP_ERR("Device has started streaming!\n");
		return -EBUSY;
	}

	mipi->sequence = 0;

	sdinfo = mipi->current_subdev;
	ret = v4l2_device_call_until_err(&mipi->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 1);
	if (ret && (ret != -ENOIOCTLCMD)) {
		MIP_ERR("streamon failed in subdevice!\n");

		list_for_each_entry_safe(buf, tmp, &mipi->dma_queue, list) {
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_QUEUED);
		}

		return ret;
	}

	spin_lock_irqsave(&mipi->dma_queue_lock, flags);

	// Get the next frame from the dma queue.
	mipi->cur_frm = list_entry(mipi->dma_queue.next, struct sp_buffer, list);

	// Remove buffer from the dma queue.
	list_del_init(&mipi->cur_frm->list);

	addr = vb2_dma_contig_plane_dma_addr(&mipi->cur_frm->vb.vb2_buf, 0);

	spin_unlock_irqrestore(&mipi->dma_queue_lock, flags);

	writel(addr, &mipi->csiiw_regs->csiiw_base_addr);
	writel(mEXTENDED_ALIGNED(mipi->fmt.fmt.pix.bytesperline, 16), &mipi->csiiw_regs->csiiw_stride);
	writel((mipi->fmt.fmt.pix.height<<16)|mipi->fmt.fmt.pix.bytesperline, &mipi->csiiw_regs->csiiw_frame_size);

	// Reset Serial-to-parallel Flip-flop
	analog_cfg1 = readl(&mipi->mipicsi_regs->mipi_analog_cfg1);
	analog_cfg1 |= 0x0001;
	udelay(1);
	writel(analog_cfg1, &mipi->mipicsi_regs->mipi_analog_cfg1);
	udelay(1);
	analog_cfg1 &= ~1;
	writel(analog_cfg1, &mipi->mipicsi_regs->mipi_analog_cfg1);

	// Enable csiiw and fe_irq
	writel(0x12701, &mipi->csiiw_regs->csiiw_config0);

	mipi->streaming = true;
	mipi->skip_first_int = true;

	DBG_INFO("%s: cur_frm = %p, addr = %08lx\n", __FUNCTION__, mipi->cur_frm, addr);

	return 0;
}

static void sp_stop_streaming(struct vb2_queue *vq)
{
	struct sp_mipi_device *mipi = vb2_get_drv_priv(vq);
	struct sp_mipi_subdev_info *sdinfo;
	struct sp_buffer *buf;
	unsigned long flags;
	int ret;

	DBG_INFO("%s\n", __FUNCTION__);

	if (!mipi->streaming) {
		MIP_ERR("Device has stopped already!\n");
		return;
	}

	sdinfo = mipi->current_subdev;
	ret = v4l2_device_call_until_err(&mipi->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 0);
	if (ret && (ret != -ENOIOCTLCMD)) {
		MIP_ERR("streamon failed in subdevice!\n");
		return;
	}

	// FW must mask irq to avoid unmap issue (for test code)
	writel(0x32700, &mipi->csiiw_regs->csiiw_config0);      // Disable csiiw, fs_irq and fe_irq

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
		DBG_INFO("video buffer #%d is done!\n", buf->vb.vb2_buf.index);
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

	DBG_INFO("%s\n", __FUNCTION__);

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

	DBG_INFO("%s: index = %d\n", __FUNCTION__, fmtdesc->index);

	if (fmtdesc->index >= mipi->current_subdev->formats_size) {
		return -EINVAL;
	}

	if (fmtdesc->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		MIP_ERR("Invalid V4L2 buffer type!\n");
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

	DBG_INFO("%s\n", __FUNCTION__);

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		MIP_ERR("Invalid V4L2 buffer type!\n");
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

	DBG_INFO("%s\n", __FUNCTION__);

	if (mipi->streaming) {
		MIP_ERR("Device has started streaming!\n");
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

	DBG_INFO("%s\n", __FUNCTION__);

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

	DBG_INFO("%s\n", __FUNCTION__);

#ifdef CONFIG_PM_RUNTIME_MIPI
	if (pm_runtime_get_sync(mipi->pdev) < 0)
		goto out;
#endif

	mutex_lock(&mipi->lock);

	ret = v4l2_fh_open(file);
	if (ret) {
		MIP_ERR("v4l2_fh_open failed!\n");
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

	DBG_INFO("%s\n", __FUNCTION__);

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
	const struct sp_fmt *cur_fmt;
	int num_subdevs = 0;
	int ret, i;

	DBG_INFO("%s\n", __FUNCTION__);

	// Allocate memory for 'sp_mipi_device'.
	mipi = kzalloc(sizeof(struct sp_mipi_device), GFP_KERNEL);
	if (!mipi) {
		MIP_ERR("Failed to allocate memory!\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mipi->pdev = &pdev->dev;
	mipi->caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;

	// Set the driver data in platform device.
	platform_set_drvdata(pdev, mipi);

	// Get and set 'mipicsi' register base.
	ret = sp_mipi_get_register_base(pdev, (void**)&mipi->mipicsi_regs, MIPICSI_REG_NAME);
	if (ret) {
		return ret;
	}

	// Get and set 'csiiw' register base.
	ret = sp_mipi_get_register_base(pdev, (void**)&mipi->csiiw_regs, CSIIW_REG_NAME);
	if (ret) {
		return ret;
	}
	MIP_INFO("%s: mipicsi_regs = 0x%p, csiiw_regs = 0x%p\n",
		__FUNCTION__, mipi->mipicsi_regs, mipi->csiiw_regs);

	// Get clock resource 'clk_mipicsi'.
	mipi->mipicsi_clk = devm_clk_get(dev, "clk_mipicsi");
	if (IS_ERR(mipi->mipicsi_clk)) {
		ret = PTR_ERR(mipi->mipicsi_clk);
		MIP_ERR("Failed to retrieve clock resource \'clk_mipicsi\'!\n");
		goto err_get_mipicsi_clk;
	}

	// Get clock resource 'clk_csiiw'.
	mipi->csiiw_clk = devm_clk_get(dev, "clk_csiiw");
	if (IS_ERR(mipi->csiiw_clk)) {
		ret = PTR_ERR(mipi->csiiw_clk);
		MIP_ERR("Failed to retrieve clock resource \'clk_csiiw\'!\n");
		goto err_get_csiiw_clk;
	}

	// Get reset controller resource 'rstc_mipicsi'.
	mipi->mipicsi_rstc = devm_reset_control_get(&pdev->dev, "rstc_mipicsi");
	if (IS_ERR(mipi->mipicsi_rstc)) {
		ret = PTR_ERR(mipi->mipicsi_rstc);
		MIP_ERR("Failed to retrieve reset controller 'rstc_mipicsi\'!\n");
		goto err_get_mipicsi_rstc;
	}

	// Get reset controller resource 'rstc_csiiw'.
	mipi->csiiw_rstc = devm_reset_control_get(&pdev->dev, "rstc_csiiw");
	if (IS_ERR(mipi->csiiw_rstc)) {
		ret = PTR_ERR(mipi->csiiw_rstc);
		MIP_ERR("Failed to retrieve reset controller 'rstc_csiiw\'!\n");
		goto err_get_csiiw_rstc;
	}

	// Get cam_gpio0.
	mipi->cam_gpio0 = devm_gpiod_get(&pdev->dev, "cam_gpio0", GPIOD_OUT_HIGH);
	if (!IS_ERR(mipi->cam_gpio0)) {
		MIP_INFO("cam_gpio0 is at G_MX[%d].\n", desc_to_gpio(mipi->cam_gpio0));
		gpiod_set_value(mipi->cam_gpio0,1);
	}

	// Get cam_gpio1.
	mipi->cam_gpio1 = devm_gpiod_get(&pdev->dev, "cam_gpio1", GPIOD_OUT_HIGH);
	if (!IS_ERR(mipi->cam_gpio1)) {
		MIP_INFO("cam_gpio1 is at G_MX[%d].\n", desc_to_gpio(mipi->cam_gpio1));
		gpiod_set_value(mipi->cam_gpio1,1);
	}

	// Get i2c id.
	ret = of_property_read_u32(pdev->dev.of_node, "i2c-id", &mipi->i2c_id);
	if (ret) {
		MIP_ERR("Failed to retrieve \'i2c-id\'!\n");
		goto err_get_i2c_id;
	}

	// Enable 'mipicsi' clock.
	ret = clk_prepare_enable(mipi->mipicsi_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'mipicsi\' clock!\n");
		goto err_en_mipicsi_clk;
	}

	// Enable 'csiiw' clock.
	ret = clk_prepare_enable(mipi->csiiw_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'csiiw\' clock!\n");
		goto err_en_csiiw_clk;
	}

	// De-assert 'mipicsi' reset controller.
	ret = reset_control_deassert(mipi->mipicsi_rstc);
	if (ret) {
		MIP_ERR("Failed to deassert 'mipicsi' reset controller!\n");
		goto err_deassert_mipicsi_rstc;
	}

	// De-assert 'csiiw' reset controller.
	ret = reset_control_deassert(mipi->csiiw_rstc);
	if (ret) {
		MIP_ERR("Failed to deassert 'csiiw' reset controller!\n");
		goto err_deassert_csiiw_rstc;
	}

	// Register V4L2 device.
	ret = v4l2_device_register(&pdev->dev, &mipi->v4l2_dev);
	if (ret) {
		MIP_ERR("Unable to register V4L2 device!\n");
		goto err_v4l2_register;
	}
	MIP_INFO("Registered V4L2 device.\n");

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
		MIP_ERR("Failed to initialize vb2 queue!\n");
		goto err_vb2_queue_init;
	}

	// Initialize dma queues.
	INIT_LIST_HEAD(&mipi->dma_queue);

	// Initialize field of video device.
	vfd = &mipi->video_dev;
	vfd->fops       = &sp_mipi_fops;
	vfd->ioctl_ops  = &sp_mipi_ioctl_ops;
	vfd->device_caps= mipi->caps;
	vfd->release    = video_device_release_empty;
	vfd->v4l2_dev   = &mipi->v4l2_dev;
	vfd->queue      = &mipi->buffer_queue;
	//vfd->tvnorms  = 0;
	strlcpy(vfd->name, MIPI_CSI_RX_NAME, sizeof(vfd->name));
	vfd->lock       = &mipi->lock;  // protect all fops and v4l2 ioctls.

	video_set_drvdata(vfd, mipi);

	// Register video device.
	ret = video_register_device(&mipi->video_dev, VFL_TYPE_GRABBER, video_nr);
	if (ret) {
		MIP_ERR("Unable to register video device!\n");
		ret = -ENODEV;
		goto err_video_register;
	}
	MIP_INFO("Registered video device \'/dev/video%d\'.\n", vfd->num);

	// Get i2c_info for sub-device.
	sp_mipi_cfg = kmalloc(sizeof(*sp_mipi_cfg), GFP_KERNEL);
	if (!sp_mipi_cfg) {
		MIP_ERR("Failed to allocate memory for \'sp_mipi_cfg\'!\n");
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
		MIP_ERR("Failed to get i2c adapter #%d!\n", sp_mipi_cfg->i2c_adapter_id);
		ret = -ENODEV;
		goto err_i2c_get_adapter;
	}
	mipi->i2c_adap = i2c_adap;
	MIP_INFO("Got i2c adapter #%d.\n", sp_mipi_cfg->i2c_adapter_id);

	for (i = 0; i < num_subdevs; i++) {
		sdinfo = &sp_mipi_cfg->sub_devs[i];

		// Load up the subdevice.
		subdev = v4l2_i2c_new_subdev_board(&mipi->v4l2_dev,
						    i2c_adap,
						    &sdinfo->board_info,
						    NULL);
		if (subdev) {
			subdev->grp_id = sdinfo->grp_id;
			MIP_INFO("Registered V4L2 subdevice \'%s\'.\n", sdinfo->name);
			break;
		}
	}
	if (i == num_subdevs) {
		MIP_ERR("Failed to register V4L2 subdevice!\n");
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
		goto err_get_format;
	}
	mipi->cur_format = cur_fmt;

	mipicsi_init(mipi);
	csiiw_init(mipi);

	ret = csiiw_irq_init(mipi);
	if (ret) {
		goto err_csiiw_irq_init;
	}

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
	return 0;

err_get_format:
err_csiiw_irq_init:
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
err_deassert_csiiw_rstc:
err_deassert_mipicsi_rstc:
	clk_disable_unprepare(mipi->csiiw_clk);

err_en_csiiw_clk:
	clk_disable_unprepare(mipi->mipicsi_clk);

err_en_mipicsi_clk:
err_get_i2c_id:
err_get_csiiw_rstc:
err_get_mipicsi_rstc:
err_get_csiiw_clk:
err_get_mipicsi_clk:
err_alloc:
	kfree(mipi);
	return ret;
}

static int sp_mipi_remove(struct platform_device *pdev)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);

	DBG_INFO("%s\n", __FUNCTION__);

	i2c_put_adapter(mipi->i2c_adap);
	kfree(mipi->cfg);

	video_unregister_device(&mipi->video_dev);
	v4l2_device_unregister(&mipi->v4l2_dev);

	clk_disable_unprepare(mipi->csiiw_clk);
	clk_disable_unprepare(mipi->mipicsi_clk);

	kfree(mipi);

#ifdef CONFIG_PM_RUNTIME_MIPI
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
#endif

	return 0;
}

static int sp_mipi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);

	MIP_INFO("MIPI suspend.\n");

	// Disable 'mipicsi' and 'csiiw' clock.
	clk_disable(mipi->mipicsi_clk);
	clk_disable(mipi->csiiw_clk);

	return 0;
}

static int sp_mipi_resume(struct platform_device *pdev)
{
	struct sp_mipi_device *mipi = platform_get_drvdata(pdev);
	int ret;

	MIP_INFO("MIPI resume.\n");

	// Enable 'mipicsi' clock.
	ret = clk_prepare_enable(mipi->mipicsi_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'mipicsi\' clock!\n");
	}

	// Enable 'csiiw' clock.
	ret = clk_prepare_enable(mipi->csiiw_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'csiiw\' clock!\n");
	}

	return 0;
}

#ifdef CONFIG_PM_RUNTIME_MIPI
static int sp_mipi_runtime_suspend(struct device *dev)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);

	MIP_INFO("MIPI runtime suspend.\n");

	// Disable 'mipicsi' and 'csiiw' clock.
	clk_disable(mipi->mipicsi_clk);
	clk_disable(mipi->csiiw_clk);

	return 0;
}

static int sp_mipi_runtime_resume(struct device *dev)
{
	struct sp_mipi_device *mipi = dev_get_drvdata(dev);
	int ret;

	MIP_INFO("MIPI runtime resume.\n");

	// Enable 'mipicsi' clock.
	ret = clk_prepare_enable(mipi->mipicsi_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'mipicsi\' clock!\n");
	}

	// Enable 'csiiw' clock.
	ret = clk_prepare_enable(mipi->csiiw_clk);
	if (ret) {
		MIP_ERR("Failed to enable \'csiiw\' clock!\n");
	}

	return 0;
}

static const struct dev_pm_ops sp7021_mipicsi_rx_pm_ops = {
	.runtime_suspend = sp_mipi_runtime_suspend,
	.runtime_resume  = sp_mipi_runtime_resume,
};
#endif

static const struct of_device_id sp_mipicsi_rx_of_match[] = {
	{ .compatible = "sunplus,sp7021-mipicsi-rx", },
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
