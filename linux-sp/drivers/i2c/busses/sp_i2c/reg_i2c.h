#ifndef __REG_I2C_H__
#define __REG_I2C_H__

#include <mach/io_map.h>

/****************************************
* I2C Master
****************************************/

//control0
#define I2C_CTL0_FREQ(x)                  (x<<24)  //bit[26:24]
#define I2C_CTL0_PREFETCH                 (1<<18)  //Now as read mode need to set high, otherwise don¡¦t care
#define I2C_CTL0_RESTART_EN               (1<<17)  //0:disable 1:enable
#define I2C_CTL0_SUBADDR_EN               (1<<16)  //For restart mode need to set high
#define I2C_CTL0_SW_RESET                 (1<<15)
#define I2C_CTL0_SLAVE_ADDR(x)            (x<<1)   //bit[7:1]

//control1
#define I2C_CTL1_ALL_CLR                  (0x3FF)
#define I2C_CTL1_EMPTY_CLR                (1<<9)
#define I2C_CTL1_SCL_HOLD_TOO_LONG_CLR    (1<<8)
#define I2C_CTL1_SCL_WAIT_CLR             (1<<7)
#define I2C_CTL1_EMPTY_THRESHOLD_CLR      (1<<6)
#define I2C_CTL1_DATA_NACK_CLR            (1<<5)
#define I2C_CTL1_ADDRESS_NACK_CLR         (1<<4)
#define I2C_CTL1_BUSY_CLR                 (1<<3)
#define I2C_CTL1_CLKERR_CLR               (1<<2)
#define I2C_CTL1_DONE_CLR                 (1<<1)
#define I2C_CTL1_SIFBUSY_CLR              (1<<0)

//control2
#define I2C_CTL2_FREQ_CUSTOM(x)           (x<<0)   //bit[10:0]
#define I2C_CTL2_SCL_DELAY(x)             (x<<24)  //bit[25:24]
#define I2C_CTL2_SDA_HALF_ENABLE          (1<<31)

//control6
#define I2C_CTL6_BURST_RDATA_CLR          I2C_EN1_BURST_RDATA_INT

//control7
#define I2C_CTL7_RDCOUNT(x)               (x<<16)  //bit[31:16]
#define I2C_CTL7_WRCOUNT(x)               (x<<0)   //bit[15:0]

//interrupt
#define I2C_INT_RINC_INDEX(x)             (x<<18)  //bit[20:18]
#define I2C_INT_WINC_INDEX(x)             (x<<15)  //bit[17:15]
#define I2C_INT_SCL_HOLD_TOO_LONG_FLAG    (1<<11)
#define I2C_INT_WFIFO_ENABLE              (1<<10)
#define I2C_INT_FULL_FLAG                 (1<<9)
#define I2C_INT_EMPTY_FLAG                (1<<8)
#define I2C_INT_SCL_WAIT_FLAG             (1<<7)
#define I2C_INT_EMPTY_THRESHOLD_FLAG      (1<<6)
#define I2C_INT_DATA_NACK_FLAG            (1<<5)
#define I2C_INT_ADDRESS_NACK_FLAG         (1<<4)
#define I2C_INT_BUSY_FLAG                 (1<<3)
#define I2C_INT_CLKERR_FLAG               (1<<2)
#define I2C_INT_DONE_FLAG                 (1<<1)
#define I2C_INT_SIFBUSY_FLAG              (1<<0)

//interrupt enable0
#define I2C_EN0_SCL_HOLD_TOO_LONG_INT     (1<<13)
#define I2C_EN0_NACK_INT                  (1<<12)
#define I2C_EN0_CTL_EMPTY_THRESHOLD(x)    (x<<9)  //bit[11:9]
#define I2C_EN0_EMPTY_INT                 (1<<8)
#define I2C_EN0_SCL_WAIT_INT              (1<<7)
#define I2C_EN0_EMPTY_THRESHOLD_INT       (1<<6)
#define I2C_EN0_DATA_NACK_INT             (1<<5)
#define I2C_EN0_ADDRESS_NACK_INT          (1<<4)
#define I2C_EN0_BUSY_INT                  (1<<3)
#define I2C_EN0_CLKERR_INT                (1<<2)
#define I2C_EN0_DONE_INT                  (1<<1)
#define I2C_EN0_SIFBUSY_INT               (1<<0)

//interrupt enable1
#define I2C_EN1_BURST_RDATA_INT           (0x80008000)  //must sync with GET_BYTES_EACHTIME

//interrupt enable2
#define I2C_EN2_BURST_RDATA_OVERFLOW_INT  (0xFFFFFFFF)

//i2c master mode
#define I2C_MODE_DMA_MODE                 (1<<2)
#define I2C_MODE_MANUAL_MODE              (1<<1)  //0:trigger mode 1:auto mode
#define I2C_MODE_MANUAL_TRIG              (1<<0)

//i2c master status2
#define I2C_SW_RESET_DONE                 (1<<0)


/****************************************
* GDMA
****************************************/

//dma config
#define I2C_DMA_CFG_DMA_GO                (1<<8)
#define I2C_DMA_CFG_NON_BUF_MODE          (1<<2)
#define I2C_DMA_CFG_SAME_SLAVE            (1<<1)
#define I2C_DMA_CFG_DMA_MODE              (1<<0)

