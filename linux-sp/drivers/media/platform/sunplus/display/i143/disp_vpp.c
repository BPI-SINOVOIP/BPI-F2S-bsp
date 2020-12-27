// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver for vpp layer
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include <linux/module.h>

#include <media/sunplus/disp/i143/display.h>
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_vpp.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static volatile DISP_VPOST_REG_t *pVPOSTReg;
static volatile DISP_VSCL_REG_t *pVSCLReg;
static volatile DISP_DDFCH_REG_t *pDDFCHReg;
static volatile DISP_VPPDMA_REG_t *pVPPDMAReg;


static const char * const DdfchFmt[] = {"NV12", "NV16", "YUY2"};
static const char * const VppdmaFmt[] = {"RGB888", "RGB565", "UYVY", "NV16", "YUY2"};
static const char * const path_sel[] = {"VPPDMA", "OSD0", "DDFCH", "DDFCH"};
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_VPP_Init(void *pInHWReg1, void *pInHWReg2, void *pInHWReg3, void *pInHWReg4)
{
	int width, height;
	pVPOSTReg = (DISP_VPOST_REG_t *)pInHWReg1;
	pVSCLReg = (DISP_VSCL_REG_t *)pInHWReg2;
	pDDFCHReg = (DISP_DDFCH_REG_t *)pInHWReg3;
	pVPPDMAReg = (DISP_VPPDMA_REG_t *)pInHWReg4;

#ifdef DISP_480P
	width = 720;
	height = 480;
#endif
#ifdef DISP_576P
	width = 720;
	height = 576;
#endif
#ifdef DISP_720P
	width = 1280;
	height = 720;
#endif
#ifdef DISP_1080P
	width = 1920;
	height = 1080;
#endif

#if (VPP_PATH_SEL == 0)
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
#elif (VPP_PATH_SEL == 1)
	pVSCLReg->vscl0_config2 			= 0x0000121F;	//G187.01
#elif (VPP_PATH_SEL == 2)
	pVSCLReg->vscl0_config2 			= 0x0000221F;	//G187.01
#endif
	pVSCLReg->vscl0_actrl_i_xlen 		= width;//0x000002D0;	//G187.03
	pVSCLReg->vscl0_actrl_i_ylen 		= height;	//G187.04
	pVSCLReg->vscl0_actrl_s_xstart		= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart		= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen		= width;//0x000002D0;	//G187.07
	pVSCLReg->vscl0_actrl_s_ylen		= height;	//G187.08
	pVSCLReg->vscl0_dctrl_o_xlen		= width;//0x000002D0;	//G187.09
	pVSCLReg->vscl0_dctrl_o_ylen		= height;	//G187.10	
	pVSCLReg->vscl0_dctrl_d_xstart		= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart		= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen		= width;//0x000002D0;	//G187.13
	pVSCLReg->vscl0_dctrl_d_ylen		= height;	//G187.14
	pVSCLReg->vscl0_hint_ctrl			= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	= 0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	= 0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		= 0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		= 0x00000000;	//G187.22
	pVSCLReg->vscl0_vint_ctrl			= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	= 0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	= 0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		= 0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		= 0x00000000;	//G188.04
	
	#if (VPP_PATH_SEL == 0)
		pVPPDMAReg->vdma_gc 				= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
		#if (VPPDMA_FMT_HDMI == 0) //RGB888
			pVPPDMAReg->vdma_cfg 				= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888
			pVPPDMAReg->vdma_lstd_size 			= width*3; //G186.05 , stride size RGB888
		#elif (VPPDMA_FMT_HDMI == 1) //RGB565
			pVPPDMAReg->vdma_cfg 				= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565
			pVPPDMAReg->vdma_lstd_size 			= (width<<1); //G186.05 , stride size RGB565
		#elif (VPPDMA_FMT_HDMI == 2) //YUV422_UYVY
			pVPPDMAReg->vdma_cfg 				= 0x00000002; //G186.02 , no swap , bist off , fmt = UYVY
			pVPPDMAReg->vdma_lstd_size 			= (width<<1); //G186.05 , stride size YUY2
		#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
			pVPPDMAReg->vdma_cfg 				= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
			pVPPDMAReg->vdma_lstd_size 			= (width); //G186.05 , stride size YUV422_NV16
		#elif (VPPDMA_FMT_HDMI == 4) //YUV422_YUY2
			pVPPDMAReg->vdma_cfg 				= 0x00000802; //G186.02 , with swap , bist off , fmt = YUY2
			pVPPDMAReg->vdma_lstd_size 			= (width<<1); //G186.05 , stride size YUY2
		#else //RGB888
			pVPPDMAReg->vdma_cfg 				= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
			pVPPDMAReg->vdma_lstd_size 			= width*3; //G186.05 , stride size RGB888	
		#endif
		pVPPDMAReg->vdma_frame_size 		= ((height << 16) | width);//0x01E002D0; //G186.03 , frame_h , frame_w
		pVPPDMAReg->vdma_crop_st 			= 0x00000000; //G186.04 , start_h , Start_w
	#endif

	#if (VPP_PATH_SEL == 2)
	//pDDFCHReg->ddfch_latch_en 				= 1; //G185.00 //latch en
	//pDDFCHReg->ddfch_mode_option			= 0x00000000; //G185.01 //YUV422
	//pDDFCHReg->ddfch_enable 					= 0xd0; //G185.02 //fetch en

	//pDDFCHReg->ddfch_luma_base_addr_0 = (0x120000)>>10;
	//pDDFCHReg->ddfch_crma_base_addr_0 = (0x120000 + 768*480)>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(width, 128); //video line pitch
	//pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	//pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((height << 16) | width); //y size & x size
	#endif
}

