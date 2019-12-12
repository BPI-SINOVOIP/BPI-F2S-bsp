/*
 * Sunplus SPI controller driver 
 *
 * Author: Sunplus Technology Co., Ltd.
 *	
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/clk.h>
#include <linux/reset.h> 
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>

#define SLAVE_INT_IN


//#define PM_RUNTIME_SPI

/* ---------------------------------------------------------------------------------------------- */

//#define SPI_FUNC_DEBUG
//#define SPI_DBG_INFO
//#define SPI_DBG_ERR

#ifdef SPI_FUNC_DEBUG
	#define FUNC_DEBUG()    printk(KERN_INFO "[SPI] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef SPI_DBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "[SPI] Info: "  fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef SPI_DBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "[SPI] Info: "  fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif
/* ---------------------------------------------------------------------------------------------- */




#define SPI_FULL_DUPLEX


#define MAS_REG_NAME "spi_master"
#define SLA_REG_NAME "spi_slave"

#define DMA_IRQ_NAME "dma_w_intr"
#define MAS_IRQ_NAME "mas_risc_intr"

#define SLA_IRQ_NAME "slave_risc_intr"

#define SPI_MASTER_NUM (4)
#define SPI_MSG_DATA_SIZE (255)



#define CLEAR_MASTER_INT (1<<6)
#define MASTER_INT (1<<7)

#define DMA_READ (0xd)
#define SLA_SW_RST (1<<1)


#define DMA_WRITE (0x4d)
#define SPI_START (1<<0)
#define SPI_BUSY (1<<0)
#define CLEAR_DMA_W_INT (1<<7)
#define DMA_W_INT (1<<8)
#define CLEAR_ADDR_BIT (~(0x180))
#define ADDR_BIT(x) (x<<7)
#define DMA_DATA_RDY (1<<0)
#define PENTAGRAM_SPI_SLAVE_SET (0x2c)



#define TOTAL_LENGTH(x) (x<<24)
#define TX_LENGTH(x) (x<<16)
#define GET_TX_LENGTH(x)  ((x>>16)&0xFF)


#define FINISH_FLAG (1<<6)
#define FINISH_FLAG_MASK (1<<15)
#define RX_FULL_FLAG (1<<5)
#define RX_FULL_FLAG_MASK (1<<14)
#define RX_EMP_FLAG (1<<4)
#define RX_EMP_FLAG_MASK (1<<13)
#define TX_FULL_FLAG (1<<3)
#define TX_FULL_FLAG_MASK (1<<12)
#define TX_EMP_FLAG (1<<2)
#define TX_EMP_FLAG_MASK (1<<11)
#define SPI_START_FD (1<<0)
#define FD_SEL (1<<6)
#define LSB_SEL (1<<4)
#define WRITE_BYTE(x) (x<<9)
#define READ_BYTE(x) (x<<7)
#define CLK_DIVIDER(x) (x<<16)
#define INIT_SPI_MODE (~0x7F)
#define CLEAN_RW_BYTE (~0x780)
#define CLEAN_FLUG_MASK (~0xF800)


#define DELAY_ENABLE (1<<3)
#define CPOL_FD (1<<0)
#define CPHA_R (1<<1)
#define CPHA_W (1<<2)
#define LSB_SEL (1<<4)
#define CS_POR (1<<5)

#define SPI_FD_BUSY (1<<7)
#define SPI_FD_INTR (1<<7)

#define FD_SW_RST (1<<1)

#define RX_CNT (0xF<<12)
#define RX_CNT_MASK(x) (x>>12)
#define TX_CNT (0xF<<8)
#define TX_CNT_MASK(x) (x>>8)

#define DEG_CORE_SPI_LATCH0 (0xB<<8)
#define DEG_CORE_SPI_LATCH1 (0xC<<8)

#define LSB_SEL_MST (1<<1)

#define SPI_DEG_SEL(x) (x<<0)
#define DEG_SPI_MST_MASK (0xFF<<2)
#define DEG_SPI_MST(x) (x>>2)

#define FIFO_DATA_BITS (16*8)    // 16 BYTES



/* slave*/
#define CLEAR_SLAVE_INT (1<<8)
#define SLAVE_DATA_RDY (1<<0)



enum SPI_MODE
{
	SPI_MASTER_READ = 0,
	SPI_MASTER_WRITE = 1,
	SPI_MASTER_RW = 2,
	SPI_SLAVE_READ = 3,
	SPI_SLAVE_WRITE = 4,
	SPI_IDLE = 5
};



typedef struct{
	// Group 091 : SPI_MASTER
    unsigned int  MST_TX_DATA_ADDR                      ; // 00  (ADDR : 0x9C00_2D80)
    unsigned int  MST_TX_DATA_3_2_1_0                   ; // 01  (ADDR : 0x9C00_2D84)
    unsigned int  MST_TX_DATA_7_6_5_4                   ; // 02  (ADDR : 0x9C00_2D88)
    unsigned int  MST_TX_DATA_11_10_9_8                 ; // 03  (ADDR : 0x9C00_2D8C)
    unsigned int  MST_TX_DATA_15_14_13_12               ; // 04  (ADDR : 0x9C00_2D90)
    unsigned int  G091_RESERVED_0[4]                    ; //     (ADDR : 0x9C00_2D94) ~ (ADDR : 0x9C00_2DA0)
    unsigned int  MST_RX_DATA_3_2_1_0                   ; // 09  (ADDR : 0x9C00_2DA4)
    unsigned int  MST_RX_DATA_7_6_5_4                   ; // 10  (ADDR : 0x9C00_2DA8)
    unsigned int  MST_RX_DATA_11_10_9_8                 ; // 11  (ADDR : 0x9C00_2DAC)
    unsigned int  MST_RX_DATA_15_14_13_12               ; // 12  (ADDR : 0x9C00_2DB0)
	  unsigned int  FIFO_DATA                             ; // 13  (ADDR : 0x9C00_2DB4)
  	unsigned int  SPI_FD_STATUS                         ; // 14  (ADDR : 0x9C00_2DB8)
  	unsigned int  SPI_FD_CONFIG                         ; // 15  (ADDR : 0x9C00_2DBC)
	  unsigned int  G091_RESERVED_1                       ; // 16  (ADDR : 0x9C00_2DC0)
    unsigned int  SPI_CTRL_CLKSEL                       ; // 17  (ADDR : 0x9C00_2DC4)
    unsigned int  BYTE_NO                               ; // 18  (ADDR : 0x9C00_2DC8)
    unsigned int  SPI_INT_BUSY                          ; // 19  (ADDR : 0x9C00_2DCC)
    unsigned int  DMA_CTRL                              ; // 20  (ADDR : 0x9C00_2DD0)
    unsigned int  DMA_LENGTH                            ; // 21  (ADDR : 0x9C00_2DD4)
    unsigned int  DMA_ADDR                              ; // 22  (ADDR : 0x9C00_2DD8)
    unsigned int  G091_RESERVED_2[1]                    ; // 23  (ADDR : 0x9C00_2DDC)
    unsigned int  DMA_ADDR_STAT                         ; // 24  (ADDR : 0x9C00_2DE0)
    unsigned int  G091_RESERVED_3[1]                    ; // 25  (ADDR : 0x9C00_2DE4)
    unsigned int  UART_DMA_CTRL                         ; // 26  (ADDR : 0x9C00_2DE8)
    unsigned int  G091_RESERVED_4[1]                    ; // 27  (ADDR : 0x9C00_2DEC)
    unsigned int  SPI_MST_DEBUG_SEL                     ; // 28  (ADDR : 0x9C00_2DF0) 
    unsigned int  SPI_COM_DEBUG_SEL                     ; // 29  (ADDR : 0x9C00_2DF4)
    unsigned int  SPI_EXTRA_CYCLE                       ; // 30  (ADDR : 0x9C00_2DF8)
    unsigned int  MST_DMA_DATA_RDY                      ; // 31  (ADDR : 0x9C00_2DFC)
}SPI_MAS;


typedef struct{
	// Group 092 : SPI_SLAVE
	unsigned int SLV_TX_DATA_2_1_0                     ; // 00  (ADDR : 0x9C00_2E00) 
	unsigned int SLV_TX_DATA_6_5_4_3                   ; // 01  (ADDR : 0x9C00_2E04)
	unsigned int SLV_TX_DATA_10_9_8_7                  ; // 02  (ADDR : 0x9C00_2E08)
	unsigned int SLV_TX_DATA_14_13_12_11               ; // 03  (ADDR : 0x9C00_2E0C)
	unsigned int SLV_TX_DATA_15                        ; // 04  (ADDR : 0x9C00_2E10)
	unsigned int G092_RESERVED_0[4]                    ; //     (ADDR : 0x9C00_2E14) ~ (ADDR : 0x9C00_2E20)
	unsigned int SLV_RX_DATA_3_2_1_0                   ; // 09  (ADDR : 0x9C00_2E24)
	unsigned int SLV_RX_DATA_7_6_5_4                   ; // 10  (ADDR : 0x9C00_2E28)
	unsigned int SLV_RX_DATA_11_10_9_8                 ; // 11  (ADDR : 0x9C00_2E2C)
	unsigned int SLV_RX_DATA_15_14_13_12               ; // 12  (ADDR : 0x9C00_2E30)
	unsigned int G092_RESERVED_1[4]                    ; //     (ADDR : 0x9C00_2E34) ~ (ADDR : 0x9C00_2E40)
	unsigned int RISC_INT_DATA_RDY                     ; // 17  (ADDR : 0x9C00_2E44)
	unsigned int SLV_DMA_CTRL                          ; // 18  (ADDR : 0x9C00_2E48)
	unsigned int SLV_DMA_LENGTH                        ; // 19  (ADDR : 0x9C00_2E4C)
	unsigned int SLV_DMA_INI_ADDR                      ; // 20  (ADDR : 0x9C00_2E50)
	unsigned int G092_RESERVED_2[2]                    ; //     (ADDR : 0x9C00_2E54) ~ (ADDR : 0x9C00_2E58)
	unsigned int ADDR_SPI_BUSY                         ; // 23  (ADDR : 0x9C00_2E5C)
	unsigned int G092_RESERVED_3[8]                    ; //     (ADDR : 0x9C00_2E60) ~ (ADDR : 0x9C00_2E7C)
	
}SPI_SLA;


