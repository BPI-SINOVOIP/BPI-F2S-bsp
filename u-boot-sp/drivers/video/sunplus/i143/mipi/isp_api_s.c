#include <common.h>
#include <stdio.h>

#include "sensor_power.h"
#include "isp_api.h"
#include "i2c_api.h"
#include "isp_api_s.h"

// video test itme
// 0 - always input video from camera
// 1 - check the ability of start/stop video
// 2 - check AE and AWB functions
#define VIDEO_TEST (2)

#if (VIDEO_TEST == 2)
#define ENABLE_3A   1
#endif
#if ENABLE_3A
#include "3a.h"
int VideoStart = 0;
#endif

#define BYPASSCDSP_NEW_RAW10	(0)
#define BYPASSCDSP_RAW8			(0)
#define CDSP_SCALER_HD			(0) //scale down from FHD  to HD size		
#define CDSP_SCALER_VGA			(0) //scale down from FHD  to VGA size
#define CDSP_SCALER_QQVGA		(0) //scale down from FHD  to QVGA size
#define COLOR_BAR_MOVING		(0)
#define INTERRUPT_VS_FALLING	(0)

//front table and struct
#define MAX_FRONT_REG_LEN		(100)

unsigned char FRONT_INIT_FILE[] = {
	#include "FrontInit_s.txt"
};

typedef struct 
{
	u8 type; //reversed
	u16 adr;
	u8 dat;	
} FRONT_DATA_T;

typedef struct
{
	u16 count; //0x00, 0x19, => 0x0019=25
	FRONT_DATA_T FRONT_DATA[MAX_FRONT_REG_LEN];
	//FRONT_DATA_T FRONT_DATA[(u16)(ARRAY_SIZE(FRONT_INIT_FILE)/sizeof(FRONT_DATA_T))];
} FRONT_INIT_FILE_T;

//cdsp table and struct
#define MAX_CDSP_REG_LEN		(300)

unsigned char CDSP_INIT_FILE[] = {
	#include "CdspInit_s.txt"
};

typedef struct
{
	u8 type; //reversed
	u16 adr;
	u8 dat;	
} CDSP_DATA_T;

typedef struct
{
	u16 count; //0x01, 0x02, => 0x0102=258
	CDSP_DATA_T CDSP_DATA[MAX_CDSP_REG_LEN];
	//CDSP_DATA_T CDSP_DATA[(u16)(ARRAY_SIZE(CDSP_INIT_FILE)/sizeof(CDSP_DATA_T))];
} CDSP_INIT_FILE_T;

//sensor struct
#if ENABLE_3A
char AAA_INIT_FILE[] = {
	#include "AaaInit.txt"
};

char AE_TABLE[] = {
	#include "aeexp60.txt"
};

char GAIN_TABLE[] = {
	#include "aegain60.txt"
};

//#define AE_TABLE				"aeexp60.txt"	//aeexp60.txt, aeexp50.txt
//#define GAIN_TABLE				"aegain60.txt" 	//aegain60.txt, aegain50.txt
#define SC2310_GAIN_ADDR		(0x3E08)
#define SC2310_FRAME_LEN_ADDR	(0x320E)
#define SC2310_LINE_TOTAL_ADDR	(0x320C)
#define SC2310_EXP_LINE_ADDR	(0x3E01)
#define SC2310_PCLK				(0x046CF710)
#endif
#define SC2310_DEVICE_ADDR		(0x60)
#define MAX_SENSOR_REG_LEN		(400)

unsigned char SENSOR_INIT_FILE[] = {
	#include "SensorInit_s.txt"
};

typedef struct
{
	u8 type; //0xFE:do delay in ms , 0x00:sensor register 
	//u16 adr;
	//u16 dat;
	
	//u8 adr;
	//u16 dat;
	
	u16 adr;
	u8 dat;
	
	//u8 adr;
	//u8 dat;
	//PS. By different sensor type, adr or dat could be one or two bytes.
	//ex.
	/*
	0xFE, 0x00, 0xC8, 0x00, 0x00, //DELAY= 200ms
	0x00, 0x30, 0x1A, 0x10, 0xD8, //sensor addr=0x301a, sensor data=0x10d8
	*/	
} SENSOR_DATA_T;

typedef struct
{
	u16 count; //0x01, 0x60, => 0x0160=352
	SENSOR_DATA_T SENSOR_DATA[MAX_SENSOR_REG_LEN];
	//SENSOR_DATA_T SENSOR_DATA[(u16)(ARRAY_SIZE(SENSOR_INIT_FILE)/sizeof(SENSOR_DATA_T))];
} SENSOR_INIT_FILE_T;

/* sensor frame rate initialization */
/* sensor 0 */
#define SENSOR_FRAME_RATE   0

