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
 * @file disp_dve.c
 * @brief
 * @author Hammer Hsieh
 */
/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include "display.h"
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
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_DVE_Init(void *pInHWReg)
{
	sp_disp_info("DRV_DVE_Init \n");
		
	pDVEReg = (DISP_DVE_REG_t *)pInHWReg;

	//for sp7021
	//pDVEReg->color_bar_mode = 0;
	//pDVEReg->dve_hdmi_mode_1 = 0x3;
	//pDVEReg->dve_hdmi_mode_0 = 0x41;// latch mode on
	
	//for i143
	#ifdef DISP_64X64
	//64x64 setting
	pDVEReg->color_bar_mode = 0x0C00; //G235.0 (0x0C00)
	pDVEReg->dve_hdmi_mode_1 = 0x2; //G234.6 (3 --> 2)
	pDVEReg->dve_hdmi_mode_0 = 0x13C;// G234.16 (0x013C(64x64))latch mode on		
	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting
	pDVEReg->color_bar_mode = 0x0000; //G235.0 (0x0C00 --> 0x0000)
	pDVEReg->dve_hdmi_mode_1 = 0x3; //G234.6 (2 or 3) SD
	pDVEReg->dve_hdmi_mode_0 = 0x141;// G234.16 (0x0141(480P))latch mode on	
	#endif

	#ifdef DISP_576P
	//576P 720x576 setting
	pDVEReg->color_bar_mode = 0x0008; //G235.0 (0x0C08 --> 0x0008)
	pDVEReg->dve_hdmi_mode_1 = 0x3; //G234.6 (3) SD
	pDVEReg->dve_hdmi_mode_0 = 0x341;// G234.16 (0x0341(576P))latch mode on	
	#endif
	
	#ifdef DISP_720P
	//720P 1280x720 setting
	pDVEReg->color_bar_mode = 0x0C10; //G235.0 (0x0C10)
	pDVEReg->dve_hdmi_mode_1 = 0x2; //G234.6 (3 --> 2) HD
	pDVEReg->dve_hdmi_mode_0 = 0xD41;// G234.16 (0x0D41(720P))latch mode on	
	#endif
	
	#ifdef DISP_1080P
	//1080P 1920x1080 setting
	pDVEReg->color_bar_mode = 0x0C20; //G235.0 (0x0C20)
	//pDVEReg->color_bar_mode = 0x0C21; //G235.0 (0x0C20 --> 0x0C21) color bar en
	pDVEReg->dve_hdmi_mode_1 = 0x2; //G234.6 (3 --> 2) HD
	pDVEReg->dve_hdmi_mode_0 = 0x541;// G234.16 (0x0541(1080P))latch mode on	
	#endif	
	
}
//EXPORT_SYMBOL(DRV_DVE_Init);

