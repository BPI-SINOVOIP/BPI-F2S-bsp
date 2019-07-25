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
 * @file disp_vpp.c
 * @brief
 * @author PoChou Chen
 */
/*******************************************************************************
 *                         H E A D E R   F I L E S
 *******************************************************************************/
#include <linux/module.h>

#include "reg_disp.h"
#include "hal_disp.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifdef DEBUG_MSG
	#define DEBUG(fmt, arg...) diag_printf("[Disp][%s:%d] "fmt, __FUNCTION__, __LINE__, ##arg)
	#define MSG(fmt, arg...) diag_printf("[Disp][%s:%d] "fmt, __FUNCTION__, __LINE__, ##arg)
#else
	#define DEBUG(fmt, arg...)
	#define MSG(fmt, arg...)
#endif
#define ERRDISP(fmt, arg...) diag_printf("[Disp][%s:%d] "fmt, __FUNCTION__, __LINE__, ##arg)
#define WARNING(fmt, arg...) diag_printf("[Disp][%s:%d] "fmt, __FUNCTION__, __LINE__, ##arg)
#define INFO(fmt, arg...) diag_printf("[Disp][%s:%d] "fmt, __FUNCTION__, __LINE__, ##arg)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static volatile DISP_VPOST_REG_t *pVPOSTReg;
static volatile DISP_DDFCH_REG_t *pDDFCHReg;

extern struct sp_disp_device gDispWorkMem;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_VPP_Init(void *pInHWReg1, void *pInHWReg2)
{
	pVPOSTReg = (DISP_VPOST_REG_t *)pInHWReg1;
	pDDFCHReg = (DISP_DDFCH_REG_t *)pInHWReg2;
}

//test start
static void wreg_b(UINT32 group, UINT32 index, UINT32 value)
{
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;
	//UINT32 *base = (UINT32 *)0x9c000000;
	UINT32 *base = (UINT32 *)pDispWorkMem->pHWRegBase;

	if (group < 185 && group > 235)
		return;
	base += (group - 185) * 32 + index;

	//printf("wreg base addr 0x%x\n",base);
	*base = value;
	//printf( "group %d[%d] = %04x (%d)\n", group, index, *base, *base);
}

int vpost_setting(int x, int y, int input_w, int input_h, int output_w, int output_h)
{
#ifdef TTL_MODE_SUPPORT
		wreg_b(199, 10, 0x1); //en user mode 
		wreg_b(199, 11, 0); //x active
		wreg_b(199, 12, 21); //y active
#endif

	if ((input_w != output_w) || (input_h != output_h))
	{
		wreg_b(199, 0, 0x11); 
		wreg_b(199, 3, x);
		wreg_b(199, 4, y);
	}
	else
	{
		wreg_b(199, 0, 0x1);
		wreg_b(199, 3, 0);
		wreg_b(199, 4, 0);
	}
#ifdef TTL_MODE_SUPPORT
	wreg_b(199, 1, 320);
	wreg_b(199, 2, 240);
	wreg_b(199, 5, 320);
	wreg_b(199, 6, 240);
#else
	wreg_b(199, 1, input_w);
	wreg_b(199, 2, input_h);
	wreg_b(199, 5, output_w);
	wreg_b(199, 6, output_h);
#endif
	wreg_b(199, 9, 4);
	//wreg_b(199, 9, 7);	//border test pattern
	return 0;
}
EXPORT_SYMBOL(vpost_setting);

void vpost_dma(void)
{
	wreg_b(199, 0, 0x1010);
}

int ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int yuv_fmt)
{
//#include "vpp_pattern/vpp_test_512x300_420.h"
//#include "vpp_pattern/vpp_test_512x300_422.h"
//#include "vpp_pattern/vpp_test_1024x600_420.h"

//	luma_addr = (int)vpp_luma;
//	chroma_addr = (int)vpp_chroma;

	diag_printf("ddfch luma=0x%x, chroma=0x%x\n", luma_addr, chroma_addr);

#ifdef TTL_MODE_SUPPORT
	diag_printf("ddfch setting for LCD \n");
	wreg_b(185, 0, 1);
	if (yuv_fmt == 0)
		wreg_b(185, 1, 0x0);	//source yuv420 NV12
	else if(yuv_fmt == 1)
		wreg_b(185, 1, 0x400);	//source yuv422 NV16
	else if(yuv_fmt == 2)
		wreg_b(185, 1, 0x800);	//source yuv422 YUY2
	wreg_b(185, 2, 0xd0);
	wreg_b(185, 6, luma_addr>>10);
	wreg_b(185, 9, chroma_addr>>10);
	wreg_b(185, 20, 384);	//video line pitch
	//wreg_b(185, 21, 0x01e002d0);	//y size & x size
	wreg_b(185, 22, (0 << 16) | 0);
	wreg_b(185, 23, 0x10000);
	wreg_b(185, 28, 0x80801002);
	wreg_b(185, 21, 0x00f00140);	//y size & x size
#else
	diag_printf("ddfch setting for HDMI \n");
	wreg_b(185, 0, 1);
	if (yuv_fmt == 0)
		wreg_b(185, 1, 0x0);	//source yuv420 NV12
	else if(yuv_fmt == 1)
		wreg_b(185, 1, 0x400);	//source yuv422 NV16
	else if(yuv_fmt == 2)
		wreg_b(185, 1, 0x800);	//source yuv422 YUY2
	wreg_b(185, 2, 0xd0);
	wreg_b(185, 6, luma_addr>>10);
	wreg_b(185, 9, chroma_addr>>10);
	wreg_b(185, 20, EXTENDED_ALIGNED(w, 128));	//video line pitch
	wreg_b(185, 21, (h<<16) | w);	//y size & x size
	wreg_b(185, 22, (0 << 16) | 0);
	wreg_b(185, 23, 0x10000);
	wreg_b(185, 28, 0x80801002);
#endif

	return 0;
}
EXPORT_SYMBOL(ddfch_setting);

