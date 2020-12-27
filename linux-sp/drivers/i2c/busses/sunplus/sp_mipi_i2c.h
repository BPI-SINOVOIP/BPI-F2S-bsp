#ifndef __SP_I2C_H__
#define __SP_I2C_H__

#include "hal_mipi_i2c.h"

#define TIMEOUT_TH  10  // 1 seconds

typedef enum mipi_i2c_state {
	MIPI_I2C_SUCCESS,               /* successful */
	MIPI_I2C_ERR_I2C_BUSY,          /* I2C is busy */
	MIPI_I2C_ERR_INVALID_DEVID,     /* device id is invalid */
	MIPI_I2C_ERR_INVALID_CNT,       /* read or write count is invalid */
	MIPI_I2C_ERR_TIMEOUT_OUT,       /* wait timeout */
} mipi_i2c_state_t;

typedef struct mipi_i2c_cmd {
	hal_mipi_i2c_info_t dI2CInfo;
	unsigned int dFreq;
	unsigned int dSlaveAddr;
	unsigned int dRestartEn;
	unsigned int dWrDataCnt;
	unsigned int dRdDataCnt;
	unsigned char *pWrData;
	unsigned char *pRdData;
} mipi_i2c_cmd_t;

#endif /* __SP_I2C_H__ */
