#include <common.h>
#include <asm/io.h>

#include "reg_disp.h"
#include "sp_videoin.h"
#include "isp_api.h"
#include "i2c_api.h"
#include "sensor_power.h"
#include "isp_api_s.h"

#define RAW10_OUTPUT_FORMAT_TEST    1
#define MOVING_RAW10_COLOR_BAR_TEST 0

#if (RAW10_OUTPUT_FORMAT_TEST == 1)
unsigned char RAW10_GOLDEN_PATTERN[] = {
	#include "Raw10Golden_2lines.txt"
};
#endif

#if (MOVING_RAW10_COLOR_BAR_TEST == 1)
unsigned char MOVING_1ST_FRAME_GOLDEN_PATTERN[] = {
	#include "moving_raw10_1stFrame_yuyv.txt"
};

unsigned char MOVING_2ND_FRAME_GOLDEN_PATTERN[] = {
	#include "moving_raw10_2ndFrame_yuyv.txt"
};

unsigned char MOVING_3RD_FRAME_GOLDEN_PATTERN[] = {
	#include "moving_raw10_3rdFrame_yuyv.txt"
};
#endif

void convRAW10Pack(u8 *in_raw10, int rows, int cols, u8 *out_raw8)
{
    int i;
    unsigned char *p_in_raw10=in_raw10;
    unsigned char *p_out_raw8=out_raw8;

    for (i = 0; i < (rows * cols); i += 4)
    {
            // Remove low two bits
            p_out_raw8[0] = (p_in_raw10[0]>>2) + ((p_in_raw10[1]&0X3)<<6);
            p_out_raw8[1] = (p_in_raw10[1]>>4) + ((p_in_raw10[2]&0XF)<<4);
            p_out_raw8[2] = (p_in_raw10[2]>>6) + ((p_in_raw10[3]&0X3F)<<2);
            p_out_raw8[3] = p_in_raw10[4];

			// RAW10 format pack mode
			// 4 pixels, 5 bytes, 40 bits
            p_out_raw8 += 4;
            p_in_raw10 += 5;
    }
}

static int reg_op(u64 addr, u32 *value, char *op)
{
	u32 *p = (u32*) addr;

	if (strcmp(op, "r") == 0) {
		// Read register
		*value = readl(p);
	}
	else if (strcmp(op, "w") == 0){
		// Write register
		writel(*value, p);
		*value = readl(p);
	}
	else {
		VIDEOIN_LOGE("Error in %s\n", __FUNCTION__);
		return CMD_RET_USAGE;
	}

	return 0;
}

static int ispreg_op(u64 addr, u8 *value, char *op)
{
	u8 *p = (u8*) addr;

	if (strcmp(op, "r") == 0) {
		// Read register
		*value = readb(p);
	}
	else if (strcmp(op, "w") == 0){
		// Write register
		writeb(*value, p);
		*value = readb(p);
	}
	else {
		VIDEOIN_LOGE("Error in %s\n", __FUNCTION__);
		return CMD_RET_USAGE;
	}

	return 0;
}

static int dump_reg(u64 addr, u32 length, char *type)
{
	int i, ret = 0;
	u32 *p_32 = (u32*)addr;
	u32 value_32 = 0;
	u8 *p_8 = (u8*)addr;
	u8 value_8 = 0;

	if (strcmp(type, "b") == 0) {
		// Read 8-bit register
		for (i = 0; i < length; i++, p_8++) {
			ret = ispreg_op((u64)p_8, &value_8, "r");

			if (ret == 0)
				VIDEOIN_LOGI("Addr 0x%08llX = 0x%02X (0x%02x)\n", (u64)p_8, value_8, value_8);
			else
				break;
		}

	} else if (strcmp(type, "w") == 0) {
		// Read 32-bit register
		for (i = 0; i < length; i++, p_32++) {
			ret = reg_op((u64)p_32, &value_32, "r");

			if (ret == 0)
				VIDEOIN_LOGI("Addr 0x%08llX = 0x%08X\n", (u64)p_32, value_32);
			else
				break;
		}
	}
	else {
		VIDEOIN_LOGE("Error in %s\n", __FUNCTION__);
		ret = CMD_RET_USAGE;
	}

	return ret;
}

static int isp_VSync(u8 isp)
{
	u8 reg276b_val = 0;
	//u8 reg276b_val_pre = 0;
	u8 reg27b0_val = 0;
	u8 status = 0;
	u8 condition = 0;


	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	while (condition == 0)
	{
		switch (isp)
		{
			case ISP0_PATH:
				reg276b_val = ISPAPB0_REG8(0x276b);
				reg27b0_val = ISPAPB0_REG8(0x27b0);
				break;

			case ISP1_PATH:
				reg276b_val = ISPAPB1_REG8(0x276b);
				reg27b0_val = ISPAPB1_REG8(0x27b0);
				break;
		}

		/*
		if (reg276b_val != reg276b_val_pre) {
			VIDEOIN_LOGI("state: %u, reg276b_val: 0x%02X, reg27b0_val: 0x%02X\n", status, reg276b_val, reg27b0_val);
		}
		*/

		if (reg27b0_val & 0x02) {
			VIDEOIN_LOGI("V-sync falling interrupt occurs, reg276b_val: 0x%02X, reg27b0_val: 0x%02X\n", reg276b_val, reg27b0_val);

			switch (isp)
			{
				case ISP0_PATH:
					ISPAPB0_REG8(0x27b0) = 0x02; // Write bit1 to 1 to clear V-sync falling interrupt
					break;
			
				case ISP1_PATH:
					ISPAPB1_REG8(0x27b0) = 0x02; // Write bit1 to 1 to clear V-sync falling interrupt
					break;
			}				
		}

		switch (status)
		{
			case 0:
				// Wait the V-valid signal from 0 to 1
				if (reg276b_val & 0x08) {
					VIDEOIN_LOGI("Capture VValid = 1. (reg 0x276b: 0x%02X)\n", reg276b_val);
					VIDEOIN_LOGI("ISP starts outputting image data.\n");
					status = 1;
				}
				break;

			case 1:
				// Wait the V-valid signal from 1 to 0
				if (!(reg276b_val & 0x08)) {
					VIDEOIN_LOGI("Capture VValid = 0. (reg 0x276b: 0x%02X)\n", reg276b_val);
					VIDEOIN_LOGI("ISP ends outputting image data.\n");
					status = 2;
				}
				break;

			case 2:
				// Wait the V-sync signal from 1 to 0
				if (!(reg276b_val & 0x02)) {
					VIDEOIN_LOGI("Capture VSync = 0. (reg 0x276b: 0x%02X)\n", reg276b_val);
					VIDEOIN_LOGI("Image data is ready in DRAM.\n");
					status = 3;
				}
				break;

			case 3:
				// The while loop ends.
				condition = 1;
				break;
		}

		//reg276b_val_pre = reg276b_val;
	}

	VIDEOIN_LOGI("%s end\n", __FUNCTION__);

	return 0;
}

static int ispchk_sram(u8 isp)
{
	int i;
	u8  data;

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (isp)
	{
		case ISP0_PATH:
			VIDEOIN_LOGI("Check SRAM for ISP0 path\n");
			break;

		case ISP1_PATH:
			VIDEOIN_LOGI("Check SRAM for ISP1 path\n");
			break;

		case ISP_ALL_PATH:
			VIDEOIN_LOGI("Can't check for ISP all path in same time\n");
			return 1;
			break;
	}

	VIDEOIN_LOGI("CDSP SRAM read check\n");
	switch (isp)
	{
		case ISP0_PATH:
			ISPAPB0_REG8(0x2008) = 0x00; // b1xck, b2xck and pclk_nx to cdsp_mclk
			ISPAPB0_REG8(0x2101) = 0x00; // select lens compensation R SRAM
			ISPAPB0_REG8(0x2100) = 0x03; // Enable CPU access macro and adress auto increase
			ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
			ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0

			for (i = 0; i < 10; i = i + 1) {
				data = ISPAPB0_REG8(0x2103);
				VIDEOIN_LOGI("i: %d, data: 0x%02x\n", i, data);
			}
			break;

		case ISP1_PATH:
			ISPAPB1_REG8(0x2008) = 0x00; // b1xck, b2xck and pclk_nx to cdsp_mclk
			ISPAPB1_REG8(0x2101) = 0x00; // select lens compensation R SRAM
			ISPAPB1_REG8(0x2100) = 0x03; // Enable CPU access macro and adress auto increase
			ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
			ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0

			for (i = 0; i < 10; i = i + 1) {
				data = ISPAPB1_REG8(0x2103);
				VIDEOIN_LOGI("i: %d, data: 0x%02x\n", i, data);
			}
			break;
	}

	VIDEOIN_LOGI("%s end\n", __FUNCTION__);

	return 0;
}

