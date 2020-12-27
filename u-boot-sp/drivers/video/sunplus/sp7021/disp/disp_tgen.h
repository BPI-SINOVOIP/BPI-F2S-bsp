// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#ifndef __DISP_TGEN_H__
#define __DISP_TGEN_H__

#include "display2.h"

typedef enum {
	DRV_TGEN_VPP0 = 0,
	DRV_TGEN_OSD0,
	DRV_TGEN_PTG,
	DRV_TGEN_ALL
} DRV_TGEN_Input_e;

typedef enum {
	DRV_FMT_480P = 0,
	DRV_FMT_576P,
	DRV_FMT_720P,
	DRV_FMT_1080P,
	DRV_FMT_USER_MODE,
	DRV_FMT_MAX,
} DRV_VideoFormat_e;

typedef enum {
	DRV_FrameRate_5994Hz = 0,
	DRV_FrameRate_50Hz,
	DRV_FrameRate_24Hz,
	DRV_FrameRate_MAX
} DRV_FrameRate_e;
#if 0
typedef struct DRV_SetTGEN_s {
	DRV_VideoFormat_e fmt;
	DRV_FrameRate_e fps;
	u16 htt;
	u16 vtt;
	u16 hactive;
	u16 vactive;
	u16 v_bp;
} DRV_SetTGEN_t;
#endif

void DRV_TGEN_Init(int is_hdmi, int width, int height);
void DRV_TGEN_Set(DRV_VideoFormat_e fmt, DRV_FrameRate_e fps);
int DRV_TGEN_Adjust(DRV_TGEN_Input_e Input, UINT32 Adjust);

#endif	//__DISP_TGEN_H__

