/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2020 by Sunplus Inc.                             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Inc. All rights are reserved by Sunplus Inc.                          *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Inc. reserves the right to modify this software               *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus Inc.                                                          *
 *  19, Innovation First Road, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file disp_vpp.c
 * @brief
 * @author Hammer Hsieh
 */
/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include <linux/module.h>

#include "display.h"
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
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_VPP_Init(void *pInHWReg1, void *pInHWReg2, void *pInHWReg3, void *pInHWReg4)
{

	sp_disp_info("DRV_VPP_Init \n");
	pVPOSTReg = (DISP_VPOST_REG_t *)pInHWReg1;
	pVSCLReg = (DISP_VSCL_REG_t *)pInHWReg2;
	pDDFCHReg = (DISP_DDFCH_REG_t *)pInHWReg3;
	pVPPDMAReg = (DISP_VPPDMA_REG_t *)pInHWReg4;

	//for i143
	#ifdef DISP_64X64
	//64x64 setting
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
	
	pVSCLReg->vscl0_actrl_i_xlen 	= 0x00000040;	//G187.03
	pVSCLReg->vscl0_actrl_i_ylen 	= 0x00000040;	//G187.04
	pVSCLReg->vscl0_actrl_s_xstart= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= 0x00000040;	//G187.07
	pVSCLReg->vscl0_actrl_s_ylen	= 0x00000040;	//G187.08
	pVSCLReg->vscl0_dctrl_o_xlen	= 0x00000040;	//G187.09
	pVSCLReg->vscl0_dctrl_o_ylen	= 0x00000040;	//G187.10	
	pVSCLReg->vscl0_dctrl_d_xstart= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= 0x00000040;	//G187.13
	pVSCLReg->vscl0_dctrl_d_ylen	= 0x00000040;	//G187.14
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2
	pVPPDMAReg->vdma_frame_size 			= 0x00400040; //G186.03 , frame_h , frame_w
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	pVPPDMAReg->vdma_lstd_size 				= 0x00000100; //G186.05 , stride size
	pVPPDMAReg->vdma_data_addr1 			= 0x20300000; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= 0x01dc0000; //G186.07 , 2nd planner addr (crma)

	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting
	
	//From VPPDMA
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
	//pVSCLReg->vscl0_config2 			= 0x0000029F;	//G187.01 color bar en , color bar
	//pVSCLReg->vscl0_config2 			= 0x0000039F;	//G187.01 color bar en , border
	
	//From OSD0
	//pVSCLReg->vscl0_config2 			= 0x0000121F;	//G187.01
	//pVSCLReg->vscl0_config2 			= 0x0000129F;	//G187.01 color bar en , color bar
	//pVSCLReg->vscl0_config2 			= 0x0000139F;	//G187.01 color bar en , border	
	
	//From DDFCH
	//pVSCLReg->vscl0_config2 			= 0x0000221F;	//G187.01
	//pVSCLReg->vscl0_config2 			= 0x0000229F;	//G187.01 color bar en , color bar
	//pVSCLReg->vscl0_config2 			= 0x0000239F;	//G187.01 color bar en , border		
	
	pVSCLReg->vscl0_actrl_i_xlen 	= 0x000002D0;	//G187.03
	pVSCLReg->vscl0_actrl_i_ylen 	= 0x000001E0;	//G187.04
	pVSCLReg->vscl0_actrl_s_xstart= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= 0x000002D0;	//G187.07
	pVSCLReg->vscl0_actrl_s_ylen	= 0x000001E0;	//G187.08
	pVSCLReg->vscl0_dctrl_o_xlen	= 0x000002D0;	//G187.09
	pVSCLReg->vscl0_dctrl_o_ylen	= 0x000001E0;	//G187.10	
	pVSCLReg->vscl0_dctrl_d_xstart= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= 0x000002D0;	//G187.13
	pVSCLReg->vscl0_dctrl_d_ylen	= 0x000001E0;	//G187.14
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		=	0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		=	0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		=	0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		=	0x00000000;	//G188.04
	
	#if 1
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	//pVPPDMAReg->vdma_gc 							= 0x00000028; //G186.01 , vppdma off , urgent th = 0x28
	
	#if (VPPDMA_FMT_HDMI == 0) //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x000002D0*3; //G186.05 , stride size RGB888
	#elif (VPPDMA_FMT_HDMI == 1) //RGB565
	pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test3 - ok)
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size RGB565
	#elif (VPPDMA_FMT_HDMI == 2) //YUY2
	pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2 (test1)
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size YUY2
	#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
	pVPPDMAReg->vdma_cfg 							= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0); //G186.05 , stride size YUV422_NV16
	#else //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x000002D0*3; //G186.05 , stride size RGB888	
	#endif
	//pVPPDMAReg->vdma_cfg 							= 0x00000400; //G186.02 , no swap , bist off , fmt = RGB888 (test1)
	//pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	//pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test1)
	//pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test2)
	//pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test3 - ok)
	//pVPPDMAReg->vdma_cfg 							= 0x00001002; //G186.02 , no swap , bist off , fmt = YUY2 (test1)
	//pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2 (test4)
	//pVPPDMAReg->vdma_cfg 							= 0x00000022; //G186.02 , no swap , bist on color bar , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000032; //G186.02 , no swap , bist on border , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
	pVPPDMAReg->vdma_frame_size 			= 0x01E002D0; //G186.03 , frame_h , frame_w
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	//pVPPDMAReg->vdma_lstd_size 				= (0x000002D0*3); //G186.05 , stride size RGB888
	//pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size RGB565
	//pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size YUY2
	//pVPPDMAReg->vdma_lstd_size 				= (0x000002D0); //G186.05 , stride size YUV422_NV16
	pVPPDMAReg->vdma_data_addr1 			= 0x20300000; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= 0x01dc0000; //G186.07 , 2nd planner addr (crma)
	
	//pVPOSTReg->vpost_opif_config			= 0x00000001; //G199.09 //en mask region
	//pVPOSTReg->vpost_opif_msktop			= 0x00000040; //G199.13
	//pVPOSTReg->vpost_opif_mskbot			= 0x00000040; //G199.14
	//pVPOSTReg->vpost_opif_mskleft			= 0x00000040; //G199.15
	//pVPOSTReg->vpost_opif_mskright		= 0x00000040; //G199.16
	
	#endif
	#if 1
	//pDDFCHReg->ddfch_latch_en 				= 1; //G185.00 //latch en
	//pDDFCHReg->ddfch_mode_option			= 0x00000000; //G185.01 //YUV422
	//pDDFCHReg->ddfch_enable 					= 0xd0; //G185.02 //fetch en

	//pDDFCHReg->ddfch_luma_base_addr_0 = (0x120000)>>10;
	//pDDFCHReg->ddfch_crma_base_addr_0 = (0x120000 + 768*480)>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch
	//pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	//pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((0x1E0<<16) | 0x2D0); //y size & x size
	#endif

	#endif	

	#ifdef DISP_576P
	//576P 720x576 setting
	
	//From VPPDMA
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
	pVSCLReg->vscl0_actrl_i_xlen 	= 0x000002D0;	//G187.03 (720)
	pVSCLReg->vscl0_actrl_i_ylen 	= 0x00000240;	//G187.04 (576)
	pVSCLReg->vscl0_actrl_s_xstart= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= 0x000002D0;	//G187.07 (720)
	pVSCLReg->vscl0_actrl_s_ylen	= 0x00000240;	//G187.08 (576)
	pVSCLReg->vscl0_dctrl_o_xlen	= 0x000002D0;	//G187.09 (720)
	pVSCLReg->vscl0_dctrl_o_ylen	= 0x00000240;	//G187.10	(576)
	pVSCLReg->vscl0_dctrl_d_xstart= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= 0x000002D0;	//G187.13 (720)
	pVSCLReg->vscl0_dctrl_d_ylen	= 0x00000240;	//G187.14 (576)
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		=	0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		=	0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		=	0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		=	0x00000000;	//G188.04
	
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	#if (VPPDMA_FMT_HDMI == 0) //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x000002D0*3; //G186.05 , stride size RGB888
	#elif (VPPDMA_FMT_HDMI == 1) //RGB565
	pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test3 - ok)
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size RGB565
	#elif (VPPDMA_FMT_HDMI == 2) //YUY2
	pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2 (test1)
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size YUY2
	#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
	pVPPDMAReg->vdma_cfg 							= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
	pVPPDMAReg->vdma_lstd_size 				= (0x000002D0); //G186.05 , stride size YUV422_NV16
	#else //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x000002D0*3; //G186.05 , stride size RGB888	
	#endif
	//pVPPDMAReg->vdma_cfg 							= 0x00001002; //G186.02 , no swap , bist off , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000022; //G186.02 , no swap , bist on color bar , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000032; //G186.02 , no swap , bist on border , fmt = YUY2
	pVPPDMAReg->vdma_frame_size 			= 0x024002D0; //G186.03 , frame_h , frame_w (576 720)
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	//pVPPDMAReg->vdma_lstd_size 				= (0x000002D0<<1); //G186.05 , stride size (720)
	pVPPDMAReg->vdma_data_addr1 			= 0x20300000; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= 0x01dc0000; //G186.07 , 2nd planner addr (crma)
	
	//pVPOSTReg->vpost_opif_config			= 0x00000001; //G199.09 //en mask region
	//pVPOSTReg->vpost_opif_msktop			= 0x00000040; //G199.13
	//pVPOSTReg->vpost_opif_mskbot			= 0x00000040; //G199.14
	//pVPOSTReg->vpost_opif_mskleft			= 0x00000040; //G199.15
	//pVPOSTReg->vpost_opif_mskright		= 0x00000040; //G199.16
	
	#if 0
	//pDDFCHReg->ddfch_latch_en 				= 1; //G185.00 //latch en
	//pDDFCHReg->ddfch_mode_option			= 0x00000000; //G185.01 //YUV422
	//pDDFCHReg->ddfch_enable 					= 0xd0; //G185.02 //fetch en

	//pDDFCHReg->ddfch_luma_base_addr_0 = (0x20300000)>>10;
	//pDDFCHReg->ddfch_crma_base_addr_0 = (0x20300000 + 768*576)>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch (720 -> 768)
	//pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	//pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((0x240<<16) | 0x2D0); //y size & x size (576 720)
	#endif
	#endif	

	#ifdef DISP_720P
	//720P 1280x720 setting
	
	//From VPPDMA
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
	pVSCLReg->vscl0_actrl_i_xlen 	= 0x00000500;	//G187.03 (1280)
	pVSCLReg->vscl0_actrl_i_ylen 	= 0x000002D0;	//G187.04 (720)
	pVSCLReg->vscl0_actrl_s_xstart= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= 0x00000500;	//G187.07 (1280)
	pVSCLReg->vscl0_actrl_s_ylen	= 0x000002D0;	//G187.08 (720)
	pVSCLReg->vscl0_dctrl_o_xlen	= 0x00000500;	//G187.09 (1280)
	pVSCLReg->vscl0_dctrl_o_ylen	= 0x000002D0;	//G187.10	(720)
	pVSCLReg->vscl0_dctrl_d_xstart= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= 0x00000500;	//G187.13 (1280)
	pVSCLReg->vscl0_dctrl_d_ylen	= 0x000002D0;	//G187.14 (720)
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		=	0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		=	0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		=	0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		=	0x00000000;	//G188.04
	
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	#if (VPPDMA_FMT_HDMI == 0) //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x00000500*3; //G186.05 , stride size RGB888
	#elif (VPPDMA_FMT_HDMI == 1) //RGB565
	pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test3 - ok)
	pVPPDMAReg->vdma_lstd_size 				= (0x00000500<<1); //G186.05 , stride size RGB565
	#elif (VPPDMA_FMT_HDMI == 2) //YUY2
	pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2 (test1)
	pVPPDMAReg->vdma_lstd_size 				= (0x00000500<<1); //G186.05 , stride size YUY2
	#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
	pVPPDMAReg->vdma_cfg 							= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
	pVPPDMAReg->vdma_lstd_size 				= (0x00000500); //G186.05 , stride size YUV422_NV16
	#else //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x00000500*3; //G186.05 , stride size RGB888	
	#endif
	//pVPPDMAReg->vdma_cfg 							= 0x00001002; //G186.02 , no swap , bist off , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000022; //G186.02 , no swap , bist on color bar , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000032; //G186.02 , no swap , bist on border , fmt = YUY2
	pVPPDMAReg->vdma_frame_size 			= 0x02D00500; //G186.03 , frame_h , frame_w (720 1280)
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	//pVPPDMAReg->vdma_lstd_size 				= (0x00000500<<1); //G186.05 , stride size (1280)
	pVPPDMAReg->vdma_data_addr1 			= 0x20300000; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= 0x01dc0000; //G186.07 , 2nd planner addr (crma)
	
	//pVPOSTReg->vpost_opif_config			= 0x00000001; //G199.09 //en mask region
	//pVPOSTReg->vpost_opif_msktop			= 0x00000040; //G199.13
	//pVPOSTReg->vpost_opif_mskbot			= 0x00000040; //G199.14
	//pVPOSTReg->vpost_opif_mskleft			= 0x00000040; //G199.15
	//pVPOSTReg->vpost_opif_mskright		= 0x00000040; //G199.16
	#if 0
	//pDDFCHReg->ddfch_latch_en 				= 1; //G185.00 //latch en
	//pDDFCHReg->ddfch_mode_option			= 0x00000000; //G185.01 //YUV422
	//pDDFCHReg->ddfch_enable 					= 0xd0; //G185.02 //fetch en

	//pDDFCHReg->ddfch_luma_base_addr_0 = (0x20300000)>>10;
	//pDDFCHReg->ddfch_crma_base_addr_0 = (0x20300000 + 1280*720)>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x0500, 128); //video line pitch (1280 -> 1280)
	//pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	//pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((0x2D0<<16) | 0x500); //y size & x size (720 1280)
	#endif	
	#endif
	
	#ifdef DISP_1080P
	//1080P 1920x1080 setting
	
	//From VPPDMA
	pVSCLReg->vscl0_config2 			= 0x0000021F;	//G187.01
	pVSCLReg->vscl0_actrl_i_xlen 	= 0x00000780;	//G187.03 (1920)
	pVSCLReg->vscl0_actrl_i_ylen 	= 0x00000438;	//G187.04 (1080)
	pVSCLReg->vscl0_actrl_s_xstart= 0x00000000;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= 0x00000000;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= 0x00000780;	//G187.07 (1920)
	pVSCLReg->vscl0_actrl_s_ylen	= 0x00000438;	//G187.08 (1080)
	pVSCLReg->vscl0_dctrl_o_xlen	= 0x00000780;	//G187.09 (1920)
	pVSCLReg->vscl0_dctrl_o_ylen	= 0x00000438;	//G187.10	(1080)
	pVSCLReg->vscl0_dctrl_d_xstart= 0x00000000;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= 0x00000000;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= 0x00000780;	//G187.13 (1920)
	pVSCLReg->vscl0_dctrl_d_ylen	= 0x00000438;	//G187.14 (1080)
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		=	0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		=	0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		=	0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		=	0x00000000;	//G188.04
	
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	#if (VPPDMA_FMT_HDMI == 0) //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x00000780*3; //G186.05 , stride size RGB888
	#elif (VPPDMA_FMT_HDMI == 1) //RGB565
	pVPPDMAReg->vdma_cfg 							= 0x00000001; //G186.02 , no swap , bist off , fmt = RGB565 (test3 - ok)
	pVPPDMAReg->vdma_lstd_size 				= (0x00000780<<1); //G186.05 , stride size RGB565
	#elif (VPPDMA_FMT_HDMI == 2) //YUY2
	pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2 (test1)
	pVPPDMAReg->vdma_lstd_size 				= (0x00000780<<1); //G186.05 , stride size YUY2
	#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
	pVPPDMAReg->vdma_cfg 							= 0x00000003; //G186.02 , no swap , bist off , fmt = YUV422_NV16
	pVPPDMAReg->vdma_lstd_size 				= (0x00000780); //G186.05 , stride size YUV422_NV16
	#else //RGB888
	pVPPDMAReg->vdma_cfg 							= 0x00000000; //G186.02 , no swap , bist off , fmt = RGB888 (test2 - ok)
	pVPPDMAReg->vdma_lstd_size 				= 0x00000780*3; //G186.05 , stride size RGB888	
	#endif
	//pVPPDMAReg->vdma_cfg 							= 0x00001002; //G186.02 , no swap , bist off , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000022; //G186.02 , no swap , bist on color bar , fmt = YUY2
	//pVPPDMAReg->vdma_cfg 							= 0x00000032; //G186.02 , no swap , bist on border , fmt = YUY2
	pVPPDMAReg->vdma_frame_size 			= 0x04380780; //G186.03 , frame_h , frame_w (1080 1920)
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	//pVPPDMAReg->vdma_lstd_size 				= (0x00000780<<1); //G186.05 , stride size (1920)
	pVPPDMAReg->vdma_data_addr1 			= 0x20300000; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= 0x01dc0000; //G186.07 , 2nd planner addr (crma)
	
	//pVPOSTReg->vpost_opif_config			= 0x00000001; //G199.09 //en mask region
	//pVPOSTReg->vpost_opif_msktop			= 0x00000040; //G199.13
	//pVPOSTReg->vpost_opif_mskbot			= 0x00000040; //G199.14
	//pVPOSTReg->vpost_opif_mskleft			= 0x00000040; //G199.15
	//pVPOSTReg->vpost_opif_mskright		= 0x00000040; //G199.16
	#if 0
	//pDDFCHReg->ddfch_latch_en 				= 1; //G185.00 //latch en
	//pDDFCHReg->ddfch_mode_option			= 0x00000000; //G185.01 //YUV422
	//pDDFCHReg->ddfch_enable 					= 0xd0; //G185.02 //fetch en

	//pDDFCHReg->ddfch_luma_base_addr_0 = (0x20300000)>>10;
	//pDDFCHReg->ddfch_crma_base_addr_0 = (0x20300000 + 1920*1080)>>10;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x0780, 128); //video line pitch (1920 -> 1920)
	//pDDFCHReg->ddfch_vdo_crop_offset = ((0 << 16) | 0);
	//pDDFCHReg->ddfch_config_0 = 0x10000;
	pDDFCHReg->ddfch_bist = 0x80801002;
	pDDFCHReg->ddfch_vdo_crop_size = ((0x438<<16) | 0x780); //y size & x size (1080 1920)
	#endif
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