static int ispchk_interrupt(u8 isp, u8 count)
{
	u8 i = 0;

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (isp)
	{
		case ISP0_PATH:
			VIDEOIN_LOGI("Check interrupt for ISP0 path\n");
			break;

		case ISP1_PATH:
			VIDEOIN_LOGI("Check interrupt for ISP1 path\n");
			break;

		case ISP_ALL_PATH:
			VIDEOIN_LOGI("Can't check for ISP all path in same time\n");
			return 1;
			break;
	}

	while (i < count)
	{
		// Check VValid(0x276b[3]) = 0 and VSync(0x276b[1]) = 0  to wait frame end
		VIDEOIN_LOGI("Wait frame end...\n");

		isp_VSync(isp);

		VIDEOIN_LOGI("Image no.%u is ready\n", i);
		i++;
	}

	VIDEOIN_LOGI("%s end\n", __FUNCTION__);

	return 0;
}

static int ispchk_window(u8 isp)
{
	u16 data_u16 = 0;
	u8  data_u8 = 0;
	u8  i, j;

	// window value check register tables
	u16 winval_reg_3[] = {
		#include "winval_reg_3.txt"
	};

	u16 winval_reg_4[] = {
		#include "winval_reg_4.txt"
	};

	u16 winval_reg_6[] = {
		#include "winval_reg_6.txt"
	};

	u16 winval_reg_7[] = {
		#include "winval_reg_7.txt"
	};

	u16 winval_reg_8[] = {
		#include "winval_reg_8.txt"
	};

	u16 winval_reg_9[] = {
		#include "winval_reg_9.txt"
	};

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (isp)
	{
		case ISP0_PATH:
			VIDEOIN_LOGI("Check window value for ISP0 path\n");
			break;

		case ISP1_PATH:
			VIDEOIN_LOGI("Check window value for ISP1 path\n");
			break;

		case ISP_ALL_PATH:
			VIDEOIN_LOGI("Can't check for ISP all path in same time\n");
			return 1;
			break;
	}

	// Check VValid(0x276b[3]) = 0 and VSync(0x276b[1]) = 0  to wait frame end
	VIDEOIN_LOGI("Wait fame end...\n");

	isp_VSync(isp);

	VIDEOIN_LOGI("CDSP window value check\n");

	switch (isp)
	{
		case ISP0_PATH:
			// Item 1. Check windonw value
			VIDEOIN_LOGI("Item 1\n");
			ISPAPB0_REG8(0x2212) = 0x0f;
			ISPAPB0_REG8(0x224b) = 0x07;//0x03;
			ISPAPB0_REG8(0x229f) = 0x01;
			ISPAPB0_REG8(0x346b) = 0x01;
			ISPAPB0_REG8(0x34d0) = 0x03;
			ISPAPB0_REG8(0x3203) = 0x00;
			ISPAPB0_REG8(0x325d) = 0x00; // address reset

			for (i = 0; i < 9; i++) {
				for (j = 0; j < 9; j++) {
					data_u8 = ISPAPB0_REG8(0x325e);
					VIDEOIN_LOGI("Wnd%d%d = %u\n", i, j, data_u8);
				}
			}

			data_u8 = ISPAPB0_REG8(0x325e);
			VIDEOIN_LOGI("Wnd Avg = %u\n", data_u8);

			// Item 2. Check 0x224a register value
			VIDEOIN_LOGI("Item 2\n");
			data_u8 = ISPAPB0_REG8(0x224a);
			VIDEOIN_LOGI("reg 0x224a = %u\n", data_u8);

			// Item 3. Check many registers' value
			VIDEOIN_LOGI("Item 3\n");
			for (i = 0; i < 75; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_3[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_3[i], data_u8);
			}

			// Item 4. Check many registers' value
			VIDEOIN_LOGI("Item 4\n");
			for (i = 0; i < 18; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_4[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_4[i], data_u8);
			}

			// Item 5. Check value
			VIDEOIN_LOGI("Item 5\n");
			ISPAPB0_REG8(0x3203) = 0x00; // adress reset
			for (i = 0; i < 4; i++) {
				for (j = 0; j < 16; j++) {
					data_u16 = ISPAPB0_REG16(0x3204);
					VIDEOIN_LOGI("i:%02u j:%02u = 0x%04x\n", i, j, data_u16);
				}
			}

			// Item 6. Check many registers' value
			VIDEOIN_LOGI("Item 6\n");
			ISPAPB0_REG8(0x2298) = 0x00;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_6[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_6[i], data_u8);
			}

			// Item 7. Check many registers' value
			VIDEOIN_LOGI("Item 7\n");
			ISPAPB0_REG8(0x2298) = 0x01;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_7[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_7[i], data_u8);
			}

			// Item 8. Check many registers' value
			VIDEOIN_LOGI("Item 8\n");
			ISPAPB0_REG8(0x2298) = 0x02;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_8[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_8[i], data_u8);
			}

			// Item 9. Check many registers' value
			VIDEOIN_LOGI("Item 9\n");
			ISPAPB0_REG8(0x2298) = 0x03;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB0_REG8((u64)winval_reg_9[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_9[i], data_u8);
			}
			break;

		case ISP1_PATH:
			// Item 1. Check windonw value
			VIDEOIN_LOGI("Item 1\n");
			ISPAPB1_REG8(0x2212) = 0x0f;
			ISPAPB1_REG8(0x224b) = 0x07;//0x03;
			ISPAPB1_REG8(0x229f) = 0x01;
			ISPAPB1_REG8(0x346b) = 0x01;
			ISPAPB1_REG8(0x34d0) = 0x03;
			ISPAPB1_REG8(0x3203) = 0x00;
			ISPAPB1_REG8(0x325d) = 0x00; // address reset

			for (i = 0; i < 9; i++) {
				for (j = 0; j < 9; j++) {
					data_u8 = ISPAPB1_REG8(0x325e);
					VIDEOIN_LOGI("Wnd%d%d = %u\n", i, j, data_u8);
				}
			}

			data_u8 = ISPAPB1_REG8(0x325e);
			VIDEOIN_LOGI("Wnd Avg = %u\n", data_u8);

			// Item 2. Check 0x224a register value
			VIDEOIN_LOGI("Item 2\n");
			data_u8 = ISPAPB1_REG8(0x224a);
			VIDEOIN_LOGI("reg 0x224a = %u\n", data_u8);

			// Item 3. Check many registers' value
			VIDEOIN_LOGI("Item 3\n");
			for (i = 0; i < 75; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_3[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_3[i], data_u8);
			}

			// Item 4. Check many registers' value
			VIDEOIN_LOGI("Item 4\n");
			for (i = 0; i < 18; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_4[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_4[i], data_u8);
			}

			// Item 5. Check value
			VIDEOIN_LOGI("Item 5\n");
			ISPAPB1_REG8(0x3203) = 0x00; // adress reset
			for (i = 0; i < 4; i++) {
				for (j = 0; j < 16; j++) {
					data_u16 = ISPAPB1_REG16(0x3204);
					VIDEOIN_LOGI("i:%02u j:%02u = 0x%04x\n", i, j, data_u16);
				}
			}

			// Item 6. Check many registers' value
			VIDEOIN_LOGI("Item 6\n");
			ISPAPB1_REG8(0x2298) = 0x00;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_6[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_6[i], data_u8);
			}

			// Item 7. Check many registers' value
			VIDEOIN_LOGI("Item 7\n");
			ISPAPB1_REG8(0x2298) = 0x01;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_7[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_7[i], data_u8);
			}

			// Item 8. Check many registers' value
			VIDEOIN_LOGI("Item 8\n");
			ISPAPB1_REG8(0x2298) = 0x02;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_8[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_8[i], data_u8);
			}

			// Item 9. Check many registers' value
			VIDEOIN_LOGI("Item 9\n");
			ISPAPB1_REG8(0x2298) = 0x03;
			for (i = 0; i < 8; i++) {
				data_u8 = ISPAPB1_REG8((u64)winval_reg_9[i]);
				VIDEOIN_LOGI("reg 0x%04x = 0x%02x\n", winval_reg_9[i], data_u8);
			}
			break;
	}

	VIDEOIN_LOGI("%s end\n", __FUNCTION__);

	return 0;
}