void DRV_DVE_SetMode(int mode)
{
	//int colorbarmode = pDVEReg->color_bar_mode & ~0xfe;
	int colorbarmode = pDVEReg->color_bar_mode & ~0x0cfe; //zebu test
	int hdmi_mode = pDVEReg->dve_hdmi_mode_0 & ~0x1f80;

	sp_disp_info("dve mode: %d\n", mode);

	switch (mode)
	{
		default:
		case 0:	//480P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (0 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (0 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (0 << 11) | (0 << 10) |(0 << 3); //zebu test
			break;
		case 1:	//576P
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (1 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (1 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (0 << 11) | (0 << 10) |(1 << 3); //zebu test
			break;
		case 2:	//720P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (6 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (2 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 11) | (1 << 10) |(2 << 3); //zebu test
			break;
		case 3:	//720P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (7 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (3 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 11) | (1 << 10) |(3 << 3); //zebu test
			break;
		case 4:	//1080P60
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (4 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 11) | (1 << 10) |(4 << 3); //zebu test
			break;
		case 5:	//1080P50
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (0 << 12) | (0 << 7) | (1 << 8) | (3 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (5 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 11) | (1 << 10) |(5 << 3); //zebu test
			break;
		case 6:	//1080P24
			pDVEReg->dve_hdmi_mode_0 = hdmi_mode | (1 << 12) | (0 << 7) | (1 << 8) | (2 << 9);
			//pDVEReg->color_bar_mode = colorbarmode | (6 << 3);
			pDVEReg->color_bar_mode = colorbarmode | (1 << 11) | (1 << 10) |(6 << 3); //zebu test
			break;
		case 7:	//user mode , not support
			//pDVEReg->color_bar_mode = colorbarmode | (0x12 << 3);
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

void DRV_DVE_dump(void)
{

	//dump after setting
	printk(KERN_INFO "pDVEReg G234.00 0x%08x(%d) \n",pDVEReg->dve_vsync_start_top,pDVEReg->dve_vsync_start_top);
	//printk(KERN_INFO "pDVEReg G234.01 0x%08x(%d) \n",pDVEReg->dve_vsync_start_bot,pDVEReg->dve_vsync_start_bot);
	//printk(KERN_INFO "pDVEReg G234.02 0x%08x(%d) \n",pDVEReg->dve_vsync_h_point,pDVEReg->dve_vsync_h_point);
	//printk(KERN_INFO "pDVEReg G234.03 0x%08x(%d) \n",pDVEReg->dve_vsync_pd_cnt,pDVEReg->dve_vsync_pd_cnt);
	//printk(KERN_INFO "pDVEReg G234.04 0x%08x(%d) \n",pDVEReg->dve_hsync_start,pDVEReg->dve_hsync_start);
	//printk(KERN_INFO "pDVEReg G234.05 0x%08x(%d) \n",pDVEReg->dve_hsync_pd_cnt,pDVEReg->dve_hsync_pd_cnt);
	printk(KERN_INFO "pDVEReg G234.06 0x%08x(%d) \n",pDVEReg->dve_hdmi_mode_1,pDVEReg->dve_hdmi_mode_1);
	//printk(KERN_INFO "pDVEReg G234.07 0x%08x(%d) \n",pDVEReg->dve_v_vld_top_start,pDVEReg->dve_v_vld_top_start);
	//printk(KERN_INFO "pDVEReg G234.08 0x%08x(%d) \n",pDVEReg->dve_v_vld_top_end,pDVEReg->dve_v_vld_top_end);
	//printk(KERN_INFO "pDVEReg G234.09 0x%08x(%d) \n",pDVEReg->dve_v_vld_bot_start,pDVEReg->dve_v_vld_bot_start);
	//printk(KERN_INFO "pDVEReg G234.10 0x%08x(%d) \n",pDVEReg->dve_v_vld_bot_end,pDVEReg->dve_v_vld_bot_end);
	//printk(KERN_INFO "pDVEReg G234.11 0x%08x(%d) \n",pDVEReg->dve_de_h_start,pDVEReg->dve_de_h_start);
	//printk(KERN_INFO "pDVEReg G234.12 0x%08x(%d) \n",pDVEReg->dve_de_h_end,pDVEReg->dve_de_h_end);
	//printk(KERN_INFO "pDVEReg G234.13 0x%08x(%d) \n",pDVEReg->dve_mp_tg_line_0_length,pDVEReg->dve_mp_tg_line_0_length);
	//printk(KERN_INFO "pDVEReg G234.14 0x%08x(%d) \n",pDVEReg->dve_mp_tg_frame_0_line,pDVEReg->dve_mp_tg_frame_0_line);
	//printk(KERN_INFO "pDVEReg G234.15 0x%08x(%d) \n",pDVEReg->dve_mp_tg_act_0_pix,pDVEReg->dve_mp_tg_act_0_pix);
	printk(KERN_INFO "pDVEReg G234.16 0x%08x(%d) \n",pDVEReg->dve_hdmi_mode_0,pDVEReg->dve_hdmi_mode_0);

	printk(KERN_INFO "pDVEReg G235.00 0x%08x(%d) \n",pDVEReg->color_bar_mode,pDVEReg->color_bar_mode);
	//printk(KERN_INFO "pDVEReg G235.01 0x%08x(%d) \n",pDVEReg->color_bar_v_total,pDVEReg->color_bar_v_total);
	//printk(KERN_INFO "pDVEReg G235.02 0x%08x(%d) \n",pDVEReg->color_bar_v_active,pDVEReg->color_bar_v_active);
	//printk(KERN_INFO "pDVEReg G235.03 0x%08x(%d) \n",pDVEReg->color_bar_v_active_start,pDVEReg->color_bar_v_active_start);
	//printk(KERN_INFO "pDVEReg G235.04 0x%08x(%d) \n",pDVEReg->color_bar_h_total,pDVEReg->color_bar_h_total);
	//printk(KERN_INFO "pDVEReg G235.05 0x%08x(%d) \n",pDVEReg->color_bar_h_active,pDVEReg->color_bar_h_active);
	
}


