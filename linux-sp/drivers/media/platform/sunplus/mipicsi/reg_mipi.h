#ifndef __REG_MIPI_H__
#define __REG_MIPI_H__

/****************************************
* MIPI I2C Register Definition
****************************************/
#define ISP_BASE_ADDRESS 0x2000

// 0x276A
#define ISP_2601_TRANS_MODE        (1<<0)


/* mipi-ispapb registers */
struct mipi_isp_reg {
	volatile unsigned char reg[0x1300];         /* 0x2000 ~ 0x32FF */
};

#if 0 // Original ISP register structure
struct mipi_isp_reg {
	volatile unsigned char global[0x100];               /* 0x2000 ~ 0x20FF Global Registers                       */
	volatile unsigned char cdsp[0x100];                 /* 0x2100 ~ 0x21FF CDSP Registers                         */
	volatile unsigned char cdsp_win[0x100];             /* 0x2200 ~ 0x22FF CDSP Window Register                   */
	volatile unsigned char reserved1[0x300];            /* 0x2300 ~ 0x25FF Reserved 1                             */
	volatile unsigned char ssi[0x100];                  /* 0x2600 ~ 0x26FF Synchronous Serial Interface Registers */
	volatile unsigned char sensor_if[0x100];            /* 0x2700 ~ 0x27FF CMOS Sensor Interface Registers        */
	volatile unsigned char reserved2[0x900];            /* 0x2800 ~ 0x30FF Reserved 2                             */
	volatile unsigned char new_cdsp[0x100];             /* 0x3100 ~ 0x31FF New CDSP Registers                     */
	volatile unsigned char new_cdsp_win[0x100];         /* 0x3200 ~ 0x32FF New CDSP Window Registers              */
};
#endif


/****************************************
* MIPI  Register Definition
****************************************/
// 166.0 CSIIW latch mode (csiiw_latch_mode)
#define CSIIW_LATCH_MODE_LATCH_EN_OFS   (0)
#define LATCH_EN_MASK                   (0x1<<CSIIW_LATCH_MODE_LATCH_EN_OFS)
#define LATCH_EN(x)                     ((x<<CSIIW_LATCH_MODE_LATCH_EN_OFS)&LATCH_EN_MASK)
#define LATCH_EN_ENA                    LATCH_EN(1)
#define LATCH_EN_DIS                    LATCH_EN(0)

// 166.1 CSIIW config register 0 (csiiw_config0)
#define CSIIW_CONFIG0_IRQ_MASK_FE_OFS   (17)
#define IRQ_MASK_FE_MASK                (0x1<<CSIIW_CONFIG0_IRQ_MASK_FE_OFS)
#define IRQ_MASK_FE(x)                  ((x<<CSIIW_CONFIG0_IRQ_MASK_FE_OFS)&IRQ_MASK_FE_MASK)
#define IRQ_MASK_FE_ENA                 IRQ_MASK_FE(0)
#define IRQ_MASK_FE_DIS                 IRQ_MASK_FE(1)
#define CSIIW_CONFIG0_IRQ_MASK_FS_OFS   (16)
#define IRQ_MASK_FS_MASK                (0x1<<CSIIW_CONFIG0_IRQ_MASK_FS_OFS)
#define IRQ_MASK_FS(x)                  ((x<<CSIIW_CONFIG0_IRQ_MASK_FS_OFS)&IRQ_MASK_FS_MASK)
#define IRQ_MASK_FS_ENA                 IRQ_MASK_FS(0)
#define IRQ_MASK_FS_DIS                 IRQ_MASK_FS(1)
#define CSIIW_CONFIG0_CMD_URGENT_TH_OFS (12)    // bit 15:12, 4 bits
#define CMD_URGENT_TH_MASK              (0xf<<CSIIW_CONFIG0_CMD_URGENT_TH_OFS)
#define CMD_URGENT_TH(x)                ((x<<CSIIW_CONFIG0_CMD_URGENT_TH_OFS)&CMD_URGENT_TH_MASK)
#define CSIIW_CONFIG0_CMD_QUEUE_OFS     (8)     // bit 10:8, 3 bits
#define CMD_QUEUE_MASK                  (0x7<<CSIIW_CONFIG0_CMD_QUEUE_OFS)
#define CMD_QUEUE(x)                    ((x<<CSIIW_CONFIG0_CMD_QUEUE_OFS)&CMD_QUEUE_MASK)
#define CSIIW_CONFIG0_CSIIW_EN_OFS      (0)
#define CSIIW_EN_MASK                   (0x1<<CSIIW_CONFIG0_CSIIW_EN_OFS)
#define CSIIW_EN(x)                     ((x<<CSIIW_CONFIG0_CSIIW_EN_OFS)&CSIIW_EN_MASK)
#define CSIIW_EN_ENA                    CSIIW_EN(1)
#define CSIIW_EN_DIS                    CSIIW_EN(0)

