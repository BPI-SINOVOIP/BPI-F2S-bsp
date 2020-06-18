// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "disp_dve.h"
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DVE_Init(int is_hdmi, int width, int height)
{
	int dve_bist_mode = 0;
	int dve_hdmi_mode_0 = 0;
	int dve_hdmi_mode_1 = 0;

	if (!is_hdmi) {
		if(((width == 1024) && (height == 600) ) ||  ( (width == 800) && (height == 480) )){
			//for TTL_MODE_800_480 and TTL_MODE_1024_600
			dve_bist_mode =  (DVE_TTL_BIT_SW_ON) \
											|(DVE_TTL_MODE) \
											|(DVE_COLOR_SPACE_BT601) \
											|(DVE_TTL_CLK_POL_NOR) \
											|(DVE_COLOR_BAR_USER_MODE_SEL) \
											|(DVE_NORMAL_MODE);
			G235_DVE_REG->g235_reserved[0] = 0x4A30; //G235.6
			G235_DVE_REG->g235_reserved[1] = 0x5693; //G235.7
			G235_DVE_REG->g235_reserved[2] = 0x22F6; //G235.8
			G235_DVE_REG->g235_reserved[3] = 0x2D49; //G235.9
			G235_DVE_REG->g235_reserved[4] = 0x39AC; //G235.10
			G235_DVE_REG->g235_reserved[5] = 0x040F; //G235.11
			G235_DVE_REG->g235_reserved[6] = 0x1062; //G235.12
			G235_DVE_REG->g235_reserved[7] = 0x1CC5; //G235.13
		}
		else if ( (width == 320) && (height == 240) ) {
			//for TTL_MODE_320_240
			dve_bist_mode =  (DVE_TTL_BIT_SW_OFF) \
											|(DVE_TTL_MODE) \
											|(DVE_COLOR_SPACE_BT601) \
											|(DVE_TTL_CLK_POL_INV) \
											|(DVE_COLOR_BAR_USER_MODE_SEL) \
											|(DVE_NORMAL_MODE);
		}
	
		dve_hdmi_mode_0 = (DVE_LATCH_DIS) | (DVE_444_MODE);
		dve_hdmi_mode_1 = (DVE_HDMI_ENA) | (DVE_HD_MODE);
		
		G235_DVE_REG->color_bar_mode = dve_bist_mode; 		//COLOR_BAR_MODE_DIS
		G234_DVE_REG->dve_hdmi_mode_1 = dve_hdmi_mode_1; // DVE_HDMI_ENA , DVE_SD_MODE
		G234_DVE_REG->dve_hdmi_mode_0 = dve_hdmi_mode_0;// DVE_LATCH_ENA , DVE_444_MODE

		if( (width == 1024) && (height == 600) ) {
			debug("dve init 1024_600 \n");
			G234_DVE_REG->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
			G234_DVE_REG->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
			G234_DVE_REG->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(1183);
			G234_DVE_REG->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
			G234_DVE_REG->dve_hsync_start = USER_MODE_HSYNC_START(1183);
			G234_DVE_REG->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(160);
			
			G234_DVE_REG->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(22);
			G234_DVE_REG->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(622);
			G234_DVE_REG->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(22);
			G234_DVE_REG->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(622);
			
			G234_DVE_REG->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(1343);
			G234_DVE_REG->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(1023);
			G234_DVE_REG->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(1343);
			G234_DVE_REG->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(634);
			G234_DVE_REG->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(1023);
			
			G235_DVE_REG->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(634);
			G235_DVE_REG->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(1023);
			G235_DVE_REG->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(23);
			G235_DVE_REG->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(1343);
			G235_DVE_REG->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(1023);
		}
		else if( (width == 800) && (height == 480) ) {
			debug("dve init 800_480 \n");
			G234_DVE_REG->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
			G234_DVE_REG->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
			G234_DVE_REG->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(989);
			G234_DVE_REG->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
			G234_DVE_REG->dve_hsync_start = USER_MODE_HSYNC_START(989);
			G234_DVE_REG->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(66);
			
			G234_DVE_REG->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(25);
			G234_DVE_REG->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(505);
			G234_DVE_REG->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(25);
			G234_DVE_REG->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(505);
			
			G234_DVE_REG->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(1055);
			G234_DVE_REG->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(799);
			G234_DVE_REG->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(1055);
			G234_DVE_REG->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(524);
			G234_DVE_REG->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(799);
			
			G235_DVE_REG->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(524);
			G235_DVE_REG->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(479);
			G235_DVE_REG->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(26);
			G235_DVE_REG->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(1055);
			G235_DVE_REG->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(799);
		}
		else if( (width == 320) && (height == 240) ) {
			debug("dve init 320_240 \n");
			G234_DVE_REG->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
			G234_DVE_REG->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
			G234_DVE_REG->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(339);
			G234_DVE_REG->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
			G234_DVE_REG->dve_hsync_start = USER_MODE_HSYNC_START(339);
			G234_DVE_REG->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(68);
			
			G234_DVE_REG->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(17);
			G234_DVE_REG->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(257);
			G234_DVE_REG->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(17);
			G234_DVE_REG->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(257);
			
			G234_DVE_REG->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(407);
			G234_DVE_REG->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(319);
			G234_DVE_REG->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(407);
			G234_DVE_REG->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(261);
			G234_DVE_REG->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(319);
			
			G235_DVE_REG->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(261);
			G235_DVE_REG->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(239);
			G235_DVE_REG->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(18);
			G235_DVE_REG->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(407);
			G235_DVE_REG->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(319);
		}

	}
	else {
		G235_DVE_REG->color_bar_mode = 0;
		G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
		G234_DVE_REG->dve_hdmi_mode_0 = 0x41;// latch mode on
	}

}

void DRV_DVE_SetMode(int mode)
{
	int colorbarmode = G235_DVE_REG->color_bar_mode & ~0xfe;
	int hdmi_mode = G234_DVE_REG->dve_hdmi_mode_0 & ~0x1f80;

	debug("dve mode: %d\n", mode);

	switch (mode)
	{
		default:
		case 0:	//480P
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (0 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (0 << 3);
			break;
		case 1:	//576P
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (1 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (1 << 3);
			break;
		case 2:	//720P60
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (6 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (2 << 3);
			break;
		case 3:	//720P50
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (7 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (3 << 3);
			break;
		case 4:	//1080P50
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (3 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (4 << 3);
			break;
		case 5:	//1080P60
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (5 << 3);
			break;
		case 6:	//1080P24
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (1 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			G235_DVE_REG->color_bar_mode = colorbarmode | (6 << 3);
			break;
		case 7:	//user mode
			G235_DVE_REG->color_bar_mode = colorbarmode | (0x12 << 3);
			break;
	}
}

void DRV_DVE_SetColorbar(int enable)
{
	int bist_mode;
	bist_mode = G235_DVE_REG->color_bar_mode & ~0x01;
	
	if(enable)
		G235_DVE_REG->color_bar_mode |= (1 << 0);
	else
		G235_DVE_REG->color_bar_mode = bist_mode;

}

