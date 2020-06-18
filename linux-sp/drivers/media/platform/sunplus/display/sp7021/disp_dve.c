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
 * @file disp_dve.c
 * @brief
 * @author PoChou Chen
 */
/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_dve.h"

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
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DVE_Init(void *pInHWReg)
{
#ifdef TTL_MODE_SUPPORT
	#ifdef TTL_MODE_DTS
	struct sp_disp_device *disp_dev = gDispWorkMem;
	int hfp,hp,hbp,hac,htot,vfp,vp,vbp,vac,vtot;
	#endif
	int dve_bist_mode = 0;
	int dve_hdmi_mode_0 = 0;
	int dve_hdmi_mode_1 = 0;
#endif

	pDVEReg = (DISP_DVE_REG_t *)pInHWReg;

#ifdef TTL_MODE_SUPPORT
	#ifdef TTL_MODE_DTS
	dve_bist_mode =  (DVE_TTL_BIT_SW_OFF) \
									|(DVE_TTL_MODE) \
									|(DVE_COLOR_SPACE_BT601) \
									|(DVE_TTL_CLK_POL_INV) \
									|(DVE_COLOR_BAR_USER_MODE_SEL) \
									|(DVE_NORMAL_MODE);
	if(disp_dev->TTLPar.ttl_rgb_swap) {
		//sp_disp_dbg("dve init swap rgb %d \n",(int)disp_dev->TTLPar.ttl_rgb_swap);
		dve_bist_mode |= (DVE_TTL_BIT_SW_ON);
		pDVEReg->g235_reserved[0] = 0x4A30; //G235.6
		pDVEReg->g235_reserved[1] = 0x5693; //G235.7
		pDVEReg->g235_reserved[2] = 0x22F6; //G235.8
		pDVEReg->g235_reserved[3] = 0x2D49; //G235.9
		pDVEReg->g235_reserved[4] = 0x39AC; //G235.10
		pDVEReg->g235_reserved[5] = 0x040F; //G235.11
		pDVEReg->g235_reserved[6] = 0x1062; //G235.12
		pDVEReg->g235_reserved[7] = 0x1CC5; //G235.13
	}
	if(disp_dev->TTLPar.ttl_clock_pol) {
		//sp_disp_dbg("dve init ttl clock inv %d \n",(int)disp_dev->TTLPar.ttl_clock_pol);
		dve_bist_mode |= (DVE_TTL_CLK_POL_INV);
	}
	#else
	#ifdef TTL_MODE_BIT_SWAP
	//for TTL_MODE_800_480 and TTL_MODE_1024_600
	dve_bist_mode =  (DVE_TTL_BIT_SW_ON) \
									|(DVE_TTL_MODE) \
									|(DVE_COLOR_SPACE_BT601) \
									|(DVE_TTL_CLK_POL_NOR) \
									|(DVE_COLOR_BAR_USER_MODE_SEL) \
									|(DVE_NORMAL_MODE);
	pDVEReg->g235_reserved[0] = 0x4A30; //G235.6
	pDVEReg->g235_reserved[1] = 0x5693; //G235.7
	pDVEReg->g235_reserved[2] = 0x22F6; //G235.8
	pDVEReg->g235_reserved[3] = 0x2D49; //G235.9
	pDVEReg->g235_reserved[4] = 0x39AC; //G235.10
	pDVEReg->g235_reserved[5] = 0x040F; //G235.11
	pDVEReg->g235_reserved[6] = 0x1062; //G235.12
	pDVEReg->g235_reserved[7] = 0x1CC5; //G235.13
	#else
	//for TTL_MODE_320_240
	dve_bist_mode =  (DVE_TTL_BIT_SW_OFF) \
									|(DVE_TTL_MODE) \
									|(DVE_COLOR_SPACE_BT601) \
									|(DVE_TTL_CLK_POL_INV) \
									|(DVE_COLOR_BAR_USER_MODE_SEL) \
									|(DVE_NORMAL_MODE);
	#endif
	#endif
	dve_hdmi_mode_0 = (DVE_LATCH_DIS) | (DVE_444_MODE);
	dve_hdmi_mode_1 = (DVE_HDMI_ENA) | (DVE_HD_MODE);
	
	pDVEReg->color_bar_mode = dve_bist_mode; 		//COLOR_BAR_MODE_DIS
	pDVEReg->dve_hdmi_mode_1 = dve_hdmi_mode_1; // DVE_HDMI_ENA , DVE_SD_MODE
	pDVEReg->dve_hdmi_mode_0 = dve_hdmi_mode_0;// DVE_LATCH_ENA , DVE_444_MODE

	#ifdef TTL_MODE_DTS
	//sp_disp_dbg("dve init from dts \n");
	//sp_disp_dbg("dve init %d %d \n",(int)disp_dev->TTLPar.hactive,(int)disp_dev->TTLPar.vactive);
	hfp = (int)(disp_dev->TTLPar.hfp);
	hp = (int)(disp_dev->TTLPar.hsync);
	hbp = (int)(disp_dev->TTLPar.hbp);
	hac = (int)(disp_dev->TTLPar.hactive);
	htot = hfp + hp + hbp + hac;
	
	vfp = (int)(disp_dev->TTLPar.vfp);
	vp = (int)(disp_dev->TTLPar.vsync);
	vbp = (int)(disp_dev->TTLPar.vfp);
	vac = (int)(disp_dev->TTLPar.vactive);
	vtot = vfp + vp + vbp + vac;
	
	pDVEReg->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
	pDVEReg->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
	pDVEReg->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(hac+hfp-1);
	pDVEReg->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(vp-1);
	pDVEReg->dve_hsync_start = USER_MODE_HSYNC_START(hac+hfp-1);
	pDVEReg->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(hp+hbp);
	
	pDVEReg->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(vp+vbp-1);
	pDVEReg->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(vp+vbp+vac-1);
	pDVEReg->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(vp+vbp-1);
	pDVEReg->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(vp+vbp+vac-1);
	
	pDVEReg->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(htot-1);
	pDVEReg->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(hac-1);
	pDVEReg->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(htot-1);
	pDVEReg->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(vtot-1);
	pDVEReg->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(hac-1);
	
	pDVEReg->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(vtot-1);
	pDVEReg->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(vac-1);
	pDVEReg->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(vp+vbp);
	pDVEReg->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(htot-1);
	pDVEReg->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(hac-1);	
	#else
	
#ifdef TTL_MODE_1024_600
	sp_disp_dbg("dve init 1024_600 \n");
	pDVEReg->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
	pDVEReg->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
	pDVEReg->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(1183);
	pDVEReg->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
	pDVEReg->dve_hsync_start = USER_MODE_HSYNC_START(1183);
	pDVEReg->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(160);
	
	pDVEReg->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(22);
	pDVEReg->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(622);
	pDVEReg->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(22);
	pDVEReg->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(622);
	
	pDVEReg->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(1343);
	pDVEReg->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(1023);
	pDVEReg->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(1343);
	pDVEReg->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(634);
	pDVEReg->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(1023);
	
	pDVEReg->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(634);
	pDVEReg->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(599);
	pDVEReg->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(23);
	pDVEReg->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(1343);
	pDVEReg->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(1023);
#endif
#ifdef TTL_MODE_800_480
	sp_disp_dbg("dve init 800_480 \n");
	pDVEReg->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
	pDVEReg->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
	pDVEReg->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(989);
	pDVEReg->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
	pDVEReg->dve_hsync_start = USER_MODE_HSYNC_START(989);
	pDVEReg->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(66);
	
	pDVEReg->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(25);
	pDVEReg->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(505);
	pDVEReg->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(25);
	pDVEReg->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(505);
	
	pDVEReg->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(1055);
	pDVEReg->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(799);
	pDVEReg->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(1055);
	pDVEReg->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(524);
	pDVEReg->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(799);
	
	pDVEReg->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(524);
	pDVEReg->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(479);
	pDVEReg->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(26);
	pDVEReg->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(1055);
	pDVEReg->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(799);
#endif
#ifdef TTL_MODE_320_240
	sp_disp_dbg("dve init 320_240 \n");
	pDVEReg->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
	pDVEReg->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
	pDVEReg->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(339);
	pDVEReg->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(2);
	pDVEReg->dve_hsync_start = USER_MODE_HSYNC_START(339);
	pDVEReg->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(68);
	
	pDVEReg->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(17);
	pDVEReg->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(257);
	pDVEReg->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(17);
	pDVEReg->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(257);
	
	pDVEReg->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(407);
	pDVEReg->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(319);
	pDVEReg->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(407);
	pDVEReg->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(261);
	pDVEReg->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(319);
	
	pDVEReg->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(261);
	pDVEReg->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(239);
	pDVEReg->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(18);
	pDVEReg->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(407);
	pDVEReg->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(319);
#endif
	#endif

#else
	pDVEReg->color_bar_mode = 0;
	pDVEReg->dve_hdmi_mode_1 = 0x3;
	pDVEReg->dve_hdmi_mode_0 = 0x41;// latch mode on
#endif
}
EXPORT_SYMBOL(DRV_DVE_Init);

