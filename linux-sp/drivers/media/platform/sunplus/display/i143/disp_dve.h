// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver for dve header file
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

#ifndef __DISP_DVE_H__
#define __DISP_DVE_H__

#include <media/sunplus/disp/i143/display.h>

void DRV_DVE_Init(void *pInHWReg);
void DRV_DVE_SetMode(int mode);
void DRV_DVE_SetColorbar(int enable);

/* DVE G234 REG Define */
//G234.00 (User mode)
//G234.01 (User mode)
//G234.02 (User mode)
//G234.03 (User mode)
//G234.04 (User mode)
//G234.05 (User mode)
//G234.06
#define DVE_HDMI_ENA	(ENABLE<<1)
#define DVE_HDMI_DIS	(DISABLE<<1)
#define DVE_SD_MODE	(ENABLE<<0)
#define DVE_HD_MODE	(DISABLE<<0)
//G234.07 (User mode)
//G234.08 (User mode)
//G234.09 (User mode)
//G234.10 (User mode)
//G234.11 (User mode)
//G234.12 (User mode)
//G234.13 (User mode)
//G234.14 (User mode)
//G234.15 (User mode)
//G234.16
#define DVE_LATCH_ENA	(ENABLE<<6)
#define DVE_LATCH_DIS	(DISABLE<<6)
#define DVE_444_MODE	(ENABLE<<0)
#define DVE_422_MODE	(DISABLE<<0)
//G234.17 (Reserved)
//G234.18 (Reserved)
//G234.19 (Reserved)
//G234.20 (Reserved)
//G234.21 (Reserved)
//G234.22 (Reserved)
//G234.23 (Reserved)
//G234.24 (Reserved)
//G234.25 (Reserved)
//G234.26 (Reserved)
//G234.27 (Reserved)
//G234.28 (Reserved)
//G234.29 (Reserved)
//G234.30 (Reserved)
//G234.31 (Reserved)

/* DVE G235 REG Define */
//G235.00
#define DVE_TTL_BIT_SW_ON	(ENABLE<<14)
#define DVE_TTL_BIT_SW_OFF	(DISABLE<<14)
#define DVE_TTL_MODE	(ENABLE<<13)
#define DVE_HDMI_MODE	(DISABLE<<13)
#define DVE_COLOR_SPACE_BT709	(ENABLE<<12)
#define DVE_COLOR_SPACE_BT601	(DISABLE<<12)
#define DVE_TTL_HOR_POL_INV	(ENABLE<<11)
#define DVE_TTL_HOR_POL_NOR	(DISABLE<<11)
#define DVE_TTL_VER_POL_INV	(ENABLE<<10)
#define DVE_TTL_VER_POL_NOR	(DISABLE<<10)
#define DVE_TTL_CLK_POL_INV	(ENABLE<<9)
#define DVE_TTL_CLK_POL_NOR	(DISABLE<<9)
#define DVE_COLOR_BAR_USER_MODE_SEL	(0x12<<3)
#define DVE_COLOR_BAR_1080p24_SEL	(0x06<<3)
#define DVE_COLOR_BAR_1080p50_SEL	(0x05<<3)
#define DVE_COLOR_BAR_1080p60_SEL	(0x04<<3)
#define DVE_COLOR_BAR_720p50_SEL	(0x03<<3)
#define DVE_COLOR_BAR_720p60_SEL	(0x02<<3)
#define DVE_COLOR_BAR_576p_SEL	(0x01<<3)
#define DVE_COLOR_BAR_480p_SEL	(DISABLE<<3)
#define DVE_BIST_MODE	(ENABLE<<0)
#define DVE_NORMAL_MODE	(DISABLE<<0)
//G235.01 (User mode)
//G235.02 (User mode)
//G235.03 (User mode)
//G235.04 (User mode)
//G235.05 (User mode)
//G235.06 (Reserved)
//G235.07 (Reserved)
//G235.08 (Reserved)
//G235.09 (Reserved)
//G235.10 (Reserved)
//G235.11 (Reserved)
//G235.12 (Reserved)
//G235.13 (Reserved)
//G235.14 (Reserved)
//G235.15 (Reserved)
//G235.16 (Reserved)
//G235.17 (Reserved)
//G235.18 (Reserved)
//G235.19 (Reserved)
//G235.20 (Reserved)
//G235.21 (Reserved)
//G235.22 (Reserved)
//G235.23 (Reserved)
//G235.24 (Reserved)
//G235.25 (Reserved)
//G235.26 (Reserved)
//G235.27 (Reserved)
//G235.28 (Reserved)
//G235.29 (Reserved)
//G235.30 (Reserved)
//G235.31 (Reserved)

#endif	//__DISP_DVE_H__
