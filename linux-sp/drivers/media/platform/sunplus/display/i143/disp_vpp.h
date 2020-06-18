#ifndef __DISP_VPP_H__
#define __DISP_VPP_H__

#include "display.h"

typedef enum { //for VSCL path select
	DRV_FROM_VPPDMA = 0,
	DRV_FROM_OSD0,
	DRV_FROM_DDFCH,
	DRV_FROM_MAX
} DRV_VppWindow_e;

void DRV_VPP_Init(void *pInHWReg1, void *pInHWReg2, void *pInHWReg3, void *pInHWReg4);
void vpp_path_select(DRV_VppWindow_e vpp_path_sel);
void vpp_path_select_flag(DRV_VppWindow_e vpp_path_sel);

void DRV_DDFCH_off(void);
void DRV_DDFCH_on(void);
void DRV_VPPDMA_off(void);
void DRV_VPPDMA_on(void);

void DRV_DDFCH_Init10(void);
void DRV_DDFCH_Init11(void);
void DRV_DDFCH_Init2(void);
void DRV_DDFCH_Init3(void);
void DRV_DDFCH_Init4(void);
void DRV_DDFCH_Init5(void);
void DRV_DDFCH_Init6(void);

int ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int ddfch_fmt);
int vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h);
int vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt);

//int vpost_setting(int x, int y, int input_w, int input_h, int output_w, int output_h);
//int ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int yuv_fmt);

void DRV_VSCL_SetColorbar(DRV_VppWindow_e vpp_path_sel, int enable);
void DRV_VPP_SetColorbar(DRV_VppWindow_e vpp_layer, int enable);

void DRV_VPOST_dump(void);
void DRV_VSCL_dump(void);
void DRV_VPPDMA_dump(void);
void DRV_DDFCH_dump(void);

#endif	//__DISP_VPP_H__

