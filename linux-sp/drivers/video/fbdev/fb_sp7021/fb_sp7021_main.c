/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2018 by Sunplus Inc.                             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Inc. All rights are reserved by Sunplus Inc.                          *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Inc. reserves the right to modify this software               *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus Inc.                                                          *
 *  19, Innovation First Road, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/

/**
 * @file fb_sp7021_main.c
 * @brief linux kernel framebuffer main driver
 * @author PoChou Chen
 */
/**************************************************************************
 * Header Files
 **************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>

#include <media/sp-disp/disp_osd.h>
#include "fb_sp7021_main.h"

/**************************************************************************
 * Constants
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
#define mod_dbg(pdev, fmt, arg...)	dev_dbg(&pdev->dev, \
		"[%s:%d] "fmt, \
		__func__, \
		__LINE__, \
		##arg)

#define mod_err(pdev, fmt, arg...)	dev_err(&pdev->dev, \
		"[%s:%d] Error! "fmt, \
		__func__, \
		__LINE__, \
		##arg)
#define mod_warn(pdev, fmt, arg...)	dev_warn(&pdev->dev, \
		"[%s:%d] Warning! "fmt, \
		__func__, \
		__LINE__, \
		##arg)
#define mod_info(pdev, fmt, arg...)	dev_info(&pdev->dev, \
		"[%s:%d] "fmt, \
		__func__, \
		__LINE__, \
		##arg)

/**************************************************************************
 * Data Types
 **************************************************************************/

/**************************************************************************
 * Static Functions
 **************************************************************************/
static int _sp7021_fb_pan_display(struct fb_var_screeninfo *var,
		struct fb_info *fbinfo);
static int _sp7021_fb_setcmap(struct fb_cmap *cmap, struct fb_info *info);
static int _sp7021_fb_ioctl(struct fb_info *fbinfo,
		unsigned int cmd,
		unsigned long arg);

static int _sp7021_fb_remove(struct platform_device *pdev);
static int _sp7021_fb_probe(struct platform_device *pdev);

/**************************************************************************
 * Global Data
 **************************************************************************/
struct fb_info *gFB_INFO;

static struct fb_ops framebuffer_ops = {
	.owner				= THIS_MODULE,
	.fb_pan_display		= _sp7021_fb_pan_display,
	.fb_setcmap			= _sp7021_fb_setcmap,
	.fb_ioctl			= _sp7021_fb_ioctl,
};

static const struct of_device_id _sp7021_fb_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-fb", },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, _sp7021_fb_dt_ids);

static struct platform_driver _sp7021_fb_driver = {
	.probe		= _sp7021_fb_probe,
	.remove		= _sp7021_fb_remove,
	.driver		= {
		.name	= "sp7021-fb",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(_sp7021_fb_dt_ids),
	},
};
module_platform_driver(_sp7021_fb_driver);

/**************************************************************************
 * Function Implementations
 **************************************************************************/
static int _sp7021_fb_create_device(struct platform_device *pdev,
		struct UI_FB_Info_t *Info)
{
	struct fb_info *fbinfo = NULL;
	struct framebuffer_t *fbWorkMem = NULL;
	dma_addr_t fb_phymem;

	if (!Info) {
		mod_err(pdev, "FB info is NULL\n");
		goto ERROR_HANDLE_FB_INIT;
	}

	fbinfo = framebuffer_alloc(sizeof(struct framebuffer_t), NULL);

	if (fbinfo == NULL) {
		mod_err(pdev, "framebuffer_alloc failed\n");
		goto ERROR_HANDLE_FB_INIT;
	}

	fbWorkMem = fbinfo->par;
	memset(fbWorkMem, 0, sizeof(struct framebuffer_t));
	fbWorkMem->fb = fbinfo;

	/* Initial our framebuffer context */
	fbWorkMem->ColorFmt = Info->UI_ColorFmt;
	snprintf(fbWorkMem->ColorFmtName,
			sizeof(fbWorkMem->ColorFmtName) - 1,
			"%s",
			Info->UI_Colormode.name);
	fbWorkMem->fbwidth = Info->UI_width;
	fbWorkMem->fbheight = Info->UI_height;

