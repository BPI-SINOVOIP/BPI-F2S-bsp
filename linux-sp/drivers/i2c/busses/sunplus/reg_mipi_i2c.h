#ifndef __REG_I2C_H__
#define __REG_I2C_H__

//#include <mach/io_map.h>

/****************************************
* MIPI I2C Register Definition
****************************************/
// 0x2601
#define MIPI_I2C_2601_TRANS_MODE        (1<<0)
#define MIPI_I2C_2601_NEW_TRANS_MODE    (1<<1)
#define MIPI_I2C_2601_NEW_TRANS_EN      (1<<2)

// 0x2602
#define MIPI_I2C_2602_SUBADDR_EN        (1<<0)
#define MIPI_I2C_2602_RESTART_EN        (1<<1)
#define MIPI_I2C_2602_SUBADDR_16        (1<<2)
#define MIPI_I2C_2602_PREFETCH          (1<<4)

// 0x2603
#define MIPI_I2C_2603_SYNC_IM           (0)
#define MIPI_I2C_2603_SYNC_VD_F         (1)
#define MIPI_I2C_2603_SYNC_VD_R         (2)
#define MIPI_I2C_2603_SYNC_VD_R_HD_F    (3)
#define MIPI_I2C_2603_SYNC_VD_R_HD_R    (4)
#define MIPI_I2C_2603_SYNC_VD_R_N_HD_F  (5)
#define MIPI_I2C_2603_SYNC_VD_R_N_HD_R  (6)

// 0x2650
#define MIPI_I2C_2650_BUSY              (1<<0)

// 0x2660
#define MIPI_I2C_2660_RST_SIF           (1<<0)

/****************************************
* MOON5 Register Definition
****************************************/
// 5.0 MOON5 control register #0 (MO5_CFG_0)
#define MO5_CFG0_ISPABP1_MODE_SEL_BIT   (10)
#define MO_ISPABP1_MODE_MASK            (1<<MO5_CFG0_ISPABP1_MODE_SEL_BIT)
#define MO_ISPABP1_MODE_4T              MO_ISPABP1_MODE_MASK
#define MO_ISPABP1_MODE_8T              (0<<MO5_CFG0_ISPABP1_MODE_SEL_BIT)

#define MO5_CFG0_ISPABP0_MODE_SEL_BIT   (9)
#define MO_ISPABP0_MODE_MASK            (1<<MO5_CFG0_ISPABP0_MODE_SEL_BIT)
#define MO_ISPABP0_MODE_4T              MO_ISPABP0_MODE_MASK
#define MO_ISPABP0_MODE_8T              (0<<MO5_CFG0_ISPABP0_MODE_SEL_BIT)


/****************************************
* MIPI ISP Register Definition
****************************************/
#define ISP_BASE_ADDRESS                0x2000

// 0x276A
#define ISP_2601_TRANS_MODE             (1<<0)


/* mipi-ispapb registers */
typedef struct mipi_isp_reg {
	volatile unsigned char reg[0x1300];         /* 0x2000 ~ 0x32FF */
} mipi_isp_reg_t;

/* moon5 registers*/
struct moon5_reg {
	volatile unsigned int mo5_cfg_0;            /* 00 (moon5) */
	volatile unsigned int mo5_cfg_1;            /* 01 (moon5) */
	volatile unsigned int mo5_cfg_2;            /* 02 (moon5) */
	volatile unsigned int mo5_cfg_3;            /* 03 (moon5) */
	volatile unsigned int mo5_cfg_4;            /* 04 (moon5) */
	volatile unsigned int mo5_cfg_5;            /* 05 (moon5) */
	volatile unsigned int mo5_cfg_6;            /* 06 (moon5) */
	volatile unsigned int mo5_rsv_0;            /* 07 (moon5) */
};

#endif /* __REG_I2C_H__ */
