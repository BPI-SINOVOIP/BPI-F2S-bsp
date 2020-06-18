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
 * @file display.c
 * @brief display driver
 * @author Hammer Hsieh
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
//#include <mach/irqs.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "disp_osd.h"
#include "display.h"

#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_dve.h"
#include "disp_dmix.h"
#include "disp_tgen.h"
#include "disp_vpp.h"
#include "disp_palette.h"

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define DISP_ALIGN(x, n)		(((x) + ((n)-1)) & ~((n)-1))

#ifdef SUPPORT_DEBUG_MON
	#define MON_PROC_NAME		"disp_mon"
	#define MON_CMD_LEN			(256)
#endif

#if (VPPDMA_FETCH_EN == 1) //VPPDMA fetch data
	#if ((VPPDMA_VPP_WIDTH == 720) && (VPPDMA_VPP_HEIGHT == 480))
		#if (VPPDMA_FMT_HDMI == 0) //RGB888
		unsigned char vppdma_data_array[720*480*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_720x480.h"
		};
		#elif (VPPDMA_FMT_HDMI == 1) //RGB565
		unsigned char vppdma_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB565_720x480.h"
		};
		#elif (VPPDMA_FMT_HDMI == 2) //YUV422_YUY2 (default fmt UYVY)
		unsigned char vppdma_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_UYVY_720x480.h"
		};
		#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
		unsigned char vppdma_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_NV16_720x480.h"
		};
		#else //RGB888
		unsigned char vppdma_data_array[720*480*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_720x480.h"
		};
		#endif
	#endif

	#if ((VPPDMA_VPP_WIDTH == 1280) && (VPPDMA_VPP_HEIGHT == 720))
		#if (VPPDMA_FMT_HDMI == 0) //RGB888
		unsigned char vppdma_data_array[1280*720*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_1280x720.h"
		};
		#elif (VPPDMA_FMT_HDMI == 1) //RGB565
		unsigned char vppdma_data_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB565_1280x720.h"
		};
		#elif (VPPDMA_FMT_HDMI == 2) //YUV422_YUY2 (default fmt UYVY)
		unsigned char vppdma_data_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_UYVY_1280x720.h"
			//#include "vpp_pattern/vppdma/yuv422_YUY2_1280x720.h"
		};
		#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
		unsigned char vppdma_data_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_NV16_1280x720.h"
		};
		#else //RGB888
		unsigned char vppdma_data_array[1280*720*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_1280x720.h"
		};
		#endif
	#endif

	#if ((VPPDMA_VPP_WIDTH == 1920) && (VPPDMA_VPP_HEIGHT == 1080))
		#if (VPPDMA_FMT_HDMI == 0) //RGB888
		unsigned char vppdma_data_array[1920*1080*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_1920x1080.h"
		};
		#elif (VPPDMA_FMT_HDMI == 1) //RGB565
		unsigned char vppdma_data_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB565_1920x1080.h"
		};
		#elif (VPPDMA_FMT_HDMI == 2) //YUV422_YUY2 (default fmt UYVY)
		unsigned char vppdma_data_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_UYVY_1920x1080.h"
			//#include "vpp_pattern/vppdma/yuv422_YUY2_1920x1080.h"
		};
		#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
		unsigned char vppdma_data_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_NV16_1920x1080.h"
		};
		#else //RGB888
		unsigned char vppdma_data_array[1920*1080*3] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/RGB888_1920x1080.h"
		};
		#endif
	#endif
#endif

#if (OSD0_FETCH_EN == 1) //OSD0 fetch data
	#if ((OSD0_WIDTH == 720) && (OSD0_HEIGHT == 480))
		#if (OSD0_FMT_HDMI == 0x2) //0x2:8bpp
		unsigned char osd0_data_array[720*480] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB8bpp_720x480.h"
		};
		#elif (OSD0_FMT_HDMI == 0x4) //0x4:YUY2
		unsigned char osd0_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/yuv422_YUY2_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x04, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0x8) //0x8:RGB565
		unsigned char osd0_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/RGB565_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x08, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0x9) //0x9:ARGB1555
		unsigned char osd0_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB1555_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x09, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0xa) //0xa:RGBA4444
		unsigned char osd0_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/RGBA4444_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x0a, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0xb) //0xb:ARGB4444
		unsigned char osd0_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB4444_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x0b, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0xd) //0xd:RGBA8888
		unsigned char osd0_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/RGBA8888_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x0d, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#elif (OSD0_FMT_HDMI == 0xe) //0xe:ARGB8888
		#if 0 //alpha value = 0
		unsigned char osd0_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB8888_720x480_alpha00.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			//0x0e, 0x00, 0x10, 0xff, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x0e, 0x00, 0x11, 0xff, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			//0x0e, 0x00, 0x14, 0xff, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#else
		unsigned char osd0_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB8888_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x0e, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#endif
		#else //default setting 0xe:ARGB8888
		unsigned char osd0_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd0/ARGB8888_720x480.h"
		};
		unsigned char osd0_header_array[32] __attribute__((aligned(1024))) = {
			0x0e, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x21, 0x00, 0x00, 0x00
		};
		#endif
	#endif
#endif

#if (OSD1_FETCH_EN == 1) // OSD1 fetch data
	#if ((OSD1_WIDTH == 720) && (OSD1_HEIGHT == 480))
		#if (OSD1_FMT_HDMI == 0x2) //0x2:8bpp
		unsigned char osd1_data_array[720*480] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/ARGB8bpp_720x480.h"
		};
		#elif (OSD1_FMT_HDMI == 0x4) //0x4:YUY2
		unsigned char osd1_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/yuv422_YUY2_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x04, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0x8) //0x8:RGB565
		unsigned char osd1_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/RGB565_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x08, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0x9) //0x9:ARGB1555
		unsigned char osd1_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/ARGB1555_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x09, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0xa) //0xa:RGBA4444
		unsigned char osd1_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/RGBA4444_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x0a, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0xb) //0xb:ARGB4444
		unsigned char osd1_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/ARGB4444_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x0b, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0xd) //0xd:RGBA8888
		unsigned char osd1_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/RGBA8888_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x0d, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#elif (OSD1_FMT_HDMI == 0xe) //0xe:ARGB8888
		unsigned char osd1_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/ARGB8888_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x0e, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#else //default setting 0xe:ARGB8888
		unsigned char osd1_data_array[720*480*4] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/osd1/ARGB8888_720x480.h"
		};
		unsigned char osd1_header_array[32] __attribute__((aligned(1024))) = {
			0x0e, 0x00, 0x10, 0x00, 0x01, 0xE0, 0x02, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xD0, 0xFF, 0xFF, 0xFF, 0xE0, 0x22, 0x00, 0x00, 0x00
		};
		#endif
	#endif
#endif

