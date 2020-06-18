// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "display2.h"
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_VPP_Init(void)
{
	;
}

void vpost_setting(int is_hdmi, int x, int y, int input_w, int input_h, int output_w, int output_h)
{
	if (!is_hdmi)
		debug("vpost_setting for LCD \n");
	else
		debug("vpost_setting for HDMI \n");

	if (!is_hdmi) {
		G199_VPOST_REG->vpost_mas_sla = 0x1; //en user mode

		if( (output_w == 1024) && (output_h == 600) ) {
			G199_VPOST_REG->vpost_o_act_xstart = 0; //x active
			G199_VPOST_REG->vpost_o_act_ystart = 26; //y active
		}
		else if( (output_w == 800) && (output_h == 480) ) {
			G199_VPOST_REG->vpost_o_act_xstart = 0; //x active
			G199_VPOST_REG->vpost_o_act_ystart = 29; //y active
		}
		else if( (output_w == 320) && (output_h == 240) ) {
			G199_VPOST_REG->vpost_o_act_xstart = 0; //x active
			G199_VPOST_REG->vpost_o_act_ystart = 21; //y active
		}
	}
	else {
		G199_VPOST_REG->vpost_mas_sla = 0x0; //dis user mode
		G199_VPOST_REG->vpost_o_act_xstart = 0; //x active
		G199_VPOST_REG->vpost_o_act_ystart = 0; //y active		
	}

	if ((input_w != output_w) || (input_h != output_h)) {
		G199_VPOST_REG->vpost_config1 = 0x11;
		G199_VPOST_REG->vpost_i_xstart = x;
		G199_VPOST_REG->vpost_i_ystart = y;
	}
	else {
		G199_VPOST_REG->vpost_config1 = 0x1;
		G199_VPOST_REG->vpost_i_xstart = 0;
		G199_VPOST_REG->vpost_i_ystart = 0;
	}
	G199_VPOST_REG->vpost_i_xlen = input_w;
	G199_VPOST_REG->vpost_i_ylen = input_h;
	G199_VPOST_REG->vpost_o_xlen = output_w;
	G199_VPOST_REG->vpost_o_ylen = output_h;
	G199_VPOST_REG->vpost_config2 = 4;

}

void ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int yuv_fmt)
{
	debug("ddfch luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);

	debug("ddfch setting w=%d h=%d \n",w,h);

	G185_DDFCH_REG->ddfch_latch_en = 1;
	if (yuv_fmt == 0)
		G185_DDFCH_REG->ddfch_mode_option = 0; //source yuv420 NV12
	else if(yuv_fmt == 1)
		G185_DDFCH_REG->ddfch_mode_option = 0x400; //source yuv422 NV16
	else if(yuv_fmt == 2)
		G185_DDFCH_REG->ddfch_mode_option = 0x800; //source yuv422 YUY2

	debug("disable ddfch fetch \n");
	G185_DDFCH_REG->ddfch_enable = 0xc0; //Disable DDFCH

	G185_DDFCH_REG->ddfch_luma_base_addr_0 = luma_addr>>10;
	G185_DDFCH_REG->ddfch_crma_base_addr_0 = chroma_addr>>10;
	G185_DDFCH_REG->ddfch_vdo_frame_size = EXTENDED_ALIGNED(w, 128); //video line pitch
	//G185_DDFCH_REG->ddfch_vdo_crop_size = ((h<<16) | w); //y size & x size
	G185_DDFCH_REG->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	G185_DDFCH_REG->ddfch_config_0 = 0x10000;
	G185_DDFCH_REG->ddfch_bist = 0x80801002;
	G185_DDFCH_REG->ddfch_vdo_crop_size = ((h<<16) | w); //y size & x size

}
