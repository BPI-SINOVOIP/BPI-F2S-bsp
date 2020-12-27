// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "disp_tgen.h"
#include "display2.h"
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void DRV_TGEN_Init(int is_hdmi, int width, int height)
{
	G213_TGEN_REG->tgen_config = 0x0007;		// latch mode on
	G213_TGEN_REG->tgen_source_sel = 0x0;
	G213_TGEN_REG->tgen_user_int2_config = 400;

	if(!is_hdmi) {
		if ( (width == 1024) && (height == 600) ) {
			G213_TGEN_REG->tgen_dtg_config = 1; //TGEN USER MODE
			G213_TGEN_REG->tgen_dtg_total_pixel = 1344; //Total pixel
			G213_TGEN_REG->tgen_dtg_ds_line_start_cd_point = 1024; //line start
			G213_TGEN_REG->tgen_dtg_total_line = 635; //Total line
			G213_TGEN_REG->tgen_dtg_field_end_line = 625; //field end line
			G213_TGEN_REG->tgen_dtg_start_line = 24; //start line
			//G213_TGEN_REG->tgen_reset = 1;
		}
		else if ( (width == 800) && (height == 480) ) {
			G213_TGEN_REG->tgen_dtg_config = 1; //TGEN USER MODE
			G213_TGEN_REG->tgen_dtg_total_pixel = 1056; //Total pixel
			G213_TGEN_REG->tgen_dtg_ds_line_start_cd_point = 800; //line start
			G213_TGEN_REG->tgen_dtg_total_line = 525; //Total line
			G213_TGEN_REG->tgen_dtg_field_end_line = 508; //field end line
			G213_TGEN_REG->tgen_dtg_start_line = 27; //start line
			//G213_TGEN_REG->tgen_reset = 1;
		}
		else if ( (width == 320) && (height == 240) ) {
			G213_TGEN_REG->tgen_dtg_config = 1; //TGEN USER MODE
			G213_TGEN_REG->tgen_dtg_total_pixel = 408; //Total pixel
			G213_TGEN_REG->tgen_dtg_ds_line_start_cd_point = 320; //line start
			G213_TGEN_REG->tgen_dtg_total_line = 262; //Total line
			G213_TGEN_REG->tgen_dtg_field_end_line = 260; //field end line
			G213_TGEN_REG->tgen_dtg_start_line = 19; //start line
			//G213_TGEN_REG->tgen_reset = 1;
		}
	}
	else {
		G213_TGEN_REG->tgen_dtg_config = 0; //TGEN HDMI MODE
	}
}

void DRV_TGEN_Set(DRV_VideoFormat_e fmt, DRV_FrameRate_e fps)
{
	if (fmt == DRV_FMT_USER_MODE) {
		;
	} else {
		G213_TGEN_REG->tgen_dtg_config = ((fmt & 0x7) << 8) | ((fps & 0x3) << 4);
	}
}

int DRV_TGEN_Adjust(DRV_TGEN_Input_e Input, UINT32 Adjust)
{
	switch (Input) {
	case DRV_TGEN_VPP0:
		G213_TGEN_REG->tgen_dtg_adjust1 = (G213_TGEN_REG->tgen_dtg_adjust1 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	case DRV_TGEN_OSD0:
		G213_TGEN_REG->tgen_dtg_adjust3 = (G213_TGEN_REG->tgen_dtg_adjust3 & ~(0x3F<<0)) | ((Adjust & 0x3F) << 0);
		break;
	case DRV_TGEN_PTG:
		G213_TGEN_REG->tgen_dtg_adjust4 = (G213_TGEN_REG->tgen_dtg_adjust4 & ~(0x3F<<8)) | ((Adjust & 0x3F) << 8);
		break;
	default:
		debug("Invalidate Input %d\n", Input);
		return DRV_ERR_INVALID_PARAM;
	}

	return DRV_SUCCESS;
}