typedef struct  {
	volatile unsigned int sft_cfg_22;    /* 00 */
	volatile unsigned int sft_cfg_23;    /* 01 */
	volatile unsigned int sft_cfg_24;    /* 02 */
	volatile unsigned int sft_cfg_25;    /* 03 */
	volatile unsigned int sft_cfg_26;    /* 04 */
	volatile unsigned int sft_cfg_27;    /* 05 */
	volatile unsigned int sft_cfg_28;    /* 06 */
	volatile unsigned int sft_cfg_29;    /* 07 */
	volatile unsigned int sft_cfg_30;    /* 06 */
	volatile unsigned int sft_cfg_31;    /* 07 */	
} SPI_MAS_PIN;


typedef struct  {
	volatile unsigned int sft_cfg_0;    /* 00 */
	volatile unsigned int sft_cfg_1;    /* 01 */
	volatile unsigned int sft_cfg_2;    /* 02 */
	volatile unsigned int sft_cfg_3;    /* 03 */
	volatile unsigned int sft_cfg_4;    /* 04 */
	volatile unsigned int sft_cfg_5;    /* 05 */
	volatile unsigned int sft_cfg_6;    /* 06 */
	volatile unsigned int sft_cfg_7;    /* 07 */
	volatile unsigned int sft_cfg_8;    /* 06 */
	volatile unsigned int sft_cfg_9;    /* 07 */	
} SPI_SLA_PIN;


enum {
	SPI_MASTER,
	SPI_SLAVE,
};


struct pentagram_spi_master {

	struct device *dev;
	
	int mode;
	
	struct spi_master *master;
	struct spi_controller *ctlr;

	void __iomem *mas_base;

	void __iomem *sla_base;	
	void __iomem *sft_base;
	void __iomem *sft3_base;
	
	int dma_irq;
	int mas_irq;
	int sla_irq;
	
	struct clk *spi_clk;
	struct reset_control *rstc;	
	
	spinlock_t lock;
	struct mutex		buf_lock;
	unsigned int spi_max_frequency;
	dma_addr_t tx_dma_phy_base;
	dma_addr_t rx_dma_phy_base;
	void * tx_dma_vir_base;
	void * rx_dma_vir_base;
	struct completion isr_done;
	
	struct completion sla_isr;
	unsigned int bufsiz;	
	
    unsigned int  rx_cur_len;
    unsigned int  tx_cur_len; 

    u8 tx_data_buf[SPI_MSG_DATA_SIZE];
    u8 rx_data_buf[SPI_MSG_DATA_SIZE];	
	
	int isr_flag;
};






unsigned int  data_unit;



static unsigned bufsiz = 4096;

static irqreturn_t pentagram_spi_slave_sla_irq(int irq, void *dev)
{
	unsigned long flags;
	struct pentagram_spi_master *pspim = (struct pentagram_spi_master *)dev;
	SPI_SLA* spis_reg = (SPI_SLA *)pspim->sla_base;
	unsigned int reg_temp;

    FUNC_DEBUG();

	spin_lock_irqsave(&pspim->lock, flags);
	reg_temp = readl(&spis_reg->RISC_INT_DATA_RDY);
	reg_temp |= CLEAR_SLAVE_INT;
	writel(reg_temp, &spis_reg->RISC_INT_DATA_RDY);
	spin_unlock_irqrestore(&pspim->lock, flags);

	complete(&pspim->sla_isr);

    DBG_INFO("pentagram_spi_slave_sla_irq done\n");	
	  
	return IRQ_HANDLED;
}


int pentagram_spi_slave_dma_rw(struct spi_device *spi,u8 *buf, unsigned int len, int RW_phase)
{

	struct pentagram_spi_master *pspim = spi_controller_get_devdata(spi->controller);

	SPI_SLA* spis_reg = (SPI_SLA *)(pspim->sla_base);
	SPI_MAS* spim_reg = (SPI_MAS *)(pspim->mas_base);
	struct device dev = spi->dev;
	unsigned int reg_temp;
	unsigned long timeout = msecs_to_jiffies(2000);


    FUNC_DEBUG();
	
    mutex_lock(&pspim->buf_lock);


	if(RW_phase == SPI_SLAVE_WRITE) { 
		memcpy(pspim->tx_dma_vir_base, buf, len);
		writel_relaxed(DMA_WRITE, &spis_reg->SLV_DMA_CTRL);
		writel_relaxed(len, &spis_reg->SLV_DMA_LENGTH);
		writel_relaxed(pspim->tx_dma_phy_base, &spis_reg->SLV_DMA_INI_ADDR);
		reg_temp = readl(&spis_reg->RISC_INT_DATA_RDY);
		reg_temp |= SLAVE_DATA_RDY;
		writel(reg_temp, &spis_reg->RISC_INT_DATA_RDY);
		//regs1->SLV_DMA_CTRL = 0x4d;
		//regs1->SLV_DMA_LENGTH = 0x50;//0x50
		//regs1->SLV_DMA_INI_ADDR = 0x300;
		//regs1->RISC_INT_DATA_RDY |= 0x1;
	}else if (RW_phase == SPI_SLAVE_READ) {

		//reinit_completion(&pspim->dma_isr);
		reinit_completion(&pspim->isr_done);
		writel(DMA_READ, &spis_reg->SLV_DMA_CTRL);
		writel(len, &spis_reg->SLV_DMA_LENGTH);
		writel(pspim->rx_dma_phy_base, &spis_reg->SLV_DMA_INI_ADDR);

			
	      if(!wait_for_completion_timeout(&pspim->isr_done,timeout)){
			dev_err(&dev,"wait_for_completion_timeout\n");
			goto exit_spi_slave_rw;

		}

		while((readl(&spim_reg->DMA_CTRL) & DMA_W_INT) == DMA_W_INT)
		{
			dev_dbg(&dev,"spim_reg->DMA_CTRL 0x%x\n",readl(&spim_reg->DMA_CTRL));
		};

		memcpy(buf, pspim->rx_dma_vir_base, len);
        writel(SLA_SW_RST, &spis_reg->SLV_DMA_CTRL);
	
		/* read*/
		//regs1->SLV_DMA_CTRL = 0xd;
		//regs1->SLV_DMA_LENGTH = 0x50;//0x50
		//regs1->SLV_DMA_INI_ADDR = 0x400;
	}


exit_spi_slave_rw:
	mutex_unlock(&pspim->buf_lock);
	return 0;

	
}
EXPORT_SYMBOL_GPL(pentagram_spi_slave_dma_rw);




static int pentagram_spi_abort(struct spi_controller *ctlr)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);


	complete(&pspim->sla_isr);
	complete(&pspim->isr_done);

	return 0;
}


int pentagram_spi_slave_rw(struct spi_device *spi, const u8  *buf, u8  *data_buf, unsigned int len, int RW_phase)
{

	struct pentagram_spi_master *pspim = spi_controller_get_devdata(spi->controller);

	SPI_SLA* spis_reg = (SPI_SLA *)(pspim->sla_base);
	SPI_MAS* spim_reg = (SPI_MAS *)(pspim->mas_base);
	struct device dev = spi->dev;
	unsigned int reg_temp;


    FUNC_DEBUG();
	
    mutex_lock(&pspim->buf_lock);


	if(RW_phase == SPI_SLAVE_WRITE) { 
		memcpy(pspim->tx_dma_vir_base, buf, len);
		writel_relaxed(DMA_WRITE, &spis_reg->SLV_DMA_CTRL);
		writel_relaxed(len, &spis_reg->SLV_DMA_LENGTH);
		writel_relaxed(pspim->tx_dma_phy_base, &spis_reg->SLV_DMA_INI_ADDR);
		reg_temp = readl(&spis_reg->RISC_INT_DATA_RDY);
		reg_temp |= SLAVE_DATA_RDY;
		writel(reg_temp, &spis_reg->RISC_INT_DATA_RDY);
		//regs1->SLV_DMA_CTRL = 0x4d;
		//regs1->SLV_DMA_LENGTH = 0x50;//0x50
		//regs1->SLV_DMA_INI_ADDR = 0x300;
		//regs1->RISC_INT_DATA_RDY |= 0x1;

		if (wait_for_completion_interruptible(&pspim->isr_done)){
			dev_err(&dev,"wait_for_completion_timeout\n");
			goto exit_spi_slave_rw;
		}

        writel(SLA_SW_RST, &spis_reg->SLV_DMA_CTRL);

		
	}else if (RW_phase == SPI_SLAVE_READ) {

		//reinit_completion(&pspim->dma_isr);
		reinit_completion(&pspim->isr_done);
		writel(DMA_READ, &spis_reg->SLV_DMA_CTRL);
		writel(len, &spis_reg->SLV_DMA_LENGTH);
		writel(pspim->rx_dma_phy_base, &spis_reg->SLV_DMA_INI_ADDR);


		if (wait_for_completion_interruptible(&pspim->isr_done)){
			dev_err(&dev,"wait_for_completion_timeout\n");
			goto exit_spi_slave_rw;
		}


		while((readl(&spim_reg->DMA_CTRL) & DMA_W_INT) == DMA_W_INT)
		{
			dev_dbg(&dev,"spim_reg->DMA_CTRL 0x%x\n",readl(&spim_reg->DMA_CTRL));
		};

		memcpy(data_buf, pspim->rx_dma_vir_base, len);
        writel(SLA_SW_RST, &spis_reg->SLV_DMA_CTRL);
	
		/* read*/
		//regs1->SLV_DMA_CTRL = 0xd;
		//regs1->SLV_DMA_LENGTH = 0x50;//0x50
		//regs1->SLV_DMA_INI_ADDR = 0x400;
	}


exit_spi_slave_rw:
	mutex_unlock(&pspim->buf_lock);
	return 0;

	
}


