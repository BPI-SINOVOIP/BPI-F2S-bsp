#include <common.h>

#include "isp_api.h"
#include "i2c_api.h"

#define I2C_TIMEOUT_TH  0x10000


void Reset_I2C0(void)
{
	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	ISPAPB0_REG8(0x2660) = 0x01;
	ISPAPB0_REG8(0x2660) = 0x00;	
}

void Init_I2C0(u8 DevSlaveAddr, u8 speed_div)
{
	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	ISPAPB0_REG8(0x2790) &= 0xBF;					//clear bit6:sdi gate0
	//ISPAPB0_REG8(0x27F6) |= 0x01;					//enable // for other ISP
	ISPAPB0_REG8(0x2600) = DevSlaveAddr;
	ISPAPB0_REG8(0x2604) = speed_div;				/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
}

// Write data to 16-bit address sensor
void setSensor16_I2C0(u16 addr16, u16 value16, u8 data_count)
{

	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if((addr_count+data_count)>32)return;
	if (data_count == 1)
	{
		regData[0] = (u8)(value16 & 0xFF);
	}
	else if (data_count == 2)
	{
		regData[0] = (u8)(value16 >> 8);
		regData[1] = (u8)(value16 & 0xFF);
	}
	else
	{
		//not support
		ISPI2C_LOGE("%s, no support! (data_count=%d)\n", __FUNCTION__, data_count);
		return;
	}
		
	//ISPAPB0_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB0_REG8(0x2604) = 0x00;					/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
	ISPAPB0_REG8(0x2601) = 0x04;						/* set new transaction write mode */
	ISPAPB0_REG8(0x2603) = 0;                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		ISPAPB0_REG8(0x2608) = 0;						/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2608) = (addr_count+data_count); /* total count bytes to write */
   
	/* data write */
	ISPAPB0_REG8(0x2610) = (u8)(addr16 >> 8);			/* set the high byte of subaddress */
	ISPAPB0_REG8(0x2611) = (u8)(addr16 & 0x00FF);		/* set the low byte of subaddress */
	for (i = 0; i < data_count; i++) {
		ISPAPB0_REG8((u64)(0x2612 + i)) = regData[i];	/* set data */
	}
		
	//ISPAPB0_REG8(0x2610) = addr16_H;   				/* set subaddress */
	//ISPAPB0_REG8(0x2611) = addr16_L;   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	ISPAPB0_REG8(0x2612 + i) = regData[i];			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;					//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB0_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Write data to 8-bit address sensor
void setSensor8_I2C0(u8 addr8, u16 value16)
{

	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if((addr_count+data_count)>32)return;
	regData[0] = (u8)(value16 >> 8);
	regData[1] = (u8)(value16 & 0xFF);
		
	//ISPAPB0_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB0_REG8(0x2604) = 0x00;					/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
	ISPAPB0_REG8(0x2601) = 0x04;						/* set new transaction write mode */
	ISPAPB0_REG8(0x2603) = 0;                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		ISPAPB0_REG8(0x2608) = 0;						/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2608) = (addr_count+data_count); /* total count bytes to write */
   
	/* data write */
	ISPAPB0_REG8(0x2610) = addr8;   					/* set subaddress */
	for (i = 0; i < data_count; i++) {
		ISPAPB0_REG8((u64)(0x2611 + i)) = regData[i];	/* set data */
	}
		
	//ISPAPB0_REG8(0x2610) = addr16_H;   				/* set subaddress */
	//ISPAPB0_REG8(0x2611) = addr16_L;   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	ISPAPB0_REG8(0x2612 + i) = regData[i];			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB0_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Read data from 16-bit address sensor
