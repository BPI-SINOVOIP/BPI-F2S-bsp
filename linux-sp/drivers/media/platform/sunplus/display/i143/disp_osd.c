// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Sunplus Technology, Inc.
 *
 * Sunplus I143 Display driver for osd layer
 *
 * Authors: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 */

/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <media/sunplus/disp/i143/display.h>
#include "reg_disp.h"
#include "hal_disp.h"
#include <media/sunplus/disp/i143/disp_osd.h>
#include "disp_palette.h"

#ifdef CONFIG_PM_RUNTIME_DISP
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#endif
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
struct HW_OSD_Header_s {
	u8 config0;	//config0 includes:
	// [bit 7] cu	: color table update
	// [bit 6] ft	: force transparency
	// [bit 5:4]	: reserved
	// [bit 3:0] md : bitmap format (color mode)

	u8 reserved0; // reserved bits.

	u8 config1;	//config1 includes:
	// [bit 7:5]	: reserved
	// [bit 4] b_s	: byte swap enable
	// [bit 3] KY	: reserved
	// [bit 2] bl2	: region blend alpha enable (multiplier)
	// [bit 1]		: reserved
	// [bit 0] bl	: region blend alpha enable (replace)

	u8 blend_level;	//region blend level value

	u16 v_size;		//vertical display region size (line number)
	u16 h_size;		//horizontal display region size (pixel number)

	u16 disp_start_row;		//region vertical start row (0~(y-1))
	u16 disp_start_column;	//region horizontal start column (0~(x-1))

	u8 keying_R;
	u8 keying_G;
	u8 keying_B;
	u8 keying_A;

	u16 data_start_row;
	u16 data_start_column;

	u8 reserved2;
	u8 csc_mode_sel; //color space converter mode sel
	u16 data_width;	//source bitmap crop width

	u32 link_next;
	u32 link_data;

	u32 reserved3[24];	// need 128 bytes for HDR
};
STATIC_ASSERT(sizeof(struct HW_OSD_Header_s) == 128);

struct Region_Manager_s {
	DRV_Region_Info_t	RegionInfo;

	enum DRV_OsdRegionFormat_e	Format;
	u32					Align;
	u32					NumBuff;
	u32					DataPhyAddr;
	u8					*DataAddr;
	//palette addr in osd header
	u8					*Hdr_ClutAddr;
	u32					BmpSize;
	u32					CurrBufID;

	// SW latch
	u32					DirtyFlag;
	//other side palette addr, Gearing with swap buffer.
	u8					*PaletteAddr;

	struct HW_OSD_Header_s		*pHWRegionHdr;
	//structure size should be 32 alignment.
	u32 reserved[4];
};
STATIC_ASSERT((sizeof(struct Region_Manager_s) % 4) == 0);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static DISP_OSD_REG_t *pOSDReg;
static DISP_GPOST_REG_t *pGPOSTReg;
static DISP_OSD1_REG_t *pOSD1Reg;
static DISP_GPOST1_REG_t *pGPOST1Reg;

#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
static struct Region_Manager_s *gpWinRegion;
static u32 gpWinRegion_phy;
static u8 *gpOsdHeader;
static u32 gpOsdHeader_phy;
#else
static struct Region_Manager_s *gpWinRegion;
static u64 gpWinRegion_phy;
static u8 *gpOsdHeader;
static u64 gpOsdHeader_phy;
#endif
static const char * const osd_status[] = {"dis", "en", "unknown"};
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_OSD_Init(void *pInHWReg1, void *pInHWReg2)
{
	struct sp_disp_device *pDispWorkMem = gDispWorkMem;

	pOSDReg = (DISP_OSD_REG_t *)pInHWReg1;
	pGPOSTReg = (DISP_GPOST_REG_t *)pInHWReg2;

	pOSDReg->osd_ctrl 				= 0x00b7;	//G196.00
#if (OSD0_GRP_EN == 1)
	pOSDReg->osd_en 				= 0x0001;	//G196.01
#endif
	//pOSDReg->osd_base_addr			= 0x20200000;	//G196.02
	
	pOSDReg->osd_hvld_width 		= OSD0_WIDTH;		//G196.17
	pOSDReg->osd_vvld_offset 		= 0;		//G196.18
	pOSDReg->osd_vvld_height		= OSD0_HEIGHT;		//G196.19
	pOSDReg->osd_data_fetch_ctrl	= 0x0af8;	//G196.20
	pOSDReg->osd_bist_ctrl			= 0x00;		//G196.21

	init_waitqueue_head(&pDispWorkMem->osd_wait);
	spin_lock_init(&pDispWorkMem->osd_lock);

}

