#ifndef __I2C_HAL_H__
#define __I2C_HAL_H__

#include <linux/types.h>
#include <linux/module.h>
#include "reg_mipi_i2c.h"

#define MIPI_I2C_NUM    (2)

typedef enum mipi_i2c_action_e {
	I2C_NOR_NEW_READ,
	I2C_NOR_NEW_WRITE,
	I2C_BUR_NEW_READ,
	I2C_BUR_NEW_WRITE,
} mipi_i2c_action;

typedef enum mipi_i2c_start_mode_e {
	I2C_START,
	I2C_RESTART,
} mipi_i2c_start_mode;

void hal_mipi_i2c_status_get(unsigned int device_id, unsigned char *status);
void hal_mipi_i2c_data_get(unsigned int device_id, unsigned char *rdata, unsigned int read_cnt);
void hal_mipi_i2c_data_set(unsigned int device_id, unsigned char *wdata, unsigned int write_cnt);
void hal_mipi_i2c_read_trigger(unsigned int device_id, mipi_i2c_start_mode start_mode);
void hal_mipi_i2c_rw_mode_set(unsigned int device_id, mipi_i2c_action rw_mode);
void hal_mipi_i2c_active_mode_set(unsigned int device_id, unsigned char mode);
void hal_mipi_i2c_trans_cnt_set(unsigned int device_id, unsigned int write_cnt, unsigned int read_cnt);
void hal_mipi_i2c_slave_addr_set(unsigned int device_id, unsigned int addr);
void hal_mipi_i2c_clock_freq_set(unsigned int device_id, unsigned int freq);
void hal_mipi_i2c_reset(unsigned int device_id);
void hal_mipi_i2c_base_set(unsigned int device_id, void __iomem *membase);

#endif /* __I2C_HAL_H__ */