static void hdmi_clk_init(int mode)
{
    if (mode == 0) { //HDMITX 720x480 59.94Hz as default setting
        MOON4_REG->plltv_ctl[0] = 0xFFFF0000; 	//G4.15
        MOON4_REG->plltv_ctl[1] = 0xFFFF0014; 	//G4.16
        MOON4_REG->plltv_ctl[2] = 0xFFFF2040; 	//G4.17
		MOON4_REG->plltv_ctl[3] = 0xFFFF0040; 	//G4.18
		MOON5_REG->cfg[6]		= 0xFFFF0020; 	//G5.6
    }
    else if (mode == 1) { //HDMITX 720x576 50Hz as default setting
        MOON4_REG->plltv_ctl[0] = 0xFFFF0000; 	//G4.15
        MOON4_REG->plltv_ctl[1] = 0xFFFF0014; 	//G4.16
        MOON4_REG->plltv_ctl[2] = 0xFFFF2040; 	//G4.17
		MOON4_REG->plltv_ctl[3] = 0xFFFF0040; 	//G4.18
		MOON5_REG->cfg[6]		= 0xFFFF0020; 	//G5.6
    }
    else if (mode == 2) { //HDMITX 1280x720 59.94Hz as default setting
        MOON4_REG->plltv_ctl[0] = 0xFFFF57ff; 	//G4.15
        MOON4_REG->plltv_ctl[1] = 0xFFFF2050; 	//G4.16
        MOON4_REG->plltv_ctl[2] = 0xFFFF0145; 	//G4.17
		MOON4_REG->plltv_ctl[3] = 0xFFFF0004; 	//G4.18
		MOON5_REG->cfg[6]		= 0xFFFF0020; 	//G5.6
	}
	else if (mode == 3) { //HDMITX 1920x1080 59.94Hz as default setting
        MOON4_REG->plltv_ctl[0] = 0xFFFF47ff; 	//G4.15
        MOON4_REG->plltv_ctl[1] = 0xFFFF2050; 	//G4.16
        MOON4_REG->plltv_ctl[2] = 0xFFFF0145; 	//G4.17
		MOON4_REG->plltv_ctl[3] = 0xFFFF0004; 	//G4.18
		MOON5_REG->cfg[6]		= 0xFFFF0020; 	//G5.6
	}

}

