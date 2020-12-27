#ifndef __SP_I2C_H__
#define __SP_I2C_H__

#include "hal_i2c.h"

typedef enum I2C_Status_e_ {
	I2C_SUCCESS,                /* successful */
	I2C_ERR_I2C_BUSY,           /* I2C is busy */
	I2C_ERR_INVALID_DEVID,      /* device id is invalid */
	I2C_ERR_INVALID_CNT,        /* read or write count is invalid */
	I2C_ERR_TIMEOUT_OUT,        /* wait timeout */
	I2C_ERR_RECEIVE_NACK,       /* receive NACK */
	I2C_ERR_FIFO_EMPTY,         /* FIFO empty */
	I2C_ERR_SCL_HOLD_TOO_LONG,  /* SCL hlod too long */
	I2C_ERR_RDATA_OVERFLOW,     /* rdata overflow */
	I2C_ERR_INVALID_STATE,      /* read write state is invalid */
	I2C_ERR_REQUESET_IRQ,       /* request irq failed */
} I2C_Status_e;

typedef enum I2C_State_e_ {
	I2C_WRITE_STATE,  /* i2c is write */
	I2C_READ_STATE,   /* i2c is read */
	I2C_IDLE_STATE,   /* i2c is idle */
#ifdef SUPPORT_I2C_GDMA
	I2C_DMA_WRITE_STATE,/* i2c is dma write */
	I2C_DMA_READ_STATE, /* i2c is dma read */
#endif
} I2C_State_e;

typedef struct I2C_Cmd_t_ {
	unsigned int dDevId;
	unsigned int dFreq;
	unsigned int dSlaveAddr;
	unsigned int dRestartEn;
	unsigned int dWrDataCnt;
	unsigned int dRdDataCnt;
	unsigned char *pWrData;
	unsigned char *pRdData;
} I2C_Cmd_t;

typedef struct I2C_Irq_Dma_Flag_t_
{
	unsigned char bDmaDone;
	unsigned char bWCntError;
	unsigned char bWBEnError;
	unsigned char bGDmaTimeout;
	unsigned char bIPTimeout;
	unsigned char bThreshold;
	unsigned char bLength0;
}I2C_Irq_Dma_Flag_t;

typedef struct I2C_Irq_Flag_t_ {
	unsigned char bActiveDone;
	unsigned char bAddrNack;
	unsigned char bDataNack;
	unsigned char bEmptyThreshold;
	unsigned char bFiFoEmpty;
	unsigned char bFiFoFull;
	unsigned char bSCLHoldTooLong;
	unsigned char bRdOverflow;
} I2C_Irq_Flag_t;

typedef struct I2C_Irq_Event_t_ {
	I2C_State_e eRWState;
	I2C_Irq_Flag_t stIrqFlag;
	I2C_Irq_Dma_Flag_t stIrqDmaFlag;
	unsigned int dDevId;
	unsigned int dBurstCount;
	unsigned int dBurstRemainder;
	unsigned int dDataIndex;
	unsigned int dDataTotalLen;
	unsigned int dRegDataIndex;
	unsigned char bI2CBusy;
	unsigned char bRet;
	unsigned char *pDataBuf;
} I2C_Irq_Event_t;

#endif /* __SP_I2C_H__ */
