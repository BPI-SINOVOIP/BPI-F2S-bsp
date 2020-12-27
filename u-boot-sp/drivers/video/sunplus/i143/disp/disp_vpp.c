// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "display2.h"

static const char * const VppdmaFmt[] = {"RGB888", "RGB565", "UYVY", "NV16", "YUY2"};
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_VPP_Init(void)
{
	;
}

#if 0
void vpost_setting(int is_hdmi, int x, int y, int input_w, int input_h, int output_w, int output_h)
{
	debug("vpost_setting for HDMI \n");

	G199_VPOST_REG->vpost_mas_sla = 0x0; //dis user mode
	G199_VPOST_REG->vpost_o_act_xstart = 0; //x active
	G199_VPOST_REG->vpost_o_act_ystart = 0; //y active		

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
#endif

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

void vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h)
{

	debug("vscl position (x, y) =(%d, %d)\n", x, y);
	debug("IN (%d, %d), OUT (%d, %d)\n", input_w, input_h, output_w, output_h);

	if ((input_w == output_w) && (input_h == output_h))
		debug("scaling ratio (x, y) = (1000,1000) \n");
	else {
		debug("scaling ratio (x, y) = (%04d,%04d) \n", output_w * 1000 / input_w, output_h * 1000 / input_h);
	}

	G187_VSCL_REG->vscl0_actrl_i_xlen 	= input_w;	//G187.03
	G187_VSCL_REG->vscl0_actrl_i_ylen 	= input_h;	//G187.04
	G187_VSCL_REG->vscl0_actrl_s_xstart	= x;	//G187.05
	G187_VSCL_REG->vscl0_actrl_s_ystart	= y;	//G187.06
	G187_VSCL_REG->vscl0_actrl_s_xlen	= input_w;	//G187.07
	G187_VSCL_REG->vscl0_actrl_s_ylen	= input_h;	//G187.08
	G187_VSCL_REG->vscl0_dctrl_o_xlen	= output_w;	//G187.09
	G187_VSCL_REG->vscl0_dctrl_o_ylen	= output_h;	//G187.10	
	G187_VSCL_REG->vscl0_dctrl_d_xstart	= x;	//G187.11
	G187_VSCL_REG->vscl0_dctrl_d_ystart	= y;	//G187.12
	G187_VSCL_REG->vscl0_dctrl_d_xlen	= output_w;	//G187.13
	G187_VSCL_REG->vscl0_dctrl_d_ylen	= output_h;	//G187.14

	G187_VSCL_REG->vscl0_hint_ctrl			= 0x00000002;	//G187.18
	G187_VSCL_REG->vscl0_hint_hfactor_low	= 0x00000000;	//G187.19
	//G187_VSCL_REG->vscl0_hint_hfactor_high	= 0x00000040;	//G187.20
	G187_VSCL_REG->vscl0_hint_hfactor_high	= (input_w*64)/output_w;	//G187.20
	G187_VSCL_REG->vscl0_hint_initf_low		= 0x00000000;	//G187.21
	G187_VSCL_REG->vscl0_hint_initf_high		= 0x00000000;	//G187.22	
	
	G188_VSCL_REG->vscl0_vint_ctrl			= 0x00000002;	//G188.00
	G188_VSCL_REG->vscl0_vint_vfactor_low	= 0x00000000;	//G188.01
	//G188_VSCL_REG->vscl0_vint_vfactor_high	= 0x00000040;	//G188.02
	G188_VSCL_REG->vscl0_vint_vfactor_high	= (input_h*64)/output_h;	//G188.02
	G188_VSCL_REG->vscl0_vint_initf_low		= 0x00000000;	//G188.03
	G188_VSCL_REG->vscl0_vint_initf_high		= 0x00000000;	//G188.04

	debug("scaling reg (x, y) = (0x%08x,0x%08x) \n", G187_VSCL_REG->vscl0_hint_hfactor_high, G188_VSCL_REG->vscl0_vint_vfactor_high);

}

void vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt)
{
	int vdma_cfg_tmp, vdma_frame_size_tmp;

	debug("vppdma luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);
	debug("vppdma w=%d, h=%d, fmt= %s\n", w, h, VppdmaFmt[vppdma_fmt]);

	G186_VPPDMA_REG->vdma_gc 		= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28

	vdma_cfg_tmp = G186_VPPDMA_REG->vdma_cfg;

	vdma_cfg_tmp &= ~((1 << 12) | (1 << 11) | (7 << 8) | (1 << 7) | (1 << 6) | (3 << 0));

	if (vppdma_fmt == 0)
		vdma_cfg_tmp 			= (0 << 12) | (0 << 11) | (0 << 8) | (0 << 7) | (0 << 6) | (0 << 0); //source RGB888 (default)
	else if(vppdma_fmt == 1)
		vdma_cfg_tmp 			= (0 << 12) | (0 << 11) | (0 << 8) | (0 << 7) | (0 << 6) | (1 << 0); //source RGB565
	else if(vppdma_fmt == 2)
		vdma_cfg_tmp 			= (0 << 12) | (0 << 11) | (0 << 8) | (0 << 7) | (0 << 6) | (2 << 0); //source YUV422 UYVY
	else if(vppdma_fmt == 3)
		vdma_cfg_tmp 			= (0 << 12) | (0 << 11) | (0 << 8) | (0 << 7) | (0 << 6) | (3 << 0); //source YUV422 NV16
	else if(vppdma_fmt == 4)
		vdma_cfg_tmp 			= (1 << 12) | (0 << 11) | (0 << 8) | (0 << 7) | (0 << 6) | (2 << 0); //source YUV422 YUY2		

	G186_VPPDMA_REG->vdma_cfg = vdma_cfg_tmp;

	vdma_frame_size_tmp = G186_VPPDMA_REG->vdma_frame_size;
	vdma_frame_size_tmp &= ~( (0xfff << 16) | (0x1fff << 0) );
	vdma_frame_size_tmp = ((h << 16) | w);
	G186_VPPDMA_REG->vdma_frame_size = vdma_frame_size_tmp;

	//G186_VPPDMA_REG->vdma_frame_size 			= ((h<<16) | w);; //G186.03 , frame_h , frame_w
	G186_VPPDMA_REG->vdma_crop_st 				= 0x00000000; //G186.04 , start_h , Start_w
	
	if (vppdma_fmt == 0)
		G186_VPPDMA_REG->vdma_lstd_size 		= w*3; //G186.05 , stride size , source RGB888 (default)
	else if (vppdma_fmt == 1)
		G186_VPPDMA_REG->vdma_lstd_size 		= w*2; //G186.05 , stride size , source RGB565
	else if (vppdma_fmt == 2)
		G186_VPPDMA_REG->vdma_lstd_size 		= w*2; //G186.05 , stride size , source YUV422 UYVY
	else if (vppdma_fmt == 3)
		G186_VPPDMA_REG->vdma_lstd_size 		= w; //G186.05 , stride size , source YUV422 NV16
	else if (vppdma_fmt == 4)
		G186_VPPDMA_REG->vdma_lstd_size 		= w*2; //G186.05 , stride size , source YUV422 YUY2

	if (luma_addr != 0)
		G186_VPPDMA_REG->vdma_data_addr1 		= luma_addr; //G186.06 , 1st planner addr (luma)
	if (chroma_addr != 0)
		G186_VPPDMA_REG->vdma_data_addr2 		= chroma_addr; //G186.07 , 2nd planner addr (crma)

}