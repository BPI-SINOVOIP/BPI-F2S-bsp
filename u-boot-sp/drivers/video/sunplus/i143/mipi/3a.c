#include <common.h>
#include "isp_api.h"
#include "i2c_api.h"
#include "3a.h"

#define INTR_VD_EDGE					(0x02)
#define MANUALEXPGAIN					(0xa0)
#define PUBRIGHTNESSCUR					(0x0080)
#define PUCONTRASTCUR					(0x0020)
#define PUGAINCUR						(0x0020)
#define PUBACKLIGHTCOMPENSATIONCUR		(0x0001)
#define PUBACKLIGHTCOMPENSATIONDEF		(0x0001)
#define PUHUECUR						(0x0000)
#define PUSATURATIONCUR					(0x0040)
#define PUSHARPNESSCUR					(0x0002)
#define PUGAMMACUR						(0x005a)
#define PUWHITEBALANCETEMPERATURECUR	(0x0fa0)
#define PUPOWERLINEFREQUENCYCUR			(0x0002) //1:50HZ, 2:60Hz
#define AEINDEX							(0x0074)
#define AEINDEXPREV						(0x0073)
#define AEINDEXMAX60					(0x00a7)
#define AEINDEXMAX50					(0x00a7)
#define AEINDEXMIN60					(0x0000)
#define AEINDEXMIN50					(0x0000)
#define AEDEADZONE						(0x0004)
#define AEYTARGET						(0x0030)
#define AWBBTARGETMIN					(0x005c)
#define AWBBTARGETMAX					(0x00ba)
#define AWBRTARGETMIN					(0x0046)
#define AWBRTARGETMAX					(0x0080)
#define AWBGTARGETMIN					(0x0040)
#define AWBGTARGETMAX					(0x0040)
#define AWBRGAINREG						(0x0070)
#define AWBGGAINREG						(0x0040)
#define AWBBGAINREG						(0x006e)
#define AAAFREQ							(0x0003)
#define CTEXPOSURETIMEABSOLUTECUR		(0x0000009C)
#define CTEXPOSURETIMEABSOLUTEMIN		(0x00000004)
#define CTEXPOSURETIMEABSOLUTEMAX		(0x000004e2)
#define AWB_R_WIDTH				 6
#define AWB_B_WIDTH				 8
#define AWB_MIN_GRAY_DET_CNT	0x3000 
#define AWB_CHECK_BLK_CNT		13
#define	AWB_CM_ADJ_CNT			3
extern int isVideoStart(void); //in isp_api.c

u8 AwbRRef[AWB_CHECK_BLK_CNT] = {  62,  65, 68, 73, 78, 83, 88, 92, 96, 100, /*CW 10~12*/  86, 91, 95 };
u8 AwbBRef[AWB_CHECK_BLK_CNT] = { 128, 120, 114, 108, 103, 97, 92, 86, 80, 75, /*CW 10~12*/ 103, 97, 92};
u16	AwbAdjCmColTemp[AWB_CM_ADJ_CNT] = {  42, 64, 86};
short AwbAdjCmColMat[AWB_CM_ADJ_CNT][9] = 
{
	{0x4c,-(0x100-0xf6),-(0x100-0xfe),-(0x100-0xf3),0x53,-(0x100-0xfa),0x3,-(0x100-0xeb),0x52}, //D
	{0x4c,-(0x100-0xf8),-(0x100-0xfc),-(0x100-0xf8),0x4c,-(0x100-0xfc),0x9,-(0x100-0xf0),0x47}, //Office
	{0x50,-(0x100-0xf8),-(0x100-0xf8),-(0x100-0xf4),0x50,-(0x100-0xfc),-(0x100-0xf9),-(0x100-0xf8),0x4f}, //86
};

#define	AWB_LENS_ADJ_CNT	3
short AwbAdjLensColTemp[AWB_LENS_ADJ_CNT] =	{ 122, 136, 180 };
short AwbAdjLensRVal[AWB_LENS_ADJ_CNT] =		{100, 113, 138 };
short AwbAdjLensBVal[AWB_LENS_ADJ_CNT] =		{100, 100, 100 };

#define	AE_EV_CONST					500

short Sign_CosSin[8] = {1, 1, -1, 1, -1, -1, 1, -1};
u8 Sin0_90[91] = {
	0x00, 0x04, 0x09, 0x0d, 0x12, 0x16, 0x1b, 0x1f, 0x24, 0x28,
	0x2c, 0x31, 0x35, 0x3a, 0x3e, 0x42, 0x47, 0x4b, 0x4f, 0x53,
	0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c,
	0x80, 0x84, 0x88, 0x8b, 0x8f, 0x93, 0x96, 0x9a, 0x9e, 0xa1,
	0xa4, 0xa8, 0xab, 0xaf, 0xb2, 0xb5, 0xb8, 0xbb, 0xbe, 0xc1,
	0xc4, 0xc7, 0xca, 0xcc, 0xcf, 0xd2, 0xd4, 0xd7, 0xd9, 0xdb,
	0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xed, 0xef,
	0xf1, 0xf2, 0xf3, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
	0xfc, 0xfd, 0xfd, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff};
u16 sensor_gain_addr = 0;
u16 sensor_frame_len_addr = 0;
u16 sensor_line_total_addr = 0;
u16 sensor_exp_line_addr = 0;
u32 sensor_pclk = 0;
u32 ctExposureTimeAbsoluteCur = CTEXPOSURETIMEABSOLUTECUR;
u8 AeTest = 0;
u16 AeExp = 0;
u16 AeExpGet = 0;
u8 AeExpLineMode = 0;
u8 AePreExpLineMode = 0;
u16 AeLineModeExpLineCount = 0;
u16 SensorLineCountOut = 0;
u16 AeCurExpLine = 0;
u16 SensorMinFrameLen = 0;
u16 AeGain = 0;
u16 AeGainGet = 0;
u32 AeEvConst = 0;
u32 AeCurEv = 0;
u16 pAeGainTbl[256], pAeExpTbl[256];
u8 AeTblCntVal = 0;
u16* AeExpTableSfAddr = 0;
u16* AeGainTableSfAddr = 0;
u16 AeExpLineCount120 = 0;
u16 SensorExpIn = 0;
u16 AwbCurColTemp = 0;
u16 AwbColTemp = 0;
u16 AwbBGainReg = 0;
u16 AwbRGainReg = 0;
u16 AwbGGainReg = 0;
u16 AwbRawRGain = 0;
u16 AwbRawGGain = 0;
u16 AwbRawBGain = 0;
u16 AwbRGainTarget = 0;
u16 AwbGGainTarget = 0;
u16 AwbBGainTarget = 0;
u16 AwbPreRTarget = 0;
u16 AwbDeadZone = 0;
u16 AwbPreBTarget = 0;
u32 AwbSumCount = 0;
u32 AwbSumR = 0;
u32 AwbSumG = 0;
u32 AwbSumB = 0;
u32 AeImgSize = 0;
u8 AeDarkMode = 0;
u8 AeDeadZone = 0;
u16 AeDeadLeafWinSize = 0;
u8 AeDarkThresh0 = 0;
short AeIndex = AEINDEX;
short AeIndexPrev = AEINDEXPREV;
short AeIndexGet = 0;
u8 AeIndexMax = 0xdd;
u8 AaaFreq = 0;
u8 AeIndexMin = 0;
u8 AeIndexMax60 = AEINDEXMAX60;
u8 AeIndexMax50 = AEINDEXMAX50;
u8 AeIndexMin60 = AEINDEXMIN60;
u8 AeIndexMin50 = AEINDEXMIN50;
u8 aaaFlag  =  0, aaaCount  =  0;
u8 AwbMinGain = 0;
u8 AwbFirstInit = 1;
u8 AwbMode = 0;

u8 AwbRWidth = 0;
u8 AwbBWidth = 0;
u32 AwbMinGrayDetCnt = 0;
u16 AwbCheckBlkCnt = 0;
u8* pAwbRRef = 0;
u8* pAwbBRef = 0;
u8 AwbCmAdjCnt = 0;
u16* pAwbAdjCmColTemp = 0;
short* pAwbAdjCmColMat = 0;
u8 AwbLensAdjCnt = 0;
short* pAwbAdjLensColTemp = 0;
short* pAwbAdjLensRVal = 0;
short* pAwbAdjLensBVal = 0;
short AeAdjYTarget = 0;
u16 AeYTarget = 0;
u8 AwbBTargetMin = 0;
u8 AwbBTargetMax = 0;
u8 AwbRTargetMin = 0;
u8 AwbRTargetMax = 0;
u8 AwbGTargetMin = 0;
u8 AwbGTargetMax = 0;
short AeYLayer = 0;
u16 puBacklightCompensationCur = PUBACKLIGHTCOMPENSATIONCUR;
u16 puBacklightCompensationDef = PUBACKLIGHTCOMPENSATIONDEF;
u8* pAeNormWeight = 0;
u8 AeStable = 0;
u8 AeStill = 0;

