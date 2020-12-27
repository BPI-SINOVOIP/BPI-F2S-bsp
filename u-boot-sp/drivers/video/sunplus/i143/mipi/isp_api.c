#include <common.h>

#include "isp_api.h"
#include "isp_api_s.h"

#define PATTERN_64X64_TEST      0   // output 64x64 size of color bar raw10 1920x1080
#define INTERRUPT_VS_FALLING    1   // Test V-sync falling edge count equal event interrupt


// Load settings table
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
void ispSleep(void)
{
	int i;

	for (i = 0;i < 10; i++) {
		MOON0_REG->clken[2];
	}
}

/*
	@ispReset
*/
void ispReset_raw10(void)
{
	/* ISP0 Configuration */
	ISPAPB0_REG8(0x2000) = 0x13; // reset all module include ispck
	ISPAPB0_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB0_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB0_REG8(0x2008) = 0x05; // switch b1xck/bpclk_nx to normal clocks
	ISPAPB0_REG8(0x2000) = 0x03; // release ispck reset
	ispSleep();//#(`TIMEBASE*20;
	//
	ISPAPB0_REG8(0x2000) = 0x00; // release all module reset
	//
	ISPAPB0_REG8(0x276c) = 0x01; // reset front
	ISPAPB0_REG8(0x276c) = 0x00; //
	//	
	ISPAPB0_REG8(0x2000) = 0x03;
	ISPAPB0_REG8(0x2000) = 0x00;
	//
	ISPAPB0_REG8(0x2010) = 0x00; // cclk: 48MHz

	/* ISP1 Configuration */
	ISPAPB1_REG8(0x2000) = 0x13; // reset all module include ispck
	ISPAPB1_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB1_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB1_REG8(0x2008) = 0x05; // switch b1xck/bpclk_nx to normal clocks
	ISPAPB1_REG8(0x2000) = 0x03; // release ispck reset
	ispSleep();//#(`TIMEBASE*20;
	//
	ISPAPB1_REG8(0x2000) = 0x00; // release all module reset
	//
	ISPAPB1_REG8(0x276c) = 0x01; // reset front
	ISPAPB1_REG8(0x276c) = 0x00; //
	//	
	ISPAPB1_REG8(0x2000) = 0x03;
	ISPAPB1_REG8(0x2000) = 0x00;
	//
	ISPAPB1_REG8(0x2010) = 0x00; // cclk: 48MHz
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/
void FrontInit_raw10(u16 width, u16 height,u8 sigmode, u8 probe)
{
	/* ISP0 Configuration */
	//clock setting
	ISPAPB0_REG8(0x2008) = 0x07;
	ISPAPB0_REG8(0x275e) = 0x05;
	ISPAPB0_REG8(0x275b) = 0x01;
	//
#if (PATTERN_64X64_TEST == 1)
	ISPAPB0_REG8(0x2711) = 0x30; // hfall
	ISPAPB0_REG8(0x2712) = 0x00;
	ISPAPB0_REG8(0x2713) = 0x50; // hrise
	ISPAPB0_REG8(0x2714) = 0x00;
	ISPAPB0_REG8(0x2715) = 0x00; // vfall
	ISPAPB0_REG8(0x2716) = 0x01;
	ISPAPB0_REG8(0x2717) = 0x90; // vrise
	ISPAPB0_REG8(0x2718) = 0x01;
#else
	ISPAPB0_REG8(0x2711) = 0x30; // hfall
	ISPAPB0_REG8(0x2712) = 0x02;
	ISPAPB0_REG8(0x2713) = 0x50; // hrise
	ISPAPB0_REG8(0x2714) = 0x02;
	ISPAPB0_REG8(0x2715) = 0x00; // vfall
	ISPAPB0_REG8(0x2716) = 0x0a;
	ISPAPB0_REG8(0x2717) = 0x90; // vrise
	ISPAPB0_REG8(0x2718) = 0x0a;
#endif
	ISPAPB0_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB0_REG8(0x2720) = 0x05; // hoffset
	ISPAPB0_REG8(0x2721) = 0x00;
	ISPAPB0_REG8(0x2722) = 0x03; // voffset
	ISPAPB0_REG8(0x2723) = 0x00; // 0x05; Fixed by Steve
	ISPAPB0_REG8(0x2724) = (width&0xff);//0x80; //hsize
	ISPAPB0_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB0_REG8(0x2726) = (height&0xff);//0x38; //vsize
	ISPAPB0_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB0_REG8(0x2728) = 0x40;
	ISPAPB0_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB0_REG8(0x2741) = 0xff; // syngen line total
#if (PATTERN_64X64_TEST == 1)
	ISPAPB0_REG8(0x2742) = 0x04;
	ISPAPB0_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB0_REG8(0x2744) = 0x02;
	ISPAPB0_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB0_REG8(0x2746) = 0x04;
	ISPAPB0_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB0_REG8(0x2748) = 0x00;
#else
	ISPAPB0_REG8(0x2742) = 0x0f;
	ISPAPB0_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB0_REG8(0x2744) = 0x08;
	ISPAPB0_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB0_REG8(0x2746) = 0x0f;
	ISPAPB0_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB0_REG8(0x2748) = 0x05;
#endif
	ISPAPB0_REG8(0x2749) = 0x03;
	ISPAPB0_REG8(0x274a) = 0x00;
	ISPAPB0_REG8(0x276a) = sigmode; // siggen enable(hvalidcnt): 75% still vertical color bar
#if (INTERRUPT_VS_FALLING == 1)
	ISPAPB0_REG8(0x27b4) = 0x10; // Define how many V-sync falling edge to trigger interrupt
	ISPAPB0_REG8(0x27c0) = 0x02; // Enable V-sync falling edge count equal event interrupt
#endif
	switch (probe)
	{
		case 0:
			ISPAPB_LOGI("ISP0 probe off\n");
			//ISPAPB0_REG8(0x21e9) = 0x00;
			//ISPAPB0_REG8(0x21e8) = 0x00;
			//ISPAPB0_REG8(0x20e1) = 0x0F;
			break;

		case 1:
			ISPAPB_LOGI("ISP0 probe 1\n");
			ISPAPB0_REG8(0x21e9) = 0x01;
			ISPAPB0_REG8(0x21e8) = 0x09;
			ISPAPB0_REG8(0x20e1) = 0x01;
			break;

		case 2:
			ISPAPB_LOGI("ISP0 probe 2\n");
			ISPAPB0_REG8(0x21e9) = 0x17;
			ISPAPB0_REG8(0x21e8) = 0x00;
			ISPAPB0_REG8(0x20e1) = 0x01;
			break;
	}
	//
	// set and clear of front sensor interface
	ISPAPB0_REG8(0x276C) = 0x01;
	ISPAPB0_REG8(0x276C) = 0x00;

	/* ISP1 Configuration */
	//clock setting
	ISPAPB1_REG8(0x2008) = 0x07;
	ISPAPB1_REG8(0x275e) = 0x05;
	ISPAPB1_REG8(0x275b) = 0x01;
	//
#if (PATTERN_64X64_TEST == 1)
	ISPAPB1_REG8(0x2711) = 0x30; // hfall
	ISPAPB1_REG8(0x2712) = 0x00;
	ISPAPB1_REG8(0x2713) = 0x50; // hrise
	ISPAPB1_REG8(0x2714) = 0x00;
	ISPAPB1_REG8(0x2715) = 0x00; // vfall
	ISPAPB1_REG8(0x2716) = 0x01;
	ISPAPB1_REG8(0x2717) = 0x90; // vrise
	ISPAPB1_REG8(0x2718) = 0x01;
#else
	ISPAPB1_REG8(0x2711) = 0x30; // hfall
	ISPAPB1_REG8(0x2712) = 0x02;
	ISPAPB1_REG8(0x2713) = 0x50; // hrise
	ISPAPB1_REG8(0x2714) = 0x02;
	ISPAPB1_REG8(0x2715) = 0x00; // vfall
	ISPAPB1_REG8(0x2716) = 0x0a;
	ISPAPB1_REG8(0x2717) = 0x90; // vrise
	ISPAPB1_REG8(0x2718) = 0x0a;
#endif
	ISPAPB1_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB1_REG8(0x2720) = 0x05; // hoffset
	ISPAPB1_REG8(0x2721) = 0x00;
	ISPAPB1_REG8(0x2722) = 0x03; // voffset
	ISPAPB1_REG8(0x2723) = 0x00; // 0x05; Fixed by Steve
	ISPAPB1_REG8(0x2724) = (width&0xff);//0x80; //hsize
	ISPAPB1_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB1_REG8(0x2726) = (height&0xff);//0x38; //vsize
	ISPAPB1_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB1_REG8(0x2728) = 0x40;
	ISPAPB1_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB1_REG8(0x2741) = 0xff; // syngen line total
#if (PATTERN_64X64_TEST == 1)
	ISPAPB1_REG8(0x2742) = 0x04;
	ISPAPB1_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB1_REG8(0x2744) = 0x02;
	ISPAPB1_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB1_REG8(0x2746) = 0x04;
	ISPAPB1_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB1_REG8(0x2748) = 0x00;
#else
	ISPAPB1_REG8(0x2742) = 0x0f;
	ISPAPB1_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB1_REG8(0x2744) = 0x08;
	ISPAPB1_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB1_REG8(0x2746) = 0x0f;
	ISPAPB1_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB1_REG8(0x2748) = 0x05;
#endif
	ISPAPB1_REG8(0x2749) = 0x03;
	ISPAPB1_REG8(0x274a) = 0x00;
	ISPAPB1_REG8(0x276a) = sigmode; // siggen enable(hvalidcnt): 75% still vertical color bar
#if (INTERRUPT_VS_FALLING == 1)
	ISPAPB1_REG8(0x27b4) = 0x10; // Define how many V-sync falling edge to trigger interrupt
	ISPAPB1_REG8(0x27c0) = 0x02; // Enable V-sync falling edge count equal event interrupt
#endif
	switch (probe)
	{
		case 0:
			ISPAPB_LOGI("ISP1 probe off\n");
			//ISPAPB1_REG8(0x21e9) = 0x00;
			//ISPAPB1_REG8(0x21e8) = 0x00;
			//ISPAPB1_REG8(0x20e1) = 0x0F;
			break;

		case 1:
			ISPAPB_LOGI("ISP1 probe 1\n");
			ISPAPB1_REG8(0x21e9) = 0x01;
			ISPAPB1_REG8(0x21e8) = 0x09;
			ISPAPB1_REG8(0x20e1) = 0x01;
			break;

		case 2:
			ISPAPB_LOGI("ISP1 probe 2\n");
			ISPAPB1_REG8(0x21e9) = 0x17;
			ISPAPB1_REG8(0x21e8) = 0x00;
			ISPAPB1_REG8(0x20e1) = 0x01;
			break;
	}
	//
	// set and clear of front sensor interface
	ISPAPB1_REG8(0x276C) = 0x01;
	ISPAPB1_REG8(0x276C) = 0x00;
}

/*
	@cdspSetTable
*/
void cdspSetTable_raw10(void)
{
	int i;

	/* ISP0 Configuration */
	ISPAPB0_REG8(0x2008) = 0x00; // use memory clock for pixel clock, master clock and mipi decoder clock
	// R table of lens compensation tables
	ISPAPB0_REG8(0x2101) = 0x00; // select lens compensation R SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0	
	for (i = 0; i < 768; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_LENS_COMP_R[i];
	}
	//
	// G/Gr table of lens compensation tables
	ISPAPB0_REG8(0x2101) = 0x01; // select lens compensation G/Gr SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_LENS_COMP_G[i];
	}
	//
	// B table of lens compensation tables
	ISPAPB0_REG8(0x2101) = 0x02; // select lens compensation B SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_LENS_COMP_B[i];
	}
	//
	/* write post gamma tables */
	// R table of post gamma tables
	ISPAPB0_REG8(0x2101) = 0x04; // select post gamma R SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_R[i];
	}
	//
	// G table of post gamma tables
	ISPAPB0_REG8(0x2101) = 0x05; // select post gamma G SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_G[i];
	}
	//
	// B table of of post gamma tables
	ISPAPB0_REG8(0x2101) = 0x06; // select post gamma B SRAM
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_B[i];
	}
	//
	//  fixed pattern noise tables
	ISPAPB0_REG8(0x2101) = 0x0D; // select fixed pattern noise
	ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 1952; i++)
	{
		ISPAPB0_REG8(0x2103) = SF_FIXED_PATTERN_NOISE[i];
	}
	// disable set cdsp sram
	ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0 
	ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0 
	ISPAPB0_REG8(0x2100) = 0x00; // disable CPU access macro and adress auto increase 

	/* ISP1 Configuration */
	ISPAPB1_REG8(0x2008) = 0x00; // use memory clock for pixel clock, master clock and mipi decoder clock
	// R table of lens compensation tables
	ISPAPB1_REG8(0x2101) = 0x00; // select lens compensation R SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0	
	for (i = 0; i < 768; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_LENS_COMP_R[i];
	}
	//
	// G/Gr table of lens compensation tables
	ISPAPB1_REG8(0x2101) = 0x01; // select lens compensation G/Gr SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_LENS_COMP_G[i];
	}
	//
	// B table of lens compensation tables
	ISPAPB1_REG8(0x2101) = 0x02; // select lens compensation B SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_LENS_COMP_B[i];
	}
	//
	/* write post gamma tables */
	// R table of post gamma tables
	ISPAPB1_REG8(0x2101) = 0x04; // select post gamma R SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_R[i];
	}
	//
	// G table of post gamma tables
	ISPAPB1_REG8(0x2101) = 0x05; // select post gamma G SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_G[i];
	}
	//
	// B table of of post gamma tables
	ISPAPB1_REG8(0x2101) = 0x06; // select post gamma B SRAM
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_B[i];
	}
	//
	//  fixed pattern noise tables
	ISPAPB1_REG8(0x2101) = 0x0D; // select fixed pattern noise
	ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
	for (i = 0; i < 1952; i++)
	{
		ISPAPB1_REG8(0x2103) = SF_FIXED_PATTERN_NOISE[i];
	}
	// disable set cdsp sram
	ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0 
	ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0 
	ISPAPB1_REG8(0x2100) = 0x00; // disable CPU access macro and adress auto increase 
}

