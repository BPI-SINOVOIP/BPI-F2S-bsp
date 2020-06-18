#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include "hal_mipi_i2c.h"

/* Message Definition */
#define HAL_I2C_FUNC_DEBUG
#define HAL_I2C_FUNC_INFO
#define HAL_I2C_FUNC_ERR

#ifdef HAL_I2C_FUNC_DEBUG
	#define HAL_I2C_DEBUG()    printk(KERN_INFO "[HAL I2C] DBG: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define HAL_I2C_DEBUG()
#endif
#ifdef HAL_I2C_FUNC_INFO
	#define HAL_I2C_INFO(fmt, args ...)    printk(KERN_INFO "[HAL I2C] INFO: " fmt, ## args)
#else
	#define HAL_I2C_INFO(fmt, args ...)
#endif
#ifdef HAL_I2C_FUNC_ERR
	#define HAL_I2C_ERR(fmt, args ...)    printk(KERN_ERR "[HAL I2C] ERR: " fmt, ## args)
#else
	#define HAL_I2C_ERR(fmt, args ...)
#endif

static regs_mipi_i2c_t *pMipiI2cReg[MIPI_I2C_NUM];


void hal_mipi_i2c_status_get(unsigned int device_id, unsigned char *status)
{
	*status = readb(&(pMipiI2cReg[device_id]->mipi_i2c[0x50]));
}
EXPORT_SYMBOL(hal_mipi_i2c_status_get);

void hal_mipi_i2c_data_get(unsigned int device_id, unsigned char *rdata, unsigned int read_cnt)
{
	unsigned int i = 0;

	if (device_id < MIPI_I2C_NUM) {
		for (i = 0; i < read_cnt; i++) {
			rdata[i] = readb(&(pMipiI2cReg[device_id]->mipi_i2c[0x10+i]));
		}
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_data_get);

void hal_mipi_i2c_data_set(unsigned int device_id, unsigned char *wdata, unsigned int write_cnt)
{
	unsigned int i = 0;

	if (device_id < MIPI_I2C_NUM) {
		for (i = 0; i < write_cnt; i++) {
			writeb(wdata[i], &(pMipiI2cReg[device_id]->mipi_i2c[0x10+i]));
		}
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_data_set);

void hal_mipi_i2c_read_trigger(unsigned int device_id, mipi_i2c_start_mode start_mode)
{
	unsigned char reg_2602 = 0;

	if (device_id < MIPI_I2C_NUM) {
		switch(start_mode) {
			default:
			case I2C_START:
				reg_2602 = MIPI_I2C_2602_SUBADDR_EN | MIPI_I2C_2602_PREFETCH;
				break;

			case I2C_RESTART:
				reg_2602 = MIPI_I2C_2602_SUBADDR_EN | MIPI_I2C_2602_RESTART_EN | MIPI_I2C_2602_PREFETCH;
				break;
		}

		writeb(reg_2602, &(pMipiI2cReg[device_id]->mipi_i2c[0x02]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_read_trigger);


void hal_mipi_i2c_rw_mode_set(unsigned int device_id, mipi_i2c_action rw_mode)
{
	unsigned char reg_2601 = 0;

	if (device_id < MIPI_I2C_NUM) {
		switch(rw_mode) {
			default:
			case I2C_NOR_NEW_WRITE:
				reg_2601 = MIPI_I2C_2601_NEW_TRANS_EN;
				break;

			case I2C_NOR_NEW_READ:
				reg_2601 = MIPI_I2C_2601_NEW_TRANS_EN | MIPI_I2C_2601_NEW_TRANS_MODE;
				break;
		}

		writeb(reg_2601, &(pMipiI2cReg[device_id]->mipi_i2c[0x01]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_rw_mode_set);

void hal_mipi_i2c_active_mode_set(unsigned int device_id, unsigned char mode)
{
	unsigned char reg_2603 = mode;

	if (device_id < MIPI_I2C_NUM) {
		writeb(reg_2603, &(pMipiI2cReg[device_id]->mipi_i2c[0x03]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_active_mode_set);

void hal_mipi_i2c_trans_cnt_set(unsigned int device_id, unsigned int write_cnt, unsigned int read_cnt)
{
	unsigned char reg_2608 = write_cnt;
	unsigned char reg_2609 = read_cnt;

	if (device_id < MIPI_I2C_NUM) {
		if (reg_2608 > 31) reg_2608 = 0;    // 0 for 32 bytes to write
		if (reg_2609 > 31) reg_2609 = 0;    // 0 for 32 bytes to read

		writeb(reg_2608, &(pMipiI2cReg[device_id]->mipi_i2c[0x08]));
		writeb(reg_2609, &(pMipiI2cReg[device_id]->mipi_i2c[0x09]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_trans_cnt_set);

void hal_mipi_i2c_slave_addr_set(unsigned int device_id, unsigned int addr)
{
	unsigned char reg_2600 = addr;

	if (device_id < MIPI_I2C_NUM) {
		writeb(reg_2600, &(pMipiI2cReg[device_id]->mipi_i2c[0x00]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_slave_addr_set);

void hal_mipi_i2c_clock_freq_set(unsigned int device_id, unsigned int freq)
{
	unsigned char reg_2604 = freq;

	if (device_id < MIPI_I2C_NUM) {
		writeb(reg_2604, &(pMipiI2cReg[device_id]->mipi_i2c[0x04]));
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_clock_freq_set);

void hal_mipi_i2c_reset(unsigned int device_id)
{
	unsigned char reg_2660;

	HAL_I2C_DEBUG();

	if (device_id < MIPI_I2C_NUM) {
		HAL_I2C_INFO("device_id:%01d, reg base:0x%px", device_id, pMipiI2cReg[device_id]);

		reg_2660 = readb(&(pMipiI2cReg[device_id]->mipi_i2c[0x60]));
		reg_2660 |= MIPI_I2C_2660_RST_SIF;
		writeb(reg_2660, &(pMipiI2cReg[device_id]->mipi_i2c[0x60]));
		HAL_I2C_INFO("Enable SIF reset. reg_2660:0x%02x", readb(&(pMipiI2cReg[device_id]->mipi_i2c[0x60])));
		udelay(2);
		reg_2660 &= ~MIPI_I2C_2660_RST_SIF;
		writeb(reg_2660, &(pMipiI2cReg[device_id]->mipi_i2c[0x60]));
		HAL_I2C_INFO("Disable SIF reset. reg_2660:0x%02x", readb(&(pMipiI2cReg[device_id]->mipi_i2c[0x60])));
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", device_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_reset);

void hal_mipi_i2c_base_set(unsigned int device_id, void __iomem *membase)
{
	HAL_I2C_DEBUG();

	if (device_id < MIPI_I2C_NUM) {
		HAL_I2C_INFO("device_id:%01d, reg base:0x%px", device_id, membase);

		pMipiI2cReg[device_id] = (regs_mipi_i2c_t *)membase;
	} else {
		HAL_I2C_ERR("wrong device id! device_id:%01d", device_id);
	}
}
EXPORT_SYMBOL(hal_mipi_i2c_base_set);