unsigned char SF_SENSOR_FRAME_RATE[8][2][4] = {
	{        /*  op    a[0]  a[1]  d         30fps*/
		{        0x00, 0x32, 0x0C, 0x04,        },
		{        0x00, 0x32, 0x0d, 0x4C,        },
	},
	{        /*  op    a[0]  a[1]  d         25fps*/
		{        0x00, 0x32, 0x0C, 0x05,        },
		{        0x00, 0x32, 0x0d, 0x28,        },
	},
	{        /*  op    a[0]  a[1]  d         20fps*/
		{        0x00, 0x32, 0x0C, 0x06,        },
		{        0x00, 0x32, 0x0d, 0x72,        },
	},
	{        /*  op    a[0]  a[1]  d         15fps*/
		{        0x00, 0x32, 0x0C, 0x08,        },
		{        0x00, 0x32, 0x0d, 0x98,        },
	},
	{        /*  op    a[0]  a[1]  d         10fps*/
		{        0x00, 0x32, 0x0C, 0x0C,        },
		{        0x00, 0x32, 0x0d, 0xE4,        },
	},
	{        /*  op    a[0]  a[1]  d         5fps*/
		{        0x00, 0x32, 0x0C, 0x19,        },
		{        0x00, 0x32, 0x0d, 0xC8,        },
	},
	{        /*  op    a[0]  a[1]  d         3fps*/
		{        0x00, 0x32, 0x0C, 0x2A,        },
		{        0x00, 0x32, 0x0d, 0xF8,        },
	},
	{        /*  op    a[0]  a[1]  d         1fps*/
		{        0x00, 0x32, 0x0C, 0x80,        },
		{        0x00, 0x32, 0x0d, 0xE8,        },
	}
};

// Load settings table
unsigned char SF_FIXED_PATTERN_NOISE_S[]={
	#include "FixedPatternNoise_s.txt"
};
unsigned char SF_LENS_COMP_B_S[]={
	#include "lenscompb_s.txt"
};
unsigned char SF_LENS_COMP_G_S[]={
	#include "lenscompg_s.txt"
};
unsigned char SF_LENS_COMP_R_S[]={
	#include "lenscompr_s.txt"
};
unsigned char SF_POST_GAMMA_B_S[]={
	#include "PostGammaB_s.txt"
};
unsigned char SF_POST_GAMMA_G_S[]={
	#include "PostGammaG_s.txt"
};
unsigned char SF_POST_GAMMA_R_S[]={
	#include "PostGammaR_s.txt"
};