void getSensor16_I2C0(u16 addr16, u16 *value16, u8 data_count)
{
	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//ISPAPB0_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB0_REG8(0x2604) = 0x00;			/* speed selection */
											/*
												0: 48MHz div 2048
												1: 48MHz div 1024
												2: 48MHz div 512
												3: 48MHz div 256
												4: 48MHz div 128
												5: 48MHz div 64
												6: 48MHz div 32
												7: 48MHz div 16
											*/
	ISPAPB0_REG8(0x2601) = 0x06;                	/* set new transaction READ mode */
	ISPAPB0_REG8(0x2603) = 0;               		/* set synchronization with vd */
	//
	if ((addr_count) == 32)
		ISPAPB0_REG8(0x2608) = 0;					/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2608) = (addr_count); 		/* total count bytes to write */
	//
	if ((data_count) == 32)
		ISPAPB0_REG8(0x2609) = 0;					/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2609) = (data_count); 		/* total count bytes to write */
	//			
	ISPAPB0_REG8(0x2610) = (u8)(addr16 >> 8);		/* set the high byte of subaddress */
	ISPAPB0_REG8(0x2611) = (u8)(addr16 & 0x00FF);	/* set the low byte of subaddress */
	//ISPAPB0_REG8(0x2610) = addr16_H;   			/* set subaddress */
	//ISPAPB0_REG8(0x2611) = addr16_L;   			/* set subaddress */
	ISPAPB0_REG8(0x2602) = 0x13;           			/* set restart enable, subaddress enable , subaddress 16-bit mode and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB0_REG8(0x2650) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = ISPAPB0_REG8((u64)(0x2610 + i));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (ISPAPB0_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}

	// Transfer data from big endian to little endian
	for (i = 0; i < (data_count/2); i++)
	{
		addr_count = regData[(data_count-1)];
		regData[(data_count-1)-i] = regData[i];
		regData[i] = addr_count;
	}

	if (data_count == 1)
		*value16 = *((u8 *)regData);
	else if (data_count == 2)
		*value16 = *((u16 *)regData);
	else
		*value16 = 0;

	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);
	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Read data from 8-bit address sensor
void getSensor8_I2C0(u8 addr8, u16 *value16)
{
	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//ISPAPB0_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB0_REG8(0x2604) = 0x00;			/* speed selection */
											/*
												0: 48MHz div 2048
												1: 48MHz div 1024
												2: 48MHz div 512
												3: 48MHz div 256
												4: 48MHz div 128
												5: 48MHz div 64
												6: 48MHz div 32
												7: 48MHz div 16
											*/
	ISPAPB0_REG8(0x2601) = 0x06;                	/* set new transaction READ mode */
	ISPAPB0_REG8(0x2603) = 0;               		/* set synchronization with vd */
	//
	if ((addr_count) == 32)
		ISPAPB0_REG8(0x2608) = 0;					/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2608) = (addr_count); 		/* total count bytes to write */
	//
	if ((data_count) == 32)
		ISPAPB0_REG8(0x2609) = 0;					/* total 32 bytes to write */
	else
		ISPAPB0_REG8(0x2609) = (data_count); 		/* total count bytes to write */
	//			
	ISPAPB0_REG8(0x2610) = addr8;      				/* set subaddress to write */ 
	//ISPAPB0_REG8(0x2610) = addr16_H;   			/* set subaddress */
	//ISPAPB0_REG8(0x2611) = addr16_L;   			/* set subaddress */
	ISPAPB0_REG8(0x2602) = 0x13;           			/* set restart enable, subaddress enable and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB0_REG8(0x2650) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = ISPAPB0_REG8((u64)(0x2610 + i));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (ISPAPB0_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			ISPAPB0_REG8(0x2660) = 0x01;
			ISPAPB0_REG8(0x2660) = 0x00;
			return;
		}
  	}

	// Transfer data from big endian to little endian
	for (i = 0; i < (data_count/2); i++)
	{
		addr_count = regData[(data_count-1)];
		regData[(data_count-1)-i] = regData[i];
		regData[i] = addr_count;
	}

  	*value16 = *((u16 *)regData);

	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);
	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

void Reset_I2C1(void)
{
	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	ISPAPB1_REG8(0x2660) = 0x01;
	ISPAPB1_REG8(0x2660) = 0x00;	
}

void Init_I2C1(u8 DevSlaveAddr, u8 speed_div)
{
	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	ISPAPB1_REG8(0x2790) &= 0xBF;					//clear bit6:sdi gate0
	//ISPAPB1_REG8(0x27F6) |= 0x01;					//enable // for other ISP
	ISPAPB1_REG8(0x2600) = DevSlaveAddr;
	ISPAPB1_REG8(0x2604) = speed_div;				/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
}

// Write data to 16-bit address sensor
void setSensor16_I2C1(u16 addr16, u16 value16, u8 data_count)
{

	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if((addr_count+data_count)>32)return;
	if (data_count == 1)
	{
		regData[0] = (u8)(value16 & 0xFF);
	}
	else if (data_count == 2)
	{
		regData[0] = (u8)(value16 >> 8);
		regData[1] = (u8)(value16 & 0xFF);
	}
	else
	{
		//not support
		ISPI2C_LOGE("%s, no support! (data_count=%d)\n", __FUNCTION__, data_count);
		return;
	}
		
	//ISPAPB1_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB1_REG8(0x2604) = 0x00;					/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
	ISPAPB1_REG8(0x2601) = 0x04;						/* set new transaction write mode */
	ISPAPB1_REG8(0x2603) = 0;                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		ISPAPB1_REG8(0x2608) = 0;						/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2608) = (addr_count+data_count); /* total count bytes to write */
   
	/* data write */
	ISPAPB1_REG8(0x2610) = (u8)(addr16 >> 8);			/* set the high byte of subaddress */
	ISPAPB1_REG8(0x2611) = (u8)(addr16 & 0x00FF);		/* set the low byte of subaddress */
	for (i = 0; i < data_count; i++) {
		ISPAPB1_REG8((u64)(0x2612 + i)) = regData[i];	/* set data */
	}
		
	//ISPAPB1_REG8(0x2610) = addr16_H;   				/* set subaddress */
	//ISPAPB1_REG8(0x2611) = addr16_L;   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	ISPAPB1_REG8(0x2612 + i) = regData[i];			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;					//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB1_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Write data to 8-bit address sensor
