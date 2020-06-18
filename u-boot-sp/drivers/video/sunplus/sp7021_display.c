// SPDX-License-Identifier: GPL-2.0+
/*
 * SUNPLUS SP7021 HDMI/TTL display driver
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

struct sp7021_disp_priv {
	void __iomem *regs;
	struct display_timing timing;
};

void ttl_pinmux_init(int is_hdmi)
{
    if(!is_hdmi)
        DISP_MOON1_REG->sft_cfg[4] = 0x00400040; //enable lcdif
    else 
        DISP_MOON1_REG->sft_cfg[4] = 0x00400000; //disable lcdif
}

void ttl_clk_init(int method)
{
    if (method == 0) {
        DISP_MOON4_REG->plltv_ctl[0] = 0x80418041; //en pll , clk = 27M //G4.14
        //DISP_MOON4_REG->plltv_ctl[1] = 0x00000000; //G4.15
        DISP_MOON4_REG->plltv_ctl[2] = 0xFFFF0000; //don't care //G4.16
        DISP_MOON4_REG->otp_st = 0x00200020; //clk div4 //G4.31
    }
    else {
        //with  formula ( FVCO = (27/(M+1)) * (N+1) )
        DISP_MOON4_REG->plltv_ctl[0] = 0x80020000; //don't bypass //G4.14
        //DISP_MOON4_REG->plltv_ctl[1] = 0x00000000; //G4.15
        DISP_MOON4_REG->plltv_ctl[2] = 0xFFFF1f27; //en pll , clk = 33.75M //(FVCO= (27/(M+1)) * (N+1) ) M=31,N=39 //G4.16
        DISP_MOON4_REG->otp_st = 0x00300000; //clk no div //G4.31
    }
}

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
        hdmi_clk_init(mode);
		debug("hdmitx output , mode = %d \n", mode);
	}
	else { //TTL output
		mode = 7;
		debug("TTL output , mode = %d \n", mode);
	}

    DRV_DVE_SetMode(mode);

	switch (mode)
	{
		default:
		case 0:
            debug("hdmitx output , 480P 59.94Hz \n");
			fmt = DRV_FMT_480P;
			fps = DRV_FrameRate_5994Hz;
            hdmitx_set_timming(0);
			break;
		case 1:
			fmt = DRV_FMT_576P;
			fps = DRV_FrameRate_50Hz;
            debug("hdmitx output , 576P 50Hz \n");
            hdmitx_set_timming(1);
			break;
		case 2:
            debug("hdmitx output , 720P 59.94Hz \n");
			fmt = DRV_FMT_720P;
			fps = DRV_FrameRate_5994Hz;
            hdmitx_set_timming(2);
			break;
		case 3:
			fmt = DRV_FMT_720P;
			fps = DRV_FrameRate_50Hz;
			break;
		case 4:
			fmt = DRV_FMT_1080P;
			fps = DRV_FrameRate_5994Hz;
			break;
		case 5:
			fmt = DRV_FMT_1080P;
			fps = DRV_FrameRate_50Hz;
			break;
		case 6:
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

static int sp7021_display_probe(struct udevice *dev)
{
	struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	int max_bpp = 8; //default 8BPP format
	int is_hdmi = 1; //default HDMITX out
	int width, height;
	void *fb_alloc;

	#ifdef CONFIG_EN_SP7021_TTL
	is_hdmi = 0; //Switch to TTL out
	#else	
	#endif
	width = CONFIG_VIDEO_SP7021_MAX_XRES;
	height = CONFIG_VIDEO_SP7021_MAX_YRES;

	debug("Disp: probe ... \n");

	#if 0 //TBD, for DTS
	priv->regs = (void *)dev_read_addr_index(dev,0);
	if ((fdt_addr_t)priv->regs == FDT_ADDR_T_NONE) {
		printf("%s: sp7021 dt register address error\n", __func__);
		return -EINVAL;
	}
	else {
		printf("%s: sp7021 dt register address %px \n", __func__, priv->regs);
	}
	#endif

	fb_alloc = malloc((CONFIG_VIDEO_SP7021_MAX_XRES*
					CONFIG_VIDEO_SP7021_MAX_YRES*
					(CONFIG_VIDEO_SP7021_MAX_BPP >> 3)) + 64 );
	if (fb_alloc == NULL) {
		printf("Error: malloc in %s failed! \n",__func__);
		return -EINVAL;
	}

	if(((u32)fb_alloc & 0x1f) != 0)
		fb_alloc = (void *)(((uintptr_t)fb_alloc + 32 ) & ~0x1f);

    ttl_pinmux_init(is_hdmi);
    if(!is_hdmi) {
        ttl_clk_init(1);
    }

	DRV_DMIX_Init();
	DRV_TGEN_Init(is_hdmi, width, height);
	DRV_DVE_Init(is_hdmi, width, height);
	DRV_VPP_Init();
	DRV_OSD_Init();
	DRV_hdmitx_Init(is_hdmi, width, height);

	DRV_DMIX_Layer_Init(DRV_DMIX_BG, DRV_DMIX_AlphaBlend, DRV_DMIX_PTG);
	DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_Transparent, DRV_DMIX_VPP0);
	DRV_DMIX_Layer_Init(DRV_DMIX_L6, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD0);

	disp_set_output_resolution(is_hdmi, width, height);

	if(CONFIG_VIDEO_SP7021_MAX_BPP == 16) {
		API_OSD_UI_Init(width ,height, (u32)fb_alloc, DRV_OSD_REGION_FORMAT_RGB_565);
		max_bpp = 16;
	}
	else if(CONFIG_VIDEO_SP7021_MAX_BPP == 32) {
		API_OSD_UI_Init(width ,height, (u32)fb_alloc, DRV_OSD_REGION_FORMAT_ARGB_8888);
		max_bpp = 32;
	}
	else {
		API_OSD_UI_Init(width ,height, (u32)fb_alloc, DRV_OSD_REGION_FORMAT_8BPP);
		max_bpp = 8;
	}

	uc_plat->base = (ulong)fb_alloc;
	uc_plat->size = width * height * (max_bpp >> 3);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->rot = CONFIG_VIDEO_SP7021_ROTATE;

	if(CONFIG_VIDEO_SP7021_MAX_BPP == 16) {
		uc_priv->bpix = VIDEO_BPP16;
	}
	else if(CONFIG_VIDEO_SP7021_MAX_BPP == 32) {
		uc_priv->bpix = VIDEO_BPP32;
	}
	else {
		uc_priv->bpix = VIDEO_BPP8;
	}

	#ifdef CONFIG_BMP_8BPP_UPDATE_CMAP
	if(uc_priv->bpix == VIDEO_BPP8) {
		uc_priv->cmap = (u32 *)(u32)(osd0_header+32);
	}
	#endif

	video_set_flush_dcache(dev, true);

	debug("Disp: probe done \n");

	return 0;
}

static int sp7021_display_bind(struct udevice *dev)
{
	struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);

	uc_plat->size = 0;

	return 0;
}

static const struct video_ops sp7021_display_ops = {
};

static const struct udevice_id sp7021_display_ids[] = {
	{ .compatible = "sunplus,sp7021-display" },
	{ }
};

U_BOOT_DRIVER(sp7021_display) = {
	.name	= "sp7021_display",
	.id	= UCLASS_VIDEO,
	.ops	= &sp7021_display_ops,
	.of_match = sp7021_display_ids,
	.bind	= sp7021_display_bind,
	.probe	= sp7021_display_probe,
	.priv_auto_alloc_size	= sizeof(struct sp7021_disp_priv),
};

