// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver for timing generator
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include "reg_disp.h"
#include "hal_disp.h"
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
static DISP_TGEN_REG_t *pTGENReg;

static const char * const StrFmt[] = {"480P", "576P", "720P", "1080P", "User Mode"};
static const char * const StrFps[] = {"60Hz", "50Hz", "24Hz"};
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_TGEN_Init(void *pInHWReg)
{
	pTGENReg = (DISP_TGEN_REG_t *)pInHWReg;

	pTGENReg->tgen_config			= 0x00000000;
	pTGENReg->tgen_dtg_config		= 0x00000000;
	pTGENReg->tgen_dtg_adjust1		= 0x0000100d;
	pTGENReg->tgen_reset			= 0x00000001;
	pTGENReg->tgen_user_int1_config	= 0x00000014;
	pTGENReg->tgen_user_int2_config	= 0x00000020;

}

void DRV_TGEN_GetFmtFps(DRV_VideoFormat_e *fmt, DRV_FrameRate_e *fps)
{
	UINT32 tmp_dtg_config = 0;

	tmp_dtg_config = pTGENReg->tgen_dtg_config;

	if (tmp_dtg_config & 0x1) {
		*fmt = DRV_FMT_USER_MODE;
		*fps = DRV_FrameRate_MAX;	//unknown
	} else {
		*fmt = (tmp_dtg_config >> 8) & 0x7;
		*fps = (tmp_dtg_config >> 4) & 0x3;
	}
}

unsigned int DRV_TGEN_GetLineCntNow(void)
{
	return (pTGENReg->tgen_dtg_status1 & 0xfff);
}
EXPORT_SYMBOL(DRV_TGEN_GetLineCntNow);

void DRV_TGEN_SetUserInt1(UINT32 count)
{
	pTGENReg->tgen_user_int1_config = count & 0xfff;
}

void DRV_TGEN_SetUserInt2(UINT32 count)
{
	pTGENReg->tgen_user_int2_config = count & 0xfff;
}

int DRV_TGEN_Set(DRV_SetTGEN_t *SetTGEN)
{
	if (SetTGEN->fmt >= DRV_FMT_MAX) {
		//sp_disp_err("TGEN Timing format:%d error\n", SetTGEN->fmt);
		return DRV_ERR_INVALID_PARAM;
	}

	sp_disp_dbg("TGEN Timing %s, %s\n", StrFmt[SetTGEN->fmt], StrFps[SetTGEN->fps]);

	if (SetTGEN->fmt == DRV_FMT_USER_MODE) {
		sp_disp_info("not support user mode!\n");
	} else {
		pTGENReg->tgen_dtg_config = ((SetTGEN->fmt & 0x7) << 8) | ((SetTGEN->fps & 0x3) << 4);
	}

	return DRV_SUCCESS;
}

void DRV_TGEN_Get(DRV_SetTGEN_t *GetTGEN)
{
	UINT32 tmp;

	tmp = pTGENReg->tgen_dtg_config;

	GetTGEN->fps = (tmp >> 4) & 0x3;
	GetTGEN->fmt = (tmp & 0x1) ? DRV_FMT_USER_MODE:(tmp >> 8) & 0x7;

	sp_disp_info("Cur TGEN Timing %s %s\n", StrFmt[GetTGEN->fmt], StrFps[GetTGEN->fps]);
}

void DRV_TGEN_Reset(void)
{
	pTGENReg->tgen_reset |= 0x1;
}

int DRV_TGEN_Adjust(DRV_TGEN_Input_e Input, UINT32 Adjust)
{
	switch (Input) {
	case DRV_TGEN_VPP0:
		pTGENReg->tgen_dtg_adjust1 = (pTGENReg->tgen_dtg_adjust1 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	case DRV_TGEN_OSD1:
		pTGENReg->tgen_dtg_adjust3 = (pTGENReg->tgen_dtg_adjust3 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	case DRV_TGEN_OSD0:
		pTGENReg->tgen_dtg_adjust3 = (pTGENReg->tgen_dtg_adjust3 & ~(0x3F<<0)) | ((Adjust & 0x3F) << 0);
		break;
	case DRV_TGEN_PTG:
		pTGENReg->tgen_dtg_adjust4 = (pTGENReg->tgen_dtg_adjust4 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	default:
		//sp_disp_err("Invalidate Input %d\n", Input);
		return DRV_ERR_INVALID_PARAM;
	}

	return DRV_SUCCESS;
}
