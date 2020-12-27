#include <linux/io.h>
#include "isp_api.h"
#include "i2c_api.h"
#include "3a.h"
#include "3a_types.h"

#define INTR_VD_EDGE                    (0x02)
#define MANUALEXPGAIN                   (0xa0)
#define PUBRIGHTNESSCUR                 (0x0080)
#define PUCONTRASTCUR                   (0x0020)
#define PUGAINCUR                       (0x0020)
#define PUBACKLIGHTCOMPENSATIONCUR      (0x0001)
#define PUBACKLIGHTCOMPENSATIONDEF      (0x0001)
#define PUHUECUR                        (0x0000)
#define PUSATURATIONCUR                 (0x0040)
#define PUSHARPNESSCUR                  (0x0002)
#define PUGAMMACUR                      (0x005a)
#define PUWHITEBALANCETEMPERATURECUR    (0x0fa0)
#define PUPOWERLINEFREQUENCYCUR         (0x0002) //1:50HZ, 2:60Hz
#define AEINDEX                         (0x0074)
#define AEINDEXPREV                     (0x0073)
#define AEINDEXMAX60                    (0x00a7)
#define AEINDEXMAX50                    (0x00a7)
#define AEINDEXMIN60                    (0x0000)
#define AEINDEXMIN50                    (0x0000)
#define AEDEADZONE                      (0x0004)
#define AEYTARGET                       (0x0030)
#define AWBBTARGETMIN                   (0x005c)
#define AWBBTARGETMAX                   (0x00ba)
#define AWBRTARGETMIN                   (0x0046)
#define AWBRTARGETMAX                   (0x0080)
#define AWBGTARGETMIN                   (0x0040)
#define AWBGTARGETMAX                   (0x0040)
#define AWBRGAINREG                     (0x0070)
#define AWBGGAINREG                     (0x0040)
#define AWBBGAINREG                     (0x006e)
#define AAAFREQ                         (0x0003)
#define CTEXPOSURETIMEABSOLUTECUR       (0x0000009C)
#define CTEXPOSURETIMEABSOLUTEMIN       (0x00000004)
#define CTEXPOSURETIMEABSOLUTEMAX       (0x000004e2)
#define AWB_R_WIDTH                     6
#define AWB_B_WIDTH                     8
#define AWB_MIN_GRAY_DET_CNT            0x3000 
#define AWB_CHECK_BLK_CNT               13
#define	AWB_CM_ADJ_CNT                  3

const u8 AwbRRef[AWB_CHECK_BLK_CNT] = { 62,  65,  68,  73,  78, 83, 88, 92, 96, 100, /*CW 10~12*/  86, 91, 95};
const u8 AwbBRef[AWB_CHECK_BLK_CNT] = {128, 120, 114, 108, 103, 97, 92, 86, 80,  75, /*CW 10~12*/ 103, 97, 92};
const u16 AwbAdjCmColTemp[AWB_CM_ADJ_CNT] = {42, 64, 86};
const short AwbAdjCmColMat[AWB_CM_ADJ_CNT][9] = {
	{0x4c, -(0x100-0xf6), -(0x100-0xfe), -(0x100-0xf3), 0x53, -(0x100-0xfa),           0x3, -(0x100-0xeb), 0x52},  //D
	{0x4c, -(0x100-0xf8), -(0x100-0xfc), -(0x100-0xf8), 0x4c, -(0x100-0xfc),           0x9, -(0x100-0xf0), 0x47},  //Office
	{0x50, -(0x100-0xf8), -(0x100-0xf8), -(0x100-0xf4), 0x50, -(0x100-0xfc), -(0x100-0xf9), -(0x100-0xf8), 0x4f},  //86
};

#define	AWB_LENS_ADJ_CNT    3
const short AwbAdjLensColTemp[AWB_LENS_ADJ_CNT] = {122, 136, 180};
const short AwbAdjLensRVal[AWB_LENS_ADJ_CNT]    = {100, 113, 138};
const short AwbAdjLensBVal[AWB_LENS_ADJ_CNT]    = {100, 100, 100};

#define	AE_EV_CONST         500

const short Sign_CosSin[8] = {1, 1, -1, 1, -1, -1, 1, -1};
const u8 Sin0_90[91] = {
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

extern int isVideoStart(void);  // in isp_api.c


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

void vidctrlPuBrightness(struct mipi_isp_info *isp_info, short puBrightnessCur)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	short tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	tmp = puBrightnessCur - 0x80;

	if(tmp < -128)
		tmp = -128;
	else
		if(tmp > 127) tmp = 127;

	writeb(tmp, &(regs->reg[0x21C1]));
}

void vidctrlPuContrast(struct mipi_isp_info *isp_info, short puContrastCur)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	short wContrastAdj;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	wContrastAdj = puContrastCur;

	if(1)
	{
		writeb(0x23, &(regs->reg[0x21c0]));

		if (wContrastAdj >= 4)
			wContrastAdj -= 4;
		else
			wContrastAdj = 0;
	}
	//else
	//{
	//	writeb( , &(regs->reg[0x21c0) = 0x03;
	//}

	if (wContrastAdj > 255)
		wContrastAdj = 255;
	else if(wContrastAdj < 0)
		wContrastAdj = 0;

	writeb(wContrastAdj, &(regs->reg[0x21C2]));
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

void vidctrlPuHue(struct mipi_isp_info *isp_info, short puHueCur)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
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

	writeb(temp1, &(regs->reg[0x21C3]));
	writeb(temp2, &(regs->reg[0x21C4]));
}

void vidctrlPuSaturation(struct mipi_isp_info *isp_info, short puSaturationCur)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	short sat;
	//char AdjByAe;
	short DefVal;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	sat = puSaturationCur;

	if(readb(&(regs->reg[0x21c0])) == 0x23) DefVal = 0x2a; //0x31;	//x28;//2c	//yuv	//jim:27
	else DefVal = 0x30;

	if(AaaVar->AeIndex >= 160)
		DefVal = DefVal - 5;
	else if(AaaVar->AeIndex >= 140)
		DefVal = DefVal - 5*(AaaVar->AeIndex-140)/(160-140);

	//80lux E27 Warm AwbCurColTemp = 164	aeindex = 185(0xb9)
	//20lux 3000k    AwbCurColTemp = 135 	aeindex = 210(0xd0)
	//80lux 3000k    AwbCurColTemp = 145    aeindex = 186(0xba)
	
	if(AaaVar->AwbCurColTemp < 100) 
		DefVal = DefVal+0;
	else if(AaaVar->AwbCurColTemp > 150) 
		DefVal = DefVal+16;
	else 
		DefVal = DefVal+0+(16-0)*(AaaVar->AwbCurColTemp-100)/(150-100);

	sat = (sat*DefVal)/64;

	if(sat > 255)
		sat = 255;
	if(sat < 0)
		sat = 0;

	writeb(sat, &(regs->reg[0x21C5]));
}

void vidctrlPuSharpness(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//N/A
}

void vidctrlPuGamma(struct mipi_isp_info *isp_info, u16 puGammaCur)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 i;
	u8 refGamma[15] = {  1,  4,  9, 16,  25,  36,  49,  64,  81,  98, 124, 146, 170, 196, 225 }; // Gamma 2 for wGamma = 150
	u8 defGamma[15] = { 29, 51, 70, 89, 104, 120, 134, 147, 159, 174, 187, 200, 213, 226, 240 }; // Gamma 1 for wGamma = 120
	short gamma;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//if(AeDarkMode == 0)
	{
		for (i = 0; i < 15; i++)
		{
		gamma = (((short)puGammaCur - 120) * ((short)defGamma[i] - (short)refGamma[i])) / 40 + (short)defGamma[i];
		if (gamma > 0xFF)
			gamma = 0xFF;
		else if (gamma < 0)
			gamma = 0;

		writeb((u8)gamma, &(regs->reg[0x2161+i]));
		}

	}
	//else
	//{
	//	for(i=0; i<12; i++) XBYTE[0x2161+i] = 0;
	//	XBYTE[0x2161+12] = 50;
	//	XBYTE[0x2161+13] = 100;
	//	XBYTE[0x2161+14] = 200;
	//}
}

void aeGetGainIsr(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	//u16 sfAddr;
	u16* sfAddr;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//sfAddr = AeGainTableSfAddr + (AeIndexGet << 1);
	//AeGainGet=pAeGainTbl[sfAddr];
	sfAddr = AaaVar->AeGainTableSfAddr + AaaVar->AeIndexGet;
	AaaVar->AeGainGet =* sfAddr;

	//SfSpi.sfAddr = (u8 *)&sfAddr;
	//SfSpi.sfData = (u8 *)&AeGainGet;
	//SfSpi.sfCount = 2;
	//sfReadBytesIsr_ROM();
}

