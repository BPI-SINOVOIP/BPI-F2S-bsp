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
 * @file disp_dmix.c
 * @brief
 * @author Hammer Hsieh
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_dmix.h"
#include "disp_tgen.h"

#include <linux/kernel.h>

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
static DISP_DMIX_REG_t *pDMIXReg;

#if 1 //def SP_DISP_DEBUG
	static const char * const LayerNameStr[] = {"BG", "L1", "L2", "L3", "L4", "L5", "L6"};
	static const char * const LayerModeStr[] = {"AlphaBlend", "Transparent", "Opacity"};
	static const char * const SelStr[] = {"VPP0", "VPP1", "VPP2", "OSD0", "OSD1", "OSD2", "OSD3", "PTG"};
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DMIX_Init(void *pInReg)
{
	pDMIXReg = (DISP_DMIX_REG_t *)pInReg;

	sp_disp_info("DRV_DMIX_Init \n");
	
	#if 0 //for sp7021
	pDMIXReg->dmix_pix_en_sel = 0x0006;

	// BG: pattern gen
	// L1: VPP0
	// L6: OSD0
	pDMIXReg->dmix_config0 = 0x0070;
	pDMIXReg->dmix_config1 = 0x8156;
	pDMIXReg->dmix_config2 = 0x3000;

	//DMIX PTG(default border pixel len=0 (BGC))
	pDMIXReg->dmix_ptg_config = 0x2000;
	//default BGC color
	pDMIXReg->dmix_ptg_config_4 = (0<<8) | (0x10 & 0xff);
	pDMIXReg->dmix_ptg_config_5 = (0<<8) | (0x80 & 0xff);
	pDMIXReg->dmix_ptg_config_6 = (0<<8) | (0x80 & 0xff);

	//DMIX YC adjust
	pDMIXReg->dmix_yc_adjust = 0x0;
	#endif
	
	//for i143
	// BG: pattern gen
	// L1: VPP0
	// L5: OSD1
	// L6: OSD0
	#ifdef DISP_64X64
	//64x64 setting	
	pDMIXReg->dmix_layer_config_0 = 0x34561070; // 00
	pDMIXReg->dmix_layer_config_1 = 0x00000000; // 01
	pDMIXReg->dmix_ptg_config_0 = 0x00002001; // 09
	pDMIXReg->dmix_ptg_config_2 = 0x0029f06e; // 11
	//pDMIXReg->dmix_ptg_config_2 = 0x00f0f0f0; // 11
	pDMIXReg->dmix_source_select = 0x00000006; // 20
	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting	
	pDMIXReg->dmix_layer_config_0 = 0x34561070; // 00
	pDMIXReg->dmix_layer_config_1 = 0x00000000; // 01 all layer blending
	//pDMIXReg->dmix_layer_config_1 = 0x00000002; // 01 show L1 - VPP0 , block Background 
	//pDMIXReg->dmix_layer_config_1 = 0x00000556; // 01 hide L6L5L4L3L2 show L1 - VPP0 , block Background 
	pDMIXReg->dmix_ptg_config_0 = 0x00002001; // 09
	//pDMIXReg->dmix_ptg_config_0 = 0x00000001; // 09 dmix color bar H
	//pDMIXReg->dmix_ptg_config_0 = 0x00008001; // 09 dmix color bar V
	//pDMIXReg->dmix_ptg_config_0 = 0x00004001; // 09 dmix color bar snow
	pDMIXReg->dmix_ptg_config_2 = 0x0029f06e; // 11
	//pDMIXReg->dmix_ptg_config_2 = 0x00f0f0f0; // 11
	pDMIXReg->dmix_source_select = 0x00000006; // 20	
	#endif

	#ifdef DISP_576P
	//576P 720x576 setting	
	pDMIXReg->dmix_layer_config_0 = 0x34561070; // 00
	pDMIXReg->dmix_layer_config_1 = 0x00000000; // 01 all layer blending
	pDMIXReg->dmix_ptg_config_0 = 0x00002001; // 09
	pDMIXReg->dmix_ptg_config_2 = 0x0029f06e; // 11
	pDMIXReg->dmix_source_select = 0x00000006; // 20	
	#endif
	
	#ifdef DISP_720P
	//720P 1280x720 setting	
	pDMIXReg->dmix_layer_config_0 = 0x34561070; // 00
	pDMIXReg->dmix_layer_config_1 = 0x00000000; // 01 all layer blending
	pDMIXReg->dmix_ptg_config_0 = 0x00002001; // 09
	pDMIXReg->dmix_ptg_config_2 = 0x0029f06e; // 11
	pDMIXReg->dmix_source_select = 0x00000006; // 20	
	#endif
	
	#ifdef DISP_1080P
	//1080P 1920x1080 setting	
	pDMIXReg->dmix_layer_config_0 = 0x34561070; // 00
	pDMIXReg->dmix_layer_config_1 = 0x00000000; // 01 all layer blending
	pDMIXReg->dmix_ptg_config_0 = 0x00002001; // 09
	pDMIXReg->dmix_ptg_config_2 = 0x0029f06e; // 11
	pDMIXReg->dmix_source_select = 0x00000006; // 20	
	#endif
	
}

