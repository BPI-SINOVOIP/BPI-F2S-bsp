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
 * @file disp_osd.c
 * @brief
 * @author Hammer Hsieh
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include "display.h"
#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_osd.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// OSD Header config0
#define OSD_HDR_C0_CULT				0x80
#define OSD_HDR_C0_TRANS			0x40

// OSD Header config1
#define OSD_HDR_C1_BS				0x10
#define OSD_HDR_C1_BL2				0x04
#define OSD_HDR_C1_BL				0x01

// OSD control register
#define OSD_CTRL_COLOR_MODE_YUV		(0 << 10)
#define OSD_CTRL_COLOR_MODE_RGB		(1 << 10)
#define OSD_CTRL_NOT_FETCH_EN		(1 << 8)
#define OSD_CTRL_CLUT_FMT_GBRA		(0 << 7)
#define OSD_CTRL_CLUT_FMT_ARGB		(1 << 7)
#define OSD_CTRL_LATCH_EN			(1 << 5)
#define OSD_CTRL_A32B32_EN			(1 << 4)
#define OSD_CTRL_FIFO_DEPTH			(7 << 0)

// OSD region dirty flag for SW latch
#define REGION_ADDR_DIRTY			(1 << 0)
#define REGION_GSCL_DIRTY			(1 << 1)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static DISP_OSD_REG_t *pOSDReg;
static DISP_GPOST_REG_t *pGPOSTReg;
static DISP_OSD1_REG_t *pOSD1Reg;
static DISP_GPOST1_REG_t *pGPOST1Reg;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_OSD_Init(void *pInHWReg1, void *pInHWReg2)
{
	sp_disp_info("DRV_OSD_Init \n");
	pOSDReg = (DISP_OSD_REG_t *)pInHWReg1;
	pGPOSTReg = (DISP_GPOST_REG_t *)pInHWReg2;

	//for i143
	#ifdef DISP_64X64
	//64x64 setting
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	pOSDReg->osd_en 				= 0x0001;	//G196.01
	pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 	= 64;	//G196.17
	pOSDReg->osd_vvld_offset 	= 32;	//G196.18
	pOSDReg->osd_vvld_height 	= 64;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21
	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting	
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	pOSDReg->osd_en 				= 0x0001;	//G196.01
	pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 	= 720;	//G196.17
	pOSDReg->osd_vvld_offset 	= 0;	//G196.18
	pOSDReg->osd_vvld_height 	= 480;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21
	//pOSDReg->osd_bist_ctrl 	= 0x80;	//G196.21 , color bar en , color bar
	//pOSDReg->osd_bist_ctrl 	= 0xC0;	//G196.21 , color bar en , border
	#endif

	#ifdef DISP_576P
	//576P 720x576 setting
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	pOSDReg->osd_en 				= 0x0001;	//G196.01
	pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 	= 720;	//G196.17
	pOSDReg->osd_vvld_offset 	= 0;	//G196.18
	pOSDReg->osd_vvld_height 	= 576;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21
	//pOSDReg->osd_bist_ctrl 	= 0x80;	//G196.21 , color bar en , color bar
	//pOSDReg->osd_bist_ctrl 	= 0xC0;	//G196.21 , color bar en , border
	#endif
	
	#ifdef DISP_720P
	//720P 1280x720 setting
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	pOSDReg->osd_en 				= 0x0001;	//G196.01
	pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 	= 1280;	//G196.17
	pOSDReg->osd_vvld_offset 	= 0;	//G196.18
	pOSDReg->osd_vvld_height 	= 720;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21
	//pOSDReg->osd_bist_ctrl 	= 0x80;	//G196.21 , color bar en , color bar
	//pOSDReg->osd_bist_ctrl 	= 0xC0;	//G196.21 , color bar en , border
	#endif
	
	#ifdef DISP_1080P
	//1080P 1920x1080 setting
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	pOSDReg->osd_en 				= 0x0001;	//G196.01
	pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 	= 1920;	//G196.17
	pOSDReg->osd_vvld_offset 	= 0;	//G196.18
	pOSDReg->osd_vvld_height 	= 1080;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21
	//pOSDReg->osd_bist_ctrl 	= 0x80;	//G196.21 , color bar en , color bar
	//pOSDReg->osd_bist_ctrl 	= 0xC0;	//G196.21 , color bar en , border
	#endif	
	
}