void aeGetExposureIsr(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	//u16 sfAddr;
	u16* sfAddr;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//sfAddr = AeExpTableSfAddr + (AeIndexGet << 1);
	//AeExpGet=pAeExpTbl[sfAddr];
	sfAddr = AaaVar->AeExpTableSfAddr + AaaVar->AeIndexGet;

	ISP3A_LOGD("sfAddr=0x%p, AeExpTableSfAddr=0x%p, AeIndexGet=%d\n", 
				sfAddr, AaaVar->AeExpTableSfAddr, AaaVar->AeIndexGet);

	AaaVar->AeExpGet=*sfAddr;

	//SfSpi.sfAddr = (u8 *)&sfAddr;
	//SfSpi.sfData = (u8 *)&AeExpGet;
	//SfSpi.sfCount = 2;
	//sfReadBytesIsr_ROM();

	ISP3A_LOGD("%s end\n", __FUNCTION__);
}

u32 aeGetDeltaEv(struct mipi_isp_info *isp_info, u32 desireEv, short idx)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u32 deltaEv;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//EA = 0;	/* disable all interrupts */

	AaaVar->AeIndexGet = AaaVar->AeIndex = idx;
	aeGetExposureIsr(isp_info);
	aeGetGainIsr(isp_info);

	//EA = 1;	/* Enable all interrupts */

	AaaVar->AeExp = AaaVar->AeExpGet;
	AaaVar->AeGain = AaaVar->AeGainGet;
	aeGetCurEv(isp_info);

	if (desireEv > AaaVar->AeCurEv)
		deltaEv = desireEv - AaaVar->AeCurEv;
	else
		deltaEv = AaaVar->AeCurEv - desireEv;

	return(deltaEv);
}

void vidctrlPuBacklightCompensation(struct mipi_isp_info *isp_info, short puPowerLineFrequencyCur)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (isVideoStart() == 1)
	{
		if ((puPowerLineFrequencyCur == 2))
		{	
			AaaVar->AeExpTableSfAddr = AaaVar->pAeExpTbl;//SF_AE_EXP_60;
			AaaVar->AeGainTableSfAddr = AaaVar->pAeGainTbl;//SF_AE_GAIN_60;
			AaaVar->AeIndexMax = 0xdd;//235-2;
		}
		else
		{
			AaaVar->AeExpTableSfAddr = AaaVar->pAeExpTbl;//SF_AE_EXP_50;
			AaaVar->AeGainTableSfAddr = AaaVar->pAeGainTbl;//SF_AE_GAIN_50;
			AaaVar->AeIndexMax = 0xdd;//235-2;//0xe9
		}

		if(AaaVar->AeIndex > AaaVar->AeIndexMax) AaaVar->AeIndex = AaaVar->AeIndexMax;
		if(AaaVar->AeIndex < 0) AaaVar->AeIndex = 0;
		AaaVar->AeIndexGet = AaaVar->AeIndex;

		if(1)//UvcVcCtrl.ctAutoExposureModeCur ==0x08)
		{
			if(AaaVar->AeExpLineMode == 0)
			{
				//EA = 0;	/* disable all interrupts */
				aeGetExposureIsr(isp_info);
				AaaVar->AeExp = AaaVar->AeExpGet;
			}
			else
			{
				AaaVar->AeIndex = 0;
			}

			//EA = 0;	/* disable all interrupts */
			aeGetGainIsr(isp_info);
			AaaVar->AeGain = AaaVar->AeGainGet;
			sensorSetExposureTimeIsr(isp_info);
			sensorSetGainIsr(isp_info);
			//EA = 1;	/* Enable all interrupts */
			AaaVar->aaaFlag = 0;
			AaaVar->aaaCount = 0;
			aeGetCurEv(isp_info);
		}
	}
	AaaVar->AeIndexPrev = AaaVar->AeIndex;
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

void sensorSetGainIsr(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	setSensor16_I2C0(AaaVar->sensor_gain_addr, AaaVar->AeGain, 2, isp_info);
}

void sensorExpToLineCountIsr(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 lineTotal;
	//u8 pllMul;
	//u8 pllDiv, pllDiv2, pllDiv4;
	u16 expLine;
	//u8 regAddr[2], regData[2];
	u32 pixelClock;
	u16 exp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* get input */
	exp = AaaVar->SensorExpIn;

	/* get pixel clock */
	pixelClock = AaaVar->sensor_pclk;
	//
	getSensor16_I2C0(AaaVar->sensor_line_total_addr, &lineTotal, 2, isp_info);
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
	AaaVar->SensorLineCountOut = expLine;
}
	
void sensorSetExposureTimeIsr(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 expLine;
	u16 frameLen;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* get exposure line */
	if (AaaVar->AeExpLineMode == 1)
	{
		expLine = AaaVar->AeLineModeExpLineCount;
	}
	else
	{
		AaaVar->SensorExpIn = AaaVar->AeExp;
		sensorExpToLineCountIsr(isp_info);
		expLine = AaaVar->SensorLineCountOut;
	}
	AaaVar->AeCurExpLine = expLine;

	/* set frame length */
	frameLen = AaaVar->SensorMinFrameLen;
	if ((expLine) > (frameLen-4))
		frameLen = expLine + 4;

	if (expLine < 4) // sensor minimun expline
	expLine = 4;
	expLine = expLine<<4;
	//
	setSensor16_I2C0(AaaVar->sensor_frame_len_addr, frameLen, 2, isp_info);
	setSensor16_I2C0(AaaVar->sensor_exp_line_addr, expLine, 2, isp_info);
}