void DRV_DDFCH_Init10(void)
{
	sp_disp_info("DRV_DDFCH_Init1 ddfch w/h set \n");
	pDDFCHReg->ddfch_vdo_crop_size 	= ((0x1E0<<16) | 0x2D0); //y size & x size	
	//pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch
	
	#if 0
	int i;
	sp_disp_info("DRV_DDFCH_Init1 \n");
	pVSCLReg->vscl0_config2 			= 0x0000221F;	//G187.01
	for(i = 0; i < 50; i++) ;
	pDDFCHReg->ddfch_vdo_crop_size 	= ((0x1E0<<16) | 0x2D0); //y size & x size	
	for(i = 0; i < 50; i++) ;
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch
	for(i = 0; i < 50; i++) ;
	pDDFCHReg->ddfch_bist 					= 0x80801013;
	for(i = 0; i < 50; i++) ;
	pDDFCHReg->ddfch_enable 				= 0xd0; //G185.02 //fetch en
	for(i = 0; i < 50; i++) ;
	pVPPDMAReg->vdma_gc 						= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	for(i = 0; i < 50; i++) ;
	#endif
}

void DRV_DDFCH_Init11(void)
{
	sp_disp_info("DRV_DDFCH_Init11 ddfch line pitch \n");
	//pDDFCHReg->ddfch_vdo_crop_size 	= ((0x1E0<<16) | 0x2D0); //y size & x size	
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch

}