static irqreturn_t pentagram_spi_master_dma_irq(int irq, void *dev)
{
	unsigned long flags;
	struct pentagram_spi_master *pspim = (struct pentagram_spi_master *)dev;
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	unsigned int reg_temp;

    FUNC_DEBUG();

	spin_lock_irqsave(&pspim->lock, flags);
	reg_temp = readl(&spim_reg->DMA_CTRL);
	reg_temp |= CLEAR_DMA_W_INT;
	writel(reg_temp, &spim_reg->DMA_CTRL);
	spin_unlock_irqrestore(&pspim->lock, flags);

	complete(&pspim->isr_done);
	return IRQ_HANDLED;
}
static irqreturn_t pentagram_spi_master_mas_irq(int irq, void *dev)
{
	unsigned long flags;
	struct pentagram_spi_master *pspim = (struct pentagram_spi_master *)dev;
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	unsigned int reg_temp;
	unsigned int i;
	unsigned int tx_lenght;

    FUNC_DEBUG();


	spin_lock_irqsave(&pspim->lock, flags);

    tx_lenght = GET_TX_LENGTH(readl(&spim_reg->SPI_FD_STATUS));
	DBG_INFO("get tx_lenght = 0x%x \n",tx_lenght); 

	if((readl(&spim_reg->SPI_FD_STATUS) & FINISH_FLAG) == FINISH_FLAG){
	
		DBG_INFO("FINISH_FLAG");

		    if((readl(&spim_reg->SPI_FD_STATUS) & RX_FULL_FLAG) == RX_FULL_FLAG){
		        for(i=0;i<data_unit;i++){	 // if READ_BYTE(0) i<16  can set the condition at here
		 	       pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
		 	       DBG_INFO("RXcnt492 data 0x%x  rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->rx_cur_len);		   
		 	       pspim->rx_cur_len++;
		 	    }
		    }
	
			 while(readl(&spim_reg->SPI_FD_STATUS) & RX_CNT){	
				 pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
				 DBG_INFO("RXcnt480 data 0x%x ,tx_cur_len %d rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->tx_cur_len,pspim->rx_cur_len); 
				 pspim->rx_cur_len++;
			 }
		
			 DBG_INFO("[SPI_FD]522 set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));
             goto exit_irq;
	
	}else if(((readl(&spim_reg->SPI_FD_STATUS) & TX_EMP_FLAG) == TX_EMP_FLAG) || (pspim->tx_cur_len < tx_lenght) ){

	DBG_INFO("TX_EMP_FLAG");

	   if((readl(&spim_reg->SPI_FD_STATUS) & RX_FULL_FLAG) == RX_FULL_FLAG){
			for(i=0;i<data_unit;i++){	 // if READ_BYTE(0) i<16  can set the condition at here
			    pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
			    DBG_INFO("RXcnt533 data 0x%x  rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->rx_cur_len);		   
			    pspim->rx_cur_len++;
			    if(pspim->tx_cur_len < tx_lenght){
		            writel(pspim->tx_data_buf[pspim->tx_cur_len], &spim_reg->FIFO_DATA);
		            pspim->tx_cur_len++;				
				}
			}
		}
	   
	    while(readl(&spim_reg->SPI_FD_STATUS) & RX_CNT){   
		    pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
		    DBG_INFO("RXcnt544 data 0x%x tx_cur_len = %d rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->tx_cur_len,pspim->rx_cur_len); 
		    pspim->rx_cur_len++;
			if((pspim->tx_cur_len < tx_lenght) &&  ((readl(&spim_reg->SPI_FD_STATUS) & TX_FULL_FLAG) != TX_FULL_FLAG)){
		        writel(pspim->tx_data_buf[pspim->tx_cur_len], &spim_reg->FIFO_DATA);
		        pspim->tx_cur_len++;				
			}			
	    }

		if(pspim->tx_cur_len < tx_lenght){
 		    while(tx_lenght-pspim->tx_cur_len){
		    	DBG_INFO("tx_data_buf554 0x%x  ,tx_cur_len %d \n",pspim->tx_data_buf[pspim->tx_cur_len],pspim->tx_cur_len);
				if((readl(&spim_reg->SPI_FD_STATUS) & TX_FULL_FLAG) == TX_FULL_FLAG)
			    	break;
		        writel(pspim->tx_data_buf[pspim->tx_cur_len], &spim_reg->FIFO_DATA);
				pspim->tx_cur_len++;
		    }  
		}


	    spin_unlock_irqrestore(&pspim->lock, flags);
	    return IRQ_HANDLED;
    }else if((readl(&spim_reg->SPI_FD_STATUS) & RX_FULL_FLAG) == RX_FULL_FLAG){

	DBG_INFO("RX_FULL_FLAG");

	
           for(i=0;i<data_unit;i++){    // if READ_BYTE(0) i<data_unit  can set the condition at here
            //DBG_INFO("rx_cur_len %d",pspim->rx_cur_len);
           //char_temp= (char)readl(&spim_reg->FIFO_DATA);
		   //DBG_INFO("001 char_temp %d",char_temp);
		    //pspim->rx_data_buf[pspim->rx_cur_len] = (char)readl(&spim_reg->FIFO_DATA);
			   pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
		       DBG_INFO("RXcnt492 data 0x%x  rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->rx_cur_len);		   
               pspim->rx_cur_len++;
           }

		   while((readl(&spim_reg->SPI_FD_STATUS) & RX_CNT) || ((readl(&spim_reg->SPI_FD_STATUS) & RX_FULL_FLAG) == RX_FULL_FLAG)){   
			   pspim->rx_data_buf[pspim->rx_cur_len] = readl(&spim_reg->FIFO_DATA);
			   DBG_INFO("RXcnt586 data 0x%x tx_cur_len = %d rx_cur_len = %d \n",pspim->rx_data_buf[pspim->rx_cur_len],pspim->tx_cur_len,pspim->rx_cur_len); 
			   pspim->rx_cur_len++;		   
		   }


		   spin_unlock_irqrestore(&pspim->lock, flags);
		   return IRQ_HANDLED;

	}
	


	if(pspim->isr_flag == SPI_MASTER_WRITE)
	{
		reg_temp = readl(&spim_reg->SPI_CTRL_CLKSEL);
		reg_temp |= SPI_START;
		writel(reg_temp, &spim_reg->SPI_CTRL_CLKSEL);
	}else
		pspim->isr_flag = SPI_MASTER_READ;

exit_irq:
	reg_temp = readl(&spim_reg->SPI_INT_BUSY);
	reg_temp |= CLEAR_MASTER_INT;
	writel(reg_temp, &spim_reg->SPI_INT_BUSY);		
	spin_unlock_irqrestore(&pspim->lock, flags);

	complete(&pspim->isr_done);
	return IRQ_HANDLED;
}

#if(0)

static int pentagram_spi_master_dma_write(struct spi_master *master, char *buf, unsigned int len)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(master);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	struct device dev = master->dev;
	unsigned int addr;
	unsigned int valid = 0;
	unsigned int data_len = len;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	unsigned long flags;
	int buf_offset = 0;	
	int ret;

    FUNC_DEBUG();

    mutex_lock(&pspim->buf_lock);


	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= ~FD_SEL;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);     // Change to Normal mode



	switch(buf[0])
	{
		case 0:
			addr = buf[1];
			valid = 0xff;
			buf_offset = 2;
			break;
		case 1:
			addr = buf[1] | buf[2] <<8;
			valid = 0xfff;
			buf_offset = 3;
			break;
		case 2:
			addr = buf[1] | buf[2] <<8;
			valid = 0xffff;
			buf_offset = 3;
			break;
		case 3:
			addr = buf[1] | buf[2] <<8 | buf[3] <<16;
			valid = 0xfffff;
			buf_offset = 4;
			break;
		default:
			dev_err(&dev,"wrong addr bit num \n");
			return 1;
			break;
	}
	memcpy(pspim->tx_dma_vir_base, buf + buf_offset, len);
	reinit_completion(&pspim->isr_done);

	spin_lock_irqsave(&pspim->lock, flags);
	pspim->isr_flag = SPI_MASTER_WRITE;
	spin_unlock_irqrestore(&pspim->lock, flags);

	writel(addr & valid, &spim_reg->MST_TX_DATA_ADDR);
	writel(data_len, &spim_reg->DMA_LENGTH);
	writel(pspim->tx_dma_phy_base , &spim_reg->DMA_ADDR);

	spin_lock_irqsave(&pspim->lock, flags);
	writel(DMA_WRITE, &spim_reg->DMA_CTRL);
	reg_temp = readl(&spim_reg->SPI_CTRL_CLKSEL);
	reg_temp &= CLEAR_ADDR_BIT;
	reg_temp |= ADDR_BIT(buf[0]);
	writel(reg_temp, &spim_reg->SPI_CTRL_CLKSEL);
	spin_unlock_irqrestore(&pspim->lock, flags);

	writel(DMA_DATA_RDY, &spim_reg->MST_DMA_DATA_RDY);
	if(!wait_for_completion_timeout(&pspim->isr_done, timeout))
	{
		dev_err(&dev,"wait_for_completion_timeout\n");
		ret = 1;
		goto free_master_dma_write;
	}
	while((readl(&spim_reg->SPI_INT_BUSY) & SPI_BUSY) == SPI_BUSY)
	{
		dev_dbg(&dev,"spim_reg->SPI_INT_BUSY 0x%x\n",readl(&spim_reg->SPI_INT_BUSY));
	};

	spin_lock_irqsave(&pspim->lock, flags);
	pspim->isr_flag = SPI_IDLE;
	spin_unlock_irqrestore(&pspim->lock, flags);

		ret = 0;
	
	
free_master_dma_write:
		mutex_unlock(&pspim->buf_lock);
		return ret;

}
static int pentagram_spi_master_dma_read(struct spi_master *master, char *cmd, unsigned int len)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(master);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	struct device dev = master->dev;
	unsigned int addr;
	unsigned int valid = 0;
	unsigned int data_len = len;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	unsigned long flags;
	int ret;

    FUNC_DEBUG();
   
    mutex_lock(&pspim->buf_lock);

	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= ~FD_SEL;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);     // Change to Normal mode
	
	#ifdef SLAVE_INT_IN
	while(pspim->isr_flag != SPI_MASTER_READ)
	{
		dev_dbg(&dev,"wait read isr %d\n",pspim->isr_flag);
	};
	//while((readl(&spim_reg->SPI_INT_BUSY) & MASTER_INT) == 0x0)
	//{
	//	dev_dbg(&dev,"wait spim_reg->SPI_INT_BUSY 0x%x\n",readl(&spim_reg->SPI_INT_BUSY));
	//};
	//reg_temp = readl(&spim_reg->SPI_INT_BUSY);
	//reg_temp |= CLEAR_MASTER_INT;
	//writel(reg_temp, &spim_reg->SPI_INT_BUSY);
	#endif
	switch(cmd[0])
	{
		case 0:
			addr = cmd[1];
			valid = 0xff;
			break;
		case 1:
			addr = cmd[1] | cmd[2] <<8;
			valid = 0xfff;
			break;
		case 2:
			addr = cmd[1] | cmd[2] <<8;
			valid = 0xffff;
			break;
		case 3:
			addr = cmd[1] | cmd[2] <<8 | cmd[3] <<16;
			valid = 0xfffff;
			break;
		default:
			dev_err(&dev,"wrong addr bit num \n");
			return 1;
			break;
	}
	reinit_completion(&pspim->isr_done);
	writel(addr & valid, &spim_reg->MST_TX_DATA_ADDR);
	writel(data_len, &spim_reg->DMA_LENGTH);
	writel(pspim->rx_dma_phy_base, &spim_reg->DMA_ADDR);

	spin_lock_irqsave(&pspim->lock, flags);
	writel(DMA_READ, &spim_reg->DMA_CTRL);
	reg_temp = readl(&spim_reg->SPI_CTRL_CLKSEL);
	reg_temp &= CLEAR_ADDR_BIT;
	reg_temp |= ADDR_BIT(cmd[0]);
	reg_temp |= SPI_START;
	writel(reg_temp, &spim_reg->SPI_CTRL_CLKSEL);
	spin_unlock_irqrestore(&pspim->lock, flags);

	if(!wait_for_completion_timeout(&pspim->isr_done,timeout)){
		dev_err(&dev,"wait_for_completion_timeout\n");
		ret = 1;
		goto free_master_dma_read;

	}
	while((readl(&spim_reg->DMA_CTRL) & DMA_W_INT) == DMA_W_INT){
		dev_dbg(&dev,"spim_reg->DMA_CTRL 0x%x\n",readl(&spim_reg->DMA_CTRL));
	};

	spin_lock_irqsave(&pspim->lock, flags);
	pspim->isr_flag = SPI_IDLE;
	spin_unlock_irqrestore(&pspim->lock, flags);

	ret = 0;


free_master_dma_read:
    mutex_unlock(&pspim->buf_lock);
	return ret;
	
}

#endif

static int pentagram_spi_master_fullduplex_write_read(struct spi_controller *ctlr, const u8  *buf, u8  *data_buf , unsigned int len , unsigned int tx_len)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	//struct device dev = master->dev;
	//unsigned int addr;
	//unsigned int valid = 0;
	unsigned int data_len = len;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	//unsigned long flags;
	//int buf_offset = 0;	
	unsigned int i;
	int ret;	

    FUNC_DEBUG();

	memcpy(&pspim->tx_data_buf[0], buf, data_len);
	
	DBG_INFO("data_buf 0x%x	\n",buf[0]);		
	DBG_INFO("tx_data_buf init 0x%x	,tx_cur_len %d  \n",pspim->tx_data_buf[0],pspim->tx_cur_len);	

	
	mutex_lock(&pspim->buf_lock);
	
	reinit_completion(&pspim->isr_done);

	
	// set SPI FIFO data for full duplex (SPI_FD fifo_data)  91.13
    if(pspim->tx_cur_len < data_len){
        if(data_len >= data_unit){
		    for(i=0;i<data_unit;i++){
		    DBG_INFO("tx_data_buf1143 0x%x  ,tx_cur_len %d  \n",pspim->tx_data_buf[i],pspim->tx_cur_len);	
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
	            pspim->tx_cur_len++;
	    	}
        }else{
 		    for(i=0;i<data_len;i++){
		    DBG_INFO("tx_data_buf1150 0x%x  ,cur_len %d\n",pspim->tx_data_buf[i],pspim->tx_cur_len);		
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
		    pspim->tx_cur_len++;
		    }   
        }
    }
	
    // initial SPI master config and change to Full-Duplex mode (SPI_FD_CONFIG)  91.15
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG); 	
	reg_temp &= CLEAN_RW_BYTE;
	reg_temp &= CLEAN_FLUG_MASK;	
	reg_temp |= FD_SEL;

    // set SPI master config for full duplex (SPI_FD_CONFIG)  91.15
	reg_temp |= FINISH_FLAG_MASK | TX_EMP_FLAG_MASK | RX_FULL_FLAG_MASK;    //   | CPHA_R for sunplus slave | CPHA_W for BMP280
	reg_temp |= WRITE_BYTE(0) | READ_BYTE(0);  // set read write byte from fifo
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);    


	DBG_INFO("SPI_FD_CONFIG =0x%x\n",readl(&spim_reg->SPI_FD_CONFIG));

	//printk( "[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

    // set SPI STATUS and start SPI for full duplex (SPI_FD_STATUS)  91.13
	writel(TOTAL_LENGTH(data_len) | TX_LENGTH(data_len),&spim_reg->SPI_FD_STATUS);

	DBG_INFO( "[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

	
    reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= SPI_START_FD;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 

	
		if(!wait_for_completion_timeout(&pspim->isr_done, timeout)){
			DBG_INFO("wait_for_completion_timeout 865\n");
			ret = 1;
			goto free_master_write;
		}

		if((tx_len >0) && (pspim->rx_cur_len >= tx_len))
		memcpy(data_buf, &pspim->rx_data_buf[tx_len], (pspim->rx_cur_len-tx_len));
		else
        memcpy(data_buf, &pspim->rx_data_buf[0], pspim->rx_cur_len);

		ret = 0;
		
free_master_write:
	
    // reset SPI
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= CLEAN_FLUG_MASK;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);
	
	reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= FD_SW_RST;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 

	
	mutex_unlock(&pspim->buf_lock);
		
	return ret;

}