void vidctrlCtExposureTimeAbsolute(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (AaaVar->ctExposureTimeAbsoluteCur != 0)
	{
		AaaVar->AeExp = 160000 / (AaaVar->ctExposureTimeAbsoluteCur);
		AaaVar->AeExpLineMode = 0;
		AaaVar->AeGain = MANUALEXPGAIN;

		//EA = 0;	/* disable all interrupts */
		sensorSetExposureTimeIsr(isp_info);
		sensorSetGainIsr(isp_info);
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
void vidctrlInit(struct mipi_isp_info *isp_info,
				u16 _sensor_gain_addr, 
				u16 _sensor_frame_len_addr, 
				u16 _sensor_line_total_addr, 
				u16 _sensor_exp_line_addr,
				u32 _sensor_pclk)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->sensor_gain_addr=_sensor_gain_addr;
	AaaVar->sensor_frame_len_addr=_sensor_frame_len_addr;
	AaaVar->sensor_line_total_addr=_sensor_line_total_addr;
	AaaVar->sensor_exp_line_addr=_sensor_exp_line_addr;
	AaaVar->sensor_pclk=_sensor_pclk;

	// add table init for aeGetExposureIsr()
	AaaVar->AeExpTableSfAddr = AaaVar->pAeExpTbl;//SF_AE_EXP_60;
	AaaVar->AeGainTableSfAddr = AaaVar->pAeGainTbl;//SF_AE_GAIN_60;
	AaaVar->AeIndexMax = 0xdd;//235-2;
	getSensor16_I2C0(AaaVar->sensor_frame_len_addr, &AaaVar->SensorMinFrameLen, 2, isp_info);
	//
	//EA = 0; /* disable all interrupts */
	AaaVar->AeIndexGet = AaaVar->AeIndex;
	aeGetExposureIsr(isp_info);
	AaaVar->AeExp = AaaVar->AeExpGet;
	aeGetGainIsr(isp_info);
	AaaVar->AeGain = AaaVar->AeGainGet;
	AaaVar->AeStill = 0;
	sensorSetExposureTimeIsr(isp_info);
	sensorSetGainIsr(isp_info);
	//EA = 1; /* enable all interrupts */
	//

	vidctrlPuBrightness(isp_info, PUBRIGHTNESSCUR);
	vidctrlPuContrast(isp_info, PUCONTRASTCUR);
	vidctrlPuHue(isp_info, PUHUECUR);
	vidctrlPuSaturation(isp_info, PUSATURATIONCUR);
	vidctrlPuSharpness();
	vidctrlPuGamma(isp_info, PUGAMMACUR);
	vidctrlPuBacklightCompensation(isp_info, PUPOWERLINEFREQUENCYCUR);
	vidctrlPuPowerLineFrequency();
	vidctrlPuGain();
	vidctrlCtExposureTimeAbsolute(isp_info);
	vidctrlPuWhiteBalanceTemperature();
}

void aeSetWin_ROM(struct mipi_isp_info *isp_info, u16 hsize, u16 vsize, u16 hOffset, u16 vOffset)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u16 winHSize, winVSize;
	//	

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	writeb(7, &(regs->reg[0x3206]));//9x9

	/* HwdOffset */
	writeb(LO_BYTE_OF_WORD(hOffset), &(regs->reg[0x2200]));
	writeb(HI_BYTE_OF_WORD(hOffset), &(regs->reg[0x2201]));
	/* VwdOffset */
	writeb(LO_BYTE_OF_WORD(vOffset), &(regs->reg[0x2202]));
	writeb(HI_BYTE_OF_WORD(vOffset), &(regs->reg[0x2203]));

	/* Full window size */
	winHSize = hsize - 2 * hOffset;
	winVSize = vsize - 2 * vOffset;
	/* FullHSize */
	writeb(LO_BYTE_OF_WORD(winHSize), &(regs->reg[0x2208]));
	writeb(HI_BYTE_OF_WORD(winHSize), &(regs->reg[0x2209]));
	/* FullVSize */
	writeb(LO_BYTE_OF_WORD(winVSize), &(regs->reg[0x220A]));
	writeb(HI_BYTE_OF_WORD(winVSize), &(regs->reg[0x220B]));
	/* FullHaccFac */
	if (winHSize < 8)         { writeb(0x00, &(regs->reg[0x220D])); }
	else if (winHSize < 16)   { writeb(0x01, &(regs->reg[0x220D])); }
	else if (winHSize < 32)   { writeb(0x02, &(regs->reg[0x220D])); }
	else if (winHSize < 64)   { writeb(0x03, &(regs->reg[0x220D])); }
	else if (winHSize < 128)  { writeb(0x04, &(regs->reg[0x220D])); }
	else if (winHSize < 256)  { writeb(0x05, &(regs->reg[0x220D])); }
	else if (winHSize < 512)  { writeb(0x06, &(regs->reg[0x220D])); }
	else if (winHSize < 1024) { writeb(0x07, &(regs->reg[0x220D])); }
	else if (winHSize < 2048) { writeb(0x08, &(regs->reg[0x220D])); }
	else                      { writeb(0x09, &(regs->reg[0x220D])); }
	/* FullVaccFac */
	if (winVSize < 8)         { writeb(readb(&(regs->reg[0x220D]))|0x00, &(regs->reg[0x220D])); }
	else if (winVSize < 16)   { writeb(readb(&(regs->reg[0x220D]))|0x10, &(regs->reg[0x220D])); }
	else if (winVSize < 32)   { writeb(readb(&(regs->reg[0x220D]))|0x20, &(regs->reg[0x220D])); }
	else if (winVSize < 64)   { writeb(readb(&(regs->reg[0x220D]))|0x30, &(regs->reg[0x220D])); }
	else if (winVSize < 128)  { writeb(readb(&(regs->reg[0x220D]))|0x40, &(regs->reg[0x220D])); }
	else if (winVSize < 256)  { writeb(readb(&(regs->reg[0x220D]))|0x50, &(regs->reg[0x220D])); }
	else if (winVSize < 512)  { writeb(readb(&(regs->reg[0x220D]))|0x60, &(regs->reg[0x220D])); }
	else if (winVSize < 1024) { writeb(readb(&(regs->reg[0x220D]))|0x70, &(regs->reg[0x220D])); }
	else if (winVSize < 2048) { writeb(readb(&(regs->reg[0x220D]))|0x80, &(regs->reg[0x220D])); }
	else                      { writeb(readb(&(regs->reg[0x220D]))|0x90, &(regs->reg[0x220D])); }

	/* HwdOffset */
	writeb(LO_BYTE_OF_WORD(hOffset), &(regs->reg[0x3208]));
	writeb(HI_BYTE_OF_WORD(hOffset), &(regs->reg[0x3209]));
	/* VwdOffset */
	writeb(LO_BYTE_OF_WORD(vOffset), &(regs->reg[0x320a]));
	writeb(HI_BYTE_OF_WORD(vOffset), &(regs->reg[0x320b]));


	/* Individual window size */
	winHSize /= 9;
	winVSize /= 9;
	/* AeHSize */
	writeb(LO_BYTE_OF_WORD(winHSize ), &(regs->reg[0x320c]));
	writeb(HI_BYTE_OF_WORD(winHSize ), &(regs->reg[0x320d]));
	/* AeVSize */
	writeb(LO_BYTE_OF_WORD(winVSize), &(regs->reg[0x320e]));
	writeb(HI_BYTE_OF_WORD(winVSize), &(regs->reg[0x320f]));
	/* FullHaccFac */
	if (winHSize  < 8)            { writeb(0x00, &(regs->reg[0x3207])); }
	else if (winHSize  < 16)      { writeb(0x01, &(regs->reg[0x3207])); }
	else if (winHSize  < 32)      { writeb(0x02, &(regs->reg[0x3207])); }
	else if (winHSize  < 64)      { writeb(0x03, &(regs->reg[0x3207])); }
	else if (winHSize  < 128)     { writeb(0x04, &(regs->reg[0x3207])); }
	else if (winHSize  < 256)     { writeb(0x05, &(regs->reg[0x3207])); }
	else if (winHSize  < 512)     { writeb(0x06, &(regs->reg[0x3207])); }
	else /*if (winHSize < 1024)*/ { writeb(0x07, &(regs->reg[0x3207])); }

	/* FullVaccFac */
	if (winVSize < 8)             { writeb(readb(&(regs->reg[0x3207]))|0x00, &(regs->reg[0x3207])); }
	else if (winVSize < 16)       { writeb(readb(&(regs->reg[0x3207]))|0x10, &(regs->reg[0x3207])); }
	else if (winVSize < 32)       { writeb(readb(&(regs->reg[0x3207]))|0x20, &(regs->reg[0x3207])); }
	else if (winVSize < 64)       { writeb(readb(&(regs->reg[0x3207]))|0x30, &(regs->reg[0x3207])); }
	else if (winVSize < 128)      { writeb(readb(&(regs->reg[0x3207]))|0x40, &(regs->reg[0x3207])); }
	else if (winVSize < 256)      { writeb(readb(&(regs->reg[0x3207]))|0x50, &(regs->reg[0x3207])); }
	else if (winVSize < 512)      { writeb(readb(&(regs->reg[0x3207]))|0x60, &(regs->reg[0x3207])); }
	else /*if (winVSize < 1024)*/ { writeb(readb(&(regs->reg[0x3207]))|0x70, &(regs->reg[0x3207])); }
}

void aeFuncExt(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	//u16 tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if(AaaVar->AeIndex > 130)
	{
		 writeb(12, &(regs->reg[0x2175]));
		 writeb(24, &(regs->reg[0x2176]));
		 writeb(32, &(regs->reg[0x31D0]));
		 writeb(32, &(regs->reg[0x31D1]));
		 writeb(32, &(regs->reg[0x31D2]));
		 writeb(32, &(regs->reg[0x31D3]));
		 writeb(30, &(regs->reg[0x31FE]));
		 writeb(60, &(regs->reg[0x31FF]));
	}
	else if(AaaVar->AeIndex < 100)
	{
		 writeb( 6, &(regs->reg[0x2175]));
		 writeb(12, &(regs->reg[0x2176]));
		 writeb(12, &(regs->reg[0x31D0]));
		 writeb(12, &(regs->reg[0x31D1]));
		 writeb(12, &(regs->reg[0x31D2]));
		 writeb(12, &(regs->reg[0x31D3]));
		 writeb(10, &(regs->reg[0x31FE]));
		 writeb(20, &(regs->reg[0x31FF]));
	}
	else
	{
		 writeb( 6 + (12- 6)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x2175]));
		 writeb(12 + (24-12)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x2176]));
		 writeb(12 + (32-12)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31D0]));
		 writeb(12 + (32-12)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31D1]));
		 writeb(12 + (32-12)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31D2]));
		 writeb(12 + (32-12)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31D3]));
		 writeb(10 + (30-10)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31FE]));
		 writeb(20 + (60-20)*(AaaVar->AeIndex-100)/(130-100), &(regs->reg[0x31FF]));
	}
}

void aeGetCurEv(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u32 gainVal;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (AaaVar->AeExpLineMode == 0)
	{
		gainVal = sensorGainRegToVal(AaaVar->AeGain);
		AaaVar->AeCurEv = AaaVar->AeEvConst * gainVal / (u32)AaaVar->AeExp;
	}
	else
		AaaVar->AeCurEv = AaaVar->pAeExpTbl[0];
	// else
	// {
	// 	gainVal = sensorGainRegToVal(AeGain);
	// 	AeCurEv = AeEvConst * (u32)AeCurExpLine * gainVal / (u32)0x780 / AeExpLineCount120;
	// }
}

void aeDeadLeafWinInit(struct mipi_isp_info *isp_info, u16 hsize, u16 vsize, u16 winSize)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u16 val;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* AF window */
	/* HwdOffset */
	//val = (pStrm->strmFrontHsize - winSize) / 2;
	val = ((hsize-4) - winSize) / 2;
	writeb(LO_BYTE_OF_WORD(val), &(regs->reg[0x22a0]));
	writeb(HI_BYTE_OF_WORD(val), &(regs->reg[0x22a1]));
	/* VwdOffset */
	//val = (pStrm->strmFrontVsize - winSize) / 2;
	val = ((vsize-4) - winSize) / 2;
	writeb(LO_BYTE_OF_WORD(val), &(regs->reg[0x22a2]));
	writeb(HI_BYTE_OF_WORD(val), &(regs->reg[0x22a3]));
	/* FullHSize */
	writeb(LO_BYTE_OF_WORD(winSize), &(regs->reg[0x22a4]));
	writeb(HI_BYTE_OF_WORD(winSize), &(regs->reg[0x22a5]));
	/* FullVSize */
	writeb(LO_BYTE_OF_WORD(winSize), &(regs->reg[0x22a6]));
	writeb(HI_BYTE_OF_WORD(winSize), &(regs->reg[0x22a7]));

	writeb(0x11, &(regs->reg[0x229a]));
	writeb(0x08, &(regs->reg[0x229d]));
	writeb(0x01, &(regs->reg[0x2299]));
}