void setSensor8_I2C1(u8 addr8, u16 value16)
{

	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if((addr_count+data_count)>32)return;
	regData[0] = (u8)(value16 >> 8);
	regData[1] = (u8)(value16 & 0xFF);
		
	//ISPAPB1_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB1_REG8(0x2604) = 0x00;					/* speed selection */
													/*
														0: 48MHz div 2048
														1: 48MHz div 1024
														2: 48MHz div 512
														3: 48MHz div 256
														4: 48MHz div 128
														5: 48MHz div 64
														6: 48MHz div 32
														7: 48MHz div 16
													*/
	ISPAPB1_REG8(0x2601) = 0x04;						/* set new transaction write mode */
	ISPAPB1_REG8(0x2603) = 0;                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		ISPAPB1_REG8(0x2608) = 0;						/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2608) = (addr_count+data_count); /* total count bytes to write */
   
	/* data write */
	ISPAPB1_REG8(0x2610) = addr8;   					/* set subaddress */
	for (i = 0; i < data_count; i++) {
		ISPAPB1_REG8((u64)(0x2611 + i)) = regData[i];	/* set data */
	}
		
	//ISPAPB1_REG8(0x2610) = addr16_H;   				/* set subaddress */
	//ISPAPB1_REG8(0x2611) = addr16_L;   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	ISPAPB1_REG8(0x2612 + i) = regData[i];			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB1_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Read data from 16-bit address sensor