	if (Info->UI_bufAlign % PAGE_SIZE) {
		mod_err(pdev, "UI buf align: %d, Linux page align: %d\n",
				Info->UI_bufAlign,
				(int)PAGE_SIZE);
		goto ERROR_HANDLE_FB_INIT;
	}

	fbWorkMem->fbpagesize = DISP_ALIGN((fbWorkMem->fbwidth
			* fbWorkMem->fbheight
			* (Info->UI_Colormode.bits_per_pixel >> 3)),
			Info->UI_bufAlign);
	fbWorkMem->fbpagenum = Info->UI_bufNum;
	fbWorkMem->fbsize = fbWorkMem->fbpagesize * fbWorkMem->fbpagenum;

	/* Allocate Buffer */
	fbWorkMem->fbmem = (void __iomem *)dma_zalloc_coherent(&pdev->dev,
			fbWorkMem->fbsize, &fb_phymem, GFP_KERNEL);

	if (!fbWorkMem->fbmem) {
		mod_err(pdev, "malloc failed, size %d(%dx%dx(%d/8)*%d)\n",
			fbWorkMem->fbsize,
			fbWorkMem->fbwidth,
			fbWorkMem->fbheight,
			Info->UI_Colormode.bits_per_pixel,
			fbWorkMem->fbpagenum);
		goto ERROR_HANDLE_FB_INIT;
	}

	if (fbWorkMem->ColorFmt == DRV_OSD_REGION_FORMAT_8BPP) {
		fbWorkMem->fbmem_palette = kzalloc(FB_PALETTE_LEN, GFP_KERNEL);

		if (fbWorkMem->fbmem_palette == NULL)
			goto ERROR_HANDLE_FB_INIT;
	} else
		fbWorkMem->fbmem_palette = NULL;

	fbinfo->fbops = &framebuffer_ops;
	fbinfo->flags = FBINFO_FLAG_DEFAULT;