#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
DRV_Status_e DRV_OSD_SetClut(DRV_OsdRegionHandle_t region, u32 *pClutDataPtr)
{
	struct Region_Manager_s *pRegionManager =
		(struct Region_Manager_s *)region;

	u32 copysize = 0;

	if (pRegionManager && pClutDataPtr) {
		//switch (pRegionManager->Format) {
		//case DRV_OSD_REGION_FORMAT_8BPP:
			copysize = 256 * 4;
		//	break;
		//default:
		//	goto Return;
		//}
		memcpy(pRegionManager->Hdr_ClutAddr, pClutDataPtr, copysize);

		return DRV_SUCCESS;
	}

//Return:
//	sp_disp_err("Incorrect region handle, pClutDataPtr 0x%x\n",
//			(u32)pClutDataPtr);
//
//	return DRV_ERR_INVALID_HANDLE;
	return DRV_SUCCESS;
}
#else
DRV_Status_e DRV_OSD_SetClut(DRV_OsdRegionHandle_t region, u64 *pClutDataPtr)
{
	struct Region_Manager_s *pRegionManager =
		(struct Region_Manager_s *)region;

	u32 copysize = 0;

	if (pRegionManager && pClutDataPtr) {
		//switch (pRegionManager->Format) {
		//case DRV_OSD_REGION_FORMAT_8BPP:
			copysize = 256 * 4;
		//	break;
		//default:
		//	goto Return;
		//}
		memcpy(pRegionManager->Hdr_ClutAddr, pClutDataPtr, copysize);

		return DRV_SUCCESS;
	}

//Return:
//	sp_disp_err("Incorrect region handle, pClutDataPtr 0x%llx\n",
//			(u64)pClutDataPtr);
//
//	return DRV_ERR_INVALID_HANDLE;
	return DRV_SUCCESS;
}
#endif

void DRV_OSD_IRQ(void)
{
	struct Region_Manager_s *pRegionManager = gpWinRegion;
	struct HW_OSD_Header_s *pHWOSDhdr;
	struct sp_disp_device *pDispWorkMem = gDispWorkMem;

	if (!pRegionManager)
		return;
	spin_lock(&pDispWorkMem->osd_lock);

	pHWOSDhdr = (struct HW_OSD_Header_s *)
		pRegionManager->pHWRegionHdr;
	if (pHWOSDhdr &&
			(pRegionManager->DirtyFlag & REGION_ADDR_DIRTY)) {
		pRegionManager->DirtyFlag &= ~REGION_ADDR_DIRTY;
		pHWOSDhdr->link_data = SWAP32(
				(u32)((u32)pRegionManager->DataPhyAddr
					+ (pRegionManager->BmpSize
					* (pRegionManager->CurrBufID & 0xf))));

		#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP			
		if (pRegionManager->PaletteAddr)
			DRV_OSD_SetClut((DRV_OsdRegionHandle_t)pRegionManager,
					(u32 *)pRegionManager->PaletteAddr);
		#else
		if (pRegionManager->PaletteAddr)
			DRV_OSD_SetClut((DRV_OsdRegionHandle_t)pRegionManager,
					(u64 *)pRegionManager->PaletteAddr);
		#endif
	}

	if (pDispWorkMem->osd_field_end_protect) {
		pDispWorkMem->osd_field_end_protect &= ~(1 << DRV_OSD0);
		wake_up_interruptible(&pDispWorkMem->osd_wait);
	}

	spin_unlock(&pDispWorkMem->osd_lock);
}

//#ifdef SUPPORT_DEBUG_MON
void DRV_OSD_Info(void)
{
	struct HW_OSD_Header_s *pOsdHdr = (struct HW_OSD_Header_s *)gpOsdHeader;

	sp_disp_info("Region display-order is as follows:\n");

	sp_disp_info("    Check osd output: %d %d, region ouput:%d %d\n",
		pOSDReg->osd_hvld_width,
		pOSDReg->osd_vvld_height,
		SWAP16(pOsdHdr->h_size),
		SWAP16(pOsdHdr->v_size));

	sp_disp_info("header: (x, y)=(%d, %d) (w, h)=(%d, %d) data(x, y)=(%d, %d) data width=%d\n",
				SWAP16(pOsdHdr->disp_start_column),
				SWAP16(pOsdHdr->disp_start_row),
				SWAP16(pOsdHdr->h_size),
				SWAP16(pOsdHdr->v_size),
				SWAP16(pOsdHdr->data_start_column),
				SWAP16(pOsdHdr->data_start_row),
				SWAP16(pOsdHdr->data_width));
	sp_disp_info("cu:%d ft:%d bit format:%d link data:0x%x\n\n",
			(pOsdHdr->config0 & 0x80) ? 1 : 0,
			(pOsdHdr->config0 & 0x40) ? 1 : 0,
			(pOsdHdr->config0 & 0xf),
			SWAP32(pOsdHdr->link_data));
}

