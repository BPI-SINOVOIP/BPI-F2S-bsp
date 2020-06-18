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
 * @file disp_tgen.c
 * @brief
 * @author Hammer Hsieh
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

	sp_disp_info("DRV_TGEN_Init \n");
	
	//for sp7021
	//pTGENReg->tgen_config = 0x0007;		// latch mode on
	//pTGENReg->tgen_source_sel = 0x0;
	//pTGENReg->tgen_user_int2_config = 400;
		
	//for i143
	#ifdef DISP_64X64
	//64x64 setting
	pTGENReg->tgen_config = 0x00000000;				//G213.00 // latch mode on
	pTGENReg->tgen_dtg_config = 0x00000600; 	//G213.04 0x0600(64X64)
	pTGENReg->tgen_dtg_adjust1 = 0x0000100d;	//G213.23
	pTGENReg->tgen_reset = 0x00000001;				//G213.01
	pTGENReg->tgen_user_int1_config = 0x00000014;	//G213.02
	pTGENReg->tgen_user_int2_config = 0x00000020;	//G213.03
	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting	
	pTGENReg->tgen_config = 0x00000000;				//G213.00 // latch mode on
	pTGENReg->tgen_dtg_config = 0x00000000; 	//G213.04 0x0000 (480P)
	pTGENReg->tgen_dtg_adjust1 = 0x0000100d;	//G213.23
	pTGENReg->tgen_reset = 0x00000001;				//G213.01
	pTGENReg->tgen_user_int1_config = 0x00000014;	//G213.02
	pTGENReg->tgen_user_int2_config = 0x00000020;	//G213.03
	#endif
	
	#ifdef DISP_576P
	pTGENReg->tgen_config = 0x00000000;				//G213.00 // latch mode on
	pTGENReg->tgen_dtg_config = 0x00000100; 	//G213.04 0x0100 (576P)
	pTGENReg->tgen_dtg_adjust1 = 0x0000100d;	//G213.23
	pTGENReg->tgen_reset = 0x00000001;				//G213.01
	pTGENReg->tgen_user_int1_config = 0x00000014;	//G213.02
	pTGENReg->tgen_user_int2_config = 0x00000020;	//G213.03	
	#endif
	
	#ifdef DISP_720P
	pTGENReg->tgen_config = 0x00000000;				//G213.00 // latch mode on
	pTGENReg->tgen_dtg_config = 0x00000200; 	//G213.04 0x0200 (720P)
	pTGENReg->tgen_dtg_adjust1 = 0x0000100d;	//G213.23
	pTGENReg->tgen_reset = 0x00000001;				//G213.01
	pTGENReg->tgen_user_int1_config = 0x00000014;	//G213.02
	pTGENReg->tgen_user_int2_config = 0x00000020;	//G213.03		
	#endif
	
	#ifdef DISP_1080P
	pTGENReg->tgen_config = 0x00000000;				//G213.00 // latch mode on
	pTGENReg->tgen_dtg_config = 0x00000300; 	//G213.04 0x0300 (1080P)
	pTGENReg->tgen_dtg_adjust1 = 0x0000100d;	//G213.23
	pTGENReg->tgen_reset = 0x00000001;				//G213.01
	pTGENReg->tgen_user_int1_config = 0x00000014;	//G213.02
	pTGENReg->tgen_user_int2_config = 0x00000020;	//G213.03			
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
//EXPORT_SYMBOL(DRV_TGEN_GetLineCntNow);

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
		//sp_disp_err("Timing format:%d error\n", SetTGEN->fmt);
		return DRV_ERR_INVALID_PARAM;
	}

	//sp_disp_dbg("%s, %s\n", StrFmt[SetTGEN->fmt], StrFps[SetTGEN->fps]);

	if (SetTGEN->fmt == DRV_FMT_USER_MODE) {
		pTGENReg->tgen_dtg_config = 0x0001;

		pTGENReg->tgen_dtg_total_pixel = SetTGEN->htt;		// total pixel
		pTGENReg->tgen_dtg_total_line = SetTGEN->vtt;
		pTGENReg->tgen_dtg_start_line = SetTGEN->v_bp;
		pTGENReg->tgen_dtg_ds_line_start_cd_point = SetTGEN->hactive;
		pTGENReg->tgen_dtg_field_end_line = SetTGEN->vactive + SetTGEN->v_bp + 1;
		//sp_disp_info("htt:%d, vtt:%d, h:%d, v:%d, bp:%d\n", SetTGEN->htt, SetTGEN->vtt, SetTGEN->hactive, SetTGEN->vactive, SetTGEN->v_bp);
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

	//sp_disp_dbg("%s %s\n", StrFmt[GetTGEN->fmt], StrFps[GetTGEN->fps]);
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

void DRV_TGEN_dump(void)
{
	#if 0
	//dump after setting
	printk(KERN_INFO "pTGENReg G213.00 0x%08x(%d) \n",pTGENReg->tgen_config,pTGENReg->tgen_config);
	printk(KERN_INFO "pTGENReg G213.01 0x%08x(%d) \n",pTGENReg->tgen_reset,pTGENReg->tgen_reset);
	printk(KERN_INFO "pTGENReg G213.02 0x%08x(%d) \n",pTGENReg->tgen_user_int1_config,pTGENReg->tgen_user_int1_config);
	printk(KERN_INFO "pTGENReg G213.03 0x%08x(%d) \n",pTGENReg->tgen_user_int2_config,pTGENReg->tgen_user_int2_config);
	printk(KERN_INFO "pTGENReg G213.04 0x%08x(%d) \n",pTGENReg->tgen_dtg_config,pTGENReg->tgen_dtg_config);
	printk(KERN_INFO "pTGENReg G213.05 0x%08x(%d) \n",pTGENReg->g213_reserved_1[0],pTGENReg->g213_reserved_1[0]);
	printk(KERN_INFO "pTGENReg G213.06 0x%08x(%d) \n",pTGENReg->g213_reserved_1[1],pTGENReg->g213_reserved_1[1]);
	printk(KERN_INFO "pTGENReg G213.07 0x%08x(%d) \n",pTGENReg->tgen_dtg_venc_line_rst_cnt,pTGENReg->tgen_dtg_venc_line_rst_cnt);
	printk(KERN_INFO "pTGENReg G213.08 0x%08x(%d) \n",pTGENReg->tgen_dtg_total_pixel,pTGENReg->tgen_dtg_total_pixel);
	printk(KERN_INFO "pTGENReg G213.09 0x%08x(%d) \n",pTGENReg->tgen_dtg_ds_line_start_cd_point,pTGENReg->tgen_dtg_ds_line_start_cd_point);
	printk(KERN_INFO "pTGENReg G213.10 0x%08x(%d) \n",pTGENReg->tgen_dtg_total_line,pTGENReg->tgen_dtg_total_line);
	printk(KERN_INFO "pTGENReg G213.11 0x%08x(%d) \n",pTGENReg->tgen_dtg_field_end_line,pTGENReg->tgen_dtg_field_end_line);
	printk(KERN_INFO "pTGENReg G213.12 0x%08x(%d) \n",pTGENReg->tgen_dtg_start_line,pTGENReg->tgen_dtg_start_line);
	printk(KERN_INFO "pTGENReg G213.13 0x%08x(%d) \n",pTGENReg->tgen_dtg_status1,pTGENReg->tgen_dtg_status1);
	printk(KERN_INFO "pTGENReg G213.14 0x%08x(%d) \n",pTGENReg->tgen_dtg_status2,pTGENReg->tgen_dtg_status2);
	printk(KERN_INFO "pTGENReg G213.15 0x%08x(%d) \n",pTGENReg->tgen_dtg_status3,pTGENReg->tgen_dtg_status3);
	printk(KERN_INFO "pTGENReg G213.16 0x%08x(%d) \n",pTGENReg->tgen_dtg_status4,pTGENReg->tgen_dtg_status4);
	printk(KERN_INFO "pTGENReg G213.17 0x%08x(%d) \n",pTGENReg->tgen_dtg_clk_ratio_low,pTGENReg->tgen_dtg_clk_ratio_low);
	printk(KERN_INFO "pTGENReg G213.18 0x%08x(%d) \n",pTGENReg->tgen_dtg_clk_ratio_high,pTGENReg->tgen_dtg_clk_ratio_high);
	printk(KERN_INFO "pTGENReg G213.19 0x%08x(%d) \n",pTGENReg->g213_reserved_2[0],pTGENReg->g213_reserved_2[0]);
	printk(KERN_INFO "pTGENReg G213.20 0x%08x(%d) \n",pTGENReg->g213_reserved_2[1],pTGENReg->g213_reserved_2[1]);
	printk(KERN_INFO "pTGENReg G213.21 0x%08x(%d) \n",pTGENReg->g213_reserved_2[2],pTGENReg->g213_reserved_2[2]);
	printk(KERN_INFO "pTGENReg G213.22 0x%08x(%d) \n",pTGENReg->g213_reserved_2[3],pTGENReg->g213_reserved_2[3]);
	printk(KERN_INFO "pTGENReg G213.23 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust1,pTGENReg->tgen_dtg_adjust1);
	printk(KERN_INFO "pTGENReg G213.24 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust2,pTGENReg->tgen_dtg_adjust2);
	printk(KERN_INFO "pTGENReg G213.25 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust3,pTGENReg->tgen_dtg_adjust3);
	printk(KERN_INFO "pTGENReg G213.26 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust4,pTGENReg->tgen_dtg_adjust4);
	printk(KERN_INFO "pTGENReg G213.27 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust5,pTGENReg->tgen_dtg_adjust5);
	printk(KERN_INFO "pTGENReg G213.28 0x%08x(%d) \n",pTGENReg->g213_reserved_3,pTGENReg->g213_reserved_3);
	printk(KERN_INFO "pTGENReg G213.29 0x%08x(%d) \n",pTGENReg->tgen_source_sel,pTGENReg->tgen_source_sel);
	printk(KERN_INFO "pTGENReg G213.30 0x%08x(%d) \n",pTGENReg->tgen_dtg_field_start_adj_lcnt,pTGENReg->tgen_dtg_field_start_adj_lcnt);
	printk(KERN_INFO "pTGENReg G213.31 0x%08x(%d) \n",pTGENReg->g213_reserved_4,pTGENReg->g213_reserved_4);
	#endif
	//dump after setting
	printk(KERN_INFO "pTGENReg G213.00 0x%08x(%d) \n",pTGENReg->tgen_config,pTGENReg->tgen_config);
	printk(KERN_INFO "pTGENReg G213.01 0x%08x(%d) \n",pTGENReg->tgen_reset,pTGENReg->tgen_reset);
	printk(KERN_INFO "pTGENReg G213.02 0x%08x(%d) \n",pTGENReg->tgen_user_int1_config,pTGENReg->tgen_user_int1_config);
	printk(KERN_INFO "pTGENReg G213.03 0x%08x(%d) \n",pTGENReg->tgen_user_int2_config,pTGENReg->tgen_user_int2_config);
	printk(KERN_INFO "pTGENReg G213.04 0x%08x(%d) \n",pTGENReg->tgen_dtg_config,pTGENReg->tgen_dtg_config);
	printk(KERN_INFO "pTGENReg G213.23 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust1,pTGENReg->tgen_dtg_adjust1);
	printk(KERN_INFO "pTGENReg G213.24 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust2,pTGENReg->tgen_dtg_adjust2);
	printk(KERN_INFO "pTGENReg G213.25 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust3,pTGENReg->tgen_dtg_adjust3);
	printk(KERN_INFO "pTGENReg G213.26 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust4,pTGENReg->tgen_dtg_adjust4);
	printk(KERN_INFO "pTGENReg G213.27 0x%08x(%d) \n",pTGENReg->tgen_dtg_adjust5,pTGENReg->tgen_dtg_adjust5);
	printk(KERN_INFO "pTGENReg G213.29 0x%08x(%d) \n",pTGENReg->tgen_source_sel,pTGENReg->tgen_source_sel);
	printk(KERN_INFO "pTGENReg G213.30 0x%08x(%d) \n",pTGENReg->tgen_dtg_field_start_adj_lcnt,pTGENReg->tgen_dtg_field_start_adj_lcnt);

}

