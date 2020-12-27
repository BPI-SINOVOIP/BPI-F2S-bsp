#ifndef __I2C_HAL_H__
#define __I2C_HAL_H__

#include <linux/types.h>
#include <linux/module.h>
#include "reg_mipi_i2c.h"

#define MIPI_I2C_NUM    (2)
#define MIPI_I2C_1ST_CH (4)
#define MIPI_I2C_MAX_TH (MIPI_I2C_1ST_CH + MIPI_I2C_NUM)

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

typedef struct hal_mipi_i2c_info {
	// Register base address
	mipi_isp_reg_t  *isp_regs;

	// I2C infomation
	unsigned int    dev_id;
} hal_mipi_i2c_info_t;

void hal_mipi_i2c_isp_reset(hal_mipi_i2c_info_t *i2c_info);
void hal_mipi_i2c_init(hal_mipi_i2c_info_t *i2c_info);
void hal_mipi_i2c_power_on(hal_mipi_i2c_info_t *i2c_info);
void hal_mipi_i2c_power_down(hal_mipi_i2c_info_t *i2c_info);
void hal_mipi_i2c_status_get(hal_mipi_i2c_info_t *i2c_info, unsigned char *status);
void hal_mipi_i2c_data_get(hal_mipi_i2c_info_t *i2c_info, unsigned char *rdata, unsigned int read_cnt);
void hal_mipi_i2c_data_set(hal_mipi_i2c_info_t *i2c_info, unsigned char *wdata, unsigned int write_cnt);
void hal_mipi_i2c_read_trigger(hal_mipi_i2c_info_t *i2c_info, mipi_i2c_start_mode start_mode);
void hal_mipi_i2c_rw_mode_set(hal_mipi_i2c_info_t *i2c_info, mipi_i2c_action rw_mode);
void hal_mipi_i2c_active_mode_set(hal_mipi_i2c_info_t *i2c_info, unsigned char mode);
void hal_mipi_i2c_trans_cnt_set(hal_mipi_i2c_info_t *i2c_info, unsigned int write_cnt, unsigned int read_cnt);
void hal_mipi_i2c_slave_addr_set(hal_mipi_i2c_info_t *i2c_info, unsigned int addr);
void hal_mipi_i2c_clock_freq_set(hal_mipi_i2c_info_t *i2c_info, unsigned int freq);
void hal_mipi_i2c_reset(hal_mipi_i2c_info_t *i2c_info);

#endif /* __I2C_HAL_H__ */