void DRV_OSD_HDR_Show(void)
{
	#if 0 //OSD HEADER data
	int *ptr = (int *)gpOsdHeader;
	int i;
	for (i = 0; i < 8; ++i)
		sp_disp_info("%d: 0x%08x\n", i, *(ptr+i));
	#else //OSD HEADER data (with swap)
	int i;
	u32 *osd_header;
	osd_header = (u32 *)gpOsdHeader;
	sp_disp_info("-- osd header --\n");
	for (i = 0; i < 8; ++i)
		sp_disp_info("%d: 0x%08x\n", i, osd_header[i]);
	sp_disp_info("-- osd header(swap) --\n");
	for (i = 0; i < 256; ++i) {
		if(i%32 ==0)
			sp_disp_info("%d: 0x%08x\n", i, SWAP32(osd_header[32+i]));
	}
	#endif
}

void DRV_OSD_HDR_Write(int offset, int value)
{
	int *ptr = (int *)gpOsdHeader;

	*(ptr+offset) = value;
}
//#endif

void DRV_OSD_GetColormode_Vars(struct colormode_t *var,
		enum DRV_OsdRegionFormat_e Fmt)
{
	switch (Fmt) {
	case DRV_OSD_REGION_FORMAT_8BPP:
		strcpy(var->name, "256color index");
		var->red.length		= 8;
		var->green.length	= 8;
		var->blue.length	= 8;
		var->transp.length	= 8;
		var->red.offset		= 8;
		var->green.offset	= 16;
		var->blue.offset	= 24;
		var->transp.offset	= 0;
		var->bits_per_pixel = 8;
		break;
	case DRV_OSD_REGION_FORMAT_YUY2:
		strcpy(var->name, "YUY2");
		var->nonstd			= 1;
		var->bits_per_pixel = 16;
		break;
	case DRV_OSD_REGION_FORMAT_RGB_565:
		strcpy(var->name, "RGB565");
		var->red.length		= 5;
		var->green.length	= 6;
		var->blue.length	= 5;
		var->transp.length	= 0;
		var->red.offset		= 11;
		var->green.offset	= 5;
		var->blue.offset	= 0;
		var->transp.offset	= 0;
		var->bits_per_pixel = 16;
		break;
	case DRV_OSD_REGION_FORMAT_ARGB_1555:
		strcpy(var->name, "ARGB1555");
		var->red.length		= 5;
		var->green.length	= 5;
		var->blue.length	= 5;
		var->transp.length	= 1;
		var->red.offset		= 10;
		var->green.offset	= 5;
		var->blue.offset	= 0;
		var->transp.offset	= 15;
		var->bits_per_pixel = 16;
		break;
	case DRV_OSD_REGION_FORMAT_RGBA_4444:
		strcpy(var->name, "RGBA4444");
		var->red.length		= 4;
		var->green.length	= 4;
		var->blue.length	= 4;
		var->transp.length	= 4;
		var->red.offset		= 12;
		var->green.offset	= 8;
		var->blue.offset	= 4;
		var->transp.offset	= 0;
		var->bits_per_pixel = 16;
		break;
	case DRV_OSD_REGION_FORMAT_ARGB_4444:
		strcpy(var->name, "ARGB4444");
		var->red.length		= 4;
		var->green.length	= 4;
		var->blue.length	= 4;
		var->transp.length	= 4;
		var->red.offset		= 8;
		var->green.offset	= 4;
		var->blue.offset	= 0;
		var->transp.offset	= 12;
		var->bits_per_pixel = 16;
		break;
	case DRV_OSD_REGION_FORMAT_RGBA_8888:
		strcpy(var->name, "RGBA8888");
		var->red.length		= 8;
		var->green.length	= 8;
		var->blue.length	= 8;
		var->transp.length	= 8;
		var->red.offset		= 24;
		var->green.offset	= 16;
		var->blue.offset	= 8;
		var->transp.offset	= 0;
		var->bits_per_pixel = 32;
		break;
	default:
	case DRV_OSD_REGION_FORMAT_ARGB_8888:
		strcpy(var->name, "ARGB8888");
		var->red.length		= 8;
		var->green.length	= 8;
		var->blue.length	= 8;
		var->transp.length	= 8;
		var->red.offset		= 16;
		var->green.offset	= 8;
		var->blue.offset	= 0;
		var->transp.offset	= 24;
		var->bits_per_pixel = 32;
		break;
	}
}

int DRV_OSD_Get_UI_Res(struct UI_FB_Info_t *pinfo)
{
	struct sp_disp_device *pDispWorkMem = gDispWorkMem;

	if (!pOSDReg || !pGPOSTReg)
		return 1;

	/* todo reference Output size */
	pinfo->UI_width = pDispWorkMem->UIRes.width;
	pinfo->UI_height = pDispWorkMem->UIRes.height;
	pinfo->UI_bufNum = 2;
	pinfo->UI_bufAlign = 4096;
	pinfo->UI_ColorFmt = pDispWorkMem->UIFmt;

	DRV_OSD_GetColormode_Vars(&pinfo->UI_Colormode, pinfo->UI_ColorFmt);

	return 0;
}
EXPORT_SYMBOL(DRV_OSD_Get_UI_Res);