static void hdmitx_set_timming(int mode)
{
    if (mode == 0) { //HDMITX 720x480 59.94Hz as default setting
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;
		G381_HDMITX_REG->g381_reserved[20] = 0x00000716;

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
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;
		G381_HDMITX_REG->g381_reserved[20] = 0x00000716;

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
        G385_HDMITX_REG->g385_reserved[6] = 0x00001cf9;
        G385_HDMITX_REG->g385_reserved[7] = 0x00000058;
        G385_HDMITX_REG->g385_reserved[8] = 0x00000011;
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
    else if (mode == 2) { //HDMITX 1280x720 59.94Hz as default setting
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;
		G381_HDMITX_REG->g381_reserved[20] = 0x00000110;

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
	else if (mode == 3) { //HDMITX 1920x1080 59.94Hz as default setting
        G380_HDMITX_REG->g380_reserved[5] = 0x0000141f;
        G380_HDMITX_REG->g380_reserved[6] = 0x000000ff;
        G380_HDMITX_REG->g380_reserved[8] = 0x00001001;

        G381_HDMITX_REG->g381_reserved[16] = 0x00000140;
		G381_HDMITX_REG->g381_reserved[20] = 0x00000210;

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
        G385_HDMITX_REG->g385_reserved[6] = 0x00001caa;
        G385_HDMITX_REG->g385_reserved[7] = 0x000000a8;
        G385_HDMITX_REG->g385_reserved[8] = 0x00000010;
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

}

static void disp_setting_480P(u32 buf_addr, u8 YUV_order)
{
	u32 vdma_cfg_value = 0;

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (YUV_order)
	{
		case UYVY_ORDER:
			VIDEOIN_LOGI("Display YUV422 UYVY Format\n");
			vdma_cfg_value = 0x00000002;    // G186.02 , (y normal uv normal) , bist off , fmt = YUY2
			break;

		case YUYV_ORDER:
			VIDEOIN_LOGI("Display YUV422 YUYV Format\n");
			vdma_cfg_value = 0x00001002;    // G186.02 , (y swap uv normal) , bist on color bar , fmt = YUY2
			break;
	}

	// 480P 720x480 setting
	DISP_DVE_REG->color_bar_mode  = 0x0000;     //G235.0 (0x0C00 --> 0x0000)
	DISP_DVE_REG->dve_hdmi_mode_1 = 0x3;        //G234.6 (2 or 3) SD
	DISP_DVE_REG->dve_hdmi_mode_0 = 0x141;      // G234.16 (0x0141(480P))latch mode on
	
	DISP_DMIX_REG->dmix_layer_config_0 = 0x34561070;    // 00
	DISP_DMIX_REG->dmix_layer_config_1 = 0x00000000;    // 01 all layer blending
	DISP_DMIX_REG->dmix_ptg_config_0   = 0x00002001;    // 09
	DISP_DMIX_REG->dmix_ptg_config_2   = 0x0029f06e;    // 11
	DISP_DMIX_REG->dmix_source_select  = 0x00000006;    // 20
		
	DISP_TGEN_REG->tgen_config      = 0x00000000;   //G213.00 // latch mode on
	DISP_TGEN_REG->tgen_dtg_config  = 0x00000000;   //G213.04 0x0000 (480P)
	DISP_TGEN_REG->tgen_dtg_adjust1 = 0x0000100d;   //G213.23
	DISP_TGEN_REG->tgen_reset       = 0x00000001;   //G213.01
	//DISP_TGEN_REG->tgen_user_int1_config = 0x00000014;	//G213.02
	//DISP_TGEN_REG->tgen_user_int2_config = 0x00000020;	//G213.03
	
	// From VPPDMA
	DISP_VSCL_REG->vscl0_config2 = 0x0000021F;	    //G187.01
	
	DISP_VSCL_REG->vscl0_actrl_i_xlen   = 0x000002D0;   //G187.03
	DISP_VSCL_REG->vscl0_actrl_i_ylen   = 0x000001E0;   //G187.04
	DISP_VSCL_REG->vscl0_actrl_s_xstart = 0x00000000;   //G187.05
	DISP_VSCL_REG->vscl0_actrl_s_ystart = 0x00000000;   //G187.06
	DISP_VSCL_REG->vscl0_actrl_s_xlen   = 0x000002D0;   //G187.07
	DISP_VSCL_REG->vscl0_actrl_s_ylen   = 0x000001E0;   //G187.08
	DISP_VSCL_REG->vscl0_dctrl_o_xlen   = 0x000002D0;   //G187.09
	DISP_VSCL_REG->vscl0_dctrl_o_ylen   = 0x000001E0;   //G187.10	
	DISP_VSCL_REG->vscl0_dctrl_d_xstart = 0x00000000;   //G187.11
	DISP_VSCL_REG->vscl0_dctrl_d_ystart = 0x00000000;   //G187.12
	DISP_VSCL_REG->vscl0_dctrl_d_xlen   = 0x000002D0;   //G187.13
	DISP_VSCL_REG->vscl0_dctrl_d_ylen   = 0x000001E0;   //G187.14
	
	DISP_VSCL_REG->vscl0_hint_ctrl         = 0x00000002;    //G187.18
	DISP_VSCL_REG->vscl0_hint_hfactor_low  = 0x00000000;    //G187.19
	DISP_VSCL_REG->vscl0_hint_hfactor_high = 0x00000040;    //G187.20
	DISP_VSCL_REG->vscl0_hint_initf_low    = 0x00000000;    //G187.21
	DISP_VSCL_REG->vscl0_hint_initf_high   = 0x00000000;    //G187.22	
	
	DISP_VSCL_REG->vscl0_vint_ctrl         = 0x00000002;    //G188.00
	DISP_VSCL_REG->vscl0_vint_vfactor_low  = 0x00000000;    //G188.01
	DISP_VSCL_REG->vscl0_vint_vfactor_high = 0x00000040;    //G188.02
	DISP_VSCL_REG->vscl0_vint_initf_low	   = 0x00000000;    //G188.03
	DISP_VSCL_REG->vscl0_vint_initf_high   = 0x00000000;    //G188.04
	
	DISP_VPPDMA_REG->vdma_gc         = 0x80000028;      //G186.01 , vppdma en , urgent th = 0x28
	DISP_VPPDMA_REG->vdma_cfg        = vdma_cfg_value;  // G186.02
	DISP_VPPDMA_REG->vdma_frame_size = 0x01E002D0;      //G186.03 , frame_h , frame_w
	DISP_VPPDMA_REG->vdma_crop_st    = 0x00000000;      //G186.04 , start_h , Start_w
	DISP_VPPDMA_REG->vdma_lstd_size	 = (0x000002D0<<1); //G186.05 , stride size
	DISP_VPPDMA_REG->vdma_data_addr1 = buf_addr;        //G186.06 , 1sr planner addr (luma)
	DISP_VPPDMA_REG->vdma_data_addr2 = 0x01dc0000;      //G186.07 , 2nd planner addr (crma)	
	
	VIDEOIN_LOGI("%s end\n", __FUNCTION__);
}

static void disp_setting_720P(u32 buf_addr, u8 YUV_order)
{
	u32 vdma_cfg_value = 0;

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (YUV_order)
	{
		case UYVY_ORDER:
			VIDEOIN_LOGI("Display YUV422 UYVY Format\n");
			vdma_cfg_value = 0x00000002;    // G186.02 , (y normal uv normal) , bist off , fmt = YUY2
			break;

		case YUYV_ORDER:
			VIDEOIN_LOGI("Display YUV422 YUYV Format\n");
			vdma_cfg_value = 0x00001002;    // G186.02 , (y swap uv normal) , bist on color bar , fmt = YUY2
			break;
	}

	// 720P 1280x720 settings
	DISP_DVE_REG->color_bar_mode  = 0x0C10;     //G235.0 (0x0C10)
	DISP_DVE_REG->dve_hdmi_mode_1 = 0x2;        //G234.6 (3 --> 2) HD
	DISP_DVE_REG->dve_hdmi_mode_0 = 0xD41;      // G234.16 (0x0D41(720P))latch mode on	
	
	DISP_DMIX_REG->dmix_layer_config_0 = 0x34561070;    // 00
	DISP_DMIX_REG->dmix_layer_config_1 = 0x00000000;    // 01 all layer blending
	DISP_DMIX_REG->dmix_ptg_config_0   = 0x00002001;    // 09
	DISP_DMIX_REG->dmix_ptg_config_2   = 0x0029f06e;    // 11
	DISP_DMIX_REG->dmix_source_select  = 0x00000006;    // 20	
		
	DISP_TGEN_REG->tgen_config           = 0x00000000;  //G213.00 // latch mode on
	DISP_TGEN_REG->tgen_dtg_config       = 0x00000200; 	//G213.04 0x0200 (720P)
	DISP_TGEN_REG->tgen_dtg_adjust1      = 0x0000100d;  //G213.23
	DISP_TGEN_REG->tgen_reset            = 0x00000001;  //G213.01
	DISP_TGEN_REG->tgen_user_int1_config = 0x00000014;  //G213.02
	DISP_TGEN_REG->tgen_user_int2_config = 0x00000020;  //G213.03		
	
	//From VPPDMA
	DISP_VSCL_REG->vscl0_config2        = 0x0000021F;   //G187.01
	DISP_VSCL_REG->vscl0_actrl_i_xlen   = 0x00000500;   //G187.03 (1280)
	DISP_VSCL_REG->vscl0_actrl_i_ylen   = 0x000002D0;   //G187.04 (720)
	DISP_VSCL_REG->vscl0_actrl_s_xstart = 0x00000000;   //G187.05
	DISP_VSCL_REG->vscl0_actrl_s_ystart = 0x00000000;   //G187.06
	DISP_VSCL_REG->vscl0_actrl_s_xlen   = 0x00000500;   //G187.07 (1280)
	DISP_VSCL_REG->vscl0_actrl_s_ylen   = 0x000002D0;   //G187.08 (720)
	DISP_VSCL_REG->vscl0_dctrl_o_xlen   = 0x00000500;   //G187.09 (1280)
	DISP_VSCL_REG->vscl0_dctrl_o_ylen   = 0x000002D0;   //G187.10	(720)
	DISP_VSCL_REG->vscl0_dctrl_d_xstart = 0x00000000;   //G187.11
	DISP_VSCL_REG->vscl0_dctrl_d_ystart = 0x00000000;   //G187.12
	DISP_VSCL_REG->vscl0_dctrl_d_xlen   = 0x00000500;   //G187.13 (1280)
	DISP_VSCL_REG->vscl0_dctrl_d_ylen   = 0x000002D0;   //G187.14 (720)
	
	DISP_VSCL_REG->vscl0_hint_ctrl         = 0x00000002;    //G187.18
	DISP_VSCL_REG->vscl0_hint_hfactor_low  = 0x00000000;    //G187.19
	DISP_VSCL_REG->vscl0_hint_hfactor_high = 0x00000040;    //G187.20
	DISP_VSCL_REG->vscl0_hint_initf_low	   = 0x00000000;    //G187.21
	DISP_VSCL_REG->vscl0_hint_initf_high   = 0x00000000;    //G187.22	
	
	DISP_VSCL_REG->vscl0_vint_ctrl         = 0x00000002;    //G188.00
	DISP_VSCL_REG->vscl0_vint_vfactor_low  = 0x00000000;    //G188.01
	DISP_VSCL_REG->vscl0_vint_vfactor_high = 0x00000040;    //G188.02
	DISP_VSCL_REG->vscl0_vint_initf_low    = 0x00000000;    //G188.03
	DISP_VSCL_REG->vscl0_vint_initf_high   = 0x00000000;    //G188.04
	
	DISP_VPPDMA_REG->vdma_gc         = 0x80000028;      //G186.01 , vppdma en , urgent th = 0x28
	DISP_VPPDMA_REG->vdma_cfg        = vdma_cfg_value;  // G186.02
	DISP_VPPDMA_REG->vdma_frame_size = 0x02D00500;      //G186.03 , frame_h , frame_w (720 1280)
	DISP_VPPDMA_REG->vdma_crop_st    = 0x00000000;      //G186.04 , start_h , Start_w
	DISP_VPPDMA_REG->vdma_lstd_size  = (0x00000500<<1); //G186.05 , stride size (1280)
	DISP_VPPDMA_REG->vdma_data_addr1 = buf_addr;        //G186.06 , 1sr planner addr (luma)
	DISP_VPPDMA_REG->vdma_data_addr2 = 0x01dc0000;      //G186.07 , 2nd planner addr (crma)
	
	VIDEOIN_LOGI("%s end\n", __FUNCTION__);
}

static void disp_setting_1080P(u32 buf_addr, u8 YUV_order)
{
	u32 vdma_cfg_value = 0;

	VIDEOIN_LOGI("%s start\n", __FUNCTION__);

	switch (YUV_order)
	{
		case UYVY_ORDER:
			VIDEOIN_LOGI("Display YUV422 UYVY Format\n");
			vdma_cfg_value = 0x00000002;    // G186.02 , (y normal uv normal) , bist off , fmt = YUY2
			break;

		case YUYV_ORDER:
			VIDEOIN_LOGI("Display YUV422 YUYV Format\n");
			vdma_cfg_value = 0x00001002;    // G186.02 , (y swap uv normal) , bist on color bar , fmt = YUY2
			break;
	}

	// 1080P 1920x1080 settings
	DISP_DVE_REG->color_bar_mode  = 0x0C20;   //G235.0 (0x0C20)
	DISP_DVE_REG->dve_hdmi_mode_1 = 0x2;      //G234.6 (3 --> 2) HD
	DISP_DVE_REG->dve_hdmi_mode_0 = 0x541;    // G234.16 (0x0541(1080P))latch mode on	
	
	DISP_DMIX_REG->dmix_layer_config_0 = 0x34561070;    // 00
	DISP_DMIX_REG->dmix_layer_config_1 = 0x00000000;    // 01 all layer blending
	DISP_DMIX_REG->dmix_ptg_config_0   = 0x00002001;    // 09
	DISP_DMIX_REG->dmix_ptg_config_2   = 0x0029f06e;    // 11
	DISP_DMIX_REG->dmix_source_select  = 0x00000006;    // 20	
		
	DISP_TGEN_REG->tgen_config           = 0x00000000;  //G213.00 // latch mode on
	DISP_TGEN_REG->tgen_dtg_config       = 0x00000300;  //G213.04 0x0300 (1080P)
	DISP_TGEN_REG->tgen_dtg_adjust1      = 0x0000100d;  //G213.23
	DISP_TGEN_REG->tgen_reset            = 0x00000001;  //G213.01
	DISP_TGEN_REG->tgen_user_int1_config = 0x00000014;  //G213.02
	DISP_TGEN_REG->tgen_user_int2_config = 0x00000020;  //G213.03			

	// From VPPDMA
	DISP_VSCL_REG->vscl0_config2        = 0x0000021F;   //G187.01
	DISP_VSCL_REG->vscl0_actrl_i_xlen   = 0x00000780;   //G187.03 (1920)
	DISP_VSCL_REG->vscl0_actrl_i_ylen   = 0x00000438;   //G187.04 (1080)
	DISP_VSCL_REG->vscl0_actrl_s_xstart = 0x00000000;   //G187.05
	DISP_VSCL_REG->vscl0_actrl_s_ystart = 0x00000000;   //G187.06
	DISP_VSCL_REG->vscl0_actrl_s_xlen   = 0x00000780;   //G187.07 (1920)
	DISP_VSCL_REG->vscl0_actrl_s_ylen   = 0x00000438;   //G187.08 (1080)
	DISP_VSCL_REG->vscl0_dctrl_o_xlen   = 0x00000780;   //G187.09 (1920)
	DISP_VSCL_REG->vscl0_dctrl_o_ylen   = 0x00000438;   //G187.10	(1080)
	DISP_VSCL_REG->vscl0_dctrl_d_xstart = 0x00000000;   //G187.11
	DISP_VSCL_REG->vscl0_dctrl_d_ystart = 0x00000000;   //G187.12
	DISP_VSCL_REG->vscl0_dctrl_d_xlen   = 0x00000780;   //G187.13 (1920)
	DISP_VSCL_REG->vscl0_dctrl_d_ylen   = 0x00000438;   //G187.14 (1080)
	
	DISP_VSCL_REG->vscl0_hint_ctrl         = 0x00000002;    //G187.18
	DISP_VSCL_REG->vscl0_hint_hfactor_low  = 0x00000000;    //G187.19
	DISP_VSCL_REG->vscl0_hint_hfactor_high = 0x00000040;    //G187.20
	DISP_VSCL_REG->vscl0_hint_initf_low    = 0x00000000;    //G187.21
	DISP_VSCL_REG->vscl0_hint_initf_high   = 0x00000000;    //G187.22	
	
	DISP_VSCL_REG->vscl0_vint_ctrl         = 0x00000002;    //G188.00
	DISP_VSCL_REG->vscl0_vint_vfactor_low  = 0x00000000;    //G188.01
	DISP_VSCL_REG->vscl0_vint_vfactor_high = 0x00000040;    //G188.02
	DISP_VSCL_REG->vscl0_vint_initf_low    = 0x00000000;    //G188.03
	DISP_VSCL_REG->vscl0_vint_initf_high   = 0x00000000;    //G188.04

	DISP_VPPDMA_REG->vdma_gc        = 0x80000028;       //G186.01 , vppdma en , urgent th = 0x28
	// vdma_cfg register (G186.02) settings for different YUV order and I143 color bar test
	// Show DRAM data
	//DISP_VPPDMA_REG->vdma_cfg        = 0x00000002;     //G186.02 , (y normal uv normal) , bist off , fmt = YUY2
	//DISP_VPPDMA_REG->vdma_cfg        = 0x00000802;     //G186.02 , (y normal uv swap) , bist on color bar , fmt = YUY2
	//DISP_VPPDMA_REG->vdma_cfg        = 0x00001002;     //G186.02 , (y swap uv normal) , bist on color bar , fmt = YUY2
	//DISP_VPPDMA_REG->vdma_cfg        = 0x00001802;     //G186.02 , (y swap uv swap) , bist on color bar , fmt = YUY2
	// Color bar from I143 DISP
	//DISP_VPPDMA_REG->vdma_cfg        = 0x00000022;     //G186.02 , no swap , bist on color bar , fmt = YUY2
	DISP_VPPDMA_REG->vdma_cfg        = vdma_cfg_value;  // G186.02
	DISP_VPPDMA_REG->vdma_frame_size = 0x04380780;      //G186.03 , frame_h , frame_w (1080 1920)
	DISP_VPPDMA_REG->vdma_crop_st    = 0x00000000;      //G186.04 , start_h , Start_w
	DISP_VPPDMA_REG->vdma_lstd_size  = (0x00000780<<1); //G186.05 , stride size (1920)
	DISP_VPPDMA_REG->vdma_data_addr1 = buf_addr;        //G186.06 , 1sr planner addr (luma)
	DISP_VPPDMA_REG->vdma_data_addr2 = 0x01dc0000;      //G186.07 , 2nd planner addr (crma)
	
	VIDEOIN_LOGI("%s end\n", __FUNCTION__);
}

static void disp_setting(u16 size, u32 buf_addr, u8 YUV_order)
{
	switch (size)
	{
		case 0:
			VIDEOIN_LOGI("Video size is 720x480\n");
			disp_setting_480P(buf_addr, YUV_order);
			hdmi_clk_init(0);		//HDMITX 480P CLK
			hdmitx_set_timming(0);	//HDMITX 480P PARAMETER
			break;

		case 1:
			VIDEOIN_LOGI("Video size is 1280x720\n");
			disp_setting_720P(buf_addr, YUV_order);
			hdmi_clk_init(2);		//HDMITX 720P CLK
			hdmitx_set_timming(2);	//HDMITX 720P PARAMETER
			break;

		case 2:
			VIDEOIN_LOGI("Video size is 1920x1080\n");
			disp_setting_1080P(buf_addr, YUV_order);
			hdmi_clk_init(3);		//HDMITX 1080P CLK
			hdmitx_set_timming(3);	//HDMITX 1080P PARAMETER			
			break;
	}
}

static void csiiw_setting(struct sp_videoin_info vi_info)
{
	u32 line_stride = 0;
	u32 frame_size = 0;
	u32 csiiw_config2 = 0x00000001; // NO_STRIDE_EN = 1
	u16 x_length = 0, y_length = 0;

	VIDEOIN_LOGI("csiiw_setting start\n");

	x_length = vi_info.x_len;
	y_length = vi_info.y_len;

	switch (vi_info.pattern)
	{
		case STILL_COLORBAR_YUV422_64X64:
			/* Color Bar YUV422 64x64 */
			VIDEOIN_LOGI("Color Bar YUV422 64x64 pattern\n");

			if ((x_length != 64) || (y_length != 64))
				VIDEOIN_LOGI("Modify size to %ux%u\n", x_length, y_length);

			// YUV422 packet data size
			// 2 pixels, 4 bytes, 32 bits			
			line_stride = ((u32)(x_length/2)*4) & 0x00003FF0; // Calculate line stride for YUV422
			frame_size  = (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);
			break;

		case STILL_COLORBAR_YUV422_1920X1080:
			/* Color Bar YUV422 1920x1080 */
			VIDEOIN_LOGI("Color Bar YUV422 1920x1080 pattern\n");

			if ((x_length != 1920) || (y_length != 1080))
				VIDEOIN_LOGI("Modify size to %ux%u\n", x_length, y_length);

			// YUV422 packet data size
			// 2 pixels, 4 bytes, 32 bits
			line_stride = ((u32)(x_length/2)*4) & 0x00003FF0;
			frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			/* Color Bar RAW10 1920x1080 */
			VIDEOIN_LOGI("Color Bar RAW10 1920x1080 pattern\n");

			if ((x_length != 1920) || (y_length != 1080))
				VIDEOIN_LOGI("Modify size to %ux%u\n", x_length, y_length);

			// Use ISP scaler to scale down image size
			switch (vi_info.scale)
			{
				case SCALE_DOWN_OFF:
					VIDEOIN_LOGI("Scaling down is off (%ux%u)\n", x_length, y_length);
					break;

				case SCALE_DOWN_FHD_HD:
					x_length = 1280;
					y_length = 720;
					VIDEOIN_LOGI("Scale down FHD to HD (%ux%u)\n", x_length, y_length);
					break;

				case SCALE_DOWN_FHD_WVGA:
					x_length = 720;
					y_length = 480;
					VIDEOIN_LOGI("Scale down FHD to WVGA (%ux%u)\n", x_length, y_length);
					break;

				case SCALE_DOWN_FHD_VGA:
					x_length = 640;
					y_length = 480;
					VIDEOIN_LOGI("Scale down FHD to VGA (%ux%u)\n", x_length, y_length);
					break;

				case SCALE_DOWN_FHD_QQVGA:
					x_length = 160;
					y_length = 120;
					VIDEOIN_LOGI("Scale down FHD to QQVGA (%ux%u)\n", x_length, y_length);
					break;
			}

			// RAW10 packet data size
			// 4 pixels, 5 bytes, 40 bits
			// ISP converts RAW10 to YUV422 and then output it to CSIIW.
			switch (vi_info.out_fmt)
			{
				case YUV422_FORMAT_UYVY_ORDER:
				case YUV422_FORMAT_YUYV_ORDER:
					line_stride = ((u32)(x_length/2)*4) & 0x00003FF0; // Calculate line stride for YUV422
					frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);

					if (vi_info.out_fmt == YUV422_FORMAT_YUYV_ORDER) {
						csiiw_config2 = csiiw_config2 | 0x00000002; // YCSWAP_EN = 1 (bit 1)
					}
					break;

				case RAW8_FORMAT:
					line_stride = ((u32)(x_length/2)*2) & 0x00003FF0; // Calculate line stride for RAW8
					frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);
					break;

				case RAW10_FORMAT:
					line_stride = ((u32)(x_length/2)*4) & 0x00003FF0; // Calculate line stride for RAW10
					frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);
					break;

				case RAW10_FORMAT_PACK_MODE:
					line_stride = ((u32)(x_length/4)*5) & 0x00003FF0; // Calculate line stride for RAW10 Pack mode
					frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);

					// For RAW10 format pack mode output
					csiiw_config2 = csiiw_config2 | 0x00000100; // OUTPUT_MODE = 1 (bit 8)
					break;
			}
			break;

		case SENSOR_INPUT:
			/* Color Bar YUV422 1920x1080 */
			VIDEOIN_LOGI("Sensor Input\n");

			if ((x_length != 1920) || (y_length != 1080))
				VIDEOIN_LOGI("Modify size to %ux%u\n", x_length, y_length);

			// YUV422 packet data size
			// 2 pixels, 4 bytes, 32 bits
			line_stride = ((u32)(x_length/2)*4) & 0x00003FF0;
			frame_size	= (((u32)y_length<<16) & 0x0FFF0000) | ((u32)x_length & 0x00000FFF);
			break;
	}
	VIDEOIN_LOGI("line_stride: 0x%08X, frame_size: 0x%08X\n", line_stride, frame_size);

	// Init CSI IW0
	CSI_IW0_REG->csiiw_latch_mode = 0x00000001;     // LATCH_EN = 1
	CSI_IW0_REG->csiiw_config0	  = 0x00002700;     // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
	CSI_IW0_REG->csiiw_base_addr  = BUFFER1_START_ADDRESS;
	CSI_IW0_REG->csiiw_stride	  = line_stride;    // LINE_STRIED = 0xF0 ((1920 / 2) * 4 / 16 = 240)
	CSI_IW0_REG->csiiw_frame_size = frame_size;     // YLEN = 0x438 (1080), XLEN = 0x780 (1920)
	CSI_IW0_REG->csiiw_config2	  = csiiw_config2;  // NO_STRIDE_EN = 1
	
	// Init CSI IW1
	CSI_IW1_REG->csiiw_latch_mode = 0x00000001;     // LATCH_EN = 1
	CSI_IW1_REG->csiiw_config0	  = 0x00002700;     // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
	CSI_IW1_REG->csiiw_base_addr  = BUFFER2_START_ADDRESS;
	CSI_IW1_REG->csiiw_stride	  = line_stride;    // LINE_STRIED = 0xF0 ((1920 / 2) * 4 / 16 = 240)
	CSI_IW1_REG->csiiw_frame_size = frame_size;     // YLEN = 0x438 (1080), XLEN = 0x780 (1920)
	CSI_IW1_REG->csiiw_config2	  = csiiw_config2;  // NO_STRIDE_EN = 1

	VIDEOIN_LOGI("csiiw_setting end\n");
}