u32 sensorGainRegToVal(u16 gainRegVal)
{
	u32 gain = 0;
	u16 hi_gain,lo_gain;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	hi_gain = (gainRegVal >> 8) & 0xff;
	lo_gain = gainRegVal & 0xff;

	if(hi_gain == 0x03)
		gain = lo_gain * 25;
	else if(hi_gain == 0x07)
		gain = lo_gain * 50;
	else if(hi_gain == 0x23)
		gain = lo_gain * 68;
	else if(hi_gain == 0x27)
		gain = lo_gain * 136;
	else if(hi_gain == 0x2f)
		gain = lo_gain * 272;
	
	return(gain);
}

void vidctrlPuBrightness(short puBrightnessCur)
{
	short tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	tmp = puBrightnessCur - 0x80;

	if(tmp < -128)
		tmp = -128;
	else
		if(tmp > 127) tmp = 127;

	ISPAPB0_REG8(0x21C1) = tmp;
}

void vidctrlPuContrast(short puContrastCur)
{
	short wContrastAdj;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	wContrastAdj = puContrastCur;

	if(1)
	{
		ISPAPB0_REG8(0x21c0) = 0x23;

		if (wContrastAdj >= 4)
			wContrastAdj -= 4;
		else
			wContrastAdj = 0;
	}
	//else
	//{
	//	ISPAPB0_REG8(0x21c0) = 0x03;
	//}

	if (wContrastAdj > 255)
		wContrastAdj = 255;
	else if(wContrastAdj < 0)
		wContrastAdj = 0;

	ISPAPB0_REG8(0x21C2) = wContrastAdj;
}

short mathlibCos256x_ROM(short x)
{
	short res;
	u8 sign_idx;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (x < -90)
	{
		sign_idx = 4;	/* III */
		x = 180 + x;
	}
	else if (x < 0)
	{
		sign_idx = 6;	/* IV */
		x *= -1;
	}
	else if (x < 90)
	{
		sign_idx = 0;	/* I */
	}
	else
	{
	  	sign_idx = 2;	/* II */
		x = 180 - x;
	}

	res = Sign_CosSin[sign_idx]*Sin0_90[90-x];
	//res = cos(x);

	return res;
}

short mathlibSin256x_ROM(short x)
{
	short res;
	u8 sign_idx;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (x < -90)
	{
		sign_idx = 4;	/* III */
		x = 180 + x;
	}
	else if (x < 0)
	{
		sign_idx = 6;	/* IV */
		x *= -1;
	}
	else if (x < 90)
	{
		sign_idx = 0;	/* I */
	}
	else
	{
	  	sign_idx = 2;	/* II */
		x = 180 - x;
	}

	res = Sign_CosSin[sign_idx+1]*Sin0_90[x];
	//res = sin(x);

	return res;
}

void vidctrlPuHue(short puHueCur)
{
	short wHue;
	short sin, cos;
	u8 temp1, temp2;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	wHue = puHueCur ;

	sin = mathlibSin256x_ROM(wHue);
	cos = mathlibCos256x_ROM(wHue);

	if (sin >= 0)
		temp1 = sin/4;
	else
		temp1 = ((sin/4) | 0x80);
	if (cos >= 0)
		temp2 = cos/4;
	else
		temp2 = ((cos/4) | 0x80);

	ISPAPB0_REG8(0x21C3) = temp1;
	ISPAPB0_REG8(0x21C4) = temp2;
}

void vidctrlPuSaturation(short puSaturationCur)
{
	short sat;
	//char AdjByAe;
	short DefVal;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	sat = puSaturationCur;

	if(ISPAPB0_REG8(0x21c0) == 0x23) DefVal = 0x2a; //0x31;	//x28;//2c	//yuv	//jim:27
	else DefVal = 0x30;

	if(AeIndex >= 160)
		DefVal = DefVal - 5;
	else if(AeIndex >= 140)
		DefVal = DefVal - 5*(AeIndex-140)/(160-140);

	
#if 1
	//80lux E27 Warm AwbCurColTemp = 164		aeindex = 185(0xb9)
	//20lux 3000k    AwbCurColTemp = 135 		aeindex = 210(0xd0)
	//80lux 3000k    AwbCurColTemp = 145    aeindex = 186(0xba)
	
	if(AwbCurColTemp < 100) 
		DefVal = DefVal+0;
	else if(AwbCurColTemp > 150) 
		DefVal = DefVal+16;
	else 
		DefVal =  DefVal+0+(16-0)*(AwbCurColTemp-100)/(150-100);
#endif	
	
	sat = (sat*DefVal)/64;

	if(sat > 255)
		sat = 255;
	if(sat < 0)
		sat = 0;

	ISPAPB0_REG8(0x21C5) = sat;
}

void vidctrlPuSharpness(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//N/A
}

void vidctrlPuGamma(u16 puGammaCur)
{
	u8 i;
	u8 refGamma[15] = {  1,  4,  9, 16,  25,  36,  49,  64,  81,  98, 124, 146, 170, 196,  225 }; // Gamma 2 for wGamma = 150
	u8 defGamma[15] = { 29, 51, 70, 89, 104, 120, 134, 147, 159, 174, 187, 200, 213, 226, 240 }; // Gamma 1 for wGamma = 120
	short gamma;

ISP3A_LOGD("%s start\n", __FUNCTION__);

//	if(AeDarkMode == 0)
	{
		for (i = 0; i < 15; i++)
		{
		gamma = (((short)puGammaCur - 120) * ((short)defGamma[i] - (short)refGamma[i])) / 40 + (short)defGamma[i];
		if (gamma > 0xFF)
			gamma = 0xFF;
		else if (gamma < 0)
			gamma = 0;

		ISPAPB0_REG8((unsigned long)(0x2161+i)) = (u8)gamma;
		}

	}
//	else
//	{
//		for(i=0; i<12; i++) XBYTE[0x2161+i] = 0;
//		XBYTE[0x2161+12] = 50;
//		XBYTE[0x2161+13] = 100;
//		XBYTE[0x2161+14] = 200;
//	}
}

void aeGetGainIsr(void)
{
	//u16 sfAddr;
	u16* sfAddr;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//sfAddr = AeGainTableSfAddr + (AeIndexGet << 1);
	//AeGainGet=pAeGainTbl[sfAddr];
	sfAddr = AeGainTableSfAddr + AeIndexGet;
	AeGainGet=*sfAddr;

	//SfSpi.sfAddr = (u8 *)&sfAddr;
	//SfSpi.sfData = (u8 *)&AeGainGet;
	//SfSpi.sfCount = 2;
	//sfReadBytesIsr_ROM();
}

void aeGetExposureIsr(void)
{
	//u16 sfAddr;
	u16* sfAddr;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//sfAddr = AeExpTableSfAddr + (AeIndexGet << 1);
	//AeExpGet=pAeExpTbl[sfAddr];
	sfAddr = AeExpTableSfAddr + AeIndexGet;

	ISP3A_LOGD("sfAddr=0x%p, AeExpTableSfAddr=0x%p, AeIndexGet=%d\n", 
				sfAddr, AeExpTableSfAddr, AeIndexGet);

	AeExpGet=*sfAddr;

	//SfSpi.sfAddr = (u8 *)&sfAddr;
	//SfSpi.sfData = (u8 *)&AeExpGet;
	//SfSpi.sfCount = 2;
	//sfReadBytesIsr_ROM();

	ISP3A_LOGD("%s end\n", __FUNCTION__);
}

u32 aeGetDeltaEv(u32 desireEv, short idx)
{
	u32 deltaEv;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//EA = 0;	/* disable all interrupts */

	AeIndexGet = AeIndex = idx;
	aeGetExposureIsr();
	aeGetGainIsr();

	//EA = 1;	/* Enable all interrupts */

	AeExp = AeExpGet;
	AeGain = AeGainGet;
	aeGetCurEv();

	if (desireEv > AeCurEv)
		deltaEv = desireEv - AeCurEv;
	else
		deltaEv = AeCurEv - desireEv;

	return(deltaEv);
}