void DRV_OSD_Set_UI_UnInit(void)
{
	if (!gpOsdHeader || !gpWinRegion)
		return;

	//if (gpWinRegion->Format == DRV_OSD_REGION_FORMAT_8BPP)
		dma_free_coherent(NULL,
				sizeof(struct HW_OSD_Header_s) + 1024,
				gpOsdHeader,
				gpOsdHeader_phy);
	//else
	//	dma_free_coherent(NULL,
	//			sizeof(struct HW_OSD_Header_s),
	//			gpOsdHeader,
	//			gpOsdHeader_phy);

	dma_free_coherent(NULL,
			sizeof(struct Region_Manager_s),
			gpWinRegion,
			gpWinRegion_phy);
}
EXPORT_SYMBOL(DRV_OSD_Set_UI_UnInit);

void DRV_OSD_Set_UI_Init(struct UI_FB_Info_t *pinfo)
{
	struct sp_disp_device *pDispWorkMem = gDispWorkMem;

	u32 *osd_header;
#if 0 //#ifdef CONFIG_PM_RUNTIME_DISP
	int ret;
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(pDispWorkMem->pdev) < 0)
		goto out;
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	pm_runtime_put(pDispWorkMem->pdev);		// Starting count timeout.
#endif

	//if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		gpOsdHeader = dma_alloc_coherent(pDispWorkMem->pdev,
				sizeof(struct HW_OSD_Header_s) + 1024,
				&gpOsdHeader_phy,
				GFP_KERNEL | __GFP_ZERO);
	//else
	//	gpOsdHeader = dma_alloc_coherent(pDispWorkMem->pdev,
	//			sizeof(struct HW_OSD_Header_s),
	//			&gpOsdHeader_phy,
	//			GFP_KERNEL | __GFP_ZERO);

	if (!gpOsdHeader) {
		sp_disp_err("malloc osd header fail\n");
		return;
	}

	gpWinRegion = dma_alloc_coherent(pDispWorkMem->pdev,
			sizeof(struct Region_Manager_s),
			&gpWinRegion_phy,
			GFP_KERNEL | __GFP_ZERO);

	if (!gpWinRegion) {
		sp_disp_err("malloc region header fail\n");
		return;
	}

	//todo
	//gpWinRegion->RegionInfo

	gpWinRegion->Format = pinfo->UI_ColorFmt;
	gpWinRegion->Align = pinfo->UI_bufAlign;
	gpWinRegion->NumBuff = pinfo->UI_bufNum;
	gpWinRegion->DataPhyAddr = pinfo->UI_bufAddr;
	gpWinRegion->DataAddr = (u8 *)pinfo->UI_bufAddr;
	gpWinRegion->Hdr_ClutAddr = gpOsdHeader
		+ sizeof(struct HW_OSD_Header_s);
	gpWinRegion->BmpSize = EXTENDED_ALIGNED(pinfo->UI_height
			* pinfo->UI_width
			* (pinfo->UI_Colormode.bits_per_pixel >> 3),
			pinfo->UI_bufAlign);
	gpWinRegion->PaletteAddr = (u8 *)pinfo->UI_bufAddr_pal;
	gpWinRegion->pHWRegionHdr = (struct HW_OSD_Header_s *)gpOsdHeader;

	sp_disp_dbg("osd_header=0x%x 0x%llx addr=0x%llx\n",
			(u32)((u64)gpOsdHeader),
			gpOsdHeader_phy,
			pinfo->UI_bufAddr);

	osd_header = (u32 *)gpOsdHeader;

#ifdef UI_FORCE_ALPHA
	if ( (pinfo->UI_ColorFmt >= DRV_OSD_REGION_FORMAT_ARGB_1555) && (pDispWorkMem->UIForceAlpha >=1) )
		osd_header[0] = SWAP32(0x00001100 | (pinfo->UI_ColorFmt << 24) | pDispWorkMem->UISetAlpha);
	else 