DRV_Status_e DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_e tpg_sel, int bg_color_yuv, int border_len)
{
	int dmix_mode = pDMIXReg->dmix_ptg_config_0 & (~0xf000);
	
	if (tpg_sel >= DRV_DMIX_TPG_MAX)
		return DRV_ERR_INVALID_PARAM;

	switch (tpg_sel) {
	default:
	case DRV_DMIX_TPG_MAX:
	case DRV_DMIX_TPG_BGC:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (2 << 12);
		DRV_DMIX_PTG_Color_Set_YCbCr(ENABLE, (bg_color_yuv>>16) & 0xff, (bg_color_yuv>>8) & 0xff, (bg_color_yuv>>0) & 0xff);
		break;
	case DRV_DMIX_TPG_V_COLORBAR:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (0 << 12);
		break;
	case DRV_DMIX_TPG_H_COLORBAR:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (1 << 15) | (0 << 14) | (0 << 12);
		break;
	case DRV_DMIX_TPG_BORDER:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (1 << 12);
		DRV_DMIX_PTG_Color_Set_YCbCr(DISABLE, 0, 0, 0);
		break;
	case DRV_DMIX_TPG_REGION:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (2 << 12);
		//DRV_DMIX_PTG_Color_Set_YCbCr(DISABLE, 0, 0, 0);
		break;
	case DRV_DMIX_TPG_SNOW:
		pDMIXReg->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (1 << 14) | (0 << 12);
		//DRV_DMIX_PTG_Color_Set_YCbCr(DISABLE, 0, 0, 0);
		break;
	}

	return DRV_SUCCESS;
}

void DRV_DMIX_PTG_Color_Set(UINT32 color)
{
	UINT16 Y, Cb, Cr;
	UINT16 R, G, B;

	R = (color >> 16) & 0xff;
	G = (color >> 8) & 0xff;
	B = color & 0xff;

	Y = (R * 76 / 255) + (G * 150 / 255) + (B * 29 / 255);
	Cb = -(R * 43 / 255) - (G * 85 / 255) + (B * 128 / 255) + 128;
	Cr = (R * 128 / 255) - (G * 107 / 255) - (B * 21 / 255) + 128;

	if (Cb > 255)
		Cb = 255;
	if (Cr > 255)
		Cr = 255;

	DRV_DMIX_PTG_Color_Set_YCbCr(ENABLE, (Y & 0xff), (Cb & 0xff), (Cr & 0xff));
}

void DRV_DMIX_PTG_Color_Set_YCbCr(UINT8 enable, UINT8 Y, UINT8 Cb, UINT8 Cr)
{
	if (enable) {
		pDMIXReg->dmix_ptg_config_2 = (1 << 26) | (1 << 25) | (1 << 24) | \
																	((Y & 0xff)<<16) | ((Cb & 0xff)<<8) | ((Cr & 0xff)<<0);
		//pDMIXReg->dmix_ptg_config_4 = (1 << 8) | (Y & 0xff);
		//pDMIXReg->dmix_ptg_config_5 = (1 << 8) | (Cb & 0xff);
		//pDMIXReg->dmix_ptg_config_6 = (1 << 8) | (Cr & 0xff);
	} else {
		pDMIXReg->dmix_ptg_config_2 = (0 << 26) | (0 << 25) | (0 << 24) | \
																	((Y & 0xff)<<16) | ((Cb & 0xff)<<8) | ((Cr & 0xff)<<0);		
		//pDMIXReg->dmix_ptg_config_4 &= 0xff;
		//pDMIXReg->dmix_ptg_config_5 &= 0xff;
		//pDMIXReg->dmix_ptg_config_6 &= 0xff;
	}
}

