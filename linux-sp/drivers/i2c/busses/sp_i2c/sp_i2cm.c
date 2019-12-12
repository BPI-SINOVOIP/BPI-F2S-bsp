#include <linux/delay.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/rtc.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/clk.h>   
#include <linux/reset.h> 
#include "hal_i2c.h"
#include "sp_i2c.h"

#ifdef SUPPORT_I2C_GDMA
#include <linux/dma-mapping.h>   
#endif
#ifdef CONFIG_PM_RUNTIME_I2C
#include <linux/pm_runtime.h>
#endif

//#define I2C_FUNC_DEBUG
//#define I2C_DBG_INFO
//#define I2C_DBG_ERR

#ifdef I2C_FUNC_DEBUG
	#define FUNC_DEBUG()    printk(KERN_INFO "[I2C] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef I2C_DBG_INFO
	#define DBG_INFO(fmt, args ...)    printk(KERN_INFO "[I2C] Info: " fmt, ## args)
#else
	#define DBG_INFO(fmt, args ...)
#endif

#ifdef I2C_DBG_ERR
	#define DBG_ERR(fmt, args ...)    printk(KERN_ERR "[I2C] Error: " fmt, ## args)
#else
	#define DBG_ERR(fmt, args ...)
#endif

#define I2C_FREQ             400
#define I2C_SLEEP_TIMEOUT    200
#define I2C_SCL_DELAY        1  //SCl dalay xT

#define I2CM_REG_NAME        "i2cm"

#define DEVICE_NAME          "sp7021-i2cm"
#ifdef SUPPORT_I2C_GDMA
#define I2CM_DMA_REG_NAME    "i2cmdma"
#endif

//burst write use
#define I2C_EMPTY_THRESHOLD_VALUE    4

//burst read use
#define I2C_IS_READ16BYTE

#ifdef I2C_IS_READ16BYTE
#define I2C_BURST_RDATA_BYTES        16
#define I2C_BURST_RDATA_FLAG         0x80008000
#else
#define I2C_BURST_RDATA_BYTES        4
#define I2C_BURST_RDATA_FLAG         0x88888888
#endif

#define I2C_BURST_RDATA_ALL_FLAG     0xFFFFFFFF

static unsigned bufsiz = 4096;

typedef struct SpI2C_If_t_ {
	struct i2c_msg *msgs;  /* messages currently handled */
	struct i2c_adapter adap;
	I2C_Cmd_t stCmdInfo;
	void __iomem *i2c_regs;
#ifdef SUPPORT_I2C_GDMA
	void __iomem *i2c_dma_regs;
	dma_addr_t dma_phy_base;   
	void * dma_vir_base;
#endif
	struct clk *clk;
	struct reset_control *rstc;
	unsigned int i2c_clk_freq;
	int irq;
} SpI2C_If_t;

static SpI2C_If_t stSpI2CInfo[I2C_MASTER_NUM];
static I2C_Irq_Event_t stIrqEvent[I2C_MASTER_NUM];
wait_queue_head_t i2cm_event_wait[I2C_MASTER_NUM];


#ifdef I2C_RETEST
int test_count;
#endif


unsigned char restart_w_data[32] = {0};
unsigned int  restart_write_cnt = 0;
unsigned int  restart_en = 0;



static void _sp_i2cm_intflag_check(unsigned int device_id, I2C_Irq_Event_t *pstIrqEvent)
{
	unsigned int int_flag = 0;
	unsigned int overflow_flag = 0;
	#ifdef I2C_RETEST
	unsigned int scl_belay = 0;
       #endif

	hal_i2cm_status_get(device_id, &int_flag);
	//printk("I2C int_flag = 0x%x !!\n", int_flag);
	if (int_flag & I2C_INT_DONE_FLAG) {
		DBG_INFO("I2C is done !!\n");
		pstIrqEvent->stIrqFlag.bActiveDone = 1;
	} else {
		pstIrqEvent->stIrqFlag.bActiveDone = 0;
	}

	if (int_flag & I2C_INT_ADDRESS_NACK_FLAG) {
		DBG_INFO("I2C slave address NACK !!\n");
		#ifdef I2C_RETEST
		hal_i2cm_scl_delay_read(device_id, &scl_belay);
		if (scl_belay == 0x00){
		     test_count++;
	       }
		else if (test_count > 9){
		    test_count = 1;
		}
        #endif		
		pstIrqEvent->stIrqFlag.bAddrNack = 1;
	} else {
		pstIrqEvent->stIrqFlag.bAddrNack = 0;
	}

	if (int_flag & I2C_INT_DATA_NACK_FLAG) {
		DBG_INFO("I2C slave data NACK !!\n");
		pstIrqEvent->stIrqFlag.bDataNack = 1;
	} else {
		pstIrqEvent->stIrqFlag.bDataNack = 0;
	}

	// write use
	if (int_flag & I2C_INT_EMPTY_THRESHOLD_FLAG) {
		//DBG_INFO("I2C empty threshold occur !!\n");
		pstIrqEvent->stIrqFlag.bEmptyThreshold = 1;
	} else {
		pstIrqEvent->stIrqFlag.bEmptyThreshold = 0;
	}

	// write use
	if (int_flag & I2C_INT_EMPTY_FLAG) {
		DBG_INFO("I2C FIFO empty occur !!\n");
		pstIrqEvent->stIrqFlag.bFiFoEmpty = 1;
	} else {
		pstIrqEvent->stIrqFlag.bFiFoEmpty = 0;
	}

	// write use (for debug)
	if (int_flag & I2C_INT_FULL_FLAG) {
		DBG_INFO("I2C FIFO full occur !!\n");
		pstIrqEvent->stIrqFlag.bFiFoFull = 1;
	} else {
		pstIrqEvent->stIrqFlag.bFiFoFull = 0;
	}

	if (int_flag & I2C_INT_SCL_HOLD_TOO_LONG_FLAG) {
		DBG_INFO("I2C SCL hold too long occur !!\n");
		pstIrqEvent->stIrqFlag.bSCLHoldTooLong = 1;
	} else {
		pstIrqEvent->stIrqFlag.bSCLHoldTooLong = 0;
	}
	hal_i2cm_status_clear(device_id, I2C_CTL1_ALL_CLR);

	// read use
	hal_i2cm_roverflow_flag_get(device_id, &overflow_flag);
	if (overflow_flag) {
		DBG_ERR("I2C burst read data overflow !! overflow_flag = 0x%x\n", overflow_flag);
		pstIrqEvent->stIrqFlag.bRdOverflow = 1;
	} else {
		pstIrqEvent->stIrqFlag.bRdOverflow = 0;
	}
}

#ifdef SUPPORT_I2C_GDMA
static void _sp_i2cm_dma_intflag_check(unsigned int device_id, I2C_Irq_Event_t *pstIrqEvent)
{
	unsigned int int_flag = 0;

	hal_i2cm_dma_int_flag_get(device_id, &int_flag);
	//printk("I2C int_flag = 0x%x !!\n", int_flag);
	if (int_flag & I2C_DMA_INT_DMA_DONE_FLAG) {
		DBG_INFO("I2C DMA is done !!\n");
		pstIrqEvent->stIrqDmaFlag.bDmaDone = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bDmaDone = 0;
	}
	
	if (int_flag & I2C_DMA_INT_WCNT_ERROR_FLAG) {
		DBG_INFO("I2C DMA WCNT ERR !!\n");
		pstIrqEvent->stIrqDmaFlag.bWCntError = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bWCntError = 0;
	}
	if (int_flag & I2C_DMA_INT_WB_EN_ERROR_FLAG) {
		DBG_INFO("I2C DMA WB EN ERR !!\n");
		pstIrqEvent->stIrqDmaFlag.bWBEnError = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bWBEnError = 0;
	}
	if (int_flag & I2C_DMA_INT_GDMA_TIMEOUT_FLAG) {
		DBG_INFO("I2C DMA timeout !!\n");
		pstIrqEvent->stIrqDmaFlag.bGDmaTimeout = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bGDmaTimeout = 0;
	}
	if (int_flag & I2C_DMA_INT_IP_TIMEOUT_FLAG) {
		DBG_INFO("I2C IP timeout !!\n");
		pstIrqEvent->stIrqDmaFlag.bIPTimeout = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bIPTimeout = 0;
	}
//	if (int_flag & I2C_DMA_INT_THRESHOLD_FLAG) {
//		DBG_INFO("I2C Threshold reach !!\n");
//		pstIrqEvent->stIrqDmaFlag.bThreshold = 1;
//	} else {
//		pstIrqEvent->stIrqDmaFlag.bThreshold = 0;
//	}
	if (int_flag & I2C_DMA_INT_LENGTH0_FLAG) {
		DBG_INFO("I2C Length is zero !!\n");
		pstIrqEvent->stIrqDmaFlag.bLength0 = 1;
	} else {
		pstIrqEvent->stIrqDmaFlag.bLength0 = 0;
	}
	
  hal_i2cm_dma_int_flag_clear(device_id, 0x7F);  //write 1 to clear 

}
#endif