#endif
	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		osd_header[0] = SWAP32(0x82001000);
	else
		osd_header[0] = SWAP32(0x00001000 | (pinfo->UI_ColorFmt << 24));

	osd_header[1] = SWAP32((min(pinfo->UI_height, pDispWorkMem->panelRes.height) << 16) | min(pinfo->UI_width, pDispWorkMem->panelRes.width));
	osd_header[2] = 0;
	osd_header[3] = 0;
	osd_header[4] = 0;
	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_YUY2)
		osd_header[5] = SWAP32(0x00040000 | pinfo->UI_width);
	else
		osd_header[5] = SWAP32(0x00010000 | pinfo->UI_width);
	osd_header[6] = SWAP32(0xFFFFFFE0);
	osd_header[7] = SWAP32(pinfo->UI_bufAddr);

	//OSD
	pOSDReg->osd_ctrl = OSD_CTRL_COLOR_MODE_RGB
		| OSD_CTRL_CLUT_FMT_ARGB
		| OSD_CTRL_LATCH_EN
		| OSD_CTRL_A32B32_EN
		| OSD_CTRL_FIFO_DEPTH;
	pOSDReg->osd_base_addr = gpOsdHeader_phy;
	pOSDReg->osd_hvld_offset = 0;
	pOSDReg->osd_vvld_offset = 0;
	pOSDReg->osd_hvld_width = pDispWorkMem->panelRes.width;
	pOSDReg->osd_vvld_height = pDispWorkMem->panelRes.height;
	pOSDReg->osd_bist_ctrl = 0x0;
	pOSDReg->osd_3d_h_offset = 0x0;
	pOSDReg->osd_src_decimation_sel = 0x0;

	pOSDReg->osd_en = 1;

	//GPOST bypass
	pGPOSTReg->gpost0_config = 0;
	pGPOSTReg->gpost0_master_en = 0;
	pGPOSTReg->gpost0_bg1 = 0x8010;
	pGPOSTReg->gpost0_bg2 = 0x0080;

	//GPOST PQ disable
	pGPOSTReg->gpost0_contrast_config = 0x0;

	#if 0
	sp_disp_info("--- osd header --- \n");
	for (i = 0;i < 8; i++)
		sp_disp_info("osd_header[%d] 0x%08x \n",i, SWAP32(osd_header[i]));
	//sp_disp_info("--- osd palette --- \n");
	//for (i = 0;i < 256; i++)
	//	if (i%32 == 0)
	//		sp_disp_info("osd_palette[%d] 0x%08x \n",i, SWAP32(osd_palette[i]));
	#endif

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(pDispWorkMem->pdev);
	pm_runtime_put_autosuspend(pDispWorkMem->pdev);
	//return -ENOMEM;
#endif

}
EXPORT_SYMBOL(DRV_OSD_Set_UI_Init);

void DRV_OSD_WaitVSync(void)
{
	struct Region_Manager_s *pRegionManager = gpWinRegion;
	struct sp_disp_device *pDispWorkMem = gDispWorkMem;

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(pDispWorkMem->pdev) < 0)
		goto out;
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	pm_runtime_put(pDispWorkMem->pdev);		// Starting count timeout.
#endif

	if (!pRegionManager)
		return;

	if (pRegionManager->DirtyFlag & REGION_ADDR_DIRTY)
		pDispWorkMem->osd_field_end_protect |= 1 << DRV_OSD0;

	wait_event_interruptible_timeout(pDispWorkMem->osd_wait,
					!pDispWorkMem->osd_field_end_protect,
					msecs_to_jiffies(50));

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(pDispWorkMem->pdev);
	pm_runtime_put_autosuspend(pDispWorkMem->pdev);
	//return -ENOMEM;
#endif
}
EXPORT_SYMBOL(DRV_OSD_WaitVSync);

u32 DRV_OSD_SetVisibleBuffer(u32 bBufferId)
{
	struct Region_Manager_s *pRegionManager = gpWinRegion;

	if (!pRegionManager)
		//return -1;
		return 0;

	pRegionManager->DirtyFlag |= REGION_ADDR_DIRTY;
	pRegionManager->CurrBufID = bBufferId;

	return 0;
}
EXPORT_SYMBOL(DRV_OSD_SetVisibleBuffer);

void DRV_IRQ_DISABLE(void)
{
	g_disp_state = 1;
	//printk(KERN_INFO "FB_IRQ_DISABLE \n");
	//printk(KERN_INFO "g_disp_state = %d \n", g_disp_state);
}
EXPORT_SYMBOL(DRV_IRQ_DISABLE);

void DRV_IRQ_ENABLE(void)
{
	g_disp_state = 0;
	//printk(KERN_INFO "FB_IRQ_ENABLE \n");
	//printk(KERN_INFO "g_disp_state = %d \n", g_disp_state);
}
EXPORT_SYMBOL(DRV_IRQ_ENABLE);

void DRV_OSD1_Init(void *pInHWReg1, void *pInHWReg2)
{
	//struct sp_disp_device *pDispWorkMem = gDispWorkMem;

	pOSD1Reg = (DISP_OSD1_REG_t *)pInHWReg1;
	pGPOST1Reg = (DISP_GPOST1_REG_t *)pInHWReg2;

	//480P 720x480 setting
	pOSD1Reg->osd_ctrl 			= 0x00b7;	//G197.00
#if (OSD1_GRP_EN == 1)
	pOSD1Reg->osd_en 				= 0x0001;	//G197.01
#endif
	//pOSD1Reg->osd_base_addr 	= 0x20300000;	//G197.02
	
	pOSD1Reg->osd_hvld_width 	= OSD1_WIDTH;	//G197.17
	pOSD1Reg->osd_vvld_offset 	= 0;	//G197.18
	pOSD1Reg->osd_vvld_height 	= OSD1_HEIGHT;	//G197.19
	pOSD1Reg->osd_data_fetch_ctrl 	= 0x0af8;	//G197.20
	pOSD1Reg->osd_bist_ctrl 	= 0x00;	//G197.21
	//pOSD1Reg->osd_bist_ctrl 	= 0x80;	//G197.21 , color bar en , color bar
	//pOSD1Reg->osd_bist_ctrl 	= 0xC0;	//G197.21 , color bar en , border

	//init_waitqueue_head(&pDispWorkMem->osd_wait);
	//spin_lock_init(&pDispWorkMem->osd_lock);

}