static int pentagram_spi_master_read(struct spi_controller *ctlr, const u8  *buf, u8  *data_buf , unsigned int len)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	//struct device dev = master->dev;
	//unsigned int addr;
	//unsigned int valid = 0;
	unsigned int data_len = len;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	//unsigned long flags;
	//int buf_offset = 0;	
	int ret;

    FUNC_DEBUG();


	DBG_INFO("tx_cur_len %d  %d \n",pspim->tx_cur_len);	


    mutex_lock(&pspim->buf_lock);

	reinit_completion(&pspim->isr_done);


    // initial SPI master config and change to Full-Duplex mode (SPI_FD_CONFIG)  91.15
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= CLEAN_RW_BYTE;
	reg_temp &= CLEAN_FLUG_MASK;
	reg_temp |= FD_SEL;

    // set SPI master config for full duplex (SPI_FD_CONFIG)  91.15
	reg_temp |= FINISH_FLAG_MASK | RX_FULL_FLAG_MASK;   //  set read write byte from fifo  | CPHA_R for sunplus slave
	reg_temp |= WRITE_BYTE(0) | READ_BYTE(0);   // set read write byte from fifo
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);    


	DBG_INFO("SPI_FD_CONFIG =0x%x\n",readl(&spim_reg->SPI_FD_CONFIG));

    // set SPI FIFO data for full duplex (SPI_FD fifo_data)  91.13
	writel(0, &spim_reg->FIFO_DATA);     // keep tx not empty in only read mode

    // set SPI STATUS and start SPI for full duplex (SPI_FD_STATUS)  91.13
	writel(TOTAL_LENGTH(data_len) | TX_LENGTH(0),&spim_reg->SPI_FD_STATUS);

	DBG_INFO("[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

	// start SPI transfer
    reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= SPI_START_FD;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 



	if(!wait_for_completion_timeout(&pspim->isr_done, timeout)){
		DBG_INFO("wait_for_completion_timeout  951\n");
		ret = 1;
		goto free_master_read;
	}

	memcpy(data_buf, &pspim->rx_data_buf[0], pspim->rx_cur_len);
	ret = 0;
	
free_master_read:

    // reset SPI
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= CLEAN_FLUG_MASK;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);
	
        reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= FD_SW_RST;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 
	DBG_INFO("finish FD read\n");

	mutex_unlock(&pspim->buf_lock);
	
	return ret;

}