static irqreturn_t _sp_i2cm_irqevent_handler(int irq, void *args)
{
	I2C_Irq_Event_t *pstIrqEvent = (I2C_Irq_Event_t *)args;
	unsigned int device_id = 0;
	unsigned char w_data[32] = {0};
	unsigned char r_data[I2C_BURST_RDATA_BYTES] = {0};
	unsigned int rdata_flag = 0;
	unsigned int bit_index = 0;
	int i = 0, j = 0, k = 0;

	device_id = pstIrqEvent->dDevId;
	_sp_i2cm_intflag_check(device_id, pstIrqEvent);

	switch (pstIrqEvent->eRWState) {
		case I2C_WRITE_STATE:
		case I2C_DMA_WRITE_STATE:
			if (pstIrqEvent->stIrqFlag.bActiveDone) {
				DBG_INFO("I2C write success !!\n");
				pstIrqEvent->bRet = I2C_SUCCESS;
				wake_up(&i2cm_event_wait[device_id]);
			} else if (pstIrqEvent->stIrqFlag.bAddrNack || pstIrqEvent->stIrqFlag.bDataNack) {
				DBG_ERR("I2C reveive NACK !!\n");
				pstIrqEvent->bRet = I2C_ERR_RECEIVE_NACK;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else if (pstIrqEvent->stIrqFlag.bSCLHoldTooLong) {
				DBG_ERR("I2C SCL hold too long !!\n");
				pstIrqEvent->bRet = I2C_ERR_SCL_HOLD_TOO_LONG;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else if (pstIrqEvent->stIrqFlag.bFiFoEmpty) {
				DBG_ERR("I2C FIFO empty !!\n");
				pstIrqEvent->bRet = I2C_ERR_FIFO_EMPTY;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else if ((pstIrqEvent->dBurstCount > 0)&&(pstIrqEvent->eRWState == I2C_WRITE_STATE)) {
				if (pstIrqEvent->stIrqFlag.bEmptyThreshold) {
					for (i = 0; i < I2C_EMPTY_THRESHOLD_VALUE; i++) {
						for (j = 0; j < 4; j++) {
							if (pstIrqEvent->dDataIndex >= pstIrqEvent->dDataTotalLen) {
								w_data[j] = 0;
							} else {
								w_data[j] = pstIrqEvent->pDataBuf[pstIrqEvent->dDataIndex];
							}
							pstIrqEvent->dDataIndex++;
						}
						hal_i2cm_data0_set(device_id, (unsigned int *)w_data);
						pstIrqEvent->dBurstCount--;
						
						if (pstIrqEvent->dBurstCount == 0) {
							hal_i2cm_int_en0_disable(device_id, (I2C_EN0_EMPTY_THRESHOLD_INT | I2C_EN0_EMPTY_INT));
							break;
						}
					}
					hal_i2cm_status_clear(device_id, I2C_CTL1_EMPTY_THRESHOLD_CLR);
				}
			}
			break;

		case I2C_READ_STATE:
		case I2C_DMA_READ_STATE:
			if (pstIrqEvent->stIrqFlag.bAddrNack || pstIrqEvent->stIrqFlag.bDataNack) {
				DBG_ERR("I2C reveive NACK !!\n");
				pstIrqEvent->bRet = I2C_ERR_RECEIVE_NACK;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else if (pstIrqEvent->stIrqFlag.bSCLHoldTooLong) {
				DBG_ERR("I2C SCL hold too long !!\n");
				pstIrqEvent->bRet = I2C_ERR_SCL_HOLD_TOO_LONG;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else if (pstIrqEvent->stIrqFlag.bRdOverflow) {
				DBG_ERR("I2C read data overflow !!\n");
				pstIrqEvent->bRet = I2C_ERR_RDATA_OVERFLOW;
				pstIrqEvent->stIrqFlag.bActiveDone = 1;
				wake_up(&i2cm_event_wait[device_id]);
				hal_i2cm_reset(device_id);
			} else {
				if ((pstIrqEvent->dBurstCount > 0)&&(pstIrqEvent->eRWState == I2C_READ_STATE)) {
					hal_i2cm_rdata_flag_get(device_id, &rdata_flag);
					for (i = 0; i < (32 / I2C_BURST_RDATA_BYTES); i++) {
						bit_index = (I2C_BURST_RDATA_BYTES - 1) + (I2C_BURST_RDATA_BYTES * i);
						if (rdata_flag & (1 << bit_index)) {
							for (j = 0; j < (I2C_BURST_RDATA_BYTES / 4); j++) {
								k = pstIrqEvent->dRegDataIndex + j;
								if (k >= 8) {
									k -= 8;
								}

								hal_i2cm_data_get(device_id, k, (unsigned int *)(&pstIrqEvent->pDataBuf[pstIrqEvent->dDataIndex]));
								pstIrqEvent->dDataIndex += 4;
							}
							hal_i2cm_rdata_flag_clear(device_id, (((1 << I2C_BURST_RDATA_BYTES) - 1) << (I2C_BURST_RDATA_BYTES * i)));
							pstIrqEvent->dRegDataIndex += (I2C_BURST_RDATA_BYTES / 4);
							if (pstIrqEvent->dRegDataIndex >= 8) {
								pstIrqEvent->dRegDataIndex -= 8;
							}
							pstIrqEvent->dBurstCount --;
						}
					}
				}

				if (pstIrqEvent->stIrqFlag.bActiveDone) {
					if ((pstIrqEvent->dBurstRemainder)&&(pstIrqEvent->eRWState == I2C_READ_STATE)) {
						j = 0;
						for (i = 0; i < (I2C_BURST_RDATA_BYTES / 4); i++) {
							k = pstIrqEvent->dRegDataIndex + i;
							if (k >= 8) {
								k -= 8;
							}

							hal_i2cm_data_get(device_id, k, (unsigned int *)(&r_data[j]));
							j += 4;
						}

						for (i = 0; i < pstIrqEvent->dBurstRemainder; i++) {
							pstIrqEvent->pDataBuf[pstIrqEvent->dDataIndex + i] = r_data[i];
						}
					}

					DBG_INFO("I2C read success !!\n");
					pstIrqEvent->bRet = I2C_SUCCESS;
					wake_up(&i2cm_event_wait[device_id]);
				}
			}
			break;

		default:
			break;
	}//switch case

#ifdef SUPPORT_I2C_GDMA
	_sp_i2cm_dma_intflag_check(device_id, pstIrqEvent);   

	DBG_INFO("[I2C adapter] pstIrqEvent->eRWState= 0x%x\n", pstIrqEvent->eRWState);
	switch (pstIrqEvent->eRWState) {
		case I2C_DMA_WRITE_STATE:
			DBG_INFO("I2C_DMA_WRITE_STATE !!\n");
			if (pstIrqEvent->stIrqDmaFlag.bDmaDone) {
					DBG_INFO("I2C dma write success !!\n");
					pstIrqEvent->bRet = I2C_SUCCESS;
				  wake_up(&i2cm_event_wait[device_id]);
	        //return I2C_SUCCESS;
	    }    
			break;
			
		case I2C_DMA_READ_STATE:
			DBG_INFO("I2C_DMA_READ_STATE !!\n");
			if (pstIrqEvent->stIrqDmaFlag.bDmaDone) {
					DBG_INFO("I2C dma read success !!\n");
					//hal_i2cm_dma_int_flag_clear(device_id, I2C_DMA_INT_DMA_DONE_FLAG);
					pstIrqEvent->bRet = I2C_SUCCESS;
				  wake_up(&i2cm_event_wait[device_id]);
	        //return I2C_SUCCESS;
	    }    
			break;

		default:
			break;
	}//switch case
#endif			
  //disable int_en0 for prevent intr not stop in some case , they will be enable again before read/write			
   // if((pstIrqEvent->stIrqFlag.bActiveDone == 1) || (pstIrqEvent->stIrqDmaFlag.bDmaDone == 1)){	
   //         hal_i2cm_int_en0_disable(device_id, (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT ));
   // }
	return IRQ_HANDLED;
}

static void _sp_i2cm_init_irqevent(unsigned int device_id)
{
	I2C_Irq_Event_t *pstIrqEvent = NULL;

	FUNC_DEBUG();

	pstIrqEvent = &stIrqEvent[device_id];
	memset(pstIrqEvent, 0, sizeof(I2C_Irq_Event_t));

	switch(device_id) {
		case 0:
			pstIrqEvent->dDevId = 0;
			break;

		case 1:
			pstIrqEvent->dDevId = 1;
			break;
			
		case 2:
			pstIrqEvent->dDevId = 2;
			break;

		case 3:
			pstIrqEvent->dDevId = 3;
			break;
		default:
			break;
	}
}

static int _sp_i2cm_init(unsigned int device_id, SpI2C_If_t *pstSpI2CInfo)
{
	FUNC_DEBUG();

	if (device_id >= I2C_MASTER_NUM)
	{
		DBG_ERR("I2C device id is not correct !! device_id=%d\n", device_id);
		return I2C_ERR_INVALID_DEVID;
	}

#ifdef SUPPORT_I2C_GDMA
	hal_i2cm_base_set(device_id, pstSpI2CInfo->i2c_regs);
	hal_i2cm_dma_base_set(device_id, pstSpI2CInfo->i2c_dma_regs);
	DBG_INFO("[I2C adapter] i2c_regs= 0x%x\n", pstSpI2CInfo->i2c_regs);
	DBG_INFO("[I2C adapter] i2c_dma_regs= 0x%x\n", pstSpI2CInfo->i2c_dma_regs);
#else
	hal_i2cm_base_set(device_id, pstSpI2CInfo->i2c_regs);
	DBG_INFO("[I2C adapter] i2c_regs= 0x%x\n", pstSpI2CInfo->i2c_regs);
#endif
	hal_i2cm_reset(device_id);
	
	_sp_i2cm_init_irqevent(device_id);
	init_waitqueue_head(&i2cm_event_wait[device_id]);

	return I2C_SUCCESS;
}

static int _sp_i2cm_get_irq(struct platform_device *pdev, SpI2C_If_t *pstSpI2CInfo)
{
	int irq;

	FUNC_DEBUG();

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		DBG_ERR("[I2C adapter] get irq number fail, irq = %d\n", irq);
		return -ENODEV;
	}

	pstSpI2CInfo->irq = irq;
	return I2C_SUCCESS;
}

static int _sp_i2cm_get_register_base(struct platform_device *pdev, unsigned int *membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	FUNC_DEBUG();
	DBG_INFO("[I2C adapter] register name  : %s!!\n", res_name);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if(r == NULL) {
		DBG_INFO("[I2C adapter] platform_get_resource_byname fail\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		DBG_ERR("[I2C adapter] ioremap fail\n");
		return PTR_ERR(p);
	}

	DBG_INFO("[I2C adapter] ioremap addr : 0x%x!!\n", (unsigned int)p);
	*membase = (unsigned int)p;

	return I2C_SUCCESS;
}

static int _sp_i2cm_get_resources(struct platform_device *pdev, SpI2C_If_t *pstSpI2CInfo)
{
	int ret;
	unsigned int membase = 0;

	FUNC_DEBUG();

	ret = _sp_i2cm_get_register_base(pdev, &membase, I2CM_REG_NAME);
	if (ret) {
		DBG_ERR("[I2C adapter] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	else {
		pstSpI2CInfo->i2c_regs = (void __iomem *)membase;
	}
	
#ifdef SUPPORT_I2C_GDMA
	ret = _sp_i2cm_get_register_base(pdev, &membase, I2CM_DMA_REG_NAME);
	if (ret) {
		DBG_ERR("[I2C adapter] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	else {
		pstSpI2CInfo->i2c_dma_regs = (void __iomem *)membase;
	}
#endif

	ret = _sp_i2cm_get_irq(pdev, pstSpI2CInfo);
	if (ret) {
		DBG_ERR("[I2C adapter] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	return I2C_SUCCESS;
}

int sp_i2cm_read(I2C_Cmd_t *pstCmdInfo)
{
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	unsigned char w_data[32] = {0};
	unsigned int read_cnt = 0;
	unsigned int write_cnt = 0;
	unsigned int burst_cnt = 0, burst_r = 0;
	unsigned int int0 = 0, int1 = 0, int2 = 0;
	int ret = I2C_SUCCESS;
	int i = 0;

	FUNC_DEBUG();

	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if (pstIrqEvent->bI2CBusy) {
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;

	write_cnt = pstCmdInfo->dWrDataCnt;
	read_cnt = pstCmdInfo->dRdDataCnt;

	if (pstCmdInfo->dRestartEn)
	{
		//if ((write_cnt > 32) || (write_cnt == 0)) {
		if (write_cnt > 32) {
			pstIrqEvent->bI2CBusy = 0;
			DBG_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
			return I2C_ERR_INVALID_CNT;
		}
	}

	if ((read_cnt > 0xFFFF)  || (read_cnt == 0)) {
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C read count is invalid !! read count=%d\n", read_cnt);
		return I2C_ERR_INVALID_CNT;
	}

	burst_cnt = read_cnt / I2C_BURST_RDATA_BYTES;
	burst_r = read_cnt % I2C_BURST_RDATA_BYTES;
	DBG_INFO("write_cnt = %d, read_cnt = %d, burst_cnt = %d, burst_r = %d\n",
			write_cnt, read_cnt, burst_cnt, burst_r);

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
			| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT );
	if (burst_cnt) {
		int1 = I2C_BURST_RDATA_FLAG;
		int2 = I2C_BURST_RDATA_ALL_FLAG;
	}

	pstIrqEvent->eRWState = I2C_READ_STATE;
	pstIrqEvent->dBurstCount = burst_cnt;
	pstIrqEvent->dBurstRemainder = burst_r;
	pstIrqEvent->dDataIndex = 0;
	pstIrqEvent->dRegDataIndex = 0;
	pstIrqEvent->dDataTotalLen = read_cnt;
	pstIrqEvent->pDataBuf = pstCmdInfo->pRdData;

	hal_i2cm_reset(pstCmdInfo->dDevId);
	hal_i2cm_clock_freq_set(pstCmdInfo->dDevId, pstCmdInfo->dFreq);
	hal_i2cm_slave_addr_set(pstCmdInfo->dDevId, pstCmdInfo->dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	    DBG_INFO("test_count = %d", test_count);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif
	hal_i2cm_trans_cnt_set(pstCmdInfo->dDevId, write_cnt, read_cnt);
	hal_i2cm_active_mode_set(pstCmdInfo->dDevId, I2C_TRIGGER);

	if (pstCmdInfo->dRestartEn) {
		DBG_INFO("I2C_RESTART_MODE\n");
		for (i = 0; i < write_cnt; i++) {
			w_data[i] = pstCmdInfo->pWrData[i];
		}
		hal_i2cm_data_set(pstCmdInfo->dDevId, (unsigned int *)w_data);
		hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_RESTART_MODE);
	} else {
		DBG_INFO("I2C_READ_MODE\n");
		hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_READ_MODE);
	}

	hal_i2cm_int_en0_set(pstCmdInfo->dDevId, int0);
	hal_i2cm_int_en1_set(pstCmdInfo->dDevId, int1);
	hal_i2cm_int_en2_set(pstCmdInfo->dDevId, int2);
	hal_i2cm_manual_trigger(pstCmdInfo->dDevId);	//start send data

	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqFlag.bActiveDone, (I2C_SLEEP_TIMEOUT * HZ) / 1000);
	if (ret == 0) {
		DBG_ERR("I2C read timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
    hal_i2cm_reset(pstCmdInfo->dDevId);
	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;

	return ret;
}

int sp_i2cm_write(I2C_Cmd_t *pstCmdInfo)
{
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	unsigned char w_data[32] = {0};
	unsigned int write_cnt = 0;
	unsigned int burst_cnt = 0;
	unsigned int int0 = 0;
	int ret = I2C_SUCCESS;
	int i = 0;

	FUNC_DEBUG();

	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if (pstIrqEvent->bI2CBusy) {
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;

	write_cnt = pstCmdInfo->dWrDataCnt;

	//if ((write_cnt > 0xFFFF) || (write_cnt == 0)) {
	if (write_cnt > 0xFFFF) {
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
		return I2C_ERR_INVALID_CNT;
	}

	if (write_cnt > 32) {
		burst_cnt = (write_cnt - 32) / 4;
		if ((write_cnt - 32) % 4) {
			burst_cnt += 1;
		}

		for (i = 0; i < 32; i++) {
			w_data[i] = pstCmdInfo->pWrData[i];
		}
	} else {
		for(i = 0; i < write_cnt; i++){
			w_data[i] = pstCmdInfo->pWrData[i];
		}
	}
	DBG_INFO("write_cnt = %d, burst_cnt = %d\n", write_cnt, burst_cnt);

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
			| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT );

	if (burst_cnt)
		int0 |= I2C_EN0_EMPTY_THRESHOLD_INT;

	pstIrqEvent->eRWState = I2C_WRITE_STATE;
	pstIrqEvent->dBurstCount = burst_cnt;
	pstIrqEvent->dDataIndex = i;
	pstIrqEvent->dDataTotalLen = write_cnt;
	pstIrqEvent->pDataBuf = pstCmdInfo->pWrData;

	hal_i2cm_reset(pstCmdInfo->dDevId);
	hal_i2cm_clock_freq_set(pstCmdInfo->dDevId, pstCmdInfo->dFreq);
	hal_i2cm_slave_addr_set(pstCmdInfo->dDevId, pstCmdInfo->dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    DBG_INFO("test_count = %d", test_count);
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif
	hal_i2cm_trans_cnt_set(pstCmdInfo->dDevId, write_cnt, 0);
	hal_i2cm_active_mode_set(pstCmdInfo->dDevId, I2C_TRIGGER);
	hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_WRITE_MODE);
	hal_i2cm_data_set(pstCmdInfo->dDevId, (unsigned int *)w_data);

	if (burst_cnt)
		hal_i2cm_int_en0_with_thershold_set(pstCmdInfo->dDevId, int0, I2C_EMPTY_THRESHOLD_VALUE);
	else
	hal_i2cm_int_en0_set(pstCmdInfo->dDevId, int0);

	hal_i2cm_manual_trigger(pstCmdInfo->dDevId);	//start send data

	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqFlag.bActiveDone, (I2C_SLEEP_TIMEOUT * HZ) / 20);
	if (ret == 0) {
		DBG_ERR("I2C write timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
    hal_i2cm_reset(pstCmdInfo->dDevId);
	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;

	return ret;
}

#ifdef SUPPORT_I2C_GDMA
int sp_i2cm_dma_write(I2C_Cmd_t *pstCmdInfo, SpI2C_If_t *pstSpI2CInfo)
{
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	unsigned char w_data[32] = {0};
	unsigned int write_cnt = 0;
	unsigned int burst_cnt = 0;
	unsigned int int0 = 0;
	int ret = I2C_SUCCESS;
	int i = 0;
	unsigned int dma_int = 0;
	//dma_addr_t dma_handle;

	FUNC_DEBUG();

	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if (pstIrqEvent->bI2CBusy) {
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;

	write_cnt = pstCmdInfo->dWrDataCnt;

	if (write_cnt > 0xFFFF) {
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
		return I2C_ERR_INVALID_CNT;
	}

	if (write_cnt > 32) {
		burst_cnt = (write_cnt - 32) / 4;
		if ((write_cnt - 32) % 4) {
			burst_cnt += 1;
		}

		for (i = 0; i < 32; i++) {
			w_data[i] = pstCmdInfo->pWrData[i];
		}
	} else {
		for(i = 0; i < write_cnt; i++){
			w_data[i] = pstCmdInfo->pWrData[i];
		}
	}
	DBG_INFO("write_cnt = %d, burst_cnt = %d\n", write_cnt, burst_cnt);

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
			| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT );

	if (burst_cnt)
		int0 |= I2C_EN0_EMPTY_THRESHOLD_INT;
		
  dma_int = I2C_DMA_EN_DMA_DONE_INT;

	pstIrqEvent->eRWState = I2C_DMA_WRITE_STATE;
	pstIrqEvent->dBurstCount = burst_cnt;
	pstIrqEvent->dDataIndex = i;
	pstIrqEvent->dDataTotalLen = write_cnt;
	pstIrqEvent->pDataBuf = pstCmdInfo->pWrData;


  //request dma addr map to logical addr
  //pstCmdInfo->pWrData=dma_alloc_coherent(pstCmdInfo->dDevId, write_cnt, &(dma_handle), GFP_KERNEL);
	//DBG_INFO("[I2C adapter] pstCmdInfo->pWrData adr= 0x%x\n", pstCmdInfo->pWrData);
	//DBG_INFO("[I2C adapter] dma_handle= 0x%x\n", dma_handle);
	//for(i = 0; i < write_cnt; i++){
	//		pstCmdInfo->pWrData[i] = w_data[i];  //fill data to new logical addr
	//}

	DBG_INFO("[I2C adapter] pstCmdInfo->dDevId= 0x%x\n", pstCmdInfo->dDevId);
  //copy data to virtual address
	memcpy(pstSpI2CInfo->dma_vir_base, pstCmdInfo->pWrData, pstCmdInfo->dWrDataCnt);  
  
	hal_i2cm_reset(pstCmdInfo->dDevId);
	
	hal_i2cm_dma_mode_enable(pstCmdInfo->dDevId);
	
	hal_i2cm_clock_freq_set(pstCmdInfo->dDevId, pstCmdInfo->dFreq);
	hal_i2cm_slave_addr_set(pstCmdInfo->dDevId, pstCmdInfo->dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    DBG_INFO("test_count = %d", test_count);
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif
	hal_i2cm_active_mode_set(pstCmdInfo->dDevId, I2C_AUTO);
	hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_WRITE_MODE);
	hal_i2cm_int_en0_set(pstCmdInfo->dDevId, int0);

	//hal_i2cm_dma_addr_set(pstCmdInfo->dDevId, (unsigned int)dma_handle);
	//hal_i2cm_dma_length_set(pstCmdInfo->dDevId, write_cnt);
	hal_i2cm_dma_addr_set(pstCmdInfo->dDevId, (unsigned int)pstSpI2CInfo->dma_phy_base);
	hal_i2cm_dma_length_set(pstCmdInfo->dDevId, pstCmdInfo->dWrDataCnt);
	hal_i2cm_dma_rw_mode_set(pstCmdInfo->dDevId, I2C_DMA_READ_MODE);
	hal_i2cm_dma_int_en_set(pstCmdInfo->dDevId, dma_int);
	hal_i2cm_dma_go_set(pstCmdInfo->dDevId);
  
//  ret = _sp_i2cm_irqevent_handler(pstCmdInfo->dDevId, &pstIrqEvent); 
//  hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);
//	if (ret == 1) { //IRQ_HANDLED
//	   ret = pstIrqEvent->bRet;  //if i2c dma write success, return I2C_SUCCESS
//  }
	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqDmaFlag.bDmaDone, (I2C_SLEEP_TIMEOUT * HZ) / 20);
	if (ret == 0) {
		DBG_ERR("I2C write timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
       hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);

	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;
	
  //dma_free_coherent(pstCmdInfo->dDevId, write_cnt,pstCmdInfo->pWrData, (dma_handle));

        hal_i2cm_reset(pstCmdInfo->dDevId);

	return ret;
}

int sp_i2cm_dma_read(I2C_Cmd_t *pstCmdInfo, SpI2C_If_t *pstSpI2CInfo)
{
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	unsigned char w_data[32] = {0};
	unsigned int read_cnt = 0;
	unsigned int write_cnt = 0;
	unsigned int burst_cnt = 0, burst_r = 0;
	unsigned int int0 = 0, int1 = 0, int2 = 0;
	unsigned int dma_int = 0;
	int ret = I2C_SUCCESS;
	int i = 0;
	//dma_addr_t dma_handle;

	FUNC_DEBUG();

	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if (pstIrqEvent->bI2CBusy) {
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;

	write_cnt = pstCmdInfo->dWrDataCnt;
	read_cnt = pstCmdInfo->dRdDataCnt;

	if (pstCmdInfo->dRestartEn)
	{
		if (write_cnt > 32) {
			pstIrqEvent->bI2CBusy = 0;
			DBG_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
			return I2C_ERR_INVALID_CNT;
		}
	}

	if ((read_cnt > 0xFFFF)  || (read_cnt == 0)) {
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C read count is invalid !! read count=%d\n", read_cnt);
		return I2C_ERR_INVALID_CNT;
	}

	burst_cnt = read_cnt / I2C_BURST_RDATA_BYTES;
	burst_r = read_cnt % I2C_BURST_RDATA_BYTES;
	DBG_INFO("write_cnt = %d, read_cnt = %d, burst_cnt = %d, burst_r = %d\n",
			write_cnt, read_cnt, burst_cnt, burst_r);

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
			| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT );
	if (burst_cnt) {
		int1 = I2C_BURST_RDATA_FLAG;
		int2 = I2C_BURST_RDATA_ALL_FLAG;
	}

  dma_int = I2C_DMA_EN_DMA_DONE_INT;
  
	pstIrqEvent->eRWState = I2C_DMA_READ_STATE;
	pstIrqEvent->dBurstCount = burst_cnt;
	pstIrqEvent->dBurstRemainder = burst_r;
	pstIrqEvent->dDataIndex = 0;
	pstIrqEvent->dRegDataIndex = 0;
	pstIrqEvent->dDataTotalLen = read_cnt;

	hal_i2cm_reset(pstCmdInfo->dDevId);
	hal_i2cm_dma_mode_enable(pstCmdInfo->dDevId);
	hal_i2cm_clock_freq_set(pstCmdInfo->dDevId, pstCmdInfo->dFreq);
	hal_i2cm_slave_addr_set(pstCmdInfo->dDevId, pstCmdInfo->dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    DBG_INFO("test_count = %d", test_count);
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif

	if (pstCmdInfo->dRestartEn) {
		DBG_INFO("I2C_RESTART_MODE\n");
	  hal_i2cm_active_mode_set(pstCmdInfo->dDevId, I2C_TRIGGER);
		hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_RESTART_MODE);
	  hal_i2cm_trans_cnt_set(pstCmdInfo->dDevId, write_cnt, read_cnt);
		for (i = 0; i < write_cnt; i++) {
			w_data[i] = pstCmdInfo->pWrData[i];
		}
		hal_i2cm_data_set(pstCmdInfo->dDevId, (unsigned int *)w_data);
	} else {
		DBG_INFO("I2C_READ_MODE\n");
	  hal_i2cm_active_mode_set(pstCmdInfo->dDevId, I2C_AUTO);
		hal_i2cm_rw_mode_set(pstCmdInfo->dDevId, I2C_READ_MODE);
	}

	hal_i2cm_int_en0_set(pstCmdInfo->dDevId, int0);
	hal_i2cm_int_en1_set(pstCmdInfo->dDevId, int1);
	hal_i2cm_int_en2_set(pstCmdInfo->dDevId, int2);
	
  //request dma addr map to logical addr
  //pstIrqEvent->pDataBuf = dma_alloc_coherent(pstCmdInfo->dDevId, read_cnt, &(dma_handle), GFP_KERNEL);
	//DBG_INFO("[I2C adapter] pstCmdInfo->pRdData adr= 0x%x\n", pstCmdInfo->pRdData);
	//DBG_INFO("[I2C adapter] dma_handle= 0x%x\n", dma_handle);

	//hal_i2cm_dma_addr_set(pstCmdInfo->dDevId, (unsigned int) dma_handle);  //data will save to dma_handle and mapping to pstCmdInfo->pRdData
	//hal_i2cm_dma_length_set(pstCmdInfo->dDevId, pstCmdInfo->dRdDataCnt);
	hal_i2cm_dma_addr_set(pstCmdInfo->dDevId, (unsigned int) pstSpI2CInfo->dma_phy_base);  //data will save to dma_handle and mapping to pstCmdInfo->pRdData
	hal_i2cm_dma_length_set(pstCmdInfo->dDevId, pstCmdInfo->dRdDataCnt);
	hal_i2cm_dma_rw_mode_set(pstCmdInfo->dDevId, I2C_DMA_WRITE_MODE);
	hal_i2cm_dma_int_en_set(pstCmdInfo->dDevId, dma_int);
	hal_i2cm_dma_go_set(pstCmdInfo->dDevId);

	if (pstCmdInfo->dRestartEn) {
	  hal_i2cm_manual_trigger(pstCmdInfo->dDevId);  //start send data
  }
  
//  ret = _sp_i2cm_irqevent_handler(pstCmdInfo->dDevId, &pstIrqEvent); 
//	if (ret == 1) { //IRQ_HANDLED
//	   ret = pstIrqEvent->bRet; //if i2c dma read success, return I2C_SUCCESS
//  }
//  hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);
	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqDmaFlag.bDmaDone, (I2C_SLEEP_TIMEOUT * HZ) / 20);
	if (ret == 0) {
		DBG_ERR("I2C read timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
  hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);
  
	//for (i=0 ; i<pstCmdInfo->dRdDataCnt ; i++)
	//{
	//	pstCmdInfo->pRdData[i] = pstIrqEvent->pDataBuf[i]; //copy pDataBuf to pRdData
	//}
	
	//copy data from virtual addr to pstCmdInfo->pRdData
	memcpy(pstCmdInfo->pRdData, pstSpI2CInfo->dma_vir_base, pstCmdInfo->dRdDataCnt);  

	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;
	
  //dma_free_coherent(pstCmdInfo->dDevId, pstCmdInfo->dRdDataCnt,pstIrqEvent->pDataBuf, (dma_handle));

  hal_i2cm_reset(pstCmdInfo->dDevId);

	return ret;
}
#endif

//#ifdef SUPPORT_I2C_GDMA
#if(0)

int sp_i2cm_sg_dma_write(I2C_Cmd_t *pstCmdInfo, SpI2C_If_t *pstSpI2CInfo, unsigned int dCmdNum)
{
	I2C_Cmd_t * pstI2CCmd = NULL;
	unsigned int dDevId = 0;
	unsigned int dFreq = 0;
	unsigned int dSlaveAddr = 0;
	unsigned int int0 = 0;
	int ret = I2C_SUCCESS;
	int i = 0;
	//dma_addr_t dma_handle,dma_handle1;
	dma_addr_t dma_handle;
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	unsigned char w_data[32] = {0};
	unsigned int write_cnt = 0;
	int j = 0;

	FUNC_DEBUG();

	pstI2CCmd = pstCmdInfo;
	dDevId = pstI2CCmd->dDevId;
	dFreq = pstI2CCmd->dFreq;
	dSlaveAddr = pstI2CCmd->dSlaveAddr;
	
	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if(pstIrqEvent->bI2CBusy){
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;
	
	if(dCmdNum == 0){
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C SG DMA count is invalid !! dCmdNum=%d\n", dCmdNum);
		return I2C_ERR_INVALID_CNT;
	}

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
				| I2C_EN0_ADDRESS_NACK_INT | I2C_EN0_DONE_INT );

	pstIrqEvent->eRWState = I2C_DMA_WRITE_STATE;
	
#if 0//test hard code for 2 dma block case
//dma1
  //request dma addr map to logical addr
  pstCmdInfo->pWrData=dma_alloc_coherent(pstCmdInfo->dDevId, 2, &(dma_handle), GFP_KERNEL);
	//DBG_INFO("[I2C adapter] pstCmdInfo->pWrData adr= 0x%x\n", pstCmdInfo->pWrData);
	//DBG_INFO("[I2C adapter] dma_handle= 0x%x\n", dma_handle);
	pstCmdInfo->pWrData[0] = 0x2e;  
	pstCmdInfo->pWrData[1] = 0x99; 
	 
//dma2	
  //request dma addr map to logical addr
  pstI2CCmd->pWrData=dma_alloc_coherent(pstCmdInfo->dDevId, 2, &(dma_handle1), GFP_KERNEL);
	//DBG_INFO("[I2C adapter] pstI2CCmd->pWrData adr= 0x%x\n", pstI2CCmd->pWrData);
	//DBG_INFO("[I2C adapter] dma_handle1= 0x%x\n", dma_handle1);
	pstI2CCmd->pWrData[0] = 0x2e;  
	pstI2CCmd->pWrData[1] = 0x99;  
#endif

	hal_i2cm_reset(dDevId);
	hal_i2cm_dma_mode_enable(dDevId);
	hal_i2cm_clock_freq_set(dDevId, dFreq);
	hal_i2cm_slave_addr_set(dDevId, dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    DBG_INFO("test_count = %d", test_count);
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif
	hal_i2cm_active_mode_set(dDevId, I2C_AUTO);
	hal_i2cm_rw_mode_set(dDevId, I2C_WRITE_MODE);
	hal_i2cm_int_en0_set(dDevId, int0);
	
	hal_i2cm_sg_dma_enable(dDevId);
	
	//set LLI
	for(i = 0; i < dCmdNum; i++)
	{
#if 0//test hard code for 2 dma block case
		hal_i2cm_sg_dma_access_lli_set(dDevId, i);
		if (i==0)
		   hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)dma_handle);
		else if (i==1)
		   hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)dma_handle1);
		else
		   hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)dma_handle);
		   
		hal_i2cm_sg_dma_length_set(dDevId, 2);
#else
		pstI2CCmd = pstCmdInfo + i;
		if(pstI2CCmd->dWrDataCnt == 0){
			pstIrqEvent->bI2CBusy = 0;
			DBG_ERR("I2C write count is invalid !! dWrDataCnt=%d\n", pstI2CCmd->dWrDataCnt);
			return I2C_ERR_INVALID_CNT;
		}
		
	  write_cnt = pstCmdInfo->dWrDataCnt;
	  for(j = 0; j < write_cnt; j++){
	  	w_data[j] = pstI2CCmd->pWrData[j];
	  }
	  DBG_INFO("write_cnt = %d\n", write_cnt);		

		hal_i2cm_sg_dma_access_lli_set(dDevId, i);
    //request dma addr map to logical addr
    pstI2CCmd->pWrData = dma_alloc_coherent(&p_adap->dev, pstI2CCmd->dWrDataCnt, &(dma_handle), GFP_KERNEL);
	  for(j = 0; j < write_cnt; j++){
       pstI2CCmd->pWrData[j] = w_data[j];
		}
		
		hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)dma_handle);
		hal_i2cm_sg_dma_length_set(dDevId, pstI2CCmd->dWrDataCnt);
#endif		
		hal_i2cm_sg_dma_rw_mode_set(dDevId, I2C_DMA_READ_MODE);

		if(i == (dCmdNum - 1))
			hal_i2cm_sg_dma_last_lli_set(dDevId);
	}
	hal_i2cm_sg_dma_start_lli_set(dDevId, 0);
	hal_i2cm_sg_dma_go_set(dDevId);

//	ret = _sp_i2cm_irqevent_handler(dDevId, &pstIrqEvent);
//	if (ret == 1) { //IRQ_HANDLED
//	   ret = pstIrqEvent->bRet; //if i2c sg dma write success, return I2C_SUCCESS
//  }
//	hal_i2cm_status_clear(dDevId, 0xFFFFFFFF);
	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqDmaFlag.bDmaDone, (I2C_SLEEP_TIMEOUT * HZ) / 20);
	if (ret == 0) {
		DBG_ERR("I2C write timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
  hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);
	
	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;	
	hal_i2cm_reset(pstCmdInfo->dDevId);

	return ret;
}

int sp_i2cm_sg_dma_read(I2C_Cmd_t *pstCmdInfo, SpI2C_If_t *pstSpI2CInfo, unsigned int dCmdNum)
{
    struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;
	I2C_Cmd_t * pstI2CCmd = NULL;	
	I2C_Cmd_t * pstI2CCmd1 = NULL;	
	unsigned char w_data[32] = {0};
	unsigned int dDevId = 0;
	unsigned int dFreq = 0;
	unsigned int dSlaveAddr = 0;
	unsigned int int0 = 0;
	unsigned int dma_int = 0;
	int ret = I2C_SUCCESS;
	int i = 0;
	int j = 0;
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	dma_addr_t dma_handle;
	unsigned int sgAddr[32];
	unsigned int sgDataCnt[32];
	unsigned int * NewAddr;

	FUNC_DEBUG();
	
	pstI2CCmd = pstCmdInfo;
	dDevId = pstI2CCmd->dDevId;
	dFreq = pstI2CCmd->dFreq;
	dSlaveAddr = pstI2CCmd->dSlaveAddr;
	
	if (pstCmdInfo->dDevId < I2C_MASTER_NUM) {
		pstIrqEvent = &stIrqEvent[pstCmdInfo->dDevId];
	} else {
		return I2C_ERR_INVALID_DEVID;
	}

	if(pstIrqEvent->bI2CBusy){
		DBG_ERR("I2C is busy !!\n");
		return I2C_ERR_I2C_BUSY;
	}
	_sp_i2cm_init_irqevent(pstCmdInfo->dDevId);
	pstIrqEvent->bI2CBusy = 1;

	if(pstI2CCmd->dRestartEn)
	{
		//if((pstI2CCmd->dWrDataCnt > 32) || (pstI2CCmd->dWrDataCnt == 0)){
		if(pstI2CCmd->dWrDataCnt > 32){
			pstIrqEvent->bI2CBusy = 0;
			DBG_ERR("I2C write count is invalid !! write count=%d\n", pstI2CCmd->dWrDataCnt);
			return I2C_ERR_INVALID_CNT;
		}
	}

	if(dCmdNum == 0){
		pstIrqEvent->bI2CBusy = 0;
		DBG_ERR("I2C SG DMA count is invalid !! dCmdNum=%d\n", dCmdNum);
		return I2C_ERR_INVALID_CNT;
	}

	int0 = (I2C_EN0_SCL_HOLD_TOO_LONG_INT | I2C_EN0_EMPTY_INT | I2C_EN0_DATA_NACK_INT
				| I2C_EN0_ADDRESS_NACK_INT );

	dma_int = I2C_DMA_EN_DMA_DONE_INT;
			
	pstIrqEvent->eRWState = I2C_DMA_READ_STATE;

	hal_i2cm_reset(dDevId);
	hal_i2cm_dma_mode_enable(dDevId);
	hal_i2cm_clock_freq_set(dDevId, dFreq);
	hal_i2cm_slave_addr_set(dDevId, dSlaveAddr);
	#ifdef I2C_RETEST
	if((test_count > 1) && (test_count%3 == 0)){
	    DBG_INFO("test_count = %d", test_count);
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, 0x01);
	}else{
	    hal_i2cm_scl_delay_set(pstCmdInfo->dDevId, I2C_SCL_DELAY);
       }
	#endif
	
	if(pstI2CCmd->dRestartEn)
	{
		hal_i2cm_active_mode_set(dDevId, I2C_TRIGGER);
		hal_i2cm_rw_mode_set(dDevId, I2C_RESTART_MODE);
		hal_i2cm_trans_cnt_set(dDevId, pstI2CCmd->dWrDataCnt, 0);
		DBG_INFO("I2C_RESTART_MODE\n");
		for(i = 0; i < pstI2CCmd->dWrDataCnt; i++){
			w_data[i] = pstI2CCmd->pWrData[i];
		}
		hal_i2cm_data_set(dDevId, (unsigned int *)w_data);
	}
	else
	{
		DBG_INFO("I2C_READ_MODE\n");
		hal_i2cm_active_mode_set(dDevId, I2C_AUTO);
		hal_i2cm_rw_mode_set(dDevId, I2C_READ_MODE);
	}
	
	hal_i2cm_int_en0_set(dDevId, int0);

 	hal_i2cm_sg_dma_enable(dDevId);

	//set LLI
	for(i = 0; i < dCmdNum; i++)
	{
#if 0//test hard code for 2 dma block case
		pstI2CCmd1 = (pstCmdInfo + i);
		pstI2CCmd1->dRdDataCnt = 2;
		sgDataCnt[i] = pstI2CCmd1->dRdDataCnt;
		pstI2CCmd1 = (pstCmdInfo + i + 1);
#else		
		pstI2CCmd1 = (pstCmdInfo + i);
		sgDataCnt[i] = pstI2CCmd1->dRdDataCnt;
		pstI2CCmd1 = (pstCmdInfo + i + 1);
#endif
		if(pstI2CCmd->dRdDataCnt == 0){
			pstIrqEvent->bI2CBusy = 0;
			DBG_ERR("I2C read count is invalid !! dRdDataCnt=%d\n", pstI2CCmd->dRdDataCnt);
			return I2C_ERR_INVALID_CNT;
		}

		hal_i2cm_sg_dma_access_lli_set(dDevId, i);

    //request dma addr map to logical addr
    pstI2CCmd1->pRdData = dma_alloc_coherent(&p_adap->dev, pstI2CCmd->dRdDataCnt, &(dma_handle), GFP_KERNEL);
	  //DBG_INFO("[I2C adapter] pstI2CCmd->pRdData adr= 0x%x\n", pstI2CCmd->pRdData);
	  //DBG_INFO("[I2C adapter] dma_handle= 0x%x\n", dma_handle);
		sgAddr[i] = pstI2CCmd1->pRdData;
#if 0//test hard code for 2 dma block case
		sgAddr[i+10] = &(pstI2CCmd1->pRdData[1]);
#endif
		//sgDataCnt[i] = pstI2CCmd->dRdDataCnt;
		//hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)pstI2CCmd->pRdData);
		hal_i2cm_sg_dma_addr_set(dDevId, (unsigned int)dma_handle);
		//hal_i2cm_sg_dma_length_set(dDevId, pstI2CCmd->dRdDataCnt);
		hal_i2cm_sg_dma_length_set(dDevId, sgDataCnt[i]);
		hal_i2cm_sg_dma_rw_mode_set(dDevId, I2C_DMA_WRITE_MODE);

		if(i == (dCmdNum - 1))
			hal_i2cm_sg_dma_last_lli_set(dDevId);
	}	
	hal_i2cm_dma_int_en_set(dDevId, dma_int);
	hal_i2cm_sg_dma_start_lli_set(dDevId, 0);
	hal_i2cm_sg_dma_go_set(dDevId);

	if(pstCmdInfo->dRestartEn)
	{
		hal_i2cm_manual_trigger(pstCmdInfo->dDevId); //start send data
	}
	
//	ret = _sp_i2cm_irqevent_handler(dDevId, &pstIrqEvent);
//	if (ret == 1) { //IRQ_HANDLED
//	   ret = pstIrqEvent->bRet;
//  }
//	hal_i2cm_status_clear(dDevId, 0xFFFFFFFF);
	ret = wait_event_timeout(i2cm_event_wait[pstCmdInfo->dDevId], pstIrqEvent->stIrqDmaFlag.bDmaDone, (I2C_SLEEP_TIMEOUT * HZ) / 20);
	if (ret == 0) {
		DBG_ERR("I2C read timeout !!\n");
		ret = I2C_ERR_TIMEOUT_OUT;
	} else {
		ret = pstIrqEvent->bRet;
	}
  hal_i2cm_status_clear(pstCmdInfo->dDevId, 0xFFFFFFFF);
	
#if 0//test hard code for 2 dma block case
	NewAddr = sgAddr[0];
	pstCmdInfo->pRdData[0] = *(NewAddr); //copy pDataBuf to pRdData
	NewAddr = sgAddr[10];
	pstCmdInfo->pRdData[1] = *(NewAddr); 
	NewAddr = sgAddr[1];
	pstCmdInfo->pRdData[2] = *(NewAddr); //copy pDataBuf to pRdData
	NewAddr = sgAddr[11];
	pstCmdInfo->pRdData[3] = *(NewAddr); 
#else
	for(i = 0; i < dCmdNum; i++)
	{
		 NewAddr = &sgAddr[i];
   	 for (j=0 ; j<sgDataCnt[i] ; j++)
	   {
		    (pstCmdInfo+i)->pRdData[j] = *(NewAddr+j); //copy pDataBuf to pRdData
	   }
  }
#endif

	pstIrqEvent->eRWState = I2C_IDLE_STATE;
	pstIrqEvent->bI2CBusy = 0;
	hal_i2cm_reset(pstCmdInfo->dDevId);
	return ret;
}
#endif

static int sp_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	SpI2C_If_t *pstSpI2CInfo = (SpI2C_If_t *)adap->algo_data;
	I2C_Cmd_t *pstCmdInfo = &(pstSpI2CInfo->stCmdInfo);
	int ret = I2C_SUCCESS;
	int i = 0;

	FUNC_DEBUG();

#ifdef CONFIG_PM_RUNTIME_I2C
  ret = pm_runtime_get_sync(&adap->dev);
  if (ret < 0)
  	goto out;  
#endif
	if (num == 0) {
		return -EINVAL;
	}

	memset(pstCmdInfo, 0, sizeof(I2C_Cmd_t));
	pstCmdInfo->dDevId = adap->nr;


    if(pstCmdInfo->dFreq > I2C_FREQ)
	pstCmdInfo->dFreq = I2C_FREQ;
	else
	    pstCmdInfo->dFreq = pstSpI2CInfo->i2c_clk_freq/1000;

	DBG_INFO("[I2C] set freq : %d\n", pstCmdInfo->dFreq);

	for (i = 0; i < num; i++) {
		if(msgs[i].flags & I2C_M_TEN)
			return -EINVAL;

		pstCmdInfo->dSlaveAddr = msgs[i].addr;

		if(msgs[i].flags & I2C_M_NOSTART){
			//pstCmdInfo->dWrDataCnt = msgs[i].len;
			//pstCmdInfo->pWrData = msgs[i].buf;
            restart_write_cnt = msgs[i].len;
	    	for (i = 0; i < restart_write_cnt; i++) {
		   	restart_w_data[i] = msgs[i].buf[i];
	    	}
        	restart_en = 1;						
			continue;
		}

		if ( msgs[i].flags & I2C_M_RD) {
			pstCmdInfo->dRdDataCnt = msgs[i].len;
			pstCmdInfo->pRdData = msgs[i].buf;
			if(restart_en == 1){
		    	pstCmdInfo->dWrDataCnt = restart_write_cnt;
		    	pstCmdInfo->pWrData = restart_w_data;				
		    	DBG_INFO("I2C_M_RD dWrDataCnt =%d ",pstCmdInfo->dWrDataCnt);
		    	DBG_INFO("I2C_M_RD pstCmdInfo->pWrData[0] =%x ",pstCmdInfo->pWrData[0]);
			    DBG_INFO("I2C_M_RD pstCmdInfo->pWrData[1] =%x ",pstCmdInfo->pWrData[1]);	
		    	restart_en = 0;			
			    pstCmdInfo->dRestartEn = 1;
	    	}
#ifdef SUPPORT_I2C_GDMA
      if (pstCmdInfo->dRdDataCnt < 4)
			   ret = sp_i2cm_read(pstCmdInfo);
			else   
			   ret = sp_i2cm_dma_read(pstCmdInfo,pstSpI2CInfo);
			//   ret = sp_i2cm_dma_read(pstCmdInfo);
			//ret = sp_i2cm_sg_dma_read(pstCmdInfo,2);  //test code for sg dma and 2dma case
			//msgs[i].buf = pstCmdInfo->pRdData;
#else	
			ret = sp_i2cm_read(pstCmdInfo);
#endif	
		} else {
			pstCmdInfo->dWrDataCnt = msgs[i].len;
			pstCmdInfo->pWrData = msgs[i].buf;
#ifdef SUPPORT_I2C_GDMA
      if (pstCmdInfo->dWrDataCnt < 4)
			   ret = sp_i2cm_write(pstCmdInfo);
			else   
			   ret = sp_i2cm_dma_write(pstCmdInfo,pstSpI2CInfo);
			//   ret = sp_i2cm_dma_write(pstCmdInfo,pstSpI2CInfo);
			//ret = sp_i2cm_sg_dma_write(pstCmdInfo,pstSpI2CInfo,2);  //test code for sg dma and 2dma case
#else	
			ret = sp_i2cm_write(pstCmdInfo);
#endif	
		}

		if (ret != I2C_SUCCESS) {
			return -EIO;
		}
	}

#ifdef CONFIG_PM_RUNTIME_I2C
	   pm_runtime_put(&adap->dev);
       // pm_runtime_put_autosuspend(&adap->dev);

#endif


	return num;
	
#ifdef CONFIG_PM_RUNTIME_I2C
			out :
				pm_runtime_mark_last_busy(&adap->dev);
				pm_runtime_put_autosuspend(&adap->dev);
			 return num;
#endif


	
}

static u32 sp_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm sp_algorithm = {
	.master_xfer	= sp_master_xfer,
	.functionality	= sp_functionality,
};

static int sp_i2c_probe(struct platform_device *pdev)
{
	SpI2C_If_t *pstSpI2CInfo = NULL;
	I2C_Irq_Event_t *pstIrqEvent = NULL;
	struct i2c_adapter *p_adap;
	unsigned int i2c_clk_freq;
	int device_id = 0;
	int ret = I2C_SUCCESS;
	struct device *dev = &pdev->dev;   

	FUNC_DEBUG();

	if (pdev->dev.of_node) {
		pdev->id = of_alias_get_id(pdev->dev.of_node, "i2c");
		DBG_INFO("[I2C adapter] pdev->id=%d\n", pdev->id);
		device_id = pdev->id;
	}

	pstSpI2CInfo = &stSpI2CInfo[device_id];
	memset(pstSpI2CInfo, 0, sizeof(SpI2C_If_t));

	if(!of_property_read_u32(pdev->dev.of_node, "clock-frequency", &i2c_clk_freq)) {
		dev_dbg(&pdev->dev,"clk_freq %d\n",i2c_clk_freq);
		pstSpI2CInfo->i2c_clk_freq = i2c_clk_freq;
	}else
		pstSpI2CInfo->i2c_clk_freq = I2C_FREQ*1000;

		DBG_INFO("[I2C adapter] get freq : %d\n", pstSpI2CInfo->i2c_clk_freq);



	ret = _sp_i2cm_get_resources(pdev, pstSpI2CInfo);
	if (ret != I2C_SUCCESS) {
		DBG_ERR("[I2C adapter] get resources fail !\n");
		return ret;
	}
	
	pstSpI2CInfo->clk = devm_clk_get(dev, NULL);
	//printk(KERN_INFO "pstSpI2CInfo->clk : 0x%x \n",pstSpI2CInfo->clk);
	if (IS_ERR(pstSpI2CInfo->clk)) {
		ret = PTR_ERR(pstSpI2CInfo->clk);
		dev_err(dev, "failed to retrieve clk: %d\n", ret);
		goto err_clk_disable;
	}
	ret = clk_prepare_enable(pstSpI2CInfo->clk);
	//printk(KERN_INFO "clk ret : 0x%x \n",ret);
	if (ret) {
		dev_err(dev, "failed to enable clk: %d\n", ret);
		goto err_clk_disable;
	}
	
	pstSpI2CInfo->rstc = devm_reset_control_get(dev, NULL);
	//printk(KERN_INFO "pstSpI2CInfo->rstc : 0x%x \n",pstSpI2CInfo->rstc);
	if (IS_ERR(pstSpI2CInfo->rstc)) {
		ret = PTR_ERR(pstSpI2CInfo->rstc);
		dev_err(dev, "failed to retrieve reset controller: %d\n", ret);
		goto err_reset_assert;
	}
	ret = reset_control_deassert(pstSpI2CInfo->rstc);
	//printk(KERN_INFO "reset ret : 0x%x \n",ret);
	if (ret) {
		dev_err(dev, "failed to deassert reset line: %d\n", ret);
		goto err_reset_assert;
	}
	
#ifdef SUPPORT_I2C_GDMA
	/* dma alloc*/
	pstSpI2CInfo->dma_vir_base = dma_alloc_coherent(&pdev->dev, bufsiz,
					&pstSpI2CInfo->dma_phy_base, GFP_ATOMIC);
	if(!pstSpI2CInfo->dma_vir_base) {
		dev_err(&pdev->dev, "dma_alloc_coherent fail\n");
		goto free_dma;
	}
	//dev_dbg(&pdev->dev, "dma vir 0x%x\n",pstSpI2CInfo->dma_vir_base);
	//dev_dbg(&pdev->dev, "dma phy 0x%x\n",pstSpI2CInfo->dma_phy_base);
#endif	
	
	ret = _sp_i2cm_init(device_id, pstSpI2CInfo);
	if (ret != 0) {
		DBG_ERR("[I2C adapter] i2c master %d init error\n", device_id);
		return ret;
	}

	p_adap = &pstSpI2CInfo->adap;
	sprintf(p_adap->name, "%s%d", DEVICE_NAME, device_id);
	p_adap->algo = &sp_algorithm;
	p_adap->algo_data = pstSpI2CInfo;
	p_adap->nr = device_id;
	p_adap->class = 0;
	p_adap->retries = 5;
	p_adap->dev.parent = &pdev->dev;
	p_adap->dev.of_node = pdev->dev.of_node; 

	ret = i2c_add_numbered_adapter(p_adap);
	if (ret < 0) {
		DBG_ERR("[I2C adapter] error add adapter %s\n", p_adap->name);
		goto free_dma;
	} else {
		DBG_INFO("[I2C adapter] add adapter %s success\n", p_adap->name);
		platform_set_drvdata(pdev, pstSpI2CInfo);
	}

	pstIrqEvent = &stIrqEvent[device_id];
	ret = request_irq(pstSpI2CInfo->irq, _sp_i2cm_irqevent_handler, IRQF_TRIGGER_HIGH, p_adap->name, pstIrqEvent);
	if (ret) {
		DBG_ERR("request irq fail !!\n");
		return I2C_ERR_REQUESET_IRQ;
	}

#ifdef CONFIG_PM_RUNTIME_I2C
  pm_runtime_set_autosuspend_delay(&pdev->dev,5000);
  pm_runtime_use_autosuspend(&pdev->dev);
  pm_runtime_set_active(&pdev->dev);
  pm_runtime_enable(&pdev->dev);
#endif

	return ret;
	
#ifdef SUPPORT_I2C_GDMA
free_dma:   
	dma_free_coherent(&pdev->dev, bufsiz, pstSpI2CInfo->dma_vir_base, pstSpI2CInfo->dma_phy_base);
#endif
err_reset_assert:
	reset_control_assert(pstSpI2CInfo->rstc);

err_clk_disable:
	clk_disable_unprepare(pstSpI2CInfo->clk);

	return ret;
}

static int sp_i2c_remove(struct platform_device *pdev)
{
	SpI2C_If_t *pstSpI2CInfo = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	FUNC_DEBUG();
	
#ifdef CONFIG_PM_RUNTIME_I2C
  pm_runtime_disable(&pdev->dev);
  pm_runtime_set_suspended(&pdev->dev);
#endif
#ifdef SUPPORT_I2C_GDMA   
	dma_free_coherent(&pdev->dev, bufsiz, pstSpI2CInfo->dma_vir_base, pstSpI2CInfo->dma_phy_base);
#endif

	i2c_del_adapter(p_adap);
	if (p_adap->nr < I2C_MASTER_NUM) {
		clk_disable_unprepare(pstSpI2CInfo->clk);
	  reset_control_assert(pstSpI2CInfo->rstc);
		free_irq(pstSpI2CInfo->irq, NULL);
	}

	return 0;
}

static int sp_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	SpI2C_If_t *pstSpI2CInfo = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	FUNC_DEBUG();

	if (p_adap->nr < I2C_MASTER_NUM) {
	  reset_control_assert(pstSpI2CInfo->rstc);
	}

	return 0;
}