// 166.2 CSIIW base address (csiiw_base_addr)
#define CSIIW_BASE_ADDR_BASE_ADDR_OFS   (8)
#define BASE_ADDR_MASK                  (0xFFFFFF<<CSIIW_BASE_ADDR_BASE_ADDR_OFS)
#define BASE_ADDR(x)                    ((x)&BASE_ADDR_MASK)

// 166.3 CSIIW line stride (csiiw_stride)
#define CSIIW_STRIDE_LINE_STRIDE_OFS    (4)
#define LINE_STRIDE_MASK                (0x3FF<<CSIIW_STRIDE_LINE_STRIDE_OFS)
#define LINE_STRIDE(x)                  ((x)&LINE_STRIDE_MASK)

// 166.4 CSIIW frame size (csiiw_frame_size)
#define CSIIW_FRAME_SIZE_YLEN_OFS       (16)    // bit 27:16, 12 bits
#define YLEN_MASK                       (0xFFF<<CSIIW_FRAME_SIZE_YLEN_OFS)
#define YLEN(x)                         ((x<<CSIIW_FRAME_SIZE_YLEN_OFS)&YLEN_MASK)
#define CSIIW_FRAME_SIZE_XLEN_OFS       (0)     // bit 12:0, 13 bits
#define XLEN_MASK                       (0x1FFF<<CSIIW_FRAME_SIZE_XLEN_OFS)
#define XLEN(x)                         ((x<<CSIIW_FRAME_SIZE_XLEN_OFS)&XLEN_MASK)

// 166.5 CSIIW frame buffer rorate (csiiw_frame_buf)
#define CSIIW_FRAME_BUF_BUF_ROTATE_DEBUG_OFS    (28)
#define BUF_ROTATE_DEBUG_MASK                   (0x1<<CSIIW_FRAME_BUF_BUF_ROTATE_DEBUG_OFS)
#define BUF_ROTATE_DEBUG(x)                     ((x<<CSIIW_FRAME_BUF_BUF_ROTATE_DEBUG_OFS)&BUF_ROTATE_DEBUG_MASK)
#define BUF_ROTATE_DEBUG_ENA                    BUF_ROTATE_DEBUG(1)
#define BUF_ROTATE_DEBUG_DIS                    BUF_ROTATE_DEBUG(0)
#define CSIIW_FRAME_BUF_BUF_ROTATE_NUM_OFS      (24)     // bit 26:24, 3 bits
#define BUF_ROTATE_NUM_MASK                     (0x7<<CSIIW_FRAME_BUF_BUF_ROTATE_NUM_OFS)
#define BUF_ROTATE_NUM(x)                       ((x<<CSIIW_FRAME_BUF_BUF_ROTATE_NUM_OFS)&BUF_ROTATE_NUM_MASK)
#define CSIIW_FRAME_BUF_ADDR_OFFSET_OFS         (8)     // bit 23:8, 16 bits
#define ADDR_OFFSET_MASK                        (0xFFFF<<CSIIW_FRAME_BUF_ADDR_OFFSET_OFS)
#define ADDR_OFFSET(x)                          ((x)&ADDR_OFFSET_MASK)