#ifdef TTL_MODE_SUPPORT
#ifdef TTL_MODE_DTS
void sp_disp_set_ttl_dve(void)
{
	struct sp_disp_device *disp_dev = gDispWorkMem;
	int hfp,hp,hbp,hac,htot,vfp,vp,vbp,vac,vtot;
	int dve_bist_mode = 0;
	
	dve_bist_mode = pDVEReg->color_bar_mode;
	
	if(disp_dev->TTLPar.ttl_rgb_swap)
		dve_bist_mode |= (DVE_TTL_BIT_SW_ON);
	else
		dve_bist_mode &= (~DVE_TTL_BIT_SW_ON);
	sp_disp_dbg("dve set swap rgb %d \n", (int)disp_dev->TTLPar.ttl_rgb_swap);
		
	if(disp_dev->TTLPar.ttl_clock_pol)
		dve_bist_mode |= (DVE_TTL_CLK_POL_INV);
	else
		dve_bist_mode &= (~DVE_TTL_CLK_POL_INV);
	sp_disp_dbg("dve set ttl clock inv %d \n", (int)disp_dev->TTLPar.ttl_clock_pol);
		
	pDVEReg->color_bar_mode = dve_bist_mode;
	
	hfp = (int)(disp_dev->TTLPar.hfp);
	hp = (int)(disp_dev->TTLPar.hsync);
	hbp = (int)(disp_dev->TTLPar.hbp);
	hac = (int)(disp_dev->TTLPar.hactive);
	htot = hfp + hp + hbp + hac;
	
	vfp = (int)(disp_dev->TTLPar.vfp);
	vp = (int)(disp_dev->TTLPar.vsync);
	vbp = (int)(disp_dev->TTLPar.vfp);
	vac = (int)(disp_dev->TTLPar.vactive);
	vtot = vfp + vp + vbp + vac;
	
	sp_disp_dbg("ttl_hor set : %d %d %d %d , %d \n", hfp, hp, hbp, hac, htot);
	sp_disp_dbg("ttl_ver set : %d %d %d %d , %d \n", vfp, vp, vbp, vac, vtot);
	
	pDVEReg->dve_vsync_start_top = USER_MODE_VSYNC_TOP_START(0);
	pDVEReg->dve_vsync_start_bot = USER_MODE_VSYNC_BOT_START(0);
	pDVEReg->dve_vsync_h_point = USER_MODE_VSYNC_HOR_POINT(hac+hfp-1);
	pDVEReg->dve_vsync_pd_cnt = USER_MODE_VSYNC_WITCH_LINE(vp-1);
	pDVEReg->dve_hsync_start = USER_MODE_HSYNC_START(hac+hfp-1);
	pDVEReg->dve_hsync_pd_cnt = USER_MODE_HSYNC_WITCH_PIXEL(hp+hbp);
	
	pDVEReg->dve_v_vld_top_start = USER_MODE_VER_VALID_TOP_START(vp+vbp-1);
	pDVEReg->dve_v_vld_top_end = USER_MODE_VER_VALID_TOP_END(vp+vbp+vac-1);
	pDVEReg->dve_v_vld_bot_start = USER_MODE_VER_VALID_BOT_START(vp+vbp-1);
	pDVEReg->dve_v_vld_bot_end = USER_MODE_VER_VALID_BOT_END(vp+vbp+vac-1);
	
	pDVEReg->dve_de_h_start = USER_MODE_HOR_DATA_ENABLE_START(htot-1);
	pDVEReg->dve_de_h_end = USER_MODE_HOR_DATA_ENABLE_END(hac-1);
	pDVEReg->dve_mp_tg_line_0_length = USER_MODE_TOTAL_PIXEL(htot-1);
	pDVEReg->dve_mp_tg_frame_0_line = USER_MODE_TOTAL_LINE(vtot-1);
	pDVEReg->dve_mp_tg_act_0_pix = USER_MODE_ACTIVE_PIXEL(hac-1);
	
	pDVEReg->color_bar_v_total = USER_MODE_COLORBAR_VER_TOTAL(vtot-1);
	pDVEReg->color_bar_v_active = USER_MODE_COLORBAR_VER_ACTIVE(vac-1);
	pDVEReg->color_bar_v_active_start = USER_MODE_COLORBAR_VER_ACTIVE_START(vp+vbp);
	pDVEReg->color_bar_h_total = USER_MODE_COLORBAR_HOR_TOTAL(htot-1);
	pDVEReg->color_bar_h_active = USER_MODE_COLORBAR_HOR_ACTIVE(hac-1);			

}
EXPORT_SYMBOL(sp_disp_set_ttl_dve);
#endif
#endif