#if (DDFCH_FETCH_EN == 1) // DDFCH fetch data
	#if ((DDFCH_VPP_WIDTH == 720) && (DDFCH_VPP_HEIGHT == 480))
		#if (DDFCH_FMT_HDMI == 0) //YUV420_NV12
		char vpp_yuv_array[768*480*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_720x480.h"
		};
		#elif (DDFCH_FMT_HDMI == 1) //YUV422_NV16
		char vpp_yuv_array[768*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_NV16_720x480.h"
		};
		#elif (DDFCH_FMT_HDMI == 2) //YUV422_YUY2
		char vpp_yuv_array[768*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_YUY2_720x480.h"
		};
		#else
		char vpp_yuv_array[768*480*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_720x480.h"
		};
		#endif
	#endif

	#if ((DDFCH_VPP_WIDTH == 1280) && (DDFCH_VPP_HEIGHT == 720))
		#if (DDFCH_FMT_HDMI == 0) //YUV420_NV12
		char vpp_yuv_array[1280*720*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_1280x720.h"
		};
		#elif (DDFCH_FMT_HDMI == 1) //YUV422_NV16
		char vpp_yuv_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_NV16_1280x720.h"
		};
		#elif (DDFCH_FMT_HDMI == 2) //YUV422_YUY2
		char vpp_yuv_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_YUY2_1280x720.h"
		};
		#else
		char vpp_yuv_array[1280*720*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_1280x720.h"
		};
		#endif
	#endif

	#if ((DDFCH_VPP_WIDTH == 1920) && (DDFCH_VPP_HEIGHT == 1080))
		#if (DDFCH_FMT_HDMI == 0) //YUV420_NV12
		char vpp_yuv_array[1920*1080*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_1920x1080.h"
		};
		#elif (DDFCH_FMT_HDMI == 1) //YUV422_NV16
		char vpp_yuv_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_NV16_1920x1080.h"
		};
		#elif (DDFCH_FMT_HDMI == 2) //YUV422_YUY2
		char vpp_yuv_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv422_YUY2_1920x1080.h"
		};
		#else
		char vpp_yuv_array[1920*1080*3/2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/ddfch/yuv420_NV12_1920x1080.h"
		};
		#endif
	#endif
#endif
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
// platform driver functions
static int _display_probe(struct platform_device *pdev);
static int _display_remove(struct platform_device *pdev);
static int _display_suspend(struct platform_device *pdev, pm_message_t state);
static int _display_resume(struct platform_device *pdev);

#ifdef SUPPORT_DEBUG_MON
static int _open_flush_proc(struct inode *inode, struct file *file);
static ssize_t _write_flush_proc(struct file *filp, const char __user *buffer, size_t len, loff_t *f_pos);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct sp_disp_device *gDispWorkMem;

static struct of_device_id _display_ids[] = {
	{ .compatible = "sunplus,i143-display"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, _display_ids);


// platform driver
static struct platform_driver _display_driver = {
	.probe		= _display_probe,
	.remove		= _display_remove,
	.suspend	= _display_suspend,
	.resume		= _display_resume,
	.driver		= {
		.name	= "i143_display",
		.of_match_table	= of_match_ptr(_display_ids),
	},
};
module_platform_driver(_display_driver);
#if 0
static int __init sp_disp_i143_reg(void)
{
	return platform_driver_register(&_display_driver);
}
//module_initcall(sp_disp_i143_reg);
device_initcall(sp_disp_i143_reg);

static void __exit sp_disp_i143_exit(void)
{
	platform_driver_unregister(&_display_driver);
}
module_exit(sp_disp_i143_exit);
#endif

#ifdef SUPPORT_DEBUG_MON
static const struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.open = _open_flush_proc,
	.write = _write_flush_proc,
	.read = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release,
};

struct proc_dir_entry *entry = NULL;
#endif

/**************************************************************************
 *                     S T A T I C   F U N C T I O N                      *
 **************************************************************************/
extern bool flag_vscl_path_vppdma;
extern bool flag_vscl_path_osd0; 
extern bool flag_vscl_path_ddfch;

static volatile UINT32 *G0;
static volatile UINT32 *G1;
static volatile UINT32 *G4;

uint64_t pa;

#if (OSD0_FETCH_EN == 1)
static UINT32 *osd0_data_ptr;
#endif
#if (OSD1_FETCH_EN == 1)
static UINT32 *osd1_data_ptr;
#endif
#if (VPPDMA_FETCH_EN == 1)
static UINT32 *vppdma_data_ptr;
#endif
#if (DDFCH_FETCH_EN == 1)
static UINT32 *vpp_yuv_ptr;
#endif

int vpp_alloc_size;

static irqreturn_t _display_irq_field_start(int irq, void *param)
{
	//sp_disp_info("field_start!field_start! \n");
	#if 1
	if (flag_vscl_path_vppdma) {
		vpp_path_select(DRV_FROM_VPPDMA); //default setting
		flag_vscl_path_vppdma = 0;
	}
	if (flag_vscl_path_osd0) {
		vpp_path_select(DRV_FROM_OSD0);
		flag_vscl_path_osd0 = 0;
	}
	if (flag_vscl_path_ddfch) {
		vpp_path_select(DRV_FROM_DDFCH);
		flag_vscl_path_ddfch = 0;
	}
	#endif
	return IRQ_HANDLED;
}

static irqreturn_t _display_irq_field_end(int irq, void *param)
{
	//sp_disp_info("field_end!field_end! \n");
	#if 0
	if (flag_vscl_path_vppdma) {
		vpp_path_select(DRV_FROM_VPPDMA); //default setting
		flag_vscl_path_vppdma = 0;
	}
	if (flag_vscl_path_osd0) {
		vpp_path_select(DRV_FROM_OSD0);
		flag_vscl_path_osd0 = 0;
	}
	if (flag_vscl_path_ddfch) {
		vpp_path_select(DRV_FROM_DDFCH);
		flag_vscl_path_ddfch = 0;
	}
	#endif
	return IRQ_HANDLED;
}

static irqreturn_t _display_irq_field_int1(int irq, void *param)
{
	//sp_disp_info("field_int1! \n");
	return IRQ_HANDLED;
}

static irqreturn_t _display_irq_field_int2(int irq, void *param)
{
	//sp_disp_info("field_int2! \n");
	return IRQ_HANDLED;
}

#ifdef SUPPORT_DEBUG_MON
typedef struct _BMP_HEADER
{
	unsigned short reserved0;
	unsigned short identifier;
	unsigned int file_size;
	unsigned int reserved1;
	unsigned int data_offset;
	unsigned int header_size;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bpp;
	unsigned int compression;
	unsigned short reserved;
} BMP_HEADER;

int _show_flush_proc(struct seq_file *m, void *v)
{
	sp_disp_info("_show_flush_proc_2 \n");

	return 0;
}

int _open_flush_proc(struct inode *inode, struct file *file)
{
	return single_open(file, _show_flush_proc, NULL);
}

static char* _mon_skipspace(char *p)
{
	if (p == NULL)
		return NULL;
	while (1)
	{
		int c = p[0];
		if (c == ' ' || c == '\t' || c == '\v')
			p++;
		else
			break;
	}
	return p;
}

static char* _mon_readint(char *p, int *x)
{
	int base = 10;
	int cnt, retval;

	if (x == NULL)
		return p;

	*x = 0;

	if (p == NULL)
		return NULL;

	p = _mon_skipspace(p);

	if (p[0] == '0' && p[1] == 'x')
	{
		base = 16;
		p += 2;
	}
	if (p[0] == '0' && p[1] == 'o')
	{
		base = 8;
		p += 2;
	}
	if (p[0] == '0' && p[1] == 'b')
	{
		base = 2;
		p += 2;
	}

	retval = 0;
	for (cnt = 0; 1; cnt++)
	{
		int c = *p++;
		int val;

		// translate 0~9, a~z, A~Z
		if (c >= '0' && c <= '9')
			val = c - '0';
		else if (c >= 'a' && c <= 'z')
			val = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			val = c - 'A' + 10;
		else
			break;
		if (val >= base)
			break;

		retval = retval * base + val;
	}

	if (cnt == 0)
		return p;			// no translation is done??

	*(unsigned int *) x = retval;		// store it
	return p;
}