void aeInitVar(struct mipi_isp_info *isp_info)  // call only once in main when plug in
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->AeEvConst = AE_EV_CONST;	
	AaaVar->AeCurExpLine = 3;
	AaaVar->AeExpLineMode = 0;
}

void InstallVSinterrupt(void)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	//please do by yourself
	//ex. install intrIntr0SensorVsync() function
}

void intrIntr0SensorVsync(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (((readb(&(regs->reg[0x27C0])) & INTR_VD_EDGE) == INTR_VD_EDGE) &&	/* sensor vd falling interrupt enable */
		((readb(&(regs->reg[0x27B0])) & INTR_VD_EDGE) == INTR_VD_EDGE)) {	/* sensor vd falling interrupt event */
		ISPAPI_LOGI("%s, V-sync Interrupt occur!\n", __FUNCTION__);

		writeb(INTR_VD_EDGE, &(regs->reg[0x27B0]));	/* clear vd falling edge interrupt AAF061 W1C*/

		AaaVar->aaaCount++;
		if (AaaVar->aaaCount >= AaaVar->AaaFreq)
		{
			AaaVar->aaaFlag  = 1;	/* set 3a flag*/
			AaaVar->aaaCount = 0;	/* clear count */
		}
	}
}

//initResetParam
void aaaLoadInit(struct mipi_isp_info *isp_info, const char *aaa_init_file)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->AeIndexMax60  = (u8)aaa_init_file[0];
	AaaVar->AeIndexMax50  = (u8)aaa_init_file[1];
	AaaVar->AeIndexMin60  = (u8)aaa_init_file[2];
	AaaVar->AeIndexMin50  = (u8)aaa_init_file[3];
	AaaVar->AeIndex       = (short)(aaa_init_file[4]<<8)|aaa_init_file[5];
	AaaVar->AeIndexPrev   = (short)(aaa_init_file[6]<<8)|aaa_init_file[7];
	AaaVar->AeDeadZone    = (u8)aaa_init_file[8]; 
	AaaVar->AeYTarget     = (u16)(aaa_init_file[9]<<8)|aaa_init_file[10];
	AaaVar->AwbBTargetMin = (u8)aaa_init_file[11];
	AaaVar->AwbBTargetMax = (u8)aaa_init_file[12];
	AaaVar->AwbRTargetMin = (u8)aaa_init_file[13];
	AaaVar->AwbRTargetMax = (u8)aaa_init_file[14];
	AaaVar->AwbGTargetMin = (u8)aaa_init_file[15];
	AaaVar->AwbGTargetMax = (u8)aaa_init_file[16];
	AaaVar->AwbRGainReg   = (u16)(aaa_init_file[17]<<8)|aaa_init_file[18];
	AaaVar->AwbGGainReg   = (u16)(aaa_init_file[19]<<8)|aaa_init_file[20];
	AaaVar->AwbBGainReg   = (u16)(aaa_init_file[21]<<8)|aaa_init_file[22];
	AaaVar->AaaFreq       = (u8)aaa_init_file[23];
}

void aeLoadAETbl(struct mipi_isp_info *isp_info, const char *ae_table_file)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	for (i = 0; i < 256; i++)
	{
		AaaVar->pAeExpTbl[i] = (u16)(ae_table_file[i*2]<<8)|ae_table_file[(i*2)+1];
	}
}
 
void aeLoadGainTbl(struct mipi_isp_info *isp_info, const char *gain_table_file)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	for (i = 0; i < 256; i++)
	{
		AaaVar->pAeGainTbl[i] = (u16)(gain_table_file[i*2]<<8)|gain_table_file[(i*2)+1];
	}
}

void aeInitExt(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 hsize, vsize;
	u16 hOffset, vOffset;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	aeInitVar(isp_info);
	AaaVar->aaaFlag = 0;
	AaaVar->aaaCount = 0;
	hsize = (u16)(readb(&(regs->reg[0x2725]))<<8)|readb(&(regs->reg[0x2724]));
	vsize = (u16)(readb(&(regs->reg[0x2727]))<<8)|readb(&(regs->reg[0x2726]));	
	hsize -= 4;
	vsize -= 4;
	hOffset = 0;
	vOffset = 0;
	//
	AaaVar->AeImgSize=(u32)hsize*(u32)vsize;
	aeFuncExt(isp_info);
	//	
	AaaVar->AeExpLineCount120 = 0;
	aeGetCurEv(isp_info);
	AaaVar->AeDeadZone = 2;
	aeDeadLeafWinInit(isp_info, (hsize+4), (vsize+4), AaaVar->AeDeadLeafWinSize);
	AaaVar->AeDarkMode = 0;
	writeb(AaaVar->AeDarkThresh0, &(regs->reg[0x224C]));
	AaaVar->AeIndexPrev = AaaVar->AeIndex-1;
	
	vidctrlPuSaturation(isp_info, PUSATURATIONCUR);
	aeSetWin_ROM(isp_info, hsize, vsize, hOffset, vOffset);
	AaaVar->AeDeadZone = 2;
}

void awbSetGain(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	writeb(LO_BYTE_OF_WORD(AaaVar->AwbRGainReg), &(regs->reg[0x3104]));
	writeb(HI_BYTE_OF_WORD(AaaVar->AwbRGainReg), &(regs->reg[0x3105]));

	writeb(LO_BYTE_OF_WORD(AaaVar->AwbGGainReg), &(regs->reg[0x3106]));
	writeb(HI_BYTE_OF_WORD(AaaVar->AwbGGainReg), &(regs->reg[0x3107]));

	writeb(LO_BYTE_OF_WORD(AaaVar->AwbBGainReg), &(regs->reg[0x3108]));
	writeb(HI_BYTE_OF_WORD(AaaVar->AwbBGainReg), &(regs->reg[0x3109]));
}

void awbSetIndivWin(struct mipi_isp_info *isp_info, u16 hSize, u16 vSize, u16 hOffset, u16 vOffset)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* IndHaccFac */
	if (hSize < 8)         { writeb(0x00, &(regs->reg[0x220E])); }
	else if (hSize < 16)   { writeb(0x01, &(regs->reg[0x220E])); }
	else if (hSize < 32)   { writeb(0x02, &(regs->reg[0x220E])); }
	else if (hSize < 64)   { writeb(0x03, &(regs->reg[0x220E])); }
	else if (hSize < 128)  { writeb(0x04, &(regs->reg[0x220E])); }
	else if (hSize < 256)  { writeb(0x05, &(regs->reg[0x220E])); }
	else if (hSize < 512)  { writeb(0x06, &(regs->reg[0x220E])); }
	else if (hSize < 1024) { writeb(0x07, &(regs->reg[0x220E])); }
	else if (hSize < 2048) { writeb(0x08, &(regs->reg[0x220E])); }
	else                   { writeb(0x09, &(regs->reg[0x220E])); }
	/* IndVaccFac */
	if (vSize < 8)         { writeb(readb(&(regs->reg[0x220E]))|0x00, &(regs->reg[0x220E])); }
	else if (vSize < 16)   { writeb(readb(&(regs->reg[0x220E]))|0x10, &(regs->reg[0x220E])); }
	else if (vSize < 32)   { writeb(readb(&(regs->reg[0x220E]))|0x20, &(regs->reg[0x220E])); }
	else if (vSize < 64)   { writeb(readb(&(regs->reg[0x220E]))|0x30, &(regs->reg[0x220E])); }
	else if (vSize < 128)  { writeb(readb(&(regs->reg[0x220E]))|0x40, &(regs->reg[0x220E])); }
	else if (vSize < 256)  { writeb(readb(&(regs->reg[0x220E]))|0x50, &(regs->reg[0x220E])); }
	else if (vSize < 512)  { writeb(readb(&(regs->reg[0x220E]))|0x60, &(regs->reg[0x220E])); }
	else if (vSize < 1024) { writeb(readb(&(regs->reg[0x220E]))|0x70, &(regs->reg[0x220E])); }
	else if (vSize < 2048) { writeb(readb(&(regs->reg[0x220E]))|0x80, &(regs->reg[0x220E])); }
	else                   { writeb(readb(&(regs->reg[0x220E]))|0x90, &(regs->reg[0x220E])); }

	/* HwdOffset */
	writeb(LO_BYTE_OF_WORD(hOffset), &(regs->reg[0x22E8]));
	writeb(HI_BYTE_OF_WORD(hOffset), &(regs->reg[0x22E9]));
	/* VwdOffset */
	writeb(LO_BYTE_OF_WORD(vOffset), &(regs->reg[0x22EA]));
	writeb(HI_BYTE_OF_WORD(vOffset), &(regs->reg[0x22EB]));

	/* Individual window size */
	/* IndivHSize */
	writeb(LO_BYTE_OF_WORD(hSize), &(regs->reg[0x22EC]));
	writeb(HI_BYTE_OF_WORD(hSize), &(regs->reg[0x22ED]));
	/* IndivVSize */
	writeb(LO_BYTE_OF_WORD(vSize), &(regs->reg[0x22EE]));
	writeb(HI_BYTE_OF_WORD(vSize), &(regs->reg[0x22EF]));
}


