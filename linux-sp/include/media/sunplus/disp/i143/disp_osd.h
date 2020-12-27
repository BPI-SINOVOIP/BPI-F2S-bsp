/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Sunplus I143 Display driver osd header file
 *
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

#ifndef __DISP_OSD_H__
#define __DISP_OSD_H__

#include <linux/fb.h>

typedef enum {
	DRV_OSD0 = 0,
	DRV_OSD1,
	DRV_OSD_MAX
} DRV_OsdWindow_e;

enum DRV_OsdRegionFormat_e {
	/* 8 bit/pixel with CLUT */
	DRV_OSD_REGION_FORMAT_8BPP			= 0x2,
	/* 16 bit/pixel YUY2 */
	DRV_OSD_REGION_FORMAT_YUY2			= 0x4,
	/* 16 bit/pixel RGB 5:6:5 */
	DRV_OSD_REGION_FORMAT_RGB_565		= 0x8,
	/* 16 bit/pixel ARGB 1:5:5:5 */
	DRV_OSD_REGION_FORMAT_ARGB_1555		= 0x9,
	/* 16 bit/pixel RGBA 4:4:4:4 */
	DRV_OSD_REGION_FORMAT_RGBA_4444		= 0xA,
	/* 16 bit/pixel ARGB 4:4:4:4 */
	DRV_OSD_REGION_FORMAT_ARGB_4444		= 0xB,
	/* 32 bit/pixel RGBA 8:8:8:8 */
	DRV_OSD_REGION_FORMAT_RGBA_8888		= 0xD,
	/* 32 bit/pixel ARGB 8:8:8:8 */
	DRV_OSD_REGION_FORMAT_ARGB_8888		= 0xE,
};

enum DRV_OsdTransparencyMode_e {
	DRV_OSD_TRANSPARENCY_DISABLED = 0,	/*!< transparency is disabled */
	DRV_OSD_TRANSPARENCY_ALL			/*!< the whole region is transparent */
};

#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
typedef u32 DRV_OsdRegionHandle_t;
#else
typedef u64 DRV_OsdRegionHandle_t;
#endif

enum DRV_OsdBlendMethod_e {
	DRV_OSD_BLEND_REPLACE,		/*!< OSD blend method is region alpha replace */
	DRV_OSD_BLEND_MULTIPLIER,	/*!< OSD blend method is region alpha multiplier */
	MAX_BLEND_METHOD,
};

typedef struct _DRV_Region_Info_t
{
	u32 bufW;
	u32 bufH;
	u32 startX;
	u32 startY;
	u32 actW;
	u32 actH;
} DRV_Region_Info_t;

struct colormode_t {
	char name[24];
	unsigned int bits_per_pixel;
	unsigned int nonstd;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

struct UI_FB_Info_t {
	unsigned int UI_width;
	unsigned int UI_height;
	unsigned int UI_bufNum;
	unsigned int UI_bufAlign;
	enum DRV_OsdRegionFormat_e UI_ColorFmt;
	struct colormode_t UI_Colormode;

#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
	u32 UI_bufAddr;
	u32 UI_bufAddr_pal;		/* palette */
#else
	u64 UI_bufAddr;
	u64 UI_bufAddr_pal;		/* palette */
#endif
	unsigned int UI_bufsize;
	unsigned int UI_handle;
};

void DRV_OSD_IRQ(void);
void DRV_OSD_Init(void *pInHWReg1, void *pInHWReg2);
void DRV_OSD1_Init(void *pInHWReg1, void *pInHWReg2);
void DRV_OSD_WaitVSync(void);

//#ifdef SUPPORT_DEBUG_MON
void DRV_OSD_Info(void);
void DRV_OSD_HDR_Show(void);
void DRV_OSD_HDR_Write(int offset, int value);
//#endif

void DRV_OSD0_off(void);
void DRV_OSD0_on(void);
void DRV_OSD1_off(void);
void DRV_OSD1_on(void);

int DRV_OSD_Get_UI_Res(struct UI_FB_Info_t *pinfo);
void DRV_OSD_Set_UI_UnInit(void);
void DRV_OSD_Set_UI_Init(struct UI_FB_Info_t *pinfo);
u32 DRV_OSD_SetVisibleBuffer(u32 bBufferId);
void DRV_IRQ_DISABLE(void);
void DRV_IRQ_ENABLE(void);

extern int g_disp_state;

void osd_path_cur_setting_read(void);

void DRV_OSD_SetColorbar(DRV_OsdWindow_e osd_layer, int enable);
int osd0_setting(int base_addr, int w, int h, int osd0_fmt);
int osd1_setting(int base_addr, int w, int h, int osd1_fmt);
void DRV_OSD_INIT_OSD_Header(void);
void DRV_OSD_Clear_OSD_Header(int osd_layer);
void DRV_OSD_SET_OSD_Header(struct UI_FB_Info_t *pinfo, int osd_layer);
void DRV_OSD0_dump(void);
void DRV_OSD1_dump(void);

#endif	//__DISP_OSD_H__
