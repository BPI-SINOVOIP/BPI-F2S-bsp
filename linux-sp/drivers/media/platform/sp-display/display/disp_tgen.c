/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2018 by Sunplus Inc.                             *
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
 * @file disp_tgen.c
 * @brief
 * @author PoChou Chen
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_tgen.h"

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

#ifdef SP_DISP_DEBUG
	static const char * const StrFmt[] = {"480P", "576P", "720P", "1080P", "User Mode"};
	static const char * const StrFps[] = {"60Hz", "50Hz", "24Hz"};
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_TGEN_Init(void *pInHWReg)
{
	pTGENReg = (DISP_TGEN_REG_t *)pInHWReg;

	pTGENReg->tgen_config = 0x0007;		// latch mode on
	pTGENReg->tgen_source_sel = 0x0;
	pTGENReg->tgen_user_int2_config = 400;

#if 0 //#ifdef TTL_MODE_SUPPORT
	pTGENReg->tgen_dtg_config = 1; //TGEN USER MODE
	pTGENReg->tgen_dtg_total_pixel = 408; //Total pixel
	pTGENReg->tgen_dtg_ds_line_start_cd_point = 320; //line start
	pTGENReg->tgen_dtg_total_line = 262; //Total line
	pTGENReg->tgen_dtg_field_end_line = 260; //field end line
	pTGENReg->tgen_dtg_start_line = 19; //start line
	//pTGENReg->tgen_reset = 1;
#endif

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
		sp_disp_err("Timing format:%d error\n", SetTGEN->fmt);
		return DRV_ERR_INVALID_PARAM;
	}

	sp_disp_dbg("%s, %s\n", StrFmt[SetTGEN->fmt], StrFps[SetTGEN->fps]);

	if (SetTGEN->fmt == DRV_FMT_USER_MODE) {
		pTGENReg->tgen_dtg_config = 0x0001;

		pTGENReg->tgen_dtg_total_pixel = SetTGEN->htt;		// total pixel
		pTGENReg->tgen_dtg_total_line = SetTGEN->vtt;
		pTGENReg->tgen_dtg_start_line = SetTGEN->v_bp;
		pTGENReg->tgen_dtg_ds_line_start_cd_point = SetTGEN->hactive;
		pTGENReg->tgen_dtg_field_end_line = SetTGEN->vactive + SetTGEN->v_bp + 1;
		sp_disp_err("htt:%d, vtt:%d, h:%d, v:%d, bp:%d\n", SetTGEN->htt, SetTGEN->vtt, SetTGEN->hactive, SetTGEN->vactive, SetTGEN->v_bp);
	} else {
		// [Todo] Moon register setting for pll
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

	sp_disp_dbg("%s %s\n", StrFmt[GetTGEN->fmt], StrFps[GetTGEN->fps]);
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
	case DRV_TGEN_OSD0:
		pTGENReg->tgen_dtg_adjust3 = (pTGENReg->tgen_dtg_adjust3 & ~(0x3F<<0)) | ((Adjust & 0x3F) << 0);
		break;
	case DRV_TGEN_PTG:
		pTGENReg->tgen_dtg_adjust4 = (pTGENReg->tgen_dtg_adjust4 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	default:
		sp_disp_err("Invalidate Input %d\n", Input);
		return DRV_ERR_INVALID_PARAM;
	}

	return DRV_SUCCESS;
}