void DRV_DDFCH_off(void)
{
	sp_disp_info("DRV_DDFCH_off \n");
	pDDFCHReg->ddfch_enable 				= 0xc0; //G185.02 //fetch dis
}

void DRV_DDFCH_on(void)
{
	sp_disp_info("DRV_DDFCH_on \n");
	pDDFCHReg->ddfch_enable 				= 0xd0; //G185.02 //fetch en
}
void DRV_VPPDMA_off(void)
{
	sp_disp_info("DRV_VPPDMA_off \n");
	pVPPDMAReg->vdma_gc 							= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	
}
void DRV_VPPDMA_on(void)
{
	sp_disp_info("DRV_VPPDMA_on \n");
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
}

void vpp_path_select(DRV_VppWindow_e vpp_path_sel)
{
	//sp_disp_info("vpp_path_select %d \n", vpp_path_sel);
	switch (vpp_path_sel) {
		case DRV_FROM_VPPDMA:
				sp_disp_info("vpp_path_select DRV_FROM_VPPDMA \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				pVSCLReg->vscl0_config2 			= 0x021F;	//G187.01
		break;
		case DRV_FROM_OSD0:
				sp_disp_info("vpp_path_select DRV_FROM_OSD0 \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				//pVSCLReg->vscl0_config2 			|= 0x1000;	//G187.01
				pVSCLReg->vscl0_config2 			= 0x121F;	//G187.01
		break;	
		case DRV_FROM_DDFCH:
				sp_disp_info("vpp_path_select DRV_FROM_DDFCH \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				//pVSCLReg->vscl0_config2 			|= 0x2000;	//G187.01
				pVSCLReg->vscl0_config2 			= 0x221F;	//G187.01
		break;	
		default:
		break;
	}
}

bool flag_vscl_path_vppdma = 0;
bool flag_vscl_path_osd0 = 0; 
bool flag_vscl_path_ddfch = 0;

void vpp_path_select_flag(DRV_VppWindow_e vpp_path_sel)
{
	//sp_disp_info("vpp_path_select %d \n", vpp_path_sel);
	switch (vpp_path_sel) {
		case DRV_FROM_VPPDMA:
				sp_disp_info("vpp_path_select flag DRV_FROM_VPPDMA \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				//pVSCLReg->vscl0_config2 			= 0x021F;	//G187.01
				flag_vscl_path_vppdma = 1;
		break;
		case DRV_FROM_OSD0:
				sp_disp_info("vpp_path_select flag DRV_FROM_OSD0 \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				//pVSCLReg->vscl0_config2 			|= 0x1000;	//G187.01
				//pVSCLReg->vscl0_config2 			= 0x121F;	//G187.01
				flag_vscl_path_osd0 = 1;
		break;	
		case DRV_FROM_DDFCH:
				sp_disp_info("vpp_path_select flag DRV_FROM_DDFCH \n");
				//pVSCLReg->vscl0_config2 			&= (~0x3000);	//G187.01
				//pVSCLReg->vscl0_config2 			|= 0x2000;	//G187.01
				//pVSCLReg->vscl0_config2 			= 0x221F;	//G187.01
				flag_vscl_path_ddfch = 1;
		break;	
		default:
		break;
	}
}

int ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int ddfch_fmt)
{
	sp_disp_dbg("ddfch luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);
	sp_disp_dbg("ddfch w=%d, h=%d, fmt= %s\n", w, h, DdfchFmt[ddfch_fmt]);

	#if (DDFCH_GRP_EN == 1)
	pDDFCHReg->ddfch_latch_en = 1;
	#else
	//pDDFCHReg->ddfch_latch_en = 0;
	#endif
	
	if (ddfch_fmt == 0){
		pDDFCHReg->ddfch_mode_option = 0; //source yuv420 NV12
		//sp_disp_dbg("ddfch setting fmt YUV420_NV12 \n");
	}
	else if(ddfch_fmt == 1) {
		pDDFCHReg->ddfch_mode_option = 0x400; //source yuv422 NV16
		//sp_disp_dbg("ddfch setting fmt YUV422_NV16 \n");
	}
	else if(ddfch_fmt == 2) {
		pDDFCHReg->ddfch_mode_option = 0x800; //source yuv422 YUY2
		//sp_disp_dbg("ddfch setting fmt YUV422_YUY2 \n");
	}

	#if (DDFCH_GRP_EN == 1)
	pDDFCHReg->ddfch_enable = 0xd0;
	#else
	//pDDFCHReg->ddfch_enable = 0xc0;
	#endif
	pDDFCHReg->ddfch_luma_base_addr_0 = luma_addr>>10;
	pDDFCHReg->ddfch_crma_base_addr_0 = chroma_addr>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(w, 128); //video line pitch
	//pDDFCHReg->ddfch_vdo_crop_size = ((h<<16) | w); //y size & x size
	pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((h<<16) | w); //y size & x size

	//pVPPDMAReg->vdma_gc 				&= (~0x80000000); //G186.01 , vppdma dis
	//pVPPDMAReg->vdma_gc 					= 0x00000028; //G186.01 , vppdma dis

	return 0;
}
EXPORT_SYMBOL(ddfch_setting);

int vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h)
{
	//sp_disp_info("vscl setting for HDMI \n");

	sp_disp_dbg("vscl position (x, y) =(%d, %d)\n", x, y);
	sp_disp_dbg("IN (%d, %d), OUT (%d, %d)\n", input_w, input_h, output_w, output_h);

	if ((input_w == output_w) && (input_h == output_h))
		sp_disp_dbg("scaling ratio (x, y) = (1000,1000) \n");
	else {
		sp_disp_dbg("scaling ratio (x, y) = (%04d,%04d) \n", output_w * 1000 / input_w, output_h * 1000 / input_h);
	}

	pVSCLReg->vscl0_actrl_i_xlen 	= input_w;	//G187.03
	pVSCLReg->vscl0_actrl_i_ylen 	= input_h;	//G187.04
	pVSCLReg->vscl0_actrl_s_xstart	= x;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart	= y;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= input_w;	//G187.07
	pVSCLReg->vscl0_actrl_s_ylen	= input_h;	//G187.08
	pVSCLReg->vscl0_dctrl_o_xlen	= output_w;	//G187.09
	pVSCLReg->vscl0_dctrl_o_ylen	= output_h;	//G187.10	
	pVSCLReg->vscl0_dctrl_d_xstart	= x;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart	= y;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= output_w;	//G187.13
	pVSCLReg->vscl0_dctrl_d_ylen	= output_h;	//G187.14

	pVSCLReg->vscl0_hint_ctrl			= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	= 0x00000000;	//G187.19
	//pVSCLReg->vscl0_hint_hfactor_high	= 0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_hfactor_high	= (input_w*64)/output_w;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		= 0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		= 0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl			= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	= 0x00000000;	//G188.01
	//pVSCLReg->vscl0_vint_vfactor_high	= 0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_vfactor_high	= (input_h*64)/output_h;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		= 0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		= 0x00000000;	//G188.04

	sp_disp_dbg("scaling reg (x, y) = (0x%08x,0x%08x) \n", pVSCLReg->vscl0_hint_hfactor_high, pVSCLReg->vscl0_vint_vfactor_high);

	return 0;
}
EXPORT_SYMBOL(vscl_setting);

void vppdma_flip_setting(int flip_v, int flip_h)
{
	int vdma_cfg_tmp;

	vdma_cfg_tmp = pVPPDMAReg->vdma_cfg;

	vdma_cfg_tmp &= ~((1 << 7) | (1 << 6));

	if ( (flip_v == 0) && (flip_h == 0) )
		vdma_cfg_tmp 	= (0 << 7) | (0 << 6); //flip_v normal, flip_h normal
	else if ( (flip_v == 1) && (flip_h == 0) )
		vdma_cfg_tmp 	= (1 << 7) | (0 << 6); //flip_v swap, flip_h normal
	else if ( (flip_v == 0) && (flip_h == 1) )
		vdma_cfg_tmp 	= (0 << 7) | (1 << 6); //flip_v normal, flip_h swap
	else if ( (flip_v == 1) && (flip_h == 1) )
		vdma_cfg_tmp 	= (1 << 7) | (1 << 6); //flip_v swap, flip_h normal

	pVPPDMAReg->vdma_cfg = vdma_cfg_tmp;

}
EXPORT_SYMBOL(vppdma_flip_setting);

int vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt)
{
	int vdma_cfg_tmp, vdma_frame_size_tmp;

	sp_disp_dbg("vppdma luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);
	sp_disp_dbg("vppdma w=%d, h=%d, fmt= %s\n", w, h, VppdmaFmt[vppdma_fmt]);

	#if (VPPDMA_GRP_EN == 1)
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	#else
	pVPPDMAReg->vdma_gc 							= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	#endif

	vdma_cfg_tmp = pVPPDMAReg->vdma_cfg;

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

	pVPPDMAReg->vdma_cfg = vdma_cfg_tmp;

	vdma_frame_size_tmp = pVPPDMAReg->vdma_frame_size;
	vdma_frame_size_tmp &= ~( (0xfff << 16) | (0x1fff << 0) );
	vdma_frame_size_tmp = ((h << 16) | w);
	pVPPDMAReg->vdma_frame_size = vdma_frame_size_tmp;

	//pVPPDMAReg->vdma_frame_size 			= ((h<<16) | w);; //G186.03 , frame_h , frame_w
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	
	if (vppdma_fmt == 0)
		pVPPDMAReg->vdma_lstd_size 				= w*3; //G186.05 , stride size , source RGB888 (default)
	else if (vppdma_fmt == 1)
		pVPPDMAReg->vdma_lstd_size 				= w*2; //G186.05 , stride size , source RGB565
	else if (vppdma_fmt == 2)
		pVPPDMAReg->vdma_lstd_size 				= w*2; //G186.05 , stride size , source YUV422 UYVY
	else if (vppdma_fmt == 3)
		pVPPDMAReg->vdma_lstd_size 				= w; //G186.05 , stride size , source YUV422 NV16
	else if (vppdma_fmt == 4)
		pVPPDMAReg->vdma_lstd_size 				= w*2; //G186.05 , stride size , source YUV422 YUY2

	if (luma_addr != 0)
		pVPPDMAReg->vdma_data_addr1 			= luma_addr; //G186.06 , 1st planner addr (luma)
	if (chroma_addr != 0)
		pVPPDMAReg->vdma_data_addr2 			= chroma_addr; //G186.07 , 2nd planner addr (crma)

	return 0;
}
EXPORT_SYMBOL(vppdma_setting);

void vpp_path_cur_setting_read(void)
{
	int tmp;

	sp_disp_dbg("vpp_path_cur_setting_read \n");
	tmp = pVPPDMAReg->vdma_frame_size;
	//temp_w = ((tmp >> 0) & 0x1fff);
	//temp_h = ((tmp >> 16) & 0xfff);
	sp_disp_dbg("vppdma w %d , h %d \n", (tmp & 0x1fff), ((tmp >> 16)&0xfff));
	//sp_disp_info("vppdma w %d , h %d \n", temp_w, temp_h);

	tmp = pVSCLReg->vscl0_config2;
	tmp = (tmp & 0x3000) >> 12;
	sp_disp_dbg("vscl path sel = %s \n", path_sel[tmp]);

	sp_disp_dbg("vscl i_xlen %d , o_xlen %d \n", pVSCLReg->vscl0_actrl_i_xlen, pVSCLReg->vscl0_dctrl_o_xlen);
	sp_disp_dbg("vscl i_ylen %d , y_xlen %d \n", pVSCLReg->vscl0_actrl_i_ylen, pVSCLReg->vscl0_dctrl_o_ylen);


}
EXPORT_SYMBOL(vpp_path_cur_setting_read);

void DRV_VSCL_SetColorbar(DRV_VppWindow_e vpp_path_sel, int enable)
{
	//sp_disp_info("DRV_VSCL_SetColorbar %d \n", enable);
	switch (vpp_path_sel) {
		case DRV_FROM_VPPDMA:
				sp_disp_info("DRV_VSCL_SetColorbar DRV_FROM_VPPDMA %d \n", enable);
				pVPPDMAReg->vdma_gc 					= 0x80000028; //G186.01 , vppdma en
				if (enable) {
					if(enable == 1) {
						pVSCLReg->vscl0_config2 			= 0x0000029f; //G187.01 , color bar en
					}
					else if(enable == 2) {
						pVSCLReg->vscl0_config2 			= 0x0000039f; //G187.01 , color bar en(bor)
					}
				}
				else {
					pVSCLReg->vscl0_config2 			= 0x0000021f; //G187.01 , color bar dis
				}
		break;
		case DRV_FROM_OSD0:
				sp_disp_info("DRV_VSCL_SetColorbar DRV_FROM_OSD0 %d \n", enable);
				if (enable) {
					if(enable == 1) {
						pVSCLReg->vscl0_config2 			= 0x0000129f; //G187.01 , color bar en
					}
					else if(enable == 2) {
						pVSCLReg->vscl0_config2 			= 0x0000139f; //G187.01 , color bar en(bor)
					}
				}
				else {
					pVSCLReg->vscl0_config2 			= 0x0000121f; //G187.01 , color bar dis
				}

		break;	
		case DRV_FROM_DDFCH:
				sp_disp_info("DRV_VSCL_SetColorbar DRV_FROM_DDFCH %d \n", enable);
				pVPPDMAReg->vdma_gc 					= 0x00000028; //G186.01 , vppdma dis
				if (enable) {
					if(enable == 1) {
						pVSCLReg->vscl0_config2 			= 0x0000229f; //G187.01 , color bar en
					}
					else if(enable == 2) {
						pVSCLReg->vscl0_config2 			= 0x0000239f; //G187.01 , color bar en(bor)
					}
				}
				else {
					pVSCLReg->vscl0_config2 			= 0x0000221f; //G187.01 , color bar dis
				}

		break;	
		default:
		break;
	}
}

void DRV_VPP_SetColorbar(DRV_VppWindow_e vpp_path_sel, int enable)
{
	//sp_disp_info("DRV_VPP_SetColorbar sw%d %d \n", vpp_path_sel, enable);
	switch (vpp_path_sel) {
		case DRV_FROM_VPPDMA:
				sp_disp_info("DRV_VPP_SetColorbar from vppdma %d \n", enable);
				pVSCLReg->vscl0_config2 			= 0x0000021f; //G187.01 , switch to vppdma
				pVPPDMAReg->vdma_gc 					= 0x80000028; //G186.01 , vppdma en
				if (enable) {
					if(enable == 1) {
						pVPPDMAReg->vdma_cfg 				= 0x00000022; //G186.02 , bist on color bar
					}
					else if(enable == 2) {
						pVPPDMAReg->vdma_cfg 				= 0x00000032; //G186.02 , bist on color bar (bor)
					}
				}
				else {
					pVPPDMAReg->vdma_cfg 				= 0x0000002; //G186.02 , bist off
				}
		break;
		case DRV_FROM_OSD0:
				sp_disp_info("DRV_VPP_SetColorbar from osd0 %d \n", enable);
				//inplement in disp_osd.c
		break;	
		case DRV_FROM_DDFCH:
				sp_disp_info("DRV_VPP_SetColorbar from ddfch %d \n", enable);
				
				if (enable) {
					if(enable == 1) {
						pDDFCHReg->ddfch_bist 				= 0x80801013; //G185.28 , color bar en (color bar)
					}
					else if(enable == 2) {
						pDDFCHReg->ddfch_bist 				= 0x80801011; //G185.28 , color bar en (half color bar)
					}
					else if(enable == 3) {
						pDDFCHReg->ddfch_bist 				= 0x80801001; //G185.28 , color bar en (half border)
					}
				}
				else {
					pDDFCHReg->ddfch_bist 				= 0x80801002; //G185.28 , color bar dis
				}
				
		break;	
		default:
		break;
	}
}

void DRV_VPOST_dump(void)
{
	;
}
void DRV_VSCL_dump(void)
{
	//dump after setting
	printk(KERN_INFO "pVSCLReg G187.00 0x%08x(%d) \n",pVSCLReg->vscl0_config1,pVSCLReg->vscl0_config1);
	printk(KERN_INFO "pVSCLReg G187.01 0x%08x(%d) \n",pVSCLReg->vscl0_config2,pVSCLReg->vscl0_config2);
	printk(KERN_INFO "pVSCLReg G187.03 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_i_xlen,pVSCLReg->vscl0_actrl_i_xlen);
	printk(KERN_INFO "pVSCLReg G187.04 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_i_ylen,pVSCLReg->vscl0_actrl_i_ylen);
	printk(KERN_INFO "pVSCLReg G187.05 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_s_xstart,pVSCLReg->vscl0_actrl_s_xstart);
	printk(KERN_INFO "pVSCLReg G187.06 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_s_ystart,pVSCLReg->vscl0_actrl_s_ystart);
	printk(KERN_INFO "pVSCLReg G187.07 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_s_xlen,pVSCLReg->vscl0_actrl_s_xlen);
	printk(KERN_INFO "pVSCLReg G187.08 0x%08x(%d) \n",pVSCLReg->vscl0_actrl_s_ylen,pVSCLReg->vscl0_actrl_s_ylen);
	printk(KERN_INFO "pVSCLReg G187.09 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_o_xlen,pVSCLReg->vscl0_dctrl_o_xlen);
	printk(KERN_INFO "pVSCLReg G187.10 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_o_ylen,pVSCLReg->vscl0_dctrl_o_ylen);
	printk(KERN_INFO "pVSCLReg G187.11 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_d_xstart,pVSCLReg->vscl0_dctrl_d_xstart);
	printk(KERN_INFO "pVSCLReg G187.12 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_d_ystart,pVSCLReg->vscl0_dctrl_d_ystart);
	printk(KERN_INFO "pVSCLReg G187.13 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_d_xlen,pVSCLReg->vscl0_dctrl_d_xlen);
	printk(KERN_INFO "pVSCLReg G187.14 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_d_ylen,pVSCLReg->vscl0_dctrl_d_ylen);
	printk(KERN_INFO "pVSCLReg G187.15 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_bgc_c,pVSCLReg->vscl0_dctrl_bgc_c);
	printk(KERN_INFO "pVSCLReg G187.16 0x%08x(%d) \n",pVSCLReg->vscl0_dctrl_bgc_y,pVSCLReg->vscl0_dctrl_bgc_y);
	#if 1
	printk(KERN_INFO "pVSCLReg G187.18 0x%08x(%d) \n",pVSCLReg->vscl0_hint_ctrl,pVSCLReg->vscl0_hint_ctrl);
	printk(KERN_INFO "pVSCLReg G187.19 0x%08x(%d) \n",pVSCLReg->vscl0_hint_hfactor_low,pVSCLReg->vscl0_hint_hfactor_low);
	printk(KERN_INFO "pVSCLReg G187.20 0x%08x(%d) \n",pVSCLReg->vscl0_hint_hfactor_high,pVSCLReg->vscl0_hint_hfactor_high);
	printk(KERN_INFO "pVSCLReg G187.21 0x%08x(%d) \n",pVSCLReg->vscl0_hint_initf_low,pVSCLReg->vscl0_hint_initf_low);
	printk(KERN_INFO "pVSCLReg G187.22 0x%08x(%d) \n",pVSCLReg->vscl0_hint_initf_high,pVSCLReg->vscl0_hint_initf_high);

	printk(KERN_INFO "pVSCLReg G188.00 0x%08x(%d) \n",pVSCLReg->vscl0_vint_ctrl,pVSCLReg->vscl0_vint_ctrl);
	printk(KERN_INFO "pVSCLReg G188.01 0x%08x(%d) \n",pVSCLReg->vscl0_vint_vfactor_low,pVSCLReg->vscl0_vint_vfactor_low);
	printk(KERN_INFO "pVSCLReg G188.02 0x%08x(%d) \n",pVSCLReg->vscl0_vint_vfactor_high,pVSCLReg->vscl0_vint_vfactor_high);
	printk(KERN_INFO "pVSCLReg G188.03 0x%08x(%d) \n",pVSCLReg->vscl0_vint_initf_low,pVSCLReg->vscl0_vint_initf_low);
	printk(KERN_INFO "pVSCLReg G188.04 0x%08x(%d) \n",pVSCLReg->vscl0_vint_initf_high,pVSCLReg->vscl0_vint_initf_high);
	#endif
}
void DRV_VPPDMA_dump(void)
{
	printk(KERN_INFO "pVPPDMAReg G186.00 0x%08x(%d) \n",pVPPDMAReg->vdma_ver,pVPPDMAReg->vdma_ver);
	printk(KERN_INFO "pVPPDMAReg G186.01 0x%08x(%d) \n",pVPPDMAReg->vdma_gc,pVPPDMAReg->vdma_gc);
	printk(KERN_INFO "pVPPDMAReg G186.02 0x%08x(%d) \n",pVPPDMAReg->vdma_cfg,pVPPDMAReg->vdma_cfg);
	printk(KERN_INFO "pVPPDMAReg G186.03 0x%08x(%d) \n",pVPPDMAReg->vdma_frame_size,pVPPDMAReg->vdma_frame_size);
	printk(KERN_INFO "pVPPDMAReg G186.04 0x%08x(%d) \n",pVPPDMAReg->vdma_crop_st,pVPPDMAReg->vdma_crop_st);
	printk(KERN_INFO "pVPPDMAReg G186.05 0x%08x(%d) \n",pVPPDMAReg->vdma_lstd_size,pVPPDMAReg->vdma_lstd_size);
	printk(KERN_INFO "pVPPDMAReg G186.06 0x%08x(%d) \n",pVPPDMAReg->vdma_data_addr1,pVPPDMAReg->vdma_data_addr1);
	printk(KERN_INFO "pVPPDMAReg G186.07 0x%08x(%d) \n",pVPPDMAReg->vdma_data_addr2,pVPPDMAReg->vdma_data_addr2);
	
}
void DRV_DDFCH_dump(void)
{
	//dump after setting
	printk(KERN_INFO "pDDFCHReg G185.00 0x%08x(%d) \n",pDDFCHReg->ddfch_latch_en,pDDFCHReg->ddfch_latch_en);
	printk(KERN_INFO "pDDFCHReg G185.01 0x%08x(%d) \n",pDDFCHReg->ddfch_mode_option,pDDFCHReg->ddfch_mode_option);
	printk(KERN_INFO "pDDFCHReg G185.02 0x%08x(%d) \n",pDDFCHReg->ddfch_enable,pDDFCHReg->ddfch_enable);
	printk(KERN_INFO "pDDFCHReg G185.03 0x%08x(%d) \n",pDDFCHReg->ddfch_urgent_thd,pDDFCHReg->ddfch_urgent_thd);
	printk(KERN_INFO "pDDFCHReg G185.04 0x%08x(%d) \n",pDDFCHReg->ddfch_cmdq_thd,pDDFCHReg->ddfch_cmdq_thd);
	printk(KERN_INFO "pDDFCHReg G185.05 0x%08x(%d) \n",pDDFCHReg->g185_reserved0,pDDFCHReg->g185_reserved0);
	printk(KERN_INFO "pDDFCHReg G185.06 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_0,pDDFCHReg->ddfch_luma_base_addr_0);
	printk(KERN_INFO "pDDFCHReg G185.09 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_0,pDDFCHReg->ddfch_crma_base_addr_0);
	printk(KERN_INFO "pDDFCHReg G185.15 0x%08x(%d) \n",pDDFCHReg->ddfch_frame_id,pDDFCHReg->ddfch_frame_id);
	printk(KERN_INFO "pDDFCHReg G185.16 0x%08x(%d) \n",pDDFCHReg->ddfch_free_run_control,pDDFCHReg->ddfch_free_run_control);
	printk(KERN_INFO "pDDFCHReg G185.20 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_frame_size,pDDFCHReg->ddfch_vdo_frame_size);
	printk(KERN_INFO "pDDFCHReg G185.21 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_size,pDDFCHReg->ddfch_vdo_crop_size);
	printk(KERN_INFO "pDDFCHReg G185.22 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_offset,pDDFCHReg->ddfch_vdo_crop_offset);
	printk(KERN_INFO "pDDFCHReg G185.23 0x%08x(%d) \n",pDDFCHReg->ddfch_config_0,pDDFCHReg->ddfch_config_0);
	printk(KERN_INFO "pDDFCHReg G185.24 0x%08x(%d) \n",pDDFCHReg->ddfch_config_1,pDDFCHReg->ddfch_config_1);
	printk(KERN_INFO "pDDFCHReg G185.28 0x%08x(%d) \n",pDDFCHReg->ddfch_bist,pDDFCHReg->ddfch_bist);		
}
