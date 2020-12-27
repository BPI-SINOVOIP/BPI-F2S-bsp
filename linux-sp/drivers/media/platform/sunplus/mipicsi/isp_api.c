#include <linux/io.h>
#include <linux/delay.h>
#include "isp_api.h"
#include "isp_test_api.h"
#include "i2c_api.h"
#include "sensor_power.h"

// Compiler switch definition
//#define ISP_RESET       // Do ISP reset in SC2310 sub-device driver
//#define SENSOR_INIT     // Initialize sensor in SC2310 sub-device driver
#define ENABLE_3A
#ifdef ENABLE_3A
#include "3a.h"
#endif
int VideoStart = 0;

#define BYPASSCDSP_NEW_RAW10	(0)
#define BYPASSCDSP_RAW8			(0)
#define CDSP_SCALER_HD			(0) //scale down from FHD  to HD size		
#define CDSP_SCALER_VGA			(0) //scale down from FHD  to VGA size
#define CDSP_SCALER_QQVGA		(0) //scale down from FHD  to QVGA size
#define COLOR_BAR_MOVING		(0)
#define INTERRUPT_VS_FALLING	(0)

// front table and struct
#define MAX_FRONT_REG_LEN		(100)

const unsigned char FRONT_INIT_FILE[] = {
	#include "FrontInit.txt"
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

// cdsp table and struct
#define MAX_CDSP_REG_LEN		(300)

const unsigned char CDSP_INIT_FILE[] = {
	#include "CdspInit.txt"
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

// sensor struct
#ifdef ENABLE_3A
const char AAA_INIT_FILE[] = {
	#include "AaaInit.txt"
};

const char AE_EXP_TABLE[] = {
	#include "aeexp60.txt"
};

const char AE_GAIN_TABLE[] = {
	#include "aegain60.txt"
};

//#define AE_EXP_TABLE				"aeexp60.txt"	// aeexp60.txt for Taiwan, aeexp50.txt for China
//#define AE_GAIN_TABLE			"aegain60.txt" 	// aegain60.txt for Taiwan, aegain50.txt for China
#define SC2310_GAIN_ADDR		(0x3E08)
#define SC2310_FRAME_LEN_ADDR	(0x320E)
#define SC2310_LINE_TOTAL_ADDR	(0x320C)
#define SC2310_EXP_LINE_ADDR	(0x3E01)
#define SC2310_PCLK				(0x046CF710)
#endif
#define SC2310_DEVICE_ADDR		(0x60)
//#define MAX_SENSOR_REG_LEN		(400) // This causes a warning. warning: the frame size of 2416 bytes is larger than 2048 bytes.
#define MAX_SENSOR_REG_LEN		(200)

#ifdef SENSOR_INIT
const unsigned char SENSOR_INIT_FILE[] = {
	#include "SensorInit.txt"
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
#endif

/* sensor frame rate initialization */
/* sensor 0 */
#define SENSOR_FRAME_RATE   0

const unsigned char SF_SENSOR_FRAME_RATE[8][2][4] = {
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
static const unsigned char SF_FIXED_PATTERN_NOISE[]={
	#include "FixedPatternNoise.txt"
};
static const unsigned char SF_LENS_COMP_B[]={
	#include "lenscompb.txt"
};
static const unsigned char SF_LENS_COMP_G[]={
	#include "lenscompg.txt"
};
static const unsigned char SF_LENS_COMP_R[]={
	#include "lenscompr.txt"
};
static const unsigned char SF_POST_GAMMA_B[]={
	#include "PostGammaB.txt"
};
static const unsigned char SF_POST_GAMMA_G[]={
	#include "PostGammaG.txt"
};
static const unsigned char SF_POST_GAMMA_R[]={
	#include "PostGammaR.txt"
};


#ifdef ISP_RESET
static void sleep(unsigned long sec)
{
	int i;

	ISPAPI_LOGI("%s, total time: %ld seconds\n", __FUNCTION__, sec);
	for (i = 0; i < sec; i++)
	{
		mdelay(1000);
		ISPAPI_LOGI("%s, %d sec\n", __FUNCTION__, i+1);
	}
}

/*
	@ispSleep this function depends on O.S.
*/
static void ispSleep(int delay)
{
	#define ISP_DELAY_TIMEBASE  21  // 20.83 ns
	u64 time;

	// Calculate how many time to delay in ns
	time = delay * ISP_DELAY_TIMEBASE;
	ISPAPI_LOGI("%s, delay %lld ns\n", __FUNCTION__, time);
	ndelay(time);
}

/*
	@ispReset
*/
static void ispReset(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGI("%s, %d\n", __FUNCTION__, __LINE__);

	writeb(0x13, &(regs->reg[0x2000])); // reset all module include ispck
	writeb(0x1c, &(regs->reg[0x2003])); // enable phase clocks
	writeb(0x07, &(regs->reg[0x2005])); // enbale p1xck
	writeb(0x05, &(regs->reg[0x2008])); // switch b1xck/bpclk_nx to normal clocks
	writeb(0x03, &(regs->reg[0x2000])); // release ispck reset
	ispSleep(20);                     //#(`TIMEBASE*20;
	//
	writeb(0x00, &(regs->reg[0x2000])); // release all module reset
	//
	writeb(0x01, &(regs->reg[0x276c])); // reset front
	writeb(0x00, &(regs->reg[0x276c])); // 
	//
	writeb(0x03, &(regs->reg[0x2000]));
	writeb(0x00, &(regs->reg[0x2000]));
	//
	writeb(0x00, &(regs->reg[0x2010])); // cclk: 48MHz
}
#endif // #ifdef ISP_RESET

/*
	@FrontInit
	ex. FrontInit(1920, 1080);
*/
static void FrontInit(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	FRONT_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	// Conver table data to FRONT_INIT_FILE_T struct
	total = ARRAY_SIZE(FRONT_INIT_FILE);
	data[0] = FRONT_INIT_FILE[0];
	data[1] = FRONT_INIT_FILE[1];
	data[2] = FRONT_INIT_FILE[2];
	data[3] = FRONT_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPI_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPI_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = FRONT_INIT_FILE[i];
		data[1] = FRONT_INIT_FILE[i+1];
		data[2] = FRONT_INIT_FILE[i+2];
		data[3] = FRONT_INIT_FILE[i+3];
		read_file.FRONT_DATA[(i-2)/4].type = data[0];
		read_file.FRONT_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.FRONT_DATA[(i-2)/4].dat  = data[3];
		//ISPAPI_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	// Clock setting
	writeb(0x07, &(regs->reg[0x2008]));

	ISPAPI_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
	for (i = 0; i < read_file.count; i++)
	{
		//ISPAPI_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.FRONT_DATA[i].type, read_file.FRONT_DATA[i].adr, read_file.FRONT_DATA[i].dat);
		if (read_file.FRONT_DATA[i].type == 0x00)
		{
			writeb(read_file.FRONT_DATA[i].dat, &(regs->reg[read_file.FRONT_DATA[i].adr]));
		}
	}
	//
	writeb(0x00, &(regs->reg[0x2720]));	/* hoffset */
	writeb(0x00, &(regs->reg[0x2721]));
	writeb(0x00, &(regs->reg[0x2722]));	/* voffset */
	writeb(0x00, &(regs->reg[0x2723]));
	writeb(0x84, &(regs->reg[0x2724]));	/* hsize */
	writeb(0x07, &(regs->reg[0x2725]));
	writeb(0x3C, &(regs->reg[0x2726]));	/* vsize */
	writeb(0x04, &(regs->reg[0x2727]));
	writeb(0x01, &(regs->reg[0x275A]));
	writeb(0x00, &(regs->reg[0x2759]));
	writeb(0x00, &(regs->reg[0x2604]));
	//writeb(0x01, &(regs->reg[0x2007]));

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@cdspSetTable
*/
static void cdspSetTable(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	int i;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	writeb(0x00, &(regs->reg[0x2008])); //use memory clock for pixel clock, master clock and mipi decoder clock
	// R table of lens compensation tables
	writeb(0x00, &(regs->reg[0x2101])); // select lens compensation R SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0	
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_R[i], &(regs->reg[0x2103]));
	}
	//
	// G/Gr table of lens compensation tables
	writeb(0x01, &(regs->reg[0x2101])); // select lens compensation G/Gr SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_G[i], &(regs->reg[0x2103]));
	}
	//
	// B table of lens compensation tables
	writeb(0x02, &(regs->reg[0x2101])); // select lens compensation B SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	for (i = 0; i < 768; i++)
	{
		writeb(SF_LENS_COMP_B[i], &(regs->reg[0x2103]));
	}
	//
	/* write post gamma tables */
	// R table of post gamma tables
	writeb(0x04, &(regs->reg[0x2101])); // select post gamma R SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_R[i], &(regs->reg[0x2103]));
	}
	//
	// G table of post gamma tables
	writeb(0x05, &(regs->reg[0x2101])); // select post gamma G SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_G[i], &(regs->reg[0x2103]));
	}
	//
	// B table of of post gamma tables
	writeb(0x06, &(regs->reg[0x2101])); // select post gamma B SRAM
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	for (i = 0; i < 512; i++)
	{
		writeb(SF_POST_GAMMA_B[i], &(regs->reg[0x2103]));
	}
	//
	//  fixed pattern noise tables
	writeb(0x0D, &(regs->reg[0x2101])); // select fixed pattern noise
	writeb(0x03, &(regs->reg[0x2100])); // enable CPU access macro and adress auto increase
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0
	//for (i = 0; i < 1952; i++)
	for (i = 0; i < 1312; i++)
	{
		writeb(SF_FIXED_PATTERN_NOISE[i], &(regs->reg[0x2103]));
	}
	// disable set cdsp sram
	writeb(0x00, &(regs->reg[0x2104])); // select macro page 0 
	writeb(0x00, &(regs->reg[0x2102])); // set macro address to 0 
	writeb(0x00, &(regs->reg[0x2100])); // disable CPU access macro and adress auto increase 

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@sensorInit
*/
#ifdef SENSOR_INIT
static void sensorInit(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	SENSOR_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

 	// conver table data to SENSOR_INIT_FILE_T struct
	total = ARRAY_SIZE(SENSOR_INIT_FILE);
	data[0] = SENSOR_INIT_FILE[0];
	data[1] = SENSOR_INIT_FILE[1];
	data[2] = SENSOR_INIT_FILE[2];
	data[3] = SENSOR_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPI_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPI_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n",
		__FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = SENSOR_INIT_FILE[i];
		data[1] = SENSOR_INIT_FILE[i+1];
		data[2] = SENSOR_INIT_FILE[i+2];
		data[3] = SENSOR_INIT_FILE[i+3];
		read_file.SENSOR_DATA[(i-2)/4].type = data[0];
		read_file.SENSOR_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.SENSOR_DATA[(i-2)/4].dat  = data[3];
		//ISPAPI_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	Reset_I2C0(isp_info);
	Init_I2C0(SC2310_DEVICE_ADDR, 0, isp_info);

	ISPAPI_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
	for (i = 0; i < read_file.count; i++)
	{
		//ISPAPI_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);
		if (read_file.SENSOR_DATA[i].type == 0xFE)
		{
			udelay(read_file.SENSOR_DATA[i].adr*1000);
		}
		else if (read_file.SENSOR_DATA[i].type == 0x00)
		{
			setSensor16_I2C0((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1, isp_info);
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
			ISPAPI_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n",
				__FUNCTION__, i, data[0], data[1], data[2], data[3]);
		}

		ISPAPI_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			ISPAPI_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n",
				__FUNCTION__, read_file.SENSOR_DATA[i].type, read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat);

			if (read_file.SENSOR_DATA[i].type == 0xFE)
			{
				udelay(read_file.SENSOR_DATA[i].adr*1000);
			}
			else if (read_file.SENSOR_DATA[i].type == 0x00)
			{
				setSensor16_I2C0((unsigned long)read_file.SENSOR_DATA[i].adr, read_file.SENSOR_DATA[i].dat, 1, isp_info);
			}
		}
	}

	/* use pixel clock, master clock and mipi decoder clock as they are  */
	writeb(0x07, &(regs->reg[0x2008]));

	/* set and clear of front sensor interface */
	writeb(0x01, &(regs->reg[0x276C]));
	writeb(0x00, &(regs->reg[0x276C]));

	/* set and clear of front i2c interface */
	writeb(0x01, &(regs->reg[0x2660]));
	writeb(0x00, &(regs->reg[0x2660]));

	/* set and clear of cdsp */
	writeb(0x01, &(regs->reg[0x21D0]));
	writeb(0x00, &(regs->reg[0x21D0]));

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}
#endif