static int sp_videoin_run(char *op, struct sp_videoin_info vi_info)
{
	unsigned int i;
	long long    buf1_addr, buf2_addr;
	long long    *pbuf1, *pbuf2;

    VIDEOIN_LOGI("sp_videoin_run start\n");

    buf1_addr = BUFFER1_START_ADDRESS;
	buf2_addr = BUFFER2_START_ADDRESS;
	pbuf1 = (long long *)buf1_addr;
	pbuf2 = (long long *)buf2_addr;

	if (strcmp(op, "start") == 0)
	{
		// Configure MOON0 registers to enable clock and disable reset
		MOON0_REG->clken[2] = 0xFFFFFFFF; // 0.3  mo_clken2, 0x9C00_000C
		MOON0_REG->clken[4] = 0xFFFFFFFF; // 0.5  mo_clken4, 0x9C00_0014
		MOON0_REG->reset[2] = 0xFFFF0000; // 0.23 mo_reset2, 0x9C00_005C
		MOON0_REG->reset[4] = 0xFFFF0000; // 0.25 mo_reset4, 0x9C00_0064

		// Configure MOON5 register2 to set ISPABP0/1 interval mode to 4T
		MOON5_REG->cfg[0] = 0x06000600;   // 5.0  mo5_cfg_0, 0x9C00_0280

	    // CSI IW init
		csiiw_setting(vi_info);

		/* Load a picture into DRAM */
    	// Enable CSIIW
    	switch (vi_info.isp)
    	{
    		case ISP0_PATH:
			    VIDEOIN_LOGI("Enable CSIIW0 only\n");
				CSI_IW0_REG->csiiw_config0 = 0x00002701; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 1
				break;

			case ISP1_PATH:
			    VIDEOIN_LOGI("Enable CSIIW1 only\n");
				CSI_IW1_REG->csiiw_config0 = 0x00002701; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 1
				break;

			case ISP_ALL_PATH:
			    VIDEOIN_LOGI("Enable CSIIW0/1\n");
				CSI_IW0_REG->csiiw_config0 = 0x00002701; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 1
				CSI_IW1_REG->csiiw_config0 = 0x00002701; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 1
				break;
    	}

		// ISP init
		isp_setting(vi_info);
	} else if (strcmp(op, "stop") == 0) {
    	// Disable CSIIW
    	switch (vi_info.isp)
    	{
    		case ISP0_PATH:
			    VIDEOIN_LOGI("Disable CSIIW0 only\n");
				CSI_IW0_REG->csiiw_config0 = 0x00002700; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
				break;

			case ISP1_PATH:
			    VIDEOIN_LOGI("Disable CSIIW1 only\n");
				CSI_IW1_REG->csiiw_config0 = 0x00002700; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
				break;

			case ISP_ALL_PATH:
				VIDEOIN_LOGI("Disable CSIIW0/1\n");
				CSI_IW0_REG->csiiw_config0 = 0x00002700; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
				CSI_IW1_REG->csiiw_config0 = 0x00002700; // CMD_URGENT_TH = 2, CMD_QUEUE = 7, CSIIW_EN = 0
				break;
    	}
	} else if (strcmp(op, "clear") == 0) {
		// Clear DRAM buffer
		// The length of "long long" data type is 8 bytes (64 bits).
	    VIDEOIN_LOGI("Clear DRAM buffers\n");
	    VIDEOIN_LOGI("buf1_addr: 0x%016llX, buf2_addr: 0x%016llX\n", buf1_addr, buf2_addr);

    	switch (vi_info.isp)
    	{
    		case ISP0_PATH:
				VIDEOIN_LOGI("Clear buffer for CSIIW0\n");
				for (i = 0; i < (BUFFER_SIZE / 8); i++) {
					*pbuf1 = 0xFFFFFFFFFFFFFFFF;
					buf1_addr += 8;
					pbuf1 = (long long *)buf1_addr;
				}
				break;

			case ISP1_PATH:
				VIDEOIN_LOGI("Clear buffer for CSIIW1\n");
				for (i = 0; i < (BUFFER_SIZE / 8); i++) {
					*pbuf2 = 0xFFFFFFFFFFFFFFFF;
					buf2_addr += 8;
					pbuf2 = (long long *)buf2_addr;
				}
				break;

			case ISP_ALL_PATH:
				VIDEOIN_LOGI("Clear buffer for CSIIW0/1\n");
				for (i = 0; i < (BUFFER_SIZE / 8); i++) {
					*pbuf1 = 0xFFFFFFFFFFFFFFFF;
					*pbuf2 = 0xFFFFFFFFFFFFFFFF;
					buf1_addr += 8;
					buf2_addr += 8;
					pbuf1 = (long long *)buf1_addr;
					pbuf2 = (long long *)buf2_addr;
				}
				break;
    	}
	}

    VIDEOIN_LOGI("sp_videoin_run end\n");

	return 0;
}