void awbInitVar(struct mipi_isp_info *isp_info)
{ 
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->AwbRWidth = AWB_R_WIDTH;
	AaaVar->AwbBWidth = AWB_B_WIDTH;
	AaaVar->AwbMinGrayDetCnt = AWB_MIN_GRAY_DET_CNT;

	AaaVar->AwbCheckBlkCnt = AWB_CHECK_BLK_CNT;
	AaaVar->pAwbRRef = AwbRRef;
	AaaVar->pAwbBRef = AwbBRef;

	AaaVar->AwbCmAdjCnt = AWB_CM_ADJ_CNT;
	AaaVar->pAwbAdjCmColTemp = AwbAdjCmColTemp; 
	AaaVar->pAwbAdjCmColMat = (const short*)AwbAdjCmColMat;

	AaaVar->AwbLensAdjCnt = AWB_LENS_ADJ_CNT;
	AaaVar->pAwbAdjLensColTemp = AwbAdjLensColTemp;
	AaaVar->pAwbAdjLensRVal = AwbAdjLensRVal;
	AaaVar->pAwbAdjLensBVal = AwbAdjLensBVal;

	AaaVar->AwbFirstInit = 1;
	AaaVar->AwbMode = 0;
}

void awbAdjLensComp(u8 init)
{
	//N/A
}

void awbAdjColMat(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	short r1, r2, r3;
	u8 i, j;
	short cm[10];
	u16 tmp;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	tmp = AaaVar->AwbCurColTemp;

	if (tmp < AaaVar->pAwbAdjCmColTemp[0])                       tmp = AaaVar->pAwbAdjCmColTemp[0];
	if (tmp > AaaVar->pAwbAdjCmColTemp[AaaVar->AwbCmAdjCnt - 1]) tmp = AaaVar->pAwbAdjCmColTemp[AaaVar->AwbCmAdjCnt - 1];

	for (j = 0; j <= AaaVar->AwbCmAdjCnt - 2; j++)
	{
	 	if ((tmp >= AaaVar->pAwbAdjCmColTemp[j]) && (tmp <= AaaVar->pAwbAdjCmColTemp[j+1]))
			break;
	}

	r1 = AaaVar->pAwbAdjCmColTemp[j+1] - tmp;
	r2 = tmp - AaaVar->pAwbAdjCmColTemp[j];
	r3 = AaaVar->pAwbAdjCmColTemp[j+1] - AaaVar->pAwbAdjCmColTemp[j];

	for (i = 0; i < 9; i++)
		cm[i] = (r1 * (short)AaaVar->pAwbAdjCmColMat[j * 9 + i] + r2 * (short)AaaVar->pAwbAdjCmColMat[(j+1) * 9 + i]) / r3;

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
		writeb((u8)cm[i], &(regs->reg[0x2148 + i * 2]));

		if (cm[i] < 0) writeb(1, &(regs->reg[0x2148 + i * 2 + 1]));
		else           writeb(0, &(regs->reg[0x2148 + i * 2 + 1]));
	}
}

void awbInit(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	awbInitVar(isp_info);
	AaaVar->AwbMinGain = 0x42;
	writeb(0x31, &(regs->reg[0x2217]));   /* new awb */
	writeb(0x71, &(regs->reg[0x3100]));   /* new awb gain */
	writeb(0xf8, &(regs->reg[0x2218]));   /* hi threshold */
	writeb(0x05, &(regs->reg[0x221B]));   /* lo threshold */

	writeb(64, &(regs->reg[0x2219]));     /* gr shift */
	writeb(64, &(regs->reg[0x221A]));     /* gb shift */

	writeb(0xf8, &(regs->reg[0x2213]));   /* lum high threshold in Sec-1 */
	writeb(0xf8, &(regs->reg[0x2214]));   /* lum high threshold in Sec-2 */
	writeb(0xf8, &(regs->reg[0x2215]));   /* lum high threshold in Sec-3 */
	writeb(0xf8, &(regs->reg[0x2216]));   /* lum high threshold in Sec-4 */
	writeb(0xf8, &(regs->reg[0x3221]));   /* lum high threshold in Sec-5 */

	writeb(0x05, &(regs->reg[0x221C]));   /* lum low threshold in Sec-1 */
	writeb(0x05, &(regs->reg[0x221D]));   /* lum low threshold in Sec-2 */
	writeb(0x05, &(regs->reg[0x221E]));   /* lum low threshold in Sec-3 */
	writeb(0x05, &(regs->reg[0x221F]));   /* lum low threshold in Sec-4 */
	writeb(0x05, &(regs->reg[0x3220]));   /* lum low threshold in Sec-5 */

	writeb(0x00, &(regs->reg[0x2131]));   /* r gain high byte */ 
	writeb(0x00, &(regs->reg[0x2139]));   /* b gain high byte */
	writeb(0x00, &(regs->reg[0x2135]));   /* gr gain high byte */
	writeb(0x00, &(regs->reg[0x213D]));   /* gb gain high byte */
	writeb(64, &(regs->reg[0x2130]));     /* r / b / gr / gb gain low byte */
	writeb(64, &(regs->reg[0x2138]));     /* r / b / gr / gb gain low byte */
	writeb(64, &(regs->reg[0x2134]));     /* r / b / gr / gb gain low byte */
	writeb(64, &(regs->reg[0x213C]));     /* r / b / gr / gb gain low byte */
	writeb(0, &(regs->reg[0x2210]));      //AWB full,Histogram full

	awbSetIndivWin(isp_info, 80, 80, 80, 80);
	awbSetGain(isp_info);

	if (AaaVar->AwbFirstInit)
	{	
		AaaVar->AwbColTemp = AaaVar->AwbCurColTemp = (u16)AaaVar->AwbBGainReg * 64 / AaaVar->AwbRGainReg;
		AaaVar->AwbRGainTarget = AaaVar->AwbRawRGain = AaaVar->AwbRGainReg;
		AaaVar->AwbBGainTarget = AaaVar->AwbRawBGain = AaaVar->AwbBGainReg;
		AaaVar->AwbGGainTarget = 0x40;
	}
	else
	{
		AaaVar->AwbColTemp = AaaVar->AwbCurColTemp = (u16)AaaVar->AwbRawBGain * 64 / AaaVar->AwbRawRGain;
	}

	awbAdjLensComp(1);
	awbAdjColMat(isp_info);

	AaaVar->AwbMode = 0;
	AaaVar->AwbDeadZone = 4;
	AaaVar->AwbFirstInit = 0;
}

void aeAdjYTarget_RAM(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->AeAdjYTarget = (short)AaaVar->AeYTarget + ((short)AaaVar->puBacklightCompensationCur - (short)AaaVar->puBacklightCompensationDef)*16;	

	if(AaaVar->AeIndex > 170)      AaaVar->AeAdjYTarget -= 20;
	else if(AaaVar->AeIndex > 130) AaaVar->AeAdjYTarget -= 20*(AaaVar->AeIndex - 130)/(170-130);	
	if (AaaVar->AeAdjYTarget < 2)	AaaVar->AeAdjYTarget = 2; 
}

short aeGetYLayer(struct mipi_isp_info *isp_info, u8 *pWeight)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u32 winSize;
	u32 normWinSize;
	u32 ySum;
	u8 i;	
	//u16 weightSum;
	u16 tempY;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	/* Calculate yLayer */
	/* individual window size */
	winSize = ((u32)((u16)(readb(&(regs->reg[0x320d])) & 0x01)) << 8 | readb(&(regs->reg[0x320c]))) *
			  ((u32)((u16)(readb(&(regs->reg[0x320f])) & 0x01)) << 8 | readb(&(regs->reg[0x320e])));

	/* normalized individual window size */
	normWinSize = ((u32)(8 << ( readb(&(regs->reg[0x3207])) & 0x07))) *
				  ((u32)(8 << ((readb(&(regs->reg[0x3207])) & 0x70) >> 4)));

	writeb(0, &(regs->reg[0x325d]));
	/* read ae window 1 ~ 25 */	
	//weightSum = ySum = 0;
	ySum = 0;
	
	for (i = 0; i < 81; i++)
	{
		tempY = (u16)readb(&(regs->reg[0x325e])) * normWinSize / winSize;
		ySum += (u16)tempY;
   }

	AaaVar->AeYLayer  = (ySum / 81);
	
	return (AaaVar->AeYLayer);
}

u16 aeAdjIndexRatio(struct mipi_isp_info *isp_info, u16 adjYTarget, short yLayer)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
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
		AaaVar->AeDeadZone = 4;	
	}
	
	return(adjRatio);
}

