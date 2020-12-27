#include <linux/io.h>
#include "isp_api.h"
#include "i2c_api.h"

#define I2C_TIMEOUT_TH  0x10000


void Reset_I2C0(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	writeb(0x01, &(regs->reg[0x2660]));
	writeb(0x00, &(regs->reg[0x2660]));
}

void Init_I2C0(u8 DevSlaveAddr, u8 speed_div, struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 data;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	data = readb(&(regs->reg[0x2790])) & 0xBF;
	writeb(data, &(regs->reg[0x2790]));				//clear bit6:sdi gate0
	//writeb(0x01, &(regs->reg[0x27F6]));					//enable // for other ISP
	writeb(DevSlaveAddr, &(regs->reg[0x2600]));
	writeb(speed_div, &(regs->reg[0x2604]));		/* speed selection */
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
void setSensor16_I2C0(u16 addr16, u16 value16, u8 data_count, struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if ((addr_count+data_count) > 32) return;
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
		
	//writeb(SensorSlaveAddr, &(regs->reg[0x2600]));
	//writeb(0x00, &(regs->reg[0x2604]));					/* speed selection */
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
	writeb(0x04, &(regs->reg[0x2601]));						/* set new transaction write mode */
	writeb(0x00, &(regs->reg[0x2603]));                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		writeb(0x00, &(regs->reg[0x2608]));						/* total 32 bytes to write */
	else
		writeb((addr_count+data_count), &(regs->reg[0x2608])); /* total count bytes to write */
   
	/* data write */
	writeb((u8)(addr16 >> 8), &(regs->reg[0x2610]));			/* set the high byte of subaddress */
	writeb((u8)(addr16 & 0x00FF), &(regs->reg[0x2611]));		/* set the low byte of subaddress */
	for (i = 0; i < data_count; i++) {
		writeb(regData[i], &(regs->reg[0x2612+i]));	/* set data */
	}
		
	//writeb(addr16_H, &(regs->reg[0x2610]));   				/* set subaddress */
	//writeb(addr16_L, &(regs->reg[0x2611]));   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	writeb(regData[i], &(regs->reg[0x2612+i]));			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;					//ULONG SensorI2cTimeOut defined in ROM
	while (readb(&(regs->reg[0x2650])) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Write data to 8-bit address sensor
void setSensor8_I2C0(u8 addr8, u16 value16, struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if((addr_count+data_count)>32)return;
	regData[0] = (u8)(value16 >> 8);
	regData[1] = (u8)(value16 & 0xFF);
		
	//writeb(SensorSlaveAddr, &(regs->reg[0x2600]));
	//writeb(0x00, &(regs->reg[0x2604]));					/* speed selection */
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
	writeb(0x04, &(regs->reg[0x2601]));						/* set new transaction write mode */
	writeb(0x00, &(regs->reg[0x2603]));                  			/* set synchronization with vd */
	if ((addr_count+data_count) == 32)
		writeb(0x00, &(regs->reg[0x2608]));						/* total 32 bytes to write */
	else
		writeb((addr_count+data_count), &(regs->reg[0x2608])); /* total count bytes to write */
   
	/* data write */
	writeb(addr8, &(regs->reg[0x2610]));   					/* set subaddress */
	for (i = 0; i < data_count; i++) {
		writeb(regData[i], &(regs->reg[0x2611 + i]));	/* set data */
	}
		
	//writeb(addr16_H, &(regs->reg[0x2610]));   				/* set subaddress */
	//writeb(addr16_L, &(regs->reg[0x2611]));   				/* set subaddress */
	//for (i = 0; i < data_count; i++)
	//	writeb(regData[i], &(regs->reg[0x2612 + i]));			/* set data */	

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (readb(&(regs->reg[0x2650])) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
			return;
		}
  	}	
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	ISPI2C_LOGI("%s, Done\n", __FUNCTION__);
}

// Read data from 16-bit address sensor
void getSensor16_I2C0(u16 addr16, u16 *value16, u8 data_count, struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 i;
	//u8 addr_count = 0x02, data_count = 0x02;
	u8 addr_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//writeb(, &(regs->reg[0x2600) = SensorSlaveAddr;
	//writeb(, &(regs->reg[0x2604) = 0x00;			/* speed selection */
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
	writeb(0x06, &(regs->reg[0x2601]));                	/* set new transaction READ mode */
	writeb(0x00, &(regs->reg[0x2603]));               		/* set synchronization with vd */
	//
	if (addr_count == 32)
		writeb(0x00, &(regs->reg[0x2608]));					/* total 32 bytes to write */
	else
		writeb(addr_count, &(regs->reg[0x2608])); 		/* total count bytes to write */
	//
	if (data_count == 32)
		writeb(0x00, &(regs->reg[0x2609]));					/* total 32 bytes to write */
	else
		writeb(data_count, &(regs->reg[0x2609])); 		/* total count bytes to write */
	//			
	writeb((u8)(addr16 >> 8), &(regs->reg[0x2610]));		/* set the high byte of subaddress */
	writeb((u8)(addr16 & 0x00FF), &(regs->reg[0x2611]));	/* set the low byte of subaddress */
	//writeb(addr16_H, &(regs->reg[0x2610]));   			/* set subaddress */
	//writeb(addr16_L, &(regs->reg[0x2611]));   			/* set subaddress */
	writeb(0x13, &(regs->reg[0x2602]));           			/* set restart enable, subaddress enable , subaddress 16-bit mode and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (readb(&(regs->reg[0x2650])) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = readb(&(regs->reg[0x2610 + i]));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (readb(&(regs->reg[0x2650])) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
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
void getSensor8_I2C0(u8 addr8, u16 *value16, struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 i;
	u8 addr_count = 0x01, data_count = 0x02;
	u8 regData[2];
	u32 SensorI2cTimeOut;

	ISPI2C_LOGI("%s, start\n", __FUNCTION__);

	if (addr_count>32 || data_count>32)return;
	//writeb(SensorSlaveAddr, &(regs->reg[0x2600]));
	//writeb(0x00, &(regs->reg[0x2604]));			/* speed selection */
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
	writeb(0x06, &(regs->reg[0x2601]));                	/* set new transaction READ mode */
	writeb(0x00, &(regs->reg[0x2603]));               		/* set synchronization with vd */
	//
	if (addr_count == 32)
		writeb(0x00, &(regs->reg[0x2608]));					/* total 32 bytes to write */
	else
		writeb(addr_count, &(regs->reg[0x2608])); 		/* total count bytes to write */
	//
	if ((data_count) == 32)
		writeb(0x00, &(regs->reg[0x2609]));					/* total 32 bytes to write */
	else
		writeb(data_count, &(regs->reg[0x2609])); 		/* total count bytes to write */
	//			
	writeb(addr8, &(regs->reg[0x2610]));      				/* set subaddress to write */ 
	//writeb(addr16_H, &(regs->reg[0x2610]));   			/* set subaddress */
	//writeb(addr16_L, &(regs->reg[0x2611]));   			/* set subaddress */
	writeb(0x13, &(regs->reg[0x2602]));           			/* set restart enable, subaddress enable and prefetech */ 

	SensorI2cTimeOut = I2C_TIMEOUT_TH;				//ULONG SensorI2cTimeOut defined in ROM
	while (readb(&(regs->reg[0x2650])) == 0x01)			/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			ISPI2C_LOGE("%s, I2C Timeout!\n", __FUNCTION__);

			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
			return;
		}
  	}
	ISPI2C_LOGD("%s, I2C time: %d\n", __FUNCTION__, I2C_TIMEOUT_TH-SensorI2cTimeOut);

	/* data read */
	for (i = 0; i < data_count; i++)
	{   
		regData[i] = readb(&(regs->reg[0x2610 + i]));	/* get data */
	}

	SensorI2cTimeOut = I2C_TIMEOUT_TH;
	while (readb(&(regs->reg[0x2650])) == 0x01)				/* busy waiting until serial interface not busy */  
	{
		SensorI2cTimeOut--;
		if (SensorI2cTimeOut == 0)
		{
			/* reset front i2c interface */                            
			writeb(0x01, &(regs->reg[0x2660]));
			writeb(0x00, &(regs->reg[0x2660]));
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