void DRV_OSD1_Init(void *pInHWReg1, void *pInHWReg2)
{
	sp_disp_info("DRV_OSD1_Init \n");
	pOSD1Reg = (DISP_OSD1_REG_t *)pInHWReg1;
	pGPOST1Reg = (DISP_GPOST1_REG_t *)pInHWReg2;
	
	//for i143
	#ifdef DISP_64X64
	//64x64 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
	pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= 64;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 16;	//G197.18
	pOSD1Reg->osd_vvld_height 	= 64;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	#endif
	
	#ifdef DISP_480P
	//480P 720x480 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
	pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= 720;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= 480;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	//pOSD1Reg->osd_bist_ctrl 	= 0x80;	//G197.21 , color bar en , color bar
	//pOSD1Reg->osd_bist_ctrl 	= 0xC0;	//G197.21 , color bar en , border
	#endif

	#ifdef DISP_576P
	//576P 720x576 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
	pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= 720;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= 576;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	//pOSD1Reg->osd_bist_ctrl 	= 0x80;	//G197.21 , color bar en , color bar
	//pOSD1Reg->osd_bist_ctrl 	= 0xC0;	//G197.21 , color bar en , border
	#endif
	
	#ifdef DISP_720P
	//720P 1280x720 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
	pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= 1280;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= 720;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	//pOSD1Reg->osd_bist_ctrl 	= 0x80;	//G197.21 , color bar en , color bar
	//pOSD1Reg->osd_bist_ctrl 	= 0xC0;	//G197.21 , color bar en , border
	#endif
	
	#ifdef DISP_1080P
	//1080P 1920x1080 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
	pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= 1920;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= 1080;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	//pOSD1Reg->osd_bist_ctrl 	= 0x80;	//G197.21 , color bar en , color bar
	//pOSD1Reg->osd_bist_ctrl 	= 0xC0;	//G197.21 , color bar en , border
	#endif	
	
}

void DRV_OSD0_off(void)
{
		sp_disp_info("DRV_OSD0_off \n");
		pOSDReg->osd_en 				= 0x0000;	//G196.01 , osd0 off
}
void DRV_OSD0_on(void)
{
		sp_disp_info("DRV_OSD0_on \n");
		pOSDReg->osd_en 				= 0x0001;	//G196.01 , osd0 en
}
void DRV_OSD1_off(void)
{
		sp_disp_info("DRV_OSD1_off \n");
		pOSD1Reg->osd_en 				= 0x0000;	//G197.01 , osd1 off
}
void DRV_OSD1_on(void)
{
		sp_disp_info("DRV_OSD1_on \n");
		pOSD1Reg->osd_en 				= 0x0001;	//G197.01 , osd1 en
}

void DRV_OSD_SetColorbar(DRV_OsdWindow_e osd_layer, int enable)
{
	sp_disp_info("DRV_OSD_SetColorbar osd%d %d \n", osd_layer, enable);
	switch (osd_layer) {
		case DRV_OSD0: //osd0
				if (enable) {
					if(enable == 1) {
						pOSDReg->osd_bist_ctrl = 0x80;	//G196.21 , color bar en , color bar
					}
					else if(enable == 2) {
						pOSDReg->osd_bist_ctrl = 0xC0;	//G196.21 , color bar en , border
					}
				}
				else {
					pOSDReg->osd_bist_ctrl 	&= (~0xC0);	//G196.21
				}
		break;
		case DRV_OSD1: //osd1
				if (enable) {
					if(enable == 1) {
						pOSD1Reg->osd_bist_ctrl = 0x80;	//G197.21 , color bar en , color bar
					}
					else if(enable == 2) {
						pOSD1Reg->osd_bist_ctrl = 0xC0;	//G197.21 , color bar en , border
					}
				}
				else {
					pOSD1Reg->osd_bist_ctrl &= (~0xC0);	//G197.21
				}
		break;		
		default:
		break;
	}
	
}
//EXPORT_SYMBOL(DRV_OSD_SetColorbar);