DRV_Status_e DRV_DMIX_Layer_Init(DRV_DMIX_LayerId_e Layer, DRV_DMIX_LayerMode_e LayerMode, DRV_DMIX_InputSel_e FG_Sel)
{
	UINT32 tmp, tmp1;
	DRV_TGEN_Input_e input;

	if ((((int)Layer >= DRV_DMIX_L2) && ((int)Layer <= DRV_DMIX_L5)) ||
			((int)LayerMode < DRV_DMIX_AlphaBlend) || ((int)LayerMode > DRV_DMIX_Opacity) ||
			(((int)FG_Sel != DRV_DMIX_VPP0) && ((int)FG_Sel != DRV_DMIX_OSD0) && ((int)FG_Sel != DRV_DMIX_PTG))) {
		//sp_disp_err("Layer %d, LayerMode %d, InSel %d\n", Layer, LayerMode, FG_Sel);
		return DRV_ERR_INVALID_PARAM;
	}

	sp_disp_dbg("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[Layer], LayerModeStr[LayerMode], SelStr[FG_Sel]);

	tmp  = pDMIXReg->dmix_layer_config_0;
	tmp1 = pDMIXReg->dmix_layer_config_1;

	//Set layer mode
	if (Layer != DRV_DMIX_BG) {
		//Clear layer mode bit
		tmp1 &= ~(0x3 << ((Layer - 1) << 1));
		tmp1 |= (LayerMode << ((Layer - 1) << 1));
	}

	tmp = (tmp & ~(0X7 << ((Layer * 4) + 4))) | (FG_Sel << ((Layer * 4) + 4));

	//Finish set amix layer information
	pDMIXReg->dmix_layer_config_0 = tmp;
	pDMIXReg->dmix_layer_config_1 = tmp1;
	//pDMIXReg->dmix_config2 = tmp2;

	//{
		//DRV_TGEN_Input_e input;

		switch (FG_Sel) {
		case DRV_DMIX_VPP0:
			input = DRV_TGEN_VPP0;
			break;
		case DRV_DMIX_OSD0:
			input = DRV_TGEN_OSD0;
			break;
		case DRV_DMIX_OSD1:
			input = DRV_TGEN_OSD1;
			break;			
		case DRV_DMIX_PTG:
			input = DRV_TGEN_PTG;
			break;
		default:
			input = DRV_TGEN_ALL;
			break;
		}
		if (input != DRV_TGEN_ALL) {
			if (input == DRV_TGEN_PTG)
				DRV_TGEN_Adjust(input, 0x10);
			else
				DRV_TGEN_Adjust(input, 0x10 - ((Layer - DRV_DMIX_L1) << 1));
		}
	//}

	return DRV_SUCCESS;
}

DRV_Status_e DRV_DMIX_Layer_Set(DRV_DMIX_LayerMode_e LayerMode, DRV_DMIX_InputSel_e FG_Sel)
{
	UINT32 tmp;
	DRV_DMIX_LayerId_e Layer;

	if (((int)LayerMode < DRV_DMIX_AlphaBlend) || ((int)LayerMode > DRV_DMIX_Opacity) ||
			(((int)FG_Sel != DRV_DMIX_VPP0) && ((int)FG_Sel != DRV_DMIX_OSD1) && ((int)FG_Sel != DRV_DMIX_OSD0) && ((int)FG_Sel != DRV_DMIX_PTG))) {
		//sp_disp_err("Layer %d, LayerMode %d, InSel %d\n", Layer, LayerMode, FG_Sel);
		return DRV_ERR_INVALID_PARAM;
	}

	switch (FG_Sel) {
	case DRV_DMIX_VPP0:
		Layer = DRV_DMIX_L1;
		break;
	case DRV_DMIX_OSD1:
		Layer = DRV_DMIX_L5;
		break;		
	case DRV_DMIX_OSD0:
		Layer = DRV_DMIX_L6;
		break;
	case DRV_DMIX_PTG:
		Layer = DRV_DMIX_BG;
		break;
	default:
		goto ERROR;
	}
	sp_disp_dbg("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[Layer], LayerModeStr[LayerMode], SelStr[FG_Sel]);

	tmp = pDMIXReg->dmix_layer_config_1;

	//Set layer mode
	if (Layer != DRV_DMIX_BG) {
		tmp &= ~(0x3 << ((Layer - 1) << 1));
		tmp |= (LayerMode << ((Layer - 1) << 1));
	}

	//Finish set amix layer information
	pDMIXReg->dmix_layer_config_1 = tmp;

ERROR:
	return DRV_SUCCESS;
}

void DRV_DMIX_Layer_Mode_Set(DRV_DMIX_LayerId_e Layer, DRV_DMIX_LayerMode_e LayerMode)
{
	UINT32 tmp;

	tmp = pDMIXReg->dmix_layer_config_1;
	//Set layer mode
	if (Layer != DRV_DMIX_BG) {
		tmp &= ~(0x3 << ((Layer - 1) << 1));
		tmp |= (LayerMode << ((Layer - 1) << 1));
	}
	//Finish set amix layer information
	pDMIXReg->dmix_layer_config_1 = tmp;

}

void DRV_DMIX_Layer_Get(DRV_DMIX_Layer_Set_t *pLayerInfo)
{
	UINT32 tmp, tmp1, test;

	tmp  = pDMIXReg->dmix_layer_config_0;
	tmp1 = pDMIXReg->dmix_layer_config_1;

	pLayerInfo->LayerMode = (tmp1 >> ((pLayerInfo->Layer - 1) << 1) & 0x3);

	test = (tmp >> ((pLayerInfo->Layer * 4) + 4)) & 0x7;

	pLayerInfo->FG_Sel = test;

	sp_disp_dbg("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[pLayerInfo->Layer], LayerModeStr[pLayerInfo->LayerMode], SelStr[pLayerInfo->FG_Sel]);
}

void DRV_DMIX_Layer_Info(void)
{
	UINT32 tmp0, tmp1, test, LayerMode, Layer, FG_Sel;

	tmp0  = pDMIXReg->dmix_layer_config_0;
	tmp1 = pDMIXReg->dmix_layer_config_1;
	
	for (Layer = 1; Layer <= 6 ; Layer++) {
		LayerMode = (tmp1 >> ((Layer - 1) << 1) & 0x3);
		test = (tmp0 >> ((Layer * 4) + 4)) & 0x7;
		FG_Sel = test;
		sp_disp_info("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[Layer], LayerModeStr[LayerMode], SelStr[FG_Sel]);
	}
}

DRV_Status_e DRV_DMIX_Plane_Alpha_Set(DRV_DMIX_PlaneAlpha_t *PlaneAlphaInfo)
{
	UINT32 tmp1;
	UINT32 tmp2;
	int ret = DRV_SUCCESS;

	sp_disp_info("Layer%d: En%d, Fix%d, 0x%x\n", PlaneAlphaInfo->Layer, PlaneAlphaInfo->EnPlaneAlpha, PlaneAlphaInfo->EnFixAlpha, PlaneAlphaInfo->AlphaValue);

	tmp1 = pDMIXReg->dmix_plane_alpha_config_0;
	tmp2 = pDMIXReg->dmix_plane_alpha_config_1;

	switch (PlaneAlphaInfo->Layer) {
	case DRV_DMIX_L1:
		tmp1 = (tmp1 & ~0xFF00) |
				((PlaneAlphaInfo->EnPlaneAlpha & 0x1) << 15) |
				((PlaneAlphaInfo->EnFixAlpha & 0x1) << 14) |
				((PlaneAlphaInfo->AlphaValue & 0x3F) << 8);
		break;
	case DRV_DMIX_L5:
		tmp2 = (tmp2 & ~0xFF00) |
				((PlaneAlphaInfo->EnPlaneAlpha & 0x1) << 15) |
				((PlaneAlphaInfo->EnFixAlpha & 0x1) << 14) |
				((PlaneAlphaInfo->AlphaValue & 0x3F) << 8);
		break;		
	case DRV_DMIX_L6:
		tmp2 = (tmp2 & ~0x00FF) |
				((PlaneAlphaInfo->EnPlaneAlpha & 0x1) << 7) |
				((PlaneAlphaInfo->EnFixAlpha & 0x1) << 6) |
				(PlaneAlphaInfo->AlphaValue & 0x3F);
		break;
	default:
		sp_disp_err("Invalid Layer %d\n", PlaneAlphaInfo->Layer);
		ret = DRV_ERR_INVALID_PARAM;
		break;
	}

	pDMIXReg->dmix_plane_alpha_config_0 = tmp1;
	pDMIXReg->dmix_plane_alpha_config_1 = tmp2;

	return ret;
}

void DRV_DMIX_PQ_OnOff(int OutId, int enable)
{
	if (enable)
		//pDMIXReg->dmix_yc_adjust |= 0x3;
		pDMIXReg->dmix_adjust_config_0 = (1<<17) | (1<<16);
	else
		//pDMIXReg->dmix_yc_adjust = 0;
		pDMIXReg->dmix_adjust_config_0 = (0<<17) | (0<<16);
}

void DRV_DMIX_dump(void)
{
	#if 0
	//dump after setting
	printk(KERN_INFO "pDMIXReg G217.00 0x%08x(%d) \n",pDMIXReg->dmix_layer_config_0,pDMIXReg->dmix_layer_config_0);
	printk(KERN_INFO "pDMIXReg G217.01 0x%08x(%d) \n",pDMIXReg->dmix_layer_config_1,pDMIXReg->dmix_layer_config_1);
	printk(KERN_INFO "pDMIXReg G217.02 0x%08x(%d) \n",pDMIXReg->dmix_plane_alpha_config_0,pDMIXReg->dmix_plane_alpha_config_0);
	printk(KERN_INFO "pDMIXReg G217.03 0x%08x(%d) \n",pDMIXReg->dmix_plane_alpha_config_1,pDMIXReg->dmix_plane_alpha_config_1);
	printk(KERN_INFO "pDMIXReg G217.04 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_0,pDMIXReg->dmix_adjust_config_0);
	printk(KERN_INFO "pDMIXReg G217.05 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_1,pDMIXReg->dmix_adjust_config_1);
	printk(KERN_INFO "pDMIXReg G217.06 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_2,pDMIXReg->dmix_adjust_config_2);
	printk(KERN_INFO "pDMIXReg G217.07 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_3,pDMIXReg->dmix_adjust_config_3);
	printk(KERN_INFO "pDMIXReg G217.08 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_4,pDMIXReg->dmix_adjust_config_4);
	printk(KERN_INFO "pDMIXReg G217.09 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_0,pDMIXReg->dmix_ptg_config_0);
	printk(KERN_INFO "pDMIXReg G217.10 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_1,pDMIXReg->dmix_ptg_config_1);
	printk(KERN_INFO "pDMIXReg G217.11 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_2,pDMIXReg->dmix_ptg_config_2);
	printk(KERN_INFO "pDMIXReg G217.12 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_3,pDMIXReg->dmix_ptg_config_3);
	printk(KERN_INFO "pDMIXReg G217.13 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_4,pDMIXReg->dmix_ptg_config_4);
	printk(KERN_INFO "pDMIXReg G217.14 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_0,pDMIXReg->dmix_dtg_config_0);
	printk(KERN_INFO "pDMIXReg G217.15 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_1,pDMIXReg->dmix_dtg_config_1);
	printk(KERN_INFO "pDMIXReg G217.16 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_2,pDMIXReg->dmix_dtg_config_2);
	printk(KERN_INFO "pDMIXReg G217.17 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_3,pDMIXReg->dmix_dtg_config_3);
	printk(KERN_INFO "pDMIXReg G217.18 0x%08x(%d) \n",pDMIXReg->dmix_exp_config_0,pDMIXReg->dmix_exp_config_0);
	printk(KERN_INFO "pDMIXReg G217.19 0x%08x(%d) \n",pDMIXReg->dmix_exp_config_1,pDMIXReg->dmix_exp_config_1);
	printk(KERN_INFO "pDMIXReg G217.20 0x%08x(%d) \n",pDMIXReg->dmix_source_select,pDMIXReg->dmix_source_select);
	printk(KERN_INFO "pDMIXReg G217.21 0x%08x(%d) \n",pDMIXReg->dmix_chksum,pDMIXReg->dmix_chksum);
	printk(KERN_INFO "pDMIXReg G217.22 0x%08x(%d) \n",pDMIXReg->dmix_version,pDMIXReg->dmix_version);
	printk(KERN_INFO "pDMIXReg G217.23 0x%08x(%d) \n",pDMIXReg->dmix_time_detect_0,pDMIXReg->dmix_time_detect_0);
	printk(KERN_INFO "pDMIXReg G217.24 0x%08x(%d) \n",pDMIXReg->dmix_time_detect_1,pDMIXReg->dmix_time_detect_1);
	printk(KERN_INFO "pDMIXReg G217.25 0x%08x(%d) \n",pDMIXReg->dmix_time_detect_2,pDMIXReg->dmix_time_detect_2);
	//printk(KERN_INFO "pDMIXReg G217.26 0x%08x(%d) \n",pDMIXReg->g217_reserved2[0],pDMIXReg->g217_reserved2[0]);
	//printk(KERN_INFO "pDMIXReg G217.27 0x%08x(%d) \n",pDMIXReg->g217_reserved2[1],pDMIXReg->g217_reserved2[1]);
	//printk(KERN_INFO "pDMIXReg G217.28 0x%08x(%d) \n",pDMIXReg->g217_reserved2[2],pDMIXReg->g217_reserved2[2]);
	//printk(KERN_INFO "pDMIXReg G217.29 0x%08x(%d) \n",pDMIXReg->g217_reserved2[3],pDMIXReg->g217_reserved2[3]);
	//printk(KERN_INFO "pDMIXReg G217.30 0x%08x(%d) \n",pDMIXReg->g217_reserved2[4],pDMIXReg->g217_reserved2[4]);
	//printk(KERN_INFO "pDMIXReg G217.31 0x%08x(%d) \n",pDMIXReg->g217_reserved2[5],pDMIXReg->g217_reserved2[5]);
	#endif
	//dump after setting
	printk(KERN_INFO "pDMIXReg G217.00 0x%08x(%d) \n",pDMIXReg->dmix_layer_config_0,pDMIXReg->dmix_layer_config_0);
	printk(KERN_INFO "pDMIXReg G217.01 0x%08x(%d) \n",pDMIXReg->dmix_layer_config_1,pDMIXReg->dmix_layer_config_1);
	//printk(KERN_INFO "pDMIXReg G217.02 0x%08x(%d) \n",pDMIXReg->dmix_plane_alpha_config_0,pDMIXReg->dmix_plane_alpha_config_0);
	//printk(KERN_INFO "pDMIXReg G217.03 0x%08x(%d) \n",pDMIXReg->dmix_plane_alpha_config_1,pDMIXReg->dmix_plane_alpha_config_1);
	//printk(KERN_INFO "pDMIXReg G217.04 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_0,pDMIXReg->dmix_adjust_config_0);
	//printk(KERN_INFO "pDMIXReg G217.05 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_1,pDMIXReg->dmix_adjust_config_1);
	//printk(KERN_INFO "pDMIXReg G217.06 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_2,pDMIXReg->dmix_adjust_config_2);
	//printk(KERN_INFO "pDMIXReg G217.07 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_3,pDMIXReg->dmix_adjust_config_3);
	//printk(KERN_INFO "pDMIXReg G217.08 0x%08x(%d) \n",pDMIXReg->dmix_adjust_config_4,pDMIXReg->dmix_adjust_config_4);
	printk(KERN_INFO "pDMIXReg G217.09 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_0,pDMIXReg->dmix_ptg_config_0);
	//printk(KERN_INFO "pDMIXReg G217.10 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_1,pDMIXReg->dmix_ptg_config_1);
	printk(KERN_INFO "pDMIXReg G217.11 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_2,pDMIXReg->dmix_ptg_config_2);
	//printk(KERN_INFO "pDMIXReg G217.12 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_3,pDMIXReg->dmix_ptg_config_3);
	//printk(KERN_INFO "pDMIXReg G217.13 0x%08x(%d) \n",pDMIXReg->dmix_ptg_config_4,pDMIXReg->dmix_ptg_config_4);
	//printk(KERN_INFO "pDMIXReg G217.14 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_0,pDMIXReg->dmix_dtg_config_0);
	//printk(KERN_INFO "pDMIXReg G217.15 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_1,pDMIXReg->dmix_dtg_config_1);
	//printk(KERN_INFO "pDMIXReg G217.16 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_2,pDMIXReg->dmix_dtg_config_2);
	//printk(KERN_INFO "pDMIXReg G217.17 0x%08x(%d) \n",pDMIXReg->dmix_dtg_config_3,pDMIXReg->dmix_dtg_config_3);
	//printk(KERN_INFO "pDMIXReg G217.18 0x%08x(%d) \n",pDMIXReg->dmix_exp_config_0,pDMIXReg->dmix_exp_config_0);
	//printk(KERN_INFO "pDMIXReg G217.19 0x%08x(%d) \n",pDMIXReg->dmix_exp_config_1,pDMIXReg->dmix_exp_config_1);
	printk(KERN_INFO "pDMIXReg G217.20 0x%08x(%d) \n",pDMIXReg->dmix_source_select,pDMIXReg->dmix_source_select);	

}