void osd_path_cur_setting_read(void)
{

	sp_disp_dbg("osd_path_cur_setting_read \n");

	sp_disp_dbg("osd0(%s) w %d , h %d \n",osd_status[pOSDReg->osd_en] , pOSDReg->osd_hvld_width, pOSDReg->osd_vvld_height);
	sp_disp_dbg("osd1(%s) w %d , h %d \n",osd_status[pOSD1Reg->osd_en] , pOSD1Reg->osd_hvld_width, pOSD1Reg->osd_vvld_height);

}
EXPORT_SYMBOL(osd_path_cur_setting_read);

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
EXPORT_SYMBOL(DRV_OSD_SetColorbar);

int osd0_setting(int base_addr, int w, int h, int osd0_fmt)
{
	//struct sp_disp_device *disp_dev = gDispWorkMem;
	//u32 *osd_header;
	//osd_header = (u32 *)disp_dev->Osd0Header;
	
	sp_disp_info("osd0_setting fmt = %d \n", osd0_fmt);
	
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

#ifdef	SP_DISP_OSD_PARM
void DRV_OSD_Clear_OSD_Header(int osd_layer)
{
	struct sp_disp_device *disp_dev = gDispWorkMem;
	u32 *osd_header = NULL;
	int i;
	
	sp_disp_dbg("DRV_OSD_Clear_OSD_Header for osd%d \n",osd_layer);

	if ((!disp_dev->Osd0Header) || (!disp_dev->Osd1Header))
		return;

	if (osd_layer == 0) {
		osd_header = (u32 *)disp_dev->Osd0Header;
		//printk(KERN_INFO "disp_dev->Osd0Header %p \n",disp_dev->Osd0Header);
		//printk(KERN_INFO "disp_dev->Osd0Header_phy %x \n",disp_dev->Osd0Header_phy);
	}
	else if (osd_layer == 1) {
		osd_header = (u32 *)disp_dev->Osd1Header;
		//printk(KERN_INFO "disp_dev->Osd1Header %p \n",disp_dev->Osd1Header);
		//printk(KERN_INFO "disp_dev->Osd1Header_phy %x \n",disp_dev->Osd1Header_phy);
	}

	for (i = 0;i < 8; i++)
		osd_header[i] = 0;

}
EXPORT_SYMBOL(DRV_OSD_Clear_OSD_Header);

void DRV_OSD_INIT_OSD_Header(void)
{
	struct sp_disp_device *disp_dev = gDispWorkMem;

#if 0
	sp_disp_info("DRV_OSD_INIT_OSD_Header \n");

	sp_disp_info("sizeof(short) = %ld \n", sizeof(short));
	sp_disp_info("sizeof(int) = %ld \n", sizeof(int));
	sp_disp_info("sizeof(long) = %ld \n", sizeof(long));
	sp_disp_info("sizeof(long long) = %ld \n", sizeof(long long));
	
	sp_disp_info("sizeof(size_t) = %ld \n", sizeof(size_t));
	sp_disp_info("sizeof(off_t) = %ld \n", sizeof(off_t));
	sp_disp_info("sizeof(void *) = %ld \n", sizeof(void *));
	sp_disp_info("sizeof(struct HW_OSD_Header_s) = %ld \n", sizeof(struct HW_OSD_Header_s));
#endif

	#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
	disp_dev->Osd0Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd0Header_phy,
			GFP_KERNEL);
	#else
	disp_dev->Osd0Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd0Header_phy,
			GFP_KERNEL);
	#endif

	if (!disp_dev->Osd0Header) {
		sp_disp_err("malloc osd0 header fail\n");
		return;
	}
	//else {
	//	sp_disp_info("disp_dev->Osd0Header = %px \n",disp_dev->Osd0Header);
	//	sp_disp_info("disp_dev->Osd0Header_phy = %llx \n",disp_dev->Osd0Header_phy);
	//}
	//#endif

	#ifdef CONFIG_MACH_PENTAGRAM_I143_ACHIP
	disp_dev->Osd1Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd1Header_phy,
			GFP_KERNEL);
	#else
	disp_dev->Osd1Header = dma_alloc_coherent(disp_dev->pdev,
			sizeof(struct HW_OSD_Header_s) + 1024,
			&disp_dev->Osd1Header_phy,
			GFP_KERNEL);
	#endif
	
	if (!disp_dev->Osd1Header) {
		sp_disp_err("malloc osd1 header fail\n");
		return;
	}
	//else {
	//	sp_disp_info("disp_dev->Osd1Header = %px \n",disp_dev->Osd1Header);
	//	sp_disp_info("disp_dev->Osd1Header_phy = %llx \n",disp_dev->Osd1Header_phy);
	//}
	//#endif

}
EXPORT_SYMBOL(DRV_OSD_INIT_OSD_Header);

