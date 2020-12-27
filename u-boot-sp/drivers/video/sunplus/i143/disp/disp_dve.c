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

	G235_DVE_REG->color_bar_mode = 0;
	G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
	G234_DVE_REG->dve_hdmi_mode_0 = 0x141;// latch mode on

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
			G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
			G235_DVE_REG->color_bar_mode = colorbarmode | (0 << 3);
			break;
		case 1:	//576P
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (1 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
			G235_DVE_REG->color_bar_mode = colorbarmode | (1 << 3);
			break;
		case 2:	//720P60
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (6 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
			G235_DVE_REG->color_bar_mode = colorbarmode | (2 << 3);
			break;
		case 3:	//720P50
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (7 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x3;
			G235_DVE_REG->color_bar_mode = colorbarmode | (3 << 3);
			break;
		case 4:	//1080P60
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x2;
			G235_DVE_REG->color_bar_mode = colorbarmode | (5 << 3);
			break;
		case 5:	//1080P50
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (3 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x2;
			G235_DVE_REG->color_bar_mode = colorbarmode | (4 << 3);
			break;
		case 6:	//1080P24
			G234_DVE_REG->dve_hdmi_mode_0 = hdmi_mode | (1 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			G234_DVE_REG->dve_hdmi_mode_1 = 0x2;
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

