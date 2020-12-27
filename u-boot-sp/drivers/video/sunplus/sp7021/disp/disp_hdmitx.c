// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus - All Rights Reserved
 *
 * Author(s): Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include "reg_disp.h"
#include "disp_hdmitx.h"
#include "display2.h"
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void hdmi_clk_init(int mode)
{
    if (mode ==  0) { //720x480 59.94Hz
        DISP_MOON4_REG->plltv_ctl[0] = 0xFFFF8001; 	//G4.14
        DISP_MOON4_REG->plltv_ctl[1] = 0xFFFF0004; 	//G4.15
        DISP_MOON4_REG->plltv_ctl[2] = 0xFFFF010a; 	//G4.16
        DISP_MOON4_REG->otp_st = 0x00300000; 		//G4.31
    }
    else if (mode ==  1) { //720x576 50Hz
        ; //TBD
    }
    else if (mode ==  2) { //1280x720 59.94Hz
        DISP_MOON4_REG->plltv_ctl[0] = 0xFFFF0001; 	//G4.14
        DISP_MOON4_REG->plltv_ctl[1] = 0xFFFF0084; 	//G4.15
        DISP_MOON4_REG->plltv_ctl[2] = 0xFFFF010a; 	//G4.16
        DISP_MOON4_REG->otp_st = 0x00300000; 		//G4.31
    }
    else {
        ;//TBD , the rest setting
    }
}

void DRV_hdmitx_Init(int is_hdmi, int width, int height)
{
	if(is_hdmi) {
		hdmi_clk_init(0);
		hdmitx_set_timming(0);
	}
	else {
		;//TBD , turn off hdmitx if necessary
	}
}

void hdmitx_set_ptg(int enable)
{
	if (enable) {
        G381_HDMITX_REG->g381_reserved[20] = 0x00000017;
	}
	else {
        G381_HDMITX_REG->g381_reserved[20] = 0x00000016;
	}
}

void hdmitx_set_timming(int mode)
{
    if (mode == 0) { //HDMITX 720x480 59.94Hz as default setting
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;
        G380_HDMITX_REG->g380_reserved[20] = 0x0000a59b;
        G380_HDMITX_REG->g380_reserved[21] = 0x00007a2d;
        G380_HDMITX_REG->g380_reserved[22] = 0x00009238;
        G380_HDMITX_REG->g380_reserved[23] = 0x000016e2;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;

        G382_HDMITX_REG->g382_reserved[12] = 0x00000043;
        G382_HDMITX_REG->g382_reserved[16] = 0x0000e411;
        G382_HDMITX_REG->g382_reserved[17] = 0x000001b5;
        G382_HDMITX_REG->g382_reserved[18] = 0x0000007e;
        G382_HDMITX_REG->g382_reserved[26] = 0x00000085;
        G382_HDMITX_REG->g382_reserved[27] = 0x00001800;
        G382_HDMITX_REG->g382_reserved[28] = 0x00000000;

        G383_HDMITX_REG->g383_reserved[1] = 0x00000003;
        G383_HDMITX_REG->g383_reserved[2] = 0x00000004;
        G383_HDMITX_REG->g383_reserved[3] = 0x00000000;
        G383_HDMITX_REG->g383_reserved[7] = 0x00005018;
        G383_HDMITX_REG->g383_reserved[8] = 0x000000a0;
        G383_HDMITX_REG->g383_reserved[11] = 0x00000010;

        G385_HDMITX_REG->g385_reserved[0] = 0x00001a1a;
        G385_HDMITX_REG->g385_reserved[1] = 0x0000101a;
        G385_HDMITX_REG->g385_reserved[2] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[3] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[4] = 0x00000010;
        G385_HDMITX_REG->g385_reserved[5] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[6] = 0x00001c08;
        G385_HDMITX_REG->g385_reserved[7] = 0x00000058;
        G385_HDMITX_REG->g385_reserved[8] = 0x00000002;
        G385_HDMITX_REG->g385_reserved[9] = 0x00000024;
        G385_HDMITX_REG->g385_reserved[10] = 0x00000204;
        G385_HDMITX_REG->g385_reserved[11] = 0x0000007a;
        G385_HDMITX_REG->g385_reserved[12] = 0x0000034a;
        G385_HDMITX_REG->g385_reserved[13] = 0x00000170;
        G385_HDMITX_REG->g385_reserved[14] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[15] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[16] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[17] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[18] = 0x00000000;
 
    }
    else if (mode == 1) { //HDMITX 720x576 50Hz as default setting
        ; //TBD
    }
    else if (mode == 2) { //HDMITX 1280x720 59.94Hz as default setting
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;
        G380_HDMITX_REG->g380_reserved[20] = 0x00005936;
        G380_HDMITX_REG->g380_reserved[21] = 0x0000e1c0;
        G380_HDMITX_REG->g380_reserved[22] = 0x0000a52e;
        G380_HDMITX_REG->g380_reserved[23] = 0x0000c7a6;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;
 
        G382_HDMITX_REG->g382_reserved[12] = 0x00000040;
        G382_HDMITX_REG->g382_reserved[16] = 0x0000e411;
        G382_HDMITX_REG->g382_reserved[17] = 0x000001b5;
        G382_HDMITX_REG->g382_reserved[18] = 0x0000007e;
        G382_HDMITX_REG->g382_reserved[26] = 0x00000085;
        G382_HDMITX_REG->g382_reserved[27] = 0x00001800;
        G382_HDMITX_REG->g382_reserved[28] = 0x00000000;

        G383_HDMITX_REG->g383_reserved[1] = 0x00000003;
        G383_HDMITX_REG->g383_reserved[2] = 0x00000004;
        G383_HDMITX_REG->g383_reserved[3] = 0x00000000;
        G383_HDMITX_REG->g383_reserved[7] = 0x00005018;
        G383_HDMITX_REG->g383_reserved[8] = 0x000000a0;
        G383_HDMITX_REG->g383_reserved[11] = 0x00000010;

        G385_HDMITX_REG->g385_reserved[0] = 0x00001a1a;
        G385_HDMITX_REG->g385_reserved[1] = 0x0000101a;
        G385_HDMITX_REG->g385_reserved[2] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[3] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[4] = 0x00000010;
        G385_HDMITX_REG->g385_reserved[5] = 0x00001010;
        G385_HDMITX_REG->g385_reserved[6] = 0x00001cb6;
        G385_HDMITX_REG->g385_reserved[7] = 0x000000a8;
        G385_HDMITX_REG->g385_reserved[8] = 0x00000004;
        G385_HDMITX_REG->g385_reserved[9] = 0x00000024;
        G385_HDMITX_REG->g385_reserved[10] = 0x00000204;
        G385_HDMITX_REG->g385_reserved[11] = 0x0000007a;
        G385_HDMITX_REG->g385_reserved[12] = 0x0000034a;
        G385_HDMITX_REG->g385_reserved[13] = 0x00000170;
        G385_HDMITX_REG->g385_reserved[14] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[15] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[16] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[17] = 0x00000000;
        G385_HDMITX_REG->g385_reserved[18] = 0x00000000;
    }
    else {
		;//TBD , the rest setting
    }
}