void vidctrlPuBacklightCompensation(short puPowerLineFrequencyCur)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if(isVideoStart()==1)
	{
		if ((puPowerLineFrequencyCur == 2))
		{	
			AeExpTableSfAddr = pAeExpTbl;//SF_AE_EXP_60;
			AeGainTableSfAddr = pAeGainTbl;//SF_AE_GAIN_60;
			AeIndexMax = 0xdd;//235-2;
		}
		else
		{			
			AeExpTableSfAddr = pAeExpTbl;//SF_AE_EXP_50;
			AeGainTableSfAddr = pAeGainTbl;//SF_AE_GAIN_50;
			AeIndexMax = 0xdd;//235-2;//0xe9
		}

		if(AeIndex > AeIndexMax) AeIndex = AeIndexMax;
		if(AeIndex < 0) AeIndex = 0;
		AeIndexGet = AeIndex;

		if(1)//UvcVcCtrl.ctAutoExposureModeCur ==0x08)
		{
			if(AeExpLineMode == 0)
			{
				//EA = 0;	/* disable all interrupts */
				aeGetExposureIsr();
				AeExp = AeExpGet;
			}
			else
			{
				AeIndex = 0;
			}

			//EA = 0;	/* disable all interrupts */
			aeGetGainIsr();
			AeGain = AeGainGet;			
			sensorSetExposureTimeIsr();
			sensorSetGainIsr();
			//EA = 1;	/* Enable all interrupts */
			aaaFlag = 0;
			aaaCount = 0;
			aeGetCurEv();
		}
	}
	AeIndexPrev = AeIndex;
}

void vidctrlPuPowerLineFrequency(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//N/A
}

void vidctrlPuGain(void)
{
	//N/A
}

void sensorSetGainIsr(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	setSensor16_I2C0(sensor_gain_addr, AeGain, 2);	
}

void sensorExpToLineCountIsr(void)
{
	u16 lineTotal;
	//u8 pllMul;
	//u8 pllDiv, pllDiv2, pllDiv4;
	u16 expLine;
	//u8 regAddr[2], regData[2];
	u32 pixelClock;
	u16 exp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* get input */
	exp = SensorExpIn;

	/* get pixel clock */
	pixelClock = sensor_pclk;
	//
	getSensor16_I2C0(sensor_line_total_addr, &lineTotal, 2);
	lineTotal *= 2;
	//
	/* calculate exposure line */
	/* expLine line/frame = (pixelClock pixel/s / AeExp frame/s) / lineTotal pixel/line */
	//expLine = (u16)(((u32)16 * pllMul * 24000000) / ((u32)lineTotal * pllDiv * pllDiv2 * pllDiv4 * AeExp));
	//pixelClock = ((u32)6000000 * pllMul) / ((u32)pllDiv * pllDiv2 * pllDiv4);
	if (exp == 0x215)	/* 1600/3 */
		expLine = (u16)(((u32)3 * pixelClock) / ((u32)lineTotal * 100));
	else if (exp == 0x10b)	/* 1600/6 */
		expLine = (u16)(((u32)6 * pixelClock) / ((u32)lineTotal * 100));
	else if (exp == 0x85)	/* 1600/12 */
		expLine = (u16)(((u32)6 * pixelClock) / ((u32)lineTotal * 50));
	else if(exp == 0x7b)	//1600/13
		expLine = (u16)(((u32)13 * pixelClock) / ((u32)lineTotal * 100));
	else
		expLine = (u16)(((u32)pixelClock * 16) /((u32)lineTotal * exp));

	/* set output */
	SensorLineCountOut = expLine;
}
	
void sensorSetExposureTimeIsr(void)
{
	u16 expLine;
	u16 frameLen;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* get exposure line */
	if (AeExpLineMode == 1)
	{
		expLine = AeLineModeExpLineCount;
	}
	else
	{
		SensorExpIn = AeExp;
		sensorExpToLineCountIsr();
		expLine = SensorLineCountOut;
	}
	AeCurExpLine = expLine;

	/* set frame length */
	frameLen = SensorMinFrameLen;
	if ((expLine) > (frameLen-4))
		frameLen = expLine + 4;

	if (expLine < 4) // sensor minimun expline
		expLine = 4;
	expLine = expLine<<4;
	//
	setSensor16_I2C0(sensor_frame_len_addr, frameLen, 2);
	setSensor16_I2C0(sensor_exp_line_addr, expLine, 2);
}

void vidctrlCtExposureTimeAbsolute(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (ctExposureTimeAbsoluteCur!= 0)
	{
		AeExp = 160000 / (ctExposureTimeAbsoluteCur);
		AeExpLineMode = 0;
		AeGain = MANUALEXPGAIN;

		//EA = 0;	/* disable all interrupts */
		sensorSetExposureTimeIsr();
		sensorSetGainIsr();
		//EA = 1;	/* Enable all interrupts */

	}
}

void vidctrlPuWhiteBalanceTemperature(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	// N/A
}
/*
	Do once only after each VideoStart
*/
void vidctrlInit(u16 _sensor_gain_addr, 
				u16 _sensor_frame_len_addr, 
				u16 _sensor_line_total_addr, 
				u16 _sensor_exp_line_addr,
				u32 _sensor_pclk)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	sensor_gain_addr=_sensor_gain_addr;
	sensor_frame_len_addr=_sensor_frame_len_addr;
	sensor_line_total_addr=_sensor_line_total_addr;
	sensor_exp_line_addr=_sensor_exp_line_addr;
	sensor_pclk=_sensor_pclk;

	// add table init for aeGetExposureIsr()
	AeExpTableSfAddr = pAeExpTbl;//SF_AE_EXP_60;
	AeGainTableSfAddr = pAeGainTbl;//SF_AE_GAIN_60;
	AeIndexMax = 0xdd;//235-2;
	getSensor16_I2C0(sensor_frame_len_addr, &SensorMinFrameLen, 2);
	//
	//EA = 0; /* disable all interrupts */
	AeIndexGet = AeIndex;
	aeGetExposureIsr();
	AeExp = AeExpGet;
	aeGetGainIsr();
	AeGain = AeGainGet;
	AeStill = 0;
	sensorSetExposureTimeIsr();
	sensorSetGainIsr();
	//EA = 1; /* enable all interrupts */
	//

	vidctrlPuBrightness(PUBRIGHTNESSCUR);
	vidctrlPuContrast(PUCONTRASTCUR);
	vidctrlPuHue(PUHUECUR);
	vidctrlPuSaturation(PUSATURATIONCUR);
	vidctrlPuSharpness();
	vidctrlPuGamma(PUGAMMACUR);
	vidctrlPuBacklightCompensation(PUPOWERLINEFREQUENCYCUR);
	vidctrlPuPowerLineFrequency();
	vidctrlPuGain();
	vidctrlCtExposureTimeAbsolute();
	vidctrlPuWhiteBalanceTemperature();
}

void aeSetWin_ROM(u16 hsize, u16 vsize, u16 hOffset, u16 vOffset)
{	
	u16 winHSize, winVSize;
	//	

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	ISPAPB0_REG8(0x3206) = 7;//9x9

	/* HwdOffset */
	ISPAPB0_REG8(0x2200) = LO_BYTE_OF_WORD(hOffset);
	ISPAPB0_REG8(0x2201) = HI_BYTE_OF_WORD(hOffset);
	/* VwdOffset */
	ISPAPB0_REG8(0x2202) = LO_BYTE_OF_WORD(vOffset);
	ISPAPB0_REG8(0x2203) = HI_BYTE_OF_WORD(vOffset);

	/* Full window size */
	winHSize = hsize - 2 * hOffset;
	winVSize = vsize - 2 * vOffset;
	/* FullHSize */
	ISPAPB0_REG8(0x2208) = LO_BYTE_OF_WORD(winHSize);
	ISPAPB0_REG8(0x2209) = HI_BYTE_OF_WORD(winHSize);
	/* FullVSize */
	ISPAPB0_REG8(0x220A) = LO_BYTE_OF_WORD(winVSize);
	ISPAPB0_REG8(0x220B) = HI_BYTE_OF_WORD(winVSize);
	/* FullHaccFac */
	if (winHSize < 8)			{ ISPAPB0_REG8(0x220D) = 0x00; }
	else if (winHSize < 16) 	{ ISPAPB0_REG8(0x220D) = 0x01; }
	else if (winHSize < 32) 	{ ISPAPB0_REG8(0x220D) = 0x02; }
	else if (winHSize < 64) 	{ ISPAPB0_REG8(0x220D) = 0x03; }
	else if (winHSize < 128) 	{ ISPAPB0_REG8(0x220D) = 0x04; }
	else if (winHSize < 256) 	{ ISPAPB0_REG8(0x220D) = 0x05; }
	else if (winHSize < 512) 	{ ISPAPB0_REG8(0x220D) = 0x06; }
	else if (winHSize < 1024)	{ ISPAPB0_REG8(0x220D) = 0x07; }
	else if (winHSize < 2048)	{ ISPAPB0_REG8(0x220D) = 0x08; }
	else 						{ ISPAPB0_REG8(0x220D) = 0x09; }
	/* FullVaccFac */
	if (winVSize < 8)			{ ISPAPB0_REG8(0x220D) |= 0x00; }
	else if (winVSize < 16) 	{ ISPAPB0_REG8(0x220D) |= 0x10; }
	else if (winVSize < 32) 	{ ISPAPB0_REG8(0x220D) |= 0x20; }
	else if (winVSize < 64) 	{ ISPAPB0_REG8(0x220D) |= 0x30; }
	else if (winVSize < 128) 	{ ISPAPB0_REG8(0x220D) |= 0x40; }
	else if (winVSize < 256) 	{ ISPAPB0_REG8(0x220D) |= 0x50; }
	else if (winVSize < 512) 	{ ISPAPB0_REG8(0x220D) |= 0x60; }
	else if (winVSize < 1024)	{ ISPAPB0_REG8(0x220D) |= 0x70; }
	else if (winVSize < 2048)	{ ISPAPB0_REG8(0x220D) |= 0x80; }
	else						{ ISPAPB0_REG8(0x220D) |= 0x90; }

	/* HwdOffset */
	ISPAPB0_REG8(0x3208) = LO_BYTE_OF_WORD(hOffset);
	ISPAPB0_REG8(0x3209) = HI_BYTE_OF_WORD(hOffset);
	/* VwdOffset */
	ISPAPB0_REG8(0x320a) = LO_BYTE_OF_WORD(vOffset);
	ISPAPB0_REG8(0x320b) = HI_BYTE_OF_WORD(vOffset);


	/* Individual window size */
	winHSize /= 9;
	winVSize /= 9;
	/* AeHSize */
	ISPAPB0_REG8(0x320c) = LO_BYTE_OF_WORD(winHSize );
	ISPAPB0_REG8(0x320d) = HI_BYTE_OF_WORD(winHSize );
	/* AeVSize */
	ISPAPB0_REG8(0x320e) = LO_BYTE_OF_WORD(winVSize);
	ISPAPB0_REG8(0x320f) = HI_BYTE_OF_WORD(winVSize);
	/* FullHaccFac */
	if (winHSize  < 8)			{ ISPAPB0_REG8(0x3207) = 0x00; }
	else if (winHSize  < 16) 	{ ISPAPB0_REG8(0x3207) = 0x01; }
	else if (winHSize  < 32) 	{ ISPAPB0_REG8(0x3207) = 0x02; }
	else if (winHSize  < 64) 	{ ISPAPB0_REG8(0x3207) = 0x03; }
	else if (winHSize  < 128) 	{ ISPAPB0_REG8(0x3207) = 0x04; }
	else if (winHSize  < 256) 	{ ISPAPB0_REG8(0x3207) = 0x05; }
	else if (winHSize  < 512) 	{ ISPAPB0_REG8(0x3207) = 0x06; }
	else /*if (winHSize < 1024)*/	{ ISPAPB0_REG8(0x3207) = 0x07; }

	/* FullVaccFac */
	if (winVSize < 8)			{ ISPAPB0_REG8(0x3207) |= 0x00; }
	else if (winVSize < 16) 	{ ISPAPB0_REG8(0x3207) |= 0x10; }
	else if (winVSize < 32) 	{ ISPAPB0_REG8(0x3207) |= 0x20; }
	else if (winVSize < 64) 	{ ISPAPB0_REG8(0x3207) |= 0x30; }
	else if (winVSize < 128) 	{ ISPAPB0_REG8(0x3207) |= 0x40; }
	else if (winVSize < 256) 	{ ISPAPB0_REG8(0x3207) |= 0x50; }
	else if (winVSize < 512) 	{ ISPAPB0_REG8(0x3207) |= 0x60; }
	else /*if (winVSize < 1024)*/	{ ISPAPB0_REG8(0x3207) |= 0x70; }
}

void aeFuncExt(void)
{
	//u16 tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if(AeIndex > 130)
	{
		 ISPAPB0_REG8(0x2175) = 12;
		 ISPAPB0_REG8(0x2176) = 24;
		 ISPAPB0_REG8(0x31D0) = ISPAPB0_REG8(0x31D1) = ISPAPB0_REG8(0x31D2) = ISPAPB0_REG8(0x31D3) = 32;
		 ISPAPB0_REG8(0x31FE) = 30;
		 ISPAPB0_REG8(0x31FF) = 60;
	}
	else if(AeIndex < 100)
	{
		 ISPAPB0_REG8(0x2175) = 6;
		 ISPAPB0_REG8(0x2176) = 12;
		 ISPAPB0_REG8(0x31D0) = ISPAPB0_REG8(0x31D1) = ISPAPB0_REG8(0x31D2) = ISPAPB0_REG8(0x31D3) = 12;
		 ISPAPB0_REG8(0x31FE) = 10;
		 ISPAPB0_REG8(0x31FF) = 20;
	}
	else
	{
		 ISPAPB0_REG8(0x2175) = 6 + (12-6)*(AeIndex-100)/(130-100);
		 ISPAPB0_REG8(0x2176) = 12 + (24-12)*(AeIndex-100)/(130-100);
		 ISPAPB0_REG8(0x31D0) = ISPAPB0_REG8(0x31D1) = ISPAPB0_REG8(0x31D2) = ISPAPB0_REG8(0x31D3) = 12 + (32-12)*(AeIndex-100)/(130-100);
		 ISPAPB0_REG8(0x31FE) = 10 + (30-10)*(AeIndex-100)/(130-100);
		 ISPAPB0_REG8(0x31FF) = 20 + (60-20)*(AeIndex-100)/(130-100);
	}
}

void aeGetCurEv(void)
{
	u32 gainVal;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (AeExpLineMode == 0)
	{
		gainVal = sensorGainRegToVal(AeGain);
		AeCurEv = AeEvConst * gainVal / (u32)AeExp;
	}
	else
		AeCurEv = pAeExpTbl[0];
	// else
	// {
	// 	gainVal = sensorGainRegToVal(AeGain);
	// 	AeCurEv = AeEvConst * (u32)AeCurExpLine * gainVal / (u32)0x780 / AeExpLineCount120;
	// }
}

void aeDeadLeafWinInit(u16 hsize, u16 vsize, u16 winSize)
{
	u16 val;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* AF window */
	/* HwdOffset */
	//val = (pStrm->strmFrontHsize - winSize) / 2;
	val = ((hsize-4) - winSize) / 2;
	ISPAPB0_REG8(0x22a0) = LO_BYTE_OF_WORD(val);
	ISPAPB0_REG8(0x22a1) = HI_BYTE_OF_WORD(val);
	/* VwdOffset */
	//val = (pStrm->strmFrontVsize - winSize) / 2;
	val = ((vsize-4) - winSize) / 2;
	ISPAPB0_REG8(0x22a2) = LO_BYTE_OF_WORD(val);
	ISPAPB0_REG8(0x22a3) = HI_BYTE_OF_WORD(val);
	/* FullHSize */
	ISPAPB0_REG8(0x22a4) = LO_BYTE_OF_WORD(winSize);
	ISPAPB0_REG8(0x22a5) = HI_BYTE_OF_WORD(winSize);
	/* FullVSize */
	ISPAPB0_REG8(0x22a6) = LO_BYTE_OF_WORD(winSize);
	ISPAPB0_REG8(0x22a7) = HI_BYTE_OF_WORD(winSize);

	ISPAPB0_REG8(0x229a) = 0x11;
	ISPAPB0_REG8(0x229d) = 0x08;
	ISPAPB0_REG8(0x2299) = 0x01;
}

void aeInitVar(void)//call only once in main when plug in
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AeEvConst = AE_EV_CONST;	
	AeCurExpLine = 3;
	AeExpLineMode = 0;
}

void InstallVSinterrupt(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//please do by yourself
	//ex. install intrIntr0SensorVsync() function
}

void intrIntr0SensorVsync(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (((ISPAPB0_REG8(0x27C0) & INTR_VD_EDGE) == INTR_VD_EDGE) &&	/* sensor vd falling interrupt enable */
		((ISPAPB0_REG8(0x27B0) & INTR_VD_EDGE) == INTR_VD_EDGE)) {	/* sensor vd falling interrupt event */
		ISPAPB_LOGI("%s, V-sync Interrupt occur!\n", __FUNCTION__);

		ISPAPB0_REG8(0x27B0) = INTR_VD_EDGE;	/* clear vd falling edge interrupt AAF061 W1C*/

		aaaCount++;
		if (aaaCount >= AaaFreq)
		{
			aaaFlag  = 1;	/* set 3a flag*/
			aaaCount = 0;	/* clear count */
		}
	}
}

//initResetParam
void aaaLoadInit(char *aaa_init_file)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//please do fopen(aaa_init_file, "rt");
	//fread data into below variable
	/*
		AeIndexMax60;   
		AeIndexMax50;   
		AeIndexMin60;   
		AeIndexMin50;   
		AeIndex;         
		AeIndexPrev;      
		AeDeadZone; 
		AeYTarget;
		AwbBTargetMin;   
		AwbBTargetMax;   
		AwbRTargetMin;   
		AwbRTargetMax;   
		AwbGTargetMin;   
		AwbGTargetMax;   
		AwbRGainReg;        
		AwbGGainReg;        
		AwbBGainReg;		
		AaaFreq;
	*/

	AeIndexMax60  = (u8)aaa_init_file[0];
	AeIndexMax50  = (u8)aaa_init_file[1];
	AeIndexMin60  = (u8)aaa_init_file[2];
	AeIndexMin50  = (u8)aaa_init_file[3];
	AeIndex       = (short)(aaa_init_file[4]<<8)|aaa_init_file[5];
	AeIndexPrev   = (short)(aaa_init_file[6]<<8)|aaa_init_file[7];
	AeDeadZone    = (u8)aaa_init_file[8]; 
	AeYTarget     = (u16)(aaa_init_file[9]<<8)|aaa_init_file[10];
	AwbBTargetMin = (u8)aaa_init_file[11];
	AwbBTargetMax = (u8)aaa_init_file[12];
	AwbRTargetMin = (u8)aaa_init_file[13];
	AwbRTargetMax = (u8)aaa_init_file[14];
	AwbGTargetMin = (u8)aaa_init_file[15];
	AwbGTargetMax = (u8)aaa_init_file[16];
	AwbRGainReg   = (u16)(aaa_init_file[17]<<8)|aaa_init_file[18];
	AwbGGainReg   = (u16)(aaa_init_file[19]<<8)|aaa_init_file[20];
	AwbBGainReg   = (u16)(aaa_init_file[21]<<8)|aaa_init_file[22];
	AaaFreq       = (u8)aaa_init_file[23];
}

void aeLoadAETbl(char *ae_table_file)
{
	u16 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//please do fopen(ae_table_file, "rt");
	//fread into pAeExpTbl[256]

	for (i = 0; i < 256; i++)
	{
		pAeExpTbl[i] = (u16)(ae_table_file[i*2]<<8)|ae_table_file[(i*2)+1];
	}
}
 
void aeLoadGainTbl(char *gain_table_file)
{
	u16 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//please do fopen(gain_table_file, "rt");
	//fread into pAeGainTbl[256]

	for (i = 0; i < 256; i++)
	{
		pAeGainTbl[i] = (u16)(gain_table_file[i*2]<<8)|gain_table_file[(i*2)+1];
	}
}

void aeInitExt(void)
{
	u16 hsize, vsize;
	u16 hOffset, vOffset;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	aeInitVar();
	aaaFlag=0;
	aaaCount=0;
	hsize=(u16)(ISPAPB0_REG8(0x2725)<<8)|ISPAPB0_REG8(0x2724);
	vsize=(u16)(ISPAPB0_REG8(0x2727)<<8)|ISPAPB0_REG8(0x2726);	
	hsize-=4;
	vsize-=4;
	hOffset=0;
	vOffset=0;
	//
	AeImgSize=(u32)hsize*(u32)vsize;
	aeFuncExt();
	//	
	AeExpLineCount120 = 0;
	aeGetCurEv();
	AeDeadZone = 2;
	aeDeadLeafWinInit((hsize+4), (vsize+4), AeDeadLeafWinSize);
	AeDarkMode = 0;
	ISPAPB0_REG8(0x224C) = AeDarkThresh0;
	AeIndexPrev = AeIndex-1;
	
	vidctrlPuSaturation(PUSATURATIONCUR);
	aeSetWin_ROM(hsize, vsize, hOffset, vOffset);
	AeDeadZone = 2;
}

void awbSetGain(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	ISPAPB0_REG8(0x3104) = LO_BYTE_OF_WORD(AwbRGainReg);
	ISPAPB0_REG8(0x3105) = HI_BYTE_OF_WORD(AwbRGainReg);

	ISPAPB0_REG8(0x3106) = LO_BYTE_OF_WORD(AwbGGainReg);
	ISPAPB0_REG8(0x3107) = HI_BYTE_OF_WORD(AwbGGainReg);

	ISPAPB0_REG8(0x3108) = LO_BYTE_OF_WORD(AwbBGainReg);
	ISPAPB0_REG8(0x3109) = HI_BYTE_OF_WORD(AwbBGainReg);
}

void awbSetIndivWin(u16 hSize, u16 vSize, u16 hOffset, u16 vOffset)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* IndHaccFac */
	if (hSize < 8)			{ ISPAPB0_REG8(0x220E) = 0x00; }
	else if (hSize < 16) 	{ ISPAPB0_REG8(0x220E) = 0x01; }
	else if (hSize < 32) 	{ ISPAPB0_REG8(0x220E) = 0x02; }
	else if (hSize < 64) 	{ ISPAPB0_REG8(0x220E) = 0x03; }
	else if (hSize < 128) 	{ ISPAPB0_REG8(0x220E) = 0x04; }
	else if (hSize < 256) 	{ ISPAPB0_REG8(0x220E) = 0x05; }
	else if (hSize < 512) 	{ ISPAPB0_REG8(0x220E) = 0x06; }
	else if (hSize < 1024)	{ ISPAPB0_REG8(0x220E) = 0x07; }
	else if (hSize < 2048)	{ ISPAPB0_REG8(0x220E) = 0x08; }
	else 						{ ISPAPB0_REG8(0x220E) = 0x09; }
	/* IndVaccFac */
	if (vSize < 8)			{ ISPAPB0_REG8(0x220E) |= 0x00; }
	else if (vSize < 16) 	{ ISPAPB0_REG8(0x220E) |= 0x10; }
	else if (vSize < 32) 	{ ISPAPB0_REG8(0x220E) |= 0x20; }
	else if (vSize < 64) 	{ ISPAPB0_REG8(0x220E) |= 0x30; }
	else if (vSize < 128) 	{ ISPAPB0_REG8(0x220E) |= 0x40; }
	else if (vSize < 256) 	{ ISPAPB0_REG8(0x220E) |= 0x50; }
	else if (vSize < 512) 	{ ISPAPB0_REG8(0x220E) |= 0x60; }
	else if (vSize < 1024)	{ ISPAPB0_REG8(0x220E) |= 0x70; }
	else if (vSize < 2048)	{ ISPAPB0_REG8(0x220E) |= 0x80; }
	else						{ ISPAPB0_REG8(0x220E) |= 0x90; }

	/* HwdOffset */
	ISPAPB0_REG8(0x22E8) = LO_BYTE_OF_WORD(hOffset);
	ISPAPB0_REG8(0x22E9) = HI_BYTE_OF_WORD(hOffset);
	/* VwdOffset */
	ISPAPB0_REG8(0x22EA) = LO_BYTE_OF_WORD(vOffset);
	ISPAPB0_REG8(0x22EB) = HI_BYTE_OF_WORD(vOffset);

	/* Individual window size */
	/* IndivHSize */
	ISPAPB0_REG8(0x22EC) = LO_BYTE_OF_WORD(hSize);
	ISPAPB0_REG8(0x22ED) = HI_BYTE_OF_WORD(hSize);
	/* IndivVSize */
	ISPAPB0_REG8(0x22EE) = LO_BYTE_OF_WORD(vSize);
	ISPAPB0_REG8(0x22EF) = HI_BYTE_OF_WORD(vSize);
}


void awbInitVar(void)
{ 
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AwbRWidth = AWB_R_WIDTH;
	AwbBWidth = AWB_B_WIDTH;
	AwbMinGrayDetCnt = AWB_MIN_GRAY_DET_CNT;

	AwbCheckBlkCnt = AWB_CHECK_BLK_CNT;
	pAwbRRef = AwbRRef;
	pAwbBRef = AwbBRef;

	AwbCmAdjCnt = AWB_CM_ADJ_CNT;
	pAwbAdjCmColTemp = AwbAdjCmColTemp; 
	pAwbAdjCmColMat = (short *)AwbAdjCmColMat;

	AwbLensAdjCnt = AWB_LENS_ADJ_CNT;
	pAwbAdjLensColTemp = AwbAdjLensColTemp;
	pAwbAdjLensRVal = AwbAdjLensRVal;
	pAwbAdjLensBVal = AwbAdjLensBVal;

	AwbFirstInit = 1;
	AwbMode = 0;
}

void awbAdjLensComp(u8 init)
{
	//N/A
}

void awbAdjColMat(void)
{
	short r1, r2, r3;
	u8 i, j;
	short cm[10];
	u16 tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	tmp = AwbCurColTemp;

	if (tmp < pAwbAdjCmColTemp[0])					tmp = pAwbAdjCmColTemp[0];
	if (tmp > pAwbAdjCmColTemp[AwbCmAdjCnt - 1])	tmp = pAwbAdjCmColTemp[AwbCmAdjCnt - 1];

	for (j = 0; j <= AwbCmAdjCnt - 2; j++)
	{
	 	if ((tmp >= pAwbAdjCmColTemp[j]) && (tmp <= pAwbAdjCmColTemp[j+1]))
			break;
	}

	r1 = pAwbAdjCmColTemp[j+1] - tmp;
	r2 = tmp - pAwbAdjCmColTemp[j];
	r3 = pAwbAdjCmColTemp[j+1] - pAwbAdjCmColTemp[j];

	for (i = 0; i < 9; i++)
		cm[i] = (r1 * (short)pAwbAdjCmColMat[j * 9 + i] + r2 * (short)pAwbAdjCmColMat[(j+1) * 9 + i]) / r3;

	cm[0] = 0x40 - (cm[1] + cm[2]);
	cm[4] = 0x40 - (cm[3] + cm[5]);
	cm[8] = 0x40 - (cm[6] + cm[7]);

	//#ifdef ROM_48K
	//if (FunctionTable.awbAdjColMatExt)
	//{
	//	FunctionTable.awbAdjColMatExt(cm);
	//}
	//#endif
	for (i = 0; i < 9; i++)
	{
		ISPAPB0_REG8((unsigned long)0x2148 + i * 2) = (u8)cm[i];

		if (cm[i] < 0)	ISPAPB0_REG8((unsigned long)0x2148 + i * 2 + 1) = 1;
		else			ISPAPB0_REG8((unsigned long)0x2148 + i * 2 + 1) = 0;
	}
}

void awbInit(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	awbInitVar();
	AwbMinGain = 0x42;
	ISPAPB0_REG8(0x2217) = 0x31;	/* new awb */
	ISPAPB0_REG8(0x3100) = 0x71;	/* new awb gain */
	ISPAPB0_REG8(0x2218) = 0xf8;	/* hi threshold */
	ISPAPB0_REG8(0x221B) = 0x05;	/* lo threshold */

	ISPAPB0_REG8(0x2219) = 64;		/* gr shift */
	ISPAPB0_REG8(0x221A) = 64;		/* gb shift */

	ISPAPB0_REG8(0x2213) = 0xf8;	/* lum high threshold in Sec-1 */
	ISPAPB0_REG8(0x2214) = 0xf8;	/* lum high threshold in Sec-2 */
	ISPAPB0_REG8(0x2215) = 0xf8;	/* lum high threshold in Sec-3 */
	ISPAPB0_REG8(0x2216) = 0xf8;	/* lum high threshold in Sec-4 */
	ISPAPB0_REG8(0x3221) = 0xf8;	/* lum high threshold in Sec-5 */

	ISPAPB0_REG8(0x221C) = 0x05;	/* lum low threshold in Sec-1 */
	ISPAPB0_REG8(0x221D) = 0x05;	/* lum low threshold in Sec-2 */
	ISPAPB0_REG8(0x221E) = 0x05;	/* lum low threshold in Sec-3 */
	ISPAPB0_REG8(0x221F) = 0x05;	/* lum low threshold in Sec-4 */
	ISPAPB0_REG8(0x3220) = 0x05;	/* lum low threshold in Sec-5 */

	ISPAPB0_REG8(0x2131) = 0x00;	/* r gain high byte */ 
	ISPAPB0_REG8(0x2139) = 0x00;	/* b gain high byte */
	ISPAPB0_REG8(0x2135) = 0x00;	/* gr gain high byte */
	ISPAPB0_REG8(0x213D) = 0x00;	/* gb gain high byte */
	ISPAPB0_REG8(0x2130) = ISPAPB0_REG8(0x2138) = ISPAPB0_REG8(0x2134) = ISPAPB0_REG8(0x213C) = 64; /* r / b / gr / gb gain low byte */
	ISPAPB0_REG8(0x2210) = 0; //AWB full,Histogram full

	awbSetIndivWin(80,80, 80, 80);
	awbSetGain();

	if (AwbFirstInit)
	{	
		AwbColTemp = AwbCurColTemp = (u16)AwbBGainReg * 64 / AwbRGainReg;
		AwbRGainTarget = AwbRawRGain = AwbRGainReg;
		AwbBGainTarget = AwbRawBGain = AwbBGainReg;
		AwbGGainTarget = 0x40;
	}
	else
	{
		AwbColTemp = AwbCurColTemp = (u16)AwbRawBGain * 64 / AwbRawRGain;
	}

	awbAdjLensComp(1);
	awbAdjColMat();

	AwbMode = 0;
	AwbDeadZone = 4;
	AwbFirstInit = 0;
}

void aeAdjYTarget_RAM(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AeAdjYTarget = (short)AeYTarget + ((short)puBacklightCompensationCur - (short)puBacklightCompensationDef)*16;	
	
	if(AeIndex > 170) AeAdjYTarget -= 20;
	else if(AeIndex > 130) AeAdjYTarget -= 20*(AeIndex - 130)/(170-130);	
	if (AeAdjYTarget < 2)			AeAdjYTarget = 2; 

}

short aeGetYLayer(u8 *pWeight)
{
	u32 winSize;
	u32 normWinSize;
	u32 ySum;
	u8 i;	
	//u16 weightSum;
	u16 tempY;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* Calculate yLayer */
	/* individual window size */
	winSize = ((u32)((u16)(ISPAPB0_REG8(0x320d) & 0x01)) << 8 | ISPAPB0_REG8(0x320c)) *
				((u32)((u16)(ISPAPB0_REG8(0x320f) & 0x01)) << 8 | ISPAPB0_REG8(0x320e));

	/* normalized individual window size */
	normWinSize = ((u32)(8 << ( ISPAPB0_REG8(0x3207) & 0x07))) *			
								((u32)(8 << ((ISPAPB0_REG8(0x3207) & 0x70) >> 4)));

	ISPAPB0_REG8(0x325d) = 0;
	/* read ae window 1 ~ 25 */	
	//weightSum = ySum = 0;
	ySum = 0;
	
	for (i = 0; i < 81; i++)
	{
		tempY = (u16)ISPAPB0_REG8(0x325e) * normWinSize / winSize;
		ySum += (u16)tempY;
   }

	AeYLayer  = (ySum / 81);
	
	return (AeYLayer);
}

u16 aeAdjIndexRatio(u16 adjYTarget, short yLayer)
{
	u16 adjRatio;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (adjYTarget > yLayer * 12)
		adjRatio = 300;	
	else if(adjYTarget < yLayer / 12)
		adjRatio = 50;
	else
	{
		adjRatio = (u32)adjYTarget * 100 / yLayer;
		if (adjRatio > 100)	adjRatio = 100 + (adjRatio - 100) * 4 / 10;
		else				adjRatio = 100- (100 - adjRatio) * 4 / 10;
		AeDeadZone = 4;	
	}
	
	return(adjRatio);
}