void DRV_DVE_SetMode(int mode)
{
	int colorbarmode = pDVEReg->color_bar_mode & ~0xfe;
	int hdmi_mode = pDVEReg->dve_hdmi_mode_0 & ~0x1f80;

	sp_disp_dbg("dve mode: %d\n", mode);

	switch (mode)
	{
		default:
		case 0:	//480P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (0 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (0 << 3);
			break;
		case 1:	//576P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (1 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 3);
			break;
		case 2:	//720P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (6 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (2 << 3);
			break;
		case 3:	//720P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (7 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (3 << 3);
			break;
		case 4:	//1080P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (3 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (4 << 3);
			break;
		case 5:	//1080P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (5 << 3);
			break;
		case 6:	//1080P24
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (1 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			pDVEReg->color_bar_mode = colorbarmode | (6 << 3);
			break;
		case 7:	//user mode
			pDVEReg->color_bar_mode = colorbarmode | (0x12 << 3);
			break;
	}
}

void DRV_DVE_SetColorbar(int enable)
{
#ifdef TTL_MODE_SUPPORT
	int bist_mode;
	bist_mode = pDVEReg->color_bar_mode & ~0x01;
	
	if(enable)
		pDVEReg->color_bar_mode |= (1 << 0);
	else
		pDVEReg->color_bar_mode = bist_mode;
#else
	pDVEReg->color_bar_mode |= (1 << 0);
#endif
}