static int sp_videoin_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct sp_videoin_info vi_info; 
	char *cmd;
	char *op;
	char *type;
	int  ret = 0;
	u32  addr = 0, leng = 0;
	u32  value_32 = 0;
	u16  xlen = 64, ylen = 64;
	u16  size = 0;
	u8   value_8 = 0;
	u8   pattern = 0;
	u8   isp_path = 0;
	u8   isp_probe = 0;
	u8   output_format = 0;
	u8   scale = 0;
	u8   check_item = 0;
	u8   YUV_order = 0;

	if (argc < 2) {
		// Show usage when parameter number is less than 2
		ret = CMD_RET_USAGE;
	} else {
		// Get subcommand
		cmd = argv[1];

		if (strcmp(cmd, "run") == 0) {
			// Get parameters from argv[] array
			op = argv[2];
			if (argc > 3) isp_path = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) pattern = simple_strtoul(argv[4], NULL, 16);
			if (argc > 5) xlen = simple_strtoul(argv[5], NULL, 16);
			if (argc > 6) ylen = simple_strtoul(argv[6], NULL, 16);
			if (argc > 7) output_format = simple_strtoul(argv[7], NULL, 16);
			if (argc > 8) scale = simple_strtoul(argv[8], NULL, 16);
			if (argc > 9) isp_probe = simple_strtoul(argv[9], NULL, 16);

			VIDEOIN_LOGI("run, %s, isp_path: %u, pattern: %u, xlen: %u, ylen: %u\n", op, isp_path, pattern, xlen, ylen);
			VIDEOIN_LOGI("run, %s, output_format: %u, scale: %u, isp_probe: %u\n", op, output_format, scale, isp_probe);

			vi_info.isp     = isp_path;
			vi_info.pattern = pattern;
			vi_info.x_len   = xlen;
			vi_info.y_len   = ylen;
			vi_info.out_fmt = output_format;
			vi_info.scale   = scale;
			vi_info.probe   = isp_probe;

			ret = sp_videoin_run(op, vi_info);
		} else if (strcmp(cmd, "dump") == 0) {
			if (argc < 5) {
				ret = CMD_RET_USAGE;
			} else {
				// Get parameters from argv[] array
				type = argv[2];
				addr = simple_strtoul(argv[3], NULL, 16);
				leng = simple_strtoul(argv[4], NULL, 16);
				VIDEOIN_LOGI("dump, type: %s, addr: 0x%08X, leng: 0x%08x\n", type, addr, leng);
		
				if (leng != 0) {
					ret = dump_reg(addr, leng, type);
				}
				else {
					ret = CMD_RET_USAGE;
				}
			}
		} else if (strcmp(cmd, "reg") == 0) {
			if (argc < 4) {
				ret = CMD_RET_USAGE;
			} else {
				// Get parameters from argv[] array
				op = argv[2];
				addr = simple_strtoul(argv[3], NULL, 16);
				value_32 = (argc > 4)? simple_strtoul(argv[4], NULL, 16) : 0;
				VIDEOIN_LOGI("reg, op: %s, addr: 0x%08X, value_32: 0x%08X\n", op, addr, value_32);

				ret = reg_op(addr, &value_32, op);

				if (ret != CMD_RET_USAGE) {
					if (strcmp(op, "r") == 0)
						op = "Read";
					else if (strcmp(op, "w") == 0)
						op = "Write";

					VIDEOIN_LOGI("%s reg 0x%08X = 0x%08X\n", op, addr, value_32);		
				}
			}
		} else if (strcmp(cmd, "ispreg") == 0) {
			if (argc < 4) {
				ret = CMD_RET_USAGE;
			} else {
				// Get parameters from argv[] array
				op = argv[2];
				addr = simple_strtoul(argv[3], NULL, 16);
				value_8 = (argc > 4)? simple_strtoul(argv[4], NULL, 16) : 0;
				VIDEOIN_LOGI("ispreg, op: %s, addr: 0x%08X, value_8: 0x%02X\n", op, addr, value_8);
			
				ret = ispreg_op(addr, &value_8, op);
			
				if (ret != CMD_RET_USAGE) {
					if (strcmp(op, "r") == 0)
						op = "Read";
					else if (strcmp(op, "w") == 0)
						op = "Write";
			
					VIDEOIN_LOGI("%s ispreg 0x%08X = 0x%02X\n", op, addr, value_8); 	
				}
			}
		} else if (strcmp(cmd, "ispchk") == 0) {
			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) check_item = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) value_8 = simple_strtoul(argv[4], NULL, 16);
			VIDEOIN_LOGI("ispchk, isp_path:%u, check_item: %u, count: %u\n", isp_path, check_item, value_8);

			switch (check_item)
			{
				case 0:
					// CDSP SRAM check
					ret = ispchk_sram(isp_path);
					break;

				case 1:
					// V-sync falling edge equal count event interrupt check
					ret = ispchk_interrupt(isp_path, value_8);
					break;

				case 2:
					// Window value check
					ret = ispchk_window(isp_path);
					break;
			}
		} else if (strcmp(cmd, "disp") == 0) {
			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) size = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) YUV_order = simple_strtoul(argv[4], NULL, 16);
			VIDEOIN_LOGI("disp, isp_path:%u, size: %u, YUV_order: %u\n", isp_path, size, YUV_order);

			addr = (isp_path == 0)? BUFFER1_START_ADDRESS : BUFFER2_START_ADDRESS;
			disp_setting(size, addr, YUV_order);
			ret = 0;
		} else if (strcmp(cmd, "cmp") == 0) {	
			unsigned char *ppattern = 0;
			u32 diff = 0;
			u8 *p;

			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) pattern = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) output_format = simple_strtoul(argv[4], NULL, 16);
			if (argc > 5) size = simple_strtoul(argv[5], NULL, 16);

			VIDEOIN_LOGI("cmpare, isp_path: %u, pattern: %u, output_format: %u, count: %u\n", isp_path, pattern, output_format, size);