void aeAdjIndex(void)
{
	u32 ratio;
	u32 desireEv, deltaEv, minDEv,PreEV;
	short i, desireAeIdx;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	ratio = aeAdjIndexRatio(AeAdjYTarget, AeYLayer);
	AeTest = 0;

	if (AeExpLineMode == 1 || (AeIndex == 0 && AeYLayer > AeAdjYTarget))
	{
		AeTest = 1;
		AeExpLineMode = 1;
		
		AeExp = (u16)(AeCurExpLine * ratio / 100);
			
		//aePostAdjIndex_RAM(&AeExp);
		
		if(AeExp < 4) AeExp = 4;

		if ((AeExpLineCount120 != 0) && (AeExp > AeExpLineCount120)) 
			AeExpLineMode = 0;

		AeIndexGet = AeIndex = 0;
		//EA = 0;	/* disable all interrupts */
		AeLineModeExpLineCount = AeExp;
		aeGetGainIsr();
		AeGain = AeGainGet;
	}
	else
	{
		//DELTA_EV d;
		
		AeExpLineMode = FALSE;
		desireEv = AeCurEv * ratio / 100;
				
		minDEv = aeGetDeltaEv(desireEv, AeIndex);	
		
		PreEV = AeCurEv;
		desireAeIdx = AeIndex;

		if (AeAdjYTarget > AeYLayer)
		{
			AeTest = 2; 
			for (i = AeIndex+1; i <= AeIndexMax; i++)
			{				
				deltaEv = aeGetDeltaEv(desireEv, i);
				

				if ((deltaEv > minDEv)&&(AeCurEv > PreEV))
					break;
				else if (deltaEv <= minDEv)
				{
					minDEv = deltaEv;
					desireAeIdx = i;
				}
				PreEV = AeCurEv;
			}
		}
		else
		{
			AeTest = 3;
			for (i = AeIndex-1; i >= 0; i--)
			{
				deltaEv = aeGetDeltaEv(desireEv, i);

				if ((deltaEv > minDEv)&&(AeCurEv < PreEV))
					break;
				else if (deltaEv <= minDEv)
				{
					minDEv = deltaEv;
					desireAeIdx = i;
				}
				PreEV = AeCurEv;
			}
		}

		//aePostAdjIndex_RAM((u16 *)&desireAeIdx);

		//EA = 0;	/* disable all interrupts */
		AeIndex = AeIndexGet = desireAeIdx;
		//uartPutString_ROM("AeIndex=");
		//uartPutNumber_ROM(AeIndex);
		
		aeGetGainIsr();
		AeGain = AeGainGet;
	}

	if(AeExpLineMode == FALSE)
	{
		aeGetExposureIsr();
		AeExp = AeExpGet;
	}
	//EA = 1;	/* Enable all interrupts */

	aeGetCurEv();
}

void aeFunc(u8 aeOn)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (aeOn)
	{
	
		if (AeExpLineCount120 == 0)
		{
			//EA = 0;	/* disable all interrupts */
			SensorExpIn = 0x780;
			sensorExpToLineCountIsr();
			AeExpLineCount120 = SensorLineCountOut;
			//EA = 1;	/* Enable all interrupts */
		}
		aeAdjYTarget_RAM();
		aeGetYLayer(pAeNormWeight);
		aaaFlag = 0;	//to avoid AE call continuoustly if no exposure/gain changed
	
		if ((AeYLayer < (AeAdjYTarget - AeDeadZone)) || (AeYLayer > (AeAdjYTarget + AeDeadZone)))
		{
			AeDeadZone = 4;			
			aeAdjIndex();

		}

		/* Set exposure and gain by auto-exposure index */
		if ((AeIndex != AeIndexPrev)  ||
			((AeExpLineMode == 1) && (AeCurExpLine != AeLineModeExpLineCount)) ||
			(AePreExpLineMode != AeExpLineMode))
		{			
			aaaFlag = 0;
			aaaCount = 0;
			AePreExpLineMode = AeExpLineMode;
						
			//EA = 0;	/* disable all interrupts */
			AeStill = 0;

			sensorSetExposureTimeIsr();
			sensorSetGainIsr();
			//EA = 1;	/* enable all interrupts */

			AeIndexPrev = AeIndex;
			AeStable = 0;
		}
		else
		{
			AeStable++;
		}	
		aeFuncExt();	
	}
}

u32 mathlibUcharToUlong(u8 x, u8 y, u8 u, u8 v)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	return ((unsigned long) (((unsigned long)x)<<24)|(((unsigned long)y)<<16)|(((unsigned long)u)<<8)|((unsigned long)(v)));
}

u32 awbCount(u8 id)
{
	u8 offset;
	u32 count;
	//u8 i;   // SunplusIT: added for debug test

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (id < 4)
	{
		offset = id * 12;
		//count = u8_TO_u32(0x00, ISPAPB0_REG8(0x2262 + offset], ISPAPB0_REG8(0x2261 + offset], ISPAPB0_REG8(0x2260 + offset]);
		count = mathlibUcharToUlong(0x00, ISPAPB0_REG8((unsigned long)0x2262 + offset), ISPAPB0_REG8((unsigned long)0x2261 + offset), ISPAPB0_REG8((unsigned long)0x2260 + offset));

	}
	else // if (id == 4)
	{
		//count = u8_TO_u32(0x00, ISPAPB0_REG8(0x3232], ISPAPB0_REG8(0x3231], ISPAPB0_REG8(0x3230]);
		count = mathlibUcharToUlong(0x00, ISPAPB0_REG8(0x3232), ISPAPB0_REG8(0x3231), ISPAPB0_REG8(0x3230));
	}

	/* SunplusIT: added for debug test
	if((ISPAPB0_REG8(0x0190) & 0x10) == 0x10)
	{
		i = ISPAPB0_REG8(0x2BCD);
		ISPAPB0_REG8((unsigned long)0x3b00 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b01 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b02 + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b03 + i * 4) = count;
		ISPAPB0_REG8(0x2BCD) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x20) == 0x20)
	{
		i = ISPAPB0_REG8(0x2BCE);
		ISPAPB0_REG8((unsigned long)0x3b14 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b15 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b16 + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b17 + i * 4) = count;
		ISPAPB0_REG8(0x2BCE) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x40) == 0x40)
	{
		i = ISPAPB0_REG8(0x2BCF);
		ISPAPB0_REG8((unsigned long)0x3b28 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b29 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b2A + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b2B + i * 4) = count;
		ISPAPB0_REG8(0x2BCF) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x80) == 0x80)
	{
		i = ISPAPB0_REG8(0x28CD);
		ISPAPB0_REG8((unsigned long)0x3b3C + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b3D + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b3E + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b3F + i * 4) = count;
		ISPAPB0_REG8(0x28CD) += 1;
	}
	*/
	return(count);
}

u32 awbColSum(u8 col, u8 id)
{
	u8 offset;
	u32 count;
	//u8 i;   // SunplusIT: added for debug test

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (col == 1)
	{
		if (id < 4)
		{
			offset = id * 4;
			//count = u8_TO_u32(ISPAPB0_REG8(0x3213 + offset), ISPAPB0_REG8(0x3212 + offset], ISPAPB0_REG8(0x3211 + offset], ISPAPB0_REG8(0x3210 + offset]);
			count = mathlibUcharToUlong(ISPAPB0_REG8((unsigned long)0x3213 + offset), ISPAPB0_REG8((unsigned long)0x3212 + offset), ISPAPB0_REG8((unsigned long)0x3211 + offset), ISPAPB0_REG8((unsigned long)0x3210 + offset));
		}
		else
		{
			//count = u8_TO_u32(ISPAPB0_REG8(0x323f), ISPAPB0_REG8(0x323e), ISPAPB0_REG8(0x323d), ISPAPB0_REG8(0x323c));
			count = mathlibUcharToUlong(ISPAPB0_REG8(0x323f), ISPAPB0_REG8(0x323e), ISPAPB0_REG8(0x323d), ISPAPB0_REG8(0x323c));
		}
	}
	else
	{
		if (col == 2)	offset = 4;
		else			offset = 0;

		if (id < 4)
		{
		   offset += (id * 12);
		   //count = u8_TO_u32(ISPAPB0_REG8((unsigned long)0x2267 + offset), ISPAPB0_REG8((unsigned long)0x2266 + offset), ISPAPB0_REG8((unsigned long)0x2265 + offset), ISPAPB0_REG8((unsigned long)0x2264 + offset));
			 count = mathlibUcharToUlong(ISPAPB0_REG8((unsigned long)0x2267 + offset), ISPAPB0_REG8((unsigned long)0x2266 + offset), ISPAPB0_REG8((unsigned long)0x2265 + offset), ISPAPB0_REG8((unsigned long)0x2264 + offset));
		}
		else // if (id == 4)
		{
		   //count = u8_TO_u32(ISPAPB0_REG8((unsigned long)0x3237 + offset), ISPAPB0_REG8((unsigned long)0x3236 + offset), ISPAPB0_REG8((unsigned long)0x3235 + offset), ISPAPB0_REG8((unsigned long)0x3234 + offset));
			 count = mathlibUcharToUlong(ISPAPB0_REG8((unsigned long)0x3237 + offset), ISPAPB0_REG8((unsigned long)0x3236 + offset), ISPAPB0_REG8((unsigned long)0x3235 + offset), ISPAPB0_REG8((unsigned long)0x3234 + offset));
		}
	}

	/* SunplusIT: added for debug test
	if((ISPAPB0_REG8(0x0190) & 0x10) == 0x10)
	{
		i = ISPAPB0_REG8(0x2BCD);
		ISPAPB0_REG8((unsigned long)0x3b00 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b01 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b02 + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b03 + i * 4) = count;
		ISPAPB0_REG8(0x2BCD) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x20) == 0x20)
	{
		i = ISPAPB0_REG8(0x2BCE);
		ISPAPB0_REG8((unsigned long)0x3b14 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b15 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b16 + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b17 + i * 4) = count;
		ISPAPB0_REG8(0x2BCE) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x40) == 0x40)
	{
		i = ISPAPB0_REG8(0x2BCF);
		ISPAPB0_REG8((unsigned long)0x3b28 + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b29 + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b2A + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b2B + i * 4) = count;
		ISPAPB0_REG8(0x2BCF) += 1;
	}
	else if((ISPAPB0_REG8(0x0190) & 0x80) == 0x80)
	{
		i = ISPAPB0_REG8(0x28CD);
		ISPAPB0_REG8((unsigned long)0x3b3C + i * 4) = count >> 24;
		ISPAPB0_REG8((unsigned long)0x3b3D + i * 4) = count >> 16;
		ISPAPB0_REG8((unsigned long)0x3b3E + i * 4) = count >> 7;
		ISPAPB0_REG8((unsigned long)0x3b3F + i * 4) = count;
		ISPAPB0_REG8(0x28CD) += 1;
	}
	*/
	return(count);
}

