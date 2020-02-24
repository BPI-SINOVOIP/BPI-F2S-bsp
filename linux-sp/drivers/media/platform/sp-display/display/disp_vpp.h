#ifndef __DISP_VPP_H__
#define __DISP_VPP_H__

#include "mach/display/display.h"

void DRV_VPP_Init(void *pInHWReg1, void *pInHWReg2);
int vpost_setting(int x, int y, int input_w, int input_h, int output_w, int output_h);
#ifdef TTL_MODE_SUPPORT
#ifdef TTL_MODE_DTS
void sp_disp_set_ttl_vpp(void);
#endif
#endif
int ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int yuv_fmt);

#endif	//__DISP_VPP_H__

