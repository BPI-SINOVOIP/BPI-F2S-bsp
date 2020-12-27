// SPDX-License-Identifier: GPL-2.0+
/*
 * SUNPLUS I143 HDMI display driver
 *
 * (C) Copyright 2020 hammer.hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include <display.h>
#include <dm.h>
#include <malloc.h>
#include <video.h>
#include <dm/uclass-internal.h>
#include <linux/io.h>
#include "display2.h"
#include "reg_disp.h"
#include "disp_dmix.h"
#include "disp_tgen.h"
#include "disp_dve.h"
#include "disp_vpp.h"
#include "disp_osd.h"
#include "disp_hdmitx.h"

extern u32 osd0_header[];

DECLARE_GLOBAL_DATA_PTR;

struct i143_disp_priv {
	void __iomem *regs;
	struct display_timing timing;
};

void disp_set_output_resolution(int is_hdmi, int width, int height)
{
    int mode = 0;
    DRV_VideoFormat_e fmt;
    DRV_FrameRate_e fps;

	if(is_hdmi) { //hdmitx output
		if((width == 720)&&(height == 480)) {
			mode = 0;
		}
		else if((width == 720)&&(height == 576)) {
			mode = 1;
		}
		else if((width == 1280)&&(height == 720)) {
			mode = 2;
		}
		else if((width == 1920)&&(height == 1080)) {
			mode = 4;
		}
		else {
			mode = 0;
		}
        //hdmi_clk_init(mode);
		//debug("hdmitx output , mode = %d \n", mode);
	}

    DRV_DVE_SetMode(mode);

	switch (mode)
	{
		default:
		case 0:
            debug("hdmitx output , 480P 59.94Hz \n");
			fmt = DRV_FMT_480P;
			fps = DRV_FrameRate_5994Hz;
			hdmi_clk_init(0);
            hdmitx_set_timming(0);
			break;
		case 1:
			fmt = DRV_FMT_576P;
			fps = DRV_FrameRate_50Hz;
            debug("hdmitx output , 576P 50Hz \n");
			hdmi_clk_init(1);
            hdmitx_set_timming(1);
			break;
		case 2:
            debug("hdmitx output , 720P 59.94Hz \n");
			fmt = DRV_FMT_720P;
			fps = DRV_FrameRate_5994Hz;
			hdmi_clk_init(2);
            hdmitx_set_timming(2);
			break;
		case 3:
			debug("hdmitx output , 720P 50Hz \n");
			fmt = DRV_FMT_720P;
			fps = DRV_FrameRate_50Hz;
			break;
		case 4:
			debug("hdmitx output , 1080P 59.94Hz \n");
			fmt = DRV_FMT_1080P;
			fps = DRV_FrameRate_5994Hz;
			hdmi_clk_init(3);
			hdmitx_set_timming(3);
			break;
		case 5:
			debug("hdmitx output , 1080P 50Hz \n");
			fmt = DRV_FMT_1080P;
			fps = DRV_FrameRate_50Hz;
			break;
		case 6:
			debug("hdmitx output , 1080P 24Hz \n");
			fmt = DRV_FMT_1080P;
			fps = DRV_FrameRate_24Hz;
			break;
		case 7:
			fmt = DRV_FMT_USER_MODE;
			fps = DRV_FrameRate_5994Hz;
			debug("set TGEN user mode\n");
			break;
	}

    DRV_TGEN_Set(fmt, fps);

}

static int i143_display_probe(struct udevice *dev)
{
	struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	int max_bpp = 8; //default 8BPP format
	int is_hdmi = 1; //default HDMITX out
	int width, height;
	void *fb_alloc;

	width = CONFIG_VIDEO_I143_MAX_XRES;
	height = CONFIG_VIDEO_I143_MAX_YRES;

	debug("Disp: probe ... \n");

	#if 0 //TBD, for DTS
	priv->regs = (void *)dev_read_addr_index(dev,0);
	if ((fdt_addr_t)priv->regs == FDT_ADDR_T_NONE) {
		debug("%s: i143 dt register address error\n", __func__);
		return -EINVAL;
	}
	else {
		debug("%s: i143 dt register address %px \n", __func__, priv->regs);
	}
	#endif

	fb_alloc = malloc((CONFIG_VIDEO_I143_MAX_XRES*
					CONFIG_VIDEO_I143_MAX_YRES*
					(CONFIG_VIDEO_I143_MAX_BPP >> 3)) + 64 );

	if (fb_alloc == NULL) {
		debug("Error: malloc in %s failed! \n",__func__);
		return -EINVAL;
	}

	fb_alloc = (void *)((u64)fb_alloc & (~0x80000000));

	if(((u64)fb_alloc & 0x3f) != 0)
		fb_alloc = (void *)(((uintptr_t)fb_alloc + 64 ) & ~0x3f);

	DRV_DMIX_Init();
	DRV_TGEN_Init(is_hdmi, width, height);
	DRV_DVE_Init(is_hdmi, width, height);
	DRV_VPP_Init();
	DRV_OSD_Init();
	DRV_hdmitx_Init(is_hdmi, width, height);
	
	DRV_DMIX_Layer_Init(DRV_DMIX_BG, DRV_DMIX_AlphaBlend, DRV_DMIX_PTG);
	DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_Opacity, DRV_DMIX_VPP0);
	DRV_DMIX_Layer_Init(DRV_DMIX_L5, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD1);
	DRV_DMIX_Layer_Init(DRV_DMIX_L6, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD0);

	vscl_setting(0, 0, width, height, width, height);
	vppdma_setting(0, 0, width, height, 0);

	disp_set_output_resolution(is_hdmi, width, height);

	if(CONFIG_VIDEO_I143_MAX_BPP == 16) {
		API_OSD_UI_Init(width ,height, (u32)((u64)fb_alloc), DRV_OSD_REGION_FORMAT_RGB_565);
		max_bpp = 16;
	}
	else if(CONFIG_VIDEO_I143_MAX_BPP == 32) {
		API_OSD_UI_Init(width ,height, (u32)((u64)fb_alloc), DRV_OSD_REGION_FORMAT_ARGB_8888);
		max_bpp = 32;
	}
	else {
		API_OSD_UI_Init(width ,height, (u32)((u64)fb_alloc), DRV_OSD_REGION_FORMAT_8BPP);
		max_bpp = 8;
	}

	uc_plat->base = (u64)fb_alloc;
	uc_plat->size = width * height * (max_bpp >> 3);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->rot = CONFIG_VIDEO_I143_ROTATE;

	if(CONFIG_VIDEO_I143_MAX_BPP == 16) {
		uc_priv->bpix = VIDEO_BPP16;
	}
	else if(CONFIG_VIDEO_I143_MAX_BPP == 32) {
		uc_priv->bpix = VIDEO_BPP32;
	}
	else {
		uc_priv->bpix = VIDEO_BPP8;
	}

	#ifdef CONFIG_BMP_8BPP_UPDATE_CMAP
	if(uc_priv->bpix == VIDEO_BPP8) {
		uc_priv->cmap = (u32 *)(osd0_header+32);
	}
	#endif

	video_set_flush_dcache(dev, true);

	debug("Disp: probe done \n");

	return 0;
}

static int i143_display_bind(struct udevice *dev)
{
	struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);

	uc_plat->size = 0;

	return 0;
}

static const struct video_ops i143_display_ops = {
};

static const struct udevice_id i143_display_ids[] = {
	{ .compatible = "sunplus,i143-display" },
	{ }
};

U_BOOT_DRIVER(i143_display) = {
	.name	= "i143_display",
	.id	= UCLASS_VIDEO,
	.ops	= &i143_display_ops,
	.of_match = i143_display_ids,
	.bind	= i143_display_bind,
	.probe	= i143_display_probe,
	.priv_auto_alloc_size	= sizeof(struct i143_disp_priv),
};

