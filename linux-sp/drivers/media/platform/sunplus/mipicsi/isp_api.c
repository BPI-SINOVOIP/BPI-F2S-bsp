#include <linux/io.h>
#include <linux/delay.h>
#include "isp_api.h"
#include "reg_mipi.h"

#define PATTERN_64X64_TEST          0   // Output 64x64 size of color bar raw10 1920x1080
#define INTERRUPT_VS_FALLING        1   // Test V-sync falling edge count equal event interrupt
#define YUV422_ORIGINAL_SETTING     1


// CDSP setting table
unsigned char SF_FIXED_PATTERN_NOISE[] = {
	#include "FixedPatternNoise.txt"
};

unsigned char SF_LENS_COMP_B[] = {
	#include "lenscompb.txt"
};

unsigned char SF_LENS_COMP_G[] = {
	#include "lenscompg.txt"
};

unsigned char SF_LENS_COMP_R[] = {
	#include "lenscompr.txt"
};

unsigned char SF_POST_GAMMA_B[] = {
	#include "PostGammaB.txt"
};

unsigned char SF_POST_GAMMA_G[] = {
	#include "PostGammaG.txt"
};

unsigned char SF_POST_GAMMA_R[] = {
	#include "PostGammaR.txt"
};

/*
	@ispSleep this function depends on O.S.
*/
void ispSleep(int times)
{
	#define ISP_DELAY_TIMEBASE  21  // 20.83 ns
	u64 time;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	// Calculate how many time to delay in ns
	time = times * ISP_DELAY_TIMEBASE;
	ISPAPB_LOGI("Delay %lld ns\n", time);
	ndelay(time);

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@ispReset
*/
void ispReset_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x13, &(regs->global[0x00]));        // reset all module include ispck
	writeb(0x1c, &(regs->global[0x03]));        // enable phase clocks
	writeb(0x07, &(regs->global[0x05]));        // enbale p1xck
	writeb(0x05, &(regs->global[0x08]));        // switch b1xck/bpclk_nx to normal clocks
	writeb(0x03, &(regs->global[0x00]));        // release ispck reset
	ispSleep(20);                               // #(`TIMEBASE*20;
	//
	writeb(0x00, &(regs->global[0x00]));        // release all module reset
	//
	writeb(0x01, &(regs->sensor_if[0x6c]));     // reset front
	writeb(0x00, &(regs->sensor_if[0x6c]));     //
	//
	writeb(0x03, &(regs->global[0x00]));        // release ispck reset
	writeb(0x00, &(regs->global[0x00]));        // release all module reset
	//
	writeb(0x00, &(regs->global[0x10]));        // cclk: 48MHz

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/
void FrontInit_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;
	u8 reg_2724 = (isp_info->width & 0xFF);
	u8 reg_2725 = ((isp_info->width >> 8) & 0xFF);
	u8 reg_2726 = (isp_info->height & 0xFF);
	u8 reg_2727 = ((isp_info->height >> 8) & 0xFF);
	u8 reg_276a = 0x30;                         // raw10, h-synccount and v-sync count, 75% saturation
	u8 SigMode = STILL_WHITE;
	u8 SigGenEn = isp_info->isp_mode;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	// select test pattern
	switch (isp_info->test_pattern)
	{
		case STILL_VERTICAL_COLOR_BAR:
			ISPAPB_LOGI("Pattern mode: Still vertical color bar\n");
			SigMode |= 0x0c;                    // 75% still vertical color bar
			break;

		case MOVING_HORIZONTAL_COLOR_BAR:
			ISPAPB_LOGI("Pattern mode: Moving horizontal color bar\n");
			SigMode = 0x0f;                     // 75% moving horizontal color bar
			break;

		default:
			ISPAPB_LOGE("No such test pattern! (%d)\n" , isp_info->test_pattern);
	}
	reg_276a |= SigMode | (SigGenEn<<7);

	/* ISP0 Configuration */
	// clock setting
	writeb(0x07, &(regs->global[0x08]));
	writeb(0x05, &(regs->sensor_if[0x5e]));
	writeb(0x05, &(regs->sensor_if[0x5e]));
	//
#if (PATTERN_64X64_TEST == 1)
	writeb(0x30, &(regs->sensor_if[0x11]));     // hfall
	writeb(0x00, &(regs->sensor_if[0x12]));
	writeb(0x50, &(regs->sensor_if[0x13]));     // hrise
	writeb(0x00, &(regs->sensor_if[0x14]));
	writeb(0x00, &(regs->sensor_if[0x15]));     // vfall
	writeb(0x01, &(regs->sensor_if[0x16]));
	writeb(0x90, &(regs->sensor_if[0x17]));     // vrise
	writeb(0x01, &(regs->sensor_if[0x18]));
#else
	writeb(0x30, &(regs->sensor_if[0x11]));     // hfall
	writeb(0x02, &(regs->sensor_if[0x12]));
	writeb(0x50, &(regs->sensor_if[0x13]));     // hrise
	writeb(0x02, &(regs->sensor_if[0x14]));
	writeb(0x00, &(regs->sensor_if[0x15]));     // vfall
	writeb(0x0a, &(regs->sensor_if[0x16]));
	writeb(0x90, &(regs->sensor_if[0x17]));     // vrise
	writeb(0x0a, &(regs->sensor_if[0x18]));
#endif
	writeb(0x33, &(regs->sensor_if[0x10]));     // H/V reshape enable
	writeb(0x05, &(regs->sensor_if[0x20]));     // hoffset
	writeb(0x00, &(regs->sensor_if[0x21]));
	writeb(0x03, &(regs->sensor_if[0x22]));     // voffset
	writeb(0x00, &(regs->sensor_if[0x23]));     // 0x05; Fixed by Steve
	writeb(reg_2724, &(regs->sensor_if[0x24])); // hsize
	writeb(reg_2725, &(regs->sensor_if[0x25])); //
	writeb(reg_2726, &(regs->sensor_if[0x26])); //vsize
	writeb(reg_2727, &(regs->sensor_if[0x27])); //
	writeb(0x40, &(regs->sensor_if[0x28]));
	writeb(0x01, &(regs->sensor_if[0x40]));     // syngen enable
	writeb(0xff, &(regs->sensor_if[0x41]));     // syngen line total
#if (PATTERN_64X64_TEST == 1)
	writeb(0x04, &(regs->sensor_if[0x42]));
	writeb(0x00, &(regs->sensor_if[0x43]));     // syngen line blank
	writeb(0x02, &(regs->sensor_if[0x44]));
	writeb(0xaf, &(regs->sensor_if[0x45]));     // syngen frame total
	writeb(0x04, &(regs->sensor_if[0x46]));
	writeb(0x03, &(regs->sensor_if[0x47]));     // syngen frame blank
	writeb(0x00, &(regs->sensor_if[0x48]));
#else
	writeb(0x0f, &(regs->sensor_if[0x42]));
	writeb(0x00, &(regs->sensor_if[0x43]));     // syngen line blank
	writeb(0x08, &(regs->sensor_if[0x44]));
	writeb(0xaf, &(regs->sensor_if[0x45]));     // syngen frame total
	writeb(0x0f, &(regs->sensor_if[0x46]));
	writeb(0x03, &(regs->sensor_if[0x47]));     // syngen frame blank
	writeb(0x05, &(regs->sensor_if[0x48]));
#endif
	writeb(0x03, &(regs->sensor_if[0x49]));
	writeb(0x00, &(regs->sensor_if[0x4a]));
	writeb(reg_276a, &(regs->sensor_if[0x6a])); // siggen enable(hvalidcnt): 75% still vertical color bar
#if (INTERRUPT_VS_FALLING == 1)
	writeb(0x10, &(regs->sensor_if[0xb4]));     // Define how many V-sync falling edge to trigger interrupt
	writeb(0x02, &(regs->sensor_if[0xc0]));     // Enable V-sync falling edge count equal event interrupt
#endif
	switch (isp_info->probe)
	{
		case 0:
			ISPAPB_LOGI("ISP0 probe off\n");
			//writeb(0x00, &(regs->cdsp[0xe9]));
			//writeb(0x00, &(regs->cdsp[0xe8]));
			//writeb(0x0F, &(regs->global[0xe1]));
			break;

		case 1:
			ISPAPB_LOGI("ISP0 probe 1\n");
			writeb(0x01, &(regs->cdsp[0xe9]));
			writeb(0x09, &(regs->cdsp[0xe8]));
			writeb(0x01, &(regs->global[0xe1]));
			break;

		case 2:
			ISPAPB_LOGI("ISP0 probe 2\n");
			writeb(0x17, &(regs->cdsp[0xe9]));
			writeb(0x00, &(regs->cdsp[0xe8]));
			writeb(0x01, &(regs->global[0xe1]));
			break;
	}
	//
	// set and clear of front sensor interface
	writeb(0x01, &(regs->sensor_if[0x6c]));
	writeb(0x00, &(regs->sensor_if[0x6c]));

	ISPAPB_LOGI("0x276a:0x%02x, reg_276a0x%02x\n", readb(&(regs->sensor_if[0x6a])), reg_276a);
	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable
*/
void cdspSetTable_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;
	int i;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x00, &(regs->global[0x08]));    // use memory clock for pixel clock, master clock and mipi decoder clock
	// R table of lens compensation tables
	writeb(0x00, &(regs->cdsp[0x01]));      // select lens compensation R SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0	
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_R[i], &(regs->cdsp[0x03]));
	}
	//
	// G/Gr table of lens compensation tables
	writeb(0x01, &(regs->cdsp[0x01]));      // select lens compensation G/Gr SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_G[i], &(regs->cdsp[0x03]));
	}
	//
	// B table of lens compensation tables
	writeb(0x02, &(regs->cdsp[0x01]));      // select lens compensation B SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_B[i], &(regs->cdsp[0x03]));
	}
	//
	/* write post gamma tables */
	// R table of post gamma tables
	writeb(0x04, &(regs->cdsp[0x01]));      // select post gamma R SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_R[i], &(regs->cdsp[0x03]));
	}
	//
	// G table of post gamma tables
	writeb(0x05, &(regs->cdsp[0x01]));      // select post gamma G SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_G[i], &(regs->cdsp[0x03]));
	}
	//
	// B table of of post gamma tables
	writeb(0x06, &(regs->cdsp[0x01]));      // select post gamma B SRAM
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_B[i], &(regs->cdsp[0x03]));
	}
	//
	//  fixed pattern noise tables
	writeb(0x0D, &(regs->cdsp[0x01]));      // select fixed pattern noise
	writeb(0x03, &(regs->cdsp[0x00]));      // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0
	for (i = 0; i < 1952; i++)
	{
		writeb(SF_FIXED_PATTERN_NOISE[i], &(regs->cdsp[0x03]));
	}
	// disable set cdsp sram
	writeb(0x00, &(regs->cdsp[0x04]));      // select macro page 0 
	writeb(0x00, &(regs->cdsp[0x02]));      // set macro address to 0 
	writeb(0x00, &(regs->cdsp[0x00]));      // disable CPU access macro and adress auto increase 

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@sensorInit
*/
void sensorInit_raw10(void)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@CdspInit
*/
void CdspInit_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;
	u8 reg_21d1 = 0x00, reg_21d2 = 0x00, reg_2311 = 0x00;
	u8 reg_21b0 = 0x00, reg_21b1 = 0x00, reg_21b2 = 0x00, reg_21b3 = 0x00;
	u8 reg_21b4 = 0x00, reg_21b5 = 0x00, reg_21b6 = 0x00, reg_21b7 = 0x00;
	u8 reg_21b8 = 0x00;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch (isp_info->output_fmt)
	{
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ISPAPB_LOGI("YUV422 format output\n");
			reg_21d1 = 0x00;                    // use CDSP
			reg_21d2 = 0x00;                    // YUV422
			reg_2311 = 0x00;                    // format:2bytes
			break;

		case RAW8_FORMAT:
			ISPAPB_LOGI("RAW8 format output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x01;                    // raw8
			reg_2311 = 0x10;                    // format:1byte
			break;

		case RAW10_FORMAT:
			ISPAPB_LOGI("RAW10 format output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x02;                    // raw10
			reg_2311 = 0x10;                    // format:2byte (like yuv format)
			break;
			
		case RAW10_FORMAT_PACK_MODE:
			ISPAPB_LOGI("RAW10 format pack mode output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x02;                    // raw10
			reg_2311 = 0x00;                    // format:10bits pack mode
			break;
	}

	switch (isp_info->scale)
	{
		case SCALE_DOWN_OFF:
			ISPAPB_LOGI("Scaler is off\n");
			reg_21b0 = 0x00;                    // factor for Hsize
			reg_21b1 = 0x00;                    //
			reg_21b2 = 0x00;                    // factor for Vsize
			reg_21b3 = 0x00;                    //
			//
			reg_21b4 = 0x00;                    // factor for Hsize
			reg_21b5 = 0x00;                    //
			reg_21b6 = 0x00;                    // factor for Vsize
			reg_21b7 = 0x00;                    //
			//
			reg_21b8 = 0x00;                    // disable H/V scale down
			break;

		case SCALE_DOWN_FHD_HD:
			ISPAPB_LOGI("Scale down from FHD to HD size\n");
			// H = 1280 * 65536 / 1920 = 0xAAAB
			// V =  720 * 65536 / 1080 = 0xAAAB
			reg_21b0 = 0xAB;                    // factor for Hsize
			reg_21b1 = 0xAA;                    //
			reg_21b2 = 0xAB;                    // factor for Vsize
			reg_21b3 = 0xAA;                    //
			//
			reg_21b4 = 0xAB;                    // factor for Hsize
			reg_21b5 = 0xAA;                    //
			reg_21b6 = 0xAB;                    // factor for Vsize
			reg_21b7 = 0xAA;                    //
			//
			reg_21b8 = 0x2F;                    // enable H/V scale down
			break;

		case SCALE_DOWN_FHD_VGA:
			ISPAPB_LOGI("Scale down from FHD to VGA size\n");
			// H = 640 * 65536 / 1920 = 0x5556
			// V = 480 * 65536 / 1080 = 0x71C8
			reg_21b0 = 0x56;                    // factor for Hsize
			reg_21b1 = 0x55;                    //
			reg_21b2 = 0xC8;                    // factor for Vsize
			reg_21b3 = 0x71;                    //
			//
			reg_21b4 = 0x56;                    // factor for Hsize
			reg_21b5 = 0x55;                    //
			reg_21b6 = 0xC8;                    // factor for Vsize
			reg_21b7 = 0x71;                    //
			//
			reg_21b8 = 0x2F;                    // enable H/V scale down
			break;
			
		case SCALE_DOWN_FHD_QVGA:
			ISPAPB_LOGI("Scale down from FHD to QVGA size\n");
			// H = 160 * 65536 / 1920 = 0x1556
			// V = 120 * 65536 / 1080 = 0x1C72
			reg_21b0 = 0x56;                    // factor for Hsize
			reg_21b1 = 0x15;                    //
			reg_21b2 = 0x72;                    // factor for Vsize
			reg_21b3 = 0x1C;                    //
			//
			reg_21b4 = 0x56;                    // factor for Hsize
			reg_21b5 = 0x15;                    //
			reg_21b6 = 0x72;                    // factor for Vsize
			reg_21b7 = 0x1C;                    //
			//
			reg_21b8 = 0x2F;                    // enable H/V scale down
			break;
	}


	/* ISP0 Configuration */
	// clock setting
	writeb(0x07, &(regs->global[0x08]));
	writeb(0x05, &(regs->sensor_if[0x5e]));
	writeb(0x01, &(regs->sensor_if[0x5b]));
	//
	// CDSP register setting 
	writeb(0x00, &(regs->cdsp[0x06]));          // pixel and line switch
	writeb(0x20, &(regs->cdsp[0x07]));
	writeb(0x03, &(regs->cdsp[0x08]));          // enable manual OB 
	writeb(0x00, &(regs->cdsp[0x09]));
	writeb(0x00, &(regs->cdsp[0x0d]));          // vertical mirror line: original
	writeb(0x00, &(regs->cdsp[0x0e]));          // double buffer be set immediately
	writeb(0x00, &(regs->cdsp[0x0f]));          // without sync vd  
	writeb(0xe9, &(regs->cdsp[0x10]));          // enable global gain, LchStep=32, LcvStep=64 
	writeb(0x01, &(regs->cdsp[0x11]));          // LcHinc  
	writeb(0x01, &(regs->cdsp[0x12]));          // LcVinc  
	writeb(0x42, &(regs->cdsp[0x13]));          // LC Rgain
	writeb(0x34, &(regs->cdsp[0x14]));          // LC Ggain
	writeb(0x3c, &(regs->cdsp[0x15]));          // LC Bgain
	writeb(0x08, &(regs->cdsp[0x16]));          // LC Xoffset
	writeb(0x00, &(regs->cdsp[0x17]));
	writeb(0x18, &(regs->cdsp[0x18]));          // LC Yoffset
	writeb(0x00, &(regs->cdsp[0x19]));
	writeb(0x89, &(regs->cdsp[0x1a]));          // Centvoffset=8 Centhoffset=9   
	writeb(0x9a, &(regs->cdsp[0x1b]));          // Centvsize=9 Centhsize=10 
	writeb(0x32, &(regs->cdsp[0x1c]));          // Quan_n=3 Quan_m=2
	writeb(0x89, &(regs->cdsp[0x1d]));          // rbactthr
	writeb(0x04, &(regs->cdsp[0x1e]));          // low bad pixel threshold
	writeb(0xf7, &(regs->cdsp[0x1f]));          // high bad pixel threshold
	writeb(0x03, &(regs->cdsp[0x20]));          // enable bad pixel 
	writeb(0x01, &(regs->cdsp[0x21]));          // enable bad pixel replacement 
	writeb(0x00, &(regs->cdsp[0x24]));          // HdrmapMode=0
	writeb(0x44, &(regs->cdsp[0x25]));          // enable HDR saturation
	writeb(0xf0, &(regs->cdsp[0x26]));          // disable HDR 
	writeb(0x77, &(regs->cdsp[0x27]));          // HdrFac1: 5 and HdrFac2: 6
	writeb(0x77, &(regs->cdsp[0x28]));          // HdrFac3: 8 and HdrFac4: 7
	writeb(0x77, &(regs->cdsp[0x29]));          // HdrFac5: 9 and HdrFac6: 8
	writeb(0x00, &(regs->cdsp[0x2a]));          // HdrGain0: (34/64)
	writeb(0x00, &(regs->cdsp[0x2b]));          // HdrGain1: (51/64)
	writeb(0x00, &(regs->cdsp[0x2c]));          // HdrGain2: (68/64)
	writeb(0x00, &(regs->cdsp[0x2d]));          // HdrGain3: (16/64)
	writeb(0x00, &(regs->cdsp[0x2e]));          // HdrGain4: (02/64)
	writeb(0x00, &(regs->cdsp[0x2f]));          // HdrGain5: (01/64)
	writeb(0x40, &(regs->cdsp[0x30]));          // R WB gain:
	writeb(0x00, &(regs->cdsp[0x31]));
	writeb(0x00, &(regs->cdsp[0x32]));          // R WB offset:
	writeb(0x40, &(regs->cdsp[0x34]));          // Gr WB gain: 
	writeb(0x00, &(regs->cdsp[0x35]));
	writeb(0x00, &(regs->cdsp[0x36]));          // Gr WB offset:    
	writeb(0x40, &(regs->cdsp[0x38]));          // B WB gain:
	writeb(0x00, &(regs->cdsp[0x39]));
	writeb(0x00, &(regs->cdsp[0x3a]));          // B WB offset:
	writeb(0x40, &(regs->cdsp[0x3c]));          // Gb WB gain: 
	writeb(0x00, &(regs->cdsp[0x3d]));
	writeb(0x00, &(regs->cdsp[0x3e]));          // Gb WB offset:    
	writeb(0x70, &(regs->cdsp[0x3f]));          // WB enable  
	writeb(0x13, &(regs->cdsp[0x40]));          // disable mask
	writeb(0x57, &(regs->cdsp[0x41]));          // H/V edge threshold   
	writeb(0x06, &(regs->cdsp[0x42]));          // ir4x4 type
	writeb(0x11, &(regs->cdsp[0x43]));          // Fpncmpen
	writeb(0x13, &(regs->cdsp[0x44]));          // FPNgain 
	writeb(0x02, &(regs->cdsp[0x45]));          // FPNhoffset
	writeb(0x00, &(regs->cdsp[0x46]));          // 
	writeb(0x05, &(regs->cdsp[0x47]));          // irfacY  
	writeb(0x46, &(regs->cdsp[0x48]));          // color matrix setting 
	writeb(0x00, &(regs->cdsp[0x49]));
	writeb(0xfc, &(regs->cdsp[0x4a]));
	writeb(0x01, &(regs->cdsp[0x4b]));
	writeb(0xfe, &(regs->cdsp[0x4c]));
	writeb(0x01, &(regs->cdsp[0x4d]));
	writeb(0xf7, &(regs->cdsp[0x4e]));
	writeb(0x01, &(regs->cdsp[0x4f]));
	writeb(0x4c, &(regs->cdsp[0x50]));
	writeb(0x00, &(regs->cdsp[0x51]));
	writeb(0xfd, &(regs->cdsp[0x52]));
	writeb(0x01, &(regs->cdsp[0x53]));
	writeb(0x00, &(regs->cdsp[0x54]));
	writeb(0x00, &(regs->cdsp[0x55]));
	writeb(0xf1, &(regs->cdsp[0x56]));
	writeb(0x01, &(regs->cdsp[0x57]));
	writeb(0x4f, &(regs->cdsp[0x58]));
	writeb(0x00, &(regs->cdsp[0x59]));
	writeb(0x01, &(regs->cdsp[0x5c]));          // enable post gamma
	writeb(0x01, &(regs->cdsp[0x5d]));          // enable dither    
	writeb(0x11, &(regs->cdsp[0x5e]));          // enable Y LUT
	writeb(0x0a, &(regs->cdsp[0x60]));          // Y LUT LPF step   
	writeb(0x10, &(regs->cdsp[0x61]));          // Y LUT value 
	writeb(0x20, &(regs->cdsp[0x62]));
	writeb(0x30, &(regs->cdsp[0x63]));
	writeb(0x40, &(regs->cdsp[0x64]));
	writeb(0x50, &(regs->cdsp[0x65]));
	writeb(0x60, &(regs->cdsp[0x66]));
	writeb(0x70, &(regs->cdsp[0x67]));
	writeb(0x80, &(regs->cdsp[0x68]));
	writeb(0x90, &(regs->cdsp[0x69]));
	writeb(0xa0, &(regs->cdsp[0x6a]));
	writeb(0xb0, &(regs->cdsp[0x6b]));
	writeb(0xc0, &(regs->cdsp[0x6c]));
	writeb(0xd0, &(regs->cdsp[0x6d]));
	writeb(0xe0, &(regs->cdsp[0x6e]));
	writeb(0xf0, &(regs->cdsp[0x6f]));
	writeb(0x13, &(regs->cdsp[0x70]));          // YUVSPF setting   
	writeb(0x7f, &(regs->cdsp[0x71]));          // enable Y 5x5 edge
	writeb(0x21, &(regs->cdsp[0x72]));          // enable UV SRAM replace mode   
	writeb(0x73, &(regs->cdsp[0x73]));          // enable Y SRAM replace
	writeb(0x77, &(regs->cdsp[0x74]));          // enable all Y 5x5 edge fusion  
	writeb(0x04, &(regs->cdsp[0x75]));          // YFuLThr 
	writeb(0x08, &(regs->cdsp[0x76]));          // YFuHThr 
	writeb(0x02, &(regs->cdsp[0x77]));          // YEtrLMin
	writeb(0x01, &(regs->cdsp[0x78]));          // YEtrLMax
	writeb(0x51, &(regs->cdsp[0x79]));          // enable YHdn and factor: 256   
	writeb(0x55, &(regs->cdsp[0x7a]));          // YHdn position initialization  
	writeb(0x00, &(regs->cdsp[0x7b]));
	writeb(0xf2, &(regs->cdsp[0x7c]));          // YHdn right boundary  
	writeb(0x06, &(regs->cdsp[0x7d]));
	writeb(0x0c, &(regs->cdsp[0x7e]));          // YHdn low threshold   
	writeb(0x18, &(regs->cdsp[0x7f]));          // YHdn high threshold  
	writeb(0x36, &(regs->cdsp[0x80]));          // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1  
	writeb(0x90, &(regs->cdsp[0x81]));          // Fe00 and Fe01    
	writeb(0x99, &(regs->cdsp[0x82]));          // Fe02 and Fe11    
	writeb(0x52, &(regs->cdsp[0x83]));          // Fe12 and Fe22    
	writeb(0x02, &(regs->cdsp[0x84]));          // Fea
	writeb(0x20, &(regs->cdsp[0x85]));          // Y 5x5 edge gain  
	writeb(0x24, &(regs->cdsp[0x86]));
	writeb(0x2a, &(regs->cdsp[0x87]));
	writeb(0x24, &(regs->cdsp[0x88]));
	writeb(0x22, &(regs->cdsp[0x89]));
	writeb(0x1e, &(regs->cdsp[0x8a]));
	writeb(0x08, &(regs->cdsp[0x8b]));          // Y 5x5 edge threshold 
	writeb(0x10, &(regs->cdsp[0x8c]));
	writeb(0x18, &(regs->cdsp[0x8d]));
	writeb(0x20, &(regs->cdsp[0x8e]));
	writeb(0x40, &(regs->cdsp[0x8f]));
	writeb(0x02, &(regs->cdsp[0x90]));          // Y 5x5 edge coring value
	writeb(0x03, &(regs->cdsp[0x91]));
	writeb(0x04, &(regs->cdsp[0x92]));
	writeb(0x05, &(regs->cdsp[0x93]));
	writeb(0x05, &(regs->cdsp[0x94]));
	writeb(0x05, &(regs->cdsp[0x95]));
	writeb(0x20, &(regs->cdsp[0x96]));          // Y 5x5 edge coring threshold   
	writeb(0x40, &(regs->cdsp[0x97]));
	writeb(0x80, &(regs->cdsp[0x98]));
	writeb(0xa0, &(regs->cdsp[0x99]));
	writeb(0xc0, &(regs->cdsp[0x9a]));
	writeb(0x08, &(regs->cdsp[0x9b]));          // PEdgeThr
	writeb(0x10, &(regs->cdsp[0x9c]));          // NEdgeThr
	writeb(0x64, &(regs->cdsp[0x9d]));          // PEdgeSkR and NEdgeSkR
	writeb(0x08, &(regs->cdsp[0x9e]));          // Ysr Sobel filter threshold    
	writeb(0x04, &(regs->cdsp[0x9f]));          // Ysr weight
	writeb(reg_21b0, &(regs->cdsp[0xb0]));      // factor for Hsize
	writeb(reg_21b1, &(regs->cdsp[0xb1]));      //
	writeb(reg_21b2, &(regs->cdsp[0xb2]));      // factor for Vsize
	writeb(reg_21b3, &(regs->cdsp[0xb3]));      //
	writeb(reg_21b4, &(regs->cdsp[0xb4]));      // factor for Hsize
	writeb(reg_21b5, &(regs->cdsp[0xb5]));      //
	writeb(reg_21b6, &(regs->cdsp[0xb6]));      // factor for Vsize
	writeb(reg_21b7, &(regs->cdsp[0xb7]));      //
	writeb(reg_21b8, &(regs->cdsp[0xb8]));      // disable H/V scale down 
	writeb(0x03, &(regs->cdsp[0xba]));          // 2D YUV scaling before YUV spf 
	writeb(0x03, &(regs->cdsp[0xc0]));          // enable bchs/scale
	writeb(0x00, &(regs->cdsp[0xc1]));          // Y brightness
	writeb(0x1c, &(regs->cdsp[0xc2]));          // Y contrast
	writeb(0x0f, &(regs->cdsp[0xc3]));          // Hue SIN data
	writeb(0x3e, &(regs->cdsp[0xc4]));          // Hue COS data
	writeb(0x28, &(regs->cdsp[0xc5]));          // Hue saturation   
	writeb(0x05, &(regs->cdsp[0xc6]));          // Y offset
	writeb(0x80, &(regs->cdsp[0xc7]));          // Y center
	writeb(0x3a, &(regs->cdsp[0xce]));          // LC Gn gain
	writeb(reg_21d1, &(regs->cdsp[0xd1]));      // use CDSP
	writeb(reg_21d2, &(regs->cdsp[0xd2]));      // YUV422
	writeb(reg_2311, &(regs->reserved1[0x11])); // format:2bytes
	writeb(0x70, &(regs->new_cdsp[0x00]));      // enable new WB offset/gain and offset after gain
	writeb(0x03, &(regs->new_cdsp[0x01]));      // R new WB offset: 5
	writeb(0x01, &(regs->new_cdsp[0x02]));      // G new WB offset: 84
	writeb(0x02, &(regs->new_cdsp[0x03]));      // B new WB offset: 16
	writeb(0x3f, &(regs->new_cdsp[0x04]));      // R new WB gain: 22
	writeb(0x00, &(regs->new_cdsp[0x05]));
	writeb(0x3d, &(regs->new_cdsp[0x06]));      // G new WB gain: 0 
	writeb(0x00, &(regs->new_cdsp[0x07]));
	writeb(0x44, &(regs->new_cdsp[0x08]));      // B new WB gain: 56
	writeb(0x00, &(regs->new_cdsp[0x09]));
	writeb(0x22, &(regs->new_cdsp[0x7a]));      // enable rgbedgeen mul 2 div 1 enable rgbmode
	writeb(0x0a, &(regs->new_cdsp[0x7b]));      // rgbedgelothr
	writeb(0x28, &(regs->new_cdsp[0x7c]));      // rgbedgehithr
	writeb(0x03, &(regs->new_cdsp[0xaf]));      // enable HDR H/V smoothing mode 
	writeb(0x10, &(regs->new_cdsp[0xb0]));      // hdrsmlthr 
	writeb(0x40, &(regs->new_cdsp[0xb1]));      // hdrsmlmax 
	writeb(0x00, &(regs->new_cdsp[0xc0]));      // sYEdgeGainMode: 0
	writeb(0x09, &(regs->new_cdsp[0xc1]));      // sFe00 and sFe01  
	writeb(0x14, &(regs->new_cdsp[0xc2]));      // sFea and Fe11    
	writeb(0x00, &(regs->new_cdsp[0xc3]));      // Y 3x3 edge gain  
	writeb(0x04, &(regs->new_cdsp[0xc4]));
	writeb(0x0c, &(regs->new_cdsp[0xc5]));
	writeb(0x12, &(regs->new_cdsp[0xc6]));
	writeb(0x16, &(regs->new_cdsp[0xc7]));
	writeb(0x18, &(regs->new_cdsp[0xc8]));
	writeb(0x08, &(regs->new_cdsp[0xc9]));      // Y 3x3 edge threshold 
	writeb(0x0c, &(regs->new_cdsp[0xca]));
	writeb(0x10, &(regs->new_cdsp[0xcb]));
	writeb(0x18, &(regs->new_cdsp[0xcc]));
	writeb(0x28, &(regs->new_cdsp[0xcd]));
	writeb(0x03, &(regs->new_cdsp[0xce]));      // enable SpfBlkDatEn   
	writeb(0x08, &(regs->new_cdsp[0xcf]));      // Y77StdThr 
	writeb(0x7d, &(regs->new_cdsp[0xd0]));      // YDifMinSThr1:125 
	writeb(0x6a, &(regs->new_cdsp[0xd1]));      // YDifMinSThr2:106 
	writeb(0x62, &(regs->new_cdsp[0xd2]));      // YDifMinBThr1:98  
	writeb(0x4d, &(regs->new_cdsp[0xd3]));      // YDifMinBThr2:77  
	writeb(0x12, &(regs->new_cdsp[0xd4]));      // YStdLDiv and YStdMDiv
	writeb(0x00, &(regs->new_cdsp[0xd5]));      // YStdHDiv
	writeb(0x78, &(regs->new_cdsp[0xd6]));      // YStdMThr
	writeb(0x64, &(regs->new_cdsp[0xd7]));      // YStdMMean 
	writeb(0x82, &(regs->new_cdsp[0xd8]));      // YStdHThr
	writeb(0x8c, &(regs->new_cdsp[0xd9]));      // YStdHMean 
	writeb(0x24, &(regs->new_cdsp[0xf0]));      // YLowSel 
	writeb(0x04, &(regs->new_cdsp[0xf3]));      // YEtrMMin
	writeb(0x03, &(regs->new_cdsp[0xf4]));      // YEtrMMax
	writeb(0x06, &(regs->new_cdsp[0xf5]));      // YEtrHMin
	writeb(0x05, &(regs->new_cdsp[0xf6]));      // YEtrHMax
	writeb(0x01, &(regs->new_cdsp[0xf8]));      // enbale Irhigtmin 
	writeb(0x06, &(regs->new_cdsp[0xf9]));      // enbale Irhigtmin_r   
	writeb(0x09, &(regs->new_cdsp[0xfa]));      // enbale Irhigtmin_b   
	writeb(0xf8, &(regs->new_cdsp[0xfb]));      // Irhighthr 
	writeb(0xff, &(regs->new_cdsp[0xfc]));      // Irhratio_r
	writeb(0xf8, &(regs->new_cdsp[0xfd]));      // Irhratio_b
	writeb(0x0a, &(regs->new_cdsp[0xfe]));      // UVThr1  
	writeb(0x14, &(regs->new_cdsp[0xff]));      // UVThr2  
	writeb(0x6c, &(regs->cdsp_win[0x08]));      // FullHwdSize 
	writeb(0x07, &(regs->cdsp_win[0x09]));
	writeb(0xe8, &(regs->cdsp_win[0x0a]));      // FullVwdSize 
	writeb(0x03, &(regs->cdsp_win[0x0b]));
	writeb(0x77, &(regs->cdsp_win[0x0d]));      // PfullHwdSize, PfullVwdSize    
	writeb(0x00, &(regs->cdsp_win[0x10]));      // Window program selection (all use full window) 
	writeb(0x0c, &(regs->cdsp_win[0x11]));      // AE position RGB domain    
	writeb(0x00, &(regs->cdsp_win[0x12]));      // window hold 
	writeb(0xf0, &(regs->cdsp_win[0x13]));      // NAWB luminance high threshold 1 
	writeb(0xf1, &(regs->cdsp_win[0x14]));      // NAWB luminance high threshold 2 
	writeb(0xf2, &(regs->cdsp_win[0x15]));      // NAWB luminance high threshold 3 
	writeb(0xf3, &(regs->cdsp_win[0x16]));      // NAWB luminance high threshold 4 
	writeb(0x71, &(regs->cdsp_win[0x17]));      // enable new AWB, AWB clamp and block pipe go mode    
	writeb(0xf6, &(regs->cdsp_win[0x18]));      // NAWB R/G/B high threshold
	writeb(0x10, &(regs->cdsp_win[0x19]));      // NAWB GR shift    
	writeb(0x7f, &(regs->cdsp_win[0x1a]));      // NAWB GB shift    
	writeb(0x02, &(regs->cdsp_win[0x1b]));      // NAWB R/G/B low threshold 
	writeb(0x05, &(regs->cdsp_win[0x1c]));      // NAWB luminance low threshold 1
	writeb(0x06, &(regs->cdsp_win[0x1d]));      // NAWB luminance low threshold 2
	writeb(0x07, &(regs->cdsp_win[0x1e]));      // NAWB luminance low threshold 3
	writeb(0x08, &(regs->cdsp_win[0x1f]));      // NAWB luminance low threshold 4
	writeb(0x06, &(regs->cdsp_win[0x20]));      // NAWB GR low threshold 1
	writeb(0xf3, &(regs->cdsp_win[0x21]));      // NAWB GR high threshold 1 
	writeb(0x03, &(regs->cdsp_win[0x22]));      // NAWB GB low threshold 1
	writeb(0xee, &(regs->cdsp_win[0x23]));      // NAWB GB high threshold 1 
	writeb(0x08, &(regs->cdsp_win[0x24]));      // NAWB GR low threshold 2
	writeb(0xf8, &(regs->cdsp_win[0x25]));      // NAWB GR high threshold 2 
	writeb(0x05, &(regs->cdsp_win[0x26]));      // NAWB GB low threshold 2
	writeb(0xf0, &(regs->cdsp_win[0x27]));      // NAWB GB high threshold 2 
	writeb(0x0a, &(regs->cdsp_win[0x28]));      // NAWB GR low threshold 3
	writeb(0xfa, &(regs->cdsp_win[0x29]));      // NAWB GR high threshold 3 
	writeb(0x07, &(regs->cdsp_win[0x2a]));      // NAWB GB low threshold 3
	writeb(0xf4, &(regs->cdsp_win[0x2b]));      // NAWB GB high threshold 3 
	writeb(0x0c, &(regs->cdsp_win[0x2c]));      // NAWB GR low threshold 4
	writeb(0xfd, &(regs->cdsp_win[0x2d]));      // NAWB GR high threshold 4 
	writeb(0x09, &(regs->cdsp_win[0x2e]));      // NAWB GB low threshold 4
	writeb(0xf6, &(regs->cdsp_win[0x2f]));      // NAWB GB high threshold 4 
	writeb(0x05, &(regs->cdsp_win[0x4b]));      // Histogram 
	writeb(0x77, &(regs->cdsp_win[0x4c]));      // Low threshold    
	writeb(0x88, &(regs->cdsp_win[0x4d]));      // High threshold   
	writeb(0x01, &(regs->cdsp_win[0xa0]));      // naf1en  
	writeb(0x01, &(regs->cdsp_win[0xa1]));      // naf1jlinecnt
	writeb(0x08, &(regs->cdsp_win[0xa2]));      // naf1lowthr1 
	writeb(0x05, &(regs->cdsp_win[0xa3]));      // naf1lowthr2 
	writeb(0x02, &(regs->cdsp_win[0xa4]));      // naf1lowthr3 
	writeb(0x00, &(regs->cdsp_win[0xa5]));      // naf1lowthr4
	writeb(0xff, &(regs->cdsp_win[0xa6]));      // naf1highthr
	writeb(0x00, &(regs->cdsp_win[0xa8]));      // naf1hoffset[7:0]
	writeb(0x00, &(regs->cdsp_win[0xa9]));      // naf1hoffset[10:8]
	writeb(0x00, &(regs->cdsp_win[0xaa]));      // naf1voffset[7:0] 
	writeb(0x00, &(regs->cdsp_win[0xab]));      // naf1voffset[10:8]
	writeb(0x6c, &(regs->cdsp_win[0xac]));      // naf1hsize[7:0] 
	writeb(0x07, &(regs->cdsp_win[0xad]));      // naf1hsize[10:8]
	writeb(0xe8, &(regs->cdsp_win[0xae]));      // naf1vsize[7:0] 
	writeb(0x03, &(regs->cdsp_win[0xaf]));      // naf1vsize[10:8]
	writeb(0x01, &(regs->cdsp_win[0xf1]));      // enable AFD and H average: 4 pixels 
	writeb(0x04, &(regs->cdsp_win[0xf2]));      // AFD Hdist
	writeb(0x00, &(regs->cdsp_win[0xf3]));      // AFD Vdist
	writeb(0x00, &(regs->cdsp_win[0xf4]));      // AfdHwdOffset0    
	writeb(0x00, &(regs->cdsp_win[0xf5]));
	writeb(0x00, &(regs->cdsp_win[0xf6]));      // AfdWVOffset0
	writeb(0x00, &(regs->cdsp_win[0xf7]));
	writeb(0x6c, &(regs->cdsp_win[0xf8]));      // AfdWHSize 
	writeb(0x07, &(regs->cdsp_win[0xf9]));
	writeb(0xe8, &(regs->cdsp_win[0xfa]));      // AfdWVSize 
	writeb(0x03, &(regs->cdsp_win[0xfb]));
	writeb(0x0f, &(regs->new_cdsp_win[0x00]));  // enable His16 and auto address increase
	writeb(0x03, &(regs->new_cdsp_win[0x01]));  // His16HDist
	writeb(0x03, &(regs->new_cdsp_win[0x02]));  // His16VDist
	writeb(0x07, &(regs->new_cdsp_win[0x06]));  // AE window: 9x9 AE 
	writeb(0x55, &(regs->new_cdsp_win[0x07]));  // Pae9HaccFac, Pae9VaccFac
	writeb(0x00, &(regs->new_cdsp_win[0x08]));  // Ae9HOffset
	writeb(0x00, &(regs->new_cdsp_win[0x09]));
	writeb(0x00, &(regs->new_cdsp_win[0x0a]));  // Ae9VOffset
	writeb(0x00, &(regs->new_cdsp_win[0x0b]));
#if 1 // window value check
	writeb(0xd2, &(regs->new_cdsp_win[0x0c]));  // Ae9Hsize
	writeb(0x00, &(regs->new_cdsp_win[0x0d]));
	writeb(0x75, &(regs->new_cdsp_win[0x0e]));  // Ae9Vsize
	writeb(0x00, &(regs->new_cdsp_win[0x0f])); 
#else
	writeb(0xd7, &(regs->new_cdsp_win[0x0c]));  // Ae9Hsize
	writeb(0x00, &(regs->new_cdsp_win[0x0d]));
	writeb(0xd7, &(regs->new_cdsp_win[0x0e]));  // Ae9Vsize
	writeb(0x00, &(regs->new_cdsp_win[0x0f])); 
#endif
	writeb(0x09, &(regs->new_cdsp_win[0x20]));  // NAWB luminance low threshold 5  
	writeb(0xf4, &(regs->new_cdsp_win[0x21]));  // NAWB luminance high threshold 5 
	writeb(0x0e, &(regs->new_cdsp_win[0x22]));  // NAWB GR low threshold 5  
	writeb(0xfe, &(regs->new_cdsp_win[0x23]));  // NAWB GR high threshold 5 
	writeb(0x0b, &(regs->new_cdsp_win[0x24]));  // NAWB GB low threshold 5  
	writeb(0xf8, &(regs->new_cdsp_win[0x25]));  // NAWB GB high threshold 5 
	writeb(0x00, &(regs->new_cdsp_win[0x26]));  // NAE position 
	writeb(0x00, &(regs->new_cdsp_win[0x90]));  // SFullHOffset
	writeb(0x00, &(regs->new_cdsp_win[0x91]));    
	writeb(0x00, &(regs->new_cdsp_win[0x92]));  // SFullVOffset
	writeb(0x00, &(regs->new_cdsp_win[0x93]));    
	writeb(0x01, &(regs->new_cdsp_win[0x9f]));  // FullOffsetMode 

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@ispReset
*/
void ispReset_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x1c, &(regs->global[0x03]));    // enable phase clocks
	writeb(0x07, &(regs->global[0x05]));    // enbale p1xck
	writeb(0x00, &(regs->global[0x10]));    // cclk: 48MHz

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/

void FrontInit_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;
	u8 reg_276a = 0x70;                         // yuv422, h-synccount and v-sync count, 75% saturation
	u8 reg_2724 = (isp_info->width & 0xFF);
	u8 reg_2725 = ((isp_info->width >> 8) & 0xFF);
	u8 reg_2726 = (isp_info->height & 0xFF);
	u8 reg_2727 = ((isp_info->height >> 8) & 0xFF);
	u8 SigMode = STILL_WHITE;
	u8 SigGenEn = isp_info->isp_mode;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);
	ISPAPB_LOGE("regs: 0x%px\n", regs);
	ISPAPB_LOGE("width: %d, height: %d\n", isp_info->width, isp_info->height);

	// select test pattern
	ISPAPB_LOGI("Pattern mode: %d\n", isp_info->test_pattern);
	switch (isp_info->test_pattern)
	{
		case STILL_WHITE:
			ISPAPB_LOGI("Still white\n");
			SigMode |= 0x00;                    // still white screen
			break;

		case STILL_VERTICAL_COLOR_BAR:
			ISPAPB_LOGI("Still vertical color bar\n");
			SigMode |= 0x0c;                    // still vertical color bar
			break;

		case MOVING_HORIZONTAL_COLOR_BAR:
			ISPAPB_LOGI("Moving horizontal color bar\n");
			SigMode = 0x0f;                     // moving horizontal color bar
			break;
	}
	reg_276a |= SigMode | (SigGenEn<<7);

	/* ISP0 Configuration */
#if (YUV422_ORIGINAL_SETTING == 1) // Original
	//
	writeb(0x30, &(regs->sensor_if[0x11]));     // hfall
	writeb(0x02, &(regs->sensor_if[0x12]));
	writeb(0x50, &(regs->sensor_if[0x13]));     // hrise
	writeb(0x02, &(regs->sensor_if[0x14]));
	writeb(0x00, &(regs->sensor_if[0x15]));     // vfall
	writeb(0x0a, &(regs->sensor_if[0x16]));
	writeb(0x90, &(regs->sensor_if[0x17]));     // vrise
	writeb(0x0a, &(regs->sensor_if[0x18]));
	writeb(0x33, &(regs->sensor_if[0x10]));     // H/V reshape enable
	writeb(0x05, &(regs->sensor_if[0x20]));     // hoffset
	writeb(0x00, &(regs->sensor_if[0x21]));
	writeb(0x03, &(regs->sensor_if[0x22]));     // voffset
	writeb(0x05, &(regs->sensor_if[0x23]));
	writeb(reg_2724, &(regs->sensor_if[0x24])); // hsize
	writeb(reg_2725, &(regs->sensor_if[0x25]));
	writeb(reg_2726, &(regs->sensor_if[0x26])); // vsize
	writeb(reg_2727, &(regs->sensor_if[0x27]));
	writeb(0x40, &(regs->sensor_if[0x28]));
	writeb(0x01, &(regs->sensor_if[0x40]));     // syngen enable
	writeb(0xff, &(regs->sensor_if[0x41]));     // syngen line total
	writeb(0x0f, &(regs->sensor_if[0x42]));
	writeb(0x00, &(regs->sensor_if[0x43]));     // syngen line blank
	writeb(0x08, &(regs->sensor_if[0x44]));
	writeb(0xaf, &(regs->sensor_if[0x45]));     // syngen frame total
	writeb(0x0f, &(regs->sensor_if[0x46]));
	writeb(0x03, &(regs->sensor_if[0x47]));     // syngen frame blank
	writeb(0x05, &(regs->sensor_if[0x48]));
	writeb(0x03, &(regs->sensor_if[0x49]));
	writeb(0x00, &(regs->sensor_if[0x4a]));
	writeb(reg_276a, &(regs->sensor_if[0x6a])); // siggen enable(hvalidcnt): 75% still vertical color bar
	writeb(0x01, &(regs->sensor_if[0x05]));
#else // New (2020/01/21)
	//
	writeb(0x30, &(regs->sensor_if[0x11]));     // hfall
	writeb(0x00, &(regs->sensor_if[0x12]));
	writeb(0x50, &(regs->sensor_if[0x13]));     // hrise
	writeb(0x00, &(regs->sensor_if[0x14]));
	writeb(0x00, &(regs->sensor_if[0x15]));     // vfall
	writeb(0x0a, &(regs->sensor_if[0x16]));
	writeb(0x90, &(regs->sensor_if[0x17]));     // vrise
	writeb(0x0a, &(regs->sensor_if[0x18]));
	writeb(0x33, &(regs->sensor_if[0x10]));     // H/V reshape enable
	writeb(0x05, &(regs->sensor_if[0x20]));     // hoffset
	writeb(0x00, &(regs->sensor_if[0x21]));
	writeb(0x03, &(regs->sensor_if[0x22]));     // voffset
	writeb(0x00, &(regs->sensor_if[0x23]));
	writeb(reg_2724, &(regs->sensor_if[0x24])); // hsize
	writeb(reg_2725, &(regs->sensor_if[0x25]));
	writeb(reg_2726, &(regs->sensor_if[0x26])); // vsize
	writeb(reg_2727, &(regs->sensor_if[0x27]));
	writeb(0x40, &(regs->sensor_if[0x28]));
	writeb(0x01, &(regs->sensor_if[0x40]));     // syngen enable
	writeb(0x00, &(regs->sensor_if[0x41]));     // syngen line total
	writeb(0x08, &(regs->sensor_if[0x42]));
	writeb(0x50, &(regs->sensor_if[0x43]));     // syngen line blank
	writeb(0x00, &(regs->sensor_if[0x44]));
	writeb(0xaf, &(regs->sensor_if[0x45]));     // syngen frame total
	writeb(0x05, &(regs->sensor_if[0x46]));
	writeb(0x03, &(regs->sensor_if[0x47]));     // syngen frame blank
	writeb(0x00, &(regs->sensor_if[0x48]));
	writeb(0x03, &(regs->sensor_if[0x49]));
	writeb(0x00, &(regs->sensor_if[0x4a]));
	writeb(reg_276a, &(regs->sensor_if[0x6a])); // siggen enable(hvalidcnt): 75% still vertical color bar
	writeb(0x01, &(regs->sensor_if[0x05]));
#endif // #if 0 // Original
	switch (isp_info->probe)
	{
		case 0:
			ISPAPB_LOGI("ISP0 probe off\n");
			//writeb(0x00, &(regs->cdsp[0xe9]));
			//writeb(0x00, &(regs->cdsp[0xe8]));
			//writeb(0x0F, &(regs->global[0xe1]));
			break;

		case 1:
			ISPAPB_LOGI("ISP0 probe 1\n");
			writeb(0x01, &(regs->cdsp[0xe9]));
			writeb(0x09, &(regs->cdsp[0xe8]));
			writeb(0x01, &(regs->global[0xe1]));
			break;

		case 2:
			ISPAPB_LOGI("ISP0 probe 2\n");
			writeb(0x17, &(regs->cdsp[0xe9]));
			writeb(0x00, &(regs->cdsp[0xe8]));
			writeb(0x01, &(regs->global[0xe1]));
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable
*/
void cdspSetTable_yuv422(void)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@sensorInit
*/
void sensorInit_yuv422(void)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	/* ISP1 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB1_REG8(0x2660) = 0x01;
	//ISPAPB1_REG8(0x2660) = 0x00;

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@CdspInit
*/
void CdspInit_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// clock setting
	writeb(0x07, &(regs->global[0x08]));
	writeb(0x05, &(regs->sensor_if[0x5e]));
	writeb(0x01, &(regs->sensor_if[0x5b]));
	//
	// CDSP register setting 
	writeb(0x00, &(regs->cdsp[0x06]));      // pixel and line switch
	writeb(0x20, &(regs->cdsp[0x07]));
	writeb(0x03, &(regs->cdsp[0x08]));      // enable manual OB
	writeb(0x00, &(regs->cdsp[0x09]));
	writeb(0x00, &(regs->cdsp[0x0d]));      // vertical mirror line: original
	writeb(0x00, &(regs->cdsp[0x0e]));      // double buffer be set immediately
	writeb(0x00, &(regs->cdsp[0x0f]));      // without sync vd
	writeb(0xe9, &(regs->cdsp[0x10]));      // enable global gain, LchStep=32, LcvStep=64
	writeb(0x01, &(regs->cdsp[0x11]));      // LcHinc
	writeb(0x01, &(regs->cdsp[0x12]));      // LcVinc
	writeb(0x42, &(regs->cdsp[0x13]));      // LC Rgain
	writeb(0x34, &(regs->cdsp[0x14]));      // LC Ggain
	writeb(0x3c, &(regs->cdsp[0x15]));      // LC Bgain
	writeb(0x08, &(regs->cdsp[0x16]));      // LC Xoffset 
	writeb(0x00, &(regs->cdsp[0x17]));
	writeb(0x18, &(regs->cdsp[0x18]));      // LC Yoffset 
	writeb(0x00, &(regs->cdsp[0x19]));
	writeb(0x89, &(regs->cdsp[0x1a]));      // Centvoffset=8 Centhoffset=9
	writeb(0x9a, &(regs->cdsp[0x1b]));      // Centvsize=9 Centhsize=10
	writeb(0x32, &(regs->cdsp[0x1c]));      // Quan_n=3 Quan_m=2
	writeb(0x89, &(regs->cdsp[0x1d]));      // rbactthr
	writeb(0x04, &(regs->cdsp[0x1e]));      // low bad pixel threshold 
	writeb(0xf7, &(regs->cdsp[0x1f]));      // high bad pixel threshold 
	writeb(0x03, &(regs->cdsp[0x20]));      // enable bad pixel
	writeb(0x01, &(regs->cdsp[0x21]));      // enable bad pixel replacement
	writeb(0x00, &(regs->cdsp[0x24]));      // HdrmapMode=0
	writeb(0x44, &(regs->cdsp[0x25]));      // enable HDR saturation
	writeb(0xf0, &(regs->cdsp[0x26]));      // disable HDR
	writeb(0x77, &(regs->cdsp[0x27]));      // HdrFac1: 5 and HdrFac2: 6
	writeb(0x77, &(regs->cdsp[0x28]));      // HdrFac3: 8 and HdrFac4: 7
	writeb(0x77, &(regs->cdsp[0x29]));      // HdrFac5: 9 and HdrFac6: 8
	writeb(0x00, &(regs->cdsp[0x2a]));      // HdrGain0: (34/64)
	writeb(0x00, &(regs->cdsp[0x2b]));      // HdrGain1: (51/64)
	writeb(0x00, &(regs->cdsp[0x2c]));      // HdrGain2: (68/64)
	writeb(0x00, &(regs->cdsp[0x2d]));      // HdrGain3: (16/64)
	writeb(0x00, &(regs->cdsp[0x2e]));      // HdrGain4: (02/64)
	writeb(0x00, &(regs->cdsp[0x2f]));      // HdrGain5: (01/64)
	writeb(0x40, &(regs->cdsp[0x30]));      // R WB gain: 
	writeb(0x00, &(regs->cdsp[0x31]));
	writeb(0x00, &(regs->cdsp[0x32]));      // R WB offset:
	writeb(0x40, &(regs->cdsp[0x34]));      // Gr WB gain:
	writeb(0x00, &(regs->cdsp[0x35]));
	writeb(0x00, &(regs->cdsp[0x36]));      // Gr WB offset:
	writeb(0x40, &(regs->cdsp[0x38]));      // B WB gain: 
	writeb(0x00, &(regs->cdsp[0x39]));
	writeb(0x00, &(regs->cdsp[0x3a]));      // B WB offset:
	writeb(0x40, &(regs->cdsp[0x3c]));      // Gb WB gain:
	writeb(0x00, &(regs->cdsp[0x3d]));
	writeb(0x00, &(regs->cdsp[0x3e]));      // Gb WB offset:
	writeb(0x70, &(regs->cdsp[0x3f]));      // WB enable
	writeb(0x13, &(regs->cdsp[0x40]));      // disable mask
	writeb(0x57, &(regs->cdsp[0x41]));      // H/V edge threshold
	writeb(0x06, &(regs->cdsp[0x42]));      // ir4x4 type 
	writeb(0x11, &(regs->cdsp[0x43]));      // Fpncmpen
	writeb(0x13, &(regs->cdsp[0x44]));      // FPNgain 
	writeb(0x02, &(regs->cdsp[0x45]));      // FPNhoffset 
	writeb(0x00, &(regs->cdsp[0x46]));      // 
	writeb(0x05, &(regs->cdsp[0x47]));      // irfacY
	writeb(0x46, &(regs->cdsp[0x48]));      // color matrix setting 
	writeb(0x00, &(regs->cdsp[0x49]));
	writeb(0xfc, &(regs->cdsp[0x4a]));
	writeb(0x01, &(regs->cdsp[0x4b]));
	writeb(0xfe, &(regs->cdsp[0x4c]));
	writeb(0x01, &(regs->cdsp[0x4d]));
	writeb(0xf7, &(regs->cdsp[0x4e]));
	writeb(0x01, &(regs->cdsp[0x4f]));
	writeb(0x4c, &(regs->cdsp[0x50]));
	writeb(0x00, &(regs->cdsp[0x51]));
	writeb(0xfd, &(regs->cdsp[0x52]));
	writeb(0x01, &(regs->cdsp[0x53]));
	writeb(0x00, &(regs->cdsp[0x54]));
	writeb(0x00, &(regs->cdsp[0x55]));
	writeb(0xf1, &(regs->cdsp[0x56]));
	writeb(0x01, &(regs->cdsp[0x57]));
	writeb(0x4f, &(regs->cdsp[0x58]));
	writeb(0x00, &(regs->cdsp[0x59]));
	writeb(0x01, &(regs->cdsp[0x5c]));      // enable post gamma 
	writeb(0x01, &(regs->cdsp[0x5d]));      // enable dither
	writeb(0x11, &(regs->cdsp[0x5e]));      // enable Y LUT
	writeb(0x0a, &(regs->cdsp[0x60]));      // Y LUT LPF step 
	writeb(0x10, &(regs->cdsp[0x61]));      // Y LUT value
	writeb(0x20, &(regs->cdsp[0x62]));
	writeb(0x30, &(regs->cdsp[0x63]));
	writeb(0x40, &(regs->cdsp[0x64]));
	writeb(0x50, &(regs->cdsp[0x65]));
	writeb(0x60, &(regs->cdsp[0x66]));
	writeb(0x70, &(regs->cdsp[0x67]));
	writeb(0x80, &(regs->cdsp[0x68]));
	writeb(0x90, &(regs->cdsp[0x69]));
	writeb(0xa0, &(regs->cdsp[0x6a]));
	writeb(0xb0, &(regs->cdsp[0x6b]));
	writeb(0xc0, &(regs->cdsp[0x6c]));
	writeb(0xd0, &(regs->cdsp[0x6d]));
	writeb(0xe0, &(regs->cdsp[0x6e]));
	writeb(0xf0, &(regs->cdsp[0x6f]));
	writeb(0x13, &(regs->cdsp[0x70]));      // YUVSPF setting 
	writeb(0x7f, &(regs->cdsp[0x71]));      // enable Y 5x5 edge 
	writeb(0x21, &(regs->cdsp[0x72]));      // enable UV SRAM replace mode
	writeb(0x73, &(regs->cdsp[0x73]));      // enable Y SRAM replace
	writeb(0x77, &(regs->cdsp[0x74]));      // enable all Y 5x5 edge fusion
	writeb(0x04, &(regs->cdsp[0x75]));      // YFuLThr 
	writeb(0x08, &(regs->cdsp[0x76]));      // YFuHThr 
	writeb(0x02, &(regs->cdsp[0x77]));      // YEtrLMin
	writeb(0x01, &(regs->cdsp[0x78]));      // YEtrLMax
	writeb(0x51, &(regs->cdsp[0x79]));      // enable YHdn and factor: 256
	writeb(0x55, &(regs->cdsp[0x7a]));      // YHdn position initialization
	writeb(0x00, &(regs->cdsp[0x7b]));
	writeb(0xf2, &(regs->cdsp[0x7c]));      // YHdn right boundary
	writeb(0x06, &(regs->cdsp[0x7d]));
	writeb(0x0c, &(regs->cdsp[0x7e]));      // YHdn low threshold
	writeb(0x18, &(regs->cdsp[0x7f]));      // YHdn high threshold
	writeb(0x36, &(regs->cdsp[0x80]));      // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1
	writeb(0x90, &(regs->cdsp[0x81]));      // Fe00 and Fe01
	writeb(0x99, &(regs->cdsp[0x82]));      // Fe02 and Fe11
	writeb(0x52, &(regs->cdsp[0x83]));      // Fe12 and Fe22
	writeb(0x02, &(regs->cdsp[0x84]));      // Fea
	writeb(0x20, &(regs->cdsp[0x85]));      // Y 5x5 edge gain
	writeb(0x24, &(regs->cdsp[0x86]));
	writeb(0x2a, &(regs->cdsp[0x87]));
	writeb(0x24, &(regs->cdsp[0x88]));
	writeb(0x22, &(regs->cdsp[0x89]));
	writeb(0x1e, &(regs->cdsp[0x8a]));
	writeb(0x08, &(regs->cdsp[0x8b]));      // Y 5x5 edge threshold 
	writeb(0x10, &(regs->cdsp[0x8c]));
	writeb(0x18, &(regs->cdsp[0x8d]));
	writeb(0x20, &(regs->cdsp[0x8e]));
	writeb(0x40, &(regs->cdsp[0x8f]));
	writeb(0x02, &(regs->cdsp[0x90]));      // Y 5x5 edge coring value 
	writeb(0x03, &(regs->cdsp[0x91]));
	writeb(0x04, &(regs->cdsp[0x92]));
	writeb(0x05, &(regs->cdsp[0x93]));
	writeb(0x05, &(regs->cdsp[0x94]));
	writeb(0x05, &(regs->cdsp[0x95]));
	writeb(0x20, &(regs->cdsp[0x96]));      // Y 5x5 edge coring threshold
	writeb(0x40, &(regs->cdsp[0x97]));
	writeb(0x80, &(regs->cdsp[0x98]));
	writeb(0xa0, &(regs->cdsp[0x99]));
	writeb(0xc0, &(regs->cdsp[0x9a]));
	writeb(0x08, &(regs->cdsp[0x9b]));      // PEdgeThr
	writeb(0x10, &(regs->cdsp[0x9c]));      // NEdgeThr
	writeb(0x64, &(regs->cdsp[0x9d]));      // PEdgeSkR and NEdgeSkR
	writeb(0x08, &(regs->cdsp[0x9e]));      // Ysr Sobel filter threshold 
	writeb(0x04, &(regs->cdsp[0x9f]));      // Ysr weight 
	writeb(0x00, &(regs->cdsp[0xb8]));      // disable H/V scale down
	writeb(0x03, &(regs->cdsp[0xba]));      // 2D YUV scaling before YUV spf 
	writeb(0x03, &(regs->cdsp[0xc0]));      // enable bchs/scale 
	writeb(0x00, &(regs->cdsp[0xc1]));      // Y brightness
	writeb(0x1c, &(regs->cdsp[0xc2]));      // Y contrast 
	writeb(0x0f, &(regs->cdsp[0xc3]));      // Hue SIN data
	writeb(0x3e, &(regs->cdsp[0xc4]));      // Hue COS data
	writeb(0x28, &(regs->cdsp[0xc5]));      // Hue saturation 
	writeb(0x05, &(regs->cdsp[0xc6]));      // Y offset
	writeb(0x80, &(regs->cdsp[0xc7]));      // Y center
	writeb(0x3a, &(regs->cdsp[0xce]));      // LC Gn gain 
	writeb(0x00, &(regs->cdsp[0xd1]));      // use CDSP
	writeb(0x00, &(regs->cdsp[0xd2]));      // YUV422
	writeb(0x70, &(regs->new_cdsp[0x00]));  // enable new WB offset/gain and offset after gain
	writeb(0x03, &(regs->new_cdsp[0x01]));  // R new WB offset: 5
	writeb(0x01, &(regs->new_cdsp[0x02]));  // G new WB offset: 84
	writeb(0x02, &(regs->new_cdsp[0x03]));  // B new WB offset: 16
	writeb(0x3f, &(regs->new_cdsp[0x04]));  // R new WB gain: 22 
	writeb(0x00, &(regs->new_cdsp[0x05]));
	writeb(0x3d, &(regs->new_cdsp[0x06]));  // G new WB gain: 0
	writeb(0x00, &(regs->new_cdsp[0x07]));
	writeb(0x44, &(regs->new_cdsp[0x08]));  // B new WB gain: 56 
	writeb(0x00, &(regs->new_cdsp[0x09]));
	writeb(0x22, &(regs->new_cdsp[0x7a]));  // enable rgbedgeen mul 2 div 1 enable rgbmode
	writeb(0x0a, &(regs->new_cdsp[0x7b]));  // rgbedgelothr
	writeb(0x28, &(regs->new_cdsp[0x7c]));  // rgbedgehithr
	writeb(0x03, &(regs->new_cdsp[0xaf]));  // enable HDR H/V smoothing mode 
	writeb(0x10, &(regs->new_cdsp[0xb0]));  // hdrsmlthr
	writeb(0x40, &(regs->new_cdsp[0xb1]));  // hdrsmlmax
	writeb(0x00, &(regs->new_cdsp[0xc0]));  // sYEdgeGainMode: 0 
	writeb(0x09, &(regs->new_cdsp[0xc1]));  // sFe00 and sFe01
	writeb(0x14, &(regs->new_cdsp[0xc2]));  // sFea and Fe11
	writeb(0x00, &(regs->new_cdsp[0xc3]));  // Y 3x3 edge gain
	writeb(0x04, &(regs->new_cdsp[0xc4]));
	writeb(0x0c, &(regs->new_cdsp[0xc5]));
	writeb(0x12, &(regs->new_cdsp[0xc6]));
	writeb(0x16, &(regs->new_cdsp[0xc7]));
	writeb(0x18, &(regs->new_cdsp[0xc8]));
	writeb(0x08, &(regs->new_cdsp[0xc9]));  // Y 3x3 edge threshold 
	writeb(0x0c, &(regs->new_cdsp[0xca]));
	writeb(0x10, &(regs->new_cdsp[0xcb]));
	writeb(0x18, &(regs->new_cdsp[0xcc]));
	writeb(0x28, &(regs->new_cdsp[0xcd]));
	writeb(0x03, &(regs->new_cdsp[0xce]));  // enable SpfBlkDatEn
	writeb(0x08, &(regs->new_cdsp[0xcf]));  // Y77StdThr
	writeb(0x7d, &(regs->new_cdsp[0xd0]));  // YDifMinSThr1:125
	writeb(0x6a, &(regs->new_cdsp[0xd1]));  // YDifMinSThr2:106
	writeb(0x62, &(regs->new_cdsp[0xd2]));  // YDifMinBThr1:98
	writeb(0x4d, &(regs->new_cdsp[0xd3]));  // YDifMinBThr2:77
	writeb(0x12, &(regs->new_cdsp[0xd4]));  // YStdLDiv and YStdMDiv
	writeb(0x00, &(regs->new_cdsp[0xd5]));  // YStdHDiv
	writeb(0x78, &(regs->new_cdsp[0xd6]));  // YStdMThr
	writeb(0x64, &(regs->new_cdsp[0xd7]));  // YStdMMean
	writeb(0x82, &(regs->new_cdsp[0xd8]));  // YStdHThr
	writeb(0x8c, &(regs->new_cdsp[0xd9]));  // YStdHMean
	writeb(0x24, &(regs->new_cdsp[0xf0]));  // YLowSel 
	writeb(0x04, &(regs->new_cdsp[0xf3]));  // YEtrMMin
	writeb(0x03, &(regs->new_cdsp[0xf4]));  // YEtrMMax
	writeb(0x06, &(regs->new_cdsp[0xf5]));  // YEtrHMin
	writeb(0x05, &(regs->new_cdsp[0xf6]));  // YEtrHMax
	writeb(0x01, &(regs->new_cdsp[0xf8]));  // enbale Irhigtmin
	writeb(0x06, &(regs->new_cdsp[0xf9]));  // enbale Irhigtmin_r
	writeb(0x09, &(regs->new_cdsp[0xfa]));  // enbale Irhigtmin_b
	writeb(0xf8, &(regs->new_cdsp[0xfb]));  // Irhighthr
	writeb(0xff, &(regs->new_cdsp[0xfc]));  // Irhratio_r 
	writeb(0xf8, &(regs->new_cdsp[0xfd]));  // Irhratio_b 
	writeb(0x0a, &(regs->new_cdsp[0xfe]));  // UVThr1
	writeb(0x14, &(regs->new_cdsp[0xff]));  // UVThr2

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void ispReset(struct mipi_isp_info *isp_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch(isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ispReset_yuv422(isp_info);
			break;

		case RAW10_FORMAT:
			ispReset_raw10(isp_info);
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

//void FrontInit(u8 pattern,u16 xlen,u16 ylen, u8 probe)
void FrontInit(struct mipi_isp_info *isp_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch(isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			FrontInit_yuv422(isp_info);
			break;

		case RAW10_FORMAT:
			FrontInit_raw10(isp_info);
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void cdspSetTable(struct mipi_isp_info *isp_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch(isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			cdspSetTable_yuv422();
			break;

		case RAW10_FORMAT:
			cdspSetTable_raw10(isp_info);
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void sensorInit(struct mipi_isp_info *isp_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch(isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			sensorInit_yuv422();
			break;

		case RAW10_FORMAT:
			sensorInit_raw10();
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void CdspInit(struct mipi_isp_info *isp_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	switch (isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			CdspInit_yuv422(isp_info);
			break;

		case RAW10_FORMAT:
			CdspInit_raw10(isp_info);
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void isp_setting(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = isp_info->mipi_isp_regs;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);
	ISPAPB_LOGI("isp_mode: %d, test_pattern: %d\n", isp_info->isp_mode, isp_info->test_pattern);

	if (isp_info->isp_mode == ISP_NORMAL_MODE)
		ISPAPB_LOGI("MIPI ISP is normal mode.\n");
	else
		ISPAPB_LOGI("MIPI ISP is test mode.\n");

	switch (isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ISPAPB_LOGI("ISP input format is YUV422\n");
			ISPAPB_LOGI("Image size to %dx%d\n", isp_info->width, isp_info->height);

			ispReset(isp_info); 
			writeb(0x01, &(regs->cdsp[0xd0]));          // sofware reset CDSP interface (active)
			cdspSetTable(isp_info);
			CdspInit(isp_info);
			FrontInit(isp_info);
			sensorInit(isp_info);
			//
			writeb(0x00, &(regs->cdsp[0xd0]));          // sofware reset CDSP interface (inactive)
			writeb(0x01, &(regs->sensor_if[0x6c]));     // sofware reset Front interface
			writeb(0x00, &(regs->sensor_if[0x6c]));

			//
			readb(&(regs->cdsp[0xd4]));                 // 0x21d4, 0x80
			readb(&(regs->cdsp[0xd5]));                 // 0x21d5, 0x07
			readb(&(regs->cdsp[0xd6]));                 // 0x21d6, 0x38
			readb(&(regs->cdsp[0xd7]));                 // 0x21d7, 0x04
			break;

		case RAW10_FORMAT:
			ISPAPB_LOGI("ISP input format is RAW10\n");
			ISPAPB_LOGI("Image size to %dx%d\n", isp_info->width, isp_info->height);

			ispReset(isp_info);
			writeb(0x01, &(regs->cdsp[0xd0]));          // sofware reset CDSP interface (active)
			cdspSetTable(isp_info);
			FrontInit(isp_info);
			CdspInit(isp_info);
			sensorInit(isp_info);
			//
			writeb(0x00, &(regs->reserved2[0x300]));    // siggen enable(hvalidcnt): 75% still vertical color bar
			writeb(0x00, &(regs->cdsp[0xd0]));          // sofware reset CDSP interface (inactive)
			//
			readb(&(regs->cdsp[0xd4]));                 // 0x80
			readb(&(regs->cdsp[0xd5]));                 // 0x07
			readb(&(regs->cdsp[0xd6]));                 // 0x38
			readb(&(regs->cdsp[0xd7]));                 // 0x04
		break;
	}
	
	ISPAPB_LOGI("0x21d4:0x%02x, 0x21d5:0x%02x, 0x21d6:0x%02x, 0x21d7:0x%02x\n",
		readb(&(regs->cdsp[0xd4])), readb(&(regs->cdsp[0xd5])),
		readb(&(regs->cdsp[0xd6])), readb(&(regs->cdsp[0xd7])));


	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}