void sensorDump(u8 isp)
{
	SENSOR_INIT_FILE_T read_file;
	int i, total;
	u16 dat = 0xff;
	u8 data[4];

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	// conver table data to SENSOR_INIT_FILE_T struct
	total = ARRAY_SIZE(SENSOR_INIT_FILE);
	data[0] = SENSOR_INIT_FILE[0];
	data[1] = SENSOR_INIT_FILE[1];
	data[2] = SENSOR_INIT_FILE[2];
	data[3] = SENSOR_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPB_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPB_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = SENSOR_INIT_FILE[i];
		data[1] = SENSOR_INIT_FILE[i+1];
		data[2] = SENSOR_INIT_FILE[i+2];
		data[3] = SENSOR_INIT_FILE[i+3];
		read_file.SENSOR_DATA[(i-2)/4].type = data[0];
		read_file.SENSOR_DATA[(i-2)/4].adr	= (data[1]<<8)|data[2];
		read_file.SENSOR_DATA[(i-2)/4].dat	= data[3];
		//ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	/* ISP0 path */
	if ((isp == 0) || (isp == 2))
	{
		Reset_I2C0();
		Init_I2C0(SC2310_DEVICE_ADDR, 0);

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			if (read_file.SENSOR_DATA[i].type == 0xFE)
			{
				//udelay(read_file.SENSOR_DATA[i].adr*1000);
			}
			else if (read_file.SENSOR_DATA[i].type == 0x00)
			{
				dat = 0xff;
				getSensor16_I2C0((unsigned long)read_file.SENSOR_DATA[i].adr, &dat, 1);
				ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, dat);
			}
		}
	}

	/* ISP1 path */
	if ((isp == 1) || (isp == 2))
	{
		Reset_I2C1();
		Init_I2C1(SC2310_DEVICE_ADDR, 0);

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			if (read_file.SENSOR_DATA[i].type == 0xFE)
			{
				//udelay(read_file.SENSOR_DATA[i].adr*1000);
			}
			else if (read_file.SENSOR_DATA[i].type == 0x00)
			{
				dat = 0xff;
				getSensor16_I2C1((unsigned long)read_file.SENSOR_DATA[i].adr, &dat, 1);
				ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, dat);
			}
		}
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void sleep(unsigned long sec)
{
	int i;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);
	ISPAPB_LOGI("%s, total time: %ld seconds\n", __FUNCTION__, sec);

	for (i = 0; i < sec; i++)
	{
		mdelay(1000);
		ISPAPB_LOGI("%s, %d seconds\n", __FUNCTION__, i+1);
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@ispSleep_s this function depends on O.S.
*/
void ispSleep_s(void)
{
	int i;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	for (i = 0;i < 10; i++) {
		MOON0_REG->clken[2];
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@ispReset_s
*/
void ispReset_s(struct sp_videoin_info vi_info)
{
#if ENABLE_3A
	//
	VideoStart=0;
	//
#endif

	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		ISPAPB0_REG8(0x2000) = 0x13; //reset all module include ispck
	    ISPAPB0_REG8(0x2003) = 0x1c; //enable phase clocks
	    ISPAPB0_REG8(0x2005) = 0x07; //enbale p1xck
	    ISPAPB0_REG8(0x2008) = 0x05; //switch b1xck/bpclk_nx to normal clocks
	    ISPAPB0_REG8(0x2000) = 0x03; //release ispck reset
		ispSleep_s();//#(`TIMEBASE*20;
		//
		ISPAPB0_REG8(0x2000) = 0x00; //release all module reset
		//
		ISPAPB0_REG8(0x276c) = 0x01; //reset front
		ISPAPB0_REG8(0x276c) = 0x00; //
		//	
		ISPAPB0_REG8(0x2000) = 0x03;
		ISPAPB0_REG8(0x2000) = 0x00;
		//
		ISPAPB0_REG8(0x2010) = 0x00;//cclk: 48MHz
	}

	/* ISP0 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		ISPAPB1_REG8(0x2000) = 0x13; //reset all module include ispck
	    ISPAPB1_REG8(0x2003) = 0x1c; //enable phase clocks
	    ISPAPB1_REG8(0x2005) = 0x07; //enbale p1xck
	    ISPAPB1_REG8(0x2008) = 0x05; //switch b1xck/bpclk_nx to normal clocks
	    ISPAPB1_REG8(0x2000) = 0x03; //release ispck reset
		ispSleep_s();//#(`TIMEBASE*20;
		//
		ISPAPB1_REG8(0x2000) = 0x00; //release all module reset
		//
		ISPAPB1_REG8(0x276c) = 0x01; //reset front
		ISPAPB1_REG8(0x276c) = 0x00; //
		//	
		ISPAPB1_REG8(0x2000) = 0x03;
		ISPAPB1_REG8(0x2000) = 0x00;
		//
		ISPAPB1_REG8(0x2010) = 0x00;//cclk: 48MHz
	}
}

/*
	@FrontInit_s
	ex. FrontInit_s(1920, 1080);
*/
void FrontInit_s(struct sp_videoin_info vi_info)
{
	FRONT_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	// conver table data to FRONT_INIT_FILE_T struct
	total = ARRAY_SIZE(FRONT_INIT_FILE);
	data[0] = FRONT_INIT_FILE[0];
	data[1] = FRONT_INIT_FILE[1];
	data[2] = FRONT_INIT_FILE[2];
	data[3] = FRONT_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPB_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPB_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = FRONT_INIT_FILE[i];
		data[1] = FRONT_INIT_FILE[i+1];
		data[2] = FRONT_INIT_FILE[i+2];
		data[3] = FRONT_INIT_FILE[i+3];
		read_file.FRONT_DATA[(i-2)/4].type = data[0];
		read_file.FRONT_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.FRONT_DATA[(i-2)/4].dat  = data[3];
		//ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		//clock setting
		ISPAPB0_REG8(0x2008) = 0x07;

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.FRONT_DATA[i].type, read_file.FRONT_DATA[i].adr, read_file.FRONT_DATA[i].dat);
			if (read_file.FRONT_DATA[i].type == 0x00)
			{
				ISPAPB0_REG8((unsigned long)read_file.FRONT_DATA[i].adr) = read_file.FRONT_DATA[i].dat;
			}
		}
		//
		ISPAPB0_REG8(0x2720) = 0x00;	/* hoffset */
		ISPAPB0_REG8(0x2721) = 0x00;
		ISPAPB0_REG8(0x2722) = 0x00;	/* voffset */
		ISPAPB0_REG8(0x2723) = 0x00;
		ISPAPB0_REG8(0x2724) = 0x84;	/* hsize */
		ISPAPB0_REG8(0x2725) = 0x07;
		ISPAPB0_REG8(0x2726) = 0x3C;	/* vsize */
		ISPAPB0_REG8(0x2727) = 0x04;
		ISPAPB0_REG8(0x275A) = 0x01;
		ISPAPB0_REG8(0x2759) = 0x00;
		ISPAPB0_REG8(0x2604) = 0x00;
		//ISPAPB0_REG8(0x2007) = 0x01;
	}

	/* ISP1 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		//clock setting
		ISPAPB1_REG8(0x2008) = 0x07;

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.FRONT_DATA[i].type, read_file.FRONT_DATA[i].adr, read_file.FRONT_DATA[i].dat);
			if (read_file.FRONT_DATA[i].type == 0x00)
			{
				ISPAPB1_REG8((unsigned long)read_file.FRONT_DATA[i].adr) = read_file.FRONT_DATA[i].dat;
			}
		}
		//
		ISPAPB1_REG8(0x2720) = 0x00;	/* hoffset */
		ISPAPB1_REG8(0x2721) = 0x00;
		ISPAPB1_REG8(0x2722) = 0x00;	/* voffset */
		ISPAPB1_REG8(0x2723) = 0x00;
		ISPAPB1_REG8(0x2724) = 0x84;	/* hsize */
		ISPAPB1_REG8(0x2725) = 0x07;
		ISPAPB1_REG8(0x2726) = 0x3C;	/* vsize */
		ISPAPB1_REG8(0x2727) = 0x04;
		ISPAPB1_REG8(0x275A) = 0x01;
		ISPAPB1_REG8(0x2759) = 0x00;
		ISPAPB1_REG8(0x2604) = 0x00;
		//ISPAPB1_REG8(0x2007) = 0x01;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable_s
*/
void cdspSetTable_s(struct sp_videoin_info vi_info)
{
	int i;

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		ISPAPB0_REG8(0x2008) = 0x00; //use memory clock for pixel clock, master clock and mipi decoder clock
		// R table of lens compensation tables
		ISPAPB0_REG8(0x2101) = 0x00; // select lens compensation R SRAM
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0	
		for (i = 0; i < 768; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_LENS_COMP_R_S[i];
		}
		//
		// G/Gr table of lens compensation tables
		ISPAPB0_REG8(0x2101) = 0x01; // select lens compensation G/Gr SRAM
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 768; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_LENS_COMP_G_S[i];
		}
		//
		// B table of lens compensation tables
		ISPAPB0_REG8(0x2101) = 0x02; // select lens compensation B SRAM
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 768; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_LENS_COMP_B_S[i];
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
			ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_R_S[i];
		}
		//
		// G table of post gamma tables
		ISPAPB0_REG8(0x2101) = 0x05; // select post gamma G SRAM
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 512; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_G_S[i];
		}
		//
		// B table of of post gamma tables
		ISPAPB0_REG8(0x2101) = 0x06; // select post gamma B SRAM
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 512; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_POST_GAMMA_B_S[i];
		}
		//
		//  fixed pattern noise tables
		ISPAPB0_REG8(0x2101) = 0x0D; // select fixed pattern noise
		ISPAPB0_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0
		//for (i = 0; i < 1952; i++)
		for (i = 0; i < 1312; i++)
		{
			ISPAPB0_REG8(0x2103) = SF_FIXED_PATTERN_NOISE_S[i];
		}
		// disable set cdsp sram
		ISPAPB0_REG8(0x2104) = 0x00; // select macro page 0 
		ISPAPB0_REG8(0x2102) = 0x00; // set macro address to 0 
		ISPAPB0_REG8(0x2100) = 0x00; // disable CPU access macro and adress auto increase 
	}

	/* ISP1 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		ISPAPB1_REG8(0x2008) = 0x00; //use memory clock for pixel clock, master clock and mipi decoder clock
		// R table of lens compensation tables
		ISPAPB1_REG8(0x2101) = 0x00; // select lens compensation R SRAM
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0	
		for (i = 0; i < 768; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_LENS_COMP_R_S[i];
		}
		//
		// G/Gr table of lens compensation tables
		ISPAPB1_REG8(0x2101) = 0x01; // select lens compensation G/Gr SRAM
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 768; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_LENS_COMP_G_S[i];
		}
		//
		// B table of lens compensation tables
		ISPAPB1_REG8(0x2101) = 0x02; // select lens compensation B SRAM
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 768; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_LENS_COMP_B_S[i];
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
			ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_R_S[i];
		}
		//
		// G table of post gamma tables
		ISPAPB1_REG8(0x2101) = 0x05; // select post gamma G SRAM
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 512; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_G_S[i];
		}
		//
		// B table of of post gamma tables
		ISPAPB1_REG8(0x2101) = 0x06; // select post gamma B SRAM
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
		for (i = 0; i < 512; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_POST_GAMMA_B_S[i];
		}
		//
		//  fixed pattern noise tables
		ISPAPB1_REG8(0x2101) = 0x0D; // select fixed pattern noise
		ISPAPB1_REG8(0x2100) = 0x03; // enable CPU access macro and adress auto increase
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0
		//for (i = 0; i < 1952; i++)
		for (i = 0; i < 1312; i++)
		{
			ISPAPB1_REG8(0x2103) = SF_FIXED_PATTERN_NOISE_S[i];
		}
		// disable set cdsp sram
		ISPAPB1_REG8(0x2104) = 0x00; // select macro page 0 
		ISPAPB1_REG8(0x2102) = 0x00; // set macro address to 0 
		ISPAPB1_REG8(0x2100) = 0x00; // disable CPU access macro and adress auto increase 
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@sensorInit_s
*/
void sensorInit_s(struct sp_videoin_info vi_info)
{
	SENSOR_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

 	// conver table data to SENSOR_INIT_FILE_T struct
	total = ARRAY_SIZE(SENSOR_INIT_FILE);
	data[0] = SENSOR_INIT_FILE[0];
	data[1] = SENSOR_INIT_FILE[1];
	data[2] = SENSOR_INIT_FILE[2];
	data[3] = SENSOR_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPB_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPB_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = SENSOR_INIT_FILE[i];
		data[1] = SENSOR_INIT_FILE[i+1];
		data[2] = SENSOR_INIT_FILE[i+2];
		data[3] = SENSOR_INIT_FILE[i+3];
		read_file.SENSOR_DATA[(i-2)/4].type = data[0];
		read_file.SENSOR_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.SENSOR_DATA[(i-2)/4].dat  = data[3];
		//ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		Reset_I2C0();
		Init_I2C0(SC2310_DEVICE_ADDR, 0);

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);

			if (read_file.SENSOR_DATA[i].type == 0xFE)
			{
				udelay(read_file.SENSOR_DATA[i].adr*1000);
			}
			else if (read_file.SENSOR_DATA[i].type == 0x00)
			{
				setSensor16_I2C0((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1);
			}
		}

		/* set seneor frame rate */
		/* In SensorInit.txt, the default is 30fps. So skip setting frame rate if  SENSOR_FRAME_RATE = 0*/
		if (SENSOR_FRAME_RATE) {
			read_file.count = 2;
			for (i = 0; i < read_file.count; i++)
			{
				data[0] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][0];
				data[1] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][1];
				data[2] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][2];
				data[3] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][3];
				read_file.SENSOR_DATA[i].type = data[0];
				read_file.SENSOR_DATA[i].adr	= (data[1]<<8)|data[2];
				read_file.SENSOR_DATA[i].dat	= data[3];
				ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
			}

			ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
			for (i = 0; i < read_file.count; i++)
			{
				ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);

				if (read_file.SENSOR_DATA[i].type == 0xFE)
				{
					udelay(read_file.SENSOR_DATA[i].adr*1000);
				}
				else if (read_file.SENSOR_DATA[i].type == 0x00)
				{
					setSensor16_I2C0((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1);
				}
			}
		}

		/* use pixel clock, master clock and mipi decoder clock as they are  */
		ISPAPB0_REG8(0x2008) = 0x07;

		/* set and clear of front sensor interface */
		ISPAPB0_REG8(0x276C) = 0x01;
		ISPAPB0_REG8(0x276C) = 0x00;

		/* set and clear of front i2c interface */
		ISPAPB0_REG8(0x2660) = 0x01;
		ISPAPB0_REG8(0x2660) = 0x00;

		/* set and clear of cdsp */
		ISPAPB0_REG8(0x21D0) = 0x01;
		ISPAPB0_REG8(0x21D0) = 0x00;
	}

	/* ISP1 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		Reset_I2C1();
		Init_I2C1(SC2310_DEVICE_ADDR, 0);

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);

			if (read_file.SENSOR_DATA[i].type == 0xFE)
			{
				udelay(read_file.SENSOR_DATA[i].adr*1000);
			}
			else if (read_file.SENSOR_DATA[i].type == 0x00)
			{
				setSensor16_I2C1((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1);
			}
		}

		/* set seneor frame rate */
		/* In SensorInit.txt, the default is 30fps. So skip setting frame rate if  SENSOR_FRAME_RATE = 0*/
		if (SENSOR_FRAME_RATE) {
			read_file.count = 2;
			for (i = 0; i < read_file.count; i++)
			{
				data[0] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][0];
				data[1] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][1];
				data[2] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][2];
				data[3] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][3];
				read_file.SENSOR_DATA[i].type = data[0];
				read_file.SENSOR_DATA[i].adr	= (data[1]<<8)|data[2];
				read_file.SENSOR_DATA[i].dat	= data[3];
				ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
			}

			ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
			for (i = 0; i < read_file.count; i++)
			{
				ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);

				if (read_file.SENSOR_DATA[i].type == 0xFE)
				{
					udelay(read_file.SENSOR_DATA[i].adr*1000);
				}
				else if (read_file.SENSOR_DATA[i].type == 0x00)
				{
					setSensor16_I2C1((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1);
				}
			}
		}

		/* use pixel clock, master clock and mipi decoder clock as they are  */
		ISPAPB1_REG8(0x2008) = 0x07;

		/* set and clear of front sensor interface */
		ISPAPB1_REG8(0x276C) = 0x01;
		ISPAPB1_REG8(0x276C) = 0x00;

		/* set and clear of front i2c interface */
		ISPAPB1_REG8(0x2660) = 0x01;
		ISPAPB1_REG8(0x2660) = 0x00;

		/* set and clear of cdsp */
		ISPAPB1_REG8(0x21D0) = 0x01;
		ISPAPB1_REG8(0x21D0) = 0x00;
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@CdspInit_s
*/
void CdspInit_s(struct sp_videoin_info vi_info)
{	
	CDSP_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

 	// conver table data to CDSP_INIT_FILE_T struct
	total = ARRAY_SIZE(CDSP_INIT_FILE);
	data[0] = CDSP_INIT_FILE[0];
	data[1] = CDSP_INIT_FILE[1];
	data[2] = CDSP_INIT_FILE[2];
	data[3] = CDSP_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPB_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPB_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = CDSP_INIT_FILE[i];
		data[1] = CDSP_INIT_FILE[i+1];
		data[2] = CDSP_INIT_FILE[i+2];
		data[3] = CDSP_INIT_FILE[i+3];
		read_file.CDSP_DATA[(i-2)/4].type = data[0];
		read_file.CDSP_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.CDSP_DATA[(i-2)/4].dat  = data[3];
		//ISPAPB_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		//clock setting	
		ISPAPB0_REG8(0x21d0) = 0x01; //sofware reset CDSP interface (active)
		ISPAPB0_REG8(0x21d0) = 0x00; //sofware reset CDSP interface (inactive)

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i<read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.CDSP_DATA[i].type, read_file.CDSP_DATA[i].adr, read_file.CDSP_DATA[i].dat);

			if (read_file.CDSP_DATA[i].type == 0x00)
			{
				ISPAPB0_REG8((unsigned long)read_file.CDSP_DATA[i].adr) = read_file.CDSP_DATA[i].dat;
			}
		}
		//
		ISPAPB0_REG8(0x220C) = 0x33; //SF_CDSP_INIT_SM
		ISPAPB0_REG8(0x21c0) = 0x23; /* enable bight, contrast hue and saturation cropping mode */
	/*	                             
		#if (CDSP_SCALER_HD)//scale down from FHD  to HD size
			//H=1280*65536/(1920) = 0xAAAB
		   //V=720*65536/(1080) = 0xAAAB
			ISPAPB0_REG8(0x21b0) = 0xAB;//factor for Hsize
			ISPAPB0_REG8(0x21b1) = 0xAA;
			ISPAPB0_REG8(0x21b2) = 0xAB;//factor for Vsize
			ISPAPB0_REG8(0x21b3) = 0xAA;
			//
			ISPAPB0_REG8(0x21b4) = 0xAB;//factor for Hsize
			ISPAPB0_REG8(0x21b5) = 0xAA;
			ISPAPB0_REG8(0x21b6) = 0xAB;//factor for Vsize
			ISPAPB0_REG8(0x21b7) = 0xAA;
			//
			ISPAPB0_REG8(0x21b8) = 0x2F;	//enable	
		#elif (CDSP_SCALER_VGA)//scale down from FHD to VGA size	
		   //H=640*65536/(1920) = 0x5556
		   //V=480*65536/(1080) = 0x71C8
			ISPAPB0_REG8(0x21b0) = 0x56;//factor for Hsize
			ISPAPB0_REG8(0x21b1) = 0x55;
			ISPAPB0_REG8(0x21b2) = 0xC8;//factor for Vsize
			ISPAPB0_REG8(0x21b3) = 0x71;
			//
			ISPAPB0_REG8(0x21b4) = 0x56;//factor for Hsize
			ISPAPB0_REG8(0x21b5) = 0x55;
			ISPAPB0_REG8(0x21b6) = 0xC8;//factor for Vsize
			ISPAPB0_REG8(0x21b7) = 0x71;
			//
			ISPAPB0_REG8(0x21b8) = 0x2F;	//enable	
		#elif (CDSP_SCALER_QQVGA)//scale down from FHD  to QVGA size	     
			//H=160*65536/(1920) = 0x1556
		    //V=120*65536/(1080) = 0x1C72
			ISPAPB0_REG8(0x21b0) = 0x56;//factor for Hsize
			ISPAPB0_REG8(0x21b1) = 0x15;
			ISPAPB0_REG8(0x21b2) = 0x72;//factor for Vsize
			ISPAPB0_REG8(0x21b3) = 0x1C;
			//
			ISPAPB0_REG8(0x21b4) = 0x56;//factor for Hsize
			ISPAPB0_REG8(0x21b5) = 0x15;
			ISPAPB0_REG8(0x21b6) = 0x72;//factor for Vsize
			ISPAPB0_REG8(0x21b7) = 0x1C;
			//
			ISPAPB0_REG8(0x21b8) = 0x2F;	//enable
		#else	
			ISPAPB0_REG8(0x21b8) = 0x00; //disable H/V scale down                              
		#endif                           
	*/
	}

	/* ISP1 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		//clock setting	
		ISPAPB1_REG8(0x21d0) = 0x01; //sofware reset CDSP interface (active)
		ISPAPB1_REG8(0x21d0) = 0x00; //sofware reset CDSP interface (inactive)

		ISPAPB_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i<read_file.count; i++)
		{
			//ISPAPB_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.CDSP_DATA[i].type, read_file.CDSP_DATA[i].adr, read_file.CDSP_DATA[i].dat);

			if (read_file.CDSP_DATA[i].type == 0x00)
			{
				ISPAPB1_REG8((unsigned long)read_file.CDSP_DATA[i].adr) = read_file.CDSP_DATA[i].dat;
			}
		}
		//
		ISPAPB1_REG8(0x220C) = 0x33; //SF_CDSP_INIT_SM
		ISPAPB1_REG8(0x21c0) = 0x23; /* enable bight, contrast hue and saturation cropping mode */
	/*	                             
	#if (CDSP_SCALER_HD)//scale down from FHD  to HD size
			//H=1280*65536/(1920) = 0xAAAB
		   //V=720*65536/(1080) = 0xAAAB
			ISPAPB1_REG8(0x21b0) = 0xAB;//factor for Hsize
			ISPAPB1_REG8(0x21b1) = 0xAA;
			ISPAPB1_REG8(0x21b2) = 0xAB;//factor for Vsize
			ISPAPB1_REG8(0x21b3) = 0xAA;
			//
			ISPAPB1_REG8(0x21b4) = 0xAB;//factor for Hsize
			ISPAPB1_REG8(0x21b5) = 0xAA;
			ISPAPB1_REG8(0x21b6) = 0xAB;//factor for Vsize
			ISPAPB1_REG8(0x21b7) = 0xAA;
			//
			ISPAPB1_REG8(0x21b8) = 0x2F;	//enable	
	#elif (CDSP_SCALER_VGA)//scale down from FHD to VGA size	
		   //H=640*65536/(1920) = 0x5556
		   //V=480*65536/(1080) = 0x71C8
			ISPAPB1_REG8(0x21b0) = 0x56;//factor for Hsize
			ISPAPB1_REG8(0x21b1) = 0x55;
			ISPAPB1_REG8(0x21b2) = 0xC8;//factor for Vsize
			ISPAPB1_REG8(0x21b3) = 0x71;
			//
			ISPAPB1_REG8(0x21b4) = 0x56;//factor for Hsize
			ISPAPB1_REG8(0x21b5) = 0x55;
			ISPAPB1_REG8(0x21b6) = 0xC8;//factor for Vsize
			ISPAPB1_REG8(0x21b7) = 0x71;
			//
			ISPAPB1_REG8(0x21b8) = 0x2F;	//enable	
	#elif (CDSP_SCALER_QQVGA)//scale down from FHD	to QVGA size		 
			//H=160*65536/(1920) = 0x1556
		    //V=120*65536/(1080) = 0x1C72
			ISPAPB1_REG8(0x21b0) = 0x56;//factor for Hsize
			ISPAPB1_REG8(0x21b1) = 0x15;
			ISPAPB1_REG8(0x21b2) = 0x72;//factor for Vsize
			ISPAPB1_REG8(0x21b3) = 0x1C;
			//
			ISPAPB1_REG8(0x21b4) = 0x56;//factor for Hsize
			ISPAPB1_REG8(0x21b5) = 0x15;
			ISPAPB1_REG8(0x21b6) = 0x72;//factor for Vsize
			ISPAPB1_REG8(0x21b7) = 0x1C;
			//
			ISPAPB1_REG8(0x21b8) = 0x2F;	//enable
	#else	
			ISPAPB1_REG8(0x21b8) = 0x00; //disable H/V scale down                              
	#endif							 
	*/
	}

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