void DRV_DDFCH_Init2(void)
{
	sp_disp_info("DRV_DDFCH_Init2 ddfch en \n");
	pDDFCHReg->ddfch_enable 				= 0xd0; //G185.02 //fetch en
	#if 0
	sp_disp_info("DRV_DDFCH_Init2 \n");
	pVSCLReg->vscl0_config2 			= 0x0000221F;	//G187.01
	
	pDDFCHReg->ddfch_vdo_crop_size 	= ((0x1E0<<16) | 0x2D0); //y size & x size	
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch
	pDDFCHReg->ddfch_bist 					= 0x80801013;
	pDDFCHReg->ddfch_enable 				= 0xd0; //G185.02 //fetch en
	
	pVPPDMAReg->vdma_gc 						= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	#endif

}

void DRV_DDFCH_Init3(void)
{
	sp_disp_info("DRV_DDFCH_Init3 ddfch color bar \n");
	pDDFCHReg->ddfch_bist 					= 0x80801013; //ddfch color bar
	#if 0
	sp_disp_info("DRV_DDFCH_Init3 \n");
	pVSCLReg->vscl0_config2 			= 0x0000221F;	//G187.01
	
	pDDFCHReg->ddfch_vdo_crop_size 	= ((0x1E0<<16) | 0x2D0); //y size & x size	
	pDDFCHReg->ddfch_vdo_frame_size = EXTENDED_ALIGNED(0x02D0, 128); //video line pitch
	pDDFCHReg->ddfch_bist 					= 0x80801013;
	pDDFCHReg->ddfch_enable 				= 0xd0; //G185.02 //fetch en
	
	pVPPDMAReg->vdma_gc 						= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	#endif

}

void DRV_DDFCH_Init4(void)
{
	sp_disp_info("DRV_DDFCH_Init4 vppdma off \n");
	pVPPDMAReg->vdma_gc 						= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
}

void DRV_DDFCH_Init5(void)
{
	sp_disp_info("DRV_DDFCH_Init5 vppdma on \n");
	pVPPDMAReg->vdma_gc 						= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
}