void awbSetCenter(u8 id, u8 centerR, u8 centerB, u8 widthR, u8 widthB)
{
	u8 offset;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (id < 4)
	{
	   offset = id << 2;
	   if (centerR < widthR)		ISPAPB0_REG8((unsigned long)0x2220 + offset) = 0;
	   else							ISPAPB0_REG8((unsigned long)0x2220 + offset) = centerR - widthR;
	   ISPAPB0_REG8((unsigned long)0x2221 + offset) = centerR + widthR;
	   ISPAPB0_REG8((unsigned long)0x2222 + offset) = centerB - widthB;
	   if (centerB > 255 - widthB)	ISPAPB0_REG8((unsigned long)0x2223 + offset) = 255;
	   else							ISPAPB0_REG8((unsigned long)0x2223 + offset) = centerB + widthB;
	}
	else //if(id == 4)
	{
	   ISPAPB0_REG8(0x3222) = centerR - widthR;
	   ISPAPB0_REG8(0x3223) = centerR + widthR;
	   ISPAPB0_REG8(0x3224) = centerB - widthB;
	   ISPAPB0_REG8(0x3225) = centerB + widthB;
	}
}

u16 awbAdjGain(u16 AwbGainReg, u16 AwbGainTarget)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (AwbGainTarget > (AwbGainReg + 8))
		AwbGainReg += 4;
	else if(AwbGainTarget > (AwbGainReg + 4))
		AwbGainReg += 2;
	else if(AwbGainTarget > AwbGainReg)
		AwbGainReg += 1;
	else if ((AwbGainTarget + 8) < AwbGainReg)
		AwbGainReg -= 4;
	else if ((AwbGainTarget + 4) < AwbGainReg)
		AwbGainReg -= 2;
	else if (AwbGainTarget < AwbGainReg)
		AwbGainReg -= 1;
	
	return(AwbGainReg);
}
/*
void awbSetGain(void)
{
	ISPAPB0_REG8(0x3104) = LO_BYTE_OF_WORD(AwbRGainReg);
	ISPAPB0_REG8(0x3105) = HI_BYTE_OF_WORD(AwbRGainReg);

	ISPAPB0_REG8(0x3106) = LO_BYTE_OF_WORD(AwbGGainReg);
	ISPAPB0_REG8(0x3107) = HI_BYTE_OF_WORD(AwbGGainReg);

	ISPAPB0_REG8(0x3108) = LO_BYTE_OF_WORD(AwbBGainReg);
	ISPAPB0_REG8(0x3109) = HI_BYTE_OF_WORD(AwbBGainReg);
}
*/
void awbFuncExt(void)
{           
	//u8 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AwbRGainReg = awbAdjGain(AwbRGainReg, AwbRGainTarget+2);
	AwbGGainReg = awbAdjGain(AwbGGainReg, AwbGGainTarget);
	AwbBGainReg = awbAdjGain(AwbBGainReg, AwbBGainTarget+2);

	awbSetGain();
	
	if (AwbColTemp > AwbCurColTemp + 10)		AwbCurColTemp = AwbCurColTemp + 5;
	else if (AwbColTemp > AwbCurColTemp)		AwbCurColTemp++;
	else if (AwbColTemp < AwbCurColTemp - 10)	AwbCurColTemp =  AwbCurColTemp - 5;
	else if (AwbColTemp < AwbCurColTemp)		AwbCurColTemp--;

	//awbAdjLensComp(0);
	awbAdjColMat();
}

void awbFunc(u8 awbOn)
{
	u16 i;
	u16 sR,sG,sB;
	u16 tmp;
	u16 minG;
	//u8 AwbCenterR,AwbCenterB;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (awbOn)
	{		
		if(AwbMode == 3)
		{
			for(i=10; i<AwbCheckBlkCnt; i++)
			{
					AwbSumCount = AwbSumCount + awbCount(i-10);
					AwbSumR = AwbSumR + awbColSum(0, i-10);
					AwbSumG = AwbSumG + awbColSum(1, i-10);
					AwbSumB = AwbSumB + awbColSum(2, i-10);
			}

			if (AwbSumCount > 0x80)
			{				
				sR = (u16)(AwbSumR / AwbSumCount);
				sG = (u16)(AwbSumG / AwbSumCount);
				sB = (u16)(AwbSumB / AwbSumCount);

				AwbRawRGain = (u16)((0x40 * sG + sR / 2) / sR);
				AwbRawBGain = (u16)((0x40 * sG + sB / 2) / sB);		 

				AwbRGainTarget = AwbRawRGain;
				AwbBGainTarget = AwbRawBGain;			

				tmp = (u16)AwbBGainTarget * 64 / AwbRGainTarget;

				if ((tmp > AwbColTemp + 4) || (tmp < AwbColTemp - 4))
					AwbColTemp = tmp;

				if (AwbRGainTarget < AwbRTargetMin)	AwbRGainTarget = AwbRTargetMin;
				if (AwbRGainTarget > AwbRTargetMax)	AwbRGainTarget = AwbRTargetMax;
				if (AwbBGainTarget < AwbBTargetMin)	AwbBGainTarget = AwbBTargetMin;
				if (AwbBGainTarget > AwbBTargetMax)	AwbBGainTarget = AwbBTargetMax;


				if(AeIndex < AeIndexMax - 30) 
					AwbDeadZone = 2;
				else
					AwbDeadZone = 2+4*(AeIndex + 30 - AeIndexMax)/30;
					
				if ((AwbPreRTarget - AwbDeadZone <= AwbRGainTarget) && (AwbPreRTarget + AwbDeadZone >= AwbRGainTarget) &&
					(AwbPreBTarget - AwbDeadZone <= AwbBGainTarget) && (AwbPreBTarget + AwbDeadZone >= AwbBGainTarget))
				{
					AwbRGainTarget = AwbPreRTarget;
					AwbBGainTarget = AwbPreBTarget;
				}	 
				else
				{
					AwbPreRTarget = AwbRGainTarget;
					AwbPreBTarget = AwbBGainTarget;
				}
				
				AwbGGainTarget = minG =0x40;
				if (AwbRGainTarget < minG)	minG = AwbRGainTarget;
				if (AwbBGainTarget < minG)	minG = AwbBGainTarget;
				
				AwbRGainTarget = ((u16)AwbRGainTarget * 0x40) / minG ;
				AwbGGainTarget = ((u16)AwbGGainTarget * 0x40) / minG ;
				AwbBGainTarget = ((u16)AwbBGainTarget * 0x40) / minG ;
								
			}

			AwbMode = 0;
		}

		if(AwbMode == 0)
		{
			AwbSumCount = AwbSumR = AwbSumG = AwbSumB = 0;
		}
		else
		{
			for(i=0; i<5; i++)
			{
				AwbSumCount = AwbSumCount + awbCount(i);
				AwbSumR = AwbSumR + awbColSum(0, i);
				AwbSumG = AwbSumG + awbColSum(1, i);
				AwbSumB = AwbSumB + awbColSum(2, i);
			}
		}

		tmp = (AwbMode % 5) * 5;		
		for (i = tmp; (i < (tmp + 5)) && (i < AwbCheckBlkCnt); i++)
		{
				awbSetCenter(i - tmp, pAwbRRef[i], pAwbBRef[i], AwbRWidth, AwbBWidth);
		}

		AwbMode++;	

		awbFuncExt();
	}
}

/*
	aaaAeAwbAf should be called in main loop.
*/
void aaaAeAwbAf(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if(isVideoStart()==1)
	{
		if (aaaFlag==1)  //will be enable at intrIntr0SensorVsync()
		{
			aaaFlag = 0;
			aeFunc(1);
			awbFunc(1);
		}
	}
}