void DRV_OSD_SET_OSD_Header(struct UI_FB_Info_t *pinfo, int osd_layer)
{
	struct sp_disp_device *disp_dev = gDispWorkMem;

	u32 *osd_header = NULL;
	u32 *osd_palette = NULL;

	#if 1
	int i;
	#endif
#if 0 //#ifdef CONFIG_PM_RUNTIME_DISP
	int ret;
#endif

	sp_disp_dbg("DRV_OSD_SET_OSD_Header for osd%d \n", osd_layer);

#ifdef CONFIG_PM_RUNTIME_DISP
	if (pm_runtime_get_sync(disp_dev->pdev) < 0)
		goto out;
#endif

#ifdef CONFIG_PM_RUNTIME_DISP
	pm_runtime_put(disp_dev->pdev);		// Starting count timeout.
#endif

	//OSD Header/Palette set
	if (osd_layer == 0) {
		osd_header = (u32 *)disp_dev->Osd0Header;
		osd_palette = (u32 *)disp_dev->Osd0Header+32;
	}
	else if (osd_layer == 1) {
		osd_header = (u32 *)disp_dev->Osd1Header;
		osd_palette = (u32 *)disp_dev->Osd1Header+32;
	}
	
	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		osd_header[0] = SWAP32(0x82001000);
	else
		osd_header[0] = SWAP32(0x00001000 | (pinfo->UI_ColorFmt << 24));

	osd_header[1] = SWAP32((min(pinfo->UI_height, disp_dev->panelRes.height) << 16) | min(pinfo->UI_width, disp_dev->panelRes.width));
	osd_header[2] = 0;
	osd_header[3] = 0;
	osd_header[4] = 0;
	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_YUY2)
		osd_header[5] = SWAP32(0x00040000 | pinfo->UI_width);
	else
		osd_header[5] = SWAP32(0x00010000 | pinfo->UI_width);
	osd_header[6] = SWAP32(0xFFFFFFE0);
	osd_header[7] = SWAP32(pinfo->UI_bufAddr);

	//OSD Register set
	pOSDReg->osd_ctrl = OSD_CTRL_COLOR_MODE_RGB
		| OSD_CTRL_CLUT_FMT_ARGB
		| OSD_CTRL_LATCH_EN
		| OSD_CTRL_A32B32_EN
		| OSD_CTRL_FIFO_DEPTH;

	if (osd_layer == 0) {
		pOSDReg->osd_base_addr = disp_dev->Osd0Header_phy;
	}
	else if (osd_layer == 1) {
		pOSDReg->osd_base_addr = disp_dev->Osd1Header_phy;
	}		
	
	pOSDReg->osd_hvld_offset = 0;
	pOSDReg->osd_vvld_offset = 0;
	pOSDReg->osd_hvld_width = disp_dev->panelRes.width;
	pOSDReg->osd_vvld_height = disp_dev->panelRes.height;
	pOSDReg->osd_bist_ctrl = 0x0;
	pOSDReg->osd_3d_h_offset = 0x0;
	pOSDReg->osd_src_decimation_sel = 0x0;

	pOSDReg->osd_en = 1;

	//GPOST bypass
	pGPOSTReg->gpost0_config = 0;
	pGPOSTReg->gpost0_master_en = 0;
	pGPOSTReg->gpost0_bg1 = 0x8010;
	pGPOSTReg->gpost0_bg2 = 0x0080;

	//GPOST PQ disable
	pGPOSTReg->gpost0_contrast_config = 0x0;

	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_8BPP) {
		if (osd_layer == 0) {
			if (disp_dev->dev[0]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
				sp_disp_dbg("osd0 palette grey scale \n");
				for (i = 0; i < 256; i++) {
					osd_palette[i] = SWAP32(disp_osd_8bpp_pal_grey[i]); //8bpp grey scale table (argb)
					//osd_palette[i] = SWAP32(disp_osd_8bpp_pal_color[i]); //8bpp 256 color table (argb)
				}
			}
			else if (disp_dev->dev[0]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
				sp_disp_dbg("osd0 palette 256 color \n");
				for (i = 0; i < 256; i++) {
					//osd_palette[i] = SWAP32(disp_osd_8bpp_pal_grey[i]); //8bpp grey scale table (argb)
					osd_palette[i] = SWAP32(disp_osd_8bpp_pal_color[i]); //8bpp 256 color table (argb)
				}
			}
		}
		else if (osd_layer == 1) {
			if (disp_dev->dev[1]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY) {
				sp_disp_dbg("osd0 palette grey scale \n");
				for (i = 0; i < 256; i++) {
					osd_palette[i] = SWAP32(disp_osd_8bpp_pal_grey[i]); //8bpp grey scale table (argb)
					//osd_palette[i] = SWAP32(disp_osd_8bpp_pal_color[i]); //8bpp 256 color table (argb)
				}
			}
			else if (disp_dev->dev[1]->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_PAL8) {
				sp_disp_dbg("osd0 palette 256 color \n");
				for (i = 0; i < 256; i++) {
					//osd_palette[i] = SWAP32(disp_osd_8bpp_pal_grey[i]); //8bpp grey scale table (argb)
					osd_palette[i] = SWAP32(disp_osd_8bpp_pal_color[i]); //8bpp 256 color table (argb)
				}
			}
		}
		
	}

	#if 0
	sp_disp_info("--- osd header --- \n");
	for (i = 0;i < 8; i++)
		sp_disp_info("osd_header[%d] 0x%08x \n",i, SWAP32(osd_header[i]));
	sp_disp_info("--- osd palette --- \n");
	for (i = 0;i < 256; i++)
		if (i%32 == 0)
			sp_disp_info("osd_palette[%d] 0x%08x \n",i, SWAP32(osd_palette[i]));
	#endif