void getSensor16_I2C1(u16 addr16, u16 *value16, u8 data_count)
{
	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//ISPAPB1_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB1_REG8(0x2604) = 0x00;			/* speed selection */
											/*
												0: 48MHz div 2048
												1: 48MHz div 1024
												2: 48MHz div 512
												3: 48MHz div 256
												4: 48MHz div 128
												5: 48MHz div 64
												6: 48MHz div 32
												7: 48MHz div 16
											*/
	ISPAPB1_REG8(0x2601) = 0x06;                	/* set new transaction READ mode */
	ISPAPB1_REG8(0x2603) = 0;               		/* set synchronization with vd */
	//
	if ((addr_count) == 32)
		ISPAPB1_REG8(0x2608) = 0;					/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2608) = (addr_count); 		/* total count bytes to write */
	//
	if ((data_count) == 32)
		ISPAPB1_REG8(0x2609) = 0;					/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2609) = (data_count); 		/* total count bytes to write */
	//			
	ISPAPB1_REG8(0x2610) = (u8)(addr16 >> 8);		/* set the high byte of subaddress */
	ISPAPB1_REG8(0x2611) = (u8)(addr16 & 0x00FF);	/* set the low byte of subaddress */
	//ISPAPB1_REG8(0x2610) = addr16_H;   			/* set subaddress */
	//ISPAPB1_REG8(0x2611) = addr16_L;   			/* set subaddress */
	ISPAPB1_REG8(0x2602) = 0x13;           			/* set restart enable, subaddress enable , subaddress 16-bit mode and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB1_REG8(0x2650) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = ISPAPB1_REG8((u64)(0x2610 + i));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (ISPAPB1_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}

	// Transfer data from big endian to little endian
	for (i = 0; i < (data_count/2); i++)
	{
		addr_count = regData[(data_count-1)];
		regData[(data_count-1)-i] = regData[i];
		regData[i] = addr_count;
	}

	if (data_count == 1)
		*value16 = *((u8 *)regData);
	else if (data_count == 2)
		*value16 = *((u16 *)regData);
	else
		*value16 = 0;

	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);
	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Read data from 8-bit address sensor
void getSensor8_I2C1(u8 addr8, u16 *value16)
{
	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//ISPAPB1_REG8(0x2600) = SensorSlaveAddr;
	//ISPAPB1_REG8(0x2604) = 0x00;			/* speed selection */
											/*
												0: 48MHz div 2048
												1: 48MHz div 1024
												2: 48MHz div 512
												3: 48MHz div 256
												4: 48MHz div 128
												5: 48MHz div 64
												6: 48MHz div 32
												7: 48MHz div 16
											*/
	ISPAPB1_REG8(0x2601) = 0x06;                	/* set new transaction READ mode */
	ISPAPB1_REG8(0x2603) = 0;               		/* set synchronization with vd */
	//
	if ((addr_count) == 32)
		ISPAPB1_REG8(0x2608) = 0;					/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2608) = (addr_count); 		/* total count bytes to write */
	//
	if ((data_count) == 32)
		ISPAPB1_REG8(0x2609) = 0;					/* total 32 bytes to write */
	else
		ISPAPB1_REG8(0x2609) = (data_count); 		/* total count bytes to write */
	//			
	ISPAPB1_REG8(0x2610) = addr8;      				/* set subaddress to write */ 
	//ISPAPB1_REG8(0x2610) = addr16_H;   			/* set subaddress */
	//ISPAPB1_REG8(0x2611) = addr16_L;   			/* set subaddress */
	ISPAPB1_REG8(0x2602) = 0x13;           			/* set restart enable, subaddress enable and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (ISPAPB1_REG8(0x2650) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = ISPAPB1_REG8((u64)(0x2610 + i));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (ISPAPB1_REG8(0x2650) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			ISPAPB1_REG8(0x2660) = 0x01;
			ISPAPB1_REG8(0x2660) = 0x00;
			return;
		}
  	}

	// Transfer data from big endian to little endian
	for (i = 0; i < (data_count/2); i++)
	{
		addr_count = regData[(data_count-1)];
		regData[(data_count-1)-i] = regData[i];
		regData[i] = addr_count;
	}

  	*value16 = *((u16 *)regData);

	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);
	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