static int pentagram_spi_master_write(struct spi_controller *ctlr, const u8  *buf, u8  *data_buf , unsigned int len)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	//struct device dev = master->dev;
	//unsigned int addr;
	//unsigned int valid = 0;
	unsigned int data_len = len;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	//unsigned long flags;
	//int buf_offset = 0;	
	unsigned int i;
	int ret;	

    FUNC_DEBUG();

	memcpy(&pspim->tx_data_buf[0], buf, data_len);

	DBG_INFO("data_buf 0x%x	\n",buf[0]);		
	DBG_INFO("tx_data_buf init 0x%x	,tx_cur_len %d\n",pspim->tx_data_buf[0],pspim->tx_cur_len);	

	DBG_INFO("SPI_FD_CONFIG =0x%x\n",readl(&spim_reg->SPI_FD_CONFIG));



    mutex_lock(&pspim->buf_lock);

	reinit_completion(&pspim->isr_done);

    // set SPI FIFO data for full duplex (SPI_FD fifo_data)  91.13
    if(pspim->tx_cur_len < data_len){
        if(data_len >= data_unit){
		    for(i=0;i<data_unit;i++){
		    DBG_INFO("tx_data_buf1143 0x%x  ,tx_cur_len %d \n",pspim->tx_data_buf[i],pspim->tx_cur_len);	
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
	      	    pspim->tx_cur_len++;
	    	}
        }else{
 		    for(i=0;i<data_len;i++){
		    DBG_INFO("tx_data_buf1150 0x%x  ,cur_len %d  \n",pspim->tx_data_buf[i],pspim->tx_cur_len);		
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
		    pspim->tx_cur_len++;
		    }   
        }
    }
	
    // initial SPI master config and change to Full-Duplex mode (SPI_FD_CONFIG)  91.15
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG); 
	reg_temp &= CLEAN_RW_BYTE;
	reg_temp &= CLEAN_FLUG_MASK;	
	reg_temp |= FD_SEL;

    // set SPI master config for full duplex (SPI_FD_CONFIG)  91.15
	reg_temp |= FINISH_FLAG_MASK | TX_EMP_FLAG_MASK | RX_FULL_FLAG_MASK;   //   | CPHA_R for sunplus slave | CPHA_W
	reg_temp |= WRITE_BYTE(0) | READ_BYTE(0);   // set read write byte from fifo
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);    


	DBG_INFO("SPI_FD_CONFIG =0x%x\n",readl(&spim_reg->SPI_FD_CONFIG));


	
	//printk( "[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

    // set SPI STATUS and start SPI for full duplex (SPI_FD_STATUS)  91.13
	writel(TOTAL_LENGTH(data_len) | TX_LENGTH(data_len),&spim_reg->SPI_FD_STATUS);

	DBG_INFO("[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

	
    reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= SPI_START_FD;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 

	
		if(!wait_for_completion_timeout(&pspim->isr_done, timeout)){
			DBG_INFO("wait_for_completion_timeout 1121\n");
			ret = 1;
			goto free_master_write;
		}

		ret = 0;
		
	free_master_write:
	
    // reset SPI
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= CLEAN_FLUG_MASK;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);
	
	reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= FD_SW_RST;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 
	DBG_INFO("finish FD write\n");
	
	mutex_unlock(&pspim->buf_lock);
		
	return ret;



}


static int pentagram_spi_master_combine_write_read(struct spi_controller *ctlr,
          struct spi_transfer *first, unsigned int transfers_cnt)
{
	
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;
	//struct device dev = master->dev;
	//unsigned int addr;
	//unsigned int valid = 0;
	unsigned int data_len = 0 ;
	unsigned int reg_temp = 0;
	unsigned long timeout = msecs_to_jiffies(200);
	//unsigned long flags;
	//int buf_offset = 0;	
	unsigned int i;
	int ret;	

	struct spi_transfer *t = first;
	bool xfer_rx = false;


    FUNC_DEBUG();

	memset(&pspim->tx_data_buf[0], 0, SPI_MSG_DATA_SIZE);
		
	DBG_INFO("tx_data_buf init 0x%x	,tx_cur_len %d ,transfers_cnt  %d \n",pspim->tx_data_buf[0],pspim->tx_cur_len,transfers_cnt);	
	DBG_INFO("txrx: tx %p, rx %p, len %d\n", t->tx_buf, t->rx_buf, t->len);

	
	mutex_lock(&pspim->buf_lock);
	
	reinit_completion(&pspim->isr_done);

	for (i = 0; i < transfers_cnt; i++) {

		if (t->tx_buf) 
			memcpy(&pspim->tx_data_buf[data_len], t->tx_buf, t->len);

		if (t->rx_buf) 
			xfer_rx = true;

		data_len += t->len;

		t = list_entry(t->transfer_list.next, struct spi_transfer,
			       transfer_list);
	}

	DBG_INFO("txrx1198: tx %p, rx %p, len %d\n", t->tx_buf, t->rx_buf, t->len);

	DBG_INFO("tx_data_buf init 0x%x	,tx_cur_len %d ,data_len  %d \n",pspim->tx_data_buf[0],pspim->tx_cur_len,data_len);	
	DBG_INFO("xfer_rx %d   \n",xfer_rx);	


	
	// set SPI FIFO data for full duplex (SPI_FD fifo_data)  91.13
    if(pspim->tx_cur_len < data_len){
        if(data_len >= data_unit){
		    for(i=0;i<data_unit;i++){
		    DBG_INFO("tx_data_buf1143 0x%x  ,tx_cur_len %d\n",pspim->tx_data_buf[i],pspim->tx_cur_len);	
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
	    	   pspim->tx_cur_len++;
	    	}
        }else{
 		    for(i=0;i<data_len;i++){
		    DBG_INFO("tx_data_buf1150 0x%x  ,cur_len %d \n",pspim->tx_data_buf[i],pspim->tx_cur_len);		
		    writel(pspim->tx_data_buf[i], &spim_reg->FIFO_DATA);
		    pspim->tx_cur_len++;
		    }   
        }
    }
	
    // initial SPI master config and change to Full-Duplex mode (SPI_FD_CONFIG)  91.15
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG); 	
	reg_temp &= CLEAN_RW_BYTE;
	reg_temp &= CLEAN_FLUG_MASK;	
	reg_temp |= FD_SEL;

    // set SPI master config for full duplex (SPI_FD_CONFIG)  91.15
	reg_temp |= FINISH_FLAG_MASK | TX_EMP_FLAG_MASK | RX_FULL_FLAG_MASK;    //   | CPHA_R for sunplus slave | CPHA_W for BMP280
	reg_temp |= WRITE_BYTE(0) | READ_BYTE(0);  // set read write byte from fifo
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);    


	DBG_INFO("SPI_FD_CONFIG =0x%x\n",readl(&spim_reg->SPI_FD_CONFIG));

	//printk( "[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

    // set SPI STATUS and start SPI for full duplex (SPI_FD_STATUS)  91.13
	writel(TOTAL_LENGTH(data_len) | TX_LENGTH(data_len),&spim_reg->SPI_FD_STATUS);

	DBG_INFO( "[SPI_FD] set SPI_FD_STATUS =0x%x\n",readl(&spim_reg->SPI_FD_STATUS));

	
        reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= SPI_START_FD;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 

	
	if(!wait_for_completion_timeout(&pspim->isr_done, timeout)){
		DBG_INFO("wait_for_completion_timeout 1215 \n");
		ret = 1;
		goto free_master_combite_rw;
	}

	if (xfer_rx == false){
		ret = 0;
		goto free_master_combite_rw;
	}



	data_len = 0;
	t = first;
	
	for (i = 0; i < transfers_cnt; i++) {
	    if (t->rx_buf){
			//DBG_INFO("txrx1269: tx %p, rx %p, len %d\n", t->tx_buf, t->rx_buf, t->len);
		    memcpy(t->rx_buf, &pspim->rx_data_buf[data_len], t->len);
	    }


		    //DBG_INFO("RXcnt1268 data 0x%x data_len = %d  \n",pspim->rx_data_buf[0],data_len); 
		    //DBG_INFO("RXcnt1268 data 0x%x data_len = %d	\n",pspim->rx_data_buf[1],data_len); 
                    //DBG_INFO("RXcnt1268 data 0x%x data_len = %d	\n",pspim->rx_data_buf[2],data_len); 
                    //DBG_INFO("RXcnt1268 data15 0x%x data_len = %d	\n",pspim->rx_data_buf[15],data_len); 
                    //DBG_INFO("RXcnt1268 data32 0x%x data_len = %d	\n",pspim->rx_data_buf[32],data_len); 

		    //DBG_INFO("RXcnt1269 data 0x%x rx_cur_len = %d \n",pspim->rx_data_buf[data_len],pspim->rx_cur_len); 
		    //DBG_INFO("RXcnt1269 data 0x%x t->len = %d \n",pspim->rx_data_buf[data_len+t->len],t->len); 
	
		    data_len += t->len;
		
			t = list_entry(t->transfer_list.next, struct spi_transfer,
					   transfer_list);
	}

		ret = 0;
		
free_master_combite_rw:
	
    // reset SPI
	reg_temp = readl(&spim_reg->SPI_FD_CONFIG);  
	reg_temp &= CLEAN_FLUG_MASK;
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);
	
	reg_temp = readl(&spim_reg->SPI_FD_STATUS);
	reg_temp |= FD_SW_RST;
	writel(reg_temp, &spim_reg->SPI_FD_STATUS); 

	
	mutex_unlock(&pspim->buf_lock);
		
	return ret;

}