static void _debug_cmd(char *tmpbuf)
{
	UINT32 ret;
	DRV_SetTGEN_t SetTGEN;

	tmpbuf = _mon_skipspace(tmpbuf);

	sp_disp_info("run disp debug cmd \n");
	if (!strncasecmp(tmpbuf, "dump", 4))
	{
		int dump_sel = 0;
		tmpbuf = _mon_readint(tmpbuf + 4, (int *)&dump_sel);
		
		if (dump_sel == 0) {
			sp_disp_info("dump all disp reg \n");
			
			DRV_DVE_dump(); //G234 G235
			DRV_DMIX_dump(); //G217
			DRV_TGEN_dump(); //G213
			
			DRV_VPOST_dump(); //G199
			DRV_VSCL_dump(); //G187 G188
			DRV_VPPDMA_dump(); //G186
			DRV_DDFCH_dump(); //G185
			
			DRV_OSD0_dump(); //G196 , (G206 if necessary)
			DRV_OSD1_dump(); //G197 , (G207 if necessary)	
		}
		else if (dump_sel == 1) {
			sp_disp_info("dump video out reg \n");
			DRV_DVE_dump(); //G234 G235
			DRV_DMIX_dump(); //G217
			DRV_TGEN_dump(); //G213
		}
		else if (dump_sel == 2) {
			sp_disp_info("dump vpp path reg \n");
			DRV_VPOST_dump(); //G199
			DRV_VSCL_dump(); //G187 G188
			DRV_VPPDMA_dump(); //G186
			DRV_DDFCH_dump(); //G185
		}
		else if (dump_sel == 3) {
			sp_disp_info("dump osd path reg \n");
			DRV_OSD0_dump(); //G196 , (G206 if necessary)
			DRV_OSD1_dump(); //G197 , (G207 if necessary)	
		}

	}
	else if (!strncasecmp(tmpbuf, "layer", 5))
	{
		int layer_num = 0;
		int layer_val = 0;
		
		tmpbuf = _mon_readint(tmpbuf + 5, (int *)&layer_num);
		tmpbuf = _mon_readint(tmpbuf, (int *)&layer_val);

		//if (layer_num >= 6)
		//	 layer_num = 6;
		if (layer_val >= 2)
			 layer_val = 2;
		
		if ((layer_num >= 1)&&(layer_num <= 6)) {
			sp_disp_info("set L%d as %d \n", layer_num, layer_val);
			DRV_DMIX_Layer_Mode_Set(layer_num,  layer_val);
		}
		else if (layer_num == 7) { //off
			sp_disp_info("set all layer blended \n");
			/* DRV_DMIX_AlphaBlend / DRV_DMIX_Transparent / DRV_DMIX_Opacity */
			DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //nornal osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //nornal osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_VPP0); //nornal vpp0 layer
		}
		else if (layer_num == 8) { //dmix
			sp_disp_info("set layer for dmix \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //hide osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //hide osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
		}
		else if (layer_num == 9) { //vscl
			sp_disp_info("set layer for vscl \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //show vpp0 layer
		}
		else if (layer_num == 10) { //ddfch
			sp_disp_info("set layer for ddfch \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //nornal osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //nornal osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //nornal vpp0 layer
		}
		else if (layer_num == 11) { //vppdma
			sp_disp_info("set layer for vppdma \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //show vpp0 layer		
		}
		else if (layer_num == 12) { //osd0
			sp_disp_info("set layer for osd0 \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_OSD0); //show osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
		}
		else if (layer_num == 13) { //osd1
			sp_disp_info("set layer for osd1 \n");
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_OSD1); //show osd1 layer
			DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
		}
		else {
			sp_disp_info("show layer info: \n");
			DRV_DMIX_Layer_Info();
		}
	}
	else if (!strncasecmp(tmpbuf, "fc", 2))
	{
		int mode = 0;
		int flow = 0;
		int width = 0, height = 0;

		tmpbuf = _mon_readint(tmpbuf + 2, (int *)&mode);
		tmpbuf = _mon_readint(tmpbuf, (int *)&flow);

		if (flow >= 1 ) {
			DRV_DVE_SetMode(mode);

			switch (mode)
			{
				default:
				case 0:
					sp_disp_info("Set 480P 59.94Hz \n");
					SetTGEN.fmt = DRV_FMT_480P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					width = 720;
					height = 480;
					break;
				case 1:
					sp_disp_info("Set 576P 50Hz \n");
					SetTGEN.fmt = DRV_FMT_576P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					width = 720;
					height = 576;
					break;
				case 2:
					sp_disp_info("Set 720P 59.94Hz \n");
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					width = 1280;
					height = 720;
					break;
				case 3:
					sp_disp_info("Set 720P 50Hz \n");
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					width = 1280;
					height = 720;
					break;
				case 4:
					sp_disp_info("Set 1080P 59.94Hz \n");
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					width = 1920;
					height = 1080;
					break;
				case 5:
					sp_disp_info("Set 1080P 50Hz \n");
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					width = 1920;
					height = 1080;
					break;
				case 6:
					sp_disp_info("Set 1080P 24Hz \n");
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_24Hz;
					width = 1920;
					height = 1080;
					break;
				case 7:
					sp_disp_info("user mode unsupport\n");
					break;
			}
	
			ret = DRV_TGEN_Set(&SetTGEN);
			if (ret != DRV_SUCCESS)
				sp_disp_err("TGEN Set failed, ret = %d\n", ret);
			
		}
		if (flow >= 2 ) {
			ret = vscl_setting(0, 0, width, height, width, height);
			if (ret != DRV_SUCCESS)
				sp_disp_err("VSCL Set failed, ret = %d\n", ret);		
		}
		if (flow >= 6 ) {
			ret = ddfch_setting(0x120000, 0x120000+ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, width, height, DDFCH_FMT_HDMI);
			if (ret != DRV_SUCCESS)
				sp_disp_err("DDFCH Set failed, ret = %d\n", ret);	
		}	
		if (flow >= 3 ) {
			ret = vppdma_setting(0x20300000, 0x20300000+VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT, width, height, VPPDMA_FMT_HDMI);
			if (ret != DRV_SUCCESS)
				sp_disp_err("VPPDMA Set failed, ret = %d\n", ret);
		}
		if (flow >= 4 ) {
			ret = osd0_setting(0x20200000, width, height, DRV_OSD_REGION_FORMAT_YUY2);
			if (ret != DRV_SUCCESS)
				sp_disp_err("OSD0 Set failed, ret = %d\n", ret);
		}
		if (flow >= 5 ) {
			ret = osd1_setting(0x20100000, width, height, DRV_OSD_REGION_FORMAT_YUY2);
			if (ret != DRV_SUCCESS)
				sp_disp_err("OSD1 Set failed, ret = %d\n", ret);	
		}
			
		//DRV_TGEN_Reset();			
		
	}
	else if (!strncasecmp(tmpbuf, "switch", 6)) {
		int path_sw = 0;
		
		tmpbuf = _mon_readint(tmpbuf + 6, (int *)&path_sw);
		
		if(path_sw == 0) {
			sp_disp_info("turn off osd0 \n");
			DRV_OSD0_off();
		}
		else if(path_sw == 1) {
			sp_disp_info("switch to osd0  \n");
			DRV_OSD0_on();
			DRV_OSD1_off();
			//vpp_path_select(DRV_FROM_OSD0);
			vpp_path_select_flag(DRV_FROM_OSD0);
		}
		else if(path_sw == 2) {
			sp_disp_info("switch to vppdma! \n");
			DRV_OSD0_off();
			DRV_DDFCH_off();
			DRV_VPPDMA_on();
			//DRV_DDFCH_off();
			//vpp_path_select(DRV_FROM_VPPDMA);
			vpp_path_select_flag(DRV_FROM_VPPDMA);
		}		
		else if(path_sw == 3) {
			sp_disp_info("switch to ddfch! \n");
			DRV_OSD0_off();
			DRV_VPPDMA_off();
			DRV_DDFCH_on();	
			//DRV_VPPDMA_off();
			//vpp_path_select(DRV_FROM_DDFCH);
			vpp_path_select_flag(DRV_FROM_DDFCH);
		}
	}
	else if (!strncasecmp(tmpbuf, "path", 4)) {
		tmpbuf = _mon_skipspace(tmpbuf + 4);
		
		if (!strncasecmp(tmpbuf, "osd0", 4)) {
			sp_disp_info("vpp_path_select : osd0 \n");
			vpp_path_select(DRV_FROM_OSD0);
		}
		else if (!strncasecmp(tmpbuf, "ddfch", 5)) {
			sp_disp_info("vpp_path_select : ddfch \n");
			vpp_path_select(DRV_FROM_DDFCH);
		}
		else if (!strncasecmp(tmpbuf, "vppdma", 6)) {
			sp_disp_info("vpp_path_select : vppdma \n");
			vpp_path_select(DRV_FROM_VPPDMA);
		}
	}
	else if (!strncasecmp(tmpbuf, "ddfch", 5)) {
		tmpbuf = _mon_skipspace(tmpbuf + 5);
		
		if (!strncasecmp(tmpbuf, "init10", 6)) {
			sp_disp_info("ddfch : init10 : w=720/h=480 \n");
			DRV_DDFCH_Init10(); //G185 21
		}
		else if (!strncasecmp(tmpbuf, "init11", 6)) {
			sp_disp_info("ddfch : init11 : ddfch line pitch \n");
			DRV_DDFCH_Init11(); //G185 20
		}		
		else if (!strncasecmp(tmpbuf, "init2", 5)) {
			sp_disp_info("ddfch : init2 : ddfch en \n");
			DRV_DDFCH_Init2(); //G185 2
		}
		else if (!strncasecmp(tmpbuf, "init3", 5)) {
			sp_disp_info("ddfch : init3 : ddfch color bar \n");
			DRV_DDFCH_Init3(); //G185 28
			//DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 1); //G185.28 = 0x80801013
		}
		else if (!strncasecmp(tmpbuf, "init4", 5)) {
			sp_disp_info("ddfch : init4 : vppdma off \n");
			DRV_DDFCH_Init4(); //G186 1 0x28 vppdma off
		}	
		else if (!strncasecmp(tmpbuf, "init5", 5)) {
			sp_disp_info("ddfch : init5 : vppdma on \n");
			DRV_DDFCH_Init5(); //G186 1 0x28 vppdma on
		}
		else if (!strncasecmp(tmpbuf, "init6", 5)) {
			sp_disp_info("ddfch : init6 : ddfch dis \n");
			DRV_DDFCH_Init6(); //G185 2 0xc0 ddfch dis
		}		
		else if (!strncasecmp(tmpbuf, "inita", 5)) {
			sp_disp_info("ddfch : inita : all process \n");
			vpp_path_select(DRV_FROM_VPPDMA); //G187 1 0x221f
			DRV_DDFCH_Init10(); //G185 21
			DRV_DDFCH_Init11(); //G185 20
			DRV_DDFCH_Init2(); //G185 2
			DRV_DDFCH_Init3(); //G185 28
			DRV_DDFCH_Init4(); //G186 1 0x28
		}				
		
	}	
	else if (!strncasecmp(tmpbuf, "bist", 4))
	{
		int bist_mode = 0;
		
		tmpbuf = _mon_skipspace(tmpbuf + 4);
		
		if (!strncasecmp(tmpbuf, "dve", 3)) {
			tmpbuf = _mon_readint(tmpbuf + 3, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp dve bist en \n");
				DRV_DVE_SetColorbar(1);
			}
			else {
				sp_disp_info("disp dve bist dis \n");
				DRV_DVE_SetColorbar(0);
			}
		}
		else if (!strncasecmp(tmpbuf, "dmix", 4)) {
			tmpbuf = _mon_readint(tmpbuf + 4, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp dmix bist en (hor) \n");
				DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_H_COLORBAR, 0x29f06e, 0);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp dmix bist en (ver) \n");
				DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_V_COLORBAR, 0x29f06e, 0);
			}
			else if (bist_mode == 3) {
				sp_disp_info("disp dmix bist en (snow) \n");
				DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_SNOW, 0x29f06e, 0);
			}
			else {
				sp_disp_info("disp dmix bist dis \n");
				DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x000000, 0);
			}
		}
		else if (!strncasecmp(tmpbuf, "osd0", 4))
		{
			tmpbuf = _mon_readint(tmpbuf + 4, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp osd0 bist en \n");
				DRV_OSD_SetColorbar(DRV_OSD0, 1);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp osd0 bist en (bor)\n");
				DRV_OSD_SetColorbar(DRV_OSD0, 2);
			}
			else {
				sp_disp_info("disp osd0 bist dis \n");
				DRV_OSD_SetColorbar(DRV_OSD0, 0);
			}
		}
		else if (!strncasecmp(tmpbuf, "osd1", 4))
		{
			tmpbuf = _mon_readint(tmpbuf + 4, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp osd1 bist en \n");
				DRV_OSD_SetColorbar(DRV_OSD1, 1);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp osd1 bist en (bor)\n");
				DRV_OSD_SetColorbar(DRV_OSD1, 2);
			}
			else {
				sp_disp_info("disp osd1 bist dis \n");
				DRV_OSD_SetColorbar(DRV_OSD1, 0);
			}
		}
		else if (!strncasecmp(tmpbuf, "vscl", 4))
		{
			tmpbuf = _mon_readint(tmpbuf + 4, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp vscl bist en \n");
				DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA, 1);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp vscl bist en (bor) \n");
				DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA, 2);
			}
			else {
				sp_disp_info("disp vscl bist dis \n");
				DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA, 0);
			}
		}
		else if (!strncasecmp(tmpbuf, "ddfch", 5))
		{
			tmpbuf = _mon_readint(tmpbuf + 5, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp ddfch bist en \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 1);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp ddfch bist en(bor) \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 2);
			}
			else if (bist_mode == 3) {
				sp_disp_info("disp ddfch bist en(half1) \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 3);
			}
			else {
				sp_disp_info("disp ddfch bist en(half2) \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 0);
			}
		}
		else if (!strncasecmp(tmpbuf, "vppdma", 6))
		{
			tmpbuf = _mon_readint(tmpbuf + 6, (int *)&bist_mode);
			if (bist_mode == 1) {
				sp_disp_info("disp vppdma bist en \n");
				DRV_VPP_SetColorbar(DRV_FROM_VPPDMA, 1);
			}
			else if (bist_mode == 2) {
				sp_disp_info("disp vppdma bist en (bor)\n");
				DRV_VPP_SetColorbar(DRV_FROM_VPPDMA, 2);
			}
			else {
				sp_disp_info("disp vppdma bist dis \n");
				DRV_VPP_SetColorbar(DRV_FROM_VPPDMA, 0);
			}
		}
	}
	else if (!strncasecmp(tmpbuf, "color", 5))
	{
		tmpbuf = _mon_skipspace(tmpbuf + 5);
		
		if (!strncasecmp(tmpbuf, "off", 3)) {
			sp_disp_info("disp all bist dis \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //nornal osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //nornal osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_VPP0); //nornal vpp0 layer
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 0);
			DRV_OSD_SetColorbar(DRV_OSD1, 0);
			DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA,0);
			//DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 0);
			DRV_VPP_SetColorbar(DRV_FROM_VPPDMA, 0);
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "dve", 3)) {
			sp_disp_info("disp dve bist en \n");
			DRV_DVE_SetColorbar(1);
		}
		else if (!strncasecmp(tmpbuf, "dmix", 4)) {
			sp_disp_info("disp dmix bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //hide osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //hide osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
			//DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_V_COLORBAR, 0x29f06e, 0);
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_H_COLORBAR, 0x29f06e, 0);
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "osd0", 4))
		{
			sp_disp_info("disp osd0 bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_OSD0); //show osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 1);
			DRV_OSD_SetColorbar(DRV_OSD1, 0);
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "osd1", 4))
		{
			sp_disp_info("disp osd1 bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_OSD1); //show osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_VPP0); //hide vpp0 layer
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 0);
			DRV_OSD_SetColorbar(DRV_OSD1, 1);
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "vscl", 4))
		{
			sp_disp_info("disp vscl bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //show vpp0 layer
			///DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //show osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //show osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_VPP0); //show vpp0 layer			
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 0);
			DRV_OSD_SetColorbar(DRV_OSD1, 0);
			DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA, 1); //default setting from vppdma
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "ddfch", 5))
		{
			sp_disp_info("disp ddfch bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //nornal osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //nornal osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //nornal vpp0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //show osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //show osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_VPP0); //show vpp0 layer			
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 0);
			DRV_OSD_SetColorbar(DRV_OSD1, 0);
			DRV_VSCL_SetColorbar(DRV_FROM_DDFCH,0);
			DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 1);
			
			DRV_TGEN_Reset();	
		}
		else if (!strncasecmp(tmpbuf, "vppdma", 6))
		{
			sp_disp_info("disp vppdma bist en \n");
			DRV_DVE_SetColorbar(0);
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD0); //hide osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Transparent , DRV_DMIX_OSD1); //hide osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_Opacity , DRV_DMIX_VPP0); //show vpp0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD0); //show osd0 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_OSD1); //show osd1 layer
			//DRV_DMIX_Layer_Set(DRV_DMIX_AlphaBlend , DRV_DMIX_VPP0); //show vpp0 layer
			DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_REGION, 0x00000000, 0);
			DRV_OSD_SetColorbar(DRV_OSD0, 0);
			DRV_OSD_SetColorbar(DRV_OSD1, 0);
			DRV_VSCL_SetColorbar(DRV_FROM_VPPDMA,0);
			DRV_VPP_SetColorbar(DRV_FROM_VPPDMA, 1);
			
			DRV_TGEN_Reset();	
		}
	}	
	#if 0
	else if (!strncasecmp(tmpbuf, "osd", 3))
	{
		tmpbuf = _mon_skipspace(tmpbuf + 3);

		if (!strncasecmp(tmpbuf, "info", 4))
			DRV_OSD_Info();
		else if (!strncasecmp(tmpbuf, "hdrs", 4))
			DRV_OSD_HDR_Show();
		else if (!strncasecmp(tmpbuf, "hdrw", 4))
		{
			int offset, value;
			tmpbuf = _mon_readint(tmpbuf + 4, (int *)&offset);
			tmpbuf = _mon_readint(tmpbuf, (int *)&value);
			DRV_OSD_HDR_Write(offset, value);
		}
	}
	#endif
	else
		sp_disp_err("unknown command:%s\n", tmpbuf);

	(void)tmpbuf;
}

