#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include "hal_mipi_i2c.h"

/* Message Definition */
//#define HAL_I2C_FUNC_DEBUG
#define HAL_I2C_FUNC_INFO
#define HAL_I2C_FUNC_ERR

#ifdef HAL_I2C_FUNC_DEBUG
	#define HAL_I2C_DBG(fmt, args ...)      printk(KERN_INFO "[HAL I2C] DBG: " fmt, ## args)
#else
	#define HAL_I2C_DBG(fmt, args ...)
#endif
#ifdef HAL_I2C_FUNC_INFO
	#define HAL_I2C_INFO(fmt, args ...)     printk(KERN_INFO "[HAL I2C] INFO: " fmt, ## args)
#else
	#define HAL_I2C_INFO(fmt, args ...)
#endif
#ifdef HAL_I2C_FUNC_ERR
	#define HAL_I2C_ERR(fmt, args ...)      printk(KERN_ERR "[HAL I2C] ERR: " fmt, ## args)
#else
	#define HAL_I2C_ERR(fmt, args ...)
#endif

#define I2C_TIMEOUT_TH  0x10000


/*
	@ispSleep_i2c this function depends on O.S.
*/
void ispSleep_i2c(int delay)
{
	#define ISP_DELAY_TIMEBASE  21  // 20.83 ns
	u64 time;

	// Calculate how many time to delay in ns
	time = delay * ISP_DELAY_TIMEBASE;
	ndelay(time);
	HAL_I2C_DBG("%s, delay %lld ns\n", __FUNCTION__, time);
}

/*
	@hal_mipi_i2c_isp_reset
*/
void hal_mipi_i2c_isp_reset(hal_mipi_i2c_info_t *i2c_info)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	writeb(0x13, &(regs->reg[0x2000])); //reset all module include ispck
	writeb(0x1c, &(regs->reg[0x2003])); //enable phase clocks
	writeb(0x07, &(regs->reg[0x2005])); //enbale p1xck
	writeb(0x05, &(regs->reg[0x2008])); //switch b1xck/bpclk_nx to normal clocks
	writeb(0x03, &(regs->reg[0x2000])); //release ispck reset
	ispSleep_i2c(20);                     //#(`TIMEBASE*20;
	//
	writeb(0x00, &(regs->reg[0x2000])); //release all module reset
	//
	writeb(0x01, &(regs->reg[0x276c])); //reset front
	writeb(0x00, &(regs->reg[0x276c])); //
	//	
	writeb(0x03, &(regs->reg[0x2000]));
	writeb(0x00, &(regs->reg[0x2000]));
	//
	writeb(0x00, &(regs->reg[0x2010])); //cclk: 48MHz
}
EXPORT_SYMBOL(hal_mipi_i2c_isp_reset);

void hal_mipi_i2c_power_on(hal_mipi_i2c_info_t *i2c_info)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	u8 data;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	/* enable sensor mclk and i2c sck */
	writeb(0x00, &(regs->reg[0x2781]));
	writeb(0x08, &(regs->reg[0x2785]));

	data = readb(&(regs->reg[0x2042])) | 0x03;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[8,9] output enable */
	data = readb(&(regs->reg[0x2044])) | 0x03;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8,9] output high - power up */

	mdelay(1);

	//writeb(0xFD, &(regs->reg[0x2042]));
	//writeb(0xFD, &(regs->reg[0x2044]));
	data = readb(&(regs->reg[0x2044])) & (~0x1);
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8] output low - reset */

	mdelay(1);

	data = readb(&(regs->reg[0x2044])) | 0x03;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8,9] output high - power up */
}
EXPORT_SYMBOL(hal_mipi_i2c_power_on);

void hal_mipi_i2c_power_down(hal_mipi_i2c_info_t *i2c_info)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	u8 data;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	/* disable sensor mclk and i2c sck */
	//writeb(0x48, &(regs->reg[0x2781]));
	writeb(0x00, &(regs->reg[0x2785]));
	udelay(6);                              /* delay 128 extclk = 6 us */

	/* xgpio[8) - 0: reset, 1: normal */
	data = readb(&(regs->reg[0x2042])) | 0x01;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[8] output enable */
	data = readb(&(regs->reg[0x2044])) & (~0x1);
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8] output low - reset */

	/* xgpio[9) - 0: power down, 1: power up */
	data = readb(&(regs->reg[0x2042])) | 0x02;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[9] output enable */
	data = readb(&(regs->reg[0x2044])) & 0xFD;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[9] output low - power down */
}
EXPORT_SYMBOL(hal_mipi_i2c_power_down);

void hal_mipi_i2c_init(hal_mipi_i2c_info_t *i2c_info)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	u8 data;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	data = readb(&(regs->reg[0x2790])) & 0xBF;
	writeb(data, &(regs->reg[0x2790]));				//clear bit6:sdi gate0

	//writeb(0x01, &(regs->reg[0x27F6]));			//enable // for other ISP
	//writeb(DevSlaveAddr, &(regs->reg[0x2600]));
	//writeb(speed_div, &(regs->reg[0x2604]));		/* speed selection */
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

void hal_mipi_i2c_status_get(hal_mipi_i2c_info_t *i2c_info, unsigned char *status)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		*status = readb(&(regs->reg[0x2650]));
		HAL_I2C_DBG("%s, 2650=0x%02x\n", __FUNCTION__, *status);

		//while (readb(&(regs->reg[0x2650])) == 0x01)			/* busy waiting until serial interface not busy */	
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_status_get);