int osd0_setting(int base_addr, int w, int h, int osd0_fmt)
{
	//struct sp_disp_device *disp_dev = gDispWorkMem;
	//u32 *osd_header;
	//osd_header = (u32 *)disp_dev->Osd0Header;
	
	sp_disp_info("osd0_setting fmt = %d \n", osd0_fmt);
	
	#if 0
	if (osd0_fmt == 0x2)
		osd_header[0] = SWAP32(0x82001000);
	else
		osd_header[0] = SWAP32(0x00001000 | (osd0_fmt<<24));
		
	osd_header[1] = SWAP32(h<<16 | w );
	osd_header[2] = 0;
	osd_header[3] = 0;
	osd_header[4] = 0;
	if (osd0_fmt == 0x4)
		osd_header[5] = SWAP32(0x00040000 | w ); //YUY2
	else
		osd_header[5] = SWAP32(0x00010000 | w ); //not YUY2
		
	osd_header[6] = SWAP32(0xFFFFFFE0);
	osd_header[7] = SWAP32(base_addr);
	

	sp_disp_info("osd_header %px \n",osd_header);
	sp_disp_info("&osd_header %px \n",&osd_header);
	sp_disp_info("osd_header[0] 0x%08x \n",osd_header[0]);
	sp_disp_info("osd_header[1] 0x%08x \n",osd_header[1]);
	sp_disp_info("osd_header[2] 0x%08x \n",osd_header[2]);
	sp_disp_info("osd_header[3] 0x%08x \n",osd_header[3]);
	sp_disp_info("osd_header[4] 0x%08x \n",osd_header[4]);
	sp_disp_info("osd_header[5] 0x%08x \n",osd_header[5]);
	sp_disp_info("osd_header[6] 0x%08x \n",osd_header[6]);
	sp_disp_info("osd_header[7] 0x%08x \n",osd_header[7]);
	#endif
	
	pOSDReg->osd_ctrl 			= 0x00b7;	//G196.00
	#if (OSD0_GRP_EN == 1)
	pOSDReg->osd_en 				= 0x0001;	//G196.01 , osd0 en
	#else
	pOSDReg->osd_en 				= 0x0000;	//G196.01 , osd0 dis
	#endif
	pOSDReg->osd_base_addr 	= base_addr;	//G196.02
	//pOSDReg->osd_base_addr 	= 0x20200000;	//G196.02
	//pOSDReg->osd_base_addr 	= (u32)(((u64)(&osd_header))&0xffffffff);	//G196.02
	
	pOSDReg->osd_hvld_offset 	= 0;	//G196.16
	pOSDReg->osd_hvld_width 	= w;	//G196.17
	pOSDReg->osd_vvld_offset 	= 0;	//G196.18
	pOSDReg->osd_vvld_height 	= h;	//G196.19
	pOSDReg->osd_data_fetch_ctrl 	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl 	= 0x00;	//G196.21

	//DRV_OSD0_dump();

	return 0;
}

int osd1_setting(int base_addr, int w, int h, int osd1_fmt)
{

	sp_disp_info("osd1_setting fmt = %d \n", osd1_fmt);
	
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
	#if (OSD1_GRP_EN == 1)
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01 , osd1 en
	#else
	pOSD1Reg->osd_en 				= 0x0000;	//G197.01 , osd1 dis
	#endif
	pOSD1Reg->osd_base_addr 	= base_addr;	//G197.02
	//pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_offset 	= 0;	//G197.16
	pOSD1Reg->osd_hvld_width 	= w;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= h;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21

	//DRV_OSD1_dump();

	return 0;
}

void DRV_OSD_INIT_OSD_Header(void)
{
	struct sp_disp_device *disp_dev = gDispWorkMem;

	sp_disp_info("DRV_OSD_INIT_OSD_Header \n");

	sp_disp_info("sizeof(short) = %ld \n", sizeof(short));
	sp_disp_info("sizeof(int) = %ld \n", sizeof(int));
	sp_disp_info("sizeof(long) = %ld \n", sizeof(long));
	sp_disp_info("sizeof(long long) = %ld \n", sizeof(long long));
	
	sp_disp_info("sizeof(size_t) = %ld \n", sizeof(size_t));
	sp_disp_info("sizeof(off_t) = %ld \n", sizeof(off_t));
	sp_disp_info("sizeof(void *) = %ld \n", sizeof(void *));
	sp_disp_info("sizeof(struct HW_OSD_Header_s) = %ld \n", sizeof(struct HW_OSD_Header_s));

	#if 1
	disp_dev->Osd0Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd0Header_phy,
			GFP_KERNEL);
	
	if (!disp_dev->Osd0Header) {
		sp_disp_err("malloc osd0 header fail\n");
		return;
	}
	else {
		sp_disp_info("disp_dev->Osd0Header = %px \n",disp_dev->Osd0Header);
	}
	#endif

	#if 1
	disp_dev->Osd1Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd1Header_phy,
			GFP_KERNEL);
	
	if (!disp_dev->Osd1Header) {
		sp_disp_err("malloc osd1 header fail\n");
		return;
	}
	else {
		sp_disp_info("disp_dev->Osd1Header = %px \n",disp_dev->Osd1Header);
	}
	#endif

}
//EXPORT_SYMBOL(DRV_OSD_INIT_OSD_Header);