void aeAdjIndex(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u32 ratio;
	u32 desireEv, deltaEv, minDEv,PreEV;
	short i, desireAeIdx;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	ratio = aeAdjIndexRatio(isp_info, AaaVar->AeAdjYTarget, AaaVar->AeYLayer);
	AaaVar->AeTest = 0;

	if (AaaVar->AeExpLineMode == 1 || (AaaVar->AeIndex == 0 && AaaVar->AeYLayer > AaaVar->AeAdjYTarget))
	{
		AaaVar->AeTest = 1;
		AaaVar->AeExpLineMode = 1;
		
		AaaVar->AeExp = (u16)(AaaVar->AeCurExpLine * ratio / 100);
			
		//aePostAdjIndex_RAM(&AaaVar->AeExp);
		
		if(AaaVar->AeExp < 4) AaaVar->AeExp = 4;

		if ((AaaVar->AeExpLineCount120 != 0) && (AaaVar->AeExp > AaaVar->AeExpLineCount120)) 
			AaaVar->AeExpLineMode = 0;

		AaaVar->AeIndexGet = AaaVar->AeIndex = 0;
		//EA = 0;	/* disable all interrupts */
		AaaVar->AeLineModeExpLineCount = AaaVar->AeExp;
		aeGetGainIsr(isp_info);
		AaaVar->AeGain = AaaVar->AeGainGet;
	}
	else
	{
		//DELTA_EV d;
		
		AaaVar->AeExpLineMode = FALSE;
		desireEv = AaaVar->AeCurEv * ratio / 100;
				
		minDEv = aeGetDeltaEv(isp_info, desireEv, AaaVar->AeIndex);	
		
		PreEV = AaaVar->AeCurEv;
		desireAeIdx = AaaVar->AeIndex;

		if (AaaVar->AeAdjYTarget > AaaVar->AeYLayer)
		{
			AaaVar->AeTest = 2; 
			for (i = AaaVar->AeIndex+1; i <= AaaVar->AeIndexMax; i++)
			{				
				deltaEv = aeGetDeltaEv(isp_info, desireEv, i);
				

				if ((deltaEv > minDEv)&&(AaaVar->AeCurEv > PreEV))
					break;
				else if (deltaEv <= minDEv)
				{
					minDEv = deltaEv;
					desireAeIdx = i;
				}
				PreEV = AaaVar->AeCurEv;
			}
		}
		else
		{
			AaaVar->AeTest = 3;
			for (i = AaaVar->AeIndex-1; i >= 0; i--)
			{
				deltaEv = aeGetDeltaEv(isp_info, desireEv, i);

				if ((deltaEv > minDEv)&&(AaaVar->AeCurEv < PreEV))
					break;
				else if (deltaEv <= minDEv)
				{
					minDEv = deltaEv;
					desireAeIdx = i;
				}
				PreEV = AaaVar->AeCurEv;
			}
		}

		//aePostAdjIndex_RAM((u16 *)&desireAeIdx);

		//EA = 0;	/* disable all interrupts */
		AaaVar->AeIndex = AaaVar->AeIndexGet = desireAeIdx;
		//uartPutString_ROM("AaaVar->AeIndex=");
		//uartPutNumber_ROM(AaaVar->AeIndex);
		
		aeGetGainIsr(isp_info);
		AaaVar->AeGain = AaaVar->AeGainGet;
	}

	if(AaaVar->AeExpLineMode == FALSE)
	{
		aeGetExposureIsr(isp_info);
		AaaVar->AeExp = AaaVar->AeExpGet;
	}
	//EA = 1;	/* Enable all interrupts */

	aeGetCurEv(isp_info);
}

void aeFunc(struct mipi_isp_info *isp_info, u8 aeOn)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (aeOn)
	{
	
		if (AaaVar->AeExpLineCount120 == 0)
		{
			//EA = 0;	/* disable all interrupts */
			AaaVar->SensorExpIn = 0x780;
			sensorExpToLineCountIsr(isp_info);
			AaaVar->AeExpLineCount120 = AaaVar->SensorLineCountOut;
			//EA = 1;	/* Enable all interrupts */
		}
		aeAdjYTarget_RAM(isp_info);
		aeGetYLayer(isp_info, AaaVar->pAeNormWeight);
		AaaVar->aaaFlag = 0;	// to avoid AE call continuoustly if no exposure/gain changed
	
		if ((AaaVar->AeYLayer < (AaaVar->AeAdjYTarget - AaaVar->AeDeadZone)) || (AaaVar->AeYLayer > (AaaVar->AeAdjYTarget + AaaVar->AeDeadZone)))
		{
			AaaVar->AeDeadZone = 4;			
			aeAdjIndex(isp_info);

		}

		/* Set exposure and gain by auto-exposure index */
		if ((AaaVar->AeIndex != AaaVar->AeIndexPrev)  ||
			((AaaVar->AeExpLineMode == 1) && (AaaVar->AeCurExpLine != AaaVar->AeLineModeExpLineCount)) ||
			(AaaVar->AePreExpLineMode != AaaVar->AeExpLineMode))
		{			
			AaaVar->aaaFlag = 0;
			AaaVar->aaaCount = 0;
			AaaVar->AePreExpLineMode = AaaVar->AeExpLineMode;
						
			//EA = 0;	/* disable all interrupts */
			AaaVar->AeStill = 0;

			sensorSetExposureTimeIsr(isp_info);
			sensorSetGainIsr(isp_info);
			//EA = 1;	/* enable all interrupts */

			AaaVar->AeIndexPrev = AaaVar->AeIndex;
			AaaVar->AeStable = 0;
		}
		else
		{
			AaaVar->AeStable++;
		}	
		aeFuncExt(isp_info);	
	}
}

u32 mathlibUcharToUlong(u8 x, u8 y, u8 u, u8 v)
{
	ISP3A_LOGD("%s start\n", __FUNCTION__);

	return ((unsigned long) (((unsigned long)x)<<24)|(((unsigned long)y)<<16)|(((unsigned long)u)<<8)|((unsigned long)(v)));
}

u32 awbCount(struct mipi_isp_info *isp_info, u8 id)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 offset;
	u32 count;
	//u8 i;   // SunplusIT: added for debug test

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (id < 4)
	{
		offset = id * 12;
		//count = u8_TO_u32(0x00, readb(&(regs->reg[0x2262 + offset])), readb(&(regs->reg[0x2261 + offset])), readb(&(regs->reg[0x2260 + offset])));
		count = mathlibUcharToUlong(0x00, readb(&(regs->reg[0x2262+offset])), readb(&(regs->reg[0x2261+offset])), readb(&(regs->reg[0x2260+offset])));
	}
	else // if (id == 4)
	{
		//count = u8_TO_u32(0x00, readb(&(regs->reg[0x3232])), readb(&(regs->reg[0x3231])), readb(&(regs->reg[0x3230])));
		count = mathlibUcharToUlong(0x00, readb(&(regs->reg[0x3232])), readb(&(regs->reg[0x3231])), readb(&(regs->reg[0x3230])));
	}

	/* SunplusIT: added for debug test
	if((readb(&(regs->reg[0x0190])) & 0x10) == 0x10)
	{
		i = readb(&(regs->reg[0x2BCD]));
		writeb(count >> 24, &(regs->reg[0x3b00 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b01 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b02 + i * 4]));
		writeb(count,       &(regs->reg[0x3b03 + i * 4]));
		writeb(readb(&(regs->reg[0x2BCD]))+1, &(regs->reg[0x2BCD]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x20) == 0x20)
	{
		i = readb(&(regs->reg[0x2BCE]));
		writeb(count >> 24, &(regs->reg[0x3b14 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b15 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b16 + i * 4]));
		writeb(count,       &(regs->reg[0x3b17 + i * 4]));
		writeb(readb(&(regs->reg[0x2BCE]))+1, &(regs->reg[0x2BCE]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x40) == 0x40)
	{
		i = readb(&(regs->reg[0x2BCF]));
		writeb(count >> 24, &(regs->reg[0x3b28 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b29 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b2A + i * 4]));
		writeb(count,       &(regs->reg[0x3b2B + i * 4]));
		writeb(readb(&(regs->reg[0x2BCF]))+1, &(regs->reg[0x2BCF]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x80) == 0x80)
	{
		i = readb(&(regs->reg[0x28CD]));
		writeb(count >> 24, &(regs->reg[0x3b3C + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b3D + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b3E + i * 4]));
		writeb(count,       &(regs->reg[0x3b3F + i * 4]));
		writeb(readb(&(regs->reg[0x28CD]))+1, &(regs->reg[0x28CD]));
	}
	*/
	ISP3A_LOGD("%s end\n", __FUNCTION__);
	return(count);
}

