// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
//#include <linux/dma-mapping.h>
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

#ifdef CONFIG_PM_RUNTIME_DISP
#include <linux/pm_runtime.h>
#endif

#include <media/sunplus/disp/i143/disp_osd.h>
#include <media/sunplus/disp/i143/display.h>

#include "reg_disp.h"
#include "hal_disp.h"
#include "disp_dve.h"
#include "disp_dmix.h"
#include "disp_tgen.h"
#include "disp_vpp.h"
//#include "disp_palette.h"

#ifdef TIMING_SYNC_720P60
#include <media/sunplus/hdmi/hdmitx.h>
#endif

#ifdef SP_DISP_V4L2_SUPPORT
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
//#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-dma-contig.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#endif
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define DISP_ALIGN(x, n)		(((x) + ((n)-1)) & ~((n)-1))

#ifdef SUPPORT_DEBUG_MON
	#define MON_PROC_NAME		"disp_mon"
	#define MON_CMD_LEN			(256)
#endif

#if (VPPDMA_FETCH_EN == 1) //VPPDMA fetch data
	#if ((VPPDMA_VPP_WIDTH == 640) && (VPPDMA_VPP_HEIGHT == 480))
		#if (VPPDMA_FMT_HDMI == 0) //RGB888
		unsigned char vppdma_data_array[640*480*3] __attribute__((aligned(1024))) = {
			0x00
		};
		#elif (VPPDMA_FMT_HDMI == 1) //RGB565
		unsigned char vppdma_data_array[640*480*2] __attribute__((aligned(1024))) = {
			0x00
		};
		#elif (VPPDMA_FMT_HDMI == 2) //YUV422_YUY2 (default fmt UYVY)
		unsigned char vppdma_data_array[640*480*2] __attribute__((aligned(1024))) = {
			0x00
		};
		#elif (VPPDMA_FMT_HDMI == 3) //YUV422_NV16
		unsigned char vppdma_data_array[640*480*2] __attribute__((aligned(1024))) = {
			0x00
		};
		#elif (VPPDMA_FMT_HDMI == 4) //YUV422_YUY2
		unsigned char vppdma_data_array[640*480*2] __attribute__((aligned(1024))) = {
			0x00
		};		
		#else //RGB888
		unsigned char vppdma_data_array[640*480*3] __attribute__((aligned(1024))) = {
			0x00
		};
		#endif
	#endif
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
		#elif (VPPDMA_FMT_HDMI == 4) //YUV422_YUY2
		unsigned char vppdma_data_array[720*480*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_UYVY_720x480.h"
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
		#elif (VPPDMA_FMT_HDMI == 4) //YUV422_YUY2
		unsigned char vppdma_data_array[1280*720*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_YUY2_1280x720.h"
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
		#elif (VPPDMA_FMT_HDMI == 4) //YUV422_YUY2
		unsigned char vppdma_data_array[1920*1080*2] __attribute__((aligned(1024))) = {
			#include "vpp_pattern/vppdma/yuv422_YUY2_1920x1080.h"
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

#ifdef SP_DISP_V4L2_SUPPORT
static unsigned int allocator = 0;
module_param(allocator, uint, 0444);
MODULE_PARM_DESC(allocator, " memory allocator selection, default is 0.\n"
			     "\t    0 == dma-contig\n"
			     "\t    1 == vmalloc");
#define DISP_NAME			"sp_disp"
#define DISP_OSD0_NAME		    "sp_disp_osd0"
#define DISP_OSD1_NAME		    "sp_disp_osd1"
#define DISP_VPP0_NAME		    "sp_disp_vpp0"

#if 0
#define print_List()
#else
static void print_List(struct list_head *head){
	struct list_head *listptr;
	struct videobuf_buffer *entry;

	sp_disp_dbg("********************************************************\n");
	sp_disp_dbg("(HEAD addr =  %p, next = %p, prev = %p)\n", head, head->next, head->prev);
	list_for_each(listptr, head) {
		entry = list_entry(listptr, struct videobuf_buffer, stream);
		sp_disp_dbg("list addr = %p | next = %p | prev = %p\n", &entry->stream, entry->stream.next,
			 entry->stream.prev);
	}
	sp_disp_dbg("********************************************************\n");
}
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

#ifdef TIMING_SYNC_720P60
extern int hdmitx_enable_display(int);
extern void hdmitx_set_timming(enum hdmitx_timing timing);
extern void hdmitx_get_timming(enum hdmitx_timing *timing);
enum hdmitx_timing hdmitx_cur_timing;
static const char * const hdmitx_res[] = {"480P60Hz", "576P50Hz", "720P60Hz", "1080P60Hz", "USER"};
#endif

extern void sp_disp_set_vpp_resolution_v4l2(struct sp_disp_device *disp_dev, int is_hdmi);

int g_disp_state = 1;
EXPORT_SYMBOL(g_disp_state);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct sp_disp_device *gDispWorkMem;

static struct of_device_id _display_ids[] = {
	{ .compatible = "sunplus,i143-display"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, _display_ids);

#ifdef CONFIG_PM_RUNTIME_DISP
static int sp_disp_runtime_suspend(struct device *dev)
{
	struct sp_disp_device *disp_dev = dev_get_drvdata(dev);
	
	sp_disp_dbg("runtime suppend \n");
	//reset_control_assert(disp_dev->rstc);
	
	#if 1
	// Disable 'dispplay' and 'hdmitx' clock.

	clk_disable(disp_dev->tgen_clk);
	clk_disable(disp_dev->dmix_clk);
	clk_disable(disp_dev->osd0_clk);
	clk_disable(disp_dev->gpost0_clk);
	clk_disable(disp_dev->osd1_clk);
	clk_disable(disp_dev->gpost1_clk);
	clk_disable(disp_dev->vpost_clk);
	clk_disable(disp_dev->ddfch_clk);
	clk_disable(disp_dev->vppdma_clk);
	clk_disable(disp_dev->vscl_clk);
	clk_disable(disp_dev->dve_clk);
	//clk_disable(disp_dev->hdmi_clk);

	#endif

	return 0;
}

static int sp_disp_runtime_resume(struct device *dev)
{
	struct sp_disp_device *disp_dev = dev_get_drvdata(dev);
	int ret;
	sp_disp_dbg("runtime resume \n");
	//reset_control_deassert(disp_dev->rstc);

	#if 1
	// Enable 'display' clock.
	ret = clk_prepare_enable(disp_dev->tgen_clk);
	if (ret) {
		sp_disp_err("Failed to enable tgen_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->dmix_clk);
	if (ret) {
		sp_disp_err("Failed to enable dmix_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->osd0_clk);
	if (ret) {
		sp_disp_err("Failed to enable osd0_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->gpost0_clk);
	if (ret) {
		sp_disp_err("Failed to enable gpost0_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->osd1_clk);
	if (ret) {
		sp_disp_err("Failed to enable osd1_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->gpost1_clk);
	if (ret) {
		sp_disp_err("Failed to enable gpost1_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->vpost_clk);
	if (ret) {
		sp_disp_err("Failed to enable vpost_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->ddfch_clk);
	if (ret) {
		sp_disp_err("Failed to enable ddfch_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->vppdma_clk);
	if (ret) {
		sp_disp_err("Failed to enable vppdma_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->vscl_clk);
	if (ret) {
		sp_disp_err("Failed to enable vscl_clk! \n");
	}
	ret = clk_prepare_enable(disp_dev->dve_clk);
	if (ret) {
		sp_disp_err("Failed to enable dve_clk! \n");
	}
	#if 0
	// Enable 'hdmitx' clock.
	ret = clk_prepare_enable(disp_dev->hdmi_clk);
	if (ret) {
		sp_disp_err("Failed to enable hdmi_clk! \n");
	}
	#endif	
	#endif

	return 0;
}
static const struct dev_pm_ops i143_disp_pm_ops = {
	.runtime_suspend = sp_disp_runtime_suspend,
	.runtime_resume  = sp_disp_runtime_resume,
};

#define sp_disp_pm_ops  (&i143_disp_pm_ops)
#endif

// platform driver
static struct platform_driver _display_driver = {
	.probe		= _display_probe,
	.remove		= _display_remove,
	.suspend	= _display_suspend,
	.resume		= _display_resume,
	.driver		= {
		.name	= "i143_display",
		.of_match_table	= of_match_ptr(_display_ids),
#ifdef CONFIG_PM_RUNTIME_DISP
		.pm			= sp_disp_pm_ops,
#endif
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

#ifdef SP_DISP_V4L2_SUPPORT
static int sp_disp_open(struct file *file)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct video_device *vdev = video_devdata(file);
	struct sp_disp_fh *fh;

	sp_disp_dbg("sp_disp_open \n");

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(disp_dev->pdev) < 0)
		goto out;
#endif

	/* Allocate memory for the file handle object */
	fh = kmalloc(sizeof(struct sp_disp_device), GFP_KERNEL);
	if (!fh) {
		sp_disp_err("Allocate memory fail\n");
		return -ENOMEM;
	}

	/* store pointer to fh in private_data member of file */
	file->private_data = fh;
	fh->disp_dev = disp_dev;
	v4l2_fh_init(&fh->fh, vdev);

	mutex_lock(&disp_dev->lock);
	/* Set io_allowed member to false */
	fh->io_allowed = 0;
	v4l2_fh_add(&fh->fh);
	mutex_unlock(&disp_dev->lock);

	return 0;

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(disp_dev->pdev);
	pm_runtime_put_autosuspend(disp_dev->pdev);
	return -ENOMEM;
#endif

}
static int sp_disp_release(struct file *file)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	int ret;
	
	sp_disp_dbg("sp_disp_release \n");
	
	mutex_lock(&disp_dev->lock);
	
	ret = _vb2_fop_release(file,NULL);
	
	mutex_unlock(&disp_dev->lock);

#ifdef CONFIG_PM_RUNTIME_DISP
	pm_runtime_put(disp_dev->pdev);		// Starting count timeout.
#endif

	return ret;
}

static const struct v4l2_file_operations sp_disp_fops = {
	.owner				= THIS_MODULE,
	.open				= sp_disp_open,
	.release			= sp_disp_release,
	.unlocked_ioctl		= video_ioctl2,
	.mmap 				= vb2_fop_mmap,
	.poll 				= vb2_fop_poll,
};

static const struct vb2_mem_ops *const sp_mem_ops[2] = {
	&vb2_dma_contig_memops,
	&vb2_vmalloc_memops,
};

static int sp_queue_setup(struct vb2_queue *vq, unsigned *nbuffers, unsigned *nplanes,
		       unsigned sizes[], struct device *alloc_devs[])
{
	/* Get the file handle object and layer object */
	struct sp_disp_layer *layer = vb2_get_drv_priv(vq);
	struct sp_disp_device *disp_dev = layer->disp_dev;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;	
	unsigned size = 0;

	sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, layer->video_dev.name);

	if(!(strcmp(DISP_OSD0_NAME, layer->video_dev.name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, layer->video_dev.name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, layer->video_dev.name)))
		dev_id = SP_DISP_DEVICE_2;

	if(disp_dev->dev[dev_id]) {
		size = layer->fmt.fmt.pix.sizeimage;
	}

	if (*nplanes) {
		if (sizes[0] < size) {
			return -EINVAL;
		}
		size = sizes[0];
	}	
	
	*nplanes = 1;
	sizes[0] = size;

	/* Store number of buffers allocated in numbuffer member */
	if ((vq->num_buffers + *nbuffers) < MIN_BUFFERS) {
		*nbuffers = MIN_BUFFERS - vq->num_buffers;
	}
		
	sp_disp_dbg("[%s:%d] count = %u, size = %u \n", __FUNCTION__, __LINE__, *nbuffers, sizes[0]);

	return 0;

}

static int sp_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vb2_queue *q = vb->vb2_queue;
	struct sp_disp_layer *layer = vb2_get_drv_priv(q);
	unsigned long size = layer->fmt.fmt.pix.sizeimage;
	unsigned long addr;

#ifdef CONFIG_PM_RUNTIME_DISP
	struct sp_disp_device *disp = layer->disp_dev;

	if (pm_runtime_get_sync(disp->pdev) < 0)
		goto out;

	pm_runtime_put(disp->pdev);		// Starting count timeout.
#endif

	sp_disp_dbg("[%s:%d] buf size = %ld \n", __FUNCTION__, __LINE__, size);

	vb2_set_plane_payload(vb, 0, layer->fmt.fmt.pix.sizeimage);

	if (vb2_get_plane_payload(vb, 0) > vb2_plane_size(vb, 0)) {
		sp_disp_err("Buffer is too small (%lu < %lu)!\n", vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	addr = vb2_dma_contig_plane_dma_addr(vb, 0);

	vbuf->field = layer->fmt.fmt.pix.field;

	return 0;

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(disp->pdev);
	pm_runtime_put_autosuspend(disp->pdev);
	return -ENOMEM;
#endif

}

static void sp_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vb2_queue *q = vb->vb2_queue;
	struct sp_disp_layer *layer = vb2_get_drv_priv(q);
	struct sp_disp_device *disp = layer->disp_dev;
	struct sp_disp_buffer *buf = container_of(vbuf, struct sp_disp_buffer, vb);
	unsigned long flags = 0;

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(disp->pdev) < 0)
		goto out;

	pm_runtime_put(disp->pdev);		// Starting count timeout.
#endif

	sp_disp_dbg("[%s:%d] buf queue \n", __FUNCTION__, __LINE__);
	
	// Add the buffer to the DMA queue.
	spin_lock_irqsave(&disp->dma_queue_lock, flags);
	list_add_tail(&buf->list, &layer->dma_queue);
	print_List(&layer->dma_queue);
	spin_unlock_irqrestore(&disp->dma_queue_lock, flags);

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(disp->pdev);
	pm_runtime_put_autosuspend(disp->pdev);
	//return -ENOMEM;
#endif

}

static int sp_start_streaming(struct vb2_queue *vq, unsigned count)
{
	struct sp_disp_layer *layer = vb2_get_drv_priv(vq);
	struct sp_disp_device *disp = layer->disp_dev;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct UI_FB_Info_t Info;
	unsigned long flags;
	unsigned long addr;

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(disp->pdev) < 0)
		goto out;
	
	pm_runtime_put(disp->pdev);		// Starting count timeout.
#endif

	if (layer->streaming) {
		sp_disp_dbg("Device has started streaming!\n");
		return -EBUSY;
	}

	layer->sequence = 0;

	if(!(strcmp(DISP_OSD0_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_0;
	}
	else if(!(strcmp(DISP_OSD1_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_1;
	}
	else if(!(strcmp(DISP_VPP0_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_2;
	}
	else
		sp_disp_dbg("[%s:%d] unknown Layer queue \n", __FUNCTION__, __LINE__);
	
	sp_disp_dbg("[%s:%d] Layer name = %s , count = %d \n", __FUNCTION__, __LINE__, layer->video_dev.name, count);

	spin_lock_irqsave(&disp->dma_queue_lock, flags);

	/* Get the next frame from the buffer queue */
	layer->cur_frm = list_entry(layer->dma_queue.next,
				struct sp_disp_buffer, list);

	// Remove buffer from the dma queue.
	list_del_init(&layer->cur_frm->list);

	addr = vb2_dma_contig_plane_dma_addr(&layer->cur_frm->vb.vb2_buf, 0);

	#ifdef I143_SYS_PORT
	addr &= ~0x80000000;
	#endif

	spin_unlock_irqrestore(&disp->dma_queue_lock, flags);

	if(dev_id == SP_DISP_DEVICE_0) {
		if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB32) {
			Info.UI_ColorFmt = 0xe;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB32 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ABGR32) {
			Info.UI_ColorFmt = 0xd;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ABGR32 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB444) {
			Info.UI_ColorFmt = 0xb;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB444 \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB444) {
			Info.UI_ColorFmt = 0xa;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB444 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB555) {
			Info.UI_ColorFmt = 0x9;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB555 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
			Info.UI_ColorFmt = 0x8;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB565 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			Info.UI_ColorFmt = 0x4;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_YUYV \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
			Info.UI_ColorFmt = 0x2;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_PAL8 \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
			Info.UI_ColorFmt = 0x2;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_GREY \n");
		}

		Info.UI_width = layer->fmt.fmt.pix.width;
		Info.UI_height = layer->fmt.fmt.pix.height;
		Info.UI_bufAddr = addr;

		sp_disp_dbg("[%s:%d] width = %d , height = %d \n", __FUNCTION__, __LINE__,Info.UI_width,Info.UI_height);
		sp_disp_dbg("[%s:%d] addr = 0x%08lx \n", __FUNCTION__, __LINE__,addr);

		#ifdef	SP_DISP_OSD_PARM
		DRV_OSD_SET_OSD_Header(&Info,0);
		#endif

	}
	else if(dev_id == SP_DISP_DEVICE_1) {
		if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB32) {
			Info.UI_ColorFmt = 0xe;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB32 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ABGR32) {
			Info.UI_ColorFmt = 0xd;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ABGR32 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB444) {
			Info.UI_ColorFmt = 0xb;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB444 \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB444) {
			Info.UI_ColorFmt = 0xa;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB444 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB555) {
			Info.UI_ColorFmt = 0x9;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_ARGB555 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
			Info.UI_ColorFmt = 0x8;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB565 \n");
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			Info.UI_ColorFmt = 0x4;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_YUYV \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
			Info.UI_ColorFmt = 0x2;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_PAL8 \n");
		}	
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
			Info.UI_ColorFmt = 0x2;
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_GREY \n");
		}

		Info.UI_width = layer->fmt.fmt.pix.width;
		Info.UI_height = layer->fmt.fmt.pix.height;
		Info.UI_bufAddr = addr;

		sp_disp_dbg("[%s:%d] width = %d , height = %d \n", __FUNCTION__, __LINE__,Info.UI_width,Info.UI_height);
		sp_disp_dbg("[%s:%d] addr = 0x%08lx \n", __FUNCTION__, __LINE__,addr);
		
		#ifdef	SP_DISP_OSD_PARM
		DRV_OSD_SET_OSD_Header(&Info,1);
		#endif
	}
	else if(dev_id == SP_DISP_DEVICE_2) {
		sp_disp_dbg("[%s:%d] width = %d , height = %d \n", __FUNCTION__, __LINE__,layer->fmt.fmt.pix.width,layer->fmt.fmt.pix.height);
		sp_disp_dbg("[%s:%d] addr = 0x%08lx \n", __FUNCTION__, __LINE__,addr);
#if DDFCH_FETCH_EN
		if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_NV12 \n");
			ddfch_setting(addr, addr + ALIGN(layer->fmt.fmt.pix.width, 128)*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 0);	
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV16) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_NV16 \n");
			ddfch_setting(addr, addr + ALIGN(layer->fmt.fmt.pix.width, 128)*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 1);
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_YUYV \n");
			ddfch_setting(addr, addr + ALIGN(layer->fmt.fmt.pix.width, 128)*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 2);
		}
#endif
#if VPPDMA_FETCH_EN
		if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB24 \n");
			vppdma_setting(addr, addr + layer->fmt.fmt.pix.width*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 0);
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_RGB565 \n");
			vppdma_setting(addr, addr + layer->fmt.fmt.pix.width*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 1);
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_UYVY \n");
			vppdma_setting(addr, addr + layer->fmt.fmt.pix.width*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 2);
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV16) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_NV16 \n");
			vppdma_setting(addr, addr + layer->fmt.fmt.pix.width*layer->fmt.fmt.pix.height, layer->fmt.fmt.pix.width, layer->fmt.fmt.pix.height, 3);
		}
		else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			sp_disp_dbg("set fmt = V4L2_PIX_FMT_YUYV \n");
			vppdma_setting(addr, addr + layer->fmt.fmt.pix.width*VPPDMA_VPP_HEIGHT, layer->fmt.fmt.pix.width, VPPDMA_VPP_HEIGHT, 4);
		}
#endif
	}

	layer->skip_first_int = 1;
	layer->streaming = 1;

	return 0;

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(disp->pdev);
	pm_runtime_put_autosuspend(disp->pdev);
	return -ENOMEM;
#endif

}

static void sp_stop_streaming(struct vb2_queue *vq)
{
	struct sp_disp_layer *layer = vb2_get_drv_priv(vq);
	struct sp_disp_device *disp = layer->disp_dev;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct sp_disp_buffer *buf;
	unsigned long flags;
	int is_hdmi = 1;
	#ifdef TIMING_SYNC_720P60
	DRV_SetTGEN_t SetTGEN;
	int mode = 0;
	#endif

	sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, layer->video_dev.name);

	if (!layer->streaming) {
		sp_disp_dbg("Device has stopped already!\n");
		return;
	}

	if(!(strcmp(DISP_OSD0_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_0;
		#ifdef	SP_DISP_OSD_PARM
		DRV_OSD_Clear_OSD_Header(0);
		#endif
		sp_disp_set_vpp_resolution_v4l2(disp, is_hdmi);
	}
	else if(!(strcmp(DISP_OSD1_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_1;
		#ifdef	SP_DISP_OSD_PARM
		DRV_OSD_Clear_OSD_Header(1);
		#endif
		sp_disp_set_vpp_resolution_v4l2(disp, is_hdmi);
	}
	else if(!(strcmp(DISP_VPP0_NAME, layer->video_dev.name))) {
		dev_id = SP_DISP_DEVICE_2;
		sp_disp_set_vpp_resolution_v4l2(disp, is_hdmi);

#ifdef TIMING_SYNC_720P60
			sp_disp_dbg("[%s:%d] get hdmitx resolution!!(S) \n", __FUNCTION__, __LINE__);
			sp_disp_dbg("[%s:%d] default hdmitx timing %s \n", __FUNCTION__, __LINE__, hdmitx_res[hdmitx_cur_timing]);

			hdmitx_set_timming(hdmitx_cur_timing);
			hdmitx_enable_display(1);

			if (hdmitx_cur_timing == 0) { //hdmitx 480P60Hz
				mode = 0;
			}
			else if (hdmitx_cur_timing == 1) { //hdmitx 576P50Hz
				mode = 1;
			}
			else if (hdmitx_cur_timing == 2) { //hdmitx 720P60Hz
				mode = 2;
			}
			else if (hdmitx_cur_timing == 3) { //hdmitx 1080P60Hz
				mode = 4;
			}
			else { //unknown setting
				;//TBD
			}

			DRV_DVE_SetMode(mode);

			switch (mode)
			{
				default:
				case 0:
					SetTGEN.fmt = DRV_FMT_480P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 1:
					SetTGEN.fmt = DRV_FMT_576P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 2:
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 3:
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 4:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 5:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 6:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_24Hz;
					break;
				case 7:
					sp_disp_dbg("user mode unsupport\n");
					break;
			}
			DRV_TGEN_Set(&SetTGEN);
			
			sp_disp_dbg("[%s:%d] get hdmitx resolution!!(E) \n", __FUNCTION__, __LINE__);
#endif

	}

	layer->streaming = 0;

	// Release all active buffers.
	spin_lock_irqsave(&disp->dma_queue_lock, flags);

	if (layer->cur_frm != NULL) {
		vb2_buffer_done(&layer->cur_frm->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	while (!list_empty(&layer->dma_queue)) {
		buf = list_entry(layer->dma_queue.next, struct sp_disp_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	spin_unlock_irqrestore(&disp->dma_queue_lock, flags);

#ifdef CONFIG_PM_RUNTIME_DISP
	pm_runtime_put(disp->pdev);		// Starting count timeout.
#endif

}

static const struct vb2_ops sp_video_qops = {
	.queue_setup            = sp_queue_setup,
	.buf_prepare            = sp_buf_prepare,
	.buf_queue              = sp_buf_queue,
	.start_streaming        = sp_start_streaming,
	.stop_streaming         = sp_stop_streaming,
	.wait_prepare           = vb2_ops_wait_prepare,
	.wait_finish            = vb2_ops_wait_finish
};


static int sp_vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *vcap)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct sp_disp_fh *fh = priv;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;

	if(!(strcmp(DISP_OSD0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_2;

	if(disp_dev->dev[dev_id]) {
		sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__,disp_dev->dev[dev_id]->video_dev.name);

		if(!(strcmp(DISP_OSD0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			strlcpy(vcap->driver, "SP Video Driver", sizeof(vcap->driver));
			strlcpy(vcap->card, "SP DISPLAY Card", sizeof(vcap->card));
			strlcpy(vcap->bus_info, "SP DISP Device BUS OSD0", sizeof(vcap->bus_info));
			vcap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
			vcap->capabilities = vcap->device_caps | V4L2_CAP_DEVICE_CAPS;
		}
		else if(!(strcmp(DISP_OSD1_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			strlcpy(vcap->driver, "SP Video Driver", sizeof(vcap->driver));
			strlcpy(vcap->card, "SP DISPLAY Card", sizeof(vcap->card));
			strlcpy(vcap->bus_info, "SP DISP Device BUS OSD1", sizeof(vcap->bus_info));
			vcap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
			vcap->capabilities = vcap->device_caps | V4L2_CAP_DEVICE_CAPS;
		}
		else if(!(strcmp(DISP_VPP0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			strlcpy(vcap->driver, "SP Video Driver", sizeof(vcap->driver));
			strlcpy(vcap->card, "SP DISPLAY Card", sizeof(vcap->card));
			strlcpy(vcap->bus_info, "SP DISP Device BUS VPP0", sizeof(vcap->bus_info));
			vcap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
			vcap->capabilities = vcap->device_caps | V4L2_CAP_DEVICE_CAPS;
		}
		else
			sp_disp_dbg("[%s:%d] unknown layer !! \n", __FUNCTION__, __LINE__);
	}
	else {
		sp_disp_dbg("[%s:%d] disp_dev->dev[%d] empty \n", __FUNCTION__, __LINE__, dev_id);
	}

	return 0;
}

static const char * const StrFmt[] = {"480P", "576P", "720P", "1080P", "User Mode"};
static const char * const StrFps[] = {"60Hz", "50Hz", "24Hz"};

static int sp_display_g_fmt(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct sp_disp_fh *fh = priv;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct v4l2_format *fmt1 = NULL;

	char fmtstr[8];

	DRV_VideoFormat_e tgen_fmt;
	DRV_FrameRate_e tgen_fps;

	DRV_TGEN_GetFmtFps(&tgen_fmt,&tgen_fps);
	sp_disp_dbg("[%s:%d] tgen.fmt = %s \n", __FUNCTION__, __LINE__,StrFmt[tgen_fmt]);
	sp_disp_dbg("[%s:%d] tgen.fps = %s \n", __FUNCTION__, __LINE__,StrFps[tgen_fps]);

	memset(fmtstr, 0, 8);

	/* If buffer type is video output */
	if (V4L2_BUF_TYPE_VIDEO_OUTPUT != fmt->type) {
		sp_disp_err("[%s:%d] invalid type\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	if(!(strcmp(DISP_OSD0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_2;

	if(disp_dev->dev[dev_id]) {
		fmt1 = &disp_dev->dev[dev_id]->fmt;
		sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__,disp_dev->dev[dev_id]->video_dev.name);

		if(!(strcmp(DISP_OSD0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			memcpy(fmtstr, &disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat, 4);
			sp_disp_dbg("[%s:%d] cur fmt = %s \n", __FUNCTION__, __LINE__, fmtstr);
		}
		else if(!(strcmp(DISP_OSD1_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			memcpy(fmtstr, &disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat, 4);
			sp_disp_dbg("[%s:%d] cur fmt = %s \n", __FUNCTION__, __LINE__, fmtstr);
		}
		else if(!(strcmp(DISP_VPP0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			memcpy(fmtstr, &disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat, 4);
			sp_disp_dbg("[%s:%d] cur fmt = %s \n", __FUNCTION__, __LINE__, fmtstr);
		}
		else
			sp_disp_dbg("[%s:%d] unknown layer !! \n", __FUNCTION__, __LINE__);
		/* Fill in the information about format */
		fmt->fmt.pix = fmt1->fmt.pix;
	}
	else {
		sp_disp_dbg("[%s:%d] disp_dev->dev[%d] empty \n", __FUNCTION__, __LINE__, dev_id);
	}

	return 0;

}

static struct sp_fmt osd0_formats[] = {
	{
		.name     = "GREY", /* ui_format 0x2 = 8bpp (ARGB) , with grey paletteb */
		.fourcc   = V4L2_PIX_FMT_GREY,
		.width    = 720,
		.height   = 480,
		.depth    = 8,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "PAL8", /* ui_format 0x2 = 8bpp (ARGB) , with color palette */
		.fourcc   = V4L2_PIX_FMT_PAL8,
		.width    = 720,
		.height   = 480,
		.depth    = 8,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "YUYV", /* ui_format 0x4 = YUY2 */
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "RGBP", /* ui_format 0x8 = RGB565 */
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR15", /* ui_format 0x9 = ARGB1555 */
		.fourcc   = V4L2_PIX_FMT_ARGB555,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "R444", /* ui_format 0xa = RGBA4444 */
		.fourcc   = V4L2_PIX_FMT_RGB444,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR12", /* ui_format 0xb = ARGB4444 */
		.fourcc   = V4L2_PIX_FMT_ARGB444,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR24", /* ui_format 0xd = RGBA8888 */
		.fourcc   = V4L2_PIX_FMT_ABGR32,
		.width    = 720,
		.height   = 480,
		.depth    = 32,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "BA24", /* ui_format 0xe = ARGB8888 */
		.fourcc   = V4L2_PIX_FMT_ARGB32,
		.width    = 720,
		.height   = 480,
		.depth    = 32,
		.walign   = 1,
		.halign   = 1,
	},	
};

static struct sp_fmt osd1_formats[] = {
	{
		.name     = "GREY", /* ui_format 0x2 = 8bpp (ARGB) , with grey paletteb */
		.fourcc   = V4L2_PIX_FMT_GREY,
		.width    = 720,
		.height   = 480,
		.depth    = 8,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "PAL8", /* ui_format 0x2 = 8bpp (ARGB) , with color palette */
		.fourcc   = V4L2_PIX_FMT_PAL8,
		.width    = 720,
		.height   = 480,
		.depth    = 8,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "YUYV", /* ui_format 0x4 = YUY2 */
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "RGBP", /* ui_format 0x8 = RGB565 */
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR15", /* ui_format 0x9 = ARGB1555 */
		.fourcc   = V4L2_PIX_FMT_ARGB555,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "R444", /* ui_format 0xa = RGBA4444 */
		.fourcc   = V4L2_PIX_FMT_RGB444,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR12", /* ui_format 0xb = ARGB4444 */
		.fourcc   = V4L2_PIX_FMT_ARGB444,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "AR24", /* ui_format 0xd = RGBA8888 */
		.fourcc   = V4L2_PIX_FMT_ABGR32,
		.width    = 720,
		.height   = 480,
		.depth    = 32,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "BA24", /* ui_format 0xe = ARGB8888 */
		.fourcc   = V4L2_PIX_FMT_ARGB32,
		.width    = 720,
		.height   = 480,
		.depth    = 32,
		.walign   = 1,
		.halign   = 1,
	},	
};
#if DDFCH_FETCH_EN
static struct sp_fmt vpp0_formats[] = {
	{
		.name     = "NV12", /* vpp0_format = NV12 */
		.fourcc   = V4L2_PIX_FMT_NV12,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "NV16", /* vpp0_format = NV16 */
		.fourcc   = V4L2_PIX_FMT_NV16,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "YUYV", /* vpp0_format = YUY2 */
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
};
#endif
#if VPPDMA_FETCH_EN //vppdma
static struct sp_fmt vpp0_formats[] = {
	{
		.name     = "RGB888", /* vpp0_format = RGB888 */
		.fourcc   = V4L2_PIX_FMT_RGB24,
		.width    = 720,
		.height   = 480,
		.depth    = 24,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "RGB565", /* vpp0_format = RGB565 */
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "UYVY", /* vpp0_format = UYVY */
		.fourcc   = V4L2_PIX_FMT_UYVY,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "NV16", /* vpp0_format = NV16 */
		.fourcc   = V4L2_PIX_FMT_NV16,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
	{
		.name     = "YUYV", /* vpp0_format = YUY2 */
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.width    = 720,
		.height   = 480,
		.depth    = 16,
		.walign   = 1,
		.halign   = 1,
	},
};
#endif
static struct sp_vout_layer_info sp_vout_layer[] = {
	{
		.name = "osd0",
		.formats = osd0_formats,
		.formats_size = ARRAY_SIZE(osd0_formats),
	},
	{
		.name = "osd1",
		.formats = osd1_formats,
		.formats_size = ARRAY_SIZE(osd1_formats),
	},
	{
		.name = "vpp0",
		.formats = vpp0_formats,
		.formats_size = ARRAY_SIZE(vpp0_formats),
	},
};

static int sp_display_enum_fmt(struct file *file, void  *priv,
				   struct v4l2_fmtdesc *fmtdesc)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct sp_disp_fh *fh = priv;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct sp_fmt *fmt;

	if(!(strcmp(DISP_OSD0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_2;

	//sp_disp_dbg("[%s:%d] sp_display_enum_fmt , name = %s \n", __FUNCTION__, __LINE__, disp_dev->dev[dev_id]->video_dev.name);

	if(disp_dev->dev[dev_id]) {
		if(!(strcmp(DISP_OSD0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			if (fmtdesc->index == 0) {
				sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, disp_dev->dev[dev_id]->video_dev.name);
				sp_disp_dbg("[%s:%d] Support-formats = %d \n", __FUNCTION__, __LINE__, ARRAY_SIZE(osd0_formats));
			}

			if (fmtdesc->index >= ARRAY_SIZE(osd0_formats)) {
				//sp_disp_err("[%s:%d] stop\n", __FUNCTION__, __LINE__);
				return -EINVAL;
			}
			fmt = &osd0_formats[fmtdesc->index];
			strlcpy(fmtdesc->description, fmt->name, sizeof(fmtdesc->description));
			fmtdesc->pixelformat = fmt->fourcc;
		}
		else if(!(strcmp(DISP_OSD1_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			if (fmtdesc->index == 0) {
				sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, disp_dev->dev[dev_id]->video_dev.name);
				sp_disp_dbg("[%s:%d] Support-formats = %d \n", __FUNCTION__, __LINE__, ARRAY_SIZE(osd1_formats));
			}

			if (fmtdesc->index >= ARRAY_SIZE(osd1_formats)) {
				//sp_disp_err("[%s:%d] stop\n", __FUNCTION__, __LINE__);
				return -EINVAL;
			}
			fmt = &osd1_formats[fmtdesc->index];
			strlcpy(fmtdesc->description, fmt->name, sizeof(fmtdesc->description));
			fmtdesc->pixelformat = fmt->fourcc;			
		}
		else if(!(strcmp(DISP_VPP0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			if (fmtdesc->index == 0) {
				sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, disp_dev->dev[dev_id]->video_dev.name);
				sp_disp_dbg("[%s:%d] Support-formats = %d \n", __FUNCTION__, __LINE__, ARRAY_SIZE(vpp0_formats));
			}

			if (fmtdesc->index >= ARRAY_SIZE(vpp0_formats)) {
				//sp_disp_err("[%s:%d] stop\n", __FUNCTION__, __LINE__);
				return -EINVAL;
			}
			fmt = &vpp0_formats[fmtdesc->index];
			strlcpy(fmtdesc->description, fmt->name, sizeof(fmtdesc->description));
			fmtdesc->pixelformat = fmt->fourcc;
		}
		else
			sp_disp_dbg("[%s:%d] unknown layer !! \n", __FUNCTION__, __LINE__);

	}
	else {
		sp_disp_dbg("[%s:%d] disp_dev->dev[%d] empty \n", __FUNCTION__, __LINE__, dev_id);
	}

	return 0;
}

static int sp_try_format(struct sp_disp_device *disp_dev,
			struct v4l2_pix_format *pixfmt, int layer_id)
{
	struct sp_vout_layer_info *pixfmt0 = &sp_vout_layer[layer_id];
	struct sp_fmt *pixfmt1 = NULL;
	int i,match=0;

	if (pixfmt0->formats_size <= 0) {
		sp_disp_err("[%s:%d] invalid formats \n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	for(i = 0; i < (pixfmt0->formats_size) ; i++)
	{	
		if (layer_id == 0)
			pixfmt1 = &osd0_formats[i];
		else if (layer_id == 1)
			pixfmt1 = &osd1_formats[i];
		else if (layer_id == 2)
			pixfmt1 = &vpp0_formats[i];

		if(pixfmt->pixelformat == pixfmt1->fourcc) {
			match = 1;
			sp_disp_dbg("[%s:%d] found match \n", __FUNCTION__, __LINE__);
			break;
		}
	}
	if(!match) {
		sp_disp_dbg("[%s:%d] not found match \n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	return 0;
}

static int sp_display_try_fmt(struct file *file, void *priv,
				  struct v4l2_format *fmt)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct sp_disp_fh *fh = priv;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
	int ret;

	sp_disp_dbg("[%s:%d] sp_display_try_fmt \n", __FUNCTION__, __LINE__);

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		sp_disp_err("[%s:%d] Invalid V4L2 buffer type!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	if(!(strcmp(DISP_OSD0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_2;

	/* Check for valid pixel format */
	ret = sp_try_format(disp_dev, pixfmt, dev_id);
	if (ret)
		return ret;

	return 0;

}

static int sp_display_s_fmt(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	struct sp_disp_device *disp_dev = video_drvdata(file);
	struct sp_disp_fh *fh = priv;
	enum sp_disp_device_id dev_id = SP_DISP_DEVICE_0;
	struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
	int ret;
	char fmtstr[8];
	#ifdef TIMING_SYNC_720P60
	DRV_SetTGEN_t SetTGEN;
	int mode;
	#endif

	sp_disp_dbg("[%s:%d] sp_display_s_fmt \n", __FUNCTION__, __LINE__);

	memset(fmtstr,0,8);

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		sp_disp_err("[%s:%d] invalid type \n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	if(!(strcmp(DISP_OSD0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_0;
	else if(!(strcmp(DISP_OSD1_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_1;
	else if(!(strcmp(DISP_VPP0_NAME, fh->fh.vdev->name)))
		dev_id = SP_DISP_DEVICE_2;

	/* Check for valid pixel format */
	ret = sp_try_format(disp_dev, pixfmt, dev_id);
	if (ret)
		return ret;

	if(disp_dev->dev[dev_id]) {
		sp_disp_dbg("[%s:%d] Layer name = %s \n", __FUNCTION__, __LINE__, disp_dev->dev[dev_id]->video_dev.name);
		sp_disp_dbg("[%s:%d] width  = %d , height = %d \n", __FUNCTION__, __LINE__, pixfmt->width, pixfmt->height);
		memcpy(fmtstr, &pixfmt->pixelformat, 4);
		sp_disp_dbg("[%s:%d] set fmt = %s \n", __FUNCTION__, __LINE__, fmtstr);

		if(!(strcmp(DISP_OSD0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			disp_dev->dev[dev_id]->fmt.type                 = fmt->type;
			disp_dev->dev[dev_id]->fmt.fmt.pix.width        = pixfmt->width;
			disp_dev->dev[dev_id]->fmt.fmt.pix.height       = pixfmt->height;
			disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat  = pixfmt->pixelformat; // from sp_try_format
			disp_dev->dev[dev_id]->fmt.fmt.pix.field        = pixfmt->field;
			disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = pixfmt->bytesperline;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = (pixfmt->width)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = pixfmt->sizeimage;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = (pixfmt->width)*(pixfmt->height)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.colorspace   = pixfmt->colorspace;
		}
		else if(!(strcmp(DISP_OSD1_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			disp_dev->dev[dev_id]->fmt.type                 = fmt->type;
			disp_dev->dev[dev_id]->fmt.fmt.pix.width        = pixfmt->width;
			disp_dev->dev[dev_id]->fmt.fmt.pix.height       = pixfmt->height;
			disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat  = pixfmt->pixelformat; // from sp_try_format
			disp_dev->dev[dev_id]->fmt.fmt.pix.field        = pixfmt->field;
			disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = pixfmt->bytesperline;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = (pixfmt->width)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = pixfmt->sizeimage;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = (pixfmt->width)*(pixfmt->height)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.colorspace   = pixfmt->colorspace;
		}
		else if(!(strcmp(DISP_VPP0_NAME, disp_dev->dev[dev_id]->video_dev.name))) {
			disp_dev->dev[dev_id]->fmt.type                 = fmt->type;
			disp_dev->dev[dev_id]->fmt.fmt.pix.width        = pixfmt->width;
			disp_dev->dev[dev_id]->fmt.fmt.pix.height       = pixfmt->height;
			disp_dev->dev[dev_id]->fmt.fmt.pix.pixelformat  = pixfmt->pixelformat; // from sp_try_format
			disp_dev->dev[dev_id]->fmt.fmt.pix.field        = pixfmt->field;
			disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = pixfmt->bytesperline;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.bytesperline = (pixfmt->width)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = pixfmt->sizeimage;
			//disp_dev->dev[dev_id]->fmt.fmt.pix.sizeimage    = (pixfmt->width)*(pixfmt->height)*4;
			disp_dev->dev[dev_id]->fmt.fmt.pix.colorspace   = pixfmt->colorspace;

#ifdef TIMING_SYNC_720P60
			//vpp_path_cur_setting_read();
			//osd_path_cur_setting_read();
			hdmitx_get_timming(&hdmitx_cur_timing);
			//sp_disp_dbg("[%s:%d] get hdmitx timing %d \n", __FUNCTION__, __LINE__, hdmitx_cur_timing);
			
			sp_disp_dbg("[%s:%d] get hdmitx timing %s \n", __FUNCTION__, __LINE__, hdmitx_res[hdmitx_cur_timing]);

			if((pixfmt->width <= 720)&&(pixfmt->height <= 480)) {
				vscl_setting(0, 0, pixfmt->width, pixfmt->height, pixfmt->width, 480);
				mode = 0;
				hdmitx_set_timming(HDMITX_TIMING_480P);
			}
			else if((pixfmt->width <= 720)&&(pixfmt->height <= 576)) {
				vscl_setting(0, 0, pixfmt->width, pixfmt->height, pixfmt->width, 576);
				mode = 1;
				hdmitx_set_timming(HDMITX_TIMING_576P);
			}
			else if((pixfmt->width <= 1280)&&(pixfmt->height <= 720)) {
				vscl_setting(0, 0, pixfmt->width, pixfmt->height, pixfmt->width, 720);
				mode = 2;
				hdmitx_set_timming(HDMITX_TIMING_720P60);
			}
			else { //if((pixfmt->width <= 1920)&&(pixfmt->height <= 1080))
				vscl_setting(0, 0, pixfmt->width, pixfmt->height, pixfmt->width, 1080);
				mode = 4;
				hdmitx_set_timming(HDMITX_TIMING_1080P60);
			}
			hdmitx_enable_display(1);

			DRV_DVE_SetMode(mode);

			switch (mode)
			{
				default:
				case 0:
					SetTGEN.fmt = DRV_FMT_480P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 1:
					SetTGEN.fmt = DRV_FMT_576P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 2:
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 3:
					SetTGEN.fmt = DRV_FMT_720P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 4:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_5994Hz;
					break;
				case 5:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_50Hz;
					break;
				case 6:
					SetTGEN.fmt = DRV_FMT_1080P;
					SetTGEN.fps = DRV_FrameRate_24Hz;
					break;
				case 7:
					sp_disp_dbg("user mode unsupport\n");
					break;
			}

			ret = DRV_TGEN_Set(&SetTGEN);
			if (ret != DRV_SUCCESS) {
				sp_disp_err("TGEN Set failed, ret = %d\n", ret);
				return ret;
			}
#endif
		}
		else
			sp_disp_dbg("[%s:%d] unknown layer !! \n", __FUNCTION__, __LINE__);

	}
	else {
		sp_disp_dbg("[%s:%d] disp_dev->dev[%d] empty \n", __FUNCTION__, __LINE__, dev_id);
	}

	return 0;
}

static const struct v4l2_ioctl_ops sp_disp_ioctl_ops = {
	.vidioc_querycap                = sp_vidioc_querycap,
	.vidioc_g_fmt_vid_out    		= sp_display_g_fmt,
	.vidioc_enum_fmt_vid_out 		= sp_display_enum_fmt,
	.vidioc_s_fmt_vid_out    		= sp_display_s_fmt,
	.vidioc_try_fmt_vid_out  		= sp_display_try_fmt,
	.vidioc_reqbufs                 = vb2_ioctl_reqbufs,
	.vidioc_querybuf                = vb2_ioctl_querybuf,
	.vidioc_create_bufs             = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf             = vb2_ioctl_prepare_buf,
	.vidioc_qbuf                    = vb2_ioctl_qbuf,
	.vidioc_dqbuf                   = vb2_ioctl_dqbuf,
	.vidioc_expbuf                  = vb2_ioctl_expbuf,
	.vidioc_streamon                = vb2_ioctl_streamon,
	.vidioc_streamoff               = vb2_ioctl_streamoff,
};
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
	#ifdef V4L2_TEST_DQBUF
	struct sp_disp_device *disp_dev = platform_get_drvdata(param);
	struct sp_disp_layer *layer;
	struct UI_FB_Info_t Info;
	unsigned long addr;	
	struct sp_disp_buffer *next_frm;
	int i;

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

	if (!param)
		return IRQ_HANDLED;

	for (i = 0; i < SP_DISP_MAX_DEVICES; i++) {
		layer = disp_dev->dev[i];

		if ( !disp_dev->dev[i] )
			continue;

		if (layer->skip_first_int) {
			layer->skip_first_int = 0;
			continue;
		}

		if (layer->streaming) {
			spin_lock(&disp_dev->dma_queue_lock);
			if (!list_empty(&layer->dma_queue)) {
				next_frm = list_entry(layer->dma_queue.next,
						struct  sp_disp_buffer, list);
				/* Remove that from the buffer queue */
				list_del_init(&next_frm->list);
				
				/* Mark state of the frame to active */
				next_frm->vb.vb2_buf.state = VB2_BUF_STATE_ACTIVE;

				addr = vb2_dma_contig_plane_dma_addr(&next_frm->vb.vb2_buf, 0);
				#ifdef I143_SYS_PORT
				addr &= ~0x80000000;
				#endif

				#if 1
				if(i == 0) {
					if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB32) {
						Info.UI_ColorFmt = 0xe;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ABGR32) {
						Info.UI_ColorFmt = 0xd;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB444) {
						Info.UI_ColorFmt = 0xb;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB444) {
						Info.UI_ColorFmt = 0xa;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB555) {
						Info.UI_ColorFmt = 0x9;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
						Info.UI_ColorFmt = 0x8;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
						Info.UI_ColorFmt = 0x4;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
						Info.UI_ColorFmt = 0x2;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
						Info.UI_ColorFmt = 0x2;
					}

					Info.UI_width = layer->fmt.fmt.pix.width;
					Info.UI_height = layer->fmt.fmt.pix.height;
					Info.UI_bufAddr = addr;

					#ifdef	SP_DISP_OSD_PARM
					DRV_OSD_SET_OSD_Header(&Info,0);
					#endif

				}
				else if(i == 1) {
					if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB32) {
						Info.UI_ColorFmt = 0xe;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ABGR32) {
						Info.UI_ColorFmt = 0xd;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB444) {
						Info.UI_ColorFmt = 0xb;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB444) {
						Info.UI_ColorFmt = 0xa;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_ARGB555) {
						Info.UI_ColorFmt = 0x9;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
						Info.UI_ColorFmt = 0x8;
					}
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
						Info.UI_ColorFmt = 0x4;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
						Info.UI_ColorFmt = 0x2;
					}	
					else if(layer->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
						Info.UI_ColorFmt = 0x2;
					}

					Info.UI_width = layer->fmt.fmt.pix.width;
					Info.UI_height = layer->fmt.fmt.pix.height;
					Info.UI_bufAddr = addr;

					#ifdef	SP_DISP_OSD_PARM
					DRV_OSD_SET_OSD_Header(&Info,1);
					#endif

				}
				else if(i == 2) {
					#if DDFCH_FETCH_EN
					if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
						ddfch_setting(addr, addr + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 0);	
					}
					else if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV16) {
						ddfch_setting(addr, addr + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 1);
					}
					else if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
						ddfch_setting(addr, addr + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 2);
					}
					#endif
					#if VPPDMA_FETCH_EN
					if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
						//vppdma_setting(addr, addr + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 0);
						vppdma_setting(addr, addr + (layer->fmt.fmt.pix.width)*(layer->fmt.fmt.pix.height), (layer->fmt.fmt.pix.width), (layer->fmt.fmt.pix.height), 0);
					}
					if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
						//vppdma_setting(addr, addr + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 1);
						vppdma_setting(addr, addr + (layer->fmt.fmt.pix.width)*(layer->fmt.fmt.pix.height), (layer->fmt.fmt.pix.width), (layer->fmt.fmt.pix.height), 1);
					}
					if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) {
						//vppdma_setting(addr, (addr + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 2);
						vppdma_setting(addr, addr + (layer->fmt.fmt.pix.width)*(layer->fmt.fmt.pix.height), (layer->fmt.fmt.pix.width), (layer->fmt.fmt.pix.height), 2);
					}
					else if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV16) {
						//vppdma_setting((addr), (addr + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 3);
						vppdma_setting(addr, addr + (layer->fmt.fmt.pix.width)*(layer->fmt.fmt.pix.height), (layer->fmt.fmt.pix.width), (layer->fmt.fmt.pix.height), 3);
					}
					else if(disp_dev->dev[2]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
						//vppdma_setting((addr), (addr + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 4);
						//sp_disp_info("addr bytes = %ld \n", sizeof(addr));
						//sp_disp_info("addr4  = %ld \n", addr);
						//addr = __pa(addr);
						//sp_disp_info("addr5  = %ld \n", addr);
						//addr &= ~0x80000000;
						//sp_disp_info("addr6  = %ld \n", addr);						
						vppdma_setting(addr, addr + (layer->fmt.fmt.pix.width)*(layer->fmt.fmt.pix.height), (layer->fmt.fmt.pix.width), (layer->fmt.fmt.pix.height), 4);
					}
					#endif
				}
				#endif
				layer->cur_frm->vb.vb2_buf.timestamp = ktime_get_ns();
				layer->cur_frm->vb.field = layer->fmt.fmt.pix.field;
				layer->cur_frm->vb.sequence = layer->sequence++;
				vb2_buffer_done(&layer->cur_frm->vb.vb2_buf, VB2_BUF_STATE_DONE);
				/* Make cur_frm pointing to next_frm */
				layer->cur_frm = next_frm;
			}
			spin_unlock(&disp_dev->dma_queue_lock);

		}
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
	#if 1
	if (g_disp_state == 0)
		DRV_OSD_IRQ();
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
	#if 0
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
	else 
	#endif
	if (!strncasecmp(tmpbuf, "layer", 5))
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
				sp_disp_info("disp ddfch bist en(half1) \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 2);
			}
			else if (bist_mode == 3) {
				sp_disp_info("disp ddfch bist en(half2) \n");
				DRV_VPP_SetColorbar(DRV_FROM_DDFCH, 3);
			}
			else {
				sp_disp_info("disp ddfch bist dis \n");
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

	sp_disp_dbg("ioremap address = 0x%08x\n", (unsigned int)p);
	//printk(KERN_INFO "ioremap address = 0x%08x\n", (unsigned int)p);
	//printk(KERN_INFO "ioremap address \n");
	
	*membase = p;

	return 0;
}

#ifdef SP_DISP_V4L2_SUPPORT
static int sp_disp_initialize(struct device *dev, struct sp_disp_device *disp_dev)
{
	int ret;

	sp_disp_dbg("[%s:%d] sp_disp_initialize \n", __FUNCTION__, __LINE__);

	if (!disp_dev || !dev) {
		sp_disp_err("Null device pointers.\n");
		return -ENODEV;
	}

	// Register V4L2 device.
	ret = v4l2_device_register(dev, &disp_dev->v4l2_dev);
	if (ret) {
		sp_disp_err("Unable to register v4l2 device!\n");
		goto err_v4l2_register;
	}

	// Initialize locks.
	spin_lock_init(&disp_dev->dma_queue_lock);
	mutex_init(&disp_dev->lock);

	return 0;

err_v4l2_register:
	v4l2_device_unregister(&disp_dev->v4l2_dev);
	kfree(disp_dev);

	return ret;
}
#endif

static int sp_disp_set_osd_resolution(struct device *dev, struct sp_disp_device *disp_dev)
{

	#ifdef SP_DISP_V4L2_SUPPORT
	char fmtstr[8];
	sp_disp_dbg("set osd resolution \n");

	if (of_property_read_u32(dev->of_node, "ui_width", &disp_dev->UIRes.width)) {
		disp_dev->UIRes.width = 0;
	}
	else {
		disp_dev->dev[0]->fmt.fmt.pix.width = disp_dev->UIRes.width;
		disp_dev->dev[1]->fmt.fmt.pix.width = disp_dev->UIRes.width;
	}

	if (of_property_read_u32(dev->of_node, "ui_height", &disp_dev->UIRes.height)) {
		disp_dev->UIRes.height = 0;
	}
	else {
		disp_dev->dev[0]->fmt.fmt.pix.height = disp_dev->UIRes.height;
		disp_dev->dev[1]->fmt.fmt.pix.height = disp_dev->UIRes.height;
	}

	if (of_property_read_u32(dev->of_node, "ui_format", &disp_dev->UIFmt)) {
		disp_dev->UIFmt = DRV_OSD_REGION_FORMAT_ARGB_8888;
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_8BPP) { /* 8 bit/pixel with CLUT */
		//disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_PAL8;
		//disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_PAL8;
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;		
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_YUY2) { /* 16 bit/pixel YUY2 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_RGB_565) { /* 16 bit/pixel RGB 5:6:5 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_ARGB_1555) { /* 16 bit/pixel ARGB 1:5:5:5 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB555;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB555;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_RGBA_4444) { /* 16 bit/pixel RGBA 4:4:4:4 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB444;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB444;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_ARGB_4444) { /* 16 bit/pixel ARGB 4:4:4:4 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB444;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB444;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_RGBA_8888) { /* 32 bit/pixel RGBA 8:8:8:8 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ABGR32;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ABGR32;
	}
	else if (disp_dev->UIFmt == DRV_OSD_REGION_FORMAT_ARGB_8888) { /* 32 bit/pixel ARGB 8:8:8:8 */
		disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
		disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
	}

#ifdef UI_FORCE_ALPHA
	if (of_property_read_u32(dev->of_node, "ui_force_alpha", &disp_dev->UIForceAlpha)) {
		disp_dev->UIForceAlpha = 0;
		disp_dev->UISetAlpha = 0;
	}
	else {
		if (disp_dev->UIForceAlpha >= 1) { /* enable force set alpha value */
			if (of_property_read_u32(dev->of_node, "ui_set_alpha", &disp_dev->UISetAlpha)) {
				disp_dev->UISetAlpha = 0;
			}
			else {
				if (disp_dev->UISetAlpha >= 255)
					disp_dev->UISetAlpha = 255;
				if (disp_dev->UISetAlpha <= 0)
					disp_dev->UISetAlpha = 0;
				sp_disp_dbg("ui_force_alpha = %d , ui_set_alpha = %d \n", disp_dev->UIForceAlpha, disp_dev->UISetAlpha);
			}
		}
	}
#endif

	#ifdef	SP_DISP_OSD_PARM
	DRV_OSD_INIT_OSD_Header();
	#endif

	sp_disp_dbg("ui_width = %d , ui_height = %d \n", disp_dev->UIRes.width, disp_dev->UIRes.height);

    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &disp_dev->dev[0]->fmt.fmt.pix.pixelformat, 4);
	sp_disp_dbg("dev[0] osd0_format = %s \n", fmtstr);

    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &disp_dev->dev[1]->fmt.fmt.pix.pixelformat, 4);
	sp_disp_dbg("dev[1] osd1_format = %s \n", fmtstr);

	disp_dev->dev[0]->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	//disp_dev->dev[0]->fmt.fmt.pix.width = norm_maxw();
	//disp_dev->dev[0]->fmt.fmt.pix.height = norm_maxh();
	//disp_dev->dev[0]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
	disp_dev->dev[0]->fmt.fmt.pix.field = V4L2_FIELD_NONE;
	disp_dev->dev[0]->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
	disp_dev->dev[0]->fmt.fmt.pix.priv = 0;

	disp_dev->dev[1]->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	//disp_dev->dev[1]->fmt.fmt.pix.width = norm_maxw();
	//disp_dev->dev[1]->fmt.fmt.pix.height = norm_maxh();
	//disp_dev->dev[1]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_ARGB32;
	disp_dev->dev[1]->fmt.fmt.pix.field = V4L2_FIELD_NONE;
	disp_dev->dev[1]->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
	disp_dev->dev[1]->fmt.fmt.pix.priv = 0;

	return 0;
	#else
	if (of_property_read_u32(dev->of_node, "ui_width", &disp_dev->UIRes.width))
		disp_dev->UIRes.width = 0;

	if (of_property_read_u32(dev->of_node, "ui_height", &disp_dev->UIRes.height))
		disp_dev->UIRes.height = 0;

	if (of_property_read_u32(dev->of_node, "ui_format", &disp_dev->UIFmt))
		disp_dev->UIFmt = DRV_OSD_REGION_FORMAT_ARGB_8888;

	return 0;

	#endif

}

static int sp_disp_set_output_resolution(struct sp_disp_device *disp_dev, int is_hdmi)
{
	int mode = 0;
	int ret;
	DRV_SetTGEN_t SetTGEN;

	sp_disp_dbg("set output resolution \n");

	if(is_hdmi) { //hdmitx output
		if( ((disp_dev->UIRes.width == 1920)&&(disp_dev->UIRes.height == 1080)) ||
		#if (OSD1_GRP_EN == 1)
				((OSD1_WIDTH == 1920)&&(OSD1_HEIGHT == 1080)) ||
		#endif
		#if (DDFCH_GRP_EN == 1)
				((DDFCH_VPP_WIDTH == 1920)&&(DDFCH_VPP_HEIGHT == 1080)) 
		#endif
		#if (VPPDMA_GRP_EN == 1)
				((VPPDMA_VPP_WIDTH == 1920)&&(VPPDMA_VPP_HEIGHT == 1080))
		#endif
				) {
			mode = 4;
			#ifdef TIMING_SYNC_720P60
			hdmitx_set_timming(HDMITX_TIMING_1080P60);
			hdmitx_enable_display(1);
			#endif
		}
		else if( ((disp_dev->UIRes.width == 1280)&&(disp_dev->UIRes.height == 720)) ||
		#if (OSD1_GRP_EN == 1)
				((OSD1_WIDTH == 1280)&&(OSD1_HEIGHT == 720)) ||
		#endif
		#if (DDFCH_GRP_EN == 1)
				((DDFCH_VPP_WIDTH == 1280)&&(DDFCH_VPP_HEIGHT == 720)) 
		#endif
		#if (VPPDMA_GRP_EN == 1)
				((VPPDMA_VPP_WIDTH == 1280)&&(VPPDMA_VPP_HEIGHT == 720))
		#endif
				) {
			mode = 2;
			#ifdef TIMING_SYNC_720P60
			hdmitx_set_timming(HDMITX_TIMING_720P60);
			hdmitx_enable_display(1);
			#endif
		}
		else if( ((disp_dev->UIRes.width == 720)&&(disp_dev->UIRes.height == 576)) ||
		#if (OSD1_GRP_EN == 1)
				((OSD1_WIDTH == 720)&&(OSD1_HEIGHT == 576)) ||
		#endif
		#if (DDFCH_GRP_EN == 1)
				((DDFCH_VPP_WIDTH == 720)&&(DDFCH_VPP_HEIGHT == 576)) 
		#endif
		#if (VPPDMA_GRP_EN == 1)
				((VPPDMA_VPP_WIDTH == 720)&&(VPPDMA_VPP_HEIGHT == 576))
		#endif
				) {
			mode = 1;
			#ifdef TIMING_SYNC_720P60
			hdmitx_set_timming(HDMITX_TIMING_576P);
			hdmitx_enable_display(1);
			#endif
		}
		else if( ((disp_dev->UIRes.width == 720)&&(disp_dev->UIRes.height == 480)) ||
		#if (OSD1_GRP_EN == 1)
				((OSD1_WIDTH == 720)&&(OSD1_HEIGHT == 480)) ||
		#endif
		#if (DDFCH_GRP_EN == 1)
				((DDFCH_VPP_WIDTH == 720)&&(DDFCH_VPP_HEIGHT == 480)) 
		#endif
		#if (VPPDMA_GRP_EN == 1)
				((VPPDMA_VPP_WIDTH == 720)&&(VPPDMA_VPP_HEIGHT == 480))
		#endif
				) {
			mode = 0;
			#ifdef TIMING_SYNC_720P60
			hdmitx_set_timming(HDMITX_TIMING_480P);
			hdmitx_enable_display(1);
			#endif
		}
		else {
			mode = 0;
			#ifdef TIMING_SYNC_720P60
			hdmitx_set_timming(HDMITX_TIMING_480P);
			hdmitx_enable_display(1);
			#endif
		}
		sp_disp_dbg("hdmitx output , mode = %d \n", mode);
	}

	DRV_DVE_SetMode(mode);
	//DRV_DVE_SetColorbar(ENABLE);

	switch (mode)
	{
		default:
		case 0:
			SetTGEN.fmt = DRV_FMT_480P;
			SetTGEN.fps = DRV_FrameRate_5994Hz;
			disp_dev->panelRes.width = 720;
			disp_dev->panelRes.height = 480;
			break;
		case 1:
			SetTGEN.fmt = DRV_FMT_576P;
			SetTGEN.fps = DRV_FrameRate_50Hz;
			disp_dev->panelRes.width = 720;
			disp_dev->panelRes.height = 576;
			break;
		case 2:
			SetTGEN.fmt = DRV_FMT_720P;
			SetTGEN.fps = DRV_FrameRate_5994Hz;
			disp_dev->panelRes.width = 1280;
			disp_dev->panelRes.height = 720;
			break;
		case 3:
			SetTGEN.fmt = DRV_FMT_720P;
			SetTGEN.fps = DRV_FrameRate_50Hz;
			disp_dev->panelRes.width = 1280;
			disp_dev->panelRes.height = 720;
			break;
		case 4:
			SetTGEN.fmt = DRV_FMT_1080P;
			SetTGEN.fps = DRV_FrameRate_5994Hz;
			disp_dev->panelRes.width = 1920;
			disp_dev->panelRes.height = 1080;
			break;
		case 5:
			SetTGEN.fmt = DRV_FMT_1080P;
			SetTGEN.fps = DRV_FrameRate_50Hz;
			disp_dev->panelRes.width = 1920;
			disp_dev->panelRes.height = 1080;
			break;
		case 6:
			SetTGEN.fmt = DRV_FMT_1080P;
			SetTGEN.fps = DRV_FrameRate_24Hz;
			disp_dev->panelRes.width = 1920;
			disp_dev->panelRes.height = 1080;
			break;
		case 7:
			sp_disp_dbg("user mode unsupport\n");
			break;
	}

	ret = DRV_TGEN_Set(&SetTGEN);
	if (ret != DRV_SUCCESS) {
		sp_disp_err("TGEN Set failed, ret = %d\n", ret);
		return ret;
	}

	sp_disp_dbg("UI_width = %d , UI_height = %d \n" , disp_dev->UIRes.width, disp_dev->UIRes.height);
	sp_disp_dbg("OUT_width = %d , OUT_height = %d \n" , disp_dev->panelRes.width, disp_dev->panelRes.height);
	#ifdef SP_DISP_V4L2_SUPPORT
	sp_disp_dbg("OSD0_width = %d , OSD0_height = %d \n" , disp_dev->dev[0]->fmt.fmt.pix.width, disp_dev->dev[0]->fmt.fmt.pix.height);
	sp_disp_dbg("OSD1_width = %d , OSD1_height = %d \n" , disp_dev->dev[1]->fmt.fmt.pix.width, disp_dev->dev[1]->fmt.fmt.pix.height);
	sp_disp_dbg("VPP0_width = %d , VPP0_height = %d \n" , disp_dev->dev[2]->fmt.fmt.pix.width, disp_dev->dev[2]->fmt.fmt.pix.height);
	#endif

	return 0;
}

#ifdef SP_DISP_V4L2_SUPPORT
static int sp_disp_init_multi_layer(int i, struct platform_device *pdev, struct sp_disp_device *disp_dev)
{
	struct sp_disp_layer *disp_layer = NULL;
	struct video_device *vbd = NULL;

	//sp_disp_dbg("init layer for dev[%d] \n", i);
	/* Allocate memory for four plane display objects */

	disp_dev->dev[i] =
		kzalloc(sizeof(struct sp_disp_layer), GFP_KERNEL);

	/* If memory allocation fails, return error */
	if (!disp_dev->dev[i]) {
		sp_disp_err("ran out of memory\n");
		return  -ENOMEM;
	}
	spin_lock_init(&disp_dev->dev[i]->irqlock);
	mutex_init(&disp_dev->dev[i]->opslock);

	/* Get the pointer to the layer object */
	disp_layer = disp_dev->dev[i];
	vbd = &disp_layer->video_dev;
	/* Initialize field of video device */
	vbd->release	= video_device_release_empty;
	vbd->fops	= &sp_disp_fops;
	vbd->ioctl_ops	= &sp_disp_ioctl_ops;
	vbd->minor	= -1;
	vbd->v4l2_dev   = &disp_dev->v4l2_dev;
	vbd->lock	= &disp_layer->opslock;
	vbd->vfl_dir	= VFL_DIR_TX;
	vbd->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;

	if (i == 0)
		strlcpy(vbd->name, DISP_OSD0_NAME, sizeof(vbd->name));
	else if (i == 1)
		strlcpy(vbd->name, DISP_OSD1_NAME, sizeof(vbd->name));
	else if (i == 2)
		strlcpy(vbd->name, DISP_VPP0_NAME, sizeof(vbd->name));

	disp_layer->device_id = i;
	sp_disp_dbg("init dev[%d] Layer name = %s \n", i, vbd->name);

	return 0;
}

static int sp_disp_reg_multi_layer(int i, struct platform_device *pdev, struct sp_disp_device *disp_dev)
{
	struct vb2_queue *q;
	int ret;
	int vid_num = 10;

	//sp_disp_dbg("[%s:%d] sp_disp_reg_multi_layer \n", __FUNCTION__, __LINE__);
	/* Allocate memory for four plane display objects */

	/* initialize vb2 queue */
	q = &disp_dev->dev[i]->buffer_queue;
	memset(q, 0, sizeof(*q));
	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->drv_priv = disp_dev->dev[i];
	q->ops = &sp_video_qops;
	q->mem_ops = sp_mem_ops[allocator];
	q->buf_struct_size = sizeof(struct sp_disp_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = MIN_BUFFERS;
	q->lock = &disp_dev->dev[i]->opslock;
	q->dev = disp_dev->pdev;
	ret = vb2_queue_init(q);
	if (ret) {
		sp_disp_err("vb2_queue_init() failed!\n");
		goto err_vb2_queue_init;
	}

	INIT_LIST_HEAD(&disp_dev->dev[i]->dma_queue);

	print_List(&disp_dev->dev[i]->dma_queue);

	disp_dev->dev[i]->video_dev.queue = &disp_dev->dev[i]->buffer_queue;

	// Register video device.
	ret = video_register_device(&disp_dev->dev[i]->video_dev, VFL_TYPE_GRABBER, (vid_num + i));
	if (ret) {
		sp_disp_err("Unable to register video device!\n");
		ret = -ENODEV;
		goto err_video_register;
	}

	sp_disp_dbg("reg  dev[%d] to /dev/video%d \n", i, (vid_num + i));

	disp_dev->dev[i]->disp_dev = disp_dev;
	platform_set_drvdata(pdev, disp_dev);
	/* set driver private data */
	video_set_drvdata(&disp_dev->dev[i]->video_dev, disp_dev);

	return 0;

err_vb2_queue_init:
err_video_register:
	/* Unregister video device */
	video_unregister_device(&disp_dev->dev[i]->video_dev);		
	kfree(disp_dev->dev[i]);
	disp_dev->dev[i] = NULL;

	v4l2_device_unregister(&disp_dev->v4l2_dev);
	kfree(disp_dev);

	return ret;
}
#endif

static int sp_disp_set_vpp_resolution(struct device *dev, struct sp_disp_device *disp_dev)
{

	#ifdef SP_DISP_V4L2_SUPPORT
	char fmtstr[8];
	#endif
	sp_disp_dbg("set vpp resolution \n");

	#if (DDFCH_FETCH_EN == 1) // DDFCH fetch data
	#if ((DDFCH_VPP_WIDTH == 720) && (DDFCH_VPP_HEIGHT == 480))
	vscl_setting((720-DDFCH_VPP_WIDTH)>>1, (480-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 720, 480);
	#elif ((DDFCH_VPP_WIDTH == 720) && (DDFCH_VPP_HEIGHT == 576))
	vscl_setting((720-DDFCH_VPP_WIDTH)>>1, (576-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 720, 576);	
	#elif ((DDFCH_VPP_WIDTH == 1280) && (DDFCH_VPP_HEIGHT == 720))
	vscl_setting((1280-DDFCH_VPP_WIDTH)>>1, (720-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 1280, 720);
	#elif ((DDFCH_VPP_WIDTH == 1920) && (DDFCH_VPP_HEIGHT == 1080))
	vscl_setting((1920-DDFCH_VPP_WIDTH)>>1, (1080-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 1920, 1080);
	#endif
	#ifdef SP_DISP_VPP_FIXED_ADDR
	vpp_alloc_size = (DDFCH_FMT_HDMI)?(ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT*2):(ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT*3/2);
	vpp_yuv_ptr = ioremap(0x00120000, vpp_alloc_size);
	memcpy(vpp_yuv_ptr, vpp_yuv_array, vpp_alloc_size);
	ddfch_setting(0x00120000, 0x00120000 + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	#else
	pa = __pa(vpp_yuv_array);
	pa &= ~0x80000000;
	ddfch_setting(pa, pa + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	//ddfch_setting(virt_to_phys(vpp_yuv_array), virt_to_phys((vpp_yuv_array + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT)), DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	#endif

	#ifdef SP_DISP_V4L2_SUPPORT
	if(disp_dev->dev[2]) {
		disp_dev->dev[2]->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		disp_dev->dev[2]->fmt.fmt.pix.width = DDFCH_VPP_WIDTH;
		disp_dev->dev[2]->fmt.fmt.pix.height = DDFCH_VPP_HEIGHT;

		#if (DDFCH_FMT_HDMI == 0)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		#elif (DDFCH_FMT_HDMI == 1)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV16;
		#elif (DDFCH_FMT_HDMI == 2)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		#endif

		disp_dev->dev[2]->fmt.fmt.pix.field = V4L2_FIELD_NONE;
		disp_dev->dev[2]->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
		disp_dev->dev[2]->fmt.fmt.pix.priv = 0;

		memset(fmtstr, 0, 8);
		memcpy(fmtstr, &disp_dev->dev[2]->fmt.fmt.pix.pixelformat, 4);
		sp_disp_dbg("dev[2] vpp0_format = %s \n", fmtstr);
		sp_disp_dbg("DDFCH_VPP_WIDTH = %d , DDFCH_VPP_HEIGHT = %d \n", DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT);
	}
	#endif
	#endif

	#if (VPPDMA_FETCH_EN == 1) //VPPDMA fetch data
	#if ((VPPDMA_VPP_WIDTH == 640) && (VPPDMA_VPP_HEIGHT == 480))
	vscl_setting((640-VPPDMA_VPP_WIDTH)>>1, (480-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 640, 480);
	#elif ((VPPDMA_VPP_WIDTH == 720) && (VPPDMA_VPP_HEIGHT == 480))
	vscl_setting((720-VPPDMA_VPP_WIDTH)>>1, (480-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 720, 480);
	#elif ((VPPDMA_VPP_WIDTH == 720) && (VPPDMA_VPP_HEIGHT == 576))
	vscl_setting((720-VPPDMA_VPP_WIDTH)>>1, (576-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 720, 576);
	#elif ((VPPDMA_VPP_WIDTH == 1280) && (VPPDMA_VPP_HEIGHT == 720))
	vscl_setting((1280-VPPDMA_VPP_WIDTH)>>1, (720-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 1280, 720);
	#elif ((VPPDMA_VPP_WIDTH == 1920) && (VPPDMA_VPP_HEIGHT == 1080))
	vscl_setting((1920-VPPDMA_VPP_WIDTH)>>1, (1080-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 1920, 1080);
	#endif
	#ifdef SP_DISP_VPP_FIXED_ADDR
	vpp_alloc_size = (VPPDMA_FMT_HDMI)?(VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT*2):(VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT*3);
	vppdma_data_ptr = ioremap(0x00120000, vpp_alloc_size);
	memcpy(vppdma_data_ptr, vppdma_data_array, vpp_alloc_size);
	vppdma_setting(0x00120000, 0x00120000 + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	#else
	pa = __pa(vppdma_data_array);
	pa &= ~0x80000000;
	vppdma_setting(pa, pa + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	
	//vppdma_data_ptr = ioremap(0x24000000, sizeof(vppdma_data_array));
	//memcpy(vppdma_data_ptr, vppdma_data_array, sizeof(vppdma_data_array));
	//vppdma_setting(0x24000000, 0x24000000 + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	//vppdma_setting(virt_to_phys(vppdma_data_array), virt_to_phys((vppdma_data_array + VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT)), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	//sp_disp_info("virt_to_phys(vppdma_data_array) %ld \n",virt_to_phys(vppdma_data_array));

	//vppdma_setting(virt_to_phys(vppdma_data_array), virt_to_phys((vppdma_data_array + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT)), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	#endif

	#ifdef SP_DISP_V4L2_SUPPORT
	if(disp_dev->dev[2]) {
		disp_dev->dev[2]->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		disp_dev->dev[2]->fmt.fmt.pix.width = VPPDMA_VPP_WIDTH;
		disp_dev->dev[2]->fmt.fmt.pix.height = VPPDMA_VPP_HEIGHT;

		//0:RGB888 , 1:RGB565 , 2:YUV422_UYVY , 3:YUV422_NV16 , 4:YUV422_YUY2
		#if (VPPDMA_FMT_HDMI == 0)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
		#elif (VPPDMA_FMT_HDMI == 1)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
		#elif (VPPDMA_FMT_HDMI == 2)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
		#elif (VPPDMA_FMT_HDMI == 3)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV16;
		#elif (VPPDMA_FMT_HDMI == 4)
		disp_dev->dev[2]->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		#endif

		disp_dev->dev[2]->fmt.fmt.pix.field = V4L2_FIELD_NONE;
		disp_dev->dev[2]->fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
		disp_dev->dev[2]->fmt.fmt.pix.priv = 0;

		memset(fmtstr, 0, 8);
		memcpy(fmtstr, &disp_dev->dev[2]->fmt.fmt.pix.pixelformat, 4);
		sp_disp_dbg("dev[2] vpp0_format = %s \n", fmtstr);
		sp_disp_dbg("VPPDMA_VPP_WIDTH = %d , VPPDMA_VPP_HEIGHT = %d \n", VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT);
	}
	#endif
	#endif

	return 0;

}

void sp_disp_set_vpp_resolution_v4l2(struct sp_disp_device *disp_dev, int is_hdmi)
{
	sp_disp_dbg("set vpp resolution \n");

	#if (DDFCH_FETCH_EN == 1) // DDFCH fetch data
	#if ((DDFCH_VPP_WIDTH == 720) && (DDFCH_VPP_HEIGHT == 480))
	vscl_setting((720-DDFCH_VPP_WIDTH)>>1, (480-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 720, 480);
	#elif ((DDFCH_VPP_WIDTH == 720) && (DDFCH_VPP_HEIGHT == 576))
	vscl_setting((720-DDFCH_VPP_WIDTH)>>1, (576-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 720, 576);	
	#elif ((DDFCH_VPP_WIDTH == 1280) && (DDFCH_VPP_HEIGHT == 720))
	vscl_setting((1280-DDFCH_VPP_WIDTH)>>1, (720-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 1280, 720);
	#elif ((DDFCH_VPP_WIDTH == 1920) && (DDFCH_VPP_HEIGHT == 1080))
	vscl_setting((1920-DDFCH_VPP_WIDTH)>>1, (1080-DDFCH_VPP_HEIGHT)>>1, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, 1920, 1080);
	#endif
	#ifdef SP_DISP_VPP_FIXED_ADDR
	vpp_alloc_size = (DDFCH_FMT_HDMI)?(ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT*2):(ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT*3/2);
	vpp_yuv_ptr = ioremap(0x00120000, vpp_alloc_size);
	memcpy(vpp_yuv_ptr, vpp_yuv_array, vpp_alloc_size);
	ddfch_setting(0x00120000, 0x00120000 + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	#else

	pa = __pa(vpp_yuv_array);
	pa &= ~0x80000000;	
	ddfch_setting(pa, pa + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT, DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	//ddfch_setting(virt_to_phys(vpp_yuv_array), virt_to_phys((vpp_yuv_array + ALIGN(DDFCH_VPP_WIDTH, 128)*DDFCH_VPP_HEIGHT)), DDFCH_VPP_WIDTH, DDFCH_VPP_HEIGHT, DDFCH_FMT_HDMI);
	#endif
	#endif

	#if (VPPDMA_FETCH_EN == 1) //VPPDMA fetch data
	#if ((VPPDMA_VPP_WIDTH == 640) && (VPPDMA_VPP_HEIGHT == 480))
	vscl_setting((640-VPPDMA_VPP_WIDTH)>>1, (480-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 640, 480);
	#elif ((VPPDMA_VPP_WIDTH == 720) && (VPPDMA_VPP_HEIGHT == 480))
	vscl_setting((720-VPPDMA_VPP_WIDTH)>>1, (480-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 720, 480);
	#elif ((VPPDMA_VPP_WIDTH == 720) && (VPPDMA_VPP_HEIGHT == 576))
	vscl_setting((720-VPPDMA_VPP_WIDTH)>>1, (576-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 720, 576);
	#elif ((VPPDMA_VPP_WIDTH == 1280) && (VPPDMA_VPP_HEIGHT == 720))
	vscl_setting((1280-VPPDMA_VPP_WIDTH)>>1, (720-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 1280, 720);
	#elif ((VPPDMA_VPP_WIDTH == 1920) && (VPPDMA_VPP_HEIGHT == 1080))
	vscl_setting((1920-VPPDMA_VPP_WIDTH)>>1, (1080-VPPDMA_VPP_HEIGHT)>>1, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, 1920, 1080);
	#endif
	#ifdef SP_DISP_VPP_FIXED_ADDR
	vpp_alloc_size = (VPPDMA_FMT_HDMI)?(VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT*2):(VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT*3);
	vppdma_data_ptr = ioremap(0x20300000, vpp_alloc_size);
	memcpy(vppdma_data_ptr, vppdma_data_array, vpp_alloc_size);
	vppdma_setting(0x20300000, 0x20300000 + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT, VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	#else
	pa = __pa(vppdma_data_array);
	pa &= ~0x80000000;
	vppdma_setting(pa, pa + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	
	//vppdma_data_ptr = ioremap(0x24000000, sizeof(vppdma_data_array));
	//memcpy(vppdma_data_ptr, vppdma_data_array, sizeof(vppdma_data_array));
	//vppdma_setting(0x24000000, 0x24000000 + (VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);	
	//vppdma_setting(virt_to_phys(vppdma_data_array), virt_to_phys((vppdma_data_array + VPPDMA_VPP_WIDTH * VPPDMA_VPP_HEIGHT)), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	sp_disp_dbg("virt_to_phys(vppdma_data_array) %ld \n",virt_to_phys(vppdma_data_array));

	//vppdma_setting(virt_to_phys(vppdma_data_array), virt_to_phys((vppdma_data_array + VPPDMA_VPP_WIDTH*VPPDMA_VPP_HEIGHT)), VPPDMA_VPP_WIDTH, VPPDMA_VPP_HEIGHT, VPPDMA_FMT_HDMI);
	#endif
	#endif

}
EXPORT_SYMBOL(sp_disp_set_vpp_resolution_v4l2);

static int _display_probe(struct platform_device *pdev)
{
	struct sp_disp_device *disp_dev;
	int ret;
	int is_hdmi = 1;
	DISP_REG_t *pTmpRegBase = NULL;

#ifdef SP_DISP_V4L2_SUPPORT
	int i;
#endif

	#if 0 //size check
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

	#ifdef SP_DISP_V4L2_SUPPORT
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

	ret = sp_disp_get_register_base(pdev, (void**)&pTmpRegBase,0);
	if (ret) {
		return ret;
	}

	disp_dev->pHWRegBase = pTmpRegBase;

	ret = _display_init_clk(pdev);
	if (ret)
	{
		sp_disp_err("init clk error.\n");
		return ret;
	}

	DRV_DMIX_Init((void*)&pTmpRegBase->dmix);
	DRV_TGEN_Init((void*)&pTmpRegBase->tgen);
	DRV_DVE_Init((void*)&pTmpRegBase->dve);
	DRV_VPP_Init((void*)&pTmpRegBase->vpost, (void*)&pTmpRegBase->vscl, (void*)&pTmpRegBase->ddfch, (void*)&pTmpRegBase->vppdma);
	DRV_OSD_Init((void*)&pTmpRegBase->osd, (void*)&pTmpRegBase->gpost);
	DRV_OSD1_Init((void*)&pTmpRegBase->osd1, (void*)&pTmpRegBase->gpost1);

	//DRV_DVE_dump();
	
	// DMIX setting
	/****************************************
	* BG: PTG
	* L1: VPP0
	* L5: OSD1
	* L6: OSD0
	*****************************************/
	DRV_DMIX_Layer_Init(DRV_DMIX_BG, DRV_DMIX_AlphaBlend, DRV_DMIX_PTG);
	DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_Opacity, DRV_DMIX_VPP0);
	//DRV_DMIX_Layer_Init(DRV_DMIX_L1, DRV_DMIX_AlphaBlend, DRV_DMIX_VPP0);
	DRV_DMIX_Layer_Init(DRV_DMIX_L5, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD1);
	DRV_DMIX_Layer_Init(DRV_DMIX_L6, DRV_DMIX_AlphaBlend, DRV_DMIX_OSD0);

#ifdef SP_DISP_V4L2_SUPPORT
	ret = sp_disp_initialize(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}

	for (i = 0; i < SP_DISP_MAX_DEVICES; i++) {
		if (sp_disp_init_multi_layer(i, pdev, disp_dev)) {
			ret = -ENODEV;
			return ret;
		}
		if (sp_disp_reg_multi_layer(i, pdev, disp_dev)) {
			ret = -ENODEV;
			return ret;
		}		
	}
#endif

	ret = sp_disp_set_osd_resolution(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}

	//DRV_OSD0_dump();
	//DRV_OSD1_dump();

	ret = sp_disp_set_vpp_resolution(&pdev->dev, disp_dev);
	if (ret) {
		return ret;
	}

	//DRV_VPPDMA_dump();
	//DRV_DDFCH_dump();
	//DRV_VSCL_dump();

	ret = sp_disp_set_output_resolution(disp_dev,is_hdmi);
	if (ret) {
		return ret;
	}

#ifdef SUPPORT_DEBUG_MON
	entry = proc_create(MON_PROC_NAME, S_IRUGO, NULL, &proc_fops);
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	sp_disp_dbg("[%s:%d] runtime enable \n", __FUNCTION__, __LINE__);
	pm_runtime_set_autosuspend_delay(&pdev->dev,5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif

	sp_disp_info("[%s:%d] disp probe done \n", __FUNCTION__, __LINE__);

	return 0;

err_alloc:
	printk(KERN_INFO "disp probe fail \n");
#ifdef SP_DISP_V4L2_SUPPORT
	v4l2_device_unregister(&disp_dev->v4l2_dev);
#endif
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

	sp_disp_dbg("[%s:%d]\n", __FUNCTION__, __LINE__);

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

#ifdef SP_DISP_V4L2_SUPPORT
	//video_unregister_device(&disp_dev->video_dev);
	for (i = 0; i < SP_DISP_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		disp_layer = disp_dev->dev[i];
		/* Unregister video device */
		video_unregister_device(&disp_layer->video_dev);
	}
	for (i = 0; i < SP_DISP_MAX_DEVICES; i++) {
		kfree(disp_dev->dev[i]);
		disp_dev->dev[i] = NULL;
	}

	v4l2_device_unregister(&disp_dev->v4l2_dev);
	kfree(disp_dev);
#endif

#ifdef SUPPORT_DEBUG_MON
	if (entry)
		remove_proc_entry(MON_PROC_NAME, NULL);
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	sp_disp_dbg("[%s:%d] runtime disable \n", __FUNCTION__, __LINE__);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
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

