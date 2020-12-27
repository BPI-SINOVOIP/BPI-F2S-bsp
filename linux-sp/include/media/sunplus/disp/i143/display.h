/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Sunplus I143 Display driver function header file
 *
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#define UI_FORCE_ALPHA
//Choose SYS_PORT(fast) or MEM_PORT(slow)
#define I143_SYS_PORT
//HDMI Setting
#define TIMING_SYNC_720P60

#define USE_THREE_PATH
/* *****************************************************
 *  (VPP0 = DDFCH/VPPDMA)
 *  VPP0 -- VSCL -- VPOST -- TGEN/DMIX -- DVE -- HDMITX
 *         OSD0 -- GPOST0 -- TGEN/DMIX -- DVE -- HDMITX
 *         OSD1 -- GPOST0 -- TGEN/DMIX -- DVE -- HDMITX
 * ***************************************************** */

//#define USE_TWO_PATH
/* *****************************************************
 *  OSD0 -- VSCL -- VPOST -- TGEN/DMIX -- DVE -- HDMITX
 *  OSD1 ---------- GPOST0-- TGEN/DMIX -- DVE -- HDMITX
 * ***************************************************** */

#if defined (USE_THREE_PATH)
	//Display VIDEO OUT Resolution
	#define DISP_480P			//DISP_480P , DISP_576P , DISP_720P , DISP_1080P
	//Display Path En/dis
	#define DDFCH_GRP_EN	0 	//0:DDFCH dis ; 1:DDFCH en
	#define VPPDMA_GRP_EN	1 	//0:VPPDMA dis ; 1:VPPDMA en
	#define OSD0_GRP_EN		0 	//0:OSD0 dis ; 1:OSD0 en
	#define OSD1_GRP_EN		0	//0:OSD1 dis ; 1:OSD1 en
	//Display Data fetch En/dis
	#define DDFCH_FETCH_EN	0	//0:DDFCH fetch dis ; 1:DDFCH fetch en
	#define VPPDMA_FETCH_EN	1	//0:VPPDMA fetch dis ; 1:VPPDMA en
	#define OSD0_FETCH_EN	0	//0:OSD0 fetch dis ; 1:OSD0 fetch en
	#define OSD1_FETCH_EN	0	//0:OSD1 fetch dis ; 1:OSD1 fetch en
	//Display Path Data Format
	#define DDFCH_FMT_HDMI	2	//0:YUV420_NV12 , 1:YUV422_NV16 , 2:YUV422_YUY2
	#define VPPDMA_FMT_HDMI	2	//0:RGB888 , 1:RGB565 , 2:YUV422_UYVY , 3:YUV422_NV16 , 4:YUV422_YUY2
	#define OSD0_FMT_HDMI	0xe	//0x2:8bpp     , 0x4:YUY2     , 0x8:RGB565   , 0x9:ARGB1555
								//0xa:RGBA4444 , 0xb:ARGB4444 , 0xd:RGBA8888 , 0xe:ARGB8888 
	#define OSD1_FMT_HDMI	0xe	//0x2:8bpp     , 0x4:YUY2     , 0x8:RGB565   , 0x9:ARGB1555
								//0xa:RGBA4444 , 0xb:ARGB4444 , 0xd:RGBA8888 , 0xe:ARGB8888

	#define VPP_PATH_SEL	0 	//0:From VPPDMA , 1:From OSD0 , 2:From DDFCH
#elif defined (USE_TWO_PATH)
	//Display VIDEO OUT Resolution
	#define DISP_480P			//DISP_64X64 , DISP_480P , DISP_576P , DISP_720P , DISP_1080P
	//Display Path En/dis
	#define DDFCH_GRP_EN	0 	//0:DDFCH dis ; 1:DDFCH en
	#define VPPDMA_GRP_EN	0 	//0:VPPDMA dis ; 1:VPPDMA en
	#define OSD0_GRP_EN		1 	//0:OSD0 dis ; 1:OSD0 en
	#define OSD1_GRP_EN		1	//0:OSD1 dis ; 1:OSD1 en
	//Display Data fetch En/dis
	#define DDFCH_FETCH_EN	0	//0:DDFCH fetch dis ; 1:DDFCH fetch en
	#define VPPDMA_FETCH_EN	0	//0:VPPDMA fetch dis ; 1:VPPDMA en
	#define OSD0_FETCH_EN	1	//0:OSD0 fetch dis ; 1:OSD0 fetch en
	#define OSD1_FETCH_EN	0	//0:OSD1 fetch dis ; 1:OSD1 fetch en
	//Display Path Data Format
	#define DDFCH_FMT_HDMI	2	//0:YUV420_NV12 , 1:YUV422_NV16 , 2:YUV422_YUY2
	#define VPPDMA_FMT_HDMI	2	//0:RGB888 , 1:RGB565 , 2:YUV422_UYVY , 3:YUV422_NV16 , 4:YUV422_YUY2
	#define OSD0_FMT_HDMI	0xe	//0x2:8bpp     , 0x4:YUY2     , 0x8:RGB565   , 0x9:ARGB1555
								//0xa:RGBA4444 , 0xb:ARGB4444 , 0xd:RGBA8888 , 0xe:ARGB8888 
	#define OSD1_FMT_HDMI	0xe	//0x2:8bpp     , 0x4:YUY2     , 0x8:RGB565   , 0x9:ARGB1555
								//0xa:RGBA4444 , 0xb:ARGB4444 , 0xd:RGBA8888 , 0xe:ARGB8888

	#define VPP_PATH_SEL	1 	//0:From VPPDMA , 1:From OSD0 , 2:From DDFCH
