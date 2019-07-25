#ifndef __I2C_HAL_H__
#define __I2C_HAL_H__

#include <linux/types.h>
#include <linux/module.h>
#include "reg_i2c.h"


#define I2C_MASTER_NUM    (4)

#define SUPPORT_I2C_GDMA

#define I2C_RETEST  (0)

#ifdef SUPPORT_I2C_GDMA
typedef enum
{	
	I2C_DMA_WRITE_MODE,
	I2C_DMA_READ_MODE,
}I2C_DMA_RW_Mode_e;
#endif

typedef enum I2C_RW_Mode_e_ {
	I2C_WRITE_MODE,
	I2C_READ_MODE,
	I2C_RESTART_MODE,
} I2C_RW_Mode_e;

typedef enum I2C_Active_Mode_e_ {
	I2C_TRIGGER,
	I2C_AUTO,
} I2C_Active_Mode_e;


#ifdef SUPPORT_I2C_GDMA
void hal_i2cm_sg_dma_go_set(unsigned int device_id);
void hal_i2cm_sg_dma_rw_mode_set(unsigned int device_id, I2C_DMA_RW_Mode_e rw_mode);
void hal_i2cm_sg_dma_length_set(unsigned int device_id, unsigned int length);
void hal_i2cm_sg_dma_addr_set(unsigned int device_id, unsigned int addr);
void hal_i2cm_sg_dma_last_lli_set(unsigned int device_id);
void hal_i2cm_sg_dma_access_lli_set(unsigned int device_id, unsigned int access_index);
void hal_i2cm_sg_dma_start_lli_set(unsigned int device_id, unsigned int start_index);
void hal_i2cm_sg_dma_enable(unsigned int device_id);

void hal_i2cm_dma_int_flag_clear(unsigned int device_id, unsigned int flag);
void hal_i2cm_dma_int_flag_get(unsigned int device_id, unsigned int *flag);
void hal_i2cm_dma_int_en_set(unsigned int device_id, unsigned int dma_int);
void hal_i2cm_dma_go_set(unsigned int device_id);
void hal_i2cm_dma_rw_mode_set(unsigned int device_id, I2C_DMA_RW_Mode_e rw_mode);
void hal_i2cm_dma_length_set(unsigned int device_id, unsigned int length);
void hal_i2cm_dma_addr_set(unsigned int device_id, unsigned int addr);
void hal_i2cm_dma_base_set(unsigned int device_id, void __iomem *membase);
void hal_i2cm_dma_mode_disable(unsigned int device_id);
void hal_i2cm_dma_mode_enable(unsigned int device_id);
#endif

#ifdef I2C_RETEST
void hal_i2cm_scl_delay_read(unsigned int device_id, unsigned int *delay);
#endif
void hal_i2cm_scl_delay_set(unsigned int device_id, unsigned int delay);
void hal_i2cm_roverflow_flag_get(unsigned int device_id, unsigned int *flag);
void hal_i2cm_rdata_flag_clear(unsigned int device_id, unsigned int flag);
void hal_i2cm_rdata_flag_get(unsigned int device_id, unsigned int *flag);
void hal_i2cm_status_clear(unsigned int device_id, unsigned int flag);
void hal_i2cm_status_get(unsigned int device_id, unsigned int *flag);
void hal_i2cm_int_en2_set(unsigned int device_id, unsigned int overflow_en);
void hal_i2cm_int_en1_set(unsigned int device_id, unsigned int rdata_en);
void hal_i2cm_int_en0_set(unsigned int device_id, unsigned int int0);
void hal_i2cm_int_en0_disable(unsigned int device_id, unsigned int int0);
void hal_i2cm_int_en0_with_thershold_set(unsigned int device_id, unsigned int int0, unsigned char threshold);
void hal_i2cm_data_get(unsigned int device_id, unsigned int index, unsigned int *rdata);
void hal_i2cm_data_set(unsigned int device_id, unsigned int *wdata);
void hal_i2cm_data0_set(unsigned int device_id, unsigned int *wdata);
void hal_i2cm_rw_mode_set(unsigned int device_id, I2C_RW_Mode_e rw_mode);
void hal_i2cm_manual_trigger(unsigned int device_id);
void hal_i2cm_active_mode_set(unsigned int device_id, I2C_Active_Mode_e mode);
void hal_i2cm_trans_cnt_set(unsigned int device_id, unsigned int write_cnt, unsigned int read_cnt);
void hal_i2cm_slave_addr_set(unsigned int device_id, unsigned int addr);
void hal_i2cm_clock_freq_set(unsigned int device_id, unsigned int freq);
void hal_i2cm_reset(unsigned int device_id);
void hal_i2cm_base_set(unsigned int device_id, void __iomem *membase);

#endif /* __I2C_HAL_H__ */