/*
	@CdspInit
*/
static void CdspInit(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	CDSP_INIT_FILE_T read_file;
	int i, total;
	u8 data[4];
	u8 reg_21b0 = 0x00, reg_21b1 = 0x00, reg_21b2 = 0x00, reg_21b3 = 0x00;
	u8 reg_21b4 = 0x00, reg_21b5 = 0x00, reg_21b6 = 0x00, reg_21b7 = 0x00;
	u8 reg_21b8 = 0x00;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

 	// conver table data to CDSP_INIT_FILE_T struct
	total = ARRAY_SIZE(CDSP_INIT_FILE);
	data[0] = CDSP_INIT_FILE[0];
	data[1] = CDSP_INIT_FILE[1];
	data[2] = CDSP_INIT_FILE[2];
	data[3] = CDSP_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	ISPAPI_LOGI("%s, total=%d\n", __FUNCTION__, total);
	ISPAPI_LOGI("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = CDSP_INIT_FILE[i];
		data[1] = CDSP_INIT_FILE[i+1];
		data[2] = CDSP_INIT_FILE[i+2];
		data[3] = CDSP_INIT_FILE[i+3];
		read_file.CDSP_DATA[(i-2)/4].type = data[0];
		read_file.CDSP_DATA[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.CDSP_DATA[(i-2)/4].dat  = data[3];
		//ISPAPI_LOGI("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n", __FUNCTION__, i, data[0], data[1], data[2], data[3]);
	}

	//clock setting	
	writeb(0x01, &(regs->reg[0x21d0])); //sofware reset CDSP interface (active)
	writeb(0x00, &(regs->reg[0x21d0])); //sofware reset CDSP interface (inactive)

	ISPAPI_LOGI("%s, count=%d\n", __FUNCTION__, read_file.count);
	for (i = 0; i < read_file.count; i++)
	{
		//ISPAPI_LOGI("%s, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, read_file.CDSP_DATA[i].type, read_file.CDSP_DATA[i].adr, read_file.CDSP_DATA[i].dat);

		if (read_file.CDSP_DATA[i].type == 0x00)
		{
			writeb(read_file.CDSP_DATA[i].dat, &(regs->reg[read_file.CDSP_DATA[i].adr]));
		}
	}
	//
	writeb(0x33, &(regs->reg[0x220C])); //SF_CDSP_INIT_SM
	writeb(0x23, &(regs->reg[0x21c0])); /* enable bight, contrast hue and saturation cropping mode */

	// scaler
	switch (isp_info->scale)
	{
		case SCALE_DOWN_OFF:
			ISPAPI_LOGI("Scaler is off\n");
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
			ISPAPI_LOGI("Scale down from FHD to HD size\n");
			// H = 1280 * 65536 / 1920 = 0xAAAB
			// V =	720 * 65536 / 1080 = 0xAAAB
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
			ISPAPI_LOGI("Scale down from FHD to QQVGA size\n");
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

	writeb(reg_21b0, &(regs->reg[0x21b0])); 	// factor for Hsize
	writeb(reg_21b1, &(regs->reg[0x21b1])); 	//
	writeb(reg_21b2, &(regs->reg[0x21b2])); 	// factor for Vsize
	writeb(reg_21b3, &(regs->reg[0x21b3])); 	//
	writeb(reg_21b4, &(regs->reg[0x21b4])); 	// factor for Hsize
	writeb(reg_21b5, &(regs->reg[0x21b5])); 	//
	writeb(reg_21b6, &(regs->reg[0x21b6])); 	// factor for Vsize
	writeb(reg_21b7, &(regs->reg[0x21b7])); 	//
	writeb(reg_21b8, &(regs->reg[0x21b8])); 	// disable H/V scale down

/*	                             
	#if (CDSP_SCALER_HD)//scale down from FHD  to HD size
		//H=1280*65536/(1920) = 0xAAAB
	   //V=720*65536/(1080) = 0xAAAB
		writeb(0xAB, &(regs->reg[0x21b0]));//factor for Hsize
		writeb(0xAA, &(regs->reg[0x21b1]));
		writeb(0xAB, &(regs->reg[0x21b2]));//factor for Vsize
		writeb(0xAA, &(regs->reg[0x21b3]));
		//
		writeb(0xAB, &(regs->reg[0x21b4]));//factor for Hsize
		writeb(0xAA, &(regs->reg[0x21b5]));
		writeb(0xAB, &(regs->reg[0x21b6]));//factor for Vsize
		writeb(0xAA, &(regs->reg[0x21b7]));
		//
		writeb(, &(regs->reg[0x21b8) = 0x2F;	//enable	
	#elif (CDSP_SCALER_VGA)//scale down from FHD to VGA size	
	   //H=640*65536/(1920) = 0x5556
	   //V=480*65536/(1080) = 0x71C8
		writeb(0x56, &(regs->reg[0x21b0]));//factor for Hsize
		writeb(0x55, &(regs->reg[0x21b1]));
		writeb(0xC8, &(regs->reg[0x21b2]));//factor for Vsize
		writeb(0x71, &(regs->reg[0x21b3]));
		//
		writeb(0x56, &(regs->reg[0x21b4]));//factor for Hsize
		writeb(0x55, &(regs->reg[0x21b5]));
		writeb(0xC8, &(regs->reg[0x21b6]));//factor for Vsize
		writeb(0x71, &(regs->reg[0x21b7]));
		//
		writeb(0x2F, &(regs->reg[0x21b8]));	//enable	
	#elif (CDSP_SCALER_QQVGA)//scale down from FHD  to QVGA size	     
		//H=160*65536/(1920) = 0x1556
	    //V=120*65536/(1080) = 0x1C72
		writeb(0x56, &(regs->reg[0x21b0]));//factor for Hsize
		writeb(0x15, &(regs->reg[0x21b1]));
		writeb(0x72, &(regs->reg[0x21b2]));//factor for Vsize
		writeb(0x1C, &(regs->reg[0x21b3]));
		//
		writeb(0x56, &(regs->reg[0x21b4]));//factor for Hsize
		writeb(0x15, &(regs->reg[0x21b5]));
		writeb(0x72, &(regs->reg[0x21b6]));//factor for Vsize
		writeb(0x1C, &(regs->reg[0x21b7]));
		//
		writeb(0x2F, &(regs->reg[0x21b8]));	//enable
	#else	
		writeb(0x00, &(regs->reg[0x21b8])); //disable H/V scale down                              
	#endif                           
*/

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

/*
	@ispAaaInit
*/
static void ispAaaInit(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

#ifdef ENABLE_3A
	aaaInitVar(isp_info);
	aaaLoadInit(isp_info, AAA_INIT_FILE);
	aeLoadAETbl(isp_info, AE_EXP_TABLE); 
	aeLoadGainTbl(isp_info, AE_GAIN_TABLE); 
	aeInitExt(isp_info);
	awbInit(isp_info);
#endif

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

int isVideoStart(void)
{
	return VideoStart;
}

void videoStartMode(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	FrontInit(isp_info);
	CdspInit(isp_info);
	ispAaaInit(isp_info);
#ifdef SENSOR_INIT
	sensorInit(isp_info);
#endif

	/* set and clear reset of buffer cdsp interface */
	writeb(readb(&(regs->reg[0x2300]))|0x08, &(regs->reg[0x2300]));
	writeb(readb(&(regs->reg[0x2300]))&0xF7, &(regs->reg[0x2300]));

#ifdef ENABLE_3A
	vidctrlInit(isp_info, SC2310_GAIN_ADDR, SC2310_FRAME_LEN_ADDR, SC2310_LINE_TOTAL_ADDR,
				SC2310_EXP_LINE_ADDR, SC2310_PCLK);
	VideoStart = 1;
	//InstallVSinterrupt();
	//writeb(2, &(regs->reg[0x27B0]));    /* clear vd falling edge interrupt AAF061 W1C*/
	//writeb(readb(&(regs->reg[0x27C0]))|0x02, &(regs->reg[0x27C0]));     /* enable vd falling edge interrupt */
	//writeb(, &(regs->reg[0x2B00) = 0x01;   /* enable TNR */
#endif

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

void videoStopMode(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

#ifdef ENABLE_3A	
	VideoStart = 0;
	//writeb(readb(&(regs->reg[0x27C0]))&0xFD, &(regs->reg[0x27C0]));     /* disable vd falling edge interrupt */
	//writeb(2, &(regs->reg[0x27B0]));    /* clear vd falling edge interrupt AAF061 W1C*/
#endif

	/* set reset of buffer jpeg, cdsp and usb interface */
	writeb(0x1C, &(regs->reg[0x2300]));
	writeb(0x00, &(regs->reg[0x2300]));

	/* use memory clock for pixel clock, master clock and mipi decoder clock */
	writeb(0x00, &(regs->reg[0x2008]));

	/* set reset of cdsp */
	writeb(0x01, &(regs->reg[0x21D0]));

	/* set reset of front sensor interface */
	writeb(0x01, &(regs->reg[0x276C]));

	/* set and clear reset of front i2c interface */
	writeb(0x01, &(regs->reg[0x2660]));
	writeb(0x00, &(regs->reg[0x2660]));

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

void ispVsyncIntCtrl(struct mipi_isp_info *isp_info, u8 on)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	if (on) {
		// Clear vd falling edge interrupt AAF061 W1C
		writeb(2, &(regs->reg[0x27B0]));
		// Enable vd falling edge interrupt
		writeb(readb(&(regs->reg[0x27C0]))|0x02, &(regs->reg[0x27C0]));
	}
	else {
		// Disable vd falling edge interrupt
		writeb(readb(&(regs->reg[0x27C0]))&0xFD, &(regs->reg[0x27C0]));
		// Clear vd falling edge interrupt AAF061 W1C
		writeb(2, &(regs->reg[0x27B0]));
	}
}

void ispVsyncInt(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	if (((readb(&(regs->reg[0x27C0])) & 0x02) == 0x02) &&	/* sensor vd falling interrupt enable */
		((readb(&(regs->reg[0x27B0])) & 0x02) == 0x02)) {	/* sensor vd falling interrupt event */
#ifdef ENABLE_3A
		intrIntr0SensorVsync(isp_info);
		aaaAeAwbAf(isp_info);
#endif
		// clear vd falling edge interrupt AAF061 W1C
		writeb(0x02, &(regs->reg[0x27B0]));
	}
}

void isp_setting(struct mipi_isp_info *isp_info)
{
	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	//ispReset(isp_info);
	cdspSetTable(isp_info);

	/* check video stream from camera */
	//powerSensorOn_RAM(isp_info);
	//videoStartMode(isp_info);

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}