static ssize_t _write_flush_proc(struct file *filp, const char __user *buffer, size_t len, loff_t *f_pos)
{
	char pbuf[MON_CMD_LEN];
	char *tmpbuf = pbuf;

	if (len > sizeof(pbuf))
	{
		sp_disp_err("intput error len:%d!\n", (int)len);
		return -ENOSPC;
	}

	if (copy_from_user(tmpbuf, buffer, len))
	{
		sp_disp_err("intput error!\n");
		return -EFAULT;
	}

	if (len == 0)
		tmpbuf[len] = '\0';
	else
		tmpbuf[len - 1] = '\0';

	if (!strncasecmp(tmpbuf, "dispmon", 7))
	{
		_debug_cmd(tmpbuf + 7);
	}

	return len;
}
#endif

static void _display_destory_irq(struct platform_device *pdev)
{
	int i, irq;
	int num_irq = of_irq_count(pdev->dev.of_node);

	for (i = 0; i < num_irq; ++i) {
		irq = platform_get_irq(pdev, i);

		devm_free_irq(&pdev->dev, irq, pdev);
	}
}

static int _display_init_irq(struct platform_device *pdev)
{
	int irq;
	int num_irq = of_irq_count(pdev->dev.of_node);
	UINT32 ret;

	sp_disp_info("irq num:%d\n", num_irq);

	irq = platform_get_irq(pdev, 0);
	if (irq != -ENXIO) {
		ret = devm_request_irq(&pdev->dev, irq, _display_irq_field_start, IRQF_TRIGGER_RISING, "sp_disp_irq_fs", pdev);
		if (ret) {
			sp_disp_err("request_irq error: %d, irq_num: %d\n", ret, irq);
			goto ERROR;
		}
	} else {
		sp_disp_err("platform_get_irq error\n");
		goto ERROR;
	}

	irq = platform_get_irq(pdev, 1);
	if (irq != -ENXIO) {
		ret = devm_request_irq(&pdev->dev, irq, _display_irq_field_end, IRQF_TRIGGER_RISING, "sp_disp_irq_fe", pdev);
		if (ret) {
			sp_disp_err("request_irq error: %d, irq_num: %d\n", ret, irq);
			goto ERROR;
		}
	} else {
		sp_disp_err("platform_get_irq error\n");
		goto ERROR;
	}

	irq = platform_get_irq(pdev, 2);
	if (irq != -ENXIO) {
		ret = devm_request_irq(&pdev->dev, irq, _display_irq_field_int1, IRQF_TRIGGER_RISING, "sp_disp_irq_int1", pdev);
		if (ret) {
			sp_disp_err("request_irq error: %d, irq_num: %d\n", ret, irq);
			goto ERROR;
		}
	} else {
		sp_disp_err("platform_get_irq error\n");
		goto ERROR;
	}

	irq = platform_get_irq(pdev, 3);
	if (irq != -ENXIO) {
		ret = devm_request_irq(&pdev->dev, irq, _display_irq_field_int2, IRQF_TRIGGER_RISING, "sp_disp_irq_int2", pdev);
		if (ret) {
			sp_disp_err("request_irq error: %d, irq_num: %d\n", ret, irq);
			goto ERROR;
		}
	} else {
		sp_disp_err("platform_get_irq error\n");
		goto ERROR;
	}

	return 0;
ERROR:
	return -1;
}

