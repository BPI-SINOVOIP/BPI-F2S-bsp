// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#ifndef __DISP_VPP_H__
#define __DISP_VPP_H__

void DRV_VPP_Init(void);
//void vpost_setting(int is_hdmi, int x, int y, int input_w, int input_h, int output_w, int output_h);
void ddfch_setting(int luma_addr, int chroma_addr, int w, int h, int yuv_fmt);
//int vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt);
//int vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h);
//void vppdma_flip_setting(int flip_v, int flip_h);
void vscl_setting(int x, int y, int input_w, int input_h, int output_w, int output_h);
void vppdma_setting(int luma_addr, int chroma_addr, int w, int h, int vppdma_fmt);
#endif	//__DISP_VPP_H__