static int sp_i2c_resume(struct platform_device *pdev)
{
	SpI2C_If_t *pstSpI2CInfo = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	FUNC_DEBUG();

	if (p_adap->nr < I2C_MASTER_NUM) {
	  reset_control_deassert(pstSpI2CInfo->rstc);   //release reset
	  clk_prepare_enable(pstSpI2CInfo->clk);        //enable clken and disable gclken
	}

	return 0;
}

static const struct of_device_id sp_i2c_of_match[] = {
	{ .compatible = "sunplus,sp7021-i2cm" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_i2c_of_match);

#ifdef CONFIG_PM_RUNTIME_I2C
static int sp_i2c_runtime_suspend(struct device *dev)
{
	SpI2C_If_t *pstSpI2CInfo = dev_get_drvdata(dev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	FUNC_DEBUG();

	if (p_adap->nr < I2C_MASTER_NUM) {
	  reset_control_assert(pstSpI2CInfo->rstc);
	}

	return 0;
}

static int sp_i2c_runtime_resume(struct device *dev)
{
	SpI2C_If_t *pstSpI2CInfo = dev_get_drvdata(dev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	FUNC_DEBUG();

	if (p_adap->nr < I2C_MASTER_NUM) {
	  reset_control_deassert(pstSpI2CInfo->rstc);   //release reset
	  clk_prepare_enable(pstSpI2CInfo->clk);        //enable clken and disable gclken
	}

	return 0;
}
static const struct dev_pm_ops sp7021_i2c_pm_ops = {
	.runtime_suspend = sp_i2c_runtime_suspend,
	.runtime_resume  = sp_i2c_runtime_resume,
};

#define sp_i2c_pm_ops  (&sp7021_i2c_pm_ops)
#endif

static struct platform_driver sp_i2c_driver = {
	.probe		= sp_i2c_probe,
	.remove		= sp_i2c_remove,
	.suspend	= sp_i2c_suspend,
	.resume		= sp_i2c_resume,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= DEVICE_NAME,
		.of_match_table = sp_i2c_of_match,
#ifdef CONFIG_PM_RUNTIME_I2C
		.pm     = sp_i2c_pm_ops,
#endif
	},
};

static int __init sp_i2c_adap_init(void)
{
	return platform_driver_register(&sp_i2c_driver);
}
module_init(sp_i2c_adap_init);

static void __exit sp_i2c_adap_exit(void)
{
	platform_driver_unregister(&sp_i2c_driver);
}
module_exit(sp_i2c_adap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus I2C Master Driver");