void hal_mipi_i2c_data_get(hal_mipi_i2c_info_t *i2c_info, unsigned char *rdata, unsigned int read_cnt)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned int i = 0;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		for (i = 0; i < read_cnt; i++) {
			rdata[i] = readb(&(regs->reg[0x2610+i]));
			HAL_I2C_DBG("%s, i=%d, rdata[i]=0x%02x\n", __FUNCTION__, i, rdata[i]);
		}
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_data_get);

void hal_mipi_i2c_data_set(hal_mipi_i2c_info_t *i2c_info, unsigned char *wdata, unsigned int write_cnt)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned int i = 0;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		for (i = 0; i < write_cnt; i++) {
			writeb(wdata[i], &(regs->reg[0x2610+i]));
		}

		//writeb(addr16_H, &(regs->reg[0x2610]));					/* set subaddress */
		//writeb(addr16_L, &(regs->reg[0x2611]));					/* set subaddress */
		//for (i = 0; i < data_count; i++)
		//	writeb(regData[i], &(regs->reg[0x2612+i])); 		/* set data */	
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_data_set);

void hal_mipi_i2c_read_trigger(hal_mipi_i2c_info_t *i2c_info, mipi_i2c_start_mode start_mode)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned char reg_2602 = 0;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		switch(start_mode) {
			default:
			case I2C_START:
				reg_2602 = MIPI_I2C_2602_SUBADDR_EN | MIPI_I2C_2602_PREFETCH;
				break;

			case I2C_RESTART:
				reg_2602 = MIPI_I2C_2602_SUBADDR_EN | MIPI_I2C_2602_RESTART_EN | MIPI_I2C_2602_PREFETCH;
				break;
		}

		writeb(reg_2602, &(regs->reg[0x2602]));

		//writeb(0x13, &(regs->reg[0x2602]));           			/* set restart enable, subaddress enable , subaddress 16-bit mode and prefetech */ 
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_read_trigger);


void hal_mipi_i2c_rw_mode_set(hal_mipi_i2c_info_t *i2c_info, mipi_i2c_action rw_mode)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned char reg_2601 = 0;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		switch(rw_mode) {
			default:
			case I2C_NOR_NEW_WRITE:
				reg_2601 = MIPI_I2C_2601_NEW_TRANS_EN;
				break;

			case I2C_NOR_NEW_READ:
				reg_2601 = MIPI_I2C_2601_NEW_TRANS_EN | MIPI_I2C_2601_NEW_TRANS_MODE;
				break;
		}
		writeb(reg_2601, &(regs->reg[0x2601]));	

		//writeb(0x04, &(regs->reg[0x2601])); 					/* set new transaction write mode */
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_rw_mode_set);

void hal_mipi_i2c_active_mode_set(hal_mipi_i2c_info_t *i2c_info, unsigned char mode)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned char reg_2603 = mode;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		writeb(reg_2603, &(regs->reg[0x2603]));

		//writeb(0x00, &(regs->reg[0x2603])); 					/* set synchronization with vd */
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_active_mode_set);

void hal_mipi_i2c_trans_cnt_set(hal_mipi_i2c_info_t *i2c_info, unsigned int write_cnt, unsigned int read_cnt)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned char reg_2608 = write_cnt;
	unsigned char reg_2609 = read_cnt;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		if (reg_2608 > 31) reg_2608 = 0;    // 0 for 32 bytes to write
		if (reg_2608 != 0xff) 
			writeb(reg_2608, &(regs->reg[0x2608]));

		if (reg_2609 > 31) reg_2609 = 0;    // 0 for 32 bytes to read
		if (reg_2609 != 0xff) 
			writeb(reg_2609, &(regs->reg[0x2609]));

		//if ((addr_count+data_count) == 32)
		//	writeb(0x00, &(regs->reg[0x2608])); 					/* total 32 bytes to write */
		//else
		//	writeb((addr_count+data_count), &(regs->reg[0x2608])); /* total count bytes to write */
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_trans_cnt_set);

void hal_mipi_i2c_slave_addr_set(hal_mipi_i2c_info_t *i2c_info, unsigned int addr)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;
	unsigned int reg_2600 = 0;

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		if (addr > 0x7F) {
			HAL_I2C_ERR("wrong slave address! addr:0x%04x", addr);
			return;
		}

		reg_2600 = addr << 1;   // Need to shift slave address by software 
		writeb((u8)reg_2600, &(regs->reg[0x2600]));

		reg_2600 = 0;
		reg_2600 = readb(&(regs->reg[0x2600]));
		HAL_I2C_DBG("%s, reg_2600=0x%02x\n", __FUNCTION__, reg_2600);
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_slave_addr_set);

void hal_mipi_i2c_clock_freq_set(hal_mipi_i2c_info_t *i2c_info, unsigned int freq)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		writeb((u8)freq, &(regs->reg[0x2604]));

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
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_clock_freq_set);

void hal_mipi_i2c_reset(hal_mipi_i2c_info_t *i2c_info)
{
	mipi_isp_reg_t *regs = i2c_info->isp_regs;
	unsigned int dev_id = i2c_info->dev_id;

	HAL_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((dev_id >= MIPI_I2C_1ST_CH) && (dev_id < MIPI_I2C_MAX_TH)) {
		HAL_I2C_INFO("%s, device_id:%01d, reg base:0x%px", __FUNCTION__, dev_id, i2c_info->isp_regs);

		writeb(0x01, &(regs->reg[0x2660]));
		writeb(0x00, &(regs->reg[0x2660]));
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", dev_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_reset);