#endif

//Display VIDEO IN Resolution
#ifdef DISP_480P
#define DDFCH_VPP_WIDTH		720 //(selectable ie. 320/640/720/1280/1920 , align 128)
#define DDFCH_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define VPPDMA_VPP_WIDTH	720 //(selectable ie. 320/640/720/1280/1920)
#define VPPDMA_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define OSD0_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD0_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define OSD1_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD1_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define VIDEO_OUT_WIDTH 	720 //fixed , don't change it
#define VIDEO_OUT_HEIGHT	480 //fixed , don't change it
#endif
#ifdef DISP_576P
#define DDFCH_VPP_WIDTH		720 //(selectable ie. 320/640/720/1280/1920 , align 128)
#define DDFCH_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define VPPDMA_VPP_WIDTH	720 //(selectable ie. 320/640/720/1280/1920)
#define VPPDMA_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define OSD0_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD0_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define OSD1_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD1_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define VIDEO_OUT_WIDTH 	720 //fixed , don't change it
#define VIDEO_OUT_HEIGHT	576 //fixed , don't change it
#endif
#ifdef DISP_720P
#define DDFCH_VPP_WIDTH		720 //(selectable ie. 320/640/720/1280/1920 , align 128)
#define DDFCH_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define VPPDMA_VPP_WIDTH	720 //(selectable ie. 320/640/720/1280/1920)
#define VPPDMA_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define OSD0_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD0_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define OSD1_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD1_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define VIDEO_OUT_WIDTH 	1280//fixed , don't change it
#define VIDEO_OUT_HEIGHT	720 //fixed , don't change it
#endif
#ifdef DISP_1080P
#define DDFCH_VPP_WIDTH		720 //(selectable ie. 320/640/720/1280/1920 , align 128)
#define DDFCH_VPP_HEIGHT	480 //(selectable ie. 240/480/576/720/1080)
#define VPPDMA_VPP_WIDTH	1920 //(selectable ie. 320/640/720/1280/1920)
#define VPPDMA_VPP_HEIGHT	1080 //(selectable ie. 240/480/576/720/1080)
#define OSD0_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD0_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define OSD1_WIDTH			720 //(selectable ie. 320/640/720/1280/1920)
#define OSD1_HEIGHT			480 //(selectable ie. 240/480/576/720/1080)
#define VIDEO_OUT_WIDTH 	1920//fixed , don't change it
#define VIDEO_OUT_HEIGHT	1080//fixed , don't change it
#endif

// define VPP_XXX_IN/VPP_XXX_OUT setting for VSCL zoom_in/zoom_out
#if (VPP_PATH_SEL == 0) //0:From VPPDMA
#define VPP_WIDTH_IN 			VPPDMA_VPP_WIDTH
#define VPP_HEIGHT_IN			VPPDMA_VPP_HEIGHT
#elif (VPP_PATH_SEL == 1) //1:From OSD0
#define VPP_WIDTH_IN 			OSD0_WIDTH
#define VPP_HEIGHT_IN			OSD0_HEIGHT
#elif (VPP_PATH_SEL == 2) //2:From DDFCH
#define VPP_WIDTH_IN 			DDFCH_VPP_WIDTH
#define VPP_HEIGHT_IN			DDFCH_VPP_HEIGHT
#endif
#define VPP_WIDTH_OUT 		VIDEO_OUT_WIDTH
#define VPP_HEIGHT_OUT		VIDEO_OUT_HEIGHT

//Debug Setting
#define SUPPORT_DEBUG_MON

#endif	//__DISPLAY_H__
