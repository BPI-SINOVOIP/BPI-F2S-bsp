// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "disp_dmix.h"
#include "disp_tgen.h"
#include "display2.h"

char * const LayerNameStr[] = {"BG", "L1", "L2", "L3", "L4", "L5", "L6"};
char * const LayerModeStr[] = {"AlphaBlend", "Transparent", "Opacity"};
char * const SelStr[] = {"VPP0", "VPP1", "VPP2", "OSD0", "OSD1", "OSD2", "OSD3", "PTG"};

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DMIX_Init(void)
{
	//G217_DMIX_REG->dmix_pix_en_sel = 0x0006;
	//G217_DMIX_REG->dmix_config0 = 0x0070;
	//G217_DMIX_REG->dmix_config1 = 0x8156;
	//G217_DMIX_REG->dmix_config2 = 0x3000;
	//G217_DMIX_REG->dmix_ptg_config = 0x2000;
	//G217_DMIX_REG->dmix_ptg_config_4 = (0<<8) | (0x10 & 0xff);
	//G217_DMIX_REG->dmix_ptg_config_5 = (0<<8) | (0x80 & 0xff);
	//G217_DMIX_REG->dmix_ptg_config_6 = (0<<8) | (0x80 & 0xff);
	//G217_DMIX_REG->dmix_yc_adjust = 0x0;

	G217_DMIX_REG->dmix_layer_config_0 = 0x34561070;
	G217_DMIX_REG->dmix_layer_config_1 = 0x00000000;
	G217_DMIX_REG->dmix_ptg_config_0 = 0x00002001;
	G217_DMIX_REG->dmix_ptg_config_2 = 0x0029f06e;
	G217_DMIX_REG->dmix_source_select = 0x00000006;

}

DRV_Status_e DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_e tpg_sel, int bg_color_yuv, int border_len)
{

	int dmix_mode = G217_DMIX_REG->dmix_ptg_config_0 & (~0xf000);
	
	if (tpg_sel >= DRV_DMIX_TPG_MAX)
		return DRV_ERR_INVALID_PARAM;

	switch (tpg_sel) {
	default:
	case DRV_DMIX_TPG_MAX:
	case DRV_DMIX_TPG_BGC:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (2 << 12);
		DRV_DMIX_PTG_Color_Set_YCbCr(1, (bg_color_yuv>>16) & 0xff, (bg_color_yuv>>8) & 0xff, (bg_color_yuv>>0) & 0xff);
		break;
	case DRV_DMIX_TPG_V_COLORBAR:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (0 << 12);
		break;
	case DRV_DMIX_TPG_H_COLORBAR:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (1 << 15) | (0 << 14) | (0 << 12);
		break;
	case DRV_DMIX_TPG_BORDER:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (1 << 12);
		DRV_DMIX_PTG_Color_Set_YCbCr(0, 0, 0, 0);
		break;
	case DRV_DMIX_TPG_REGION:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (0 << 14) | (2 << 12);
		//DRV_DMIX_PTG_Color_Set_YCbCr(0, 0, 0, 0);
		break;
	case DRV_DMIX_TPG_SNOW:
		G217_DMIX_REG->dmix_ptg_config_0 = dmix_mode | (0 << 15) | (1 << 14) | (0 << 12);
		//DRV_DMIX_PTG_Color_Set_YCbCr(0, 0, 0, 0);
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

	DRV_DMIX_PTG_Color_Set_YCbCr(1, (Y & 0xff), (Cb & 0xff), (Cr & 0xff));
}

void DRV_DMIX_PTG_Color_Set_YCbCr(UINT8 enable, UINT8 Y, UINT8 Cb, UINT8 Cr)
{
	if (enable) {
		G217_DMIX_REG->dmix_ptg_config_2 = (1 << 26) | (1 << 25) | (1 << 24) | \
							((Y & 0xff)<<16) | ((Cb & 0xff)<<8) | ((Cr & 0xff)<<0);
	} else {
		G217_DMIX_REG->dmix_ptg_config_2 = (0 << 26) | (0 << 25) | (0 << 24) | \
							((Y & 0xff)<<16) | ((Cb & 0xff)<<8) | ((Cr & 0xff)<<0);
	}
}

DRV_Status_e DRV_DMIX_Layer_Init(DRV_DMIX_LayerId_e Layer, DRV_DMIX_LayerMode_e LayerMode, DRV_DMIX_InputSel_e FG_Sel)
{

	UINT32 tmp, tmp1;
	DRV_TGEN_Input_e input;

	if ((((int)Layer >= DRV_DMIX_L2) && ((int)Layer <= DRV_DMIX_L4)) ||
			((int)LayerMode < DRV_DMIX_AlphaBlend) || ((int)LayerMode > DRV_DMIX_Opacity) ||
			(((int)FG_Sel != DRV_DMIX_VPP0) && ((int)FG_Sel != DRV_DMIX_OSD1) && ((int)FG_Sel != DRV_DMIX_OSD0) && ((int)FG_Sel != DRV_DMIX_PTG))) {
		//debug("Layer %d, LayerMode %d, InSel %d\n", Layer, LayerMode, FG_Sel);
		return DRV_ERR_INVALID_PARAM;
	}

	debug("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[Layer], LayerModeStr[LayerMode], SelStr[FG_Sel]);

	tmp  = G217_DMIX_REG->dmix_layer_config_0;
	tmp1 = G217_DMIX_REG->dmix_layer_config_1;

	if (Layer != DRV_DMIX_BG) {
		tmp1 &= ~(0x3 << ((Layer - 1) << 1));
		tmp1 |= (LayerMode << ((Layer - 1) << 1));
	}

	tmp = (tmp & ~(0X7 << ((Layer * 4) + 4))) | (FG_Sel << ((Layer * 4) + 4));

	G217_DMIX_REG->dmix_layer_config_0 = tmp;
	G217_DMIX_REG->dmix_layer_config_1 = tmp1;

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

	return DRV_SUCCESS;
}


DRV_Status_e DRV_DMIX_Layer_Set(DRV_DMIX_LayerMode_e LayerMode, DRV_DMIX_InputSel_e FG_Sel)
{
	UINT32 tmp;
	DRV_DMIX_LayerId_e Layer = 0;

	if (((int)LayerMode < DRV_DMIX_AlphaBlend) || ((int)LayerMode > DRV_DMIX_Opacity) ||
			(((int)FG_Sel != DRV_DMIX_VPP0) && ((int)FG_Sel != DRV_DMIX_OSD1) && ((int)FG_Sel != DRV_DMIX_OSD0) && ((int)FG_Sel != DRV_DMIX_PTG))) {
		//debug("Layer %d, LayerMode %d, InSel %d\n", Layer, LayerMode, FG_Sel);
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
	debug("Layer %s, LayerMode %s, InSel %s\n", LayerNameStr[Layer], LayerModeStr[LayerMode], SelStr[FG_Sel]);

	tmp = G217_DMIX_REG->dmix_layer_config_1;

	if (Layer != DRV_DMIX_BG) {
		tmp &= ~(0x3 << ((Layer - 1) << 1));
		tmp |= (LayerMode << ((Layer - 1) << 1));
	}

	G217_DMIX_REG->dmix_layer_config_1 = tmp;

ERROR:
	return DRV_SUCCESS;
}