void DRV_DDFCH_Init6(void)
{
	sp_disp_info("DRV_DDFCH_Init6 ddfch dis \n");
	pDDFCHReg->ddfch_enable 				= 0xc0; //G185.02 //fetch dis
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
	sp_disp_info("ddfch luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);
	//sp_disp_info("ddfch setting for HDMI \n");

	//pVSCLReg->vscl0_config2 			= 0x221F;	//G187.01 , switch to DDFCH
	#if (DDFCH_GRP_EN == 1)
	pDDFCHReg->ddfch_latch_en = 1;
	#else
	//pDDFCHReg->ddfch_latch_en = 0;
	#endif
	
	if (ddfch_fmt == 0){
		pDDFCHReg->ddfch_mode_option = 0; //source yuv420 NV12
		sp_disp_info("ddfch setting fmt YUV420_NV12 \n");
	}
	else if(ddfch_fmt == 1) {
		pDDFCHReg->ddfch_mode_option = 0x400; //source yuv422 NV16
		sp_disp_info("ddfch setting fmt YUV422_NV16 \n");
	}
	else if(ddfch_fmt == 2) {
		pDDFCHReg->ddfch_mode_option = 0x800; //source yuv422 YUY2
		sp_disp_info("ddfch setting fmt YUV422_YUY2 \n");
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
//EXPORT_SYMBOL(ddfch_setting);

int vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h)
{
	sp_disp_info("vscl setting for HDMI \n");
	
	pVSCLReg->vscl0_actrl_i_xlen 	= input_w;	//G187.03
	pVSCLReg->vscl0_actrl_i_ylen 	= input_h;	//G187.04
	pVSCLReg->vscl0_actrl_s_xstart= x;	//G187.05
	pVSCLReg->vscl0_actrl_s_ystart= y;	//G187.06
	pVSCLReg->vscl0_actrl_s_xlen	= input_w;	//G187.07
	pVSCLReg->vscl0_actrl_s_ylen	= input_h;	//G187.08
	pVSCLReg->vscl0_dctrl_o_xlen	= output_w;	//G187.09
	pVSCLReg->vscl0_dctrl_o_ylen	= output_h;	//G187.10	
	pVSCLReg->vscl0_dctrl_d_xstart= x;	//G187.11
	pVSCLReg->vscl0_dctrl_d_ystart= y;	//G187.12
	pVSCLReg->vscl0_dctrl_d_xlen	= output_w;	//G187.13
	pVSCLReg->vscl0_dctrl_d_ylen	= output_h;	//G187.14
	
	
	pVSCLReg->vscl0_hint_ctrl					= 0x00000002;	//G187.18
	pVSCLReg->vscl0_hint_hfactor_low	=	0x00000000;	//G187.19
	//pVSCLReg->vscl0_hint_hfactor_high	=	0x00000040;	//G187.20
	pVSCLReg->vscl0_hint_hfactor_high	=	(input_w*64)/output_w;	//G187.20
	pVSCLReg->vscl0_hint_initf_low		=	0x00000000;	//G187.21
	pVSCLReg->vscl0_hint_initf_high		=	0x00000000;	//G187.22	
	
	pVSCLReg->vscl0_vint_ctrl					= 0x00000002;	//G188.00
	pVSCLReg->vscl0_vint_vfactor_low	=	0x00000000;	//G188.01
	//pVSCLReg->vscl0_vint_vfactor_high	=	0x00000040;	//G188.02
	pVSCLReg->vscl0_vint_vfactor_high	=	(input_h*64)/output_h;	//G188.02
	pVSCLReg->vscl0_vint_initf_low		=	0x00000000;	//G188.03
	pVSCLReg->vscl0_vint_initf_high		=	0x00000000;	//G188.04

	return 0;
}

int vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt)
{
	sp_disp_info("vppdma luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);
	sp_disp_info("vppdma setting for HDMI \n");

	#if (VPPDMA_GRP_EN == 1)
	pVPPDMAReg->vdma_gc 							= 0x80000028; //G186.01 , vppdma en , urgent th = 0x28
	#else
	pVPPDMAReg->vdma_gc 							= 0x00000028; //G186.01 , vppdma dis , urgent th = 0x28
	#endif
	#if 0
	//pVPPDMAReg->vdma_cfg 							= 0x00000002; //G186.02 , no swap , bist off , fmt = YUY2
	pVPPDMAReg->vdma_cfg 							&= (~0x00000003); //G186.02
	
	if (vppdma_fmt == 0)
		pVPPDMAReg->vdma_cfg 			|= 0x00; //source RGB888 (default)
	else if(vppdma_fmt == 1)
		pVPPDMAReg->vdma_cfg 			|= 0x01; //source RGB565
	else if(vppdma_fmt == 2)
		pVPPDMAReg->vdma_cfg 			|= 0x02; //source YUV422 YUY2
	else if(vppdma_fmt == 3)
		pVPPDMAReg->vdma_cfg 			|= 0x03; //source YUV422 NV16
	
	#endif
		
	if (vppdma_fmt == 0)
		pVPPDMAReg->vdma_cfg 			= 0x00; //source RGB888 (default)
	else if(vppdma_fmt == 1)
		pVPPDMAReg->vdma_cfg 			= 0x01; //source RGB565
	else if(vppdma_fmt == 2)
		pVPPDMAReg->vdma_cfg 			= 0x02; //source YUV422 YUY2
	else if(vppdma_fmt == 3)
		pVPPDMAReg->vdma_cfg 			= 0x03; //source YUV422 NV16	

	
	pVPPDMAReg->vdma_frame_size 			= ((h<<16) | w);; //G186.03 , frame_h , frame_w
	pVPPDMAReg->vdma_crop_st 					= 0x00000000; //G186.04 , start_h , Start_w
	
	if (vppdma_fmt == 0)
		pVPPDMAReg->vdma_lstd_size 				= w*3; //G186.05 , stride size , source RGB888 (default)
	else if (vppdma_fmt == 1)
		pVPPDMAReg->vdma_lstd_size 				= w*2; //G186.05 , stride size , source RGB565
	else if (vppdma_fmt == 2)
		pVPPDMAReg->vdma_lstd_size 				= w*2; //G186.05 , stride size , source YUV422 YUY2
	else if (vppdma_fmt == 3)
		pVPPDMAReg->vdma_lstd_size 				= w; //G186.05 , stride size , source YUV422 NV16

	pVPPDMAReg->vdma_data_addr1 			= luma_addr; //G186.06 , 1sr planner addr (luma)
	pVPPDMAReg->vdma_data_addr2 			= chroma_addr; //G186.07 , 2nd planner addr (crma)
			
	//pVPPDMAReg->vdma_data_addr1 			= (u32)luma_addr; //G186.06 , 1sr planner addr (luma)
	//pVPPDMAReg->vdma_data_addr2 			= (u32)chroma_addr; //G186.07 , 2nd planner addr (crma)

	return 0;
}

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
					//pVPPDMAReg->vdma_cfg 				&= (~0x00000020); //G186.02 , bist off
					pVPPDMAReg->vdma_cfg 				= 0x0000002; //G186.02 , bist off
				}
		break;
		case DRV_FROM_OSD0:
				sp_disp_info("DRV_VPP_SetColorbar from osd0 %d \n", enable);
				//TBD
		break;	
		case DRV_FROM_DDFCH:
				sp_disp_info("DRV_VPP_SetColorbar from ddfch %d \n", enable);
				pVSCLReg->vscl0_config2 			= 0x0000221f; //G187.01 , switch to ddfch
				pVPPDMAReg->vdma_gc 					= 0x00000028; //G186.01 , vppdma dis
				
				if (enable) {
					pDDFCHReg->ddfch_latch_en 			= 0x00000001; //G185.00 , fetch en
					pDDFCHReg->ddfch_mode_option 		= 0x00000000; //G185.01 , source yuv420 NV12
					pDDFCHReg->ddfch_enable 				= 0x000000d0; //G185.02 , data fetch en
					pDDFCHReg->ddfch_luma_base_addr_0 = 0x00000B6E; //G185.06 , luma addr
					pDDFCHReg->ddfch_crma_base_addr_0 = 0x00000CD6;	//G185.06 , crma addr
					//pDDFCHReg->ddfch_vdo_frame_size = 0x00000300; //G185.21 , video line pitch
					//pDDFCHReg->ddfch_vdo_crop_size 	= 0x01E002D0; //G185.20 , y size & x size
					if(enable == 1) {
						pDDFCHReg->ddfch_bist 				= 0x80801013; //G185.28 , color bar en
					}
					else if(enable == 2) {
						pDDFCHReg->ddfch_bist 				= 0x80801011; //G185.28 , color bar en
					}
					else if(enable == 3) {
						pDDFCHReg->ddfch_bist 				= 0x80801001; //G185.28 , color bar en
					}
				}
				else {
					pDDFCHReg->ddfch_bist 				= 0x80801002; //G185.28 , color bar dis
					pDDFCHReg->ddfch_enable 			= 0x000000c0; //G185.02 , data fetch dis
				}
				
		break;	
		default:
		break;
	}
}