// 166.9 CSIIW config register 2 (csiiw_config2)
#define CSIIW_CONFIG2_OUTPUT_MODE_OFS   (8)
#define OUTPUT_MODE_MASK                (0x1<<CSIIW_CONFIG2_OUTPUT_MODE_OFS)
#define OUTPUT_MODE(x)                  ((x<<CSIIW_CONFIG2_OUTPUT_MODE_OFS)&OUTPUT_MODE_MASK)
#define OUTPUT_MODE_ENA                 OUTPUT_MODE(1)
#define OUTPUT_MODE_DIS                 OUTPUT_MODE(0)
#define CSIIW_CONFIG2_DATA_TYPE_OFS     (4)     // bit 6:4, 3 bits
#define DATA_TYPE_MASK                  (0x7<<CSIIW_CONFIG2_DATA_TYPE_OFS)
#define DATA_TYPE(x)                    ((x<<CSIIW_CONFIG2_DATA_TYPE_OFS)&DATA_TYPE_MASK)
#define DATA_TYPE_YUY2                  DATA_TYPE(0)
#define DATA_TYPE_RAW8                  DATA_TYPE(1)
#define DATA_TYPE_RAW10                 DATA_TYPE(2)
#define CSIIW_CONFIG2_MASTER_EN_OFS     (2)
#define MASTER_EN_MASK                  (0x1<<CSIIW_CONFIG2_MASTER_EN_OFS)
#define MASTER_EN(x)                    ((x<<CSIIW_CONFIG2_MASTER_EN_OFS)&MASTER_EN_MASK)
#define MASTER_EN_ENA                   MASTER_EN(1)
#define MASTER_EN_DIS                   MASTER_EN(0)
#define CSIIW_CONFIG2_YCSWAP_EN_OFS     (1)
#define YCSWAP_EN_MASK                  (0x1<<CSIIW_CONFIG2_YCSWAP_EN_OFS)
#define YCSWAP_EN(x)                    ((x<<CSIIW_CONFIG2_YCSWAP_EN_OFS)&YCSWAP_EN_MASK)
#define YCSWAP_EN_ENA                   YCSWAP_EN(1)
#define YCSWAP_EN_DIS                   YCSWAP_EN(0)
#define CSIIW_CONFIG2_NO_STRIDE_EN_OFS  (0)
#define NO_STRIDE_EN_MASK               (0x1<<CSIIW_CONFIG2_NO_STRIDE_EN_OFS)
#define NO_STRIDE_EN(x)                 ((x<<CSIIW_CONFIG2_NO_STRIDE_EN_OFS)&NO_STRIDE_EN_MASK)
#define NO_STRIDE_EN_ENA                NO_STRIDE_EN(1)
#define NO_STRIDE_EN_DIS                NO_STRIDE_EN(0)


/* mipi-csiiw registers */
struct csiiw_reg {
	volatile unsigned int csiiw_latch_mode;                 /* 00 (csiiw) */
	volatile unsigned int csiiw_config0;                    /* 01 (csiiw) */
	volatile unsigned int csiiw_base_addr;                  /* 02 (csiiw) */
	volatile unsigned int csiiw_stride;                     /* 03 (csiiw) */
	volatile unsigned int csiiw_frame_size;                 /* 04 (csiiw) */
	volatile unsigned int csiiw_frame_buf;                  /* 05 (csiiw) */
	volatile unsigned int csiiw_config1;                    /* 06 (csiiw) */
	volatile unsigned int csiiw_frame_size_ro;              /* 07 (csiiw) */
	volatile unsigned int csiiw_reserved;                   /* 08 (csiiw) */
	volatile unsigned int csiiw_config2;                    /* 09 (csiiw) */
	volatile unsigned int csiiw_version;                    /* 10 (csiiw) */
};

/* moon5 registers*/
struct moon5_reg {
	volatile unsigned int mo5_cfg_0;                        /* 00 (moon5) */
	volatile unsigned int mo5_cfg_1;                        /* 01 (moon5) */
	volatile unsigned int mo5_cfg_2;                        /* 02 (moon5) */
	volatile unsigned int mo5_cfg_3;                        /* 03 (moon5) */
	volatile unsigned int mo5_cfg_4;                        /* 04 (moon5) */
	volatile unsigned int mo5_cfg_5;                        /* 05 (moon5) */
	volatile unsigned int mo5_cfg_6;                        /* 06 (moon5) */
	volatile unsigned int mo5_rsv_0;                        /* 07 (moon5) */
};

#endif