/*
	@sensorInit
*/
void sensorInit_raw10(void)
{
	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	/* ISP1 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB1_REG8(0x2660) = 0x01;
	//ISPAPB1_REG8(0x2660) = 0x00;
}

/*
	@CdspInit
*/
void CdspInit_raw10(u8 output, u8 scale)
{
	u8 reg_21d1 = 0x00, reg_21d2 = 0x00, reg_2311 = 0x00;
	u8 reg_21b0 = 0x00, reg_21b1 = 0x00, reg_21b2 = 0x00, reg_21b3 = 0x00;
	u8 reg_21b4 = 0x00, reg_21b5 = 0x00, reg_21b6 = 0x00, reg_21b7 = 0x00;
	u8 reg_21b8 = 0x00;

	switch (output)
	{
		case YUV422_FORMAT_UYVY_ORDER:
		case YUV422_FORMAT_YUYV_ORDER:
			ISPAPB_LOGI("YUV422 format output\n");
			reg_21d1 = 0x00; // use CDSP
			reg_21d2 = 0x00; // YUV422
			reg_2311 = 0x00; // format:2bytes
			break;

		case RAW8_FORMAT:
			ISPAPB_LOGI("RAW8 format output\n");
			reg_21d1 = 0x01; // bypass CDSP
			reg_21d2 = 0x01; // raw8
			reg_2311 = 0x10; // format:1byte
			break;

		case RAW10_FORMAT:
			ISPAPB_LOGI("RAW10 format output\n");
			reg_21d1 = 0x01; // bypass CDSP
			reg_21d2 = 0x02; // raw10
			reg_2311 = 0x10; // format:2byte (like yuv format)
			break;
			
		case RAW10_FORMAT_PACK_MODE:
			ISPAPB_LOGI("RAW10 format pack mode output\n");
			reg_21d1 = 0x01; // bypass CDSP
			reg_21d2 = 0x02; // raw10
			reg_2311 = 0x00; // format:10bits pack mode
			break;
	}

	switch (scale)
	{
		case SCALE_DOWN_OFF:
			ISPAPB_LOGI("Scaler is off\n");
			reg_21b0 = 0x00; // factor for Hsize
			reg_21b1 = 0x00; //
			reg_21b2 = 0x00; // factor for Vsize
			reg_21b3 = 0x00; //
			//
			reg_21b4 = 0x00; // factor for Hsize
			reg_21b5 = 0x00; //
			reg_21b6 = 0x00; // factor for Vsize
			reg_21b7 = 0x00; //
			//
			reg_21b8 = 0x00; // disable H/V scale down
			break;

		case SCALE_DOWN_FHD_HD:
			ISPAPB_LOGI("Scale down from FHD to HD size\n");
			// H = 1280 * 65536 / 1920 = 0xAAAB
			// V =  720 * 65536 / 1080 = 0xAAAB
			reg_21b0 = 0xAB; // factor for Hsize
			reg_21b1 = 0xAA; //
			reg_21b2 = 0xAB; // factor for Vsize
			reg_21b3 = 0xAA; //
			//
			reg_21b4 = 0xAB; // factor for Hsize
			reg_21b5 = 0xAA; //
			reg_21b6 = 0xAB; // factor for Vsize
			reg_21b7 = 0xAA; //
			//
			reg_21b8 = 0x2F; // enable H/V scale down
			break;

		case SCALE_DOWN_FHD_WVGA:
			ISPAPB_LOGI("Scale down from FHD to WVGA size\n");
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
			ISPAPB_LOGI("Scale down from FHD to VGA size\n");
			// H = 640 * 65536 / 1920 = 0x5556
			// V = 480 * 65536 / 1080 = 0x71C8
			reg_21b0 = 0x56; // factor for Hsize
			reg_21b1 = 0x55; //
			reg_21b2 = 0xC8; // factor for Vsize
			reg_21b3 = 0x71; //
			//
			reg_21b4 = 0x56; // factor for Hsize
			reg_21b5 = 0x55; //
			reg_21b6 = 0xC8; // factor for Vsize
			reg_21b7 = 0x71; //
			//
			reg_21b8 = 0x2F; // enable H/V scale down
			break;
			
		case SCALE_DOWN_FHD_QQVGA:
			ISPAPB_LOGI("Scale down from FHD to QQVGA size\n");
			// H = 160 * 65536 / 1920 = 0x1556
			// V = 120 * 65536 / 1080 = 0x1C72
			reg_21b0 = 0x56; // factor for Hsize
			reg_21b1 = 0x15; //
			reg_21b2 = 0x72; // factor for Vsize
			reg_21b3 = 0x1C; //
			//
			reg_21b4 = 0x56; // factor for Hsize
			reg_21b5 = 0x15; //
			reg_21b6 = 0x72; // factor for Vsize
			reg_21b7 = 0x1C; //
			//
			reg_21b8 = 0x2F; // enable H/V scale down
			break;
	}


	/* ISP0 Configuration */
	// clock setting
	ISPAPB0_REG8(0x2008) = 0x07;
	ISPAPB0_REG8(0x275e) = 0x05;
	ISPAPB0_REG8(0x275b) = 0x01;
	//
	// CDSP register setting 
	ISPAPB0_REG8(0x2106) = 0x00; // pixel and line switch
	ISPAPB0_REG8(0x2107) = 0x20;
	ISPAPB0_REG8(0x2108) = 0x03; // enable manual OB 
	ISPAPB0_REG8(0x2109) = 0x00;
	ISPAPB0_REG8(0x210d) = 0x00; // vertical mirror line: original
	ISPAPB0_REG8(0x210e) = 0x00; // double buffer be set immediately
	ISPAPB0_REG8(0x210f) = 0x00; // without sync vd  
	ISPAPB0_REG8(0x2110) = 0xe9; // enable global gain, LchStep=32, LcvStep=64 
	ISPAPB0_REG8(0x2111) = 0x01; // LcHinc  
	ISPAPB0_REG8(0x2112) = 0x01; // LcVinc  
	ISPAPB0_REG8(0x2113) = 0x42; // LC Rgain
	ISPAPB0_REG8(0x2114) = 0x34; // LC Ggain
	ISPAPB0_REG8(0x2115) = 0x3c; // LC Bgain
	ISPAPB0_REG8(0x2116) = 0x08; // LC Xoffset
	ISPAPB0_REG8(0x2117) = 0x00;
	ISPAPB0_REG8(0x2118) = 0x18; // LC Yoffset
	ISPAPB0_REG8(0x2119) = 0x00;
	ISPAPB0_REG8(0x211a) = 0x89; // Centvoffset=8 Centhoffset=9   
	ISPAPB0_REG8(0x211b) = 0x9a; // Centvsize=9 Centhsize=10 
	ISPAPB0_REG8(0x211c) = 0x32; // Quan_n=3 Quan_m=2
	ISPAPB0_REG8(0x211d) = 0x89; // rbactthr
	ISPAPB0_REG8(0x211e) = 0x04; // low bad pixel threshold
	ISPAPB0_REG8(0x211f) = 0xf7; // high bad pixel threshold
	ISPAPB0_REG8(0x2120) = 0x03; // enable bad pixel 
	ISPAPB0_REG8(0x2121) = 0x01; // enable bad pixel replacement 
	ISPAPB0_REG8(0x2124) = 0x00; // HdrmapMode=0
	ISPAPB0_REG8(0x2125) = 0x44; // enable HDR saturation
	ISPAPB0_REG8(0x2126) = 0xf0; // disable HDR 
	ISPAPB0_REG8(0x2127) = 0x77; // HdrFac1: 5 and HdrFac2: 6
	ISPAPB0_REG8(0x2128) = 0x77; // HdrFac3: 8 and HdrFac4: 7
	ISPAPB0_REG8(0x2129) = 0x77; // HdrFac5: 9 and HdrFac6: 8
	ISPAPB0_REG8(0x212a) = 0x00; // HdrGain0: (34/64)
	ISPAPB0_REG8(0x212b) = 0x00; // HdrGain1: (51/64)
	ISPAPB0_REG8(0x212c) = 0x00; // HdrGain2: (68/64)
	ISPAPB0_REG8(0x212d) = 0x00; // HdrGain3: (16/64)
	ISPAPB0_REG8(0x212e) = 0x00; // HdrGain4: (02/64)
	ISPAPB0_REG8(0x212f) = 0x00; // HdrGain5: (01/64)
	ISPAPB0_REG8(0x2130) = 0x40; // R WB gain:
	ISPAPB0_REG8(0x2131) = 0x00;
	ISPAPB0_REG8(0x2132) = 0x00; // R WB offset:
	ISPAPB0_REG8(0x2134) = 0x40; // Gr WB gain: 
	ISPAPB0_REG8(0x2135) = 0x00;
	ISPAPB0_REG8(0x2136) = 0x00; // Gr WB offset:    
	ISPAPB0_REG8(0x2138) = 0x40; // B WB gain:
	ISPAPB0_REG8(0x2139) = 0x00;
	ISPAPB0_REG8(0x213a) = 0x00; // B WB offset:
	ISPAPB0_REG8(0x213c) = 0x40; // Gb WB gain: 
	ISPAPB0_REG8(0x213d) = 0x00;
	ISPAPB0_REG8(0x213e) = 0x00; // Gb WB offset:    
	ISPAPB0_REG8(0x213f) = 0x70; // WB enable  
	ISPAPB0_REG8(0x2140) = 0x13; // disable mask
	ISPAPB0_REG8(0x2141) = 0x57; // H/V edge threshold   
	ISPAPB0_REG8(0x2142) = 0x06; // ir4x4 type
	ISPAPB0_REG8(0x2143) = 0x11; // Fpncmpen
	ISPAPB0_REG8(0x2144) = 0x13; // FPNgain 
	ISPAPB0_REG8(0x2145) = 0x02; // FPNhoffset
	ISPAPB0_REG8(0x2146) = 0x00; // 
	ISPAPB0_REG8(0x2147) = 0x05; // irfacY  
	ISPAPB0_REG8(0x2148) = 0x46; // color matrix setting 
	ISPAPB0_REG8(0x2149) = 0x00;
	ISPAPB0_REG8(0x214a) = 0xfc;
	ISPAPB0_REG8(0x214b) = 0x01;
	ISPAPB0_REG8(0x214c) = 0xfe;
	ISPAPB0_REG8(0x214d) = 0x01;
	ISPAPB0_REG8(0x214e) = 0xf7;
	ISPAPB0_REG8(0x214f) = 0x01;
	ISPAPB0_REG8(0x2150) = 0x4c;
	ISPAPB0_REG8(0x2151) = 0x00;
	ISPAPB0_REG8(0x2152) = 0xfd;
	ISPAPB0_REG8(0x2153) = 0x01;
	ISPAPB0_REG8(0x2154) = 0x00;
	ISPAPB0_REG8(0x2155) = 0x00;
	ISPAPB0_REG8(0x2156) = 0xf1;
	ISPAPB0_REG8(0x2157) = 0x01;
	ISPAPB0_REG8(0x2158) = 0x4f;
	ISPAPB0_REG8(0x2159) = 0x00;
	ISPAPB0_REG8(0x215c) = 0x01; // enable post gamma
	ISPAPB0_REG8(0x215d) = 0x01; // enable dither    
	ISPAPB0_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB0_REG8(0x2160) = 0x0a; // Y LUT LPF step   
	ISPAPB0_REG8(0x2161) = 0x10; // Y LUT value 
	ISPAPB0_REG8(0x2162) = 0x20;
	ISPAPB0_REG8(0x2163) = 0x30;
	ISPAPB0_REG8(0x2164) = 0x40;
	ISPAPB0_REG8(0x2165) = 0x50;
	ISPAPB0_REG8(0x2166) = 0x60;
	ISPAPB0_REG8(0x2167) = 0x70;
	ISPAPB0_REG8(0x2168) = 0x80;
	ISPAPB0_REG8(0x2169) = 0x90;
	ISPAPB0_REG8(0x216a) = 0xa0;
	ISPAPB0_REG8(0x216b) = 0xb0;
	ISPAPB0_REG8(0x216c) = 0xc0;
	ISPAPB0_REG8(0x216d) = 0xd0;
	ISPAPB0_REG8(0x216e) = 0xe0;
	ISPAPB0_REG8(0x216f) = 0xf0;
	ISPAPB0_REG8(0x2170) = 0x13; // YUVSPF setting   
	ISPAPB0_REG8(0x2171) = 0x7f; // enable Y 5x5 edge
	ISPAPB0_REG8(0x2172) = 0x21; // enable UV SRAM replace mode   
	ISPAPB0_REG8(0x2173) = 0x73; // enable Y SRAM replace
	ISPAPB0_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion  
	ISPAPB0_REG8(0x2175) = 0x04; // YFuLThr 
	ISPAPB0_REG8(0x2176) = 0x08; // YFuHThr 
	ISPAPB0_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB0_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB0_REG8(0x2179) = 0x51; // enable YHdn and factor: 256   
	ISPAPB0_REG8(0x217a) = 0x55; // YHdn position initialization  
	ISPAPB0_REG8(0x217b) = 0x00;
	ISPAPB0_REG8(0x217c) = 0xf2; // YHdn right boundary  
	ISPAPB0_REG8(0x217d) = 0x06;
	ISPAPB0_REG8(0x217e) = 0x0c; // YHdn low threshold   
	ISPAPB0_REG8(0x217f) = 0x18; // YHdn high threshold  
	ISPAPB0_REG8(0x2180) = 0x36; // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1  
	ISPAPB0_REG8(0x2181) = 0x90; // Fe00 and Fe01    
	ISPAPB0_REG8(0x2182) = 0x99; // Fe02 and Fe11    
	ISPAPB0_REG8(0x2183) = 0x52; // Fe12 and Fe22    
	ISPAPB0_REG8(0x2184) = 0x02; // Fea
	ISPAPB0_REG8(0x2185) = 0x20; // Y 5x5 edge gain  
	ISPAPB0_REG8(0x2186) = 0x24;
	ISPAPB0_REG8(0x2187) = 0x2a;
	ISPAPB0_REG8(0x2188) = 0x24;
	ISPAPB0_REG8(0x2189) = 0x22;
	ISPAPB0_REG8(0x218a) = 0x1e;
	ISPAPB0_REG8(0x218b) = 0x08; // Y 5x5 edge threshold 
	ISPAPB0_REG8(0x218c) = 0x10;
	ISPAPB0_REG8(0x218d) = 0x18;
	ISPAPB0_REG8(0x218e) = 0x20;
	ISPAPB0_REG8(0x218f) = 0x40;
	ISPAPB0_REG8(0x2190) = 0x02; // Y 5x5 edge coring value
	ISPAPB0_REG8(0x2191) = 0x03;
	ISPAPB0_REG8(0x2192) = 0x04;
	ISPAPB0_REG8(0x2193) = 0x05;
	ISPAPB0_REG8(0x2194) = 0x05;
	ISPAPB0_REG8(0x2195) = 0x05;
	ISPAPB0_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold   
	ISPAPB0_REG8(0x2197) = 0x40;
	ISPAPB0_REG8(0x2198) = 0x80;
	ISPAPB0_REG8(0x2199) = 0xa0;
	ISPAPB0_REG8(0x219a) = 0xc0;
	ISPAPB0_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB0_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB0_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB0_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold    
	ISPAPB0_REG8(0x219f) = 0x04; // Ysr weight
	ISPAPB0_REG8(0x21b0) = reg_21b0; // factor for Hsize
	ISPAPB0_REG8(0x21b1) = reg_21b1; //
	ISPAPB0_REG8(0x21b2) = reg_21b2; // factor for Vsize
	ISPAPB0_REG8(0x21b3) = reg_21b3; //
	ISPAPB0_REG8(0x21b4) = reg_21b4; // factor for Hsize
	ISPAPB0_REG8(0x21b5) = reg_21b5; //
	ISPAPB0_REG8(0x21b6) = reg_21b6; // factor for Vsize
	ISPAPB0_REG8(0x21b7) = reg_21b7; //
	ISPAPB0_REG8(0x21b8) = reg_21b8; // disable H/V scale down 
	ISPAPB0_REG8(0x21ba) = 0x03; // 2D YUV scaling before YUV spf 
	ISPAPB0_REG8(0x21c0) = 0x03; // enable bchs/scale
	ISPAPB0_REG8(0x21c1) = 0x00; // Y brightness
	ISPAPB0_REG8(0x21c2) = 0x1c; // Y contrast
	ISPAPB0_REG8(0x21c3) = 0x0f; // Hue SIN data
	ISPAPB0_REG8(0x21c4) = 0x3e; // Hue COS data
	ISPAPB0_REG8(0x21c5) = 0x28; // Hue saturation   
	ISPAPB0_REG8(0x21c6) = 0x05; // Y offset
	ISPAPB0_REG8(0x21c7) = 0x80; // Y center
	ISPAPB0_REG8(0x21ce) = 0x3a; // LC Gn gain
	ISPAPB0_REG8(0x21d1) = reg_21d1; // use CDSP
	ISPAPB0_REG8(0x21d2) = reg_21d2; // YUV422
	ISPAPB0_REG8(0x2311) = reg_2311; // format:2bytes
	ISPAPB0_REG8(0x3100) = 0x70; // enable new WB offset/gain and offset after gain
	ISPAPB0_REG8(0x3101) = 0x03; // R new WB offset: 5
	ISPAPB0_REG8(0x3102) = 0x01; // G new WB offset: 84
	ISPAPB0_REG8(0x3103) = 0x02; // B new WB offset: 16
	ISPAPB0_REG8(0x3104) = 0x3f; // R new WB gain: 22
	ISPAPB0_REG8(0x3105) = 0x00;
	ISPAPB0_REG8(0x3106) = 0x3d; // G new WB gain: 0 
	ISPAPB0_REG8(0x3107) = 0x00;
	ISPAPB0_REG8(0x3108) = 0x44; // B new WB gain: 56
	ISPAPB0_REG8(0x3109) = 0x00;
	ISPAPB0_REG8(0x317a) = 0x22; // enable rgbedgeen mul 2 div 1 enable rgbmode
	ISPAPB0_REG8(0x317b) = 0x0a; // rgbedgelothr
	ISPAPB0_REG8(0x317c) = 0x28; // rgbedgehithr
	ISPAPB0_REG8(0x31af) = 0x03; // enable HDR H/V smoothing mode 
	ISPAPB0_REG8(0x31b0) = 0x10; // hdrsmlthr 
	ISPAPB0_REG8(0x31b1) = 0x40; // hdrsmlmax 
	ISPAPB0_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0
	ISPAPB0_REG8(0x31c1) = 0x09; // sFe00 and sFe01  
	ISPAPB0_REG8(0x31c2) = 0x14; // sFea and Fe11    
	ISPAPB0_REG8(0x31c3) = 0x00; // Y 3x3 edge gain  
	ISPAPB0_REG8(0x31c4) = 0x04;
	ISPAPB0_REG8(0x31c5) = 0x0c;
	ISPAPB0_REG8(0x31c6) = 0x12;
	ISPAPB0_REG8(0x31c7) = 0x16;
	ISPAPB0_REG8(0x31c8) = 0x18;
	ISPAPB0_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold 
	ISPAPB0_REG8(0x31ca) = 0x0c;
	ISPAPB0_REG8(0x31cb) = 0x10;
	ISPAPB0_REG8(0x31cc) = 0x18;
	ISPAPB0_REG8(0x31cd) = 0x28;
	ISPAPB0_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn   
	ISPAPB0_REG8(0x31cf) = 0x08; // Y77StdThr 
	ISPAPB0_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125 
	ISPAPB0_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106 
	ISPAPB0_REG8(0x31d2) = 0x62; // YDifMinBThr1:98  
	ISPAPB0_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77  
	ISPAPB0_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB0_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB0_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB0_REG8(0x31d7) = 0x64; // YStdMMean 
	ISPAPB0_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB0_REG8(0x31d9) = 0x8c; // YStdHMean 
	ISPAPB0_REG8(0x31f0) = 0x24; // YLowSel 
	ISPAPB0_REG8(0x31f3) = 0x04; // YEtrMMin
	ISPAPB0_REG8(0x31f4) = 0x03; // YEtrMMax
	ISPAPB0_REG8(0x31f5) = 0x06; // YEtrHMin
	ISPAPB0_REG8(0x31f6) = 0x05; // YEtrHMax
	ISPAPB0_REG8(0x31f8) = 0x01; // enbale Irhigtmin 
	ISPAPB0_REG8(0x31f9) = 0x06; // enbale Irhigtmin_r   
	ISPAPB0_REG8(0x31fa) = 0x09; // enbale Irhigtmin_b   
	ISPAPB0_REG8(0x31fb) = 0xf8; // Irhighthr 
	ISPAPB0_REG8(0x31fc) = 0xff; // Irhratio_r
	ISPAPB0_REG8(0x31fd) = 0xf8; // Irhratio_b
	ISPAPB0_REG8(0x31fe) = 0x0a; // UVThr1  
	ISPAPB0_REG8(0x31ff) = 0x14; // UVThr2  
	ISPAPB0_REG8(0x2208) = 0x6c; // FullHwdSize 
	ISPAPB0_REG8(0x2209) = 0x07;
	ISPAPB0_REG8(0x220a) = 0xe8; // FullVwdSize 
	ISPAPB0_REG8(0x220b) = 0x03;
	ISPAPB0_REG8(0x220d) = 0x77; // PfullHwdSize, PfullVwdSize    
	ISPAPB0_REG8(0x2210) = 0x00; // Window program selection (all use full window) 
	ISPAPB0_REG8(0x2211) = 0x0c; // AE position RGB domain    
	ISPAPB0_REG8(0x2212) = 0x00; // window hold 
	ISPAPB0_REG8(0x2213) = 0xf0; // NAWB luminance high threshold 1 
	ISPAPB0_REG8(0x2214) = 0xf1; // NAWB luminance high threshold 2 
	ISPAPB0_REG8(0x2215) = 0xf2; // NAWB luminance high threshold 3 
	ISPAPB0_REG8(0x2216) = 0xf3; // NAWB luminance high threshold 4 
	ISPAPB0_REG8(0x2217) = 0x71; // enable new AWB, AWB clamp and block pipe go mode    
	ISPAPB0_REG8(0x2218) = 0xf6; // NAWB R/G/B high threshold
	ISPAPB0_REG8(0x2219) = 0x10; // NAWB GR shift    
	ISPAPB0_REG8(0x221a) = 0x7f; // NAWB GB shift    
	ISPAPB0_REG8(0x221b) = 0x02; // NAWB R/G/B low threshold 
	ISPAPB0_REG8(0x221c) = 0x05; // NAWB luminance low threshold 1
	ISPAPB0_REG8(0x221d) = 0x06; // NAWB luminance low threshold 2
	ISPAPB0_REG8(0x221e) = 0x07; // NAWB luminance low threshold 3
	ISPAPB0_REG8(0x221f) = 0x08; // NAWB luminance low threshold 4
	ISPAPB0_REG8(0x2220) = 0x06; // NAWB GR low threshold 1
	ISPAPB0_REG8(0x2221) = 0xf3; // NAWB GR high threshold 1 
	ISPAPB0_REG8(0x2222) = 0x03; // NAWB GB low threshold 1
	ISPAPB0_REG8(0x2223) = 0xee; // NAWB GB high threshold 1 
	ISPAPB0_REG8(0x2224) = 0x08; // NAWB GR low threshold 2
	ISPAPB0_REG8(0x2225) = 0xf8; // NAWB GR high threshold 2 
	ISPAPB0_REG8(0x2226) = 0x05; // NAWB GB low threshold 2
	ISPAPB0_REG8(0x2227) = 0xf0; // NAWB GB high threshold 2 
	ISPAPB0_REG8(0x2228) = 0x0a; // NAWB GR low threshold 3
	ISPAPB0_REG8(0x2229) = 0xfa; // NAWB GR high threshold 3 
	ISPAPB0_REG8(0x222a) = 0x07; // NAWB GB low threshold 3
	ISPAPB0_REG8(0x222b) = 0xf4; // NAWB GB high threshold 3 
	ISPAPB0_REG8(0x222c) = 0x0c; // NAWB GR low threshold 4
	ISPAPB0_REG8(0x222d) = 0xfd; // NAWB GR high threshold 4 
	ISPAPB0_REG8(0x222e) = 0x09; // NAWB GB low threshold 4
	ISPAPB0_REG8(0x222f) = 0xf6; // NAWB GB high threshold 4 
	ISPAPB0_REG8(0x224b) = 0x05; // Histogram 
	ISPAPB0_REG8(0x224c) = 0x77; // Low threshold    
	ISPAPB0_REG8(0x224d) = 0x88; // High threshold   
	ISPAPB0_REG8(0x22a0) = 0x01; // naf1en  
	ISPAPB0_REG8(0x22a1) = 0x01; // naf1jlinecnt
	ISPAPB0_REG8(0x22a2) = 0x08; // naf1lowthr1 
	ISPAPB0_REG8(0x22a3) = 0x05; // naf1lowthr2 
	ISPAPB0_REG8(0x22a4) = 0x02; // naf1lowthr3 
	ISPAPB0_REG8(0x22a5) = 0x00; // naf1lowthr4
	ISPAPB0_REG8(0x22a6) = 0xff; // naf1highthr
	ISPAPB0_REG8(0x22a8) = 0x00; // naf1hoffset[7:0]
	ISPAPB0_REG8(0x22a9) = 0x00; // naf1hoffset[10:8]
	ISPAPB0_REG8(0x22aa) = 0x00; // naf1voffset[7:0] 
	ISPAPB0_REG8(0x22ab) = 0x00; // naf1voffset[10:8]
	ISPAPB0_REG8(0x22ac) = 0x6c; // naf1hsize[7:0] 
	ISPAPB0_REG8(0x22ad) = 0x07; // naf1hsize[10:8]
	ISPAPB0_REG8(0x22ae) = 0xe8; // naf1vsize[7:0] 
	ISPAPB0_REG8(0x22af) = 0x03; // naf1vsize[10:8]
	ISPAPB0_REG8(0x22f1) = 0x01; // enable AFD and H average: 4 pixels 
	ISPAPB0_REG8(0x22f2) = 0x04; // AFD Hdist
	ISPAPB0_REG8(0x22f3) = 0x00; // AFD Vdist
	ISPAPB0_REG8(0x22f4) = 0x00; // AfdHwdOffset0    
	ISPAPB0_REG8(0x22f5) = 0x00;
	ISPAPB0_REG8(0x22f6) = 0x00; // AfdWVOffset0
	ISPAPB0_REG8(0x22f7) = 0x00;
	ISPAPB0_REG8(0x22f8) = 0x6c; // AfdWHSize 
	ISPAPB0_REG8(0x22f9) = 0x07;
	ISPAPB0_REG8(0x22fa) = 0xe8; //AfdWVSize 
	ISPAPB0_REG8(0x22fb) = 0x03;
	ISPAPB0_REG8(0x3200) = 0x0f; // enable His16 and auto address increase
	ISPAPB0_REG8(0x3201) = 0x03; // His16HDist
	ISPAPB0_REG8(0x3202) = 0x03; // His16VDist
	ISPAPB0_REG8(0x3206) = 0x07; // AE window: 9x9 AE 
	ISPAPB0_REG8(0x3207) = 0x55; // Pae9HaccFac, Pae9VaccFac
	ISPAPB0_REG8(0x3208) = 0x00; // Ae9HOffset
	ISPAPB0_REG8(0x3209) = 0x00;
	ISPAPB0_REG8(0x320a) = 0x00; // Ae9VOffset
	ISPAPB0_REG8(0x320b) = 0x00;
#if 1 // window value check
	ISPAPB0_REG8(0x320c) = 0xd2; // Ae9Hsize
	ISPAPB0_REG8(0x320d) = 0x00;
	ISPAPB0_REG8(0x320e) = 0x75; // Ae9Vsize
	ISPAPB0_REG8(0x320f) = 0x00; 
#else
	ISPAPB0_REG8(0x320c) = 0xd7; // Ae9Hsize
	ISPAPB0_REG8(0x320d) = 0x00;
	ISPAPB0_REG8(0x320e) = 0xd7; // Ae9Vsize
	ISPAPB0_REG8(0x320f) = 0x00; 
#endif
	ISPAPB0_REG8(0x3220) = 0x09; // NAWB luminance low threshold 5  
	ISPAPB0_REG8(0x3221) = 0xf4; // NAWB luminance high threshold 5 
	ISPAPB0_REG8(0x3222) = 0x0e; // NAWB GR low threshold 5  
	ISPAPB0_REG8(0x3223) = 0xfe; // NAWB GR high threshold 5 
	ISPAPB0_REG8(0x3224) = 0x0b; // NAWB GB low threshold 5  
	ISPAPB0_REG8(0x3225) = 0xf8; // NAWB GB high threshold 5 
	ISPAPB0_REG8(0x3226) = 0x00; // NAE position 
	ISPAPB0_REG8(0x3290) = 0x00; // SFullHOffset
	ISPAPB0_REG8(0x3291) = 0x00;    
	ISPAPB0_REG8(0x3292) = 0x00; // SFullVOffset
	ISPAPB0_REG8(0x3293) = 0x00;    
	ISPAPB0_REG8(0x329f) = 0x01; // FullOffsetMode 

	/* ISP1 Configuration */
	// clock setting
	ISPAPB1_REG8(0x2008) = 0x07;
	ISPAPB1_REG8(0x275e) = 0x05;
	ISPAPB1_REG8(0x275b) = 0x01;
	//
	// CDSP register setting 
	ISPAPB1_REG8(0x2106) = 0x00; // pixel and line switch
	ISPAPB1_REG8(0x2107) = 0x20;
	ISPAPB1_REG8(0x2108) = 0x03; // enable manual OB 
	ISPAPB1_REG8(0x2109) = 0x00;
	ISPAPB1_REG8(0x210d) = 0x00; // vertical mirror line: original
	ISPAPB1_REG8(0x210e) = 0x00; // double buffer be set immediately
	ISPAPB1_REG8(0x210f) = 0x00; // without sync vd  
	ISPAPB1_REG8(0x2110) = 0xe9; // enable global gain, LchStep=32, LcvStep=64 
	ISPAPB1_REG8(0x2111) = 0x01; // LcHinc  
	ISPAPB1_REG8(0x2112) = 0x01; // LcVinc  
	ISPAPB1_REG8(0x2113) = 0x42; // LC Rgain
	ISPAPB1_REG8(0x2114) = 0x34; // LC Ggain
	ISPAPB1_REG8(0x2115) = 0x3c; // LC Bgain
	ISPAPB1_REG8(0x2116) = 0x08; // LC Xoffset
	ISPAPB1_REG8(0x2117) = 0x00;
	ISPAPB1_REG8(0x2118) = 0x18; // LC Yoffset
	ISPAPB1_REG8(0x2119) = 0x00;
	ISPAPB1_REG8(0x211a) = 0x89; // Centvoffset=8 Centhoffset=9   
	ISPAPB1_REG8(0x211b) = 0x9a; // Centvsize=9 Centhsize=10 
	ISPAPB1_REG8(0x211c) = 0x32; // Quan_n=3 Quan_m=2
	ISPAPB1_REG8(0x211d) = 0x89; // rbactthr
	ISPAPB1_REG8(0x211e) = 0x04; // low bad pixel threshold
	ISPAPB1_REG8(0x211f) = 0xf7; // high bad pixel threshold
	ISPAPB1_REG8(0x2120) = 0x03; // enable bad pixel 
	ISPAPB1_REG8(0x2121) = 0x01; // enable bad pixel replacement 
	ISPAPB1_REG8(0x2124) = 0x00; // HdrmapMode=0
	ISPAPB1_REG8(0x2125) = 0x44; // enable HDR saturation
	ISPAPB1_REG8(0x2126) = 0xf0; // disable HDR 
	ISPAPB1_REG8(0x2127) = 0x77; // HdrFac1: 5 and HdrFac2: 6
	ISPAPB1_REG8(0x2128) = 0x77; // HdrFac3: 8 and HdrFac4: 7
	ISPAPB1_REG8(0x2129) = 0x77; // HdrFac5: 9 and HdrFac6: 8
	ISPAPB1_REG8(0x212a) = 0x00; // HdrGain0: (34/64)
	ISPAPB1_REG8(0x212b) = 0x00; // HdrGain1: (51/64)
	ISPAPB1_REG8(0x212c) = 0x00; // HdrGain2: (68/64)
	ISPAPB1_REG8(0x212d) = 0x00; // HdrGain3: (16/64)
	ISPAPB1_REG8(0x212e) = 0x00; // HdrGain4: (02/64)
	ISPAPB1_REG8(0x212f) = 0x00; // HdrGain5: (01/64)
	ISPAPB1_REG8(0x2130) = 0x40; // R WB gain:
	ISPAPB1_REG8(0x2131) = 0x00;
	ISPAPB1_REG8(0x2132) = 0x00; // R WB offset:
	ISPAPB1_REG8(0x2134) = 0x40; // Gr WB gain: 
	ISPAPB1_REG8(0x2135) = 0x00;
	ISPAPB1_REG8(0x2136) = 0x00; // Gr WB offset:    
	ISPAPB1_REG8(0x2138) = 0x40; // B WB gain:
	ISPAPB1_REG8(0x2139) = 0x00;
	ISPAPB1_REG8(0x213a) = 0x00; // B WB offset:
	ISPAPB1_REG8(0x213c) = 0x40; // Gb WB gain: 
	ISPAPB1_REG8(0x213d) = 0x00;
	ISPAPB1_REG8(0x213e) = 0x00; // Gb WB offset:    
	ISPAPB1_REG8(0x213f) = 0x70; // WB enable  
	ISPAPB1_REG8(0x2140) = 0x13; // disable mask
	ISPAPB1_REG8(0x2141) = 0x57; // H/V edge threshold   
	ISPAPB1_REG8(0x2142) = 0x06; // ir4x4 type
	ISPAPB1_REG8(0x2143) = 0x11; // Fpncmpen
	ISPAPB1_REG8(0x2144) = 0x13; // FPNgain 
	ISPAPB1_REG8(0x2145) = 0x02; // FPNhoffset
	ISPAPB1_REG8(0x2146) = 0x00; // 
	ISPAPB1_REG8(0x2147) = 0x05; // irfacY  
	ISPAPB1_REG8(0x2148) = 0x46; // color matrix setting 
	ISPAPB1_REG8(0x2149) = 0x00;
	ISPAPB1_REG8(0x214a) = 0xfc;
	ISPAPB1_REG8(0x214b) = 0x01;
	ISPAPB1_REG8(0x214c) = 0xfe;
	ISPAPB1_REG8(0x214d) = 0x01;
	ISPAPB1_REG8(0x214e) = 0xf7;
	ISPAPB1_REG8(0x214f) = 0x01;
	ISPAPB1_REG8(0x2150) = 0x4c;
	ISPAPB1_REG8(0x2151) = 0x00;
	ISPAPB1_REG8(0x2152) = 0xfd;
	ISPAPB1_REG8(0x2153) = 0x01;
	ISPAPB1_REG8(0x2154) = 0x00;
	ISPAPB1_REG8(0x2155) = 0x00;
	ISPAPB1_REG8(0x2156) = 0xf1;
	ISPAPB1_REG8(0x2157) = 0x01;
	ISPAPB1_REG8(0x2158) = 0x4f;
	ISPAPB1_REG8(0x2159) = 0x00;
	ISPAPB1_REG8(0x215c) = 0x01; // enable post gamma
	ISPAPB1_REG8(0x215d) = 0x01; // enable dither    
	ISPAPB1_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB1_REG8(0x2160) = 0x0a; // Y LUT LPF step   
	ISPAPB1_REG8(0x2161) = 0x10; // Y LUT value 
	ISPAPB1_REG8(0x2162) = 0x20;
	ISPAPB1_REG8(0x2163) = 0x30;
	ISPAPB1_REG8(0x2164) = 0x40;
	ISPAPB1_REG8(0x2165) = 0x50;
	ISPAPB1_REG8(0x2166) = 0x60;
	ISPAPB1_REG8(0x2167) = 0x70;
	ISPAPB1_REG8(0x2168) = 0x80;
	ISPAPB1_REG8(0x2169) = 0x90;
	ISPAPB1_REG8(0x216a) = 0xa0;
	ISPAPB1_REG8(0x216b) = 0xb0;
	ISPAPB1_REG8(0x216c) = 0xc0;
	ISPAPB1_REG8(0x216d) = 0xd0;
	ISPAPB1_REG8(0x216e) = 0xe0;
	ISPAPB1_REG8(0x216f) = 0xf0;
	ISPAPB1_REG8(0x2170) = 0x13; // YUVSPF setting   
	ISPAPB1_REG8(0x2171) = 0x7f; // enable Y 5x5 edge
	ISPAPB1_REG8(0x2172) = 0x21; // enable UV SRAM replace mode   
	ISPAPB1_REG8(0x2173) = 0x73; // enable Y SRAM replace
	ISPAPB1_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion  
	ISPAPB1_REG8(0x2175) = 0x04; // YFuLThr 
	ISPAPB1_REG8(0x2176) = 0x08; // YFuHThr 
	ISPAPB1_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB1_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB1_REG8(0x2179) = 0x51; // enable YHdn and factor: 256   
	ISPAPB1_REG8(0x217a) = 0x55; // YHdn position initialization  
	ISPAPB1_REG8(0x217b) = 0x00;
	ISPAPB1_REG8(0x217c) = 0xf2; // YHdn right boundary  
	ISPAPB1_REG8(0x217d) = 0x06;
	ISPAPB1_REG8(0x217e) = 0x0c; // YHdn low threshold   
	ISPAPB1_REG8(0x217f) = 0x18; // YHdn high threshold  
	ISPAPB1_REG8(0x2180) = 0x36; // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1  
	ISPAPB1_REG8(0x2181) = 0x90; // Fe00 and Fe01    
	ISPAPB1_REG8(0x2182) = 0x99; // Fe02 and Fe11    
	ISPAPB1_REG8(0x2183) = 0x52; // Fe12 and Fe22    
	ISPAPB1_REG8(0x2184) = 0x02; // Fea
	ISPAPB1_REG8(0x2185) = 0x20; // Y 5x5 edge gain  
	ISPAPB1_REG8(0x2186) = 0x24;
	ISPAPB1_REG8(0x2187) = 0x2a;
	ISPAPB1_REG8(0x2188) = 0x24;
	ISPAPB1_REG8(0x2189) = 0x22;
	ISPAPB1_REG8(0x218a) = 0x1e;
	ISPAPB1_REG8(0x218b) = 0x08; // Y 5x5 edge threshold 
	ISPAPB1_REG8(0x218c) = 0x10;
	ISPAPB1_REG8(0x218d) = 0x18;
	ISPAPB1_REG8(0x218e) = 0x20;
	ISPAPB1_REG8(0x218f) = 0x40;
	ISPAPB1_REG8(0x2190) = 0x02; // Y 5x5 edge coring value
	ISPAPB1_REG8(0x2191) = 0x03;
	ISPAPB1_REG8(0x2192) = 0x04;
	ISPAPB1_REG8(0x2193) = 0x05;
	ISPAPB1_REG8(0x2194) = 0x05;
	ISPAPB1_REG8(0x2195) = 0x05;
	ISPAPB1_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold   
	ISPAPB1_REG8(0x2197) = 0x40;
	ISPAPB1_REG8(0x2198) = 0x80;
	ISPAPB1_REG8(0x2199) = 0xa0;
	ISPAPB1_REG8(0x219a) = 0xc0;
	ISPAPB1_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB1_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB1_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB1_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold    
	ISPAPB1_REG8(0x219f) = 0x04; // Ysr weight
	ISPAPB1_REG8(0x21b0) = reg_21b0; // factor for Hsize
	ISPAPB1_REG8(0x21b1) = reg_21b1; //
	ISPAPB1_REG8(0x21b2) = reg_21b2; // factor for Vsize
	ISPAPB1_REG8(0x21b3) = reg_21b3; //
	ISPAPB1_REG8(0x21b4) = reg_21b4; // factor for Hsize
	ISPAPB1_REG8(0x21b5) = reg_21b5; //
	ISPAPB1_REG8(0x21b6) = reg_21b6; // factor for Vsize
	ISPAPB1_REG8(0x21b7) = reg_21b7; //
	ISPAPB1_REG8(0x21b8) = reg_21b8; // disable H/V scale down 
	ISPAPB1_REG8(0x21ba) = 0x03; // 2D YUV scaling before YUV spf 
	ISPAPB1_REG8(0x21c0) = 0x03; // enable bchs/scale
	ISPAPB1_REG8(0x21c1) = 0x00; // Y brightness
	ISPAPB1_REG8(0x21c2) = 0x1c; // Y contrast
	ISPAPB1_REG8(0x21c3) = 0x0f; // Hue SIN data
	ISPAPB1_REG8(0x21c4) = 0x3e; // Hue COS data
	ISPAPB1_REG8(0x21c5) = 0x28; // Hue saturation   
	ISPAPB1_REG8(0x21c6) = 0x05; // Y offset
	ISPAPB1_REG8(0x21c7) = 0x80; // Y center
	ISPAPB1_REG8(0x21ce) = 0x3a; // LC Gn gain
	ISPAPB1_REG8(0x21d1) = reg_21d1; // use CDSP
	ISPAPB1_REG8(0x21d2) = reg_21d2; // YUV422
	ISPAPB0_REG8(0x2311) = reg_2311; // format:2bytes
	ISPAPB1_REG8(0x3100) = 0x70; // enable new WB offset/gain and offset after gain
	ISPAPB1_REG8(0x3101) = 0x03; // R new WB offset: 5   
	ISPAPB1_REG8(0x3102) = 0x01; // G new WB offset: 84  
	ISPAPB1_REG8(0x3103) = 0x02; // B new WB offset: 16  
	ISPAPB1_REG8(0x3104) = 0x3f; // R new WB gain: 22
	ISPAPB1_REG8(0x3105) = 0x00;
	ISPAPB1_REG8(0x3106) = 0x3d; // G new WB gain: 0 
	ISPAPB1_REG8(0x3107) = 0x00;
	ISPAPB1_REG8(0x3108) = 0x44; // B new WB gain: 56
	ISPAPB1_REG8(0x3109) = 0x00;
	ISPAPB1_REG8(0x317a) = 0x22; // enable rgbedgeen mul 2 div 1 enable rgbmode
	ISPAPB1_REG8(0x317b) = 0x0a; // rgbedgelothr
	ISPAPB1_REG8(0x317c) = 0x28; // rgbedgehithr
	ISPAPB1_REG8(0x31af) = 0x03; // enable HDR H/V smoothing mode 
	ISPAPB1_REG8(0x31b0) = 0x10; // hdrsmlthr 
	ISPAPB1_REG8(0x31b1) = 0x40; // hdrsmlmax 
	ISPAPB1_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0
	ISPAPB1_REG8(0x31c1) = 0x09; // sFe00 and sFe01  
	ISPAPB1_REG8(0x31c2) = 0x14; // sFea and Fe11    
	ISPAPB1_REG8(0x31c3) = 0x00; // Y 3x3 edge gain  
	ISPAPB1_REG8(0x31c4) = 0x04;
	ISPAPB1_REG8(0x31c5) = 0x0c;
	ISPAPB1_REG8(0x31c6) = 0x12;
	ISPAPB1_REG8(0x31c7) = 0x16;
	ISPAPB1_REG8(0x31c8) = 0x18;
	ISPAPB1_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold 
	ISPAPB1_REG8(0x31ca) = 0x0c;
	ISPAPB1_REG8(0x31cb) = 0x10;
	ISPAPB1_REG8(0x31cc) = 0x18;
	ISPAPB1_REG8(0x31cd) = 0x28;
	ISPAPB1_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn   
	ISPAPB1_REG8(0x31cf) = 0x08; // Y77StdThr 
	ISPAPB1_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125 
	ISPAPB1_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106 
	ISPAPB1_REG8(0x31d2) = 0x62; // YDifMinBThr1:98  
	ISPAPB1_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77  
	ISPAPB1_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB1_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB1_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB1_REG8(0x31d7) = 0x64; // YStdMMean 
	ISPAPB1_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB1_REG8(0x31d9) = 0x8c; // YStdHMean 
	ISPAPB1_REG8(0x31f0) = 0x24; // YLowSel 
	ISPAPB1_REG8(0x31f3) = 0x04; // YEtrMMin
	ISPAPB1_REG8(0x31f4) = 0x03; // YEtrMMax
	ISPAPB1_REG8(0x31f5) = 0x06; // YEtrHMin
	ISPAPB1_REG8(0x31f6) = 0x05; // YEtrHMax
	ISPAPB1_REG8(0x31f8) = 0x01; // enbale Irhigtmin 
	ISPAPB1_REG8(0x31f9) = 0x06; // enbale Irhigtmin_r   
	ISPAPB1_REG8(0x31fa) = 0x09; // enbale Irhigtmin_b   
	ISPAPB1_REG8(0x31fb) = 0xf8; // Irhighthr 
	ISPAPB1_REG8(0x31fc) = 0xff; // Irhratio_r
	ISPAPB1_REG8(0x31fd) = 0xf8; // Irhratio_b
	ISPAPB1_REG8(0x31fe) = 0x0a; // UVThr1  
	ISPAPB1_REG8(0x31ff) = 0x14; // UVThr2  
	ISPAPB1_REG8(0x2208) = 0x6c; // FullHwdSize 
	ISPAPB1_REG8(0x2209) = 0x07;
	ISPAPB1_REG8(0x220a) = 0xe8; // FullVwdSize 
	ISPAPB1_REG8(0x220b) = 0x03;
	ISPAPB1_REG8(0x220d) = 0x77; // PfullHwdSize, PfullVwdSize    
	ISPAPB1_REG8(0x2210) = 0x00; // Window program selection (all use full window) 
	ISPAPB1_REG8(0x2211) = 0x0c; // AE position RGB domain    
	ISPAPB1_REG8(0x2212) = 0x00; // window hold 
	ISPAPB1_REG8(0x2213) = 0xf0; // NAWB luminance high threshold 1 
	ISPAPB1_REG8(0x2214) = 0xf1; // NAWB luminance high threshold 2 
	ISPAPB1_REG8(0x2215) = 0xf2; // NAWB luminance high threshold 3 
	ISPAPB1_REG8(0x2216) = 0xf3; // NAWB luminance high threshold 4 
	ISPAPB1_REG8(0x2217) = 0x71; // enable new AWB, AWB clamp and block pipe go mode    
	ISPAPB1_REG8(0x2218) = 0xf6; // NAWB R/G/B high threshold
	ISPAPB1_REG8(0x2219) = 0x10; // NAWB GR shift    
	ISPAPB1_REG8(0x221a) = 0x7f; // NAWB GB shift    
	ISPAPB1_REG8(0x221b) = 0x02; // NAWB R/G/B low threshold 
	ISPAPB1_REG8(0x221c) = 0x05; // NAWB luminance low threshold 1
	ISPAPB1_REG8(0x221d) = 0x06; // NAWB luminance low threshold 2
	ISPAPB1_REG8(0x221e) = 0x07; // NAWB luminance low threshold 3
	ISPAPB1_REG8(0x221f) = 0x08; // NAWB luminance low threshold 4
	ISPAPB1_REG8(0x2220) = 0x06; // NAWB GR low threshold 1
	ISPAPB1_REG8(0x2221) = 0xf3; // NAWB GR high threshold 1 
	ISPAPB1_REG8(0x2222) = 0x03; // NAWB GB low threshold 1
	ISPAPB1_REG8(0x2223) = 0xee; // NAWB GB high threshold 1 
	ISPAPB1_REG8(0x2224) = 0x08; // NAWB GR low threshold 2
	ISPAPB1_REG8(0x2225) = 0xf8; // NAWB GR high threshold 2 
	ISPAPB1_REG8(0x2226) = 0x05; // NAWB GB low threshold 2
	ISPAPB1_REG8(0x2227) = 0xf0; // NAWB GB high threshold 2 
	ISPAPB1_REG8(0x2228) = 0x0a; // NAWB GR low threshold 3
	ISPAPB1_REG8(0x2229) = 0xfa; // NAWB GR high threshold 3 
	ISPAPB1_REG8(0x222a) = 0x07; // NAWB GB low threshold 3
	ISPAPB1_REG8(0x222b) = 0xf4; // NAWB GB high threshold 3 
	ISPAPB1_REG8(0x222c) = 0x0c; // NAWB GR low threshold 4
	ISPAPB1_REG8(0x222d) = 0xfd; // NAWB GR high threshold 4 
	ISPAPB1_REG8(0x222e) = 0x09; // NAWB GB low threshold 4
	ISPAPB1_REG8(0x222f) = 0xf6; // NAWB GB high threshold 4 
	ISPAPB1_REG8(0x224b) = 0x05; // Histogram 
	ISPAPB1_REG8(0x224c) = 0x77; // Low threshold    
	ISPAPB1_REG8(0x224d) = 0x88; // High threshold   
	ISPAPB1_REG8(0x22a0) = 0x01; // naf1en  
	ISPAPB1_REG8(0x22a1) = 0x01; // naf1jlinecnt
	ISPAPB1_REG8(0x22a2) = 0x08; // naf1lowthr1 
	ISPAPB1_REG8(0x22a3) = 0x05; // naf1lowthr2 
	ISPAPB1_REG8(0x22a4) = 0x02; // naf1lowthr3 
	ISPAPB1_REG8(0x22a5) = 0x00; // naf1lowthr4
	ISPAPB1_REG8(0x22a6) = 0xff; // naf1highthr
	ISPAPB1_REG8(0x22a8) = 0x00; // naf1hoffset[7:0]
	ISPAPB1_REG8(0x22a9) = 0x00; // naf1hoffset[10:8]
	ISPAPB1_REG8(0x22aa) = 0x00; // naf1voffset[7:0] 
	ISPAPB1_REG8(0x22ab) = 0x00; // naf1voffset[10:8]
	ISPAPB1_REG8(0x22ac) = 0x6c; // naf1hsize[7:0] 
	ISPAPB1_REG8(0x22ad) = 0x07; // naf1hsize[10:8]
	ISPAPB1_REG8(0x22ae) = 0xe8; // naf1vsize[7:0] 
	ISPAPB1_REG8(0x22af) = 0x03; // naf1vsize[10:8]
	ISPAPB1_REG8(0x22f1) = 0x01; // enable AFD and H average: 4 pixels 
	ISPAPB1_REG8(0x22f2) = 0x04; // AFD Hdist
	ISPAPB1_REG8(0x22f3) = 0x00; // AFD Vdist
	ISPAPB1_REG8(0x22f4) = 0x00; // AfdHwdOffset0    
	ISPAPB1_REG8(0x22f5) = 0x00;
	ISPAPB1_REG8(0x22f6) = 0x00; // AfdWVOffset0
	ISPAPB1_REG8(0x22f7) = 0x00;
	ISPAPB1_REG8(0x22f8) = 0x6c; // AfdWHSize 
	ISPAPB1_REG8(0x22f9) = 0x07;
	ISPAPB1_REG8(0x22fa) = 0xe8; //AfdWVSize 
	ISPAPB1_REG8(0x22fb) = 0x03;
	ISPAPB1_REG8(0x3200) = 0x0f; // enable His16 and auto address increase
	ISPAPB1_REG8(0x3201) = 0x03; // His16HDist
	ISPAPB1_REG8(0x3202) = 0x03; // His16VDist
	ISPAPB1_REG8(0x3206) = 0x07; // AE window: 9x9 AE 
	ISPAPB1_REG8(0x3207) = 0x55; // Pae9HaccFac, Pae9VaccFac
	ISPAPB1_REG8(0x3208) = 0x00; // Ae9HOffset
	ISPAPB1_REG8(0x3209) = 0x00;
	ISPAPB1_REG8(0x320a) = 0x00; // Ae9VOffset
	ISPAPB1_REG8(0x320b) = 0x00;
#if 1 // window value check
	ISPAPB1_REG8(0x320c) = 0xd2; // Ae9Hsize
	ISPAPB1_REG8(0x320d) = 0x00;
	ISPAPB1_REG8(0x320e) = 0x75; // Ae9Vsize
	ISPAPB1_REG8(0x320f) = 0x00; 
#else
	ISPAPB1_REG8(0x320c) = 0xd7; // Ae9Hsize
	ISPAPB1_REG8(0x320d) = 0x00;
	ISPAPB1_REG8(0x320e) = 0xd7; // Ae9Vsize
	ISPAPB1_REG8(0x320f) = 0x00; 
#endif
	ISPAPB1_REG8(0x3220) = 0x09; // NAWB luminance low threshold 5  
	ISPAPB1_REG8(0x3221) = 0xf4; // NAWB luminance high threshold 5 
	ISPAPB1_REG8(0x3222) = 0x0e; // NAWB GR low threshold 5  
	ISPAPB1_REG8(0x3223) = 0xfe; // NAWB GR high threshold 5 
	ISPAPB1_REG8(0x3224) = 0x0b; // NAWB GB low threshold 5  
	ISPAPB1_REG8(0x3225) = 0xf8; // NAWB GB high threshold 5 
	ISPAPB1_REG8(0x3226) = 0x00; // NAE position 
	ISPAPB1_REG8(0x3290) = 0x00; // SFullHOffset
	ISPAPB1_REG8(0x3291) = 0x00;    
	ISPAPB1_REG8(0x3292) = 0x00; // SFullVOffset
	ISPAPB1_REG8(0x3293) = 0x00;    
	ISPAPB1_REG8(0x329f) = 0x01; // FullOffsetMode 
}

/*
	@ispAaaInit
*/
void ispAaaInit_raw10(void)
{
}


/*
	@ispReset
*/
void ispReset_yuv422(void)
{
	/* ISP0 Configuration */
	ISPAPB0_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB0_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB0_REG8(0x2010) = 0x00; // cclk: 48MHz

	/* ISP1 Configuration */
	ISPAPB1_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB1_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB1_REG8(0x2010) = 0x00; // cclk: 48MHz
}

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/
#define YUV422_ORIGINAL_SETTING     1

void FrontInit_yuv422(u16 width, u16 height, u8 probe)
{
	/* ISP0 Configuration */
#if (YUV422_ORIGINAL_SETTING == 1) // Original
	//
	ISPAPB0_REG8(0x2711) = 0x30; // hfall
	ISPAPB0_REG8(0x2712) = 0x02;
	ISPAPB0_REG8(0x2713) = 0x50; // hrise
	ISPAPB0_REG8(0x2714) = 0x02;
	ISPAPB0_REG8(0x2715) = 0x00; // vfall
	ISPAPB0_REG8(0x2716) = 0x0a;
	ISPAPB0_REG8(0x2717) = 0x90; // vrise
	ISPAPB0_REG8(0x2718) = 0x0a;
	ISPAPB0_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB0_REG8(0x2720) = 0x05; // hoffset
	ISPAPB0_REG8(0x2721) = 0x00;
	ISPAPB0_REG8(0x2722) = 0x03; // voffset
	ISPAPB0_REG8(0x2723) = 0x05;
	ISPAPB0_REG8(0x2724) = (width&0xff);//0x80; // hsize
	ISPAPB0_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB0_REG8(0x2726) = (height&0xff);//0x38; // vsize
	ISPAPB0_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB0_REG8(0x2728) = 0x40;
	ISPAPB0_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB0_REG8(0x2741) = 0xff; // syngen line total
	ISPAPB0_REG8(0x2742) = 0x0f;
	ISPAPB0_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB0_REG8(0x2744) = 0x08;
	ISPAPB0_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB0_REG8(0x2746) = 0x0f;
	ISPAPB0_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB0_REG8(0x2748) = 0x05;
	ISPAPB0_REG8(0x2749) = 0x03;
	ISPAPB0_REG8(0x274a) = 0x00;
	ISPAPB0_REG8(0x276a) = 0xec; // siggen enable(hvalidcnt): 75% still vertical color bar
	ISPAPB0_REG8(0x2705) = 0x01;
#else // New (2020/01/21)
	//
	ISPAPB0_REG8(0x2711) = 0x30; // hfall
	ISPAPB0_REG8(0x2712) = 0x00;//0x02;
	ISPAPB0_REG8(0x2713) = 0x50; // hrise
	ISPAPB0_REG8(0x2714) = 0x00;//0x02;
	ISPAPB0_REG8(0x2715) = 0x00; // vfall
	ISPAPB0_REG8(0x2716) = 0x0a;
	ISPAPB0_REG8(0x2717) = 0x90; // vrise
	ISPAPB0_REG8(0x2718) = 0x0a;
	ISPAPB0_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB0_REG8(0x2720) = 0x05; // hoffset
	ISPAPB0_REG8(0x2721) = 0x00;
	ISPAPB0_REG8(0x2722) = 0x03; // voffset
	ISPAPB0_REG8(0x2723) = 0x00;//0x05;
	ISPAPB0_REG8(0x2724) = (width&0xff);//0x80; // hsize
	ISPAPB0_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB0_REG8(0x2726) = (height&0xff);//0x38; // vsize
	ISPAPB0_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB0_REG8(0x2728) = 0x40;
	ISPAPB0_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB0_REG8(0x2741) = 0x00;//0xff; // syngen line total
	ISPAPB0_REG8(0x2742) = 0x08;//0x0f;
	ISPAPB0_REG8(0x2743) = 0x50;//0x00; // syngen line blank
	ISPAPB0_REG8(0x2744) = 0x00;//0x08;
	ISPAPB0_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB0_REG8(0x2746) = 0x05;//0x0f;
	ISPAPB0_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB0_REG8(0x2748) = 0x00;//0x05;
	ISPAPB0_REG8(0x2749) = 0x03;
	ISPAPB0_REG8(0x274a) = 0x00;
	ISPAPB0_REG8(0x276a) = 0xec; // siggen enable(hvalidcnt): 75% still vertical color bar
	ISPAPB0_REG8(0x2705) = 0x01;
#endif // #if 0 // Original
	switch (probe)
	{
		case 0:
			ISPAPB_LOGI("ISP0 probe off\n");
			//ISPAPB0_REG8(0x21e9) = 0x00;
			//ISPAPB0_REG8(0x21e8) = 0x00;
			//ISPAPB0_REG8(0x20e1) = 0x0F;
			break;

		case 1:
			ISPAPB_LOGI("ISP0 probe 1\n");
			ISPAPB0_REG8(0x21e9) = 0x01;
			ISPAPB0_REG8(0x21e8) = 0x09;
			ISPAPB0_REG8(0x20e1) = 0x01;
			break;

		case 2:
			ISPAPB_LOGI("ISP0 probe 2\n");
			ISPAPB0_REG8(0x21e9) = 0x17;
			ISPAPB0_REG8(0x21e8) = 0x00;
			ISPAPB0_REG8(0x20e1) = 0x01;
			break;
	}

	/* ISP1 Configuration */
#if (YUV422_ORIGINAL_SETTING == 1) // Original
	//
	ISPAPB1_REG8(0x2711) = 0x30; // hfall
	ISPAPB1_REG8(0x2712) = 0x02;
	ISPAPB1_REG8(0x2713) = 0x50; // hrise
	ISPAPB1_REG8(0x2714) = 0x02;
	ISPAPB1_REG8(0x2715) = 0x00; // vfall
	ISPAPB1_REG8(0x2716) = 0x0a;
	ISPAPB1_REG8(0x2717) = 0x90; // vrise
	ISPAPB1_REG8(0x2718) = 0x0a;
	ISPAPB1_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB1_REG8(0x2720) = 0x05; // hoffset
	ISPAPB1_REG8(0x2721) = 0x00;
	ISPAPB1_REG8(0x2722) = 0x03; // voffset
	ISPAPB1_REG8(0x2723) = 0x05;
	ISPAPB1_REG8(0x2724) = (width&0xff);//0x80; // hsize
	ISPAPB1_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB1_REG8(0x2726) = (height&0xff);//0x38; // vsize
	ISPAPB1_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB1_REG8(0x2728) = 0x40;
	ISPAPB1_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB1_REG8(0x2741) = 0xff; // syngen line total
	ISPAPB1_REG8(0x2742) = 0x0f;
	ISPAPB1_REG8(0x2743) = 0x00; // syngen line blank
	ISPAPB1_REG8(0x2744) = 0x08;
	ISPAPB1_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB1_REG8(0x2746) = 0x0f;
	ISPAPB1_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB1_REG8(0x2748) = 0x05;
	ISPAPB1_REG8(0x2749) = 0x03;
	ISPAPB1_REG8(0x274a) = 0x00;
	ISPAPB1_REG8(0x276a) = 0xec; // siggen enable(hvalidcnt): 75% still vertical color bar
	ISPAPB1_REG8(0x2705) = 0x01;
#else // New (2020/01/21)
	//
	ISPAPB1_REG8(0x2711) = 0x30; // hfall
	ISPAPB1_REG8(0x2712) = 0x00;//0x02;
	ISPAPB1_REG8(0x2713) = 0x50; // hrise
	ISPAPB1_REG8(0x2714) = 0x00;//0x02;
	ISPAPB1_REG8(0x2715) = 0x00; // vfall
	ISPAPB1_REG8(0x2716) = 0x0a;
	ISPAPB1_REG8(0x2717) = 0x90; // vrise
	ISPAPB1_REG8(0x2718) = 0x0a;
	ISPAPB1_REG8(0x2710) = 0x33; // H/V reshape enable
	ISPAPB1_REG8(0x2720) = 0x05; // hoffset
	ISPAPB1_REG8(0x2721) = 0x00;
	ISPAPB1_REG8(0x2722) = 0x03; // voffset
	ISPAPB1_REG8(0x2723) = 0x00;//0x05;
	ISPAPB1_REG8(0x2724) = (width&0xff);//0x80; // hsize
	ISPAPB1_REG8(0x2725) = ((width>>8)&0xff);//0x07;
	ISPAPB1_REG8(0x2726) = (height&0xff);//0x38; // vsize
	ISPAPB1_REG8(0x2727) = ((height>>8)&0xff);//0x04;
	ISPAPB1_REG8(0x2728) = 0x40;
	ISPAPB1_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB1_REG8(0x2741) = 0x00;//0xff; // syngen line total
	ISPAPB1_REG8(0x2742) = 0x08;//0x0f;
	ISPAPB1_REG8(0x2743) = 0x50;//0x00; // syngen line blank
	ISPAPB1_REG8(0x2744) = 0x00;//0x08;
	ISPAPB1_REG8(0x2745) = 0xaf; // syngen frame total
	ISPAPB1_REG8(0x2746) = 0x05;//0x0f;
	ISPAPB1_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB1_REG8(0x2748) = 0x00;//0x05;
	ISPAPB1_REG8(0x2749) = 0x03;
	ISPAPB1_REG8(0x274a) = 0x00;
	ISPAPB1_REG8(0x276a) = 0xec; // siggen enable(hvalidcnt): 75% still vertical color bar
	ISPAPB1_REG8(0x2705) = 0x01;
#endif // #if 0 // Original
	switch (probe)
	{
		case 0:
			ISPAPB_LOGI("ISP1 probe off\n");
			//ISPAPB1_REG8(0x21e9) = 0x00;
			//ISPAPB1_REG8(0x21e8) = 0x00;
			//ISPAPB1_REG8(0x20e1) = 0x0F;
			break;

		case 1:
			ISPAPB_LOGI("ISP1 probe 1\n");
			ISPAPB1_REG8(0x21e9) = 0x01;
			ISPAPB1_REG8(0x21e8) = 0x09;
			ISPAPB1_REG8(0x20e1) = 0x01;
			break;

		case 2:
			ISPAPB_LOGI("ISP1 probe 2\n");
			ISPAPB1_REG8(0x21e9) = 0x17;
			ISPAPB1_REG8(0x21e8) = 0x00;
			ISPAPB1_REG8(0x20e1) = 0x01;
			break;
	}
}

/*
	@cdspSetTable
*/
void cdspSetTable_yuv422(void)
{
	
}

/*
	@sensorInit
*/
void sensorInit_yuv422(void)
{
	/* ISP0 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB0_REG8(0x2660) = 0x01;
	//ISPAPB0_REG8(0x2660) = 0x00;

	/* ISP1 Configuration */
	// set and clear reset of front i2c interface //
	//ISPAPB1_REG8(0x2660) = 0x01;
	//ISPAPB1_REG8(0x2660) = 0x00;
}

/*
	@CdspInit
*/
void CdspInit_yuv422(void)
{	
	/* ISP0 Configuration */
	// clock setting
	ISPAPB0_REG8(0x2008) = 0x07;
	ISPAPB0_REG8(0x275e) = 0x05;
	ISPAPB0_REG8(0x275b) = 0x01;
	//
	// CDSP register setting 
	ISPAPB0_REG8(0x2106) = 0x00; // pixel and line switch
	ISPAPB0_REG8(0x2107) = 0x20;
	ISPAPB0_REG8(0x2108) = 0x03; // enable manual OB
	ISPAPB0_REG8(0x2109) = 0x00;
	ISPAPB0_REG8(0x210d) = 0x00; // vertical mirror line: original
	ISPAPB0_REG8(0x210e) = 0x00; // double buffer be set immediately
	ISPAPB0_REG8(0x210f) = 0x00; // without sync vd
	ISPAPB0_REG8(0x2110) = 0xe9; // enable global gain, LchStep=32, LcvStep=64
	ISPAPB0_REG8(0x2111) = 0x01; // LcHinc
	ISPAPB0_REG8(0x2112) = 0x01; // LcVinc
	ISPAPB0_REG8(0x2113) = 0x42; // LC Rgain
	ISPAPB0_REG8(0x2114) = 0x34; // LC Ggain
	ISPAPB0_REG8(0x2115) = 0x3c; // LC Bgain
	ISPAPB0_REG8(0x2116) = 0x08; // LC Xoffset 
	ISPAPB0_REG8(0x2117) = 0x00;
	ISPAPB0_REG8(0x2118) = 0x18; // LC Yoffset 
	ISPAPB0_REG8(0x2119) = 0x00;
	ISPAPB0_REG8(0x211a) = 0x89; // Centvoffset=8 Centhoffset=9
	ISPAPB0_REG8(0x211b) = 0x9a; // Centvsize=9 Centhsize=10
	ISPAPB0_REG8(0x211c) = 0x32; // Quan_n=3 Quan_m=2
	ISPAPB0_REG8(0x211d) = 0x89; // rbactthr
	ISPAPB0_REG8(0x211e) = 0x04; // low bad pixel threshold 
	ISPAPB0_REG8(0x211f) = 0xf7; // high bad pixel threshold 
	ISPAPB0_REG8(0x2120) = 0x03; // enable bad pixel
	ISPAPB0_REG8(0x2121) = 0x01; // enable bad pixel replacement
	ISPAPB0_REG8(0x2124) = 0x00; // HdrmapMode=0
	ISPAPB0_REG8(0x2125) = 0x44; // enable HDR saturation
	ISPAPB0_REG8(0x2126) = 0xf0; // disable HDR
	ISPAPB0_REG8(0x2127) = 0x77; // HdrFac1: 5 and HdrFac2: 6
	ISPAPB0_REG8(0x2128) = 0x77; // HdrFac3: 8 and HdrFac4: 7
	ISPAPB0_REG8(0x2129) = 0x77; // HdrFac5: 9 and HdrFac6: 8
	ISPAPB0_REG8(0x212a) = 0x00; // HdrGain0: (34/64)
	ISPAPB0_REG8(0x212b) = 0x00; // HdrGain1: (51/64)
	ISPAPB0_REG8(0x212c) = 0x00; // HdrGain2: (68/64)
	ISPAPB0_REG8(0x212d) = 0x00; // HdrGain3: (16/64)
	ISPAPB0_REG8(0x212e) = 0x00; // HdrGain4: (02/64)
	ISPAPB0_REG8(0x212f) = 0x00; // HdrGain5: (01/64)
	ISPAPB0_REG8(0x2130) = 0x40; // R WB gain: 
	ISPAPB0_REG8(0x2131) = 0x00;
	ISPAPB0_REG8(0x2132) = 0x00; // R WB offset:
	ISPAPB0_REG8(0x2134) = 0x40; // Gr WB gain:
	ISPAPB0_REG8(0x2135) = 0x00;
	ISPAPB0_REG8(0x2136) = 0x00; // Gr WB offset:
	ISPAPB0_REG8(0x2138) = 0x40; // B WB gain: 
	ISPAPB0_REG8(0x2139) = 0x00;
	ISPAPB0_REG8(0x213a) = 0x00; // B WB offset:
	ISPAPB0_REG8(0x213c) = 0x40; // Gb WB gain:
	ISPAPB0_REG8(0x213d) = 0x00;
	ISPAPB0_REG8(0x213e) = 0x00; // Gb WB offset:
	ISPAPB0_REG8(0x213f) = 0x70; // WB enable
	ISPAPB0_REG8(0x2140) = 0x13; // disable mask
	ISPAPB0_REG8(0x2141) = 0x57; // H/V edge threshold
	ISPAPB0_REG8(0x2142) = 0x06; // ir4x4 type 
	ISPAPB0_REG8(0x2143) = 0x11; // Fpncmpen
	ISPAPB0_REG8(0x2144) = 0x13; // FPNgain 
	ISPAPB0_REG8(0x2145) = 0x02; // FPNhoffset 
	ISPAPB0_REG8(0x2146) = 0x00; // 
	ISPAPB0_REG8(0x2147) = 0x05; // irfacY
	ISPAPB0_REG8(0x2148) = 0x46; // color matrix setting 
	ISPAPB0_REG8(0x2149) = 0x00;
	ISPAPB0_REG8(0x214a) = 0xfc;
	ISPAPB0_REG8(0x214b) = 0x01;
	ISPAPB0_REG8(0x214c) = 0xfe;
	ISPAPB0_REG8(0x214d) = 0x01;
	ISPAPB0_REG8(0x214e) = 0xf7;
	ISPAPB0_REG8(0x214f) = 0x01;
	ISPAPB0_REG8(0x2150) = 0x4c;
	ISPAPB0_REG8(0x2151) = 0x00;
	ISPAPB0_REG8(0x2152) = 0xfd;
	ISPAPB0_REG8(0x2153) = 0x01;
	ISPAPB0_REG8(0x2154) = 0x00;
	ISPAPB0_REG8(0x2155) = 0x00;
	ISPAPB0_REG8(0x2156) = 0xf1;
	ISPAPB0_REG8(0x2157) = 0x01;
	ISPAPB0_REG8(0x2158) = 0x4f;
	ISPAPB0_REG8(0x2159) = 0x00;
	ISPAPB0_REG8(0x215c) = 0x01; // enable post gamma 
	ISPAPB0_REG8(0x215d) = 0x01; // enable dither
	ISPAPB0_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB0_REG8(0x2160) = 0x0a; // Y LUT LPF step 
	ISPAPB0_REG8(0x2161) = 0x10; // Y LUT value
	ISPAPB0_REG8(0x2162) = 0x20;
	ISPAPB0_REG8(0x2163) = 0x30;
	ISPAPB0_REG8(0x2164) = 0x40;
	ISPAPB0_REG8(0x2165) = 0x50;
	ISPAPB0_REG8(0x2166) = 0x60;
	ISPAPB0_REG8(0x2167) = 0x70;
	ISPAPB0_REG8(0x2168) = 0x80;
	ISPAPB0_REG8(0x2169) = 0x90;
	ISPAPB0_REG8(0x216a) = 0xa0;
	ISPAPB0_REG8(0x216b) = 0xb0;
	ISPAPB0_REG8(0x216c) = 0xc0;
	ISPAPB0_REG8(0x216d) = 0xd0;
	ISPAPB0_REG8(0x216e) = 0xe0;
	ISPAPB0_REG8(0x216f) = 0xf0;
	ISPAPB0_REG8(0x2170) = 0x13; // YUVSPF setting 
	ISPAPB0_REG8(0x2171) = 0x7f; // enable Y 5x5 edge 
	ISPAPB0_REG8(0x2172) = 0x21; // enable UV SRAM replace mode
	ISPAPB0_REG8(0x2173) = 0x73; // enable Y SRAM replace
	ISPAPB0_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion
	ISPAPB0_REG8(0x2175) = 0x04; // YFuLThr 
	ISPAPB0_REG8(0x2176) = 0x08; // YFuHThr 
	ISPAPB0_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB0_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB0_REG8(0x2179) = 0x51; // enable YHdn and factor: 256
	ISPAPB0_REG8(0x217a) = 0x55; // YHdn position initialization
	ISPAPB0_REG8(0x217b) = 0x00;
	ISPAPB0_REG8(0x217c) = 0xf2; // YHdn right boundary
	ISPAPB0_REG8(0x217d) = 0x06;
	ISPAPB0_REG8(0x217e) = 0x0c; // YHdn low threshold
	ISPAPB0_REG8(0x217f) = 0x18; // YHdn high threshold
	ISPAPB0_REG8(0x2180) = 0x36; // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1
	ISPAPB0_REG8(0x2181) = 0x90; // Fe00 and Fe01
	ISPAPB0_REG8(0x2182) = 0x99; // Fe02 and Fe11
	ISPAPB0_REG8(0x2183) = 0x52; // Fe12 and Fe22
	ISPAPB0_REG8(0x2184) = 0x02; // Fea
	ISPAPB0_REG8(0x2185) = 0x20; // Y 5x5 edge gain
	ISPAPB0_REG8(0x2186) = 0x24;
	ISPAPB0_REG8(0x2187) = 0x2a;
	ISPAPB0_REG8(0x2188) = 0x24;
	ISPAPB0_REG8(0x2189) = 0x22;
	ISPAPB0_REG8(0x218a) = 0x1e;
	ISPAPB0_REG8(0x218b) = 0x08; // Y 5x5 edge threshold 
	ISPAPB0_REG8(0x218c) = 0x10;
	ISPAPB0_REG8(0x218d) = 0x18;
	ISPAPB0_REG8(0x218e) = 0x20;
	ISPAPB0_REG8(0x218f) = 0x40;
	ISPAPB0_REG8(0x2190) = 0x02; // Y 5x5 edge coring value 
	ISPAPB0_REG8(0x2191) = 0x03;
	ISPAPB0_REG8(0x2192) = 0x04;
	ISPAPB0_REG8(0x2193) = 0x05;
	ISPAPB0_REG8(0x2194) = 0x05;
	ISPAPB0_REG8(0x2195) = 0x05;
	ISPAPB0_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold
	ISPAPB0_REG8(0x2197) = 0x40;
	ISPAPB0_REG8(0x2198) = 0x80;
	ISPAPB0_REG8(0x2199) = 0xa0;
	ISPAPB0_REG8(0x219a) = 0xc0;
	ISPAPB0_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB0_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB0_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB0_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold 
	ISPAPB0_REG8(0x219f) = 0x04; // Ysr weight 
	ISPAPB0_REG8(0x21b8) = 0x00; // disable H/V scale down
	ISPAPB0_REG8(0x21ba) = 0x03; // 2D YUV scaling before YUV spf 
	ISPAPB0_REG8(0x21c0) = 0x03; // enable bchs/scale 
	ISPAPB0_REG8(0x21c1) = 0x00; // Y brightness
	ISPAPB0_REG8(0x21c2) = 0x1c; // Y contrast 
	ISPAPB0_REG8(0x21c3) = 0x0f; // Hue SIN data
	ISPAPB0_REG8(0x21c4) = 0x3e; // Hue COS data
	ISPAPB0_REG8(0x21c5) = 0x28; // Hue saturation 
	ISPAPB0_REG8(0x21c6) = 0x05; // Y offset
	ISPAPB0_REG8(0x21c7) = 0x80; // Y center
	ISPAPB0_REG8(0x21ce) = 0x3a; // LC Gn gain 
	ISPAPB0_REG8(0x21d1) = 0x00; // use CDSP
	ISPAPB0_REG8(0x21d2) = 0x00; // YUV422
	ISPAPB0_REG8(0x3100) = 0x70; // enable new WB offset/gain and offset after gain
	ISPAPB0_REG8(0x3101) = 0x03; // R new WB offset: 5
	ISPAPB0_REG8(0x3102) = 0x01; // G new WB offset: 84
	ISPAPB0_REG8(0x3103) = 0x02; // B new WB offset: 16
	ISPAPB0_REG8(0x3104) = 0x3f; // R new WB gain: 22 
	ISPAPB0_REG8(0x3105) = 0x00;
	ISPAPB0_REG8(0x3106) = 0x3d; // G new WB gain: 0
	ISPAPB0_REG8(0x3107) = 0x00;
	ISPAPB0_REG8(0x3108) = 0x44; // B new WB gain: 56 
	ISPAPB0_REG8(0x3109) = 0x00;
	ISPAPB0_REG8(0x317a) = 0x22; // enable rgbedgeen mul 2 div 1 enable rgbmode
	ISPAPB0_REG8(0x317b) = 0x0a; // rgbedgelothr
	ISPAPB0_REG8(0x317c) = 0x28; // rgbedgehithr
	ISPAPB0_REG8(0x31af) = 0x03; // enable HDR H/V smoothing mode 
	ISPAPB0_REG8(0x31b0) = 0x10; // hdrsmlthr
	ISPAPB0_REG8(0x31b1) = 0x40; // hdrsmlmax
	ISPAPB0_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0 
	ISPAPB0_REG8(0x31c1) = 0x09; // sFe00 and sFe01
	ISPAPB0_REG8(0x31c2) = 0x14; // sFea and Fe11
	ISPAPB0_REG8(0x31c3) = 0x00; // Y 3x3 edge gain
	ISPAPB0_REG8(0x31c4) = 0x04;
	ISPAPB0_REG8(0x31c5) = 0x0c;
	ISPAPB0_REG8(0x31c6) = 0x12;
	ISPAPB0_REG8(0x31c7) = 0x16;
	ISPAPB0_REG8(0x31c8) = 0x18;
	ISPAPB0_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold 
	ISPAPB0_REG8(0x31ca) = 0x0c;
	ISPAPB0_REG8(0x31cb) = 0x10;
	ISPAPB0_REG8(0x31cc) = 0x18;
	ISPAPB0_REG8(0x31cd) = 0x28;
	ISPAPB0_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn
	ISPAPB0_REG8(0x31cf) = 0x08; // Y77StdThr
	ISPAPB0_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125
	ISPAPB0_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106
	ISPAPB0_REG8(0x31d2) = 0x62; // YDifMinBThr1:98
	ISPAPB0_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77
	ISPAPB0_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB0_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB0_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB0_REG8(0x31d7) = 0x64; // YStdMMean
	ISPAPB0_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB0_REG8(0x31d9) = 0x8c; // YStdHMean
	ISPAPB0_REG8(0x31f0) = 0x24; // YLowSel 
	ISPAPB0_REG8(0x31f3) = 0x04; // YEtrMMin
	ISPAPB0_REG8(0x31f4) = 0x03; // YEtrMMax
	ISPAPB0_REG8(0x31f5) = 0x06; // YEtrHMin
	ISPAPB0_REG8(0x31f6) = 0x05; // YEtrHMax
	ISPAPB0_REG8(0x31f8) = 0x01; // enbale Irhigtmin
	ISPAPB0_REG8(0x31f9) = 0x06; // enbale Irhigtmin_r
	ISPAPB0_REG8(0x31fa) = 0x09; // enbale Irhigtmin_b
	ISPAPB0_REG8(0x31fb) = 0xf8; // Irhighthr
	ISPAPB0_REG8(0x31fc) = 0xff; // Irhratio_r 
	ISPAPB0_REG8(0x31fd) = 0xf8; // Irhratio_b 
	ISPAPB0_REG8(0x31fe) = 0x0a; // UVThr1
	ISPAPB0_REG8(0x31ff) = 0x14; // UVThr2

	/* ISP1 Configuration */
	// clock setting
	ISPAPB1_REG8(0x2008) = 0x07;
	ISPAPB1_REG8(0x275e) = 0x05;
	ISPAPB1_REG8(0x275b) = 0x01;
	//
	// CDSP register setting 
	ISPAPB1_REG8(0x2106) = 0x00; // pixel and line switch
	ISPAPB1_REG8(0x2107) = 0x20;
	ISPAPB1_REG8(0x2108) = 0x03; // enable manual OB
	ISPAPB1_REG8(0x2109) = 0x00;
	ISPAPB1_REG8(0x210d) = 0x00; // vertical mirror line: original
	ISPAPB1_REG8(0x210e) = 0x00; // double buffer be set immediately
	ISPAPB1_REG8(0x210f) = 0x00; // without sync vd
	ISPAPB1_REG8(0x2110) = 0xe9; // enable global gain, LchStep=32, LcvStep=64
	ISPAPB1_REG8(0x2111) = 0x01; // LcHinc
	ISPAPB1_REG8(0x2112) = 0x01; // LcVinc
	ISPAPB1_REG8(0x2113) = 0x42; // LC Rgain
	ISPAPB1_REG8(0x2114) = 0x34; // LC Ggain
	ISPAPB1_REG8(0x2115) = 0x3c; // LC Bgain
	ISPAPB1_REG8(0x2116) = 0x08; // LC Xoffset 
	ISPAPB1_REG8(0x2117) = 0x00;
	ISPAPB1_REG8(0x2118) = 0x18; // LC Yoffset 
	ISPAPB1_REG8(0x2119) = 0x00;
	ISPAPB1_REG8(0x211a) = 0x89; // Centvoffset=8 Centhoffset=9
	ISPAPB1_REG8(0x211b) = 0x9a; // Centvsize=9 Centhsize=10
	ISPAPB1_REG8(0x211c) = 0x32; // Quan_n=3 Quan_m=2
	ISPAPB1_REG8(0x211d) = 0x89; // rbactthr
	ISPAPB1_REG8(0x211e) = 0x04; // low bad pixel threshold 
	ISPAPB1_REG8(0x211f) = 0xf7; // high bad pixel threshold 
	ISPAPB1_REG8(0x2120) = 0x03; // enable bad pixel
	ISPAPB1_REG8(0x2121) = 0x01; // enable bad pixel replacement
	ISPAPB1_REG8(0x2124) = 0x00; // HdrmapMode=0
	ISPAPB1_REG8(0x2125) = 0x44; // enable HDR saturation
	ISPAPB1_REG8(0x2126) = 0xf0; // disable HDR
	ISPAPB1_REG8(0x2127) = 0x77; // HdrFac1: 5 and HdrFac2: 6
	ISPAPB1_REG8(0x2128) = 0x77; // HdrFac3: 8 and HdrFac4: 7
	ISPAPB1_REG8(0x2129) = 0x77; // HdrFac5: 9 and HdrFac6: 8
	ISPAPB1_REG8(0x212a) = 0x00; // HdrGain0: (34/64)
	ISPAPB1_REG8(0x212b) = 0x00; // HdrGain1: (51/64)
	ISPAPB1_REG8(0x212c) = 0x00; // HdrGain2: (68/64)
	ISPAPB1_REG8(0x212d) = 0x00; // HdrGain3: (16/64)
	ISPAPB1_REG8(0x212e) = 0x00; // HdrGain4: (02/64)
	ISPAPB1_REG8(0x212f) = 0x00; // HdrGain5: (01/64)
	ISPAPB1_REG8(0x2130) = 0x40; // R WB gain: 
	ISPAPB1_REG8(0x2131) = 0x00;
	ISPAPB1_REG8(0x2132) = 0x00; // R WB offset:
	ISPAPB1_REG8(0x2134) = 0x40; // Gr WB gain:
	ISPAPB1_REG8(0x2135) = 0x00;
	ISPAPB1_REG8(0x2136) = 0x00; // Gr WB offset:
	ISPAPB1_REG8(0x2138) = 0x40; // B WB gain: 
	ISPAPB1_REG8(0x2139) = 0x00;
	ISPAPB1_REG8(0x213a) = 0x00; // B WB offset:
	ISPAPB1_REG8(0x213c) = 0x40; // Gb WB gain:
	ISPAPB1_REG8(0x213d) = 0x00;
	ISPAPB1_REG8(0x213e) = 0x00; // Gb WB offset:
	ISPAPB1_REG8(0x213f) = 0x70; // WB enable
	ISPAPB1_REG8(0x2140) = 0x13; // disable mask
	ISPAPB1_REG8(0x2141) = 0x57; // H/V edge threshold
	ISPAPB1_REG8(0x2142) = 0x06; // ir4x4 type 
	ISPAPB1_REG8(0x2143) = 0x11; // Fpncmpen
	ISPAPB1_REG8(0x2144) = 0x13; // FPNgain 
	ISPAPB1_REG8(0x2145) = 0x02; // FPNhoffset 
	ISPAPB1_REG8(0x2146) = 0x00; // 
	ISPAPB1_REG8(0x2147) = 0x05; // irfacY
	ISPAPB1_REG8(0x2148) = 0x46; // color matrix setting 
	ISPAPB1_REG8(0x2149) = 0x00;
	ISPAPB1_REG8(0x214a) = 0xfc;
	ISPAPB1_REG8(0x214b) = 0x01;
	ISPAPB1_REG8(0x214c) = 0xfe;
	ISPAPB1_REG8(0x214d) = 0x01;
	ISPAPB1_REG8(0x214e) = 0xf7;
	ISPAPB1_REG8(0x214f) = 0x01;
	ISPAPB1_REG8(0x2150) = 0x4c;
	ISPAPB1_REG8(0x2151) = 0x00;
	ISPAPB1_REG8(0x2152) = 0xfd;
	ISPAPB1_REG8(0x2153) = 0x01;
	ISPAPB1_REG8(0x2154) = 0x00;
	ISPAPB1_REG8(0x2155) = 0x00;
	ISPAPB1_REG8(0x2156) = 0xf1;
	ISPAPB1_REG8(0x2157) = 0x01;
	ISPAPB1_REG8(0x2158) = 0x4f;
	ISPAPB1_REG8(0x2159) = 0x00;
	ISPAPB1_REG8(0x215c) = 0x01; // enable post gamma 
	ISPAPB1_REG8(0x215d) = 0x01; // enable dither
	ISPAPB1_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB1_REG8(0x2160) = 0x0a; // Y LUT LPF step 
	ISPAPB1_REG8(0x2161) = 0x10; // Y LUT value
	ISPAPB1_REG8(0x2162) = 0x20;
	ISPAPB1_REG8(0x2163) = 0x30;
	ISPAPB1_REG8(0x2164) = 0x40;
	ISPAPB1_REG8(0x2165) = 0x50;
	ISPAPB1_REG8(0x2166) = 0x60;
	ISPAPB1_REG8(0x2167) = 0x70;
	ISPAPB1_REG8(0x2168) = 0x80;
	ISPAPB1_REG8(0x2169) = 0x90;
	ISPAPB1_REG8(0x216a) = 0xa0;
	ISPAPB1_REG8(0x216b) = 0xb0;
	ISPAPB1_REG8(0x216c) = 0xc0;
	ISPAPB1_REG8(0x216d) = 0xd0;
	ISPAPB1_REG8(0x216e) = 0xe0;
	ISPAPB1_REG8(0x216f) = 0xf0;
	ISPAPB1_REG8(0x2170) = 0x13; // YUVSPF setting 
	ISPAPB1_REG8(0x2171) = 0x7f; // enable Y 5x5 edge 
	ISPAPB1_REG8(0x2172) = 0x21; // enable UV SRAM replace mode
	ISPAPB1_REG8(0x2173) = 0x73; // enable Y SRAM replace
	ISPAPB1_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion
	ISPAPB1_REG8(0x2175) = 0x04; // YFuLThr 
	ISPAPB1_REG8(0x2176) = 0x08; // YFuHThr 
	ISPAPB1_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB1_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB1_REG8(0x2179) = 0x51; // enable YHdn and factor: 256
	ISPAPB1_REG8(0x217a) = 0x55; // YHdn position initialization
	ISPAPB1_REG8(0x217b) = 0x00;
	ISPAPB1_REG8(0x217c) = 0xf2; // YHdn right boundary
	ISPAPB1_REG8(0x217d) = 0x06;
	ISPAPB1_REG8(0x217e) = 0x0c; // YHdn low threshold
	ISPAPB1_REG8(0x217f) = 0x18; // YHdn high threshold
	ISPAPB1_REG8(0x2180) = 0x36; // YEdgeGainMode: 0, YEdgeCorMode: 1, YEdgeCentSel: 1
	ISPAPB1_REG8(0x2181) = 0x90; // Fe00 and Fe01
	ISPAPB1_REG8(0x2182) = 0x99; // Fe02 and Fe11
	ISPAPB1_REG8(0x2183) = 0x52; // Fe12 and Fe22
	ISPAPB1_REG8(0x2184) = 0x02; // Fea
	ISPAPB1_REG8(0x2185) = 0x20; // Y 5x5 edge gain
	ISPAPB1_REG8(0x2186) = 0x24;
	ISPAPB1_REG8(0x2187) = 0x2a;
	ISPAPB1_REG8(0x2188) = 0x24;
	ISPAPB1_REG8(0x2189) = 0x22;
	ISPAPB1_REG8(0x218a) = 0x1e;
	ISPAPB1_REG8(0x218b) = 0x08; // Y 5x5 edge threshold 
	ISPAPB1_REG8(0x218c) = 0x10;
	ISPAPB1_REG8(0x218d) = 0x18;
	ISPAPB1_REG8(0x218e) = 0x20;
	ISPAPB1_REG8(0x218f) = 0x40;
	ISPAPB1_REG8(0x2190) = 0x02; // Y 5x5 edge coring value 
	ISPAPB1_REG8(0x2191) = 0x03;
	ISPAPB1_REG8(0x2192) = 0x04;
	ISPAPB1_REG8(0x2193) = 0x05;
	ISPAPB1_REG8(0x2194) = 0x05;
	ISPAPB1_REG8(0x2195) = 0x05;
	ISPAPB1_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold
	ISPAPB1_REG8(0x2197) = 0x40;
	ISPAPB1_REG8(0x2198) = 0x80;
	ISPAPB1_REG8(0x2199) = 0xa0;
	ISPAPB1_REG8(0x219a) = 0xc0;
	ISPAPB1_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB1_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB1_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB1_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold 
	ISPAPB1_REG8(0x219f) = 0x04; // Ysr weight 
	ISPAPB1_REG8(0x21b8) = 0x00; // disable H/V scale down
	ISPAPB1_REG8(0x21ba) = 0x03; // 2D YUV scaling before YUV spf 
	ISPAPB1_REG8(0x21c0) = 0x03; // enable bchs/scale 
	ISPAPB1_REG8(0x21c1) = 0x00; // Y brightness
	ISPAPB1_REG8(0x21c2) = 0x1c; // Y contrast 
	ISPAPB1_REG8(0x21c3) = 0x0f; // Hue SIN data
	ISPAPB1_REG8(0x21c4) = 0x3e; // Hue COS data
	ISPAPB1_REG8(0x21c5) = 0x28; // Hue saturation 
	ISPAPB1_REG8(0x21c6) = 0x05; // Y offset
	ISPAPB1_REG8(0x21c7) = 0x80; // Y center
	ISPAPB1_REG8(0x21ce) = 0x3a; // LC Gn gain 
	ISPAPB1_REG8(0x21d1) = 0x00; // use CDSP
	ISPAPB1_REG8(0x21d2) = 0x00; // YUV422
	ISPAPB1_REG8(0x3100) = 0x70; // enable new WB offset/gain and offset after gain
	ISPAPB1_REG8(0x3101) = 0x03; // R new WB offset: 5
	ISPAPB1_REG8(0x3102) = 0x01; // G new WB offset: 84
	ISPAPB1_REG8(0x3103) = 0x02; // B new WB offset: 16
	ISPAPB1_REG8(0x3104) = 0x3f; // R new WB gain: 22 
	ISPAPB1_REG8(0x3105) = 0x00;
	ISPAPB1_REG8(0x3106) = 0x3d; // G new WB gain: 0
	ISPAPB1_REG8(0x3107) = 0x00;
	ISPAPB1_REG8(0x3108) = 0x44; // B new WB gain: 56 
	ISPAPB1_REG8(0x3109) = 0x00;
	ISPAPB1_REG8(0x317a) = 0x22; // enable rgbedgeen mul 2 div 1 enable rgbmode
	ISPAPB1_REG8(0x317b) = 0x0a; // rgbedgelothr
	ISPAPB1_REG8(0x317c) = 0x28; // rgbedgehithr
	ISPAPB1_REG8(0x31af) = 0x03; // enable HDR H/V smoothing mode 
	ISPAPB1_REG8(0x31b0) = 0x10; // hdrsmlthr
	ISPAPB1_REG8(0x31b1) = 0x40; // hdrsmlmax
	ISPAPB1_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0 
	ISPAPB1_REG8(0x31c1) = 0x09; // sFe00 and sFe01
	ISPAPB1_REG8(0x31c2) = 0x14; // sFea and Fe11
	ISPAPB1_REG8(0x31c3) = 0x00; // Y 3x3 edge gain
	ISPAPB1_REG8(0x31c4) = 0x04;
	ISPAPB1_REG8(0x31c5) = 0x0c;
	ISPAPB1_REG8(0x31c6) = 0x12;
	ISPAPB1_REG8(0x31c7) = 0x16;
	ISPAPB1_REG8(0x31c8) = 0x18;
	ISPAPB1_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold 
	ISPAPB1_REG8(0x31ca) = 0x0c;
	ISPAPB1_REG8(0x31cb) = 0x10;
	ISPAPB1_REG8(0x31cc) = 0x18;
	ISPAPB1_REG8(0x31cd) = 0x28;
	ISPAPB1_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn
	ISPAPB1_REG8(0x31cf) = 0x08; // Y77StdThr
	ISPAPB1_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125
	ISPAPB1_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106
	ISPAPB1_REG8(0x31d2) = 0x62; // YDifMinBThr1:98
	ISPAPB1_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77
	ISPAPB1_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB1_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB1_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB1_REG8(0x31d7) = 0x64; // YStdMMean
	ISPAPB1_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB1_REG8(0x31d9) = 0x8c; // YStdHMean
	ISPAPB1_REG8(0x31f0) = 0x24; // YLowSel 
	ISPAPB1_REG8(0x31f3) = 0x04; // YEtrMMin
	ISPAPB1_REG8(0x31f4) = 0x03; // YEtrMMax
	ISPAPB1_REG8(0x31f5) = 0x06; // YEtrHMin
	ISPAPB1_REG8(0x31f6) = 0x05; // YEtrHMax
	ISPAPB1_REG8(0x31f8) = 0x01; // enbale Irhigtmin
	ISPAPB1_REG8(0x31f9) = 0x06; // enbale Irhigtmin_r
	ISPAPB1_REG8(0x31fa) = 0x09; // enbale Irhigtmin_b
	ISPAPB1_REG8(0x31fb) = 0xf8; // Irhighthr
	ISPAPB1_REG8(0x31fc) = 0xff; // Irhratio_r 
	ISPAPB1_REG8(0x31fd) = 0xf8; // Irhratio_b 
	ISPAPB1_REG8(0x31fe) = 0x0a; // UVThr1
	ISPAPB1_REG8(0x31ff) = 0x14; // UVThr2
}

/*
	@ispAaaInit
*/
void ispAaaInit_yuv422(void)
{
}

void ispReset(u8 pattern)
{
	switch (pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			ispReset_yuv422();
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			ispReset_raw10();
			break;
	}
}

void FrontInit(u8 pattern,u16 xlen,u16 ylen, u8 probe)
{
	u8 sigmode = 0x00;

	switch (pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			ISPAPB_LOGI("still vertical color bar\n");
			FrontInit_yuv422(xlen, ylen, probe);
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
			ISPAPB_LOGI("still vertical color bar\n");
			sigmode = 0xbc;     // siggen enable(hvalidcnt): 75% still vertical color bar
			FrontInit_raw10(xlen, ylen, sigmode, probe);
			break;

		case MOVING_COLORBAR_RAW10_1920X1080:
			ISPAPB_LOGI("moving horizontal color bar\n");
			sigmode = 0xbf;     // siggen enable(hvalidcnt): 75% moving horizontal color bar
			FrontInit_raw10(xlen, ylen, sigmode, probe);
			break;

	}
}

void cdspSetTable(u8 pattern)
{
	switch(pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			cdspSetTable_yuv422();
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			cdspSetTable_raw10();
			break;
	}
}

void sensorInit(u8 pattern)
{
	switch(pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			sensorInit_yuv422();
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			sensorInit_raw10();
			break;
	}
}

void CdspInit(u8 pattern, u8 output, u8 scale)
{
	switch (pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			CdspInit_yuv422();
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			CdspInit_raw10(output, scale);
			break;
	}
}

void ispAaaInit(u8 pattern)
{
	switch (pattern)
	{
		case STILL_COLORBAR_YUV422_1920X1080:
			ispAaaInit_yuv422();
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			ispAaaInit_raw10();
			break;
	}
}

void isp_64x64(u16 xlen, u16 ylen)
{
	/* ISP0 Configuration */
	ISPAPB0_REG8(0x2000) = 0x13; // reset all module include ispck
	ISPAPB0_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB0_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB0_REG8(0x2008) = 0x05; // switch b1xck/bpclk_nx to normal clocks
	ISPAPB0_REG8(0x2000) = 0x03; // release ispck reset

	ispSleep();

	//#(`TIMEBASE*20;
	ISPAPB0_REG8(0x2000) = 0x00; // release all module reset

	ISPAPB0_REG8(0x276c) = 0x01; // reset front
	ISPAPB0_REG8(0x276c) = 0x00; //


	ISPAPB0_REG8(0x2000) = 0x03;
	ISPAPB0_REG8(0x2000) = 0x00;

	ISPAPB0_REG8(0x275e) = 0x03; // TG clock selection
	ISPAPB0_REG8(0x2785) = 0x08; // clk2x output enable
	ISPAPB0_REG8(0x2790) = 0x73; // disable extvdi/exthdi gated to zero
	ISPAPB0_REG8(0x2008) = 0x07; // b1xck) b2xck and pclk_nx to sensor related clocks
	ISPAPB0_REG8(0x276c) = 0x01; // sofware reset Front control circuits
	ISPAPB0_REG8(0x276c) = 0x00;
	ISPAPB0_REG8(0x21d0) = 0x01; // sofware reset CDSP interface (active)

	ISPAPB0_REG8(0x2106) = 0x02; // pixel and line switch
	ISPAPB0_REG8(0x2107) = 0x00; 

	ISPAPB0_REG8(0x210d) = 0x00; // vertical mirror line: original

	ISPAPB0_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB0_REG8(0x2160) = 0x0a; // Y LUT LPF step
	ISPAPB0_REG8(0x2161) = 0x07; // Y LUT value
	ISPAPB0_REG8(0x2162) = 0x15;
	ISPAPB0_REG8(0x2163) = 0x24;
	ISPAPB0_REG8(0x2164) = 0x36;
	ISPAPB0_REG8(0x2165) = 0x4a;
	ISPAPB0_REG8(0x2166) = 0x61;
	ISPAPB0_REG8(0x2167) = 0x6f;
	ISPAPB0_REG8(0x2168) = 0x80;
	ISPAPB0_REG8(0x2169) = 0x8f;
	ISPAPB0_REG8(0x216a) = 0xa1;
	ISPAPB0_REG8(0x216b) = 0xb0;
	ISPAPB0_REG8(0x216c) = 0xc0;
	ISPAPB0_REG8(0x216d) = 0xcf;
	ISPAPB0_REG8(0x216e) = 0xe0;
	ISPAPB0_REG8(0x216f) = 0xf0;

	ISPAPB0_REG8(0x2170) = 0x13; // YUVSPF setting
	ISPAPB0_REG8(0x2171) = 0x7f; // enable Y 5x5 edge
                                 // enable Y SDN
                                 // enable UV SDN
                                 // enable Y 3x3 edge
                                 // enable Y supper resolution
                                 // ylpmaxminsel Y33
                                 // enable yertmode
	ISPAPB0_REG8(0x2172) = 0x21; // enable UV SRAM replace mode
														   // UV SDN mode 2
	ISPAPB0_REG8(0x2173) = 0x73; // enable Y SRAM replace
                                 // enable Y extra cropping
                                 // enable all Y supper resolution fusion
	ISPAPB0_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion
                                 // enable all Y 3x3 edge fusion
	ISPAPB0_REG8(0x2175) = 0x04; // YFuLThr
	ISPAPB0_REG8(0x2176) = 0x08; // YFuHThr
	ISPAPB0_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB0_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB0_REG8(0x2179) = 0x00; // disable YHdn and factor: 64
	ISPAPB0_REG8(0x2180) = 0x36; // YEdgeGainMode: 0) YEdgeCorMode: 1) YEdgeCentSel: 1
                                 // enable Y 5x5 edge coring process
                                 // enable Y 5x5 edge overshooting process
	ISPAPB0_REG8(0x2181) = 0x90; // Fe00 and Fe01
	ISPAPB0_REG8(0x2182) = 0x99; // Fe02 and Fe11
	ISPAPB0_REG8(0x2183) = 0x52; // Fe12 and Fe22
	ISPAPB0_REG8(0x2184) = 0x02; // Fea
	ISPAPB0_REG8(0x2185) = 0x20; // Y 5x5 edge gain
	ISPAPB0_REG8(0x2186) = 0x24;
	ISPAPB0_REG8(0x2187) = 0x2a;
	ISPAPB0_REG8(0x2188) = 0x24;
	ISPAPB0_REG8(0x2189) = 0x22;
	ISPAPB0_REG8(0x218a) = 0x1e;
	ISPAPB0_REG8(0x218b) = 0x08; // Y 5x5 edge threshold
	ISPAPB0_REG8(0x218c) = 0x10;
	ISPAPB0_REG8(0x218d) = 0x18;
	ISPAPB0_REG8(0x218e) = 0x20;
	ISPAPB0_REG8(0x218f) = 0x40;
	ISPAPB0_REG8(0x2190) = 0x02; // Y 5x5 edge coring value
	ISPAPB0_REG8(0x2191) = 0x03;
	ISPAPB0_REG8(0x2192) = 0x04;
	ISPAPB0_REG8(0x2193) = 0x05;
	ISPAPB0_REG8(0x2194) = 0x05;
	ISPAPB0_REG8(0x2195) = 0x05;
	ISPAPB0_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold
	ISPAPB0_REG8(0x2197) = 0x40;
	ISPAPB0_REG8(0x2198) = 0x80;
	ISPAPB0_REG8(0x2199) = 0xa0;
	ISPAPB0_REG8(0x219a) = 0xc0;
	ISPAPB0_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB0_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB0_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB0_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold
	ISPAPB0_REG8(0x219f) = 0x04; // Ysr weight

	ISPAPB0_REG8(0x21b8) = 0x00; // disable H/V scaling down and drop mode

	ISPAPB0_REG8(0x21ba) = 0x00; // WBO before lens compensation
                                 // 2D YUV scaling after Y global tone

	ISPAPB0_REG8(0x21c0) = 0x73; // enable bchs/scale/crop
	ISPAPB0_REG8(0x21c1) = 0x02; // Y brightness
	ISPAPB0_REG8(0x21c2) = 0x24; // Y contrast
	ISPAPB0_REG8(0x21c3) = 0x1f; // Hue SIN data
	ISPAPB0_REG8(0x21c4) = 0x37; // Hue COS data
	ISPAPB0_REG8(0x21c5) = 0x28; // Hue saturation
	ISPAPB0_REG8(0x21c6) = 0x1e; // Y offset
	ISPAPB0_REG8(0x21c7) = 0x08; // Y center

	ISPAPB0_REG8(0x21d1) = 0x00; // use CDSP
	ISPAPB0_REG8(0x21d2) = 0x00; // YUV422

	ISPAPB0_REG8(0x3182) = 0x01; // sharing buffer 

	ISPAPB0_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0
	ISPAPB0_REG8(0x31c1) = 0x09; // sFe00 and sFe01
	ISPAPB0_REG8(0x31c2) = 0x14; // sFea and Fe11
	ISPAPB0_REG8(0x31c3) = 0x00; // Y 3x3 edge gain
	ISPAPB0_REG8(0x31c4) = 0x04;
	ISPAPB0_REG8(0x31c5) = 0x0c;
	ISPAPB0_REG8(0x31c6) = 0x12;
	ISPAPB0_REG8(0x31c7) = 0x16;
	ISPAPB0_REG8(0x31c8) = 0x18;
	ISPAPB0_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold
	ISPAPB0_REG8(0x31ca) = 0x0c;
	ISPAPB0_REG8(0x31cb) = 0x10;
	ISPAPB0_REG8(0x31cc) = 0x18;
	ISPAPB0_REG8(0x31cd) = 0x28;
	ISPAPB0_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn
                                 // enable spfblkpgoen
	ISPAPB0_REG8(0x31cf) = 0x08; // Y77StdThr
	ISPAPB0_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125
	ISPAPB0_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106
	ISPAPB0_REG8(0x31d2) = 0x62; // YDifMinBThr1:98
	ISPAPB0_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77
	ISPAPB0_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB0_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB0_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB0_REG8(0x31d7) = 0x64; // YStdMMean
	ISPAPB0_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB0_REG8(0x31d9) = 0x8c; // YStdHMean
	ISPAPB0_REG8(0x31f3) = 0x04; // YEtrMMin  
	ISPAPB0_REG8(0x31f4) = 0x03; // YEtrMMax  
	ISPAPB0_REG8(0x31f5) = 0x06; // YEtrHMin  
	ISPAPB0_REG8(0x31f6) = 0x05; // YEtrHMax  
	ISPAPB0_REG8(0x31fe) = 0x0a; // UVThr1
	ISPAPB0_REG8(0x31ff) = 0x14; // UVThr2

	ISPAPB0_REG8(0x2711) = 0x10; // hfall
	ISPAPB0_REG8(0x2712) = 0x00;
	ISPAPB0_REG8(0x2713) = 0x60; // hrise
	ISPAPB0_REG8(0x2714) = 0x00;

	ISPAPB0_REG8(0x2715) = 0x50; // vfall
	ISPAPB0_REG8(0x2716) = 0x00;
	ISPAPB0_REG8(0x2717) = 0x90; // vrise
	ISPAPB0_REG8(0x2718) = 0x00;

	ISPAPB0_REG8(0x2710) = 0x33; // H/V reshape enable
                                 // V reshape by continue pclk count

	ISPAPB0_REG8(0x2720) = 0x03; // hoffset
	ISPAPB0_REG8(0x2721) = 0x00;
	ISPAPB0_REG8(0x2722) = 0x03; // voffset
	ISPAPB0_REG8(0x2723) = 0x00;

	ISPAPB0_REG8(0x2724)=(u8)(xlen & 0x00FF); // hsize
	ISPAPB0_REG8(0x2725)=(u8)((xlen>>8) & 0x00FF);
	ISPAPB0_REG8(0x2726)=(u8)(ylen & 0x00FF); // vsize
	ISPAPB0_REG8(0x2727)=(u8)((ylen>>8) & 0x00FF);

	ISPAPB0_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB0_REG8(0x2741) = 0x40; // syngen line total
	ISPAPB0_REG8(0x2742) = 0x01;
	ISPAPB0_REG8(0x2743) = 0x90; // syngen line blank
	ISPAPB0_REG8(0x2744) = 0x00;
	ISPAPB0_REG8(0x2745) = 0x50; // syngen frame total
	ISPAPB0_REG8(0x2746) = 0x00;
	ISPAPB0_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB0_REG8(0x2748) = 0x00;

	ISPAPB0_REG8(0x2749) = 0x03;
	ISPAPB0_REG8(0x274a) = 0x00;

	ISPAPB0_REG8(0x2705) = 0x01; // CCIR601
	ISPAPB0_REG8(0x276a) = 0xfc; // siggen enable(YUV422): 75% still vertical color bar

	ISPAPB0_REG8(0x21d0) = 0x00; // sofware reset CDSP interface (inactive)

	ISPAPB0_REG8(0x21d4); // 0x40
	ISPAPB0_REG8(0x21d5); // 0x00
	ISPAPB0_REG8(0x21d6); // 0x40
	ISPAPB0_REG8(0x21d7); // 0x00

	/* ISP1 Configuration */
	ISPAPB1_REG8(0x2000) = 0x13; // reset all module include ispck
	ISPAPB1_REG8(0x2003) = 0x1c; // enable phase clocks
	ISPAPB1_REG8(0x2005) = 0x07; // enbale p1xck
	ISPAPB1_REG8(0x2008) = 0x05; // switch b1xck/bpclk_nx to normal clocks
	ISPAPB1_REG8(0x2000) = 0x03; // release ispck reset

	ispSleep();

	//#(`TIMEBASE*20;
	ISPAPB1_REG8(0x2000) = 0x00; // release all module reset

	ISPAPB1_REG8(0x276c) = 0x01; // reset front
	ISPAPB1_REG8(0x276c) = 0x00; //


	ISPAPB1_REG8(0x2000) = 0x03;
	ISPAPB1_REG8(0x2000) = 0x00;

	ISPAPB1_REG8(0x275e) = 0x03; // TG clock selection
	ISPAPB1_REG8(0x2785) = 0x08; // clk2x output enable
	ISPAPB1_REG8(0x2790) = 0x73; // disable extvdi/exthdi gated to zero
	ISPAPB1_REG8(0x2008) = 0x07; // b1xck) b2xck and pclk_nx to sensor related clocks
	ISPAPB1_REG8(0x276c) = 0x01; // sofware reset Front control circuits
	ISPAPB1_REG8(0x276c) = 0x00;
	ISPAPB1_REG8(0x21d0) = 0x01; // sofware reset CDSP interface (active)

	ISPAPB1_REG8(0x2106) = 0x02; // pixel and line switch
	ISPAPB1_REG8(0x2107) = 0x00; 

	ISPAPB1_REG8(0x210d) = 0x00; // vertical mirror line: original

	ISPAPB1_REG8(0x215e) = 0x11; // enable Y LUT
	ISPAPB1_REG8(0x2160) = 0x0a; // Y LUT LPF step
	ISPAPB1_REG8(0x2161) = 0x07; // Y LUT value
	ISPAPB1_REG8(0x2162) = 0x15;
	ISPAPB1_REG8(0x2163) = 0x24;
	ISPAPB1_REG8(0x2164) = 0x36;
	ISPAPB1_REG8(0x2165) = 0x4a;
	ISPAPB1_REG8(0x2166) = 0x61;
	ISPAPB1_REG8(0x2167) = 0x6f;
	ISPAPB1_REG8(0x2168) = 0x80;
	ISPAPB1_REG8(0x2169) = 0x8f;
	ISPAPB1_REG8(0x216a) = 0xa1;
	ISPAPB1_REG8(0x216b) = 0xb0;
	ISPAPB1_REG8(0x216c) = 0xc0;
	ISPAPB1_REG8(0x216d) = 0xcf;
	ISPAPB1_REG8(0x216e) = 0xe0;
	ISPAPB1_REG8(0x216f) = 0xf0;

	ISPAPB1_REG8(0x2170) = 0x13; // YUVSPF setting
	ISPAPB1_REG8(0x2171) = 0x7f; // enable Y 5x5 edge
                                 // enable Y SDN
                                 // enable UV SDN
                                 // enable Y 3x3 edge
                                 // enable Y supper resolution
                                 // ylpmaxminsel Y33
                                 // enable yertmode
	ISPAPB1_REG8(0x2172) = 0x21; // enable UV SRAM replace mode
                                 // UV SDN mode 2
	ISPAPB1_REG8(0x2173) = 0x73; // enable Y SRAM replace
                                 // enable Y extra cropping
                                 // enable all Y supper resolution fusion
	ISPAPB1_REG8(0x2174) = 0x77; // enable all Y 5x5 edge fusion
                                 // enable all Y 3x3 edge fusion
	ISPAPB1_REG8(0x2175) = 0x04; // YFuLThr
	ISPAPB1_REG8(0x2176) = 0x08; // YFuHThr
	ISPAPB1_REG8(0x2177) = 0x02; // YEtrLMin
	ISPAPB1_REG8(0x2178) = 0x01; // YEtrLMax
	ISPAPB1_REG8(0x2179) = 0x00; // disable YHdn and factor: 64
	ISPAPB1_REG8(0x2180) = 0x36; // YEdgeGainMode: 0) YEdgeCorMode: 1) YEdgeCentSel: 1
                                 // enable Y 5x5 edge coring process
                                 // enable Y 5x5 edge overshooting process
	ISPAPB1_REG8(0x2181) = 0x90; // Fe00 and Fe01
	ISPAPB1_REG8(0x2182) = 0x99; // Fe02 and Fe11
	ISPAPB1_REG8(0x2183) = 0x52; // Fe12 and Fe22
	ISPAPB1_REG8(0x2184) = 0x02; // Fea
	ISPAPB1_REG8(0x2185) = 0x20; // Y 5x5 edge gain
	ISPAPB1_REG8(0x2186) = 0x24;
	ISPAPB1_REG8(0x2187) = 0x2a;
	ISPAPB1_REG8(0x2188) = 0x24;
	ISPAPB1_REG8(0x2189) = 0x22;
	ISPAPB1_REG8(0x218a) = 0x1e;
	ISPAPB1_REG8(0x218b) = 0x08; // Y 5x5 edge threshold
	ISPAPB1_REG8(0x218c) = 0x10;
	ISPAPB1_REG8(0x218d) = 0x18;
	ISPAPB1_REG8(0x218e) = 0x20;
	ISPAPB1_REG8(0x218f) = 0x40;
	ISPAPB1_REG8(0x2190) = 0x02; // Y 5x5 edge coring value
	ISPAPB1_REG8(0x2191) = 0x03;
	ISPAPB1_REG8(0x2192) = 0x04;
	ISPAPB1_REG8(0x2193) = 0x05;
	ISPAPB1_REG8(0x2194) = 0x05;
	ISPAPB1_REG8(0x2195) = 0x05;
	ISPAPB1_REG8(0x2196) = 0x20; // Y 5x5 edge coring threshold
	ISPAPB1_REG8(0x2197) = 0x40;
	ISPAPB1_REG8(0x2198) = 0x80;
	ISPAPB1_REG8(0x2199) = 0xa0;
	ISPAPB1_REG8(0x219a) = 0xc0;
	ISPAPB1_REG8(0x219b) = 0x08; // PEdgeThr
	ISPAPB1_REG8(0x219c) = 0x10; // NEdgeThr
	ISPAPB1_REG8(0x219d) = 0x64; // PEdgeSkR and NEdgeSkR
	ISPAPB1_REG8(0x219e) = 0x08; // Ysr Sobel filter threshold
	ISPAPB1_REG8(0x219f) = 0x04; // Ysr weight

	ISPAPB1_REG8(0x21b8) = 0x00; // disable H/V scaling down and drop mode

	ISPAPB1_REG8(0x21ba) = 0x00; // WBO before lens compensation
                                 // 2D YUV scaling after Y global tone

	ISPAPB1_REG8(0x21c0) = 0x73; // enable bchs/scale/crop
	ISPAPB1_REG8(0x21c1) = 0x02; // Y brightness
	ISPAPB1_REG8(0x21c2) = 0x24; // Y contrast
	ISPAPB1_REG8(0x21c3) = 0x1f; // Hue SIN data
	ISPAPB1_REG8(0x21c4) = 0x37; // Hue COS data
	ISPAPB1_REG8(0x21c5) = 0x28; // Hue saturation
	ISPAPB1_REG8(0x21c6) = 0x1e; // Y offset
	ISPAPB1_REG8(0x21c7) = 0x08; // Y center

	ISPAPB1_REG8(0x21d1) = 0x00; // use CDSP
	ISPAPB1_REG8(0x21d2) = 0x00; // YUV422

	ISPAPB1_REG8(0x3182) = 0x01; // sharing buffer 

	ISPAPB1_REG8(0x31c0) = 0x00; // sYEdgeGainMode: 0
	ISPAPB1_REG8(0x31c1) = 0x09; // sFe00 and sFe01
	ISPAPB1_REG8(0x31c2) = 0x14; // sFea and Fe11
	ISPAPB1_REG8(0x31c3) = 0x00; // Y 3x3 edge gain
	ISPAPB1_REG8(0x31c4) = 0x04;
	ISPAPB1_REG8(0x31c5) = 0x0c;
	ISPAPB1_REG8(0x31c6) = 0x12;
	ISPAPB1_REG8(0x31c7) = 0x16;
	ISPAPB1_REG8(0x31c8) = 0x18;
	ISPAPB1_REG8(0x31c9) = 0x08; // Y 3x3 edge threshold
	ISPAPB1_REG8(0x31ca) = 0x0c;
	ISPAPB1_REG8(0x31cb) = 0x10;
	ISPAPB1_REG8(0x31cc) = 0x18;
	ISPAPB1_REG8(0x31cd) = 0x28;
	ISPAPB1_REG8(0x31ce) = 0x03; // enable SpfBlkDatEn
                                 // enable spfblkpgoen
	ISPAPB1_REG8(0x31cf) = 0x08; // Y77StdThr
	ISPAPB1_REG8(0x31d0) = 0x7d; // YDifMinSThr1:125
	ISPAPB1_REG8(0x31d1) = 0x6a; // YDifMinSThr2:106
	ISPAPB1_REG8(0x31d2) = 0x62; // YDifMinBThr1:98
	ISPAPB1_REG8(0x31d3) = 0x4d; // YDifMinBThr2:77
	ISPAPB1_REG8(0x31d4) = 0x12; // YStdLDiv and YStdMDiv
	ISPAPB1_REG8(0x31d5) = 0x00; // YStdHDiv
	ISPAPB1_REG8(0x31d6) = 0x78; // YStdMThr
	ISPAPB1_REG8(0x31d7) = 0x64; // YStdMMean
	ISPAPB1_REG8(0x31d8) = 0x82; // YStdHThr
	ISPAPB1_REG8(0x31d9) = 0x8c; // YStdHMean
	ISPAPB1_REG8(0x31f3) = 0x04; // YEtrMMin  
	ISPAPB1_REG8(0x31f4) = 0x03; // YEtrMMax  
	ISPAPB1_REG8(0x31f5) = 0x06; // YEtrHMin  
	ISPAPB1_REG8(0x31f6) = 0x05; // YEtrHMax  
	ISPAPB1_REG8(0x31fe) = 0x0a; // UVThr1
	ISPAPB1_REG8(0x31ff) = 0x14; // UVThr2

	ISPAPB1_REG8(0x2711) = 0x10; // hfall
	ISPAPB1_REG8(0x2712) = 0x00;
	ISPAPB1_REG8(0x2713) = 0x60; // hrise
	ISPAPB1_REG8(0x2714) = 0x00;

	ISPAPB1_REG8(0x2715) = 0x50; // vfall
	ISPAPB1_REG8(0x2716) = 0x00;
	ISPAPB1_REG8(0x2717) = 0x90; // vrise
	ISPAPB1_REG8(0x2718) = 0x00;

	ISPAPB1_REG8(0x2710) = 0x33; // H/V reshape enable
                                 // V reshape by continue pclk count

	ISPAPB1_REG8(0x2720) = 0x03; // hoffset
	ISPAPB1_REG8(0x2721) = 0x00;
	ISPAPB1_REG8(0x2722) = 0x03; // voffset
	ISPAPB1_REG8(0x2723) = 0x00;

	ISPAPB1_REG8(0x2724)=(u8)(xlen & 0x00FF); // hsize
	ISPAPB1_REG8(0x2725)=(u8)((xlen>>8) & 0x00FF);
	ISPAPB1_REG8(0x2726)=(u8)(ylen & 0x00FF); // vsize
	ISPAPB1_REG8(0x2727)=(u8)((ylen>>8) & 0x00FF);

	ISPAPB1_REG8(0x2740) = 0x01; // syngen enable
	ISPAPB1_REG8(0x2741) = 0x40; // syngen line total
	ISPAPB1_REG8(0x2742) = 0x01;
	ISPAPB1_REG8(0x2743) = 0x90; // syngen line blank
	ISPAPB1_REG8(0x2744) = 0x00;
	ISPAPB1_REG8(0x2745) = 0x50; // syngen frame total
	ISPAPB1_REG8(0x2746) = 0x00;
	ISPAPB1_REG8(0x2747) = 0x03; // syngen frame blank
	ISPAPB1_REG8(0x2748) = 0x00;

	ISPAPB1_REG8(0x2749) = 0x03;
	ISPAPB1_REG8(0x274a) = 0x00;

	ISPAPB1_REG8(0x2705) = 0x01; // CCIR601
	ISPAPB1_REG8(0x276a) = 0xfc; // siggen enable(YUV422): 75% still vertical color bar

	ISPAPB1_REG8(0x21d0) = 0x00; // sofware reset CDSP interface (inactive)

	ISPAPB1_REG8(0x21d4); // 0x40
	ISPAPB1_REG8(0x21d5); // 0x00
	ISPAPB1_REG8(0x21d6); // 0x40
	ISPAPB1_REG8(0x21d7); // 0x00
}

void isp_setting(struct sp_videoin_info vi_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	ISPAPB_LOGI("Test pattern: %d\n", vi_info.pattern);

	switch (vi_info.pattern)
	{
		case STILL_COLORBAR_YUV422_64X64:
			ISPAPB_LOGI("Color Bar YUV422 64x64 pattern\n");

			if ((vi_info.x_len != 64) || (vi_info.y_len != 64))
				ISPAPB_LOGI("Modify size to %dx%d\n", vi_info.x_len, vi_info.y_len);

			isp_64x64(vi_info.x_len, vi_info.y_len);
			break;

		case STILL_COLORBAR_YUV422_1920X1080:
			ISPAPB_LOGI("Color Bar YUV422 1920x1080 pattern\n");

			if ((vi_info.x_len != 1920) || (vi_info.y_len != 1080))
				ISPAPB_LOGI("Modify size to %dx%d\n", vi_info.x_len, vi_info.y_len);

			ispReset(vi_info.pattern); 
			ISPAPB0_REG8(0x21d0) = 0x01; // sofware reset CDSP interface (active)
			cdspSetTable(vi_info.pattern);
			CdspInit(vi_info.pattern, vi_info.out_fmt, vi_info.scale);
			FrontInit(vi_info.pattern, vi_info.x_len, vi_info.y_len, vi_info.probe);
			sensorInit(vi_info.pattern);
			//
			//ISPAPB0_REG8(0x2b00) = 0x00; // siggen enable(hvalidcnt): 75% still vertical color bar
			ISPAPB0_REG8(0x21d0) = 0x00; // sofware reset CDSP interface (inactive)
			ISPAPB0_REG8(0x276c) = 0x01; // sofware reset Front interface
			ISPAPB0_REG8(0x276c) = 0x00;
			//
			ISPAPB0_REG8(0x21d4); // 0x80
			ISPAPB0_REG8(0x21d5); // 0x07
			ISPAPB0_REG8(0x21d6); // 0x38
			ISPAPB0_REG8(0x21d7); // 0x04
			break;

		case STILL_COLORBAR_RAW10_1920X1080:
		case MOVING_COLORBAR_RAW10_1920X1080:
			ISPAPB_LOGI("Color Bar RAW10 1920x1080 pattern\n");

			//if ((vi_info.x_len != 1920) || (vi_info.y_len != 1080))
			if ((vi_info.x_len != 64) || (vi_info.y_len != 64))
				ISPAPB_LOGI("Modify size to %dx%d\n", vi_info.x_len, vi_info.y_len);

			ispReset(vi_info.pattern);
			ISPAPB0_REG8(0x21d0) = 0x01; //  sofware reset CDSP interface (active)
			cdspSetTable(vi_info.pattern);
			//FrontInit(1920, 1080);
			FrontInit(vi_info.pattern, vi_info.x_len, vi_info.y_len, vi_info.probe);
			CdspInit(vi_info.pattern, vi_info.out_fmt, vi_info.scale);
			sensorInit(vi_info.pattern);
			//
			ISPAPB0_REG8(0x2b00) = 0x00; // siggen enable(hvalidcnt): 75% still vertical color bar
			//ISPAPB1_REG8(0x21d0) = 0x00; // sofware reset CDSP interface (inactive)
			ISPAPB0_REG8(0x21d0) = 0x00; // sofware reset CDSP interface (inactive)
			//
			ISPAPB0_REG8(0x21d4); // 0x80
			ISPAPB0_REG8(0x21d5); // 0x07
			ISPAPB0_REG8(0x21d6); // 0x38
			ISPAPB0_REG8(0x21d7); // 0x04
			break;

		case SENSOR_INPUT:
			ISPAPB_LOGI("SC2310 Camera Module\n");

			isp_setting_s(vi_info);
			break;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}
