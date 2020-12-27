// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "display2.h"
#include "disp_osd.h"
#include "disp_palette.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// OSD Header config0
#define OSD_HDR_C0_CULT				0x80
#define OSD_HDR_C0_TRANS			0x40

// OSD Header config1
#define OSD_HDR_C1_BS				0x10
#define OSD_HDR_C1_BL2				0x04
#define OSD_HDR_C1_BL				0x01

// OSD control register
#define OSD_CTRL_COLOR_MODE_YUV		(0 << 10)
#define OSD_CTRL_COLOR_MODE_RGB		(1 << 10)
#define OSD_CTRL_NOT_FETCH_EN		(1 << 8)
#define OSD_CTRL_CLUT_FMT_GBRA		(0 << 7)
#define OSD_CTRL_CLUT_FMT_ARGB		(1 << 7)
#define OSD_CTRL_LATCH_EN			(1 << 5)
#define OSD_CTRL_A32B32_EN			(1 << 4)
#define OSD_CTRL_FIFO_DEPTH			(7 << 0)

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_OSD_Init(void)
{
	;
}

int API_OSD_UI_Init(int w, int h, u32 fb_addr, int input_fmt)
{
	UINT32 UI_width = w;
	UINT32 UI_height = h;

	if(input_fmt == DRV_OSD_REGION_FORMAT_8BPP)
		osd0_header[0] = SWAP32(0x82001000);
	else
		osd0_header[0] = SWAP32(0x00001000 | (input_fmt << 24));

	osd0_header[1] = SWAP32((UI_height << 16) | UI_width);
	osd0_header[2] = 0;
	osd0_header[3] = 0;
	osd0_header[4] = 0;
	if (input_fmt == DRV_OSD_REGION_FORMAT_YUY2)
		osd0_header[5] = SWAP32(0x00040000 | UI_width);
	else
		osd0_header[5] = SWAP32(0x00010000 | UI_width);
	osd0_header[6] = SWAP32(0xFFFFFFE0);
	osd0_header[7] = SWAP32(fb_addr);

	G196_OSD0_REG->osd_en = 0;

	flush_cache((u64)osd0_header, 128+1024); //Update osd0_header

	debug("*** [S] dump osd0_header info *** \n");
	debug("0x%08x 0x%08x 0x%08x 0x%08x \n", SWAP32(osd0_header[0]),SWAP32(osd0_header[1]),SWAP32(osd0_header[2]),SWAP32(osd0_header[3]));
	debug("0x%08x 0x%08x 0x%08x 0x%08x \n", SWAP32(osd0_header[4]),SWAP32(osd0_header[5]),SWAP32(osd0_header[6]),SWAP32(osd0_header[7]));
	debug("0x%08x 0x%08x 0x%08x 0x%08x \n", SWAP32(osd0_header[56]),SWAP32(osd0_header[57]),SWAP32(osd0_header[58]),SWAP32(osd0_header[59]));
	debug("0x%08x 0x%08x 0x%08x 0x%08x \n", SWAP32(osd0_header[60]),SWAP32(osd0_header[61]),SWAP32(osd0_header[62]),SWAP32(osd0_header[63]));
	debug("*** [E] dump osd0_header info *** \n");

	G196_OSD0_REG->osd_base_addr = (u32)((u64)&osd0_header);

	G196_OSD0_REG->osd_ctrl = OSD_CTRL_COLOR_MODE_RGB
		| OSD_CTRL_CLUT_FMT_ARGB
		| OSD_CTRL_LATCH_EN
		| OSD_CTRL_A32B32_EN
		| OSD_CTRL_FIFO_DEPTH;

	G196_OSD0_REG->osd_hvld_offset = 0;
	G196_OSD0_REG->osd_vvld_offset = 0;

	G196_OSD0_REG->osd_hvld_width = UI_width;
	G196_OSD0_REG->osd_vvld_height = UI_height;

	//G196_OSD0_REG->osd_data_fetch_ctrl = 0x0af8;
	G196_OSD0_REG->osd_bist_ctrl = 0x0;
	G196_OSD0_REG->osd_3d_h_offset = 0x0;
	G196_OSD0_REG->osd_src_decimation_sel = 0x0;
	
	G196_OSD0_REG->osd_en = 1;

	//GPOST bypass
	G206_GPOST_REG->gpost0_config = 0;
	G206_GPOST_REG->gpost0_master_en = 0;
	G206_GPOST_REG->gpost0_bg1 = 0x8010;
	G206_GPOST_REG->gpost0_bg2 = 0x0080;

	//GPOST PQ disable
	G206_GPOST_REG->gpost0_contrast_config = 0x0;

	return 0;
}
