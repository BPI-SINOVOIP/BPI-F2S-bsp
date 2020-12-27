// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver for digital video encoder
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include <media/sunplus/disp/i143/display.h>
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_dve.h"

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
static DISP_DVE_REG_t *pDVEReg;
static const char * const dve_mode[] = {"480P", "576P", "720P60", "720P50", "1080P60", "1080P50", "1080P24", "MANUAL"};
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DVE_Init(void *pInHWReg)
{	
	pDVEReg = (DISP_DVE_REG_t *)pInHWReg;

	pDVEReg->color_bar_mode = 0x0000;
	pDVEReg->dve_hdmi_mode_1 = 0x3;
	pDVEReg->dve_hdmi_mode_0 = 0x141;
	
}
EXPORT_SYMBOL(DRV_DVE_Init);

void DRV_DVE_SetMode(int mode)
{
	int colorbarmode = pDVEReg->color_bar_mode & ~0xfe;
	int hdmi_mode = pDVEReg->dve_hdmi_mode_0 & ~0x1f80;

	sp_disp_dbg("DVE Mode Set: %s\n", dve_mode[mode]);

	switch (mode)
	{
		default:
		case 0:	//480P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (0 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x3;
			pDVEReg->color_bar_mode = colorbarmode | (0 << 3);
			break;
		case 1:	//576P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (1 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x3;
			pDVEReg->color_bar_mode = colorbarmode | (1 << 3);
			break;
		case 2:	//720P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (6 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x3;
			pDVEReg->color_bar_mode = colorbarmode | (2 << 3);
			break;
		case 3:	//720P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (7 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x3;
			pDVEReg->color_bar_mode = colorbarmode | (3 << 3);
			break;
		case 4:	//1080P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x2;
			pDVEReg->color_bar_mode = colorbarmode | (4 << 3);
			break;
		case 5:	//1080P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (3 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x2;
			pDVEReg->color_bar_mode = colorbarmode | (5 << 3);
			break;
		case 6:	//1080P24
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (1 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			pDVEReg->dve_hdmi_mode_1 = 0x2;
			pDVEReg->color_bar_mode = colorbarmode | (6 << 3);
			break;
		case 7:	//user mode , not support
			break;
	}
}

void DRV_DVE_SetColorbar(int enable)
{
	int bist_mode;
	bist_mode = pDVEReg->color_bar_mode & ~0x01;

	if(enable)
		pDVEReg->color_bar_mode |= (1 << 0);
	else
		pDVEReg->color_bar_mode = bist_mode;
}