static int pentagram_spi_controller_setup(struct spi_device *spi)
{
	struct device dev = spi->dev;
	struct pentagram_spi_master *pspim = spi_controller_get_devdata(spi->controller);
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;

	unsigned int spi_id;
	unsigned int clk_rate;
	unsigned int div;
	unsigned int clk_sel;
	unsigned int reg_temp;
	unsigned long flags;
	
	dev_dbg(&dev,"%s\n",__FUNCTION__);


       spi_id = pspim->ctlr->bus_num;


        FUNC_DEBUG();

#ifdef CONFIG_PM_RUNTIME_SPI
        if(pm_runtime_enabled(pspim->dev)){
           ret = pm_runtime_get_sync(pspim->dev);
   	   if (ret < 0)
    	   goto pm_out;  
    	}
#endif

     DBG_INFO(" spi_id  = %d\n",spi_id);


	//set clock
	clk_rate = clk_get_rate(pspim->spi_clk);
	div = clk_rate / pspim->spi_max_frequency;

	clk_sel = (div / 2) - 1;
	reg_temp = PENTAGRAM_SPI_SLAVE_SET | ((clk_sel & 0x3fff) << 16);

	spin_lock_irqsave(&pspim->lock, flags);
	writel(reg_temp, &spim_reg->SPI_CTRL_CLKSEL);


	reg_temp = FD_SEL | ((clk_sel & 0xffff) << 16);                     //set up full duplex frequency and enable  full duplex 
	writel(reg_temp, &spim_reg->SPI_FD_CONFIG);

	
	spin_unlock_irqrestore(&pspim->lock, flags);

	dev_dbg(&dev,"clk_sel 0x%x\n",readl(&spim_reg->SPI_CTRL_CLKSEL));
	
	spin_lock_irqsave(&pspim->lock, flags);
	pspim->isr_flag = SPI_IDLE;
	spin_unlock_irqrestore(&pspim->lock, flags);

#ifdef CONFIG_PM_RUNTIME_SPI
         pm_runtime_put(pspim->dev);
#endif
	return 0;

#ifdef CONFIG_PM_RUNTIME_SPI
pm_out:
	pm_runtime_mark_last_busy(pspim->dev);
	pm_runtime_put_autosuspend(pspim->dev);
    DBG_INFO( "pm_out");
	return 0;								  
#endif

}
EXPORT_SYMBOL_GPL(pentagram_spi_controller_setup);


static int pentagram_spi_controller_prepare_message(struct spi_master *master,
				    struct spi_message *msg)
{
	//struct device dev = master->dev;
	FUNC_DEBUG();
	return 0;
}
static int pentagram_spi_controller_unprepare_message(struct spi_master *master,
				    struct spi_message *msg)
{
	//struct device dev = master->dev;
	FUNC_DEBUG();
	return 0;
}

static size_t pentagram_spi_max_length(struct spi_device *spi)
{

    return SPI_MSG_DATA_SIZE;

}


static void pentagram_spi_setup_transfer(struct spi_device *spi, struct spi_controller *ctlr, struct spi_transfer *t)
{
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	//struct device dev = master->dev;
	SPI_MAS* spim_reg = (SPI_MAS *)pspim->mas_base;

	unsigned int reg_temp = 0;
	unsigned int clk_rate;
	unsigned int div;
	unsigned int clk_sel;


	FUNC_DEBUG();
	
	   pspim->tx_cur_len = 0;
	   pspim->rx_cur_len = 0;
	   //memset(tx_data_buf,0,255);
	   //memset(rx_data_buf,0,255);
	
	   //hw->bpw = xfer->bits_per_word;
	   //spi->bits_per_word
	   //  div = mclk_rate / (2 * tfr->speed_hz);
	
	   //dev_dbg(&dev,"%s\n",__FUNCTION__);    
	
	   DBG_INFO( "setup %d bpw, %scpol, %scpha, %scs-high, %slsb-first, %dHz\n",
		   spi->bits_per_word,
		   spi->mode & SPI_CPOL ? "" : "~",
		   spi->mode & SPI_CPHA ? "" : "~",
		   spi->mode & SPI_CS_HIGH ? "" : "~",
		   spi->mode & SPI_LSB_FIRST ? "" : "~",	   
		   spi->max_speed_hz);
	
	
	   DBG_INFO( "transfer_one txrx: tx %p, rx %p, len %d\n",t->tx_buf, t->rx_buf, t->len);
	   DBG_INFO( "transfer_one cs_chang: %d  speed_hz %dHz	 per_word %d bpw \n",t->cs_change,t->speed_hz,t->bits_per_word);
	
	   //set clock
	   clk_rate = clk_get_rate(pspim->spi_clk);
	   
	   if(t->speed_hz <= spi->max_speed_hz){
		  div = clk_rate / t->speed_hz;
	   }else if(spi->max_speed_hz <= ctlr->max_speed_hz){
		  div = clk_rate / spi->max_speed_hz;
	   }else if(ctlr->max_speed_hz < pspim->spi_max_frequency){
		  div = clk_rate / ctlr->max_speed_hz;
	   }else{
		  div = clk_rate / pspim->spi_max_frequency;
	   }
	   
	   DBG_INFO( "clk_rate: %d	div %d \n",clk_rate,div);
	
	   clk_sel = (div / 2) - 1;
	   reg_temp = FD_SEL | ((clk_sel & 0xffff) << 16);					   //set up full duplex frequency and enable  full duplex 
	
	 // if(spi->mode & SPI_CPOL){
	 //	  reg_temp = reg_temp | CPOL_FD;  
	 //  }
	 //  if(spi->mode & SPI_CPHA){
	 //	  reg_temp = reg_temp | CPHA_W | CPHA_R;  
	 //  }

	  if(ctlr->mode_bits & SPI_CPOL){
		  reg_temp = reg_temp | CPOL_FD;  
	   }
	  if(ctlr->mode_bits & SPI_CPHA){
		  reg_temp = reg_temp | CPHA_W | CPHA_R;  
	   }

	  
	   if(spi->mode & SPI_CS_HIGH){
		  reg_temp = reg_temp | CS_POR;  
	   }
	   if(spi->mode & SPI_LSB_FIRST){
		  reg_temp = reg_temp | LSB_SEL;  
	   }

	   writel(reg_temp, &spim_reg->SPI_FD_CONFIG);	
	
	   data_unit = FIFO_DATA_BITS / t->bits_per_word;
	
	   pspim->isr_flag = SPI_IDLE;

	   DBG_INFO( "data_unit %d unit\n",data_unit);

}


					
static int pentagram_spi_controller_transfer_one(struct spi_controller *ctlr, struct spi_device *spi,
					struct spi_transfer *xfer)
{ 

	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);

	//unsigned char *data_buf;
	//unsigned char *cmd_buf;
	const u8 *cmd_buf;
	u8 *data_buf;
	unsigned int len,tx_len;
	//unsigned int temp_reg;
	int mode = SPI_IDLE; 
	int ret;
	//unsigned char *temp;


    FUNC_DEBUG();

	//tx_cur_len = 0;
	//rx_cur_len = 0;
	//memset(tx_data_buf,0,255);
	//memset(rx_data_buf,0,255);

#ifdef CONFIG_PM_RUNTIME_SPI
	if(pm_runtime_enabled(pspim->dev)){
	    ret = pm_runtime_get_sync(pspim->dev);
	    if (ret < 0)
	        goto pm_out;  
	}
#endif


if (spi_controller_is_slave(ctlr)){

	pspim->isr_flag = SPI_IDLE;

	if((xfer->tx_buf)&&(xfer->rx_buf)){
		dev_dbg(&ctlr->dev,"wrong command\n");
	}else if(xfer->tx_buf){
		dev_dbg(&ctlr->dev,"tx\n");
		cmd_buf = xfer->tx_buf;
		len = xfer->len;
		dev_dbg(&ctlr->dev,"len %d\n",len);
		mode = SPI_SLAVE_WRITE;
	}else if(xfer->rx_buf){
		dev_dbg(&ctlr->dev,"rx\n");
		data_buf = xfer->rx_buf;
		len = xfer->len;
		dev_dbg(&ctlr->dev,"len %d\n",len);
		mode = SPI_SLAVE_READ;
	}

    if(mode == SPI_SLAVE_WRITE){
	    pentagram_spi_slave_rw(spi,cmd_buf,data_buf, len, mode);
    }
	else if(mode == SPI_SLAVE_READ){
        pentagram_spi_slave_rw(spi,cmd_buf,data_buf, len, mode);
	}

}
else{

	pentagram_spi_setup_transfer(spi, ctlr, xfer);


	if((xfer->tx_buf)&&(xfer->rx_buf)){
		dev_dbg(&ctlr->dev,"rx\n");
		data_buf = xfer->rx_buf;
		cmd_buf = xfer->tx_buf;
		//temp = xfer->rx_buf;
		//dev_dbg(&master->dev,"rx %x %x\n",*temp,*(temp+1));
		//temp = xfer->tx_buf;
		//dev_dbg(&master->dev,"tx %x %x\n",*temp,*(temp+1));
		len = xfer->len;
		tx_len = 0;
		dev_dbg(&ctlr->dev,"len %d\n",len);
		mode = SPI_MASTER_RW;
	}else if(xfer->tx_buf){
		dev_dbg(&ctlr->dev,"tx\n");
		cmd_buf = xfer->tx_buf;
		//temp = xfer->tx_buf;
		//dev_dbg(&master->dev,"tx %x %x\n",*temp,*(temp+1));
		len = xfer->len;
		dev_dbg(&ctlr->dev,"len %d\n",len);
		mode = SPI_MASTER_WRITE;
	}else if(xfer->rx_buf){
		dev_dbg(&ctlr->dev,"rx\n");
		data_buf = xfer->rx_buf;
		//temp = xfer->rx_buf;
		//dev_dbg(&master->dev,"rx %x %x\n",*temp,*(temp+1));
		len = xfer->len;
		dev_dbg(&ctlr->dev,"len %d\n",len);
		mode = SPI_MASTER_READ;
	}

//	DBG_INFO("sp_spi_combo_master_slave_dma_WR");

//	sp_spi_combo_master_slave_dma_WR(master, cmd_buf, len);

//	return 0;


	if(mode == SPI_MASTER_RW){
		ret = pentagram_spi_master_fullduplex_write_read(ctlr, cmd_buf , data_buf , len , tx_len);
	}else if(mode == SPI_MASTER_WRITE){
		ret = pentagram_spi_master_write(ctlr, cmd_buf , data_buf , len);
	}else if(mode == SPI_MASTER_READ){
		ret = pentagram_spi_master_read(ctlr, cmd_buf , data_buf , len);
	}

}


   spi_finalize_current_transfer(ctlr);