static void _display_destory_clk(struct platform_device *pdev)
{
	struct sp_disp_device *disp_dev = platform_get_drvdata(pdev);

	clk_disable_unprepare(disp_dev->tgen_clk);
	clk_disable_unprepare(disp_dev->dmix_clk);
	clk_disable_unprepare(disp_dev->osd0_clk);
	clk_disable_unprepare(disp_dev->gpost0_clk);
	clk_disable_unprepare(disp_dev->osd1_clk);
	clk_disable_unprepare(disp_dev->gpost1_clk);
	clk_disable_unprepare(disp_dev->vpost_clk);
	clk_disable_unprepare(disp_dev->ddfch_clk);
	clk_disable_unprepare(disp_dev->vppdma_clk);
	clk_disable_unprepare(disp_dev->vscl_clk);
	clk_disable_unprepare(disp_dev->dve_clk);
	clk_disable_unprepare(disp_dev->hdmi_clk);
}

static int _display_init_clk(struct platform_device *pdev)
{
	struct sp_disp_device *disp_dev = platform_get_drvdata(pdev);
	int ret;

	disp_dev->tgen_clk = devm_clk_get(&pdev->dev, "DISP_TGEN");
	if (IS_ERR(disp_dev->tgen_clk))
		return PTR_ERR(disp_dev->tgen_clk);
	ret = clk_prepare_enable(disp_dev->tgen_clk);
	if (ret)
		return ret;

	disp_dev->dmix_clk = devm_clk_get(&pdev->dev, "DISP_DMIX");
	if (IS_ERR(disp_dev->dmix_clk))
		return PTR_ERR(disp_dev->dmix_clk);
	ret = clk_prepare_enable(disp_dev->dmix_clk);
	if (ret)
		return ret;

	disp_dev->osd0_clk = devm_clk_get(&pdev->dev, "DISP_OSD0");
	if (IS_ERR(disp_dev->osd0_clk))
		return PTR_ERR(disp_dev->osd0_clk);
	ret = clk_prepare_enable(disp_dev->osd0_clk);
	if (ret)
		return ret;

	disp_dev->gpost0_clk = devm_clk_get(&pdev->dev, "DISP_GPOST0");
	if (IS_ERR(disp_dev->gpost0_clk))
		return PTR_ERR(disp_dev->gpost0_clk);
	ret = clk_prepare_enable(disp_dev->gpost0_clk);
	if (ret)
		return ret;

	disp_dev->osd1_clk = devm_clk_get(&pdev->dev, "DISP_OSD1");
	if (IS_ERR(disp_dev->osd1_clk))
		return PTR_ERR(disp_dev->osd1_clk);
	ret = clk_prepare_enable(disp_dev->osd1_clk);
	if (ret)
		return ret;

	disp_dev->gpost1_clk = devm_clk_get(&pdev->dev, "DISP_GPOST1");
	if (IS_ERR(disp_dev->gpost1_clk))
		return PTR_ERR(disp_dev->gpost1_clk);
	ret = clk_prepare_enable(disp_dev->gpost1_clk);
	if (ret)
		return ret;
		
	disp_dev->vpost_clk = devm_clk_get(&pdev->dev, "DISP_VPOST");
	if (IS_ERR(disp_dev->vpost_clk))
		return PTR_ERR(disp_dev->vpost_clk);
	ret = clk_prepare_enable(disp_dev->vpost_clk);
	if (ret)
		return ret;

	disp_dev->ddfch_clk = devm_clk_get(&pdev->dev, "DISP_DDFCH");
	if (IS_ERR(disp_dev->ddfch_clk))
		return PTR_ERR(disp_dev->ddfch_clk);
	ret = clk_prepare_enable(disp_dev->ddfch_clk);
	if (ret)
		return ret;

	disp_dev->vppdma_clk = devm_clk_get(&pdev->dev, "DISP_VPPDMA");
	if (IS_ERR(disp_dev->vppdma_clk))
		return PTR_ERR(disp_dev->vppdma_clk);
	ret = clk_prepare_enable(disp_dev->vppdma_clk);
	if (ret)
		return ret;

	disp_dev->vscl_clk = devm_clk_get(&pdev->dev, "DISP_VSCL");
	if (IS_ERR(disp_dev->vscl_clk))
		return PTR_ERR(disp_dev->vscl_clk);
	ret = clk_prepare_enable(disp_dev->vscl_clk);
	if (ret)
		return ret;

	disp_dev->dve_clk = devm_clk_get(&pdev->dev, "DISP_DVE");
	if (IS_ERR(disp_dev->dve_clk))
		return PTR_ERR(disp_dev->dve_clk);
	ret = clk_prepare_enable(disp_dev->dve_clk);
	if (ret)
		return ret;

	disp_dev->hdmi_clk = devm_clk_get(&pdev->dev, "DISP_HDMI");
	if (IS_ERR(disp_dev->hdmi_clk))
		return PTR_ERR(disp_dev->hdmi_clk);
	ret = clk_prepare_enable(disp_dev->hdmi_clk);
	if (ret)
		return ret;

	return 0;
}

