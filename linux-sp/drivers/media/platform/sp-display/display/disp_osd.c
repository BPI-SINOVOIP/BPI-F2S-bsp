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
 * @file disp_osd.c
 * @brief
 * @author PoChou Chen
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include "reg_disp.h"
#include "hal_disp.h"
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
#ifdef DEBUG_MSG
	#define mod_dbg(fmt, arg...)	pr_dbg("[%s:%d] "fmt, \
			__func__, \
			__LINE__, \
			##arg)
#else
	#define mod_dbg(fmt, arg...)
#endif

#define mod_err(fmt, arg...)		pr_err("[%s:%d] Error! "fmt, \
		__func__, \
		__LINE__, \
		##arg)
#define mod_warn(fmt, arg...)		pr_warn("[%s:%d] Warning! "fmt, \
		__func__, \
		__LINE__, \
		##arg)
#define mod_info(fmt, arg...)		pr_info("[%s:%d] "fmt, \
		__func__, \
		__LINE__, \
		##arg)

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

static struct Region_Manager_s *gpWinRegion;
static u32 gpWinRegion_phy;
static u8 *gpOsdHeader;
static u32 gpOsdHeader_phy;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_OSD_Init(void *pInHWReg1, void *pInHWReg2)
{
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;

	pOSDReg = (DISP_OSD_REG_t *)pInHWReg1;
	pGPOSTReg = (DISP_GPOST_REG_t *)pInHWReg2;

	init_waitqueue_head(&pDispWorkMem->osd_wait);
	spin_lock_init(&pDispWorkMem->osd_lock);
}

DRV_Status_e DRV_OSD_SetClut(DRV_OsdRegionHandle_t region, u32 *pClutDataPtr)
{
	struct Region_Manager_s *pRegionManager =
		(struct Region_Manager_s *)region;
	u32 copysize = 0;

	if (pRegionManager && pClutDataPtr) {
		switch (pRegionManager->Format) {
		case DRV_OSD_REGION_FORMAT_8BPP:
			copysize = 256 * 4;
			break;
		default:
			goto Return;
		}
		memcpy(pRegionManager->Hdr_ClutAddr, pClutDataPtr, copysize);

		return DRV_SUCCESS;
	}

Return:
	mod_err("Incorrect region handle, pClutDataPtr 0x%x\n",
			(u32)pClutDataPtr);

	return DRV_ERR_INVALID_HANDLE;
}

void DRV_OSD_IRQ(void)
{
	struct Region_Manager_s *pRegionManager = gpWinRegion;
	struct HW_OSD_Header_s *pHWOSDhdr;
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;

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
		if (pRegionManager->PaletteAddr)
			DRV_OSD_SetClut((DRV_OsdRegionHandle_t)pRegionManager,
					(u32 *)pRegionManager->PaletteAddr);
	}

	if (pDispWorkMem->osd_field_end_protect) {
		pDispWorkMem->osd_field_end_protect &= ~(1 << DRV_OSD0);
		wake_up_interruptible(&pDispWorkMem->osd_wait);
	}

	spin_unlock(&pDispWorkMem->osd_lock);
}

void DRV_OSD_Info(void)
{
	struct HW_OSD_Header_s *pOsdHdr = (struct HW_OSD_Header_s *)gpOsdHeader;

	pr_info("Region display-order is as follows:\n");

	pr_info("    Check osd output: %d %d, region ouput:%d %d\n",
		pOSDReg->osd_hvld_width,
		pOSDReg->osd_vvld_height,
		SWAP16(pOsdHdr->h_size),
		SWAP16(pOsdHdr->v_size));

	pr_info("header: (x, y)=(%d, %d) (w, h)=(%d, %d) data(x, y)=(%d, %d) data width=%d\n",
				SWAP16(pOsdHdr->disp_start_column),
				SWAP16(pOsdHdr->disp_start_row),
				SWAP16(pOsdHdr->h_size),
				SWAP16(pOsdHdr->v_size),
				SWAP16(pOsdHdr->data_start_column),
				SWAP16(pOsdHdr->data_start_row),
				SWAP16(pOsdHdr->data_width));
	pr_info("cu:%d ft:%d bit format:%d link data:0x%x\n\n",
			(pOsdHdr->config0 & 0x80) ? 1 : 0,
			(pOsdHdr->config0 & 0x40) ? 1 : 0,
			(pOsdHdr->config0 & 0xf),
			SWAP32(pOsdHdr->link_data));
}

void DRV_OSD_HDR_Show(void)
{
	int *ptr = (int *)gpOsdHeader;
	int i;

	for (i = 0; i < 8; ++i)
		pr_info("%d: 0x%08x\n", i, *(ptr+i));
}

void DRV_OSD_HDR_Write(int offset, int value)
{
	int *ptr = (int *)gpOsdHeader;

	*(ptr+offset) = value;
}

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
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;

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

	if (gpWinRegion->Format == DRV_OSD_REGION_FORMAT_8BPP)
		dma_free_coherent(NULL,
				sizeof(struct HW_OSD_Header_s) + 1024,
				gpOsdHeader,
				gpOsdHeader_phy);
	else
		dma_free_coherent(NULL,
				sizeof(struct HW_OSD_Header_s),
				gpOsdHeader,
				gpOsdHeader_phy);

	dma_free_coherent(NULL,
			sizeof(struct Region_Manager_s),
			gpWinRegion,
			gpWinRegion_phy);
}
EXPORT_SYMBOL(DRV_OSD_Set_UI_UnInit);

void DRV_OSD_Set_UI_Init(struct UI_FB_Info_t *pinfo)
{
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;
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

	if (pinfo->UI_ColorFmt == DRV_OSD_REGION_FORMAT_8BPP)
		gpOsdHeader = dma_zalloc_coherent(NULL,
				sizeof(struct HW_OSD_Header_s) + 1024,
				&gpOsdHeader_phy,
				GFP_KERNEL);
	else
		gpOsdHeader = dma_zalloc_coherent(NULL,
				sizeof(struct HW_OSD_Header_s),
				&gpOsdHeader_phy,
				GFP_KERNEL);

	if (!gpOsdHeader) {
		mod_err("malloc osd header fail\n");
		return;
	}

	gpWinRegion = dma_zalloc_coherent(NULL,
			sizeof(struct Region_Manager_s),
			&gpWinRegion_phy,
			GFP_KERNEL);

	if (!gpWinRegion) {
		mod_err("malloc region header fail\n");
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

	mod_dbg("osd_header=0x%x 0x%x addr=0x%x\n",
			(u32)gpOsdHeader,
			gpOsdHeader_phy,
			pinfo->UI_bufAddr);

	osd_header = (u32 *)gpOsdHeader;

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
	struct sp_disp_device *pDispWorkMem = &gDispWorkMem;

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

u32 DRV_OSD_SetVisibleBuffer(u32 bBufferId)
{
	struct Region_Manager_s *pRegionManager = gpWinRegion;

	if (!pRegionManager)
		return -1;

	pRegionManager->DirtyFlag |= REGION_ADDR_DIRTY;
	pRegionManager->CurrBufID = bBufferId;

	return 0;
}