#ifdef CONFIG_PM_RUNTIME_DISP
out:
	pm_runtime_mark_last_busy(disp_dev->pdev);
	pm_runtime_put_autosuspend(disp_dev->pdev);
	//return -ENOMEM;
#endif

}
EXPORT_SYMBOL(DRV_OSD_SET_OSD_Header);
#endif

void DRV_OSD0_dump(void)
{
	//dump default setting
	printk(KERN_INFO "pOSDReg G196.00 0x%08x(%d) \n",pOSDReg->osd_ctrl,pOSDReg->osd_ctrl);
	printk(KERN_INFO "pOSDReg G196.01 0x%08x(%d) \n",pOSDReg->osd_en,pOSDReg->osd_en);
	printk(KERN_INFO "pOSDReg G196.02 0x%08x(%d) \n",pOSDReg->osd_base_addr,pOSDReg->osd_base_addr);
	printk(KERN_INFO "pOSDReg G196.16 0x%08x(%d) \n",pOSDReg->osd_hvld_offset,pOSDReg->osd_hvld_offset);
	printk(KERN_INFO "pOSDReg G196.17 0x%08x(%d) \n",pOSDReg->osd_hvld_width,pOSDReg->osd_hvld_width);
	printk(KERN_INFO "pOSDReg G196.18 0x%08x(%d) \n",pOSDReg->osd_vvld_offset,pOSDReg->osd_vvld_offset);
	printk(KERN_INFO "pOSDReg G196.19 0x%08x(%d) \n",pOSDReg->osd_vvld_height,pOSDReg->osd_vvld_height);
	printk(KERN_INFO "pOSDReg G196.20 0x%08x(%d) \n",pOSDReg->osd_data_fetch_ctrl,pOSDReg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSDReg G196.21 0x%08x(%d) \n",pOSDReg->osd_bist_ctrl,pOSDReg->osd_bist_ctrl);
}

void DRV_OSD1_dump(void)
{
	//dump after setting
	printk(KERN_INFO "pOSD1Reg G197.00 0x%08x(%d) \n",pOSD1Reg->osd_ctrl,pOSD1Reg->osd_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.01 0x%08x(%d) \n",pOSD1Reg->osd_en,pOSD1Reg->osd_en);
	printk(KERN_INFO "pOSD1Reg G197.02 0x%08x(%d) \n",pOSD1Reg->osd_base_addr,pOSD1Reg->osd_base_addr);
	printk(KERN_INFO "pOSD1Reg G197.16 0x%08x(%d) \n",pOSD1Reg->osd_hvld_offset,pOSD1Reg->osd_hvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.17 0x%08x(%d) \n",pOSD1Reg->osd_hvld_width,pOSD1Reg->osd_hvld_width);
	printk(KERN_INFO "pOSD1Reg G197.18 0x%08x(%d) \n",pOSD1Reg->osd_vvld_offset,pOSD1Reg->osd_vvld_offset);
	printk(KERN_INFO "pOSD1Reg G197.19 0x%08x(%d) \n",pOSD1Reg->osd_vvld_height,pOSD1Reg->osd_vvld_height);
	printk(KERN_INFO "pOSD1Reg G197.20 0x%08x(%d) \n",pOSD1Reg->osd_data_fetch_ctrl,pOSD1Reg->osd_data_fetch_ctrl);
	printk(KERN_INFO "pOSD1Reg G197.21 0x%08x(%d) \n",pOSD1Reg->osd_bist_ctrl,pOSD1Reg->osd_bist_ctrl);
}
