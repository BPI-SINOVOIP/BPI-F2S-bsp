// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#ifndef __DISP_OSD_H__
#define __DISP_OSD_H__

#include "display2.h"

enum DRV_OsdWindow_e {
	DRV_OSD0 = 0,
	DRV_OSD1,
	DRV_OSD_MAX
};

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

typedef u32 DRV_OsdRegionHandle_t;

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

typedef struct __attribute__ ((packed))
{
  UINT8  config0;
  UINT8  config1;
  UINT8  config2;
  UINT8  config3;
  UINT16 v_size;
  UINT16 h_size;
  UINT16 y;
  UINT16 x;
  UINT16 link_data;
  UINT16 link_next;
} osd_header_t;

typedef struct __attribute__ ((packed))
{
	UINT8	window;
	UINT8	num_buf;
	UINT8	visible;
	UINT8	active;
	UINT16  buf_addr[6];
} osd_info_t;


typedef struct osd_manager_s {
	osd_header_t	*pOSDHdr;
	osd_info_t		*pOSDInfo;
	int				size;
} osd_manager_t;

void DRV_OSD_Init(void);
int API_OSD_UI_Init(int w, int h, u32 fb_addr, int input_fmt);

#endif	//__DISP_OSD_H__