#if (RAW10_OUTPUT_FORMAT_TEST == 1)
			ppattern = RAW10_GOLDEN_PATTERN;
#endif

#if (MOVING_RAW10_COLOR_BAR_TEST == 1)
			switch (pattern)
			{
				case 0:
					ppattern = MOVING_1ST_FRAME_GOLDEN_PATTERN;
					break;

				case 1:
					ppattern = MOVING_2ND_FRAME_GOLDEN_PATTERN;
					break;

				case 2:
					ppattern = MOVING_3RD_FRAME_GOLDEN_PATTERN;
					break;
			}
#endif

			if (isp_path == 0)
				addr = BUFFER1_START_ADDRESS;
			else if (isp_path == 1)
				addr = BUFFER2_START_ADDRESS;
			else if (isp_path == 2)
				addr = BUFFER1_START_ADDRESS + (BUFFER_SIZE<<1);
			else if (isp_path == 3)
				addr = BUFFER2_START_ADDRESS + (BUFFER_SIZE<<1);

			VIDEOIN_LOGI("cmpare, data addr: 0x%08X\n", addr);

			// The data length is calculated as below.
			// For RAW10 output format, 
			// length = 1920 * 2 (bytes/pixel) * 2 (lines) = 7680 = 0x1E00
			leng = 0x1E00;
			p = (u8 *)((u64)addr);

			// Compare vi_info data buffer with RAW10 golden pattern which has
			// 2-line vi_info data. So for 1080p, the comparsion count is 540
			// (= 1080/2).
			for (xlen = 0; xlen < size; xlen++) {
				VIDEOIN_LOGI("x: %u, p: 0x%08llX\n", xlen, (u64)p);

				for (ylen = 0; ylen < leng; p++) {
					if (ppattern[ylen] == *p) {
						value_32++;
					}
					else {
						diff++;
						//VIDEOIN_LOGI("cmp diff at y: %u, value: 0x%02x (p: 0x%08llX, value: 0x%02x)\n", ylen, ppattern[ylen], (u64)p, *p);
					}

					// Increase array index
					switch (output_format)
					{
						case YUV422_FORMAT_UYVY_ORDER:
						case YUV422_FORMAT_YUYV_ORDER:
						case RAW10_FORMAT:
							// for raw10 output format
							ylen += 1;
							break;

						case RAW8_FORMAT:
							// for raw8 output format
							ylen += 2;
							break;
					}
				}

				VIDEOIN_LOGI("x:%u, %d are same, %d are different\n", xlen, value_32, diff);
			}

			VIDEOIN_LOGI("compare result: total is %d, %d are same, %d are different\n", (value_32 + diff), value_32, diff);
		} else if (strcmp(cmd, "conv") == 0) {
			u8 *in_addr, *out_addr;

			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) xlen = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) ylen = simple_strtoul(argv[4], NULL, 16);

			VIDEOIN_LOGI("convert, isp_path: %u, xlen: %u, ylen: %u\n", isp_path, xlen, ylen);

			addr = (isp_path == 0)? BUFFER1_START_ADDRESS : BUFFER2_START_ADDRESS;
			in_addr = (u8*)((u64)addr);
			out_addr = (u8*)((u64)addr + (BUFFER_SIZE<<1));

			VIDEOIN_LOGI("convert, in_addr: 0x%08llX, out_addr: 0x%08llX\n", (u64)in_addr, (u64)out_addr);

			convRAW10Pack(in_addr, ylen, xlen, out_addr);

			VIDEOIN_LOGI("convert, complete!\n");
		} else if (strcmp(cmd, "i2c") == 0) {
			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) check_item = simple_strtoul(argv[3], NULL, 16);
			if (argc > 4) addr = simple_strtoul(argv[4], NULL, 16);
			if (argc > 5) value_32 = simple_strtoul(argv[5], NULL, 16);
			if (argc > 6) leng = simple_strtoul(argv[6], NULL, 16);

			VIDEOIN_LOGI("i2c, isp_path: %u, check_item: %u, addr: 0x%04x, value: 0x%04x\n", isp_path, check_item, addr, value_32);

			switch (check_item)
			{
				case 0:
					// Init I2C and set the device slave address
					//addr = 0x5a<<1; // MUST shift the device slave address 1bit
					addr = 0x60;    // SC2310_DEVICE_ADDRs
					if (isp_path == 0) {
						Reset_I2C0();
						Init_I2C0((u8)addr, 0x00);
					} else {
						Reset_I2C1();
						Init_I2C1((u8)addr, 0x00);
					}
					break;

				case 1:
					// Read register via I2C
					if (isp_path == 0)
						getSensor8_I2C0((u8)addr, (u16 *)&value_32);
					else
						getSensor8_I2C1((u8)addr, (u16 *)&value_32);

					VIDEOIN_LOGI("i2c, addr: 0x%04x, value: 0x%04x\n", addr, (u16)value_32);
					break;

				case 2:
					// Write register via I2C					
					if (isp_path == 0)
						setSensor8_I2C0((u8)addr, value_32);
					else
						setSensor8_I2C1((u8)addr, value_32);
					break;

				case 3:
					// Read register via I2C
					if (isp_path == 0)
						getSensor16_I2C0((u16)addr, (u16 *)&value_32, leng);
					else
						getSensor16_I2C1((u16)addr, (u16 *)&value_32, leng);

					VIDEOIN_LOGI("i2c, addr: 0x%04x, value: 0x%04x\n", addr, (u16)value_32);
					break;

				case 4:
					// Write register via I2C
					if (isp_path == 0)
						setSensor16_I2C0((u16)addr, (u16)value_32, leng);
					else
						setSensor16_I2C1((u16)addr, (u16)value_32, leng);
					break;

				case 5:
					sensorDump(isp_path);
					break;
			}

			VIDEOIN_LOGI("i2c, complete!\n");
		} else if (strcmp(cmd, "pwr") == 0) {
			if (argc > 2) isp_path = simple_strtoul(argv[2], NULL, 16);
			if (argc > 3) check_item = simple_strtoul(argv[3], NULL, 16);

			VIDEOIN_LOGI("pwr, isp_path: %u, check_item: %u\n", isp_path, check_item);

			switch (check_item)
			{
				case 0:
					// Power sensor on
					powerSensorOn_RAM(isp_path);
					break;

				case 1:
					// Power sensor down
					powerSensorDown_RAM(isp_path);
					break;
			}

			VIDEOIN_LOGI("pwr, complete!\n");
		} else {
			// Show usage when wrong instruction occurs
			ret = CMD_RET_USAGE;
		}
	}

	return ret;
}