u32 awbColSum(struct mipi_isp_info *isp_info, u8 col, u8 id)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 offset;
	u32 count;
	//u8 i;   // SunplusIT: added for debug test

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (col == 1)
	{
		if (id < 4)
		{
			offset = id * 4;
			//count = u8_TO_u32(readb(&(regs->reg[0x3213+offset])), readb(&(regs->reg[0x3212+offset])), readb(&(regs->reg[0x3211+offset])), readb(&(regs->reg[0x3210+offset])));
			count = mathlibUcharToUlong(readb(&(regs->reg[0x3213+offset])), readb(&(regs->reg[0x3212+offset])), readb(&(regs->reg[0x3211+offset])), readb(&(regs->reg[0x3210+offset])));
		}
		else
		{
			//count = u8_TO_u32(readb(&(regs->reg[0x323f])), readb(&(regs->reg[0x323e])), readb(&(regs->reg[0x323d])), readb(&(regs->reg[0x323c])));
			count = mathlibUcharToUlong(readb(&(regs->reg[0x323f])), readb(&(regs->reg[0x323e])), readb(&(regs->reg[0x323d])), readb(&(regs->reg[0x323c])));
		}
	}
	else
	{
		if (col == 2) offset = 4;
		else          offset = 0;

		if (id < 4)
		{
		   offset += (id * 12);
		   //count = u8_TO_u32(readb(&(regs->reg[0x2267+offset])), readb(&(regs->reg[0x2266+offset])), readb(&(regs->reg[0x2265+offset])), readb(&(regs->reg[0x2264+offset])));
			 count = mathlibUcharToUlong(readb(&(regs->reg[0x2267+offset])), readb(&(regs->reg[0x2266+offset])), readb(&(regs->reg[0x2265+offset])), readb(&(regs->reg[0x2264+offset])));
		}
		else // if (id == 4)
		{
		   //count = u8_TO_u32(readb(&(regs->reg[0x3237+offset])), readb(&(regs->reg[0x3236+offset])), readb(&(regs->reg[0x3235+offset])), readb(&(regs->reg[0x3234+offset])));
			 count = mathlibUcharToUlong(readb(&(regs->reg[0x3237+offset])), readb(&(regs->reg[0x3236+offset])), readb(&(regs->reg[0x3235+offset])), readb(&(regs->reg[0x3234+offset])));
		}
	}

	/* SunplusIT: added for debug test
	if((readb(&(regs->reg[0x0190])) & 0x10) == 0x10)
	{
		i = readb(&(regs->reg[0x2BCD]));
		writeb(count >> 24, &(regs->reg[0x3b00 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b01 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b02 + i * 4]));
		writeb(count,       &(regs->reg[0x3b03 + i * 4]));
		writeb(readb(&(regs->reg[0x2BCD]))+1, &(regs->reg[0x2BCD]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x20) == 0x20)
	{
		i = readb(&(regs->reg[0x2BCE]));
		writeb(count >> 24, &(regs->reg[0x3b14 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b15 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b16 + i * 4]));
		writeb(count,       &(regs->reg[0x3b17 + i * 4]));
		writeb(readb(&(regs->reg[0x2BCE]))+1, &(regs->reg[0x2BCE]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x40) == 0x40)
	{
		i = readb(&(regs->reg[0x2BCF]));
		writeb(count >> 24, &(regs->reg[0x3b28 + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b29 + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b2A + i * 4]));
		writeb(count,       &(regs->reg[0x3b2B + i * 4]));
		writeb(readb(&(regs->reg[0x2BCF]))+1, &(regs->reg[0x2BCF]));
	}
	else if((readb(&(regs->reg[0x0190])) & 0x80) == 0x80)
	{
		i = readb(&(regs->reg[0x28CD]));
		writeb(count >> 24, &(regs->reg[0x3b3C + i * 4]));
		writeb(count >> 16, &(regs->reg[0x3b3D + i * 4]));
		writeb(count >> 7,  &(regs->reg[0x3b3E + i * 4]));
		writeb(count,       &(regs->reg[0x3b3F + i * 4]));
		writeb(readb(&(regs->reg[0x28CD]))+1, &(regs->reg[0x28CD]));
	}
	*/
	return(count);
}

void awbSetCenter(struct mipi_isp_info *isp_info, u8 id, u8 centerR, u8 centerB, u8 widthR, u8 widthB)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 offset;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (id < 4)
	{
	   offset = id << 2;
	   if (centerR < widthR)		writeb(0,                &(regs->reg[0x2220 + offset]));
	   else							writeb(centerR - widthR, &(regs->reg[0x2220 + offset]));
	   writeb(centerR + widthR, &(regs->reg[0x2221 + offset]));
	   writeb(centerB - widthB, &(regs->reg[0x2222 + offset]));
	   if (centerB > 255 - widthB)	writeb(255,              &(regs->reg[0x2223 + offset]));
	   else							writeb(centerB + widthB, &(regs->reg[0x2223 + offset]));
	}
	else //if(id == 4)
	{
	   writeb(centerR - widthR, &(regs->reg[0x3222]));
	   writeb(centerR + widthR, &(regs->reg[0x3223]));
	   writeb(centerB - widthB, &(regs->reg[0x3224]));
	   writeb(centerB + widthB, &(regs->reg[0x3225]));
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
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	writeb(LO_BYTE_OF_WORD(AwbRGainReg), &(regs->reg[0x3104]));
	writeb(HI_BYTE_OF_WORD(AwbRGainReg), &(regs->reg[0x3105]));

	writeb(LO_BYTE_OF_WORD(AwbGGainReg), &(regs->reg[0x3106]));
	writeb(HI_BYTE_OF_WORD(AwbGGainReg), &(regs->reg[0x3107]));

	writeb(LO_BYTE_OF_WORD(AwbBGainReg), &(regs->reg[0x3108]));
	writeb(HI_BYTE_OF_WORD(AwbBGainReg), &(regs->reg[0x3109]));
}
*/
void awbFuncExt(struct mipi_isp_info *isp_info)
{           
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	//u8 i;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->AwbRGainReg = awbAdjGain(AaaVar->AwbRGainReg, AaaVar->AwbRGainTarget+2);
	AaaVar->AwbGGainReg = awbAdjGain(AaaVar->AwbGGainReg, AaaVar->AwbGGainTarget);
	AaaVar->AwbBGainReg = awbAdjGain(AaaVar->AwbBGainReg, AaaVar->AwbBGainTarget+2);

	awbSetGain(isp_info);
	
	if (AaaVar->AwbColTemp > AaaVar->AwbCurColTemp + 10)		AaaVar->AwbCurColTemp = AaaVar->AwbCurColTemp + 5;
	else if (AaaVar->AwbColTemp > AaaVar->AwbCurColTemp)		AaaVar->AwbCurColTemp++;
	else if (AaaVar->AwbColTemp < AaaVar->AwbCurColTemp - 10)	AaaVar->AwbCurColTemp =  AaaVar->AwbCurColTemp - 5;
	else if (AaaVar->AwbColTemp < AaaVar->AwbCurColTemp)		AaaVar->AwbCurColTemp--;

	//awbAdjLensComp(0);
	awbAdjColMat(isp_info);
}

