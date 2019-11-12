#ifndef __DISP_DMIX_H__
#define __DISP_DMIX_H__

#include <media/sp-disp/display.h>

#define DMIX_LUMA_OFFSET_MIN	(-50)
#define DMIX_LUMA_OFFSET_MAX	(50)
#define DMIX_LUMA_SLOPE_MIN		(0.60)
#define DMIX_LUMA_SLOPE_MAX		(1.40)

typedef enum {
	DRV_DMIX_VPP0 = 0,
	DRV_DMIX_VPP1,	//unsupported
	DRV_DMIX_VPP2,	//unsupported
	DRV_DMIX_OSD0,
	DRV_DMIX_OSD1,	//unsupported
	DRV_DMIX_OSD2,	//unsupported
	DRV_DMIX_OSD3,	//unsupported
	DRV_DMIX_PTG,
} DRV_DMIX_InputSel_e;

typedef enum {
	DRV_DMIX_AlphaBlend,
	DRV_DMIX_Transparent,
	DRV_DMIX_Opacity,
} DRV_DMIX_LayerMode_e;

typedef enum {
	DRV_DMIX_BG,
	DRV_DMIX_L1,
	DRV_DMIX_L2,	//unsupported
	DRV_DMIX_L3,	//unsupported
	DRV_DMIX_L4,	//unsupported
	DRV_DMIX_L5,	//unsupported
	DRV_DMIX_L6,
} DRV_DMIX_LayerId_e;

typedef enum {
	DRV_DMIX_TPG_BGC,
	DRV_DMIX_TPG_V_COLORBAR,
	DRV_DMIX_TPG_H_COLORBAR,
	DRV_DMIX_TPG_BORDER,
	DRV_DMIX_TPG_SNOW,
	DRV_DMIX_TPG_MAX,
} DRV_DMIX_TPG_e;

typedef struct DRV_DMIX_PlaneAlpha_s {
	DRV_DMIX_LayerId_e Layer;
	UINT32 EnPlaneAlpha;
	UINT32 EnFixAlpha;
	UINT32 AlphaValue;
} DRV_DMIX_PlaneAlpha_t;

typedef struct DRV_DMIX_Luma_Adj_s {
	UINT32 enable;
	UINT32 brightness;
	UINT32 contrast;
	//-------------
	UINT16 CP1_Dst;
	UINT16 CP1_Src;
	UINT16 CP2_Dst;
	UINT16 CP2_Src;
	UINT16 CP3_Dst;
	UINT16 CP3_Src;
	UINT16 Slope0;
	UINT16 Slope1;
	UINT16 Slope2;
	UINT16 Slope3;
} DRV_DMIX_Luma_Adj_t;

typedef struct DRV_DMIX_Chroma_Adj_s {
	UINT32 enable;
	UINT16 satcos;
	UINT16 satsin;
} DRV_DMIX_Chroma_Adj_t;

typedef struct DRV_DMIX_Layer_Set_s {
	DRV_DMIX_LayerId_e Layer;
	DRV_DMIX_LayerMode_e LayerMode;
	DRV_DMIX_InputSel_e FG_Sel;
} DRV_DMIX_Layer_Set_t;

void DRV_DMIX_Pixel_En_Sel(void);
void DRV_DMIX_Init(void *pInReg);
DRV_Status_e DRV_DMIX_PTG_ColorBar(DRV_DMIX_TPG_e tpg_sel,
		int bg_color_yuv,
		int border_len);
void DRV_DMIX_PTG_Color_Set(UINT32 color);
void DRV_DMIX_PTG_Color_Set_YCbCr(UINT8 enable, UINT8 Y, UINT8 Cb, UINT8 Cr);
DRV_Status_e DRV_DMIX_Layer_Init(DRV_DMIX_LayerId_e Layer,
		DRV_DMIX_LayerMode_e LayerMode,
		DRV_DMIX_InputSel_e FG_Sel);
DRV_Status_e DRV_DMIX_Layer_Set(DRV_DMIX_LayerMode_e LayerMode,
		DRV_DMIX_InputSel_e FG_Sel);
void DRV_DMIX_Layer_Get(DRV_DMIX_Layer_Set_t *pLayerInfo);
DRV_Status_e DRV_DMIX_Plane_Alpha_Set(DRV_DMIX_PlaneAlpha_t *PlaneAlphaInfo);
void DRV_DMIX_PQ_OnOff(int OutId, int enable);
void DRV_DMIX_Luma_Adjust_Set(DRV_DMIX_Luma_Adj_t *LumaAdjInfo);
void DRV_DMIX_Luma_Adjust_Get(DRV_DMIX_Luma_Adj_t *LumaAdjInfo);
void DRV_DMIX_Chroma_Adjust_Set(DRV_DMIX_Chroma_Adj_t *ChromaAdjInfo);
void DRV_DMIX_Chroma_Adjust_Get(DRV_DMIX_Chroma_Adj_t *ChromaAdjInfo);

#endif	/* __DISP_DMIX_H__ */