/*
	@ispAaaInit_s
*/
void ispAaaInit_s(void)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

#if ENABLE_3A
	aaaLoadInit(AAA_INIT_FILE);
	aeLoadAETbl(AE_TABLE); 
	aeLoadGainTbl(GAIN_TABLE); 
	aeInitExt();
	awbInit();
#endif

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

#if ENABLE_3A	
int isVideoStart(void)
{
	return VideoStart;
}
#endif

void videoStartMode(struct sp_videoin_info vi_info)
{
	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	FrontInit_s(vi_info);
	CdspInit_s(vi_info);
	ispAaaInit_s();
	sensorInit_s(vi_info);

	/* set and clear reset of buffer cdsp interface */
	if ((vi_info.isp == 0) || (vi_info.isp = 2)) {
		ISPAPB0_REG8(0x2300) |= 0x08;
		ISPAPB0_REG8(0x2300) &= 0xF7;
	}
	if ((vi_info.isp == 1) || (vi_info.isp = 2)) {
		ISPAPB1_REG8(0x2300) |= 0x08;
		ISPAPB1_REG8(0x2300) &= 0xF7;
	}

#if ENABLE_3A	
	vidctrlInit(SC2310_GAIN_ADDR, SC2310_FRAME_LEN_ADDR, SC2310_LINE_TOTAL_ADDR,
				SC2310_EXP_LINE_ADDR, SC2310_PCLK);
	VideoStart = 1;
	InstallVSinterrupt();
	ISPAPB0_REG8(0x27B0) = 2;       /* clear vd falling edge interrupt AAF061 W1C*/
	ISPAPB0_REG8(0x27C0) |= 0x02;   /* enable vd falling edge interrupt */
	//ISPAPB0_REG8(0x2B00) = 0x01;   /* enable TNR */
#endif

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}