	if (fbWorkMem->ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		fbinfo->pseudo_palette = fbWorkMem->fbmem_palette;
	else
		fbinfo->pseudo_palette = NULL;
	fbinfo->screen_base = fbWorkMem->fbmem;
	fbinfo->screen_size = fbWorkMem->fbsize;

	/* Resolution */
	fbinfo->var.xres = fbWorkMem->fbwidth;
	fbinfo->var.yres = fbWorkMem->fbheight;
	fbinfo->var.xres_virtual = fbWorkMem->fbwidth;
	fbinfo->var.yres_virtual = fbWorkMem->fbheight * fbWorkMem->fbpagenum;

	/* Timing */
	/* fake setting fps 59.94 */
	fbinfo->var.left_margin		= 60;		/* HBProch */
	fbinfo->var.right_margin	= 16;		/* HFPorch */
	fbinfo->var.upper_margin	= 30;		/* VBPorch */
	fbinfo->var.lower_margin	= 10;		/* VFPorch */
	/* HSWidth */
	fbinfo->var.hsync_len		= 10 - ((fbWorkMem->fbwidth
				+ fbinfo->var.left_margin
				+ fbinfo->var.right_margin) % 10);
	/* VSWidth */
	fbinfo->var.vsync_len		= 10 - ((fbWorkMem->fbheight
				+ fbinfo->var.upper_margin
				+ fbinfo->var.lower_margin) % 10);
	/* 1000000000000/pixel clk */
	fbinfo->var.pixclock		= 1000000000
		/ ((fbWorkMem->fbwidth
					+ fbinfo->var.left_margin
					+ fbinfo->var.right_margin
					+ fbinfo->var.hsync_len) / 10)
		/ ((fbWorkMem->fbheight
					+ fbinfo->var.upper_margin
					+ fbinfo->var.lower_margin
					+ fbinfo->var.vsync_len) / 10)
		/ 6;
	fbinfo->var.pixclock += (fbinfo->var.pixclock / 1000)
		+ ((fbinfo->var.pixclock % 1000) ? 1 : 0);

	fbinfo->var.activate		= FB_ACTIVATE_NXTOPEN;
	fbinfo->var.accel_flags		= 0;
	fbinfo->var.vmode			= FB_VMODE_NONINTERLACED;

	/* color format */
	fbinfo->var.bits_per_pixel	= Info->UI_Colormode.bits_per_pixel;
	fbinfo->var.nonstd			= Info->UI_Colormode.nonstd;
	fbinfo->var.red				= Info->UI_Colormode.red;
	fbinfo->var.green			= Info->UI_Colormode.green;
	fbinfo->var.blue			= Info->UI_Colormode.blue;
	fbinfo->var.transp			= Info->UI_Colormode.transp;

	/* fixed info */
	snprintf(fbinfo->fix.id,
			sizeof(fbinfo->fix.id) - 1,
			"FB-%dx%d",
			fbWorkMem->fbwidth,
			fbWorkMem->fbheight);
	fbinfo->fix.mmio_start	= 0;
	fbinfo->fix.mmio_len	= 0;
	fbinfo->fix.type		= FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	= 0;

	if (fbWorkMem->ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		fbinfo->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		fbinfo->fix.visual = FB_VISUAL_TRUECOLOR;
	fbinfo->fix.xpanstep	= 0;
	fbinfo->fix.ypanstep	= 1;
	fbinfo->fix.ywrapstep	= 0;
	fbinfo->fix.accel		= FB_ACCEL_NONE;
	fbinfo->fix.smem_start	= fb_phymem;
	fbinfo->fix.smem_len	= fbWorkMem->fbsize;
	fbinfo->fix.line_length	= fbinfo->var.xres_virtual
		* (Info->UI_Colormode.bits_per_pixel >> 3);

	if (register_framebuffer(fbinfo) != 0) {
		mod_err(pdev, "register framebuffer failed\n");
		goto ERROR_HANDLE_FB_INIT;
	}

	Info->UI_bufAddr = (u32)fbinfo->fix.smem_start;
	if (fbinfo->pseudo_palette)
		Info->UI_bufAddr_pal = (u32)fbinfo->pseudo_palette;
	else
		Info->UI_bufAddr_pal = 0;
	Info->UI_bufsize = fbWorkMem->fbsize;

	mod_info(pdev, "mem VA 0x%x(PA 0x%x), Palette VA 0x%x(PA 0x%x), UI Res %dx%d, size %d + %d\n",
			(u32)fbWorkMem->fbmem,
			Info->UI_bufAddr,
			(u32)fbWorkMem->fbmem_palette,
			__pa(fbWorkMem->fbmem_palette),
			fbWorkMem->fbwidth,
			fbWorkMem->fbheight,
			fbWorkMem->fbsize,
			(fbWorkMem->ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
			? FB_PALETTE_LEN : 0);

	gFB_INFO = fbinfo;

	return 0;

ERROR_HANDLE_FB_INIT:
	if (Info)
		mod_err(pdev, "Create device error\n");

	kfree(fbWorkMem->fbmem_palette);

	if (fbinfo)
		framebuffer_release(fbinfo);

	return -1;
}

static int _sp7021_fb_pan_display(struct fb_var_screeninfo *var,
		struct fb_info *fbinfo)
{
	struct framebuffer_t *fb_par = (struct framebuffer_t *)fbinfo->par;
	struct platform_device *pdev = to_platform_device(fbinfo->dev);
	int ret = 0;

	fbinfo->var.xoffset = var->xoffset;
	fbinfo->var.yoffset = var->yoffset;

	ret = sp7021_fb_swapbuf(
			fbinfo->var.yoffset / fbinfo->var.yres,
			fb_par->fbpagenum);

	if (ret)
		mod_err(pdev, "swap buffer fails");

	return ret;
}

static int _sp7021_fb_ioctl(struct fb_info *fbinfo,
		unsigned int cmd,
		unsigned long arg)
{
	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		DRV_OSD_WaitVSync();
		break;
	}

	return 0;
}

static int _sp7021_fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	struct framebuffer_t *fb_par = (struct framebuffer_t *)info->par;
	int i;
	unsigned short *red, *green, *blue, *transp;
	unsigned short trans = ~0;
	unsigned int *palette = (unsigned int *)info->pseudo_palette;

	if ((fb_par->ColorFmt != DRV_OSD_REGION_FORMAT_8BPP) || (!palette))
		return -1;

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;

	for (i = cmap->start;
			(i < (cmap->start + cmap->len)) ||
				(i < (FB_PALETTE_LEN / sizeof(unsigned int)));
			++i) {
		if (transp)
			trans = *(transp++);

		palette[i] = sp7021_fb_chan_by_field((unsigned char)trans,
				&info->var.transp);
		palette[i] |= sp7021_fb_chan_by_field((unsigned char)*(red++),
				&info->var.red);
		palette[i] |= sp7021_fb_chan_by_field((unsigned char)*(green++),
				&info->var.green);
		palette[i] |= sp7021_fb_chan_by_field((unsigned char)*(blue++),
				&info->var.blue);
	}
	return 0;
}

static int _sp7021_fb_remove(struct platform_device *pdev)
{
	if (gFB_INFO) {
		if (unregister_framebuffer(gFB_INFO))
			mod_err(pdev, "unregister framebuffer error\n");
		framebuffer_release(gFB_INFO);
		gFB_INFO = NULL;
	}

	DRV_IRQ_DISABLE();
	
	DRV_OSD_Set_UI_UnInit();

	return 0;
}

static int _sp7021_fb_probe(struct platform_device *pdev)
{
	struct UI_FB_Info_t Info;
	struct framebuffer_t *fb_par = NULL;
	int ret;

	if (!pdev->dev.of_node) {
		mod_err(pdev, "invalid devicetree node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(pdev->dev.of_node)) {
		mod_err(pdev, "devicetree status is not available\n");
		return -ENODEV;
	}

	memset(&Info, 0, sizeof(struct UI_FB_Info_t));
	ret = DRV_OSD_Get_UI_Res(&Info);

	if (ret) {
		mod_dbg(pdev, "Get UI resolution fails");
		return -EPROBE_DEFER;
	}

	mod_dbg(pdev, "try Create device %dx%d, fmt %d\n",
			Info.UI_width,
			Info.UI_height,
			Info.UI_ColorFmt);

	if (Info.UI_width == 0 || Info.UI_height == 0)
		goto ERROR_HANDLE_FB_INIT;

	if (_sp7021_fb_create_device(pdev, &Info))
		goto ERROR_HANDLE_FB_INIT;

	DRV_OSD_Set_UI_Init(&Info);
	
	DRV_IRQ_ENABLE();

	fb_par = (struct framebuffer_t *)gFB_INFO->par;
	fb_par->OsdHandle = Info.UI_handle;

	return 0;

ERROR_HANDLE_FB_INIT:
	_sp7021_fb_remove(pdev);

	return -1;
}

/****************************************************************************
 * Public Function
 ****************************************************************************/
unsigned int sp7021_fb_chan_by_field(unsigned char chan,
		struct fb_bitfield *bf)
{
	unsigned int ret_chan = (unsigned int)chan;

	ret_chan >>= 8 - bf->length;

	return ret_chan << bf->offset;
}

int sp7021_fb_swapbuf(u32 buf_id, int buf_max)
{
	int ret = 0;

	if (buf_id >= buf_max)
		return -1;

	ret = DRV_OSD_SetVisibleBuffer(buf_id);

	return ret;
}

MODULE_DESCRIPTION("SP7021 Framebuffer Driver");
MODULE_AUTHOR("PoChou Chen <pochou.chen@sunplus.com>");
MODULE_LICENSE("GPL");