static int sp_disp_get_register_base(struct platform_device *pdev, void **membase, int num)
{
	struct resource *res_mem;
	void __iomem *p;

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, num);
	if (IS_ERR(res_mem)) {
		sp_disp_err("Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(res_mem);
	}

	p = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR((const void *)p)) {
		sp_disp_err("regbase error\n");
		return PTR_ERR(p);
	}

	//sp_disp_dbg("ioremap address = 0x%08x\n", (unsigned int)p);
	//printk(KERN_INFO "ioremap address = 0x%08x\n", (unsigned int)p);
	printk(KERN_INFO "ioremap address \n");
	
	*membase = p;

	return 0;
}

#if 0//def SP_DISP_V4L2_SUPPORT
static int sp_disp_initialize(struct device *dev, struct sp_disp_device *disp_dev)
{
	//int ret;

	sp_disp_dbg("[%s:%d] sp_disp_initialize \n", __FUNCTION__, __LINE__);

	if (!disp_dev || !dev) {
		sp_disp_err("Null device pointers.\n");
		return -ENODEV;
	}

	// Register V4L2 device.
//	ret = v4l2_device_register(dev, &disp_dev->v4l2_dev);
//	if (ret) {
//		sp_disp_err("Unable to register v4l2 device!\n");
//		goto err_v4l2_register;
//	}

	// Initialize locks.
	spin_lock_init(&disp_dev->dma_queue_lock);
	mutex_init(&disp_dev->lock);

	return 0;

//err_v4l2_register:
//	v4l2_device_unregister(&disp_dev->v4l2_dev);
//	kfree(disp_dev);

//	return ret;
}
#endif

static int sp_disp_set_osd_resolution(struct device *dev, struct sp_disp_device *disp_dev)
{

	sp_disp_info("set osd resolution \n");

#if (OSD0_FETCH_EN == 1) //OSD0 setting
	sp_disp_info("osd0_data_array %px \n",osd0_data_array);
	#if (OSD0_FMT_HDMI == 0x2) //0x2:8bpp
	sp_disp_info("disp_osd0_8bpp_pal_grey %px \n",disp_osd0_8bpp_pal_grey);
	//sp_disp_info("disp_osd0_8bpp_pal_color %px \n",disp_osd0_8bpp_pal_color);
	#else
	sp_disp_info("osd0_header_array %px \n",osd0_header_array);
	#endif
	sp_disp_info("virt_to_phys(osd0_data_array) %ld \n",virt_to_phys(osd0_data_array));
	#if (OSD0_FMT_HDMI == 0x2) //0x2:8bpp
	sp_disp_info("virt_to_phys(disp_osd0_8bpp_pal_grey) %ld \n",virt_to_phys(disp_osd0_8bpp_pal_grey));
	//sp_disp_info("virt_to_phys(disp_osd0_8bpp_pal_color) %ld \n",virt_to_phys(disp_osd0_8bpp_pal_color));
	#else
	sp_disp_info("virt_to_phys(osd0_header_array) %ld \n",virt_to_phys(osd0_header_array));
	#endif

	// copy osd fetch data to 0x21000000
	osd0_data_ptr = ioremap(0x21000000, sizeof(osd0_data_array));
	memcpy(osd0_data_ptr, osd0_data_array, sizeof(osd0_data_array));
	sp_disp_info("osd0_data_array size %ld \n", sizeof(osd0_data_array));

	#if (OSD0_FMT_HDMI == 0x2) //0x2:8bpp
	osd0_setting(virt_to_phys(disp_osd0_8bpp_pal_grey), OSD0_WIDTH, OSD0_HEIGHT, OSD0_FMT_HDMI);
	//osd0_setting(virt_to_phys(disp_osd0_8bpp_pal_color), OSD0_WIDTH, OSD0_HEIGHT, OSD0_FMT_HDMI);
	#else
	//the rest setting
	osd0_setting(virt_to_phys(osd0_header_array), OSD0_WIDTH, OSD0_HEIGHT, OSD0_FMT_HDMI);
	#endif
#endif

#if (OSD1_FETCH_EN == 1) //OSD1 setting
	sp_disp_info("osd1_data_array %px \n",osd1_data_array);
	#if (OSD1_FMT_HDMI == 0x2) //0x2:8bpp
	sp_disp_info("disp_osd1_8bpp_pal_grey %px \n",disp_osd1_8bpp_pal_grey);
	//sp_disp_info("disp_osd1_8bpp_pal_color %px \n",disp_osd1_8bpp_pal_color);
	#else
	sp_disp_info("osd1_header_array %px \n",osd1_header_array);
	#endif
	sp_disp_info("virt_to_phys(osd1_data_array) %ld \n",virt_to_phys(osd1_data_array));
	#if (OSD1_FMT_HDMI == 0x2) //0x2:8bpp
	sp_disp_info("virt_to_phys(disp_osd1_8bpp_pal_grey) %ld \n",virt_to_phys(disp_osd1_8bpp_pal_grey));
	//sp_disp_info("virt_to_phys(disp_osd1_8bpp_pal_color) %ld \n",virt_to_phys(disp_osd1_8bpp_pal_color));
	#else
	sp_disp_info("virt_to_phys(osd1_header_array) %ld \n",virt_to_phys(osd1_header_array));
	#endif

	// copy osd fetch data to 0x22000000
	osd1_data_ptr = ioremap(0x22000000, sizeof(osd1_data_array));
	memcpy(osd1_data_ptr, osd1_data_array, sizeof(osd1_data_array));
	sp_disp_info("osd1_data_array size %ld \n", sizeof(osd1_data_array));

	#if (OSD1_FMT_HDMI == 0x2) //0x2:8bpp
	osd1_setting(virt_to_phys(disp_osd1_8bpp_pal_grey), OSD1_WIDTH, OSD1_HEIGHT, OSD1_FMT_HDMI);
	//osd1_setting(virt_to_phys(disp_osd1_8bpp_pal_color), OSD1_WIDTH, OSD1_HEIGHT, OSD1_FMT_HDMI);
	#else
	//the rest setting
	osd1_setting(virt_to_phys(osd1_header_array), OSD1_WIDTH, OSD1_HEIGHT, OSD1_FMT_HDMI);
	#endif
#endif

	return 0;

}