void awbFunc(struct mipi_isp_info *isp_info, u8 awbOn)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;
	u16 i;
	u16 sR,sG,sB;
	u16 tmp;
	u16 minG;
	//u8 AwbCenterR,AwbCenterB;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if (awbOn)
	{		
		if(AaaVar->AwbMode == 3)
		{
			for(i=10; i<AaaVar->AwbCheckBlkCnt; i++)
			{
					AaaVar->AwbSumCount = AaaVar->AwbSumCount + awbCount(isp_info, i-10);
					AaaVar->AwbSumR = AaaVar->AwbSumR + awbColSum(isp_info, 0, i-10);
					AaaVar->AwbSumG = AaaVar->AwbSumG + awbColSum(isp_info, 1, i-10);
					AaaVar->AwbSumB = AaaVar->AwbSumB + awbColSum(isp_info, 2, i-10);
			}

			if (AaaVar->AwbSumCount > 0x80)
			{				
				sR = (u16)(AaaVar->AwbSumR / AaaVar->AwbSumCount);
				sG = (u16)(AaaVar->AwbSumG / AaaVar->AwbSumCount);
				sB = (u16)(AaaVar->AwbSumB / AaaVar->AwbSumCount);

				AaaVar->AwbRawRGain = (u16)((0x40 * sG + sR / 2) / sR);
				AaaVar->AwbRawBGain = (u16)((0x40 * sG + sB / 2) / sB);		 

				AaaVar->AwbRGainTarget = AaaVar->AwbRawRGain;
				AaaVar->AwbBGainTarget = AaaVar->AwbRawBGain;			

				tmp = (u16)AaaVar->AwbBGainTarget * 64 / AaaVar->AwbRGainTarget;

				if ((tmp > AaaVar->AwbColTemp + 4) || (tmp < AaaVar->AwbColTemp - 4))
					AaaVar->AwbColTemp = tmp;

				if (AaaVar->AwbRGainTarget < AaaVar->AwbRTargetMin)	AaaVar->AwbRGainTarget = AaaVar->AwbRTargetMin;
				if (AaaVar->AwbRGainTarget > AaaVar->AwbRTargetMax)	AaaVar->AwbRGainTarget = AaaVar->AwbRTargetMax;
				if (AaaVar->AwbBGainTarget < AaaVar->AwbBTargetMin)	AaaVar->AwbBGainTarget = AaaVar->AwbBTargetMin;
				if (AaaVar->AwbBGainTarget > AaaVar->AwbBTargetMax)	AaaVar->AwbBGainTarget = AaaVar->AwbBTargetMax;


				if(AaaVar->AeIndex < AaaVar->AeIndexMax - 30) 
					AaaVar->AwbDeadZone = 2;
				else
					AaaVar->AwbDeadZone = 2+4*(AaaVar->AeIndex + 30 - AaaVar->AeIndexMax)/30;
					
				if ((AaaVar->AwbPreRTarget - AaaVar->AwbDeadZone <= AaaVar->AwbRGainTarget) && (AaaVar->AwbPreRTarget + AaaVar->AwbDeadZone >= AaaVar->AwbRGainTarget) &&
					(AaaVar->AwbPreBTarget - AaaVar->AwbDeadZone <= AaaVar->AwbBGainTarget) && (AaaVar->AwbPreBTarget + AaaVar->AwbDeadZone >= AaaVar->AwbBGainTarget))
				{
					AaaVar->AwbRGainTarget = AaaVar->AwbPreRTarget;
					AaaVar->AwbBGainTarget = AaaVar->AwbPreBTarget;
				}	 
				else
				{
					AaaVar->AwbPreRTarget = AaaVar->AwbRGainTarget;
					AaaVar->AwbPreBTarget = AaaVar->AwbBGainTarget;
				}
				
				AaaVar->AwbGGainTarget = minG =0x40;
				if (AaaVar->AwbRGainTarget < minG)	minG = AaaVar->AwbRGainTarget;
				if (AaaVar->AwbBGainTarget < minG)	minG = AaaVar->AwbBGainTarget;
				
				AaaVar->AwbRGainTarget = ((u16)AaaVar->AwbRGainTarget * 0x40) / minG ;
				AaaVar->AwbGGainTarget = ((u16)AaaVar->AwbGGainTarget * 0x40) / minG ;
				AaaVar->AwbBGainTarget = ((u16)AaaVar->AwbBGainTarget * 0x40) / minG ;
								
			}

			AaaVar->AwbMode = 0;
		}

		if(AaaVar->AwbMode == 0)
		{
			AaaVar->AwbSumCount = AaaVar->AwbSumR = AaaVar->AwbSumG = AaaVar->AwbSumB = 0;
		}
		else
		{
			for(i=0; i<5; i++)
			{
				AaaVar->AwbSumCount = AaaVar->AwbSumCount + awbCount(isp_info, i);
				AaaVar->AwbSumR = AaaVar->AwbSumR + awbColSum(isp_info, 0, i);
				AaaVar->AwbSumG = AaaVar->AwbSumG + awbColSum(isp_info, 1, i);
				AaaVar->AwbSumB = AaaVar->AwbSumB + awbColSum(isp_info, 2, i);
			}
		}

		tmp = (AaaVar->AwbMode % 5) * 5;		
		for (i = tmp; (i < (tmp + 5)) && (i < AaaVar->AwbCheckBlkCnt); i++)
		{
				awbSetCenter(isp_info, i - tmp, AaaVar->pAwbRRef[i], AaaVar->pAwbBRef[i], AaaVar->AwbRWidth, AaaVar->AwbBWidth);
		}

		AaaVar->AwbMode++;	

		awbFuncExt(isp_info);
	}
}

void aaaInitVar(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	AaaVar->sensor_gain_addr = 0;
	AaaVar->sensor_frame_len_addr = 0;
	AaaVar->sensor_line_total_addr = 0;
	AaaVar->sensor_exp_line_addr = 0;
	AaaVar->sensor_pclk = 0;
	AaaVar->ctExposureTimeAbsoluteCur = CTEXPOSURETIMEABSOLUTECUR;
	AaaVar->AeTest = 0;
	AaaVar->AeExp = 0;
	AaaVar->AeExpGet = 0;
	AaaVar->AeExpLineMode = 0;
	AaaVar->AePreExpLineMode = 0;
	AaaVar->AeLineModeExpLineCount = 0;
	AaaVar->SensorLineCountOut = 0;
	AaaVar->AeCurExpLine = 0;
	AaaVar->SensorMinFrameLen = 0;
	AaaVar->AeGain = 0;
	AaaVar->AeGainGet = 0;
	AaaVar->AeEvConst = 0;
	AaaVar->AeCurEv = 0;
	//AaaVar->pAeGainTbl[256] = 0;
	//AaaVar->pAeExpTbl[256] = 0;
	AaaVar->AeTblCntVal = 0;
	AaaVar->AeExpTableSfAddr = 0;
	AaaVar->AeGainTableSfAddr = 0;
	AaaVar->AeExpLineCount120 = 0;
	AaaVar->SensorExpIn = 0;
	AaaVar->AwbCurColTemp = 0;
	AaaVar->AwbColTemp = 0;
	AaaVar->AwbBGainReg = 0;
	AaaVar->AwbRGainReg = 0;
	AaaVar->AwbGGainReg = 0;
	AaaVar->AwbRawRGain = 0;
	AaaVar->AwbRawGGain = 0;
	AaaVar->AwbRawBGain = 0;
	AaaVar->AwbRGainTarget = 0;
	AaaVar->AwbGGainTarget = 0;
	AaaVar->AwbBGainTarget = 0;
	AaaVar->AwbPreRTarget = 0;
	AaaVar->AwbDeadZone = 0;
	AaaVar->AwbPreBTarget = 0;
	AaaVar->AwbSumCount = 0;
	AaaVar->AwbSumR = 0;
	AaaVar->AwbSumG = 0;
	AaaVar->AwbSumB = 0;
	AaaVar->AeImgSize = 0;
	AaaVar->AeDarkMode = 0;
	AaaVar->AeDeadZone = 0;
	AaaVar->AeDeadLeafWinSize = 0;
	AaaVar->AeDarkThresh0 = 0;
	AaaVar->AeIndex = AEINDEX;
	AaaVar->AeIndexPrev = AEINDEXPREV;
	AaaVar->AeIndexGet = 0;
	AaaVar->AeIndexMax = 0xdd;
	AaaVar->AaaFreq = 0;
	AaaVar->AeIndexMin = 0;
	AaaVar->AeIndexMax60 = AEINDEXMAX60;
	AaaVar->AeIndexMax50 = AEINDEXMAX50;
	AaaVar->AeIndexMin60 = AEINDEXMIN60;
	AaaVar->AeIndexMin50 = AEINDEXMIN50;
	AaaVar->aaaFlag = 0;
	AaaVar->aaaCount = 0;
	AaaVar->AwbMinGain = 0;
	AaaVar->AwbFirstInit = 1;
	AaaVar->AwbMode = 0;

	AaaVar->AwbRWidth = 0;
	AaaVar->AwbBWidth = 0;
	AaaVar->AwbMinGrayDetCnt = 0;
	AaaVar->AwbCheckBlkCnt = 0;
	AaaVar->pAwbRRef = 0;
	AaaVar->pAwbBRef = 0;
	AaaVar->AwbCmAdjCnt = 0;
	AaaVar->pAwbAdjCmColTemp = 0;
	AaaVar->pAwbAdjCmColMat = 0;
	AaaVar->AwbLensAdjCnt = 0;
	AaaVar->pAwbAdjLensColTemp = 0;
	AaaVar->pAwbAdjLensRVal = 0;
	AaaVar->pAwbAdjLensBVal = 0;
	AaaVar->AeAdjYTarget = 0;
	AaaVar->AeYTarget = 0;
	AaaVar->AwbBTargetMin = 0;
	AaaVar->AwbBTargetMax = 0;
	AaaVar->AwbRTargetMin = 0;
	AaaVar->AwbRTargetMax = 0;
	AaaVar->AwbGTargetMin = 0;
	AaaVar->AwbGTargetMax = 0;
	AaaVar->AeYLayer = 0;
	AaaVar->puBacklightCompensationCur = PUBACKLIGHTCOMPENSATIONCUR;
	AaaVar->puBacklightCompensationDef = PUBACKLIGHTCOMPENSATIONDEF;
	AaaVar->pAeNormWeight = 0;
	AaaVar->AeStable = 0;
	AaaVar->AeStill = 0;
}

/*
	aaaAeAwbAf should be called in main loop.
*/
void aaaAeAwbAf(struct mipi_isp_info *isp_info)
{
	aaa_var_t *AaaVar = &isp_info->aaa_var;

	ISP3A_LOGD("%s start\n", __FUNCTION__);

	if(isVideoStart()==1)
	{
		if (AaaVar->aaaFlag == 1)  //will be enable at intrIntr0SensorVsync()
		{
			AaaVar->aaaFlag = 0;
			aeFunc(isp_info, 1);
			awbFunc(isp_info, 1);
		}
	}
}