#ifdef CONFIG_PM_RUNTIME_SPI
	pm_runtime_put(pspim->dev);
        DBG_INFO( "pm_put");								  
#endif


	return ret;


#ifdef CONFIG_PM_RUNTIME_SPI
pm_out:
	pm_runtime_mark_last_busy(pspim->dev);
	pm_runtime_put_autosuspend(pspim->dev);
    DBG_INFO( "pm_out");
	return 0;								  
#endif





	//if(mode == SPI_MASTER_WRITE)
	//{
	//	ret = pentagram_spi_master_dma_write(master, data_buf, len);
	//}else if(mode == SPI_MASTER_READ)
	//{
	//	ret = pentagram_spi_master_dma_read(master, cmd_buf, len);
	//	if(ret == 0)
	//		memcpy(data_buf, pspim->rx_dma_vir_base, len);
	//}

	//return ret;
}



static int pentagram_spi_controller_transfer_one_message(struct spi_controller *ctlr, struct spi_message *m)
{ 

	//struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);
	struct spi_device *spi = m->spi;

	unsigned int xfer_cnt = 0, total_len = 0;
	bool start_xfer;
	struct spi_transfer *xfer,*first_xfer = NULL;
	int ret;

	//struct spi_transfer *next_xfer,

	FUNC_DEBUG();

	//dev_dbg(&master->dev,"%s\n",__FUNCTION__);	

	start_xfer = false;

#ifdef CONFIG_PM_RUNTIME_SPI
	if(pm_runtime_enabled(pspim->dev)){
	    ret = pm_runtime_get_sync(pspim->dev);
	    if (ret < 0)
	        goto pm_out;  
	}
#endif


	list_for_each_entry(xfer, &m->transfers, transfer_list) {
	
		if(!first_xfer)
		first_xfer = xfer;

        total_len +=  xfer->len; 

		DBG_INFO("first_xfer1613: tx %p, rx %p, len %d\n", first_xfer->tx_buf, first_xfer->rx_buf, first_xfer->len);
		DBG_INFO("xfer1614: tx %p, rx %p, len %d\n", xfer->tx_buf, xfer->rx_buf, xfer->len);

		/* all combined transfers have to have the same speed */
		if (first_xfer->speed_hz != xfer->speed_hz) {
			DBG_INFO( "unable to change speed between transfers\n");
			ret = -EINVAL;
			goto exit;
		}

		/* CS will be deasserted directly after transfer */
		if (xfer->delay_usecs) {
			DBG_INFO( "can't keep CS asserted after transfer\n");
			ret = -EINVAL;
			goto exit;
		}

		if (xfer->len > SPI_MSG_DATA_SIZE) {
			DBG_INFO( "over total transfer length \n");
			ret = -EINVAL;
			goto exit;
	}


		if (list_is_last(&xfer->transfer_list, &m->transfers))
			DBG_INFO("xfer = transfer_list \n" );

		if ((total_len > SPI_MSG_DATA_SIZE))
			DBG_INFO("(total_len > SPI_MSG_DATA_SIZE) \n" );

		if (xfer->cs_change)
			DBG_INFO("xfer->cs_change \n" );



        if (list_is_last(&xfer->transfer_list, &m->transfers) || (total_len > SPI_MSG_DATA_SIZE) 
			|| xfer->cs_change){
			start_xfer = true;
			if (total_len < SPI_MSG_DATA_SIZE)
			xfer_cnt++;
#if(0)	// for test		
		//}else if((xfer_cnt > 0) && (xfer->rx_buf)){
		//    next_xfer = list_entry(xfer->transfer_list.next, struct spi_transfer,
		// 	       transfer_list);
		//	if(next_xfer->tx_buf){
		//	  start_xfer = true;
		//	  xfer_cnt++;			  
		//	}
#endif
		}
		

		if(start_xfer == true){

	        pentagram_spi_setup_transfer(spi, ctlr, first_xfer);

		    DBG_INFO("start_xfer  xfer->len ,xfer_cnt = %d \n",xfer->len,xfer_cnt );

            ret = pentagram_spi_master_combine_write_read(ctlr,first_xfer,xfer_cnt);

			if (total_len > SPI_MSG_DATA_SIZE)
			ret = pentagram_spi_master_combine_write_read(ctlr,xfer,1);

			m->actual_length += total_len;

			first_xfer = NULL;
			xfer_cnt = 0;
			total_len = 0;
			start_xfer = false;
	
		}else{
			xfer_cnt++;
		}

	}

	exit:
		m->status = ret;
		spi_finalize_current_message(ctlr);

#ifdef CONFIG_PM_RUNTIME_SPI
	pm_runtime_put(pspim->dev);
    DBG_INFO( "pm_put");								  
#endif


	return ret;


#ifdef CONFIG_PM_RUNTIME_SPI
pm_out:
	pm_runtime_mark_last_busy(pspim->dev);
	pm_runtime_put_autosuspend(pspim->dev);
    DBG_INFO( "pm_out");
	return 0;								  
#endif
	


}





static int pentagram_spi_controller_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	int mode;	
	unsigned int max_freq;
	//struct spi_master *master;
	struct spi_controller *ctlr;
	struct pentagram_spi_master *pspim;	


    FUNC_DEBUG();

	if (pdev->dev.of_node) {
		pdev->id = of_alias_get_id(pdev->dev.of_node, "spi");
		mode = of_property_read_bool(pdev->dev.of_node, "spi-slave") ? SPI_SLAVE : SPI_MASTER;
	}

    DBG_INFO(" pdev->id  = %d\n",pdev->id);
    ///DBG_INFO(" pdev->dev.of_node  = %d\n",pdev->dev.of_node);



	if (mode == SPI_SLAVE){
		ctlr = spi_alloc_slave(&pdev->dev, sizeof(*pspim));
		DBG_INFO("spi_alloc_slave of_node  = %d\n",pdev->dev.of_node);
	}
	else{
		ctlr = spi_alloc_master(&pdev->dev, sizeof(*pspim));
    	//master = spi_alloc_master(&pdev->dev, sizeof(*pspim));
    	DBG_INFO("spi_alloc_master of_node  = %d\n",pdev->dev.of_node);
	}

	if (!ctlr) {
		dev_err(&pdev->dev,"spi_alloc fail\n");
		return -ENODEV;
	}


	//ctlr->auto_runtime_pm = true;
	/* setup the master state. */
	ctlr->mode_bits = SPI_MODE_1 ;
	ctlr->bus_num = pdev->id;
	//master->setup = pentagram_spi_controller_setup;
	ctlr->prepare_message = pentagram_spi_controller_prepare_message;
	ctlr->unprepare_message = pentagram_spi_controller_unprepare_message;

	if (mode == SPI_SLAVE){
		ctlr->transfer_one = pentagram_spi_controller_transfer_one;
   	    ctlr->slave_abort = pentagram_spi_abort;
	}
	else{
		ctlr->transfer_one_message = pentagram_spi_controller_transfer_one_message;
	}

	ctlr->max_transfer_size = pentagram_spi_max_length;
	ctlr->max_message_size = pentagram_spi_max_length;
	ctlr->num_chipselect = 1;
	ctlr->dev.of_node = pdev->dev.of_node;
	ctlr->max_speed_hz = 50000000;

	platform_set_drvdata(pdev, ctlr);
	pspim = spi_controller_get_devdata(ctlr);

	pspim->ctlr = ctlr;
	pspim->dev = &pdev->dev;
	if(!of_property_read_u32(pdev->dev.of_node, "spi-max-frequency", &max_freq)) {
		dev_dbg(&pdev->dev,"max_freq %d\n",max_freq);
		pspim->spi_max_frequency = max_freq;
	}else
		pspim->spi_max_frequency = 25000000;

	spin_lock_init(&pspim->lock);
	mutex_init(&pspim->buf_lock);
	init_completion(&pspim->isr_done);
	init_completion(&pspim->sla_isr);



	/* find and map our resources */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, MAS_REG_NAME);
	if (res) {
		pspim->mas_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(pspim->mas_base)) {
			dev_err(&pdev->dev,"%s devm_ioremap_resource fail\n",MAS_REG_NAME);
			goto free_alloc;
		}
	}
	dev_dbg(&pdev->dev,"mas_base 0x%x\n",(unsigned int)pspim->mas_base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, SLA_REG_NAME);
	if (res) {
		pspim->sla_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(pspim->sla_base)) {
			dev_err(&pdev->dev,"%s devm_ioremap_resource fail\n",SLA_REG_NAME);
			goto free_alloc;
		}
	}
	dev_dbg(&pdev->dev,"sla_base 0x%x\n",(unsigned int)pspim->sla_base);