void DRV_OSD0_dump(void)
{
	#if 0
	//dump default setting
	printk(KERN_INFO "pOSDReg G196.00 0x%08x(%d) \n",pOSDReg->osd_ctrl,pOSDReg->osd_ctrl);
	printk(KERN_INFO "pOSDReg G196.01 0x%08x(%d) \n",pOSDReg->osd_en,pOSDReg->osd_en);
	printk(KERN_INFO "pOSDReg G196.02 0x%08x(%d) \n",pOSDReg->osd_base_addr,pOSDReg->osd_base_addr);
	printk(KERN_INFO "pOSDReg G196.03 0x%08x(%d) \n",pOSDReg->osd_reserved0[0],pOSDReg->osd_reserved0[0]);
	printk(KERN_INFO "pOSDReg G196.04 0x%08x(%d) \n",pOSDReg->osd_reserved0[1],pOSDReg->osd_reserved0[1]);
	printk(KERN_INFO "pOSDReg G196.05 0x%08x(%d) \n",pOSDReg->osd_reserved0[2],pOSDReg->osd_reserved0[2]);
	printk(KERN_INFO "pOSDReg G196.06 0x%08x(%d) \n",pOSDReg->osd_bus_monitor_l,pOSDReg->osd_bus_monitor_l);
	printk(KERN_INFO "pOSDReg G196.07 0x%08x(%d) \n",pOSDReg->osd_bus_monitor_h,pOSDReg->osd_bus_monitor_h);
	printk(KERN_INFO "pOSDReg G196.08 0x%08x(%d) \n",pOSDReg->osd_req_ctrl,pOSDReg->osd_req_ctrl);
	printk(KERN_INFO "pOSDReg G196.09 0x%08x(%d) \n",pOSDReg->osd_debug_cmd_lock,pOSDReg->osd_debug_cmd_lock);
	printk(KERN_INFO "pOSDReg G196.10 0x%08x(%d) \n",pOSDReg->osd_debug_burst_lock,pOSDReg->osd_debug_burst_lock);
	printk(KERN_INFO "pOSDReg G196.11 0x%08x(%d) \n",pOSDReg->osd_debug_xlen_lock,pOSDReg->osd_debug_xlen_lock);
	printk(KERN_INFO "pOSDReg G196.12 0x%08x(%d) \n",pOSDReg->osd_debug_ylen_lock,pOSDReg->osd_debug_ylen_lock);
	printk(KERN_INFO "pOSDReg G196.13 0x%08x(%d) \n",pOSDReg->osd_debug_queue_lock,pOSDReg->osd_debug_queue_lock);
	printk(KERN_INFO "pOSDReg G196.14 0x%08x(%d) \n",pOSDReg->osd_crc_chksum,pOSDReg->osd_crc_chksum);
	printk(KERN_INFO "pOSDReg G196.15 0x%08x(%d) \n",pOSDReg->osd_reserved1,pOSDReg->osd_reserved1);
	printk(KERN_INFO "pOSDReg G196.16 0x%08x(%d) \n",pOSDReg->osd_hvld_offset,pOSDReg->osd_hvld_offset);
	printk(KERN_INFO "pOSDReg G196.17 0x%08x(%d) \n",pOSDReg->osd_hvld_width,pOSDReg->osd_hvld_width);
	printk(KERN_INFO "pOSDReg G196.18 0x%08x(%d) \n",pOSDReg->osd_vvld_offset,pOSDReg->osd_vvld_offset);
	printk(KERN_INFO "pOSDReg G196.19 0x%08x(%d) \n",pOSDReg->osd_vvld_height,pOSDReg->osd_vvld_height);
	printk(KERN_INFO "pOSDReg G196.20 0x%08x(%d) \n",pOSDReg->osd_data_fetch_ctrl,pOSDReg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSDReg G196.21 0x%08x(%d) \n",pOSDReg->osd_bist_ctrl,pOSDReg->osd_bist_ctrl);
	printk(KERN_INFO "pOSDReg G196.22 0x%08x(%d) \n",pOSDReg->osd_non_fetch_0,pOSDReg->osd_non_fetch_0);
	printk(KERN_INFO "pOSDReg G196.23 0x%08x(%d) \n",pOSDReg->osd_non_fetch_1,pOSDReg->osd_non_fetch_1);
	printk(KERN_INFO "pOSDReg G196.24 0x%08x(%d) \n",pOSDReg->osd_non_fetch_2,pOSDReg->osd_non_fetch_2);
	printk(KERN_INFO "pOSDReg G196.25 0x%08x(%d) \n",pOSDReg->osd_non_fetch_3,pOSDReg->osd_non_fetch_3);
	printk(KERN_INFO "pOSDReg G196.26 0x%08x(%d) \n",pOSDReg->osd_bus_status,pOSDReg->osd_bus_status);
	printk(KERN_INFO "pOSDReg G196.27 0x%08x(%d) \n",pOSDReg->osd_3d_h_offset,pOSDReg->osd_3d_h_offset);
	printk(KERN_INFO "pOSDReg G196.28 0x%08x(%d) \n",pOSDReg->osd_reserved3,pOSDReg->osd_reserved3);
	printk(KERN_INFO "pOSDReg G196.29 0x%08x(%d) \n",pOSDReg->osd_src_decimation_sel,pOSDReg->osd_src_decimation_sel);
	printk(KERN_INFO "pOSDReg G196.30 0x%08x(%d) \n",pOSDReg->osd_bus_time_0,pOSDReg->osd_bus_time_0);
	printk(KERN_INFO "pOSDReg G196.31 0x%08x(%d) \n",pOSDReg->osd_bus_time_1,pOSDReg->osd_bus_time_1);
	#endif
	//dump default setting
	printk(KERN_INFO "pOSDReg G196.00 0x%08x(%d) \n",pOSDReg->osd_ctrl,pOSDReg->osd_ctrl);
	printk(KERN_INFO "pOSDReg G196.01 0x%08x(%d) \n",pOSDReg->osd_en,pOSDReg->osd_en);
	printk(KERN_INFO "pOSDReg G196.02 0x%08x(%d) \n",pOSDReg->osd_base_addr,pOSDReg->osd_base_addr);
	//printk(KERN_INFO "pOSDReg G196.03 0x%08x(%d) \n",pOSDReg->osd_reserved0[0],pOSDReg->osd_reserved0[0]);
	//printk(KERN_INFO "pOSDReg G196.04 0x%08x(%d) \n",pOSDReg->osd_reserved0[1],pOSDReg->osd_reserved0[1]);
	//printk(KERN_INFO "pOSDReg G196.05 0x%08x(%d) \n",pOSDReg->osd_reserved0[2],pOSDReg->osd_reserved0[2]);
	//printk(KERN_INFO "pOSDReg G196.06 0x%08x(%d) \n",pOSDReg->osd_bus_monitor_l,pOSDReg->osd_bus_monitor_l);
	//printk(KERN_INFO "pOSDReg G196.07 0x%08x(%d) \n",pOSDReg->osd_bus_monitor_h,pOSDReg->osd_bus_monitor_h);
	//printk(KERN_INFO "pOSDReg G196.08 0x%08x(%d) \n",pOSDReg->osd_req_ctrl,pOSDReg->osd_req_ctrl);
	//printk(KERN_INFO "pOSDReg G196.09 0x%08x(%d) \n",pOSDReg->osd_debug_cmd_lock,pOSDReg->osd_debug_cmd_lock);
	//printk(KERN_INFO "pOSDReg G196.10 0x%08x(%d) \n",pOSDReg->osd_debug_burst_lock,pOSDReg->osd_debug_burst_lock);
	//printk(KERN_INFO "pOSDReg G196.11 0x%08x(%d) \n",pOSDReg->osd_debug_xlen_lock,pOSDReg->osd_debug_xlen_lock);
	//printk(KERN_INFO "pOSDReg G196.12 0x%08x(%d) \n",pOSDReg->osd_debug_ylen_lock,pOSDReg->osd_debug_ylen_lock);
	//printk(KERN_INFO "pOSDReg G196.13 0x%08x(%d) \n",pOSDReg->osd_debug_queue_lock,pOSDReg->osd_debug_queue_lock);
	//printk(KERN_INFO "pOSDReg G196.14 0x%08x(%d) \n",pOSDReg->osd_crc_chksum,pOSDReg->osd_crc_chksum);
	//printk(KERN_INFO "pOSDReg G196.15 0x%08x(%d) \n",pOSDReg->osd_reserved1,pOSDReg->osd_reserved1);
	printk(KERN_INFO "pOSDReg G196.16 0x%08x(%d) \n",pOSDReg->osd_hvld_offset,pOSDReg->osd_hvld_offset);
	printk(KERN_INFO "pOSDReg G196.17 0x%08x(%d) \n",pOSDReg->osd_hvld_width,pOSDReg->osd_hvld_width);
	printk(KERN_INFO "pOSDReg G196.18 0x%08x(%d) \n",pOSDReg->osd_vvld_offset,pOSDReg->osd_vvld_offset);
	printk(KERN_INFO "pOSDReg G196.19 0x%08x(%d) \n",pOSDReg->osd_vvld_height,pOSDReg->osd_vvld_height);
	printk(KERN_INFO "pOSDReg G196.20 0x%08x(%d) \n",pOSDReg->osd_data_fetch_ctrl,pOSDReg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSDReg G196.21 0x%08x(%d) \n",pOSDReg->osd_bist_ctrl,pOSDReg->osd_bist_ctrl);
	//printk(KERN_INFO "pOSDReg G196.22 0x%08x(%d) \n",pOSDReg->osd_non_fetch_0,pOSDReg->osd_non_fetch_0);
	//printk(KERN_INFO "pOSDReg G196.23 0x%08x(%d) \n",pOSDReg->osd_non_fetch_1,pOSDReg->osd_non_fetch_1);
	//printk(KERN_INFO "pOSDReg G196.24 0x%08x(%d) \n",pOSDReg->osd_non_fetch_2,pOSDReg->osd_non_fetch_2);
	//printk(KERN_INFO "pOSDReg G196.25 0x%08x(%d) \n",pOSDReg->osd_non_fetch_3,pOSDReg->osd_non_fetch_3);
	//printk(KERN_INFO "pOSDReg G196.26 0x%08x(%d) \n",pOSDReg->osd_bus_status,pOSDReg->osd_bus_status);
	//printk(KERN_INFO "pOSDReg G196.27 0x%08x(%d) \n",pOSDReg->osd_3d_h_offset,pOSDReg->osd_3d_h_offset);
	//printk(KERN_INFO "pOSDReg G196.28 0x%08x(%d) \n",pOSDReg->osd_reserved3,pOSDReg->osd_reserved3);
	//printk(KERN_INFO "pOSDReg G196.29 0x%08x(%d) \n",pOSDReg->osd_src_decimation_sel,pOSDReg->osd_src_decimation_sel);
	//printk(KERN_INFO "pOSDReg G196.30 0x%08x(%d) \n",pOSDReg->osd_bus_time_0,pOSDReg->osd_bus_time_0);
	//printk(KERN_INFO "pOSDReg G196.31 0x%08x(%d) \n",pOSDReg->osd_bus_time_1,pOSDReg->osd_bus_time_1);	

}

void DRV_OSD1_dump(void)
{
	#if 0
	//dump after setting
	printk(KERN_INFO "pOSD1Reg G197.00 0x%08x(%d) \n",pOSD1Reg->osd_ctrl,pOSD1Reg->osd_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.01 0x%08x(%d) \n",pOSD1Reg->osd_en,pOSD1Reg->osd_en);
	printk(KERN_INFO "pOSD1Reg G197.02 0x%08x(%d) \n",pOSD1Reg->osd_base_addr,pOSD1Reg->osd_base_addr);
	printk(KERN_INFO "pOSD1Reg G197.03 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[0],pOSD1Reg->osd_reserved0[0]);
	printk(KERN_INFO "pOSD1Reg G197.04 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[1],pOSD1Reg->osd_reserved0[1]);
	printk(KERN_INFO "pOSD1Reg G197.05 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[2],pOSD1Reg->osd_reserved0[2]);
	printk(KERN_INFO "pOSD1Reg G197.06 0x%08x(%d) \n",pOSD1Reg->osd_bus_monitor_l,pOSD1Reg->osd_bus_monitor_l);
	printk(KERN_INFO "pOSD1Reg G197.07 0x%08x(%d) \n",pOSD1Reg->osd_bus_monitor_h,pOSD1Reg->osd_bus_monitor_h);
	printk(KERN_INFO "pOSD1Reg G197.08 0x%08x(%d) \n",pOSD1Reg->osd_req_ctrl,pOSD1Reg->osd_req_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.09 0x%08x(%d) \n",pOSD1Reg->osd_debug_cmd_lock,pOSD1Reg->osd_debug_cmd_lock);
	printk(KERN_INFO "pOSD1Reg G197.10 0x%08x(%d) \n",pOSD1Reg->osd_debug_burst_lock,pOSD1Reg->osd_debug_burst_lock);
	printk(KERN_INFO "pOSD1Reg G197.11 0x%08x(%d) \n",pOSD1Reg->osd_debug_xlen_lock,pOSD1Reg->osd_debug_xlen_lock);
	printk(KERN_INFO "pOSD1Reg G197.12 0x%08x(%d) \n",pOSD1Reg->osd_debug_ylen_lock,pOSD1Reg->osd_debug_ylen_lock);
	printk(KERN_INFO "pOSD1Reg G197.13 0x%08x(%d) \n",pOSD1Reg->osd_debug_queue_lock,pOSD1Reg->osd_debug_queue_lock);
	printk(KERN_INFO "pOSD1Reg G197.14 0x%08x(%d) \n",pOSD1Reg->osd_crc_chksum,pOSD1Reg->osd_crc_chksum);
	printk(KERN_INFO "pOSD1Reg G197.15 0x%08x(%d) \n",pOSD1Reg->osd_reserved1,pOSD1Reg->osd_reserved1);
	printk(KERN_INFO "pOSD1Reg G197.16 0x%08x(%d) \n",pOSD1Reg->osd_hvld_offset,pOSD1Reg->osd_hvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.17 0x%08x(%d) \n",pOSD1Reg->osd_hvld_width,pOSD1Reg->osd_hvld_width);
	printk(KERN_INFO "pOSD1Reg G197.18 0x%08x(%d) \n",pOSD1Reg->osd_vvld_offset,pOSD1Reg->osd_vvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.19 0x%08x(%d) \n",pOSD1Reg->osd_vvld_height,pOSD1Reg->osd_vvld_height);
	printk(KERN_INFO "pOSD1Reg G197.20 0x%08x(%d) \n",pOSD1Reg->osd_data_fetch_ctrl,pOSD1Reg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.21 0x%08x(%d) \n",pOSD1Reg->osd_bist_ctrl,pOSD1Reg->osd_bist_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.22 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_0,pOSD1Reg->osd_non_fetch_0);
	printk(KERN_INFO "pOSD1Reg G197.23 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_1,pOSD1Reg->osd_non_fetch_1);
	printk(KERN_INFO "pOSD1Reg G197.24 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_2,pOSD1Reg->osd_non_fetch_2);
	printk(KERN_INFO "pOSD1Reg G197.25 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_3,pOSD1Reg->osd_non_fetch_3);
	printk(KERN_INFO "pOSD1Reg G197.26 0x%08x(%d) \n",pOSD1Reg->osd_bus_status,pOSD1Reg->osd_bus_status);
	printk(KERN_INFO "pOSD1Reg G197.27 0x%08x(%d) \n",pOSD1Reg->osd_3d_h_offset,pOSD1Reg->osd_3d_h_offset);
	printk(KERN_INFO "pOSD1Reg G197.28 0x%08x(%d) \n",pOSD1Reg->osd_reserved3,pOSD1Reg->osd_reserved3);
	printk(KERN_INFO "pOSD1Reg G197.29 0x%08x(%d) \n",pOSD1Reg->osd_src_decimation_sel,pOSD1Reg->osd_src_decimation_sel);
	printk(KERN_INFO "pOSD1Reg G197.30 0x%08x(%d) \n",pOSD1Reg->osd_bus_time_0,pOSD1Reg->osd_bus_time_0);
	printk(KERN_INFO "pOSD1Reg G197.31 0x%08x(%d) \n",pOSD1Reg->osd_bus_time_1,pOSD1Reg->osd_bus_time_1);
	#endif
	//dump after setting
	printk(KERN_INFO "pOSD1Reg G197.00 0x%08x(%d) \n",pOSD1Reg->osd_ctrl,pOSD1Reg->osd_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.01 0x%08x(%d) \n",pOSD1Reg->osd_en,pOSD1Reg->osd_en);
	printk(KERN_INFO "pOSD1Reg G197.02 0x%08x(%d) \n",pOSD1Reg->osd_base_addr,pOSD1Reg->osd_base_addr);
	//printk(KERN_INFO "pOSD1Reg G197.03 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[0],pOSD1Reg->osd_reserved0[0]);
	//printk(KERN_INFO "pOSD1Reg G197.04 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[1],pOSD1Reg->osd_reserved0[1]);
	//printk(KERN_INFO "pOSD1Reg G197.05 0x%08x(%d) \n",pOSD1Reg->osd_reserved0[2],pOSD1Reg->osd_reserved0[2]);
	//printk(KERN_INFO "pOSD1Reg G197.06 0x%08x(%d) \n",pOSD1Reg->osd_bus_monitor_l,pOSD1Reg->osd_bus_monitor_l);
	//printk(KERN_INFO "pOSD1Reg G197.07 0x%08x(%d) \n",pOSD1Reg->osd_bus_monitor_h,pOSD1Reg->osd_bus_monitor_h);
	//printk(KERN_INFO "pOSD1Reg G197.08 0x%08x(%d) \n",pOSD1Reg->osd_req_ctrl,pOSD1Reg->osd_req_ctrl);
	//printk(KERN_INFO "pOSD1Reg G197.09 0x%08x(%d) \n",pOSD1Reg->osd_debug_cmd_lock,pOSD1Reg->osd_debug_cmd_lock);
	//printk(KERN_INFO "pOSD1Reg G197.10 0x%08x(%d) \n",pOSD1Reg->osd_debug_burst_lock,pOSD1Reg->osd_debug_burst_lock);
	//printk(KERN_INFO "pOSD1Reg G197.11 0x%08x(%d) \n",pOSD1Reg->osd_debug_xlen_lock,pOSD1Reg->osd_debug_xlen_lock);
	//printk(KERN_INFO "pOSD1Reg G197.12 0x%08x(%d) \n",pOSD1Reg->osd_debug_ylen_lock,pOSD1Reg->osd_debug_ylen_lock);
	//printk(KERN_INFO "pOSD1Reg G197.13 0x%08x(%d) \n",pOSD1Reg->osd_debug_queue_lock,pOSD1Reg->osd_debug_queue_lock);
	//printk(KERN_INFO "pOSD1Reg G197.14 0x%08x(%d) \n",pOSD1Reg->osd_crc_chksum,pOSD1Reg->osd_crc_chksum);
	//printk(KERN_INFO "pOSD1Reg G197.15 0x%08x(%d) \n",pOSD1Reg->osd_reserved1,pOSD1Reg->osd_reserved1);
	printk(KERN_INFO "pOSD1Reg G197.16 0x%08x(%d) \n",pOSD1Reg->osd_hvld_offset,pOSD1Reg->osd_hvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.17 0x%08x(%d) \n",pOSD1Reg->osd_hvld_width,pOSD1Reg->osd_hvld_width);
	printk(KERN_INFO "pOSD1Reg G197.18 0x%08x(%d) \n",pOSD1Reg->osd_vvld_offset,pOSD1Reg->osd_vvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.19 0x%08x(%d) \n",pOSD1Reg->osd_vvld_height,pOSD1Reg->osd_vvld_height);
	printk(KERN_INFO "pOSD1Reg G197.20 0x%08x(%d) \n",pOSD1Reg->osd_data_fetch_ctrl,pOSD1Reg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.21 0x%08x(%d) \n",pOSD1Reg->osd_bist_ctrl,pOSD1Reg->osd_bist_ctrl);
	//printk(KERN_INFO "pOSD1Reg G197.22 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_0,pOSD1Reg->osd_non_fetch_0);
	//printk(KERN_INFO "pOSD1Reg G197.23 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_1,pOSD1Reg->osd_non_fetch_1);
	//printk(KERN_INFO "pOSD1Reg G197.24 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_2,pOSD1Reg->osd_non_fetch_2);
	//printk(KERN_INFO "pOSD1Reg G197.25 0x%08x(%d) \n",pOSD1Reg->osd_non_fetch_3,pOSD1Reg->osd_non_fetch_3);
	//printk(KERN_INFO "pOSD1Reg G197.26 0x%08x(%d) \n",pOSD1Reg->osd_bus_status,pOSD1Reg->osd_bus_status);
	//printk(KERN_INFO "pOSD1Reg G197.27 0x%08x(%d) \n",pOSD1Reg->osd_3d_h_offset,pOSD1Reg->osd_3d_h_offset);
	//printk(KERN_INFO "pOSD1Reg G197.28 0x%08x(%d) \n",pOSD1Reg->osd_reserved3,pOSD1Reg->osd_reserved3);
	//printk(KERN_INFO "pOSD1Reg G197.29 0x%08x(%d) \n",pOSD1Reg->osd_src_decimation_sel,pOSD1Reg->osd_src_decimation_sel);
	//printk(KERN_INFO "pOSD1Reg G197.30 0x%08x(%d) \n",pOSD1Reg->osd_bus_time_0,pOSD1Reg->osd_bus_time_0);
	//printk(KERN_INFO "pOSD1Reg G197.31 0x%08x(%d) \n",pOSD1Reg->osd_bus_time_1,pOSD1Reg->osd_bus_time_1);

}