void DRV_VPOST_dump(void)
{
	;
	#if 0
	//dump default setting
	printk(KERN_INFO "pVPOSTReg G199.00 0x%08x(%d) \n",pVPOSTReg->vpost_config,pVPOSTReg->vpost_config);
	printk(KERN_INFO "pVPOSTReg G199.01 0x%08x(%d) \n",pVPOSTReg->vpost_adj_config,pVPOSTReg->vpost_adj_config);
	printk(KERN_INFO "pVPOSTReg G199.02 0x%08x(%d) \n",pVPOSTReg->vpost_adj_src,pVPOSTReg->vpost_adj_src);
	printk(KERN_INFO "pVPOSTReg G199.03 0x%08x(%d) \n",pVPOSTReg->vpost_adj_des,pVPOSTReg->vpost_adj_des);
	printk(KERN_INFO "pVPOSTReg G199.04 0x%08x(%d) \n",pVPOSTReg->vpost_adj_slope0,pVPOSTReg->vpost_adj_slope0);
	printk(KERN_INFO "pVPOSTReg G199.05 0x%08x(%d) \n",pVPOSTReg->vpost_adj_slope1,pVPOSTReg->vpost_adj_slope1);
	printk(KERN_INFO "pVPOSTReg G199.06 0x%08x(%d) \n",pVPOSTReg->vpost_adj_slope2,pVPOSTReg->vpost_adj_slope2);
	printk(KERN_INFO "pVPOSTReg G199.07 0x%08x(%d) \n",pVPOSTReg->vpost_adj_bound,pVPOSTReg->vpost_adj_bound);
	printk(KERN_INFO "pVPOSTReg G199.08 0x%08x(%d) \n",pVPOSTReg->vpost_cspace_config,pVPOSTReg->vpost_cspace_config);
	printk(KERN_INFO "pVPOSTReg G199.09 0x%08x(%d) \n",pVPOSTReg->vpost_opif_config,pVPOSTReg->vpost_opif_config);
	printk(KERN_INFO "pVPOSTReg G199.10 0x%08x(%d) \n",pVPOSTReg->vpost_opif_bgy,pVPOSTReg->vpost_opif_bgy);
	printk(KERN_INFO "pVPOSTReg G199.11 0x%08x(%d) \n",pVPOSTReg->vpost_opif_bgyu,pVPOSTReg->vpost_opif_bgyu);
	printk(KERN_INFO "pVPOSTReg G199.12 0x%08x(%d) \n",pVPOSTReg->vpost_opif_alpha,pVPOSTReg->vpost_opif_alpha);
	printk(KERN_INFO "pVPOSTReg G199.13 0x%08x(%d) \n",pVPOSTReg->vpost_opif_msktop,pVPOSTReg->vpost_opif_msktop);
	printk(KERN_INFO "pVPOSTReg G199.14 0x%08x(%d) \n",pVPOSTReg->vpost_opif_mskbot,pVPOSTReg->vpost_opif_mskbot);
	printk(KERN_INFO "pVPOSTReg G199.15 0x%08x(%d) \n",pVPOSTReg->vpost_opif_mskleft,pVPOSTReg->vpost_opif_mskleft);
	printk(KERN_INFO "pVPOSTReg G199.16 0x%08x(%d) \n",pVPOSTReg->vpost_opif_mskright,pVPOSTReg->vpost_opif_mskright);
	printk(KERN_INFO "pVPOSTReg G199.17 0x%08x(%d) \n",pVPOSTReg->vpost0_checksum_ans,pVPOSTReg->vpost0_checksum_ans);
	printk(KERN_INFO "pVPOSTReg G199.18 0x%08x(%d) \n",pVPOSTReg->vpp_xstart,pVPOSTReg->vpp_xstart);
	printk(KERN_INFO "pVPOSTReg G199.19 0x%08x(%d) \n",pVPOSTReg->vpp_ystart,pVPOSTReg->vpp_ystart);
	//printk(KERN_INFO "pVPOSTReg G199.20 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[0],pVPOSTReg->vpost_reserved0[0]);
	//printk(KERN_INFO "pVPOSTReg G199.21 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[1],pVPOSTReg->vpost_reserved0[1]);
	//printk(KERN_INFO "pVPOSTReg G199.22 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[2],pVPOSTReg->vpost_reserved0[2]);
	//printk(KERN_INFO "pVPOSTReg G199.23 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[3],pVPOSTReg->vpost_reserved0[3]);
	//printk(KERN_INFO "pVPOSTReg G199.24 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[4],pVPOSTReg->vpost_reserved0[4]);
	//printk(KERN_INFO "pVPOSTReg G199.25 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[5],pVPOSTReg->vpost_reserved0[5]);
	//printk(KERN_INFO "pVPOSTReg G199.26 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[6],pVPOSTReg->vpost_reserved0[6]);
	//printk(KERN_INFO "pVPOSTReg G199.27 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[7],pVPOSTReg->vpost_reserved0[7]);
	//printk(KERN_INFO "pVPOSTReg G199.28 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[8],pVPOSTReg->vpost_reserved0[8]);
	//printk(KERN_INFO "pVPOSTReg G199.29 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[9],pVPOSTReg->vpost_reserved0[9]);
	//printk(KERN_INFO "pVPOSTReg G199.30 0x%08x(%d) \n",pVPOSTReg->vpost_reserved0[10],pVPOSTReg->vpost_reserved0[10]);
	printk(KERN_INFO "pVPOSTReg G199.31 0x%08x(%d) \n",pVPOSTReg->VPOST_Version_ID,pVPOSTReg->VPOST_Version_ID);
	#endif
	
}
void DRV_VSCL_dump(void)
{
	//dump after setting
	printk(KERN_INFO "pVSCLReg G187.00 0x%08x(%d) \n",pVSCLReg->vscl0_config1,pVSCLReg->vscl0_config1);
	printk(KERN_INFO "pVSCLReg G187.01 0x%08x(%d) \n",pVSCLReg->vscl0_config2,pVSCLReg->vscl0_config2);
	//printk(KERN_INFO "pVSCLReg G187.02 0x%08x(%d) \n",pVSCLReg->g187_reserved0,pVSCLReg->g187_reserved0);
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
	//printk(KERN_INFO "pVSCLReg G187.17 0x%08x(%d) \n",pVSCLReg->g187_reserved1,pVSCLReg->g187_reserved1);
	#if 1
	printk(KERN_INFO "pVSCLReg G187.18 0x%08x(%d) \n",pVSCLReg->vscl0_hint_ctrl,pVSCLReg->vscl0_hint_ctrl);
	printk(KERN_INFO "pVSCLReg G187.19 0x%08x(%d) \n",pVSCLReg->vscl0_hint_hfactor_low,pVSCLReg->vscl0_hint_hfactor_low);
	printk(KERN_INFO "pVSCLReg G187.20 0x%08x(%d) \n",pVSCLReg->vscl0_hint_hfactor_high,pVSCLReg->vscl0_hint_hfactor_high);
	printk(KERN_INFO "pVSCLReg G187.21 0x%08x(%d) \n",pVSCLReg->vscl0_hint_initf_low,pVSCLReg->vscl0_hint_initf_low);
	printk(KERN_INFO "pVSCLReg G187.22 0x%08x(%d) \n",pVSCLReg->vscl0_hint_initf_high,pVSCLReg->vscl0_hint_initf_high);
	//printk(KERN_INFO "pVSCLReg G187.23 0x%08x(%d) \n",pVSCLReg->g187_reserved2[0],pVSCLReg->g187_reserved2[0]);
	//printk(KERN_INFO "pVSCLReg G187.24 0x%08x(%d) \n",pVSCLReg->g187_reserved2[1],pVSCLReg->g187_reserved2[1]);
	//printk(KERN_INFO "pVSCLReg G187.25 0x%08x(%d) \n",pVSCLReg->g187_reserved2[2],pVSCLReg->g187_reserved2[2]);
	//printk(KERN_INFO "pVSCLReg G187.26 0x%08x(%d) \n",pVSCLReg->g187_reserved2[3],pVSCLReg->g187_reserved2[3]);
	//printk(KERN_INFO "pVSCLReg G187.27 0x%08x(%d) \n",pVSCLReg->g187_reserved2[4],pVSCLReg->g187_reserved2[4]);
	
	//printk(KERN_INFO "pVSCLReg G187.28 0x%08x(%d) \n",pVSCLReg->vscl0_sram_ctrl,pVSCLReg->vscl0_sram_ctrl);
	//printk(KERN_INFO "pVSCLReg G187.29 0x%08x(%d) \n",pVSCLReg->vscl0_sram_addr,pVSCLReg->vscl0_sram_addr);
	//printk(KERN_INFO "pVSCLReg G187.30 0x%08x(%d) \n",pVSCLReg->vscl0_sram_write_data,pVSCLReg->vscl0_sram_write_data);
	//printk(KERN_INFO "pVSCLReg G187.31 0x%08x(%d) \n",pVSCLReg->vscl0_sram_read_data,pVSCLReg->vscl0_sram_read_data);

	printk(KERN_INFO "pVSCLReg G188.00 0x%08x(%d) \n",pVSCLReg->vscl0_vint_ctrl,pVSCLReg->vscl0_vint_ctrl);
	printk(KERN_INFO "pVSCLReg G188.01 0x%08x(%d) \n",pVSCLReg->vscl0_vint_vfactor_low,pVSCLReg->vscl0_vint_vfactor_low);
	printk(KERN_INFO "pVSCLReg G188.02 0x%08x(%d) \n",pVSCLReg->vscl0_vint_vfactor_high,pVSCLReg->vscl0_vint_vfactor_high);
	printk(KERN_INFO "pVSCLReg G188.03 0x%08x(%d) \n",pVSCLReg->vscl0_vint_initf_low,pVSCLReg->vscl0_vint_initf_low);
	printk(KERN_INFO "pVSCLReg G188.04 0x%08x(%d) \n",pVSCLReg->vscl0_vint_initf_high,pVSCLReg->vscl0_vint_initf_high);
	//printk(KERN_INFO "pVSCLReg G188.05 0x%08x(%d) \n",pVSCLReg->g188_reserved0[0],pVSCLReg->g188_reserved0[0]);
	//printk(KERN_INFO "pVSCLReg G188.06 0x%08x(%d) \n",pVSCLReg->g188_reserved0[1],pVSCLReg->g188_reserved0[1]);
	//printk(KERN_INFO "pVSCLReg G188.07 0x%08x(%d) \n",pVSCLReg->g188_reserved0[2],pVSCLReg->g188_reserved0[2]);
	//printk(KERN_INFO "pVSCLReg G188.08 0x%08x(%d) \n",pVSCLReg->g188_reserved0[3],pVSCLReg->g188_reserved0[3]);
	//printk(KERN_INFO "pVSCLReg G188.09 0x%08x(%d) \n",pVSCLReg->g188_reserved0[4],pVSCLReg->g188_reserved0[4]);
	//printk(KERN_INFO "pVSCLReg G188.10 0x%08x(%d) \n",pVSCLReg->g188_reserved0[5],pVSCLReg->g188_reserved0[5]);
	//printk(KERN_INFO "pVSCLReg G188.11 0x%08x(%d) \n",pVSCLReg->g188_reserved0[6],pVSCLReg->g188_reserved0[6]);
	//printk(KERN_INFO "pVSCLReg G188.12 0x%08x(%d) \n",pVSCLReg->g188_reserved0[7],pVSCLReg->g188_reserved0[7]);
	//printk(KERN_INFO "pVSCLReg G188.13 0x%08x(%d) \n",pVSCLReg->g188_reserved0[8],pVSCLReg->g188_reserved0[8]);
	//printk(KERN_INFO "pVSCLReg G188.14 0x%08x(%d) \n",pVSCLReg->g188_reserved0[9],pVSCLReg->g188_reserved0[9]);
	//printk(KERN_INFO "pVSCLReg G188.15 0x%08x(%d) \n",pVSCLReg->g188_reserved0[10],pVSCLReg->g188_reserved0[10]);
	//printk(KERN_INFO "pVSCLReg G188.16 0x%08x(%d) \n",pVSCLReg->g188_reserved0[11],pVSCLReg->g188_reserved0[11]);
	//printk(KERN_INFO "pVSCLReg G188.17 0x%08x(%d) \n",pVSCLReg->g188_reserved0[12],pVSCLReg->g188_reserved0[12]);
	//printk(KERN_INFO "pVSCLReg G188.18 0x%08x(%d) \n",pVSCLReg->g188_reserved0[13],pVSCLReg->g188_reserved0[13]);
	//printk(KERN_INFO "pVSCLReg G188.19 0x%08x(%d) \n",pVSCLReg->g188_reserved0[14],pVSCLReg->g188_reserved0[14]);
	//printk(KERN_INFO "pVSCLReg G188.20 0x%08x(%d) \n",pVSCLReg->g188_reserved0[15],pVSCLReg->g188_reserved0[15]);
	//printk(KERN_INFO "pVSCLReg G188.21 0x%08x(%d) \n",pVSCLReg->g188_reserved0[16],pVSCLReg->g188_reserved0[16]);
	
	//printk(KERN_INFO "pVSCLReg G188.22 0x%08x(%d) \n",pVSCLReg->vscl0_checksum_result,pVSCLReg->vscl0_checksum_result);
	//printk(KERN_INFO "pVSCLReg G188.23 0x%08x(%d) \n",pVSCLReg->vscl0_Version_ID,pVSCLReg->vscl0_Version_ID);
	
	//printk(KERN_INFO "pVSCLReg G188.24 0x%08x(%d) \n",pVSCLReg->g188_reserved1[0],pVSCLReg->g188_reserved1[0]);
	//printk(KERN_INFO "pVSCLReg G188.25 0x%08x(%d) \n",pVSCLReg->g188_reserved1[1],pVSCLReg->g188_reserved1[1]);
	//printk(KERN_INFO "pVSCLReg G188.26 0x%08x(%d) \n",pVSCLReg->g188_reserved1[2],pVSCLReg->g188_reserved1[2]);
	//printk(KERN_INFO "pVSCLReg G188.27 0x%08x(%d) \n",pVSCLReg->g188_reserved1[3],pVSCLReg->g188_reserved1[3]);
	//printk(KERN_INFO "pVSCLReg G188.28 0x%08x(%d) \n",pVSCLReg->g188_reserved1[4],pVSCLReg->g188_reserved1[4]);
	//printk(KERN_INFO "pVSCLReg G188.29 0x%08x(%d) \n",pVSCLReg->g188_reserved1[5],pVSCLReg->g188_reserved1[5]);
	//printk(KERN_INFO "pVSCLReg G188.30 0x%08x(%d) \n",pVSCLReg->g188_reserved1[6],pVSCLReg->g188_reserved1[6]);
	//printk(KERN_INFO "pVSCLReg G188.31 0x%08x(%d) \n",pVSCLReg->g188_reserved1[7],pVSCLReg->g188_reserved1[7]);
	#endif
}
void DRV_VPPDMA_dump(void)
{
	#if 0
	//dump after setting
	printk(KERN_INFO "pVPPDMAReg G186.00 0x%08x(%d) \n",pVPPDMAReg->vdma_ver,pVPPDMAReg->vdma_ver);
	printk(KERN_INFO "pVPPDMAReg G186.01 0x%08x(%d) \n",pVPPDMAReg->vdma_gc,pVPPDMAReg->vdma_gc);
	printk(KERN_INFO "pVPPDMAReg G186.02 0x%08x(%d) \n",pVPPDMAReg->vdma_cfg,pVPPDMAReg->vdma_cfg);
	printk(KERN_INFO "pVPPDMAReg G186.03 0x%08x(%d) \n",pVPPDMAReg->vdma_frame_size,pVPPDMAReg->vdma_frame_size);
	printk(KERN_INFO "pVPPDMAReg G186.04 0x%08x(%d) \n",pVPPDMAReg->vdma_crop_st,pVPPDMAReg->vdma_crop_st);
	printk(KERN_INFO "pVPPDMAReg G186.05 0x%08x(%d) \n",pVPPDMAReg->vdma_lstd_size,pVPPDMAReg->vdma_lstd_size);
	printk(KERN_INFO "pVPPDMAReg G186.06 0x%08x(%d) \n",pVPPDMAReg->vdma_data_addr1,pVPPDMAReg->vdma_data_addr1);
	printk(KERN_INFO "pVPPDMAReg G186.07 0x%08x(%d) \n",pVPPDMAReg->vdma_data_addr2,pVPPDMAReg->vdma_data_addr2);
	//printk(KERN_INFO "pVPPDMAReg G186.08 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[0],pVPPDMAReg->g186_reserved[0]);
	//printk(KERN_INFO "pVPPDMAReg G186.09 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[1],pVPPDMAReg->g186_reserved[1]);
	//printk(KERN_INFO "pVPPDMAReg G186.10 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[2],pVPPDMAReg->g186_reserved[2]);
	//printk(KERN_INFO "pVPPDMAReg G186.11 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[3],pVPPDMAReg->g186_reserved[3]);
	//printk(KERN_INFO "pVPPDMAReg G186.12 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[4],pVPPDMAReg->g186_reserved[4]);
	//printk(KERN_INFO "pVPPDMAReg G186.13 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[5],pVPPDMAReg->g186_reserved[5]);
	//printk(KERN_INFO "pVPPDMAReg G186.14 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[6],pVPPDMAReg->g186_reserved[6]);
	//printk(KERN_INFO "pVPPDMAReg G186.15 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[7],pVPPDMAReg->g186_reserved[7]);
	//printk(KERN_INFO "pVPPDMAReg G186.16 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[8],pVPPDMAReg->g186_reserved[8]);
	//printk(KERN_INFO "pVPPDMAReg G186.17 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[9],pVPPDMAReg->g186_reserved[9]);
	//printk(KERN_INFO "pVPPDMAReg G186.18 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[10],pVPPDMAReg->g186_reserved[10]);
	//printk(KERN_INFO "pVPPDMAReg G186.19 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[11],pVPPDMAReg->g186_reserved[11]);
	//printk(KERN_INFO "pVPPDMAReg G186.20 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[12],pVPPDMAReg->g186_reserved[12]);
	//printk(KERN_INFO "pVPPDMAReg G186.21 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[13],pVPPDMAReg->g186_reserved[13]);
	//printk(KERN_INFO "pVPPDMAReg G186.22 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[14],pVPPDMAReg->g186_reserved[14]);
	//printk(KERN_INFO "pVPPDMAReg G186.23 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[15],pVPPDMAReg->g186_reserved[15]);
	//printk(KERN_INFO "pVPPDMAReg G186.24 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[16],pVPPDMAReg->g186_reserved[16]);
	//printk(KERN_INFO "pVPPDMAReg G186.25 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[17],pVPPDMAReg->g186_reserved[17]);
	//printk(KERN_INFO "pVPPDMAReg G186.26 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[18],pVPPDMAReg->g186_reserved[18]);
	//printk(KERN_INFO "pVPPDMAReg G186.27 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[19],pVPPDMAReg->g186_reserved[19]);
	//printk(KERN_INFO "pVPPDMAReg G186.28 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[20],pVPPDMAReg->g186_reserved[20]);
	//printk(KERN_INFO "pVPPDMAReg G186.29 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[21],pVPPDMAReg->g186_reserved[21]);
	//printk(KERN_INFO "pVPPDMAReg G186.30 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[22],pVPPDMAReg->g186_reserved[22]);
	//printk(KERN_INFO "pVPPDMAReg G186.31 0x%08x(%d) \n",pVPPDMAReg->g186_reserved[23],pVPPDMAReg->g186_reserved[23]);
	#endif
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
	#if 0
	//dump after setting
	printk(KERN_INFO "pDDFCHReg G185.00 0x%08x(%d) \n",pDDFCHReg->ddfch_latch_en,pDDFCHReg->ddfch_latch_en);
	printk(KERN_INFO "pDDFCHReg G185.01 0x%08x(%d) \n",pDDFCHReg->ddfch_mode_option,pDDFCHReg->ddfch_mode_option);
	printk(KERN_INFO "pDDFCHReg G185.02 0x%08x(%d) \n",pDDFCHReg->ddfch_enable,pDDFCHReg->ddfch_enable);
	printk(KERN_INFO "pDDFCHReg G185.03 0x%08x(%d) \n",pDDFCHReg->ddfch_urgent_thd,pDDFCHReg->ddfch_urgent_thd);
	printk(KERN_INFO "pDDFCHReg G185.04 0x%08x(%d) \n",pDDFCHReg->ddfch_cmdq_thd,pDDFCHReg->ddfch_cmdq_thd);
	printk(KERN_INFO "pDDFCHReg G185.05 0x%08x(%d) \n",pDDFCHReg->g185_reserved0,pDDFCHReg->g185_reserved0);
	printk(KERN_INFO "pDDFCHReg G185.06 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_0,pDDFCHReg->ddfch_luma_base_addr_0);
	printk(KERN_INFO "pDDFCHReg G185.07 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_1,pDDFCHReg->ddfch_luma_base_addr_1);
	printk(KERN_INFO "pDDFCHReg G185.08 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_2,pDDFCHReg->ddfch_luma_base_addr_2);
	printk(KERN_INFO "pDDFCHReg G185.09 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_0,pDDFCHReg->ddfch_crma_base_addr_0);
	printk(KERN_INFO "pDDFCHReg G185.10 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_1,pDDFCHReg->ddfch_crma_base_addr_1);
	printk(KERN_INFO "pDDFCHReg G185.11 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_2,pDDFCHReg->ddfch_crma_base_addr_2);
	printk(KERN_INFO "pDDFCHReg G185.12 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[0],pDDFCHReg->g185_reserved1[0]);
	printk(KERN_INFO "pDDFCHReg G185.13 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[1],pDDFCHReg->g185_reserved1[1]);
	printk(KERN_INFO "pDDFCHReg G185.14 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[2],pDDFCHReg->g185_reserved1[2]);
	printk(KERN_INFO "pDDFCHReg G185.15 0x%08x(%d) \n",pDDFCHReg->ddfch_frame_id,pDDFCHReg->ddfch_frame_id);
	printk(KERN_INFO "pDDFCHReg G185.16 0x%08x(%d) \n",pDDFCHReg->ddfch_free_run_control,pDDFCHReg->ddfch_free_run_control);
	printk(KERN_INFO "pDDFCHReg G185.17 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[0],pDDFCHReg->g185_reserved2[0]);
	printk(KERN_INFO "pDDFCHReg G185.18 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[1],pDDFCHReg->g185_reserved2[1]);
	printk(KERN_INFO "pDDFCHReg G185.19 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[2],pDDFCHReg->g185_reserved2[2]);
	printk(KERN_INFO "pDDFCHReg G185.20 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_frame_size,pDDFCHReg->ddfch_vdo_frame_size);
	printk(KERN_INFO "pDDFCHReg G185.21 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_size,pDDFCHReg->ddfch_vdo_crop_size);
	printk(KERN_INFO "pDDFCHReg G185.22 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_offset,pDDFCHReg->ddfch_vdo_crop_offset);
	printk(KERN_INFO "pDDFCHReg G185.23 0x%08x(%d) \n",pDDFCHReg->ddfch_config_0,pDDFCHReg->ddfch_config_0);
	printk(KERN_INFO "pDDFCHReg G185.24 0x%08x(%d) \n",pDDFCHReg->ddfch_config_1,pDDFCHReg->ddfch_config_1);
	printk(KERN_INFO "pDDFCHReg G185.25 0x%08x(%d) \n",pDDFCHReg->g185_reserved3,pDDFCHReg->g185_reserved3);
	printk(KERN_INFO "pDDFCHReg G185.26 0x%08x(%d) \n",pDDFCHReg->ddfch_checksum_info,pDDFCHReg->ddfch_checksum_info);
	printk(KERN_INFO "pDDFCHReg G185.27 0x%08x(%d) \n",pDDFCHReg->ddfch_error_flag_info,pDDFCHReg->ddfch_error_flag_info);
	printk(KERN_INFO "pDDFCHReg G185.28 0x%08x(%d) \n",pDDFCHReg->ddfch_bist,pDDFCHReg->ddfch_bist);
	printk(KERN_INFO "pDDFCHReg G185.29 0x%08x(%d) \n",pDDFCHReg->ddfch_axi_ipbj_info,pDDFCHReg->ddfch_axi_ipbj_info);
	printk(KERN_INFO "pDDFCHReg G185.30 0x%08x(%d) \n",pDDFCHReg->g185_reserved4,pDDFCHReg->g185_reserved4);
	printk(KERN_INFO "pDDFCHReg G185.31 0x%08x(%d) \n",pDDFCHReg->ddfch_others_info,pDDFCHReg->ddfch_others_info);
	#endif
	//dump after setting
	printk(KERN_INFO "pDDFCHReg G185.00 0x%08x(%d) \n",pDDFCHReg->ddfch_latch_en,pDDFCHReg->ddfch_latch_en);
	printk(KERN_INFO "pDDFCHReg G185.01 0x%08x(%d) \n",pDDFCHReg->ddfch_mode_option,pDDFCHReg->ddfch_mode_option);
	printk(KERN_INFO "pDDFCHReg G185.02 0x%08x(%d) \n",pDDFCHReg->ddfch_enable,pDDFCHReg->ddfch_enable);
	printk(KERN_INFO "pDDFCHReg G185.03 0x%08x(%d) \n",pDDFCHReg->ddfch_urgent_thd,pDDFCHReg->ddfch_urgent_thd);
	printk(KERN_INFO "pDDFCHReg G185.04 0x%08x(%d) \n",pDDFCHReg->ddfch_cmdq_thd,pDDFCHReg->ddfch_cmdq_thd);
	printk(KERN_INFO "pDDFCHReg G185.05 0x%08x(%d) \n",pDDFCHReg->g185_reserved0,pDDFCHReg->g185_reserved0);
	printk(KERN_INFO "pDDFCHReg G185.06 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_0,pDDFCHReg->ddfch_luma_base_addr_0);
	//printk(KERN_INFO "pDDFCHReg G185.07 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_1,pDDFCHReg->ddfch_luma_base_addr_1);
	//printk(KERN_INFO "pDDFCHReg G185.08 0x%08x(%d) \n",pDDFCHReg->ddfch_luma_base_addr_2,pDDFCHReg->ddfch_luma_base_addr_2);
	printk(KERN_INFO "pDDFCHReg G185.09 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_0,pDDFCHReg->ddfch_crma_base_addr_0);
	//printk(KERN_INFO "pDDFCHReg G185.10 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_1,pDDFCHReg->ddfch_crma_base_addr_1);
	//printk(KERN_INFO "pDDFCHReg G185.11 0x%08x(%d) \n",pDDFCHReg->ddfch_crma_base_addr_2,pDDFCHReg->ddfch_crma_base_addr_2);
	//printk(KERN_INFO "pDDFCHReg G185.12 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[0],pDDFCHReg->g185_reserved1[0]);
	//printk(KERN_INFO "pDDFCHReg G185.13 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[1],pDDFCHReg->g185_reserved1[1]);
	//printk(KERN_INFO "pDDFCHReg G185.14 0x%08x(%d) \n",pDDFCHReg->g185_reserved1[2],pDDFCHReg->g185_reserved1[2]);
	printk(KERN_INFO "pDDFCHReg G185.15 0x%08x(%d) \n",pDDFCHReg->ddfch_frame_id,pDDFCHReg->ddfch_frame_id);
	printk(KERN_INFO "pDDFCHReg G185.16 0x%08x(%d) \n",pDDFCHReg->ddfch_free_run_control,pDDFCHReg->ddfch_free_run_control);
	//printk(KERN_INFO "pDDFCHReg G185.17 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[0],pDDFCHReg->g185_reserved2[0]);
	//printk(KERN_INFO "pDDFCHReg G185.18 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[1],pDDFCHReg->g185_reserved2[1]);
	//printk(KERN_INFO "pDDFCHReg G185.19 0x%08x(%d) \n",pDDFCHReg->g185_reserved2[2],pDDFCHReg->g185_reserved2[2]);
	printk(KERN_INFO "pDDFCHReg G185.20 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_frame_size,pDDFCHReg->ddfch_vdo_frame_size);
	printk(KERN_INFO "pDDFCHReg G185.21 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_size,pDDFCHReg->ddfch_vdo_crop_size);
	printk(KERN_INFO "pDDFCHReg G185.22 0x%08x(%d) \n",pDDFCHReg->ddfch_vdo_crop_offset,pDDFCHReg->ddfch_vdo_crop_offset);
	printk(KERN_INFO "pDDFCHReg G185.23 0x%08x(%d) \n",pDDFCHReg->ddfch_config_0,pDDFCHReg->ddfch_config_0);
	printk(KERN_INFO "pDDFCHReg G185.24 0x%08x(%d) \n",pDDFCHReg->ddfch_config_1,pDDFCHReg->ddfch_config_1);
	//printk(KERN_INFO "pDDFCHReg G185.25 0x%08x(%d) \n",pDDFCHReg->g185_reserved3,pDDFCHReg->g185_reserved3);
	//printk(KERN_INFO "pDDFCHReg G185.26 0x%08x(%d) \n",pDDFCHReg->ddfch_checksum_info,pDDFCHReg->ddfch_checksum_info);
	//printk(KERN_INFO "pDDFCHReg G185.27 0x%08x(%d) \n",pDDFCHReg->ddfch_error_flag_info,pDDFCHReg->ddfch_error_flag_info);
	printk(KERN_INFO "pDDFCHReg G185.28 0x%08x(%d) \n",pDDFCHReg->ddfch_bist,pDDFCHReg->ddfch_bist);
	//printk(KERN_INFO "pDDFCHReg G185.29 0x%08x(%d) \n",pDDFCHReg->ddfch_axi_ipbj_info,pDDFCHReg->ddfch_axi_ipbj_info);
	//printk(KERN_INFO "pDDFCHReg G185.30 0x%08x(%d) \n",pDDFCHReg->g185_reserved4,pDDFCHReg->g185_reserved4);
	//printk(KERN_INFO "pDDFCHReg G185.31 0x%08x(%d) \n",pDDFCHReg->ddfch_others_info,pDDFCHReg->ddfch_others_info);
		
}