//dma interrupt flag
#define I2C_DMA_INT_LENGTH0_FLAG          (1<<6)
#define I2C_DMA_INT_THRESHOLD_FLAG        (1<<5)
#define I2C_DMA_INT_IP_TIMEOUT_FLAG       (1<<4)
#define I2C_DMA_INT_GDMA_TIMEOUT_FLAG     (1<<3)
#define I2C_DMA_INT_WB_EN_ERROR_FLAG      (1<<2)
#define I2C_DMA_INT_WCNT_ERROR_FLAG       (1<<1)
#define I2C_DMA_INT_DMA_DONE_FLAG         (1<<0)

//dma interrupt enable
#define I2C_DMA_EN_LENGTH0_INT            (1<<6)
#define I2C_DMA_EN_THRESHOLD_INT          (1<<5)
#define I2C_DMA_EN_IP_TIMEOUT_INT         (1<<4)
#define I2C_DMA_EN_GDMA_TIMEOUT_INT       (1<<3)
#define I2C_DMA_EN_WB_EN_ERROR_INT        (1<<2)
#define I2C_DMA_EN_WCNT_ERROR_INT         (1<<1)
#define I2C_DMA_EN_DMA_DONE_INT           (1<<0)


/****************************************
* SG GDMA
****************************************/

//sg dma index
#define I2C_SG_DMA_LLI_RUN_INDEX(x)       (x<<8)  //bit[12:8]
#define I2C_SG_DMA_LLI_ACCESS_INDEX(x)    (x<<0)  //bit[4:0]

//sg dma config
#define I2C_SG_DMA_CFG_LAST_LLI           (1<<2)
#define I2C_SG_DMA_CFG_DMA_MODE           (1<<0)

//sg dma setting
#define I2C_SG_DMA_SET_DMA_ENABLE         (1<<31)
#define I2C_SG_DMA_SET_DMA_GO             (1<<0)


typedef struct regs_i2cm_t_ {
	volatile unsigned int control0;      /* 00 */
	volatile unsigned int control1;      /* 01 */
	volatile unsigned int control2;      /* 02 */
	volatile unsigned int control3;      /* 03 */
	volatile unsigned int control4;      /* 04 */
	volatile unsigned int control5;      /* 05 */
	volatile unsigned int i2cm_status0;  /* 06 */
	volatile unsigned int interrupt;     /* 07 */
	volatile unsigned int int_en0;       /* 08 */
	volatile unsigned int i2cm_mode;     /* 09 */
	volatile unsigned int i2cm_status1;  /* 10 */
	volatile unsigned int i2cm_status2;  /* 11 */
	volatile unsigned int control6;      /* 12 */
	volatile unsigned int int_en1;       /* 13 */
	volatile unsigned int i2cm_status3;  /* 14 */
	volatile unsigned int i2cm_status4;  /* 15 */
	volatile unsigned int int_en2;       /* 16 */
	volatile unsigned int control7;      /* 17 */
	volatile unsigned int control8;      /* 18 */
	volatile unsigned int control9;      /* 19 */
	volatile unsigned int reserved[3];   /* 20~22 */
	volatile unsigned int version;       /* 23 */
	volatile unsigned int data00_03;     /* 24 */
	volatile unsigned int data04_07;     /* 25 */
	volatile unsigned int data08_11;     /* 26 */
	volatile unsigned int data12_15;     /* 27 */
	volatile unsigned int data16_19;     /* 28 */
	volatile unsigned int data20_23;     /* 29 */
	volatile unsigned int data24_27;     /* 30 */
	volatile unsigned int data28_31;     /* 31 */
} regs_i2cm_t;

typedef	struct regs_i2cm_gdma_s{
	volatile unsigned int hw_version;                /* 00 */
	volatile unsigned int dma_config;                /* 01 */
	volatile unsigned int dma_length;                /* 02 */
	volatile unsigned int dma_addr;                  /* 03 */
	volatile unsigned int port_mux;                  /* 04 */
	volatile unsigned int int_flag;                  /* 05 */
	volatile unsigned int int_en;                    /* 06 */
	volatile unsigned int sw_reset_state;            /* 07 */
	volatile unsigned int reserved[2];               /* 08~09 */
	volatile unsigned int sg_dma_index;              /* 10 */
	volatile unsigned int sg_dma_config;             /* 11 */
	volatile unsigned int sg_dma_length;             /* 12 */
	volatile unsigned int sg_dma_addr;               /* 13 */
	volatile unsigned int reserved2;                 /* 14 */
	volatile unsigned int sg_setting;                /* 15 */
	volatile unsigned int threshold;                 /* 16 */
	volatile unsigned int reserved3;                 /* 17 */
	volatile unsigned int gdma_read_timeout;         /* 18 */
	volatile unsigned int gdma_write_timeout;        /* 19 */
	volatile unsigned int ip_read_timeout;           /* 20 */
	volatile unsigned int ip_write_timeout;          /* 21 */
	volatile unsigned int write_cnt_debug;           /* 22 */
	volatile unsigned int w_byte_en_debug;           /* 23 */
	volatile unsigned int sw_reset_write_cnt_debug;  /* 24 */
	volatile unsigned int reserved4[7];              /* 25~31 */
}regs_i2cm_gdma_t;

#endif /* __REG_I2C_H__ */