void videoStopMode(struct sp_videoin_info vi_info)
{
	/* ISP0 path */
	if ((vi_info.isp == 0) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

#if ENABLE_3A	
		VideoStart = 0;
		ISPAPB0_REG8(0x27C0) &= 0xFD;   /* disable vd falling edge interrupt */
		ISPAPB0_REG8(0x27B0) = 2;       /* clear vd falling edge interrupt AAF061 W1C*/
#endif

		/* set reset of buffer jpeg, cdsp and usb interface */
		ISPAPB0_REG8(0x2300) = 0x1C;
		ISPAPB0_REG8(0x2300) = 0x00;

		/* use memory clock for pixel clock, master clock and mipi decoder clock */
		ISPAPB0_REG8(0x2008) = 0x00;

		/* set reset of cdsp */
		ISPAPB0_REG8(0x21D0) = 0x01;

		/* set reset of front sensor interface */
		ISPAPB0_REG8(0x276C) = 0x01;

		/* set and clear reset of front i2c interface */
		ISPAPB0_REG8(0x2660) = 0x01;
		ISPAPB0_REG8(0x2660) = 0x00;
	}

	/* ISP1 path */
	if ((vi_info.isp == 1) || (vi_info.isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

#if ENABLE_3A	
		VideoStart = 0;
		ISPAPB1_REG8(0x27C0) &= 0xFD;	/* disable vd falling edge interrupt */
		ISPAPB1_REG8(0x27B0) = 2;		/* clear vd falling edge interrupt AAF061 W1C*/
#endif

		/* set reset of buffer jpeg, cdsp and usb interface */
		ISPAPB1_REG8(0x2300) = 0x1C;
		ISPAPB1_REG8(0x2300) = 0x00;

		/* use memory clock for pixel clock, master clock and mipi decoder clock */
		ISPAPB1_REG8(0x2008) = 0x00;

		/* set reset of cdsp */
		ISPAPB1_REG8(0x21D0) = 0x01;

		/* set reset of front sensor interface */
		ISPAPB1_REG8(0x276C) = 0x01;

		/* set and clear reset of front i2c interface */
		ISPAPB1_REG8(0x2660) = 0x01;
		ISPAPB1_REG8(0x2660) = 0x00;
	}
}

void isp_setting_s(struct sp_videoin_info vi_info)
{
#if (VIDEO_TEST == 1)
	int i;
#endif

	ISPAPB_LOGI("%s start\n", __FUNCTION__);

	ispReset_s(vi_info);
	cdspSetTable_s(vi_info);

#if (VIDEO_TEST == 0)
	ISPAPB_LOGI("%s, check video stream from camera\n", __FUNCTION__);

	/* check video stream from camera */
	powerSensorOn_RAM(vi_info.isp);
	videoStartMode(vi_info);
#elif (VIDEO_TEST == 1)
	ISPAPB_LOGI("%s, check the stability of start/stop video\n", __FUNCTION__);

	/* check the stability of start/stop video */
	for (i = 0; i < 10; i++)
	{
		ISPAPB_LOGI("%s, for loop: %d\n", __FUNCTION__, i);

		powerSensorOn_RAM(vi_info.isp);
		videoStartMode(vi_info);
		sleep(10);      // sec unit
		videoStopMode(vi_info);
		sleep(5);       // sec unit
		powerSensorDown_RAM(vi_info.isp);
		sleep(5);       // sec unit
	}
#elif (VIDEO_TEST == 2)
	ISPAPB_LOGI("%s, check AE and AWB functions\n", __FUNCTION__);

	/* check AE and AWB functions */
	powerSensorOn_RAM(vi_info.isp);
	videoStartMode(vi_info);
	while(1)
	{
		// Replace interrupt hardware action with software polling
		//if ((ISPAPB0_REG8(0x27B0) & INTR_VD_EDGE) == INTR_VD_EDGE) {	/* sensor vd falling interrupt event */
		if ((ISPAPB0_REG8(0x27B0) & 0x02) == 0x02) {	/* sensor vd falling interrupt event */
			intrIntr0SensorVsync();
		}
		aaaAeAwbAf();
	}
	videoStopMode(vi_info);
	powerSensorDown_RAM(vi_info.isp);
#endif

	ISPAPB_LOGI("%s end\n", __FUNCTION__);
}
