#include <linux/io.h>
#include <linux/delay.h>
#include "isp_test_api.h"
#include "isp_api.h"
#include "reg_mipi.h"

#define PATTERN_64X64_TEST          0   // Output 64x64 size of color bar raw10 1920x1080
#define INTERRUPT_VS_FALLING        1   // Test V-sync falling edge count equal event interrupt
#define YUV422_ORIGINAL_SETTING     1


// CDSP setting table
static const unsigned char SF_FIXED_PATTERN_NOISE[] = {
	#include "FixedPatternNoise_t.txt"
};

static const unsigned char SF_LENS_COMP_B[] = {
	#include "lenscompb_t.txt"
};

static const unsigned char SF_LENS_COMP_G[] = {
	#include "lenscompg_t.txt"
};

static const unsigned char SF_LENS_COMP_R[] = {
	#include "lenscompr_t.txt"
};

static const unsigned char SF_POST_GAMMA_B[] = {
	#include "PostGammaB_t.txt"
};

static const unsigned char SF_POST_GAMMA_G[] = {
	#include "PostGammaG_t.txt"
};

static const unsigned char SF_POST_GAMMA_R[] = {
	#include "PostGammaR_t.txt"
};

/*
	@ispSleep this function depends on O.S.
*/
static void ispSleep(int times)
{
	#define ISP_DELAY_TIMEBASE  21  // 20.83 ns
	u64 time;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	// Calculate how many time to delay in ns
	time = times * ISP_DELAY_TIMEBASE;
	ISPAPI_LOGI("Delay %lld ns\n", time);
	ndelay(time);

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@ispReset
*/
static void ispReset_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x13, &(regs->reg[0x2000]));         // reset all module include ispck
	writeb(0x1c, &(regs->reg[0x2003]));         // enable phase clocks
	writeb(0x07, &(regs->reg[0x2005]));         // enbale p1xck
	writeb(0x05, &(regs->reg[0x2008]));         // switch b1xck/bpclk_nx to normal clocks
	writeb(0x03, &(regs->reg[0x2000]));         // release ispck reset
	ispSleep(20);                               // #(`TIMEBASE*20;
	//
	writeb(0x00, &(regs->reg[0x2000]));         // release all module reset
	//
	writeb(0x01, &(regs->reg[0x276c]));         // reset front
	writeb(0x00, &(regs->reg[0x276c]));         //
	//
	writeb(0x03, &(regs->reg[0x2000]));         // release ispck reset
	writeb(0x00, &(regs->reg[0x2000]));         // release all module reset
	//
	writeb(0x00, &(regs->reg[0x2010]));         // cclk: 48MHz

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/
static void FrontInit_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 reg_2724 = (isp_info->width & 0xFF);
	u8 reg_2725 = ((isp_info->width >> 8) & 0xFF);
	u8 reg_2726 = (isp_info->height & 0xFF);
	u8 reg_2727 = ((isp_info->height >> 8) & 0xFF);
	u8 reg_276a = 0x30;                         // raw10, h-synccount and v-sync count, 75% saturation
	u8 SigMode = STILL_WHITE;
	u8 SigGenEn = isp_info->isp_mode;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	// select test pattern
	switch (isp_info->test_pattern)
	{
		case STILL_VERTICAL_COLOR_BAR:
			ISPAPI_LOGI("Pattern mode: Still vertical color bar\n");
			SigMode |= 0x0c;                    // 75% still vertical color bar
			break;

		case MOVING_HORIZONTAL_COLOR_BAR:
			ISPAPI_LOGI("Pattern mode: Moving horizontal color bar\n");
			SigMode = 0x0f;                     // 75% moving horizontal color bar
			break;

		default:
			ISPAPI_LOGE("No such test pattern! (%d)\n" , isp_info->test_pattern);
	}
	reg_276a |= SigMode | (SigGenEn<<7);

	/* ISP0 Configuration */
	// clock setting
	writeb(0x07, &(regs->reg[0x2008]));
	writeb(0x05, &(regs->reg[0x275e]));
	writeb(0x05, &(regs->reg[0x275e]));
	//
#if (PATTERN_64X64_TEST == 1)
	writeb(0x30, &(regs->reg[0x2711]));         //  hfall
	writeb(0x00, &(regs->reg[0x2712]));
	writeb(0x50, &(regs->reg[0x2713]));         //  hrise
	writeb(0x00, &(regs->reg[0x2714]));
	writeb(0x00, &(regs->reg[0x2715]));         //  vfall
	writeb(0x01, &(regs->reg[0x2716]));
	writeb(0x90, &(regs->reg[0x2717]));         //  vrise
	writeb(0x01, &(regs->reg[0x2718]));
#else
	writeb(0x30, &(regs->reg[0x2711]));         //  hfall
	writeb(0x02, &(regs->reg[0x2712]));
	writeb(0x50, &(regs->reg[0x2713]));         //  hrise
	writeb(0x02, &(regs->reg[0x2714]));
	writeb(0x00, &(regs->reg[0x2715]));         //  vfall
	writeb(0x0a, &(regs->reg[0x2716]));
	writeb(0x90, &(regs->reg[0x2717]));         //  vrise
	writeb(0x0a, &(regs->reg[0x2718]));
#endif
	writeb(0x33, &(regs->reg[0x2710]));         //  H/V reshape enable
	writeb(0x05, &(regs->reg[0x2720]));         //  hoffset
	writeb(0x00, &(regs->reg[0x2721]));
	writeb(0x03, &(regs->reg[0x2722]));         //  voffset
	writeb(0x00, &(regs->reg[0x2723]));         //  0x05; Fixed by Steve
	writeb(reg_2724, &(regs->reg[0x2724]));     // hsize
	writeb(reg_2725, &(regs->reg[0x2725]));     //
	writeb(reg_2726, &(regs->reg[0x2726]));     //vsize
	writeb(reg_2727, &(regs->reg[0x2727]));     //
	writeb(0x40, &(regs->reg[0x2728]));
	writeb(0x01, &(regs->reg[0x2740]));         //  syngen enable
	writeb(0xff, &(regs->reg[0x2741]));         //  syngen line total
#if (PATTERN_64X64_TEST == 1)
	writeb(0x04, &(regs->reg[0x2742]));
	writeb(0x00, &(regs->reg[0x2743]));         //  syngen line blank
	writeb(0x02, &(regs->reg[0x2744]));
	writeb(0xaf, &(regs->reg[0x2745]));         //  syngen frame total
	writeb(0x04, &(regs->reg[0x2746]));
	writeb(0x03, &(regs->reg[0x2747]));         //  syngen frame blank
	writeb(0x00, &(regs->reg[0x2748]));
#else
	writeb(0x0f, &(regs->reg[0x2742]));
	writeb(0x00, &(regs->reg[0x2743]));         //  syngen line blank
	writeb(0x08, &(regs->reg[0x2744]));
	writeb(0xaf, &(regs->reg[0x2745]));         //  syngen frame total
	writeb(0x0f, &(regs->reg[0x2746]));
	writeb(0x03, &(regs->reg[0x2747]));         //  syngen frame blank
	writeb(0x05, &(regs->reg[0x2748]));
#endif
	writeb(0x03, &(regs->reg[0x2749]));
	writeb(0x00, &(regs->reg[0x274a]));
	writeb(reg_276a, &(regs->reg[0x276a]));     // siggen enable(hvalidcnt): 75% still vertical color bar
#if (INTERRUPT_VS_FALLING == 1)
	writeb(0x10, &(regs->reg[0x27b4]));         //  Define how many V-sync falling edge to trigger interrupt
	writeb(0x02, &(regs->reg[0x27c0]));         //  Enable V-sync falling edge count equal event interrupt
#endif
	switch (isp_info->probe)
	{
		case 0:
			ISPAPI_LOGI("ISP0 probe off\n");
			//writeb(0x00, &(regs->reg[0x21e9]));
			//writeb(0x00, &(regs->reg[0x21e8]));
			//writeb(0x0F, &(regs->reg[0x20e1]));
			break;

		case 1:
			ISPAPI_LOGI("ISP0 probe 1\n");
			writeb(0x01, &(regs->reg[0x21e9]));
			writeb(0x09, &(regs->reg[0x21e8]));
			writeb(0x01, &(regs->reg[0x20e1]));
			break;

		case 2:
			ISPAPI_LOGI("ISP0 probe 2\n");
			writeb(0x17, &(regs->reg[0x21e9]));
			writeb(0x00, &(regs->reg[0x21e8]));
			writeb(0x01, &(regs->reg[0x20e1]));
			break;
	}
	//
	// set and clear of front sensor interface
	writeb(0x01, &(regs->reg[0x276c]));
	writeb(0x00, &(regs->reg[0x276c]));

	ISPAPI_LOGI("0x276a:0x%02x, reg_276a0x%02x\n", readb(&(regs->reg[0x276a])), reg_276a);
	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable
*/
static void cdspSetTable_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	int i;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x00, &(regs->reg[0x2008]));         // use memory clock for pixel clock, master clock and mipi decoder clock
	// R table of lens compensation tables
	writeb(0x00, &(regs->reg[0x2101]));         // select lens compensation R SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0	
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_R[i], &(regs->reg[0x2103]));
	}
	//
	// G/Gr table of lens compensation tables
	writeb(0x01, &(regs->reg[0x2101]));         // select lens compensation G/Gr SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_G[i], &(regs->reg[0x2103]));
	}
	//
	// B table of lens compensation tables
	writeb(0x02, &(regs->reg[0x2101]));         // select lens compensation B SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_B[i], &(regs->reg[0x2103]));
	}
	//
	/* write post gamma tables */
	// R table of post gamma tables
	writeb(0x04, &(regs->reg[0x2101]));         // select post gamma R SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_R[i], &(regs->reg[0x2103]));
	}
	//
	// G table of post gamma tables
	writeb(0x05, &(regs->reg[0x2101]));         // select post gamma G SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_G[i], &(regs->reg[0x2103]));
	}
	//
	// B table of of post gamma tables
	writeb(0x06, &(regs->reg[0x2101]));         // select post gamma B SRAM
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_B[i], &(regs->reg[0x2103]));
	}
	//
	//  fixed pattern noise tables
	writeb(0x0D, &(regs->reg[0x2101]));         // select fixed pattern noise
	writeb(0x03, &(regs->reg[0x2100]));         // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0
	for (i = 0; i < 1952; i++)
	{
		writeb(SF_FIXED_PATTERN_NOISE[i], &(regs->reg[0x2103]));
	}
	// disable set cdsp sram
	writeb(0x00, &(regs->reg[0x2104]));         // select macro page 0 
	writeb(0x00, &(regs->reg[0x2102]));         // set macro address to 0 
	writeb(0x00, &(regs->reg[0x2100]));         // disable CPU access macro and adress auto increase 

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@sensorInit
*/
static void sensorInit_raw10(void)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@CdspInit
*/
static void CdspInit_raw10(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 reg_21d1 = 0x00, reg_21d2 = 0x00, reg_2311 = 0x00;
	u8 reg_21b0 = 0x00, reg_21b1 = 0x00, reg_21b2 = 0x00, reg_21b3 = 0x00;
	u8 reg_21b4 = 0x00, reg_21b5 = 0x00, reg_21b6 = 0x00, reg_21b7 = 0x00;
	u8 reg_21b8 = 0x00;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	switch (isp_info->output_fmt)
	{
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ISPAPI_LOGI("YUV422 format output\n");
			reg_21d1 = 0x00;                    // use CDSP
			reg_21d2 = 0x00;                    // YUV422
			reg_2311 = 0x00;                    // format:2bytes
			break;

		case RAW8_FORMAT:
			ISPAPI_LOGI("RAW8 format output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x01;                    // raw8
			reg_2311 = 0x10;                    // format:1byte
			break;

		case RAW10_FORMAT:
			ISPAPI_LOGI("RAW10 format output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x02;                    // raw10
			reg_2311 = 0x10;                    // format:2byte (like yuv format)
			break;
			
		case RAW10_FORMAT_PACK_MODE:
			ISPAPI_LOGI("RAW10 format pack mode output\n");
			reg_21d1 = 0x01;                    // bypass CDSP
			reg_21d2 = 0x02;                    // raw10
			reg_2311 = 0x00;                    // format:10bits pack mode
			break;
	}

	switch (isp_info->scale)
	{
		case SCALE_DOWN_OFF:
			ISPAPI_LOGI("Scaler is off\n");
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
			ISPAPI_LOGI("Scale down from FHD to HD size\n");
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

		case SCALE_DOWN_FHD_WVGA:
			ISPAPI_LOGI("Scale down from FHD to WVGA size\n");
			// H = 720 * 65536 / 1920 = 0x6000
			// V = 480 * 65536 / 1080 = 0x71C8
			reg_21b0 = 0x00; // factor for Hsize
			reg_21b1 = 0x60; //
			reg_21b2 = 0xC8; // factor for Vsize
			reg_21b3 = 0x71; //
			//
			reg_21b4 = 0x00; // factor for Hsize
			reg_21b5 = 0x60; //
			reg_21b6 = 0xC8; // factor for Vsize
			reg_21b7 = 0x71; //
			//
			reg_21b8 = 0x2F; // enable H/V scale down
			break;

		case SCALE_DOWN_FHD_VGA:
			ISPAPI_LOGI("Scale down from FHD to VGA size\n");
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
			
		case SCALE_DOWN_FHD_QQVGA:
			ISPAPI_LOGI("Scale down from FHD to QVGA size\n");
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
	writeb(0x07, &(regs->reg[0x2008]));
	writeb(0x05, &(regs->reg[0x275e]));
	writeb(0x01, &(regs->reg[0x275b]));
	//
	// CDSP register setting 
	writeb(0x00, &(regs->reg[0x2106]));         // pixel and line switch
	writeb(0x20, &(regs->reg[0x2107]));
	writeb(0x03, &(regs->reg[0x2108]));         // enable manual OB 
	writeb(0x00, &(regs->reg[0x2109]));
	writeb(0x00, &(regs->reg[0x210d]));         // vertical mirror line: original
	writeb(0x00, &(regs->reg[0x210e]));         // double buffer be set immediately
	writeb(0x00, &(regs->reg[0x210f]));         // without sync vd  
	writeb(0xe9, &(regs->reg[0x2110]));         // enable global gain, LchStep=32, LcvStep=64 
	writeb(0x01, &(regs->reg[0x2111]));         // LcHinc  
	writeb(0x01, &(regs->reg[0x2112]));         // LcVinc  
	writeb(0x42, &(regs->reg[0x2113]));         // LC Rgain
	writeb(0x34, &(regs->reg[0x2114]));         // LC Ggain
	writeb(0x3c, &(regs->reg[0x2115]));         // LC Bgain
	writeb(0x08, &(regs->reg[0x2116]));         // LC Xoffset
	writeb(0x00, &(regs->reg[0x2117]));
	writeb(0x18, &(regs->reg[0x2118]));         // LC Yoffset
	writeb(0x00, &(regs->reg[0x2119]));
	writeb(0x89, &(regs->reg[0x211a]));         // Centvoffset=8 Centhoffset=9   
	writeb(0x9a, &(regs->reg[0x211b]));         // Centvsize=9 Centhsize=10 
	writeb(0x32, &(regs->reg[0x211c]));         // Quan_n=3 Quan_m=2
	writeb(0x89, &(regs->reg[0x211d]));         // rbactthr
	writeb(0x04, &(regs->reg[0x211e]));         // low bad pixel threshold
	writeb(0xf7, &(regs->reg[0x211f]));         // high bad pixel threshold
	writeb(0x03, &(regs->reg[0x2120]));         // enable bad pixel 
	writeb(0x01, &(regs->reg[0x2121]));         // enable bad pixel replacement 
	writeb(0x00, &(regs->reg[0x2124]));         // HdrmapMode=0
	writeb(0x44, &(regs->reg[0x2125]));         // enable HDR saturation
	writeb(0xf0, &(regs->reg[0x2126]));         // disable HDR 
	writeb(0x77, &(regs->reg[0x2127]));         // HdrFac1: 5 and HdrFac2: 6
	writeb(0x77, &(regs->reg[0x2128]));         // HdrFac3: 8 and HdrFac4: 7
	writeb(0x77, &(regs->reg[0x2129]));         // HdrFac5: 9 and HdrFac6: 8
	writeb(0x00, &(regs->reg[0x212a]));         // HdrGain0: (34/64)
	writeb(0x00, &(regs->reg[0x212b]));         // HdrGain1: (51/64)
	writeb(0x00, &(regs->reg[0x212c]));         // HdrGain2: (68/64)
	writeb(0x00, &(regs->reg[0x212d]));         // HdrGain3: (16/64)
	writeb(0x00, &(regs->reg[0x212e]));         // HdrGain4: (02/64)
	writeb(0x00, &(regs->reg[0x212f]));         // HdrGain5: (01/64)
	writeb(0x40, &(regs->reg[0x2130]));         // R WB gain:
	writeb(0x00, &(regs->reg[0x2131]));
	writeb(0x00, &(regs->reg[0x2132]));         // R WB offset:
	writeb(0x40, &(regs->reg[0x2134]));         // Gr WB gain: 
	writeb(0x00, &(regs->reg[0x2135]));
	writeb(0x00, &(regs->reg[0x2136]));         // Gr WB offset:    
	writeb(0x40, &(regs->reg[0x2138]));         // B WB gain:
	writeb(0x00, &(regs->reg[0x2139]));
	writeb(0x00, &(regs->reg[0x213a]));         // B WB offset:
	writeb(0x40, &(regs->reg[0x213c]));         // Gb WB gain: 
	writeb(0x00, &(regs->reg[0x213d]));
	writeb(0x00, &(regs->reg[0x213e]));         // Gb WB offset:    
	writeb(0x70, &(regs->reg[0x213f]));         // WB enable  
	writeb(0x13, &(regs->reg[0x2140]));         // disable mask
	writeb(0x57, &(regs->reg[0x2141]));         // H/V edge threshold   
	writeb(0x06, &(regs->reg[0x2142]));         // ir4x4 type
	writeb(0x11, &(regs->reg[0x2143]));         // Fpncmpen
	writeb(0x13, &(regs->reg[0x2144]));         // FPNgain 
	writeb(0x02, &(regs->reg[0x2145]));         // FPNhoffset
	writeb(0x00, &(regs->reg[0x2146]));         // 
	writeb(0x05, &(regs->reg[0x2147]));         // irfacY  
	writeb(0x46, &(regs->reg[0x2148]));         // color matrix setting 
	writeb(0x00, &(regs->reg[0x2149]));
	writeb(0xfc, &(regs->reg[0x214a]));
	writeb(0x01, &(regs->reg[0x214b]));
	writeb(0xfe, &(regs->reg[0x214c]));
	writeb(0x01, &(regs->reg[0x214d]));
	writeb(0xf7, &(regs->reg[0x214e]));
	writeb(0x01, &(regs->reg[0x214f]));
	writeb(0x4c, &(regs->reg[0x2150]));
	writeb(0x00, &(regs->reg[0x2151]));
	writeb(0xfd, &(regs->reg[0x2152]));
	writeb(0x01, &(regs->reg[0x2153]));
	writeb(0x00, &(regs->reg[0x2154]));
	writeb(0x00, &(regs->reg[0x2155]));
	writeb(0xf1, &(regs->reg[0x2156]));
	writeb(0x01, &(regs->reg[0x2157]));
	writeb(0x4f, &(regs->reg[0x2158]));
	writeb(0x00, &(regs->reg[0x2159]));
	writeb(0x01, &(regs->reg[0x215c]));         // enable post gamma
	writeb(0x01, &(regs->reg[0x215d]));         // enable dither    
	writeb(0x11, &(regs->reg[0x215e]));         // enable Y LUT
	writeb(0x0a, &(regs->reg[0x2160]));         // Y LUT LPF step   
	writeb(0x10, &(regs->reg[0x2161]));         // Y LUT value 
	writeb(0x20, &(regs->reg[0x2162]));
	writeb(0x30, &(regs->reg[0x2163]));
	writeb(0x40, &(regs->reg[0x2164]));
	writeb(0x50, &(regs->reg[0x2165]));
	writeb(0x60, &(regs->reg[0x2166]));
	writeb(0x70, &(regs->reg[0x2167]));
	writeb(0x80, &(regs->reg[0x2168]));
	writeb(0x90, &(regs->reg[0x2169]));
	writeb(0xa0, &(regs->reg[0x216a]));
	writeb(0xb0, &(regs->reg[0x216b]));
	writeb(0xc0, &(regs->reg[0x216c]));
	writeb(0xd0, &(regs->reg[0x216d]));
	writeb(0xe0, &(regs->reg[0x216e]));
	writeb(0xf0, &(regs->reg[0x216f]));
	writeb(0x13, &(regs->reg[0x2170]));         // YUVSPF setting   
	writeb(0x7f, &(regs->reg[0x2171]));         // enable Y 5x5 edge
	writeb(0x21, &(regs->reg[0x2172]));         // enable UV SRAM replace mode   
	writeb(0x73, &(regs->reg[0x2173]));         // enable Y SRAM replace
	writeb(0x77, &(regs->reg[0x2174]));         // enable all Y 5x5 edge fusion  
	writeb(0x04, &(regs->reg[0x2175]));         // YFuLThr 
	writeb(0x08, &(regs->reg[0x2176]));         // YFuHThr 
	writeb(0x02, &(regs->reg[0x2177]));         // YEtrLMin
	writeb(0x01, &(regs->reg[0x2178]));         // YEtrLMax
	writeb(0x51, &(regs->reg[0x2179]));         // enable YHdn and factor: 256   
	writeb(0x55, &(regs->reg[0x217a]));         // YHdn position initialization  
	writeb(0x00, &(regs->reg[0x217b]));
	writeb(0xf2, &(regs->reg[0x217c]));         // YHdn right boundary  
	writeb(0x06, &(regs->reg[0x217d]));
	writeb(0x0c, &(regs->reg[0x217e]));         // YHdn low threshold   
	writeb(0x18, &(regs->reg[0x217f]));         // YHdn high threshold  
	writeb(0x36, &(regs->reg[0x2180]));         // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1  
	writeb(0x90, &(regs->reg[0x2181]));         // Fe00 and Fe01    
	writeb(0x99, &(regs->reg[0x2182]));         // Fe02 and Fe11    
	writeb(0x52, &(regs->reg[0x2183]));         // Fe12 and Fe22    
	writeb(0x02, &(regs->reg[0x2184]));         // Fea
	writeb(0x20, &(regs->reg[0x2185]));         // Y 5x5 edge gain  
	writeb(0x24, &(regs->reg[0x2186]));
	writeb(0x2a, &(regs->reg[0x2187]));
	writeb(0x24, &(regs->reg[0x2188]));
	writeb(0x22, &(regs->reg[0x2189]));
	writeb(0x1e, &(regs->reg[0x218a]));
	writeb(0x08, &(regs->reg[0x218b]));         // Y 5x5 edge threshold 
	writeb(0x10, &(regs->reg[0x218c]));
	writeb(0x18, &(regs->reg[0x218d]));
	writeb(0x20, &(regs->reg[0x218e]));
	writeb(0x40, &(regs->reg[0x218f]));
	writeb(0x02, &(regs->reg[0x2190]));         // Y 5x5 edge coring value
	writeb(0x03, &(regs->reg[0x2191]));
	writeb(0x04, &(regs->reg[0x2192]));
	writeb(0x05, &(regs->reg[0x2193]));
	writeb(0x05, &(regs->reg[0x2194]));
	writeb(0x05, &(regs->reg[0x2195]));
	writeb(0x20, &(regs->reg[0x2196]));         // Y 5x5 edge coring threshold   
	writeb(0x40, &(regs->reg[0x2197]));
	writeb(0x80, &(regs->reg[0x2198]));
	writeb(0xa0, &(regs->reg[0x2199]));
	writeb(0xc0, &(regs->reg[0x219a]));
	writeb(0x08, &(regs->reg[0x219b]));         // PEdgeThr
	writeb(0x10, &(regs->reg[0x219c]));         // NEdgeThr
	writeb(0x64, &(regs->reg[0x219d]));         // PEdgeSkR and NEdgeSkR
	writeb(0x08, &(regs->reg[0x219e]));         // Ysr Sobel filter threshold    
	writeb(0x04, &(regs->reg[0x219f]));         // Ysr weight
	writeb(reg_21b0, &(regs->reg[0x21b0]));     // factor for Hsize
	writeb(reg_21b1, &(regs->reg[0x21b1]));     //
	writeb(reg_21b2, &(regs->reg[0x21b2]));     // factor for Vsize
	writeb(reg_21b3, &(regs->reg[0x21b3]));     //
	writeb(reg_21b4, &(regs->reg[0x21b4]));     // factor for Hsize
	writeb(reg_21b5, &(regs->reg[0x21b5]));     //
	writeb(reg_21b6, &(regs->reg[0x21b6]));     // factor for Vsize
	writeb(reg_21b7, &(regs->reg[0x21b7]));     //
	writeb(reg_21b8, &(regs->reg[0x21b8]));     // disable H/V scale down 
	writeb(0x03, &(regs->reg[0x21ba]));         // 2D YUV scaling before YUV spf 
	writeb(0x03, &(regs->reg[0x21c0]));         // enable bchs/scale
	writeb(0x00, &(regs->reg[0x21c1]));         // Y brightness
	writeb(0x1c, &(regs->reg[0x21c2]));         // Y contrast
	writeb(0x0f, &(regs->reg[0x21c3]));         // Hue SIN data
	writeb(0x3e, &(regs->reg[0x21c4]));         // Hue COS data
	writeb(0x28, &(regs->reg[0x21c5]));         // Hue saturation   
	writeb(0x05, &(regs->reg[0x21c6]));         // Y offset
	writeb(0x80, &(regs->reg[0x21c7]));         // Y center
	writeb(0x3a, &(regs->reg[0x21ce]));         // LC Gn gain
	writeb(reg_21d1, &(regs->reg[0x21d1]));     // use CDSP
	writeb(reg_21d2, &(regs->reg[0x21d2]));     // YUV422
	writeb(reg_2311, &(regs->reg[0x2311]));     // format:2bytes
	writeb(0x70, &(regs->reg[0x3100]));         // enable new WB offset/gain and offset after gain
	writeb(0x03, &(regs->reg[0x3101]));         // R new WB offset: 5
	writeb(0x01, &(regs->reg[0x3102]));         // G new WB offset: 84
	writeb(0x02, &(regs->reg[0x3103]));         // B new WB offset: 16
	writeb(0x3f, &(regs->reg[0x3104]));         // R new WB gain: 22
	writeb(0x00, &(regs->reg[0x3105]));
	writeb(0x3d, &(regs->reg[0x3106]));         // G new WB gain: 0 
	writeb(0x00, &(regs->reg[0x3107]));
	writeb(0x44, &(regs->reg[0x3108]));         // B new WB gain: 56
	writeb(0x00, &(regs->reg[0x3109]));
	writeb(0x22, &(regs->reg[0x317a]));         // enable rgbedgeen mul 2 div 1 enable rgbmode
	writeb(0x0a, &(regs->reg[0x317b]));         // rgbedgelothr
	writeb(0x28, &(regs->reg[0x317c]));         // rgbedgehithr
	writeb(0x03, &(regs->reg[0x31af]));         // enable HDR H/V smoothing mode 
	writeb(0x10, &(regs->reg[0x31b0]));         // hdrsmlthr 
	writeb(0x40, &(regs->reg[0x31b1]));         // hdrsmlmax 
	writeb(0x00, &(regs->reg[0x31c0]));         // sYEdgeGainMode: 0
	writeb(0x09, &(regs->reg[0x31c1]));         // sFe00 and sFe01  
	writeb(0x14, &(regs->reg[0x31c2]));         // sFea and Fe11    
	writeb(0x00, &(regs->reg[0x31c3]));         // Y 3x3 edge gain  
	writeb(0x04, &(regs->reg[0x31c4]));
	writeb(0x0c, &(regs->reg[0x31c5]));
	writeb(0x12, &(regs->reg[0x31c6]));
	writeb(0x16, &(regs->reg[0x31c7]));
	writeb(0x18, &(regs->reg[0x31c8]));
	writeb(0x08, &(regs->reg[0x31c9]));         // Y 3x3 edge threshold 
	writeb(0x0c, &(regs->reg[0x31ca]));
	writeb(0x10, &(regs->reg[0x31cb]));
	writeb(0x18, &(regs->reg[0x31cc]));
	writeb(0x28, &(regs->reg[0x31cd]));
	writeb(0x03, &(regs->reg[0x31ce]));         // enable SpfBlkDatEn   
	writeb(0x08, &(regs->reg[0x31cf]));         // Y77StdThr 
	writeb(0x7d, &(regs->reg[0x31d0]));         // YDifMinSThr1:125 
	writeb(0x6a, &(regs->reg[0x31d1]));         // YDifMinSThr2:106 
	writeb(0x62, &(regs->reg[0x31d2]));         // YDifMinBThr1:98  
	writeb(0x4d, &(regs->reg[0x31d3]));         // YDifMinBThr2:77  
	writeb(0x12, &(regs->reg[0x31d4]));         // YStdLDiv and YStdMDiv
	writeb(0x00, &(regs->reg[0x31d5]));         // YStdHDiv
	writeb(0x78, &(regs->reg[0x31d6]));         // YStdMThr
	writeb(0x64, &(regs->reg[0x31d7]));         // YStdMMean 
	writeb(0x82, &(regs->reg[0x31d8]));         // YStdHThr
	writeb(0x8c, &(regs->reg[0x31d9]));         // YStdHMean 
	writeb(0x24, &(regs->reg[0x31f0]));         // YLowSel 
	writeb(0x04, &(regs->reg[0x31f3]));         // YEtrMMin
	writeb(0x03, &(regs->reg[0x31f4]));         // YEtrMMax
	writeb(0x06, &(regs->reg[0x31f5]));         // YEtrHMin
	writeb(0x05, &(regs->reg[0x31f6]));         // YEtrHMax
	writeb(0x01, &(regs->reg[0x31f8]));         // enbale Irhigtmin 
	writeb(0x06, &(regs->reg[0x31f9]));         // enbale Irhigtmin_r   
	writeb(0x09, &(regs->reg[0x31fa]));         // enbale Irhigtmin_b   
	writeb(0xf8, &(regs->reg[0x31fb]));         // Irhighthr 
	writeb(0xff, &(regs->reg[0x31fc]));         // Irhratio_r
	writeb(0xf8, &(regs->reg[0x31fd]));         // Irhratio_b
	writeb(0x0a, &(regs->reg[0x31fe]));         // UVThr1  
	writeb(0x14, &(regs->reg[0x31ff]));         // UVThr2  
	writeb(0x6c, &(regs->reg[0x2208]));         // FullHwdSize 
	writeb(0x07, &(regs->reg[0x2209]));
	writeb(0xe8, &(regs->reg[0x220a]));         // FullVwdSize 
	writeb(0x03, &(regs->reg[0x220b]));
	writeb(0x77, &(regs->reg[0x220d]));         // PfullHwdSize, PfullVwdSize    
	writeb(0x00, &(regs->reg[0x2210]));         // Window program selection (all use full window) 
	writeb(0x0c, &(regs->reg[0x2211]));         // AE position RGB domain    
	writeb(0x00, &(regs->reg[0x2212]));         // window hold 
	writeb(0xf0, &(regs->reg[0x2213]));         // NAWB luminance high threshold 1 
	writeb(0xf1, &(regs->reg[0x2214]));         // NAWB luminance high threshold 2 
	writeb(0xf2, &(regs->reg[0x2215]));         // NAWB luminance high threshold 3 
	writeb(0xf3, &(regs->reg[0x2216]));         // NAWB luminance high threshold 4 
	writeb(0x71, &(regs->reg[0x2217]));         // enable new AWB, AWB clamp and block pipe go mode    
	writeb(0xf6, &(regs->reg[0x2218]));         // NAWB R/G/B high threshold
	writeb(0x10, &(regs->reg[0x2219]));         // NAWB GR shift    
	writeb(0x7f, &(regs->reg[0x221a]));         // NAWB GB shift    
	writeb(0x02, &(regs->reg[0x221b]));         // NAWB R/G/B low threshold 
	writeb(0x05, &(regs->reg[0x221c]));         // NAWB luminance low threshold 1
	writeb(0x06, &(regs->reg[0x221d]));         // NAWB luminance low threshold 2
	writeb(0x07, &(regs->reg[0x221e]));         // NAWB luminance low threshold 3
	writeb(0x08, &(regs->reg[0x221f]));         // NAWB luminance low threshold 4
	writeb(0x06, &(regs->reg[0x2220]));         // NAWB GR low threshold 1
	writeb(0xf3, &(regs->reg[0x2221]));         // NAWB GR high threshold 1 
	writeb(0x03, &(regs->reg[0x2222]));         // NAWB GB low threshold 1
	writeb(0xee, &(regs->reg[0x2223]));         // NAWB GB high threshold 1 
	writeb(0x08, &(regs->reg[0x2224]));         // NAWB GR low threshold 2
	writeb(0xf8, &(regs->reg[0x2225]));         // NAWB GR high threshold 2 
	writeb(0x05, &(regs->reg[0x2226]));         // NAWB GB low threshold 2
	writeb(0xf0, &(regs->reg[0x2227]));         // NAWB GB high threshold 2 
	writeb(0x0a, &(regs->reg[0x2228]));         // NAWB GR low threshold 3
	writeb(0xfa, &(regs->reg[0x2229]));         // NAWB GR high threshold 3 
	writeb(0x07, &(regs->reg[0x222a]));         // NAWB GB low threshold 3
	writeb(0xf4, &(regs->reg[0x222b]));         // NAWB GB high threshold 3 
	writeb(0x0c, &(regs->reg[0x222c]));         // NAWB GR low threshold 4
	writeb(0xfd, &(regs->reg[0x222d]));         // NAWB GR high threshold 4 
	writeb(0x09, &(regs->reg[0x222e]));         // NAWB GB low threshold 4
	writeb(0xf6, &(regs->reg[0x222f]));         // NAWB GB high threshold 4 
	writeb(0x05, &(regs->reg[0x224b]));         // Histogram 
	writeb(0x77, &(regs->reg[0x224c]));         // Low threshold    
	writeb(0x88, &(regs->reg[0x224d]));         // High threshold   
	writeb(0x01, &(regs->reg[0x22a0]));         // naf1en  
	writeb(0x01, &(regs->reg[0x22a1]));         // naf1jlinecnt
	writeb(0x08, &(regs->reg[0x22a2]));         // naf1lowthr1 
	writeb(0x05, &(regs->reg[0x22a3]));         // naf1lowthr2 
	writeb(0x02, &(regs->reg[0x22a4]));         // naf1lowthr3 
	writeb(0x00, &(regs->reg[0x22a5]));         // naf1lowthr4
	writeb(0xff, &(regs->reg[0x22a6]));         // naf1highthr
	writeb(0x00, &(regs->reg[0x22a8]));         // naf1hoffset[7:0]
	writeb(0x00, &(regs->reg[0x22a9]));         // naf1hoffset[10:8]
	writeb(0x00, &(regs->reg[0x22aa]));         // naf1voffset[7:0] 
	writeb(0x00, &(regs->reg[0x22ab]));         // naf1voffset[10:8]
	writeb(0x6c, &(regs->reg[0x22ac]));         // naf1hsize[7:0] 
	writeb(0x07, &(regs->reg[0x22ad]));         // naf1hsize[10:8]
	writeb(0xe8, &(regs->reg[0x22ae]));         // naf1vsize[7:0] 
	writeb(0x03, &(regs->reg[0x22af]));         // naf1vsize[10:8]
	writeb(0x01, &(regs->reg[0x22f1]));         // enable AFD and H average: 4 pixels 
	writeb(0x04, &(regs->reg[0x22f2]));         // AFD Hdist
	writeb(0x00, &(regs->reg[0x22f3]));         // AFD Vdist
	writeb(0x00, &(regs->reg[0x22f4]));         // AfdHwdOffset0    
	writeb(0x00, &(regs->reg[0x22f5]));
	writeb(0x00, &(regs->reg[0x22f6]));         // AfdWVOffset0
	writeb(0x00, &(regs->reg[0x22f7]));
	writeb(0x6c, &(regs->reg[0x22f8]));         // AfdWHSize 
	writeb(0x07, &(regs->reg[0x22f9]));
	writeb(0xe8, &(regs->reg[0x22fa]));         // AfdWVSize 
	writeb(0x03, &(regs->reg[0x22fb]));
	writeb(0x0f, &(regs->reg[0x3200]));         // enable His16 and auto address increase
	writeb(0x03, &(regs->reg[0x3201]));         // His16HDist
	writeb(0x03, &(regs->reg[0x3202]));         // His16VDist
	writeb(0x07, &(regs->reg[0x3206]));         // AE window: 9x9 AE 
	writeb(0x55, &(regs->reg[0x3207]));         // Pae9HaccFac, Pae9VaccFac
	writeb(0x00, &(regs->reg[0x3208]));         // Ae9HOffset
	writeb(0x00, &(regs->reg[0x3209]));
	writeb(0x00, &(regs->reg[0x320a]));         // Ae9VOffset
	writeb(0x00, &(regs->reg[0x320b]));
#if 1 // window value check
	writeb(0xd2, &(regs->reg[0x320c]));         // Ae9Hsize
	writeb(0x00, &(regs->reg[0x320d]));
	writeb(0x75, &(regs->reg[0x320e]));         // Ae9Vsize
	writeb(0x00, &(regs->reg[0x320f])); 
#else
	writeb(0xd7, &(regs->reg[0x320c]));         // Ae9Hsize
	writeb(0x00, &(regs->reg[0x320d]));
	writeb(0xd7, &(regs->reg[0x320e]));         // Ae9Vsize
	writeb(0x00, &(regs->reg[0x320f])); 
#endif
	writeb(0x09, &(regs->reg[0x3220]));         // NAWB luminance low threshold 5  
	writeb(0xf4, &(regs->reg[0x3221]));         // NAWB luminance high threshold 5 
	writeb(0x0e, &(regs->reg[0x3222]));         // NAWB GR low threshold 5  
	writeb(0xfe, &(regs->reg[0x3223]));         // NAWB GR high threshold 5 
	writeb(0x0b, &(regs->reg[0x3224]));         // NAWB GB low threshold 5  
	writeb(0xf8, &(regs->reg[0x3225]));         // NAWB GB high threshold 5 
	writeb(0x00, &(regs->reg[0x3226]));         // NAE position 
	writeb(0x00, &(regs->reg[0x3290]));         // SFullHOffset
	writeb(0x00, &(regs->reg[0x3291]));    
	writeb(0x00, &(regs->reg[0x3292]));         // SFullVOffset
	writeb(0x00, &(regs->reg[0x3293]));    
	writeb(0x01, &(regs->reg[0x329f]));         // FullOffsetMode 

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@ispReset
*/
static void ispReset_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	writeb(0x1c, &(regs->reg[0x2003]));         // enable phase clocks
	writeb(0x07, &(regs->reg[0x2005]));         // enbale p1xck
	writeb(0x00, &(regs->reg[0x2010]));         // cclk: 48MHz

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/

static void FrontInit_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 reg_276a = 0x70;                         // yuv422, h-synccount and v-sync count, 75% saturation
	u8 reg_2724 = (isp_info->width & 0xFF);
	u8 reg_2725 = ((isp_info->width >> 8) & 0xFF);
	u8 reg_2726 = (isp_info->height & 0xFF);
	u8 reg_2727 = ((isp_info->height >> 8) & 0xFF);
	u8 SigMode = STILL_WHITE;
	u8 SigGenEn = isp_info->isp_mode;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);
	ISPAPI_LOGI("regs: 0x%px\n", regs);
	ISPAPI_LOGI("width: %d, height: %d\n", isp_info->width, isp_info->height);

	// select test pattern
	ISPAPI_LOGI("Pattern mode: %d\n", isp_info->test_pattern);
	switch (isp_info->test_pattern)
	{
		case STILL_WHITE:
			ISPAPI_LOGI("Still white\n");
			SigMode |= 0x00;                    // still white screen
			break;

		case STILL_VERTICAL_COLOR_BAR:
			ISPAPI_LOGI("Still vertical color bar\n");
			SigMode |= 0x0c;                    // still vertical color bar
			break;

		case MOVING_HORIZONTAL_COLOR_BAR:
			ISPAPI_LOGI("Moving horizontal color bar\n");
			SigMode = 0x0f;                     // moving horizontal color bar
			break;
	}
	reg_276a |= SigMode | (SigGenEn<<7);

	/* ISP0 Configuration */
#if (YUV422_ORIGINAL_SETTING == 1) // Original
	//
	writeb(0x30, &(regs->reg[0x2711]));         // hfall
	writeb(0x02, &(regs->reg[0x2712]));
	writeb(0x50, &(regs->reg[0x2713]));         // hrise
	writeb(0x02, &(regs->reg[0x2714]));
	writeb(0x00, &(regs->reg[0x2715]));         // vfall
	writeb(0x0a, &(regs->reg[0x2716]));
	writeb(0x90, &(regs->reg[0x2717]));         // vrise
	writeb(0x0a, &(regs->reg[0x2718]));
	writeb(0x33, &(regs->reg[0x2710]));         // H/V reshape enable
	writeb(0x05, &(regs->reg[0x2720]));         // hoffset
	writeb(0x00, &(regs->reg[0x2721]));
	writeb(0x03, &(regs->reg[0x2722]));         // voffset
	writeb(0x05, &(regs->reg[0x2723]));
	writeb(reg_2724, &(regs->reg[0x2724]));     // hsize
	writeb(reg_2725, &(regs->reg[0x2725]));
	writeb(reg_2726, &(regs->reg[0x2726]));     // vsize
	writeb(reg_2727, &(regs->reg[0x2727]));
	writeb(0x40, &(regs->reg[0x2728]));
	writeb(0x01, &(regs->reg[0x2740]));         // syngen enable
	writeb(0xff, &(regs->reg[0x2741]));         // syngen line total
	writeb(0x0f, &(regs->reg[0x2742]));
	writeb(0x00, &(regs->reg[0x2743]));         // syngen line blank
	writeb(0x08, &(regs->reg[0x2744]));
	writeb(0xaf, &(regs->reg[0x2745]));         // syngen frame total
	writeb(0x0f, &(regs->reg[0x2746]));
	writeb(0x03, &(regs->reg[0x2747]));         // syngen frame blank
	writeb(0x05, &(regs->reg[0x2748]));
	writeb(0x03, &(regs->reg[0x2749]));
	writeb(0x00, &(regs->reg[0x274a]));
	writeb(reg_276a, &(regs->reg[0x276a]));     // siggen enable(hvalidcnt): 75% still vertical color bar
	writeb(0x01, &(regs->reg[0x2705]));
#else // New (2020/01/21)
	//
	writeb(0x30, &(regs->reg[0x2711]));         // hfall
	writeb(0x00, &(regs->reg[0x2712]));
	writeb(0x50, &(regs->reg[0x2713]));         // hrise
	writeb(0x00, &(regs->reg[0x2714]));
	writeb(0x00, &(regs->reg[0x2715]));         // vfall
	writeb(0x0a, &(regs->reg[0x2716]));
	writeb(0x90, &(regs->reg[0x2717]));         // vrise
	writeb(0x0a, &(regs->reg[0x2718]));
	writeb(0x33, &(regs->reg[0x2710]));         // H/V reshape enable
	writeb(0x05, &(regs->reg[0x2720]));         // hoffset
	writeb(0x00, &(regs->reg[0x2721]));
	writeb(0x03, &(regs->reg[0x2722]));         // voffset
	writeb(0x00, &(regs->reg[0x2723]));
	writeb(reg_2724, &(regs->reg[0x2724]));     // hsize
	writeb(reg_2725, &(regs->reg[0x2725]));
	writeb(reg_2726, &(regs->reg[0x2726]));     // vsize
	writeb(reg_2727, &(regs->reg[0x2727]));
	writeb(0x40, &(regs->reg[0x2728]));
	writeb(0x01, &(regs->reg[0x2740]));         // syngen enable
	writeb(0x00, &(regs->reg[0x2741]));         // syngen line total
	writeb(0x08, &(regs->reg[0x2742]));
	writeb(0x50, &(regs->reg[0x2743]));         // syngen line blank
	writeb(0x00, &(regs->reg[0x2744]));
	writeb(0xaf, &(regs->reg[0x2745]));         // syngen frame total
	writeb(0x05, &(regs->reg[0x2746]));
	writeb(0x03, &(regs->reg[0x2747]));         // syngen frame blank
	writeb(0x00, &(regs->reg[0x2748]));
	writeb(0x03, &(regs->reg[0x2749]));
	writeb(0x00, &(regs->reg[0x274a]));
	writeb(reg_276a, &(regs->reg[0x276a]));     // siggen enable(hvalidcnt): 75% still vertical color bar
	writeb(0x01, &(regs->reg[0x2705]));
#endif // #if 0 // Original
	switch (isp_info->probe)
	{
		case 0:
			ISPAPI_LOGI("ISP0 probe off\n");
			//writeb(0x00, &(regs->reg[0x21e9]));
			//writeb(0x00, &(regs->reg[0x21e8]));
			//writeb(0x0F, &(regs->reg[0x20e1]));
			break;

		case 1:
			ISPAPI_LOGI("ISP0 probe 1\n");
			writeb(0x01, &(regs->reg[0x21e9]));
			writeb(0x09, &(regs->reg[0x21e8]));
			writeb(0x01, &(regs->reg[0x20e1]));
			break;

		case 2:
			ISPAPI_LOGI("ISP0 probe 2\n");
			writeb(0x17, &(regs->reg[0x21e9]));
			writeb(0x00, &(regs->reg[0x21e8]));
			writeb(0x01, &(regs->reg[0x20e1]));
			break;
	}

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable
*/
static void cdspSetTable_yuv422(void)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@sensorInit
*/
static void sensorInit_yuv422(void)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	/* ISP1 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB1_REG8(0x2660) = 0x01;
	//ISPAPB1_REG8(0x2660) = 0x00;

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@CdspInit
*/
static void CdspInit_yuv422(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* ISP0 Configuration */
	// clock setting
	writeb(0x07, &(regs->reg[0x2008]));
	writeb(0x05, &(regs->reg[0x275e]));
	writeb(0x01, &(regs->reg[0x275b]));
	//
	// CDSP register setting 
	writeb(0x00, &(regs->reg[0x2106]));         // pixel and line switch
	writeb(0x20, &(regs->reg[0x2107]));
	writeb(0x03, &(regs->reg[0x2108]));         // enable manual OB
	writeb(0x00, &(regs->reg[0x2109]));
	writeb(0x00, &(regs->reg[0x210d]));         // vertical mirror line: original
	writeb(0x00, &(regs->reg[0x210e]));         // double buffer be set immediately
	writeb(0x00, &(regs->reg[0x210f]));         // without sync vd
	writeb(0xe9, &(regs->reg[0x2110]));         // enable global gain, LchStep=32, LcvStep=64
	writeb(0x01, &(regs->reg[0x2111]));         // LcHinc
	writeb(0x01, &(regs->reg[0x2112]));         // LcVinc
	writeb(0x42, &(regs->reg[0x2113]));         // LC Rgain
	writeb(0x34, &(regs->reg[0x2114]));         // LC Ggain
	writeb(0x3c, &(regs->reg[0x2115]));         // LC Bgain
	writeb(0x08, &(regs->reg[0x2116]));         // LC Xoffset 
	writeb(0x00, &(regs->reg[0x2117]));
	writeb(0x18, &(regs->reg[0x2118]));         // LC Yoffset 
	writeb(0x00, &(regs->reg[0x2119]));
	writeb(0x89, &(regs->reg[0x211a]));         // Centvoffset=8 Centhoffset=9
	writeb(0x9a, &(regs->reg[0x211b]));         // Centvsize=9 Centhsize=10
	writeb(0x32, &(regs->reg[0x211c]));         // Quan_n=3 Quan_m=2
	writeb(0x89, &(regs->reg[0x211d]));         // rbactthr
	writeb(0x04, &(regs->reg[0x211e]));         // low bad pixel threshold 
	writeb(0xf7, &(regs->reg[0x211f]));         // high bad pixel threshold 
	writeb(0x03, &(regs->reg[0x2120]));         // enable bad pixel
	writeb(0x01, &(regs->reg[0x2121]));         // enable bad pixel replacement
	writeb(0x00, &(regs->reg[0x2124]));         // HdrmapMode=0
	writeb(0x44, &(regs->reg[0x2125]));         // enable HDR saturation
	writeb(0xf0, &(regs->reg[0x2126]));         // disable HDR
	writeb(0x77, &(regs->reg[0x2127]));         // HdrFac1: 5 and HdrFac2: 6
	writeb(0x77, &(regs->reg[0x2128]));         // HdrFac3: 8 and HdrFac4: 7
	writeb(0x77, &(regs->reg[0x2129]));         // HdrFac5: 9 and HdrFac6: 8
	writeb(0x00, &(regs->reg[0x212a]));         // HdrGain0: (34/64)
	writeb(0x00, &(regs->reg[0x212b]));         // HdrGain1: (51/64)
	writeb(0x00, &(regs->reg[0x212c]));         // HdrGain2: (68/64)
	writeb(0x00, &(regs->reg[0x212d]));         // HdrGain3: (16/64)
	writeb(0x00, &(regs->reg[0x212e]));         // HdrGain4: (02/64)
	writeb(0x00, &(regs->reg[0x212f]));         // HdrGain5: (01/64)
	writeb(0x40, &(regs->reg[0x2130]));         // R WB gain: 
	writeb(0x00, &(regs->reg[0x2131]));
	writeb(0x00, &(regs->reg[0x2132]));         // R WB offset:
	writeb(0x40, &(regs->reg[0x2134]));         // Gr WB gain:
	writeb(0x00, &(regs->reg[0x2135]));
	writeb(0x00, &(regs->reg[0x2136]));         // Gr WB offset:
	writeb(0x40, &(regs->reg[0x2138]));         // B WB gain: 
	writeb(0x00, &(regs->reg[0x2139]));
	writeb(0x00, &(regs->reg[0x213a]));         // B WB offset:
	writeb(0x40, &(regs->reg[0x213c]));         // Gb WB gain:
	writeb(0x00, &(regs->reg[0x213d]));
	writeb(0x00, &(regs->reg[0x213e]));         // Gb WB offset:
	writeb(0x70, &(regs->reg[0x213f]));         // WB enable
	writeb(0x13, &(regs->reg[0x2140]));         // disable mask
	writeb(0x57, &(regs->reg[0x2141]));         // H/V edge threshold
	writeb(0x06, &(regs->reg[0x2142]));         // ir4x4 type 
	writeb(0x11, &(regs->reg[0x2143]));         // Fpncmpen
	writeb(0x13, &(regs->reg[0x2144]));         // FPNgain 
	writeb(0x02, &(regs->reg[0x2145]));         // FPNhoffset 
	writeb(0x00, &(regs->reg[0x2146]));         // 
	writeb(0x05, &(regs->reg[0x2147]));         // irfacY
	writeb(0x46, &(regs->reg[0x2148]));         // color matrix setting 
	writeb(0x00, &(regs->reg[0x2149]));
	writeb(0xfc, &(regs->reg[0x214a]));
	writeb(0x01, &(regs->reg[0x214b]));
	writeb(0xfe, &(regs->reg[0x214c]));
	writeb(0x01, &(regs->reg[0x214d]));
	writeb(0xf7, &(regs->reg[0x214e]));
	writeb(0x01, &(regs->reg[0x214f]));
	writeb(0x4c, &(regs->reg[0x2150]));
	writeb(0x00, &(regs->reg[0x2151]));
	writeb(0xfd, &(regs->reg[0x2152]));
	writeb(0x01, &(regs->reg[0x2153]));
	writeb(0x00, &(regs->reg[0x2154]));
	writeb(0x00, &(regs->reg[0x2155]));
	writeb(0xf1, &(regs->reg[0x2156]));
	writeb(0x01, &(regs->reg[0x2157]));
	writeb(0x4f, &(regs->reg[0x2158]));
	writeb(0x00, &(regs->reg[0x2159]));
	writeb(0x01, &(regs->reg[0x215c]));         // enable post gamma 
	writeb(0x01, &(regs->reg[0x215d]));         // enable dither
	writeb(0x11, &(regs->reg[0x215e]));         // enable Y LUT
	writeb(0x0a, &(regs->reg[0x2160]));         // Y LUT LPF step 
	writeb(0x10, &(regs->reg[0x2161]));         // Y LUT value
	writeb(0x20, &(regs->reg[0x2162]));
	writeb(0x30, &(regs->reg[0x2163]));
	writeb(0x40, &(regs->reg[0x2164]));
	writeb(0x50, &(regs->reg[0x2165]));
	writeb(0x60, &(regs->reg[0x2166]));
	writeb(0x70, &(regs->reg[0x2167]));
	writeb(0x80, &(regs->reg[0x2168]));
	writeb(0x90, &(regs->reg[0x2169]));
	writeb(0xa0, &(regs->reg[0x216a]));
	writeb(0xb0, &(regs->reg[0x216b]));
	writeb(0xc0, &(regs->reg[0x216c]));
	writeb(0xd0, &(regs->reg[0x216d]));
	writeb(0xe0, &(regs->reg[0x216e]));
	writeb(0xf0, &(regs->reg[0x216f]));
	writeb(0x13, &(regs->reg[0x2170]));         // YUVSPF setting 
	writeb(0x7f, &(regs->reg[0x2171]));         // enable Y 5x5 edge 
	writeb(0x21, &(regs->reg[0x2172]));         // enable UV SRAM replace mode
	writeb(0x73, &(regs->reg[0x2173]));         // enable Y SRAM replace
	writeb(0x77, &(regs->reg[0x2174]));         // enable all Y 5x5 edge fusion
	writeb(0x04, &(regs->reg[0x2175]));         // YFuLThr 
	writeb(0x08, &(regs->reg[0x2176]));         // YFuHThr 
	writeb(0x02, &(regs->reg[0x2177]));         // YEtrLMin
	writeb(0x01, &(regs->reg[0x2178]));         // YEtrLMax
	writeb(0x51, &(regs->reg[0x2179]));         // enable YHdn and factor: 256
	writeb(0x55, &(regs->reg[0x217a]));         // YHdn position initialization
	writeb(0x00, &(regs->reg[0x217b]));
	writeb(0xf2, &(regs->reg[0x217c]));         // YHdn right boundary
	writeb(0x06, &(regs->reg[0x217d]));
	writeb(0x0c, &(regs->reg[0x217e]));         // YHdn low threshold
	writeb(0x18, &(regs->reg[0x217f]));         // YHdn high threshold
	writeb(0x36, &(regs->reg[0x2180]));         // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1
	writeb(0x90, &(regs->reg[0x2181]));         // Fe00 and Fe01
	writeb(0x99, &(regs->reg[0x2182]));         // Fe02 and Fe11
	writeb(0x52, &(regs->reg[0x2183]));         // Fe12 and Fe22
	writeb(0x02, &(regs->reg[0x2184]));         // Fea
	writeb(0x20, &(regs->reg[0x2185]));         // Y 5x5 edge gain
	writeb(0x24, &(regs->reg[0x2186]));
	writeb(0x2a, &(regs->reg[0x2187]));
	writeb(0x24, &(regs->reg[0x2188]));
	writeb(0x22, &(regs->reg[0x2189]));
	writeb(0x1e, &(regs->reg[0x218a]));
	writeb(0x08, &(regs->reg[0x218b]));         // Y 5x5 edge threshold 
	writeb(0x10, &(regs->reg[0x218c]));
	writeb(0x18, &(regs->reg[0x218d]));
	writeb(0x20, &(regs->reg[0x218e]));
	writeb(0x40, &(regs->reg[0x218f]));
	writeb(0x02, &(regs->reg[0x2190]));         // Y 5x5 edge coring value 
	writeb(0x03, &(regs->reg[0x2191]));
	writeb(0x04, &(regs->reg[0x2192]));
	writeb(0x05, &(regs->reg[0x2193]));
	writeb(0x05, &(regs->reg[0x2194]));
	writeb(0x05, &(regs->reg[0x2195]));
	writeb(0x20, &(regs->reg[0x2196]));         // Y 5x5 edge coring threshold
	writeb(0x40, &(regs->reg[0x2197]));
	writeb(0x80, &(regs->reg[0x2198]));
	writeb(0xa0, &(regs->reg[0x2199]));
	writeb(0xc0, &(regs->reg[0x219a]));
	writeb(0x08, &(regs->reg[0x219b]));         // PEdgeThr
	writeb(0x10, &(regs->reg[0x219c]));         // NEdgeThr
	writeb(0x64, &(regs->reg[0x219d]));         // PEdgeSkR and NEdgeSkR
	writeb(0x08, &(regs->reg[0x219e]));         // Ysr Sobel filter threshold 
	writeb(0x04, &(regs->reg[0x219f]));         // Ysr weight 
	writeb(0x00, &(regs->reg[0x21b8]));         // disable H/V scale down
	writeb(0x03, &(regs->reg[0x21ba]));         // 2D YUV scaling before YUV spf 
	writeb(0x03, &(regs->reg[0x21c0]));         // enable bchs/scale 
	writeb(0x00, &(regs->reg[0x21c1]));         // Y brightness
	writeb(0x1c, &(regs->reg[0x21c2]));         // Y contrast 
	writeb(0x0f, &(regs->reg[0x21c3]));         // Hue SIN data
	writeb(0x3e, &(regs->reg[0x21c4]));         // Hue COS data
	writeb(0x28, &(regs->reg[0x21c5]));         // Hue saturation 
	writeb(0x05, &(regs->reg[0x21c6]));         // Y offset
	writeb(0x80, &(regs->reg[0x21c7]));         // Y center
	writeb(0x3a, &(regs->reg[0x21ce]));         // LC Gn gain 
	writeb(0x00, &(regs->reg[0x21d1]));         // use CDSP
	writeb(0x00, &(regs->reg[0x21d2]));         // YUV422
	writeb(0x70, &(regs->reg[0x3100]));         // enable new WB offset/gain and offset after gain
	writeb(0x03, &(regs->reg[0x3101]));         // R new WB offset: 5
	writeb(0x01, &(regs->reg[0x3102]));         // G new WB offset: 84
	writeb(0x02, &(regs->reg[0x3103]));         // B new WB offset: 16
	writeb(0x3f, &(regs->reg[0x3104]));         // R new WB gain: 22 
	writeb(0x00, &(regs->reg[0x3105]));
	writeb(0x3d, &(regs->reg[0x3106]));         // G new WB gain: 0
	writeb(0x00, &(regs->reg[0x3107]));
	writeb(0x44, &(regs->reg[0x3108]));         // B new WB gain: 56 
	writeb(0x00, &(regs->reg[0x3109]));
	writeb(0x22, &(regs->reg[0x317a]));         // enable rgbedgeen mul 2 div 1 enable rgbmode
	writeb(0x0a, &(regs->reg[0x317b]));         // rgbedgelothr
	writeb(0x28, &(regs->reg[0x317c]));         // rgbedgehithr
	writeb(0x03, &(regs->reg[0x31af]));         // enable HDR H/V smoothing mode 
	writeb(0x10, &(regs->reg[0x31b0]));         // hdrsmlthr
	writeb(0x40, &(regs->reg[0x31b1]));         // hdrsmlmax
	writeb(0x00, &(regs->reg[0x31c0]));         // sYEdgeGainMode: 0 
	writeb(0x09, &(regs->reg[0x31c1]));         // sFe00 and sFe01
	writeb(0x14, &(regs->reg[0x31c2]));         // sFea and Fe11
	writeb(0x00, &(regs->reg[0x31c3]));         // Y 3x3 edge gain
	writeb(0x04, &(regs->reg[0x31c4]));
	writeb(0x0c, &(regs->reg[0x31c5]));
	writeb(0x12, &(regs->reg[0x31c6]));
	writeb(0x16, &(regs->reg[0x31c7]));
	writeb(0x18, &(regs->reg[0x31c8]));
	writeb(0x08, &(regs->reg[0x31c9]));         // Y 3x3 edge threshold 
	writeb(0x0c, &(regs->reg[0x31ca]));
	writeb(0x10, &(regs->reg[0x31cb]));
	writeb(0x18, &(regs->reg[0x31cc]));
	writeb(0x28, &(regs->reg[0x31cd]));
	writeb(0x03, &(regs->reg[0x31ce]));         // enable SpfBlkDatEn
	writeb(0x08, &(regs->reg[0x31cf]));         // Y77StdThr
	writeb(0x7d, &(regs->reg[0x31d0]));         // YDifMinSThr1:125
	writeb(0x6a, &(regs->reg[0x31d1]));         // YDifMinSThr2:106
	writeb(0x62, &(regs->reg[0x31d2]));         // YDifMinBThr1:98
	writeb(0x4d, &(regs->reg[0x31d3]));         // YDifMinBThr2:77
	writeb(0x12, &(regs->reg[0x31d4]));         // YStdLDiv and YStdMDiv
	writeb(0x00, &(regs->reg[0x31d5]));         // YStdHDiv
	writeb(0x78, &(regs->reg[0x31d6]));         // YStdMThr
	writeb(0x64, &(regs->reg[0x31d7]));         // YStdMMean
	writeb(0x82, &(regs->reg[0x31d8]));         // YStdHThr
	writeb(0x8c, &(regs->reg[0x31d9]));         // YStdHMean
	writeb(0x24, &(regs->reg[0x31f0]));         // YLowSel 
	writeb(0x04, &(regs->reg[0x31f3]));         // YEtrMMin
	writeb(0x03, &(regs->reg[0x31f4]));         // YEtrMMax
	writeb(0x06, &(regs->reg[0x31f5]));         // YEtrHMin
	writeb(0x05, &(regs->reg[0x31f6]));         // YEtrHMax
	writeb(0x01, &(regs->reg[0x31f8]));         // enbale Irhigtmin
	writeb(0x06, &(regs->reg[0x31f9]));         // enbale Irhigtmin_r
	writeb(0x09, &(regs->reg[0x31fa]));         // enbale Irhigtmin_b
	writeb(0xf8, &(regs->reg[0x31fb]));         // Irhighthr
	writeb(0xff, &(regs->reg[0x31fc]));         // Irhratio_r 
	writeb(0xf8, &(regs->reg[0x31fd]));         // Irhratio_b 
	writeb(0x0a, &(regs->reg[0x31fe]));         // UVThr1
	writeb(0x14, &(regs->reg[0x31ff]));         // UVThr2

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

static void ispReset(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

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

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

static void FrontInit(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

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

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

static void cdspSetTable(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

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

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

static void sensorInit(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

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

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

static void CdspInit(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

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

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

void isp_test_setting(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);
	ISPAPI_LOGI("isp_mode: %d, test_pattern: %d\n", isp_info->isp_mode, isp_info->test_pattern);

	ISPAPI_LOGD("mipi_isp_regs: 0x%px, regs: 0x%px\n", isp_info->mipi_isp_regs, regs);

	if (isp_info->isp_mode == ISP_NORMAL_MODE)
		ISPAPI_LOGI("MIPI ISP is normal mode.\n");
	else
		ISPAPI_LOGI("MIPI ISP is test mode.\n");

	switch (isp_info->input_fmt)
	{
		case YUV422_FORMAT:
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ISPAPI_LOGI("ISP input format is YUV422\n");
			ISPAPI_LOGI("Image size to %dx%d\n", isp_info->width, isp_info->height);

			ispReset(isp_info); 
			writeb(0x01, &(regs->reg[0x21d0]));         // sofware reset CDSP interface (active)
			cdspSetTable(isp_info);
			CdspInit(isp_info);
			FrontInit(isp_info);
			sensorInit(isp_info);
			//
			writeb(0x00, &(regs->reg[0x21d0]));         // sofware reset CDSP interface (inactive)
			writeb(0x01, &(regs->reg[0x276c]));         // sofware reset Front interface
			writeb(0x00, &(regs->reg[0x276c]));

			//
			readb(&(regs->reg[0x21d4]));                // 0x21d4, 0x80
			readb(&(regs->reg[0x21d5]));                // 0x21d5, 0x07
			readb(&(regs->reg[0x21d6]));                // 0x21d6, 0x38
			readb(&(regs->reg[0x21d7]));                // 0x21d7, 0x04
			break;

		case RAW10_FORMAT:
			ISPAPI_LOGI("ISP input format is RAW10\n");
			ISPAPI_LOGI("Image size to %dx%d\n", isp_info->width, isp_info->height);

			ispReset(isp_info);
			writeb(0x01, &(regs->reg[0x21d0]));         // sofware reset CDSP interface (active)
			cdspSetTable(isp_info);
			FrontInit(isp_info);
			CdspInit(isp_info);
			sensorInit(isp_info);
			//
			writeb(0x00, &(regs->reg[0x2b00]));         // siggen enable(hvalidcnt): 75% still vertical color bar
			writeb(0x00, &(regs->reg[0x21d0]));         // sofware reset CDSP interface (inactive)
			//
			readb(&(regs->reg[0x21d4]));                // 0x80
			readb(&(regs->reg[0x21d5]));                // 0x07
			readb(&(regs->reg[0x21d6]));                // 0x38
			readb(&(regs->reg[0x21d7]));                // 0x04
			break;
	}
	
	ISPAPI_LOGI("0x21d4:0x%02x, 0x21d5:0x%02x, 0x21d6:0x%02x, 0x21d7:0x%02x\n",
		readb(&(regs->reg[0x21d4])), readb(&(regs->reg[0x21d5])),
		readb(&(regs->reg[0x21d6])), readb(&(regs->reg[0x21d7])));


	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}