static int sp_disp_set_vpp_resolution(struct device *dev, struct sp_disp_device *disp_dev)
{

	sp_disp_dbg("set vpp resolution \n");

	vscl_setting(0, 0, VPP_WIDTH_IN, VPP_HEIGHT_IN, VPP_WIDTH_OUT, VPP_HEIGHT_OUT);
		
	#if (VPP_PATH_SEL == 0) //(From VPPDMA)
	vpp_path_select(DRV_FROM_VPPDMA); //default setting
	//vpp_path_select_flag(DRV_FROM_VPPDMA); //default setting
	#elif (VPP_PATH_SEL == 1) //(From OSD0)
	vpp_path_select(DRV_FROM_OSD0);
	//vpp_path_select_flag(DRV_FROM_OSD0);
	#elif (VPP_PATH_SEL == 2) //(From DDFCH)
	vpp_path_select(DRV_FROM_DDFCH);
	//vpp_path_select_flag(DRV_FROM_DDFCH);
	#endif
	
	#if (VPPDMA_FETCH_EN == 1)
	sp_disp_info("vppdma_data_array %px \n",vppdma_data_array);
	sp_disp_info("pa 0x%08x \n", (u32)pa);
	pa = __pa(vppdma_data_array);
	sp_disp_info("__pa(vppdma_data_array)1 0x%08x \n", (u32)pa);
	pa &= ~0x80000000;
	sp_disp_info("__pa(vppdma_data_array)2 0x%08x \n", (u32)pa);
	vppdma_setting(pa, pa + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	
	//vppdma_data_ptr = ioremap(0x24000000, sizeof(vppdma_data_array));
	//memcpy(vppdma_data_ptr, vppdma_data_array, sizeof(vppdma_data_array));
	//vppdma_setting(0x24000000, 0x24000000 + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	//vppdma_setting(virt_to_phys(vppdma_data_array), virt_to_phys((vppdma_data_array + VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT)), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	sp_disp_info("virt_to_phys(vppdma_data_array) %ld \n",virt_to_phys(vppdma_data_array));
	#endif
	#if (DDFCH_FETCH_EN == 1)
	vpp_yuv_ptr = ioremap(0x23000000, sizeof(vpp_yuv_array));
	memcpy(vpp_yuv_ptr, vpp_yuv_array, sizeof(vpp_yuv_array));
	ddfch_setting(0x23000000, 0x23000000 + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	//ddfch_setting(virt_to_phys(vpp_yuv_array), virt_to_phys(vpp_yuv_array) + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	#endif

	return 0;

}

static int _display_probe(struct platform_device *pdev)
{
	struct sp_disp_device *disp_dev;
	int ret;
	DISP_REG_t *pTmpRegBase = NULL;

	#if 1 //size check
	sp_disp_info("sizeof(short) = %ld \n", sizeof(short));
	sp_disp_info("sizeof(int) = %ld \n", sizeof(int));
	sp_disp_info("sizeof(long) = %ld \n", sizeof(long));
	sp_disp_info("sizeof(long long) = %ld \n", sizeof(long long));
	
	sp_disp_info("sizeof(size_t) = %ld \n", sizeof(size_t));
	sp_disp_info("sizeof(off_t) = %ld \n", sizeof(off_t));
	sp_disp_info("sizeof(void *) = %ld \n", sizeof(void *));
	#endif
	
	sp_disp_info("[%s:%d] disp probe ... \n", __FUNCTION__, __LINE__);

	// Allocate memory for 'sp_disp_device'.
	disp_dev = kzalloc(sizeof(struct sp_disp_device), GFP_KERNEL);
	if (!disp_dev) {
		sp_disp_err("Failed to allocate memory!\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	#if 1
	disp_dev->pdev = &pdev->dev;
	#endif

	gDispWorkMem = disp_dev;
	
	// Set the driver data in platform device.
	platform_set_drvdata(pdev, disp_dev);

	ret = _display_init_irq(pdev);
	if (ret) {
		sp_disp_err("Error: %s, %d\n", __func__, __LINE__);
		return -1;
	}

	G0 = ioremap(0x9C000000, 32*4);
	G1 = ioremap(0x9C000080, 32*4);
	G4 = ioremap(0x9C000200, 32*4);
	
#if 1
	// PLL DIS Setting
	G4[15] = 0xffff005b;
	G4[16] = 0xffff2054;
	G4[17] = 0xffff8044;
	G4[18] = 0xffff02b8;

	//release clk and reset
	//G0[1] = 0x00000000;
	//G0[2] = 0x00000000;
	//G0[3] = 0x00000000;
	//G0[4] = 0x00000000;	
	G0[5] = 0x1c001c00;
	G0[6] = 0x01e101e1;
	G0[7] = 0x00030003;
	G0[8] = 0x00200020;
	G0[9] = 0xc040c040;
	G0[10] = 0x00070007;
	
	//G0[21] = 0x00000000;
	//G0[22] = 0x00000000;
	//G0[23] = 0x00000000;
	//G0[24] = 0x00000000;	
	G0[25] = 0x1c000000;
	G0[26] = 0x01e10000;
	G0[27] = 0x00030000;
	G0[28] = 0x00200000;
	G0[29] = 0xc0400000;
	G0[30] = 0x00070000;
#endif


	printk(KERN_INFO "MOON0 G0.00 0x%08x(%d) \n",G0[0],G0[0]);
	printk(KERN_INFO "MOON0 G0.01 0x%08x(%d) \n",G0[1],G0[1]);
	printk(KERN_INFO "MOON0 G0.02 0x%08x(%d) \n",G0[2],G0[2]);
	printk(KERN_INFO "MOON0 G0.03 0x%08x(%d) \n",G0[3],G0[3]);
	printk(KERN_INFO "MOON0 G0.04 0x%08x(%d) \n",G0[4],G0[4]);
	printk(KERN_INFO "MOON0 G0.05 0x%08x(%d) \n",G0[5],G0[5]);
	printk(KERN_INFO "MOON0 G0.06 0x%08x(%d) \n",G0[6],G0[6]);
	printk(KERN_INFO "MOON0 G0.07 0x%08x(%d) \n",G0[7],G0[7]);
	printk(KERN_INFO "MOON0 G0.08 0x%08x(%d) \n",G0[8],G0[8]);
	printk(KERN_INFO "MOON0 G0.09 0x%08x(%d) \n",G0[9],G0[9]);
	printk(KERN_INFO "MOON0 G0.10 0x%08x(%d) \n",G0[10],G0[10]);
	
	printk(KERN_INFO "MOON0 G0.21 0x%08x(%d) \n",G0[21],G0[21]);
	printk(KERN_INFO "MOON0 G0.22 0x%08x(%d) \n",G0[22],G0[22]);
	printk(KERN_INFO "MOON0 G0.23 0x%08x(%d) \n",G0[23],G0[23]);
	printk(KERN_INFO "MOON0 G0.24 0x%08x(%d) \n",G0[24],G0[24]);
	printk(KERN_INFO "MOON0 G0.25 0x%08x(%d) \n",G0[25],G0[25]);
	printk(KERN_INFO "MOON0 G0.26 0x%08x(%d) \n",G0[26],G0[26]);
	printk(KERN_INFO "MOON0 G0.27 0x%08x(%d) \n",G0[27],G0[27]);
	printk(KERN_INFO "MOON0 G0.28 0x%08x(%d) \n",G0[28],G0[28]);
	printk(KERN_INFO "MOON0 G0.29 0x%08x(%d) \n",G0[29],G0[29]);
	printk(KERN_INFO "MOON0 G0.30 0x%08x(%d) \n",G0[30],G0[30]);
	
	printk(KERN_INFO "MOON1 G1.00 0x%08x(%d) \n",G1[0],G1[0]);
	printk(KERN_INFO "MOON1 G1.01 0x%08x(%d) \n",G1[1],G1[1]);
	printk(KERN_INFO "MOON1 G1.02 0x%08x(%d) \n",G1[2],G1[2]);
	
	//printk(KERN_INFO "MOON4 G4.14 0x%08x(%d) \n",G4[14],G4[14]);
	printk(KERN_INFO "MOON4 G4.15 0x%08x(%d) \n",G4[15],G4[15]);
	printk(KERN_INFO "MOON4 G4.16 0x%08x(%d) \n",G4[16],G4[16]);
	printk(KERN_INFO "MOON4 G4.17 0x%08x(%d) \n",G4[17],G4[17]);
	printk(KERN_INFO "MOON4 G4.18 0x%08x(%d) \n",G4[18],G4[18]);
	//printk(KERN_INFO "MOON4 G4.31 0x%08x(%d) \n",G4[31],G4[31]);

	ret = sp_disp_get_register_base(pdev, (void**)&pTmpRegBase,0);
	if (ret) {
		return ret;
	}
	else
		printk(KERN_INFO "get_register_base ok \n");

	disp_dev->pHWRegBase = pTmpRegBase;

	ret = _display_init_clk(pdev);
	if (ret)
	{
		sp_disp_err("init clk error.\n");
		return ret;
	}
	else
		printk(KERN_INFO "_display_init_clk ok \n");

	DRV_DMIX_Init((void*)&pTmpRegBase->dmix);
	DRV_TGEN_Init((void*)&pTmpRegBase->tgen);
	DRV_DVE_Init((void*)&pTmpRegBase->dve);
	DRV_VPP_Init((void*)&pTmpRegBase->vpost, (void*)&pTmpRegBase->vscl, (void*)&pTmpRegBase->ddfch, (void*)&pTmpRegBase->vppdma);
	DRV_OSD_Init((void*)&pTmpRegBase->osd, (void*)&pTmpRegBase->gpost);
	DRV_OSD1_Init((void*)&pTmpRegBase->osd1, (void*)&pTmpRegBase->gpost1);

	DRV_DVE_dump();
	
	// DMIX setting
	/****************************************
	* BG: PTG
	* L1: VPP0
	* L5: OSD1
	* L6: OSD0
	*****************************************/
	#if 1
	DRV_DMIX_Layer_Init(DRV_DMIX_BG, DRV_DMIX_AlphaBlend, DRV_DMIX_PTG);
	//DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_Opacity, DRV_DMIX_VPP0);
	DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_AlphaBlend, DRV_DMIX_VPP0);
	DRV_DMIX_Layer_Init(DRV_DMIX_L5, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD1);
	DRV_DMIX_Layer_Init(DRV_DMIX_L6, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD0);
	#endif

#if 0//def SP_DISP_V4L2_SUPPORT
	ret = sp_disp_initialize(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}	
#endif

	ret = sp_disp_set_vpp_resolution(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}

	DRV_VPPDMA_dump();
	DRV_DDFCH_dump();
	DRV_VSCL_dump();

	ret = sp_disp_set_osd_resolution(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}

	//DRV_OSD0_dump();
	//DRV_OSD1_dump();

	#if 1
	DRV_OSD_INIT_OSD_Header();
	#endif
	
	sp_disp_info("[%s:%d] disp probe done \n", __FUNCTION__, __LINE__);
	//printk(KERN_INFO "disp probe done \n");
	
#ifdef SUPPORT_DEBUG_MON
	entry = proc_create(MON_PROC_NAME, S_IRUGO, NULL, &proc_fops);
#endif

	return 0;

err_alloc:
	printk(KERN_INFO "disp probe fail \n");
	kfree(disp_dev);
	return ret;
}

static int _display_remove(struct platform_device *pdev)
{
	#ifdef SP_DISP_V4L2_SUPPORT
	struct sp_disp_device *disp_dev = platform_get_drvdata(pdev);
	struct sp_disp_layer *disp_layer = NULL;
	int i;
	#endif

	//sp_disp_dbg("[%s:%d]\n", __FUNCTION__, __LINE__);
	printk(KERN_INFO "_display_remove \n");

	_display_destory_irq(pdev);
	_display_destory_clk(pdev);

#if (OSD0_FETCH_EN == 1)
	iounmap(osd0_data_ptr);
#endif
#if (OSD1_FETCH_EN == 1)
	iounmap(osd1_data_ptr);
#endif
#if (VPPDMA_FETCH_EN == 1)
	iounmap(vppdma_data_ptr);
#endif
#if (DDFCH_FETCH_EN == 1)
	iounmap(vpp_yuv_ptr);
#endif
	
	iounmap(G0);
	iounmap(G1);
	iounmap(G4);

#ifdef SUPPORT_DEBUG_MON
	if (entry)
		remove_proc_entry(MON_PROC_NAME, NULL);
#endif

	return 0;
}

static int _display_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sp_disp_device *disp_dev = platform_get_drvdata(pdev);
	//sp_disp_dbg("[%s:%d] suspend \n", __FUNCTION__, __LINE__);
	reset_control_assert(disp_dev->rstc);
		
	return 0;
}

static int _display_resume(struct platform_device *pdev)
{
	struct sp_disp_device *disp_dev = platform_get_drvdata(pdev);
	//sp_disp_dbg("[%s:%d] resume \n", __FUNCTION__, __LINE__);
	reset_control_deassert(disp_dev->rstc);

	return 0;
}

#ifdef SUPPORT_DEBUG_MON
static int set_debug_cmd(const char *val, const struct kernel_param *kp)
{
	_debug_cmd((char *)val);

	return 0;
}

static struct kernel_param_ops debug_param_ops = {
	.set = set_debug_cmd,
};

module_param_cb(debug, &debug_param_ops, NULL, 0644);
MODULE_PARM_DESC(debug,"disp debug filename.");
#endif

/**************************************************************************
 *                     P U B L I C   F U N C T I O N                      *
 **************************************************************************/

MODULE_DESCRIPTION("I143 Display Driver");
MODULE_AUTHOR("Hammer Hsieh <hammer.hsieh@sunplus.com>");
MODULE_LICENSE("GPL");