#if(0)
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, PIN_MUX_MAS_REG_NAME);
	if (res) {
		pspim->sft_base = devm_ioremap(&pdev->dev, res->start, resource_size(res) );
		if (IS_ERR(pspim->sft_base)) {
			dev_err(&pdev->dev,"%s devm_ioremap_resource fail\n",PIN_MUX_MAS_REG_NAME);
			goto free_alloc;
		}
	}
	dev_dbg(&pdev->dev,"sft_base 0x%x\n",(unsigned int)pspim->sft_base);


	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, PIN_MUX_SLA_REG_NAME);
	if (res) {
		pspim->sft3_base = devm_ioremap(&pdev->dev, res->start, resource_size(res) );
		if (IS_ERR(pspim->sft_base)) {
			dev_err(&pdev->dev,"%s devm_ioremap_resource fail\n",PIN_MUX_SLA_REG_NAME);
			goto free_alloc;
		}
	}
	dev_dbg(&pdev->dev,"sft3_base 0x%x\n",(unsigned int)pspim->sft3_base);

#endif
	/* irq*/
	pspim->dma_irq = platform_get_irq_byname(pdev, DMA_IRQ_NAME);
	if (pspim->dma_irq < 0) {
		dev_err(&pdev->dev, "failed to get %s\n", DMA_IRQ_NAME);
		goto free_alloc;
	}

	pspim->mas_irq = platform_get_irq_byname(pdev, MAS_IRQ_NAME);
	if (pspim->mas_irq < 0) {
		dev_err(&pdev->dev, "failed to get %s\n", MAS_IRQ_NAME);
		goto free_alloc;
	}


	pspim->sla_irq = platform_get_irq_byname(pdev, SLA_IRQ_NAME);
	if (pspim->sla_irq < 0) {
		dev_err(&pdev->dev, "failed to get %s\n", SLA_IRQ_NAME);
		goto free_alloc;
	}

	/* requset irq*/
	ret = devm_request_irq(&pdev->dev, pspim->dma_irq, pentagram_spi_master_dma_irq
						, IRQF_TRIGGER_RISING, pdev->name, pspim);
	if (ret) {
		dev_err(&pdev->dev, "%s devm_request_irq fail\n", DMA_IRQ_NAME);
		goto free_alloc;
	}

	ret = devm_request_irq(&pdev->dev, pspim->mas_irq, pentagram_spi_master_mas_irq
						, IRQF_TRIGGER_RISING, pdev->name, pspim);
	if (ret) {
		dev_err(&pdev->dev, "%s devm_request_irq fail\n", MAS_IRQ_NAME);
		goto free_alloc;
	}


	ret = devm_request_irq(&pdev->dev, pspim->sla_irq, pentagram_spi_slave_sla_irq
						, IRQF_TRIGGER_RISING, pdev->name, pspim);
	if (ret) {
		dev_err(&pdev->dev, "%s devm_request_irq fail\n", SLA_IRQ_NAME);
		goto free_alloc;
	}

	/* clk*/
	pspim->spi_clk = devm_clk_get(&pdev->dev,NULL);
	if(IS_ERR(pspim->spi_clk)) {
		dev_err(&pdev->dev, "devm_clk_get fail\n");
		goto free_alloc;
	}
	ret = clk_prepare_enable(pspim->spi_clk);
	if (ret)
		goto free_alloc;

	/* reset*/
	pspim->rstc = devm_reset_control_get(&pdev->dev, NULL);
	DBG_INFO( "pspim->rstc : 0x%x \n",pspim->rstc);
	if (IS_ERR(pspim->rstc)) {
		ret = PTR_ERR(pspim->rstc);
		dev_err(&pdev->dev, "SPI failed to retrieve reset controller: %d\n", ret);
		goto free_clk;
	}
	ret = reset_control_deassert(pspim->rstc);
	if (ret)
		goto free_clk;

	/* dma alloc*/
	pspim->tx_dma_vir_base = dma_alloc_coherent(&pdev->dev, bufsiz,
					&pspim->tx_dma_phy_base, GFP_ATOMIC);
	if(!pspim->tx_dma_vir_base) {
		dev_err(&pdev->dev, "dma_alloc_coherent fail\n");
		goto free_reset_assert;
	}
	dev_dbg(&pdev->dev, "tx_dma vir 0x%x\n",(unsigned int)pspim->tx_dma_vir_base);
	dev_dbg(&pdev->dev, "tx_dma phy 0x%x\n",(unsigned int)pspim->tx_dma_phy_base);

	pspim->rx_dma_vir_base = dma_alloc_coherent(&pdev->dev, bufsiz,
					&pspim->rx_dma_phy_base, GFP_ATOMIC);
	if(!pspim->rx_dma_vir_base) {
		dev_err(&pdev->dev, "dma_alloc_coherent fail\n");
		goto free_tx_dma;
	}
	dev_dbg(&pdev->dev, "rx_dma vir 0x%x\n",(unsigned int)pspim->rx_dma_vir_base);
	dev_dbg(&pdev->dev, "rx_dma phy 0x%x\n",(unsigned int)pspim->rx_dma_phy_base);

	
	//ret = spi_register_master(master);
	ret = devm_spi_register_controller(&pdev->dev, ctlr);
	if (ret != 0) {
		dev_err(&pdev->dev, "spi_register_master fail\n");
		goto free_rx_dma;
	}

#ifdef CONFIG_PM_RUNTIME_SPI
	pm_runtime_set_autosuspend_delay(&pdev->dev,5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	DBG_INFO(" CONFIG_PM_RUNTIME_SPI init \n");
#endif

	
	return 0;

free_rx_dma:
	dma_free_coherent(&pdev->dev, bufsiz, pspim->rx_dma_vir_base, pspim->rx_dma_phy_base);
free_tx_dma:
	dma_free_coherent(&pdev->dev, bufsiz, pspim->tx_dma_vir_base, pspim->tx_dma_phy_base);
free_reset_assert:
	reset_control_assert(pspim->rstc);
free_clk:
	clk_disable_unprepare(pspim->spi_clk);
free_alloc:
	//spi_master_put(master);
	spi_controller_put(ctlr);

	dev_dbg(&pdev->dev, "spi_master_probe done\n");
	return ret;
}

static int pentagram_spi_controller_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct pentagram_spi_master *pspim = spi_master_get_devdata(master);

    FUNC_DEBUG();

#ifdef CONFIG_PM_RUNTIME_SPI
	  pm_runtime_disable(&pdev->dev);
	  pm_runtime_set_suspended(&pdev->dev);
#endif

	dma_free_coherent(&pdev->dev, bufsiz, pspim->tx_dma_vir_base, pspim->tx_dma_phy_base);
	dma_free_coherent(&pdev->dev, bufsiz, pspim->rx_dma_vir_base, pspim->rx_dma_phy_base);


	spi_unregister_master(pspim->ctlr);
	clk_disable_unprepare(pspim->spi_clk);
	reset_control_assert(pspim->rstc);

	return 0;
	
}

static int pentagram_spi_controller_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spi_controller *ctlr = platform_get_drvdata(pdev);
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);


    FUNC_DEBUG();

	reset_control_assert(pspim->rstc);


	return 0;
	
}

static int pentagram_spi_controller_resume(struct platform_device *pdev)
{
	
	struct spi_controller *ctlr = platform_get_drvdata(pdev);
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);

    FUNC_DEBUG();
	
	reset_control_deassert(pspim->rstc);
	clk_prepare_enable(pspim->spi_clk);


	return 0;
	
}


#ifdef CONFIG_PM_RUNTIME_SPI
static int sp_spi_runtime_suspend(struct device *dev)
{
	struct spi_controller *ctlr = platform_get_drvdata(pdev);
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);


    FUNC_DEBUG();

	//DBG_INFO( "runtime_suspend_dev id = %s %d\n",dev->init_name,dev->id);

	reset_control_assert(pspim->rstc);

	return 0;

}

static int sp_spi_runtime_resume(struct device *dev)
{
	struct spi_controller *ctlr = platform_get_drvdata(pdev);
	struct pentagram_spi_master *pspim = spi_master_get_devdata(ctlr);


    FUNC_DEBUG();

	//DBG_INFO( "runtime_resume_dev id = %s %d\n",dev->init_name,dev->id);

	reset_control_deassert(pspim->rstc);
	clk_prepare_enable(pspim->spi_clk);

	return 0;

}

static const struct dev_pm_ops sp7021_spi_pm_ops = {
	.runtime_suspend = sp_spi_runtime_suspend,
	.runtime_resume  = sp_spi_runtime_resume,
};

#define sp_spi_pm_ops  (&sp7021_spi_pm_ops)
#endif


static const struct of_device_id pentagram_spi_controller_ids[] = {
	{.compatible = "sunplus,sp7021-spi-controller"},
	{}
};
MODULE_DEVICE_TABLE(of, pentagram_spi_controller_ids);



static struct platform_driver pentagram_spi_controller_driver = {
	.probe = pentagram_spi_controller_probe,
	.remove = pentagram_spi_controller_remove,
	.suspend	= pentagram_spi_controller_suspend,
	.resume		= pentagram_spi_controller_resume,	
	.driver = {
		.name = "sunplus,sp7021-spi-controller",
		.of_match_table = pentagram_spi_controller_ids,
	#ifdef CONFIG_PM_RUNTIME_SPI
		.pm     = sp_spi_pm_ops,
    #endif		
	},
};
module_platform_driver(pentagram_spi_controller_driver);

MODULE_AUTHOR("Sunplus");
MODULE_DESCRIPTION("Sunplus SPI controller driver");
MODULE_LICENSE("GPL");