/*******************************************************/
U_BOOT_CMD(spvi, 10, 1, sp_videoin_test,
	"sunplus video-in validation",
	"instruction description\n"
	"spvi run [start/stop/clear] isp pattern xlen ylen output scale probe - run test pattern\n"
	"\t isp - 0: isp0; 1: isp1; 2: isp0/1\n"
	"\t pattern - select test pattern\n"
	"\t\t 0: Still Color Bar YUV422 64x64\n"
	"\t\t 1: Still Color Bar YUV422 1920x1080\n"
	"\t\t 2: Still Color Bar RAW10 1920x1080\n"
	"\t\t 3: Moving Color Bar RAW10 1920x1080\n"
	"\t\t ff: Image from camera\n"
	"\t output - select output formant for raw10 pattern\n"
	"\t\t 0: YUV422 format UYVYorder\n"
	"\t\t 1: YUV422 format YUYV order\n"
	"\t\t 2: RAW8 format\n"
	"\t\t 3: RAW10 format\n"
	"\t\t 4: RAW10 format pack mode\n"
	"\t scale - scale image size\n"
	"\t\t 0: disable H/V scale down\n"
	"\t\t 1: scale down from FHD to HD size\n"
	"\t\t 2: scale down from FHD to WVGA size\n"
	"\t\t 3: scale down from FHD to VGA size\n"
	"\t\t 4: scale down from FHD to QQVGA size\n"
	"\t probe - switch ISP inside signal\n"
	"spvi dump [b/w] addr leng - dump CSI IW registers\n"
	"\t b - one byte operation\n"
	"\t w - two bytes operation\n"
	"spvi reg [r/w] addr value - csiiw register operation\n"
	"spvi ispreg [r/w] addr value - isp register operation\n"
	"spvi ispchk isp item count - check isp block\n"
	"\t isp - 0: isp0; 1: isp1\n"
	"\t item - select check isp item\n"
	"\t\t 0: SRAM check\n"
	"\t\t 1: ISP interrupt check\n"
	"\t\t 2: window value check\n"
	"\t count - how many times to wait\n"
	"spvi disp isp size order - display YUV422 image on VIDEO screen\n"
	"\t isp - 0: isp0; 1: isp1\n"
	"\t size - select display size\n"
	"\t\t 0: 720x480 vi_info\n"
	"\t\t 1: 1280x720 vi_info\n"
	"\t\t 2: 1920x1080 vi_info\n"
	"\t order - select YUV422 order\n"
	"\t\t 0: UYVY order\n"
	"\t\t 1: YUYV order\n"
	"spvi cmp isp pattern output count - compare with golden pattern\n"
	"\t isp - 0: isp0; 1: isp1; 2: isp0 conv; 3: isp1 conv\n"
	"\t pattern - select golden pattern\n"
	"\t output - select output formant\n"
	"\t count - set comparison count\n"
	"spvi conv isp xlen ylen - convert raw10 format pack mode to raw8 format\n"
	"\t isp - 0: isp0; 1: isp1\n"
	"spvi i2c isp item addr value - Validate ISP I2C\n"
	"spvi pwr isp item - Validate power control\n"
);

