/*
 * Sunplus Technology
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SP_SPINAND_H
#define __SP_SPINAND_H
#include <linux/mtd/rawnand.h>
/*
 *  spi nand platform related configs
 */
//#define CONFIG_SPINAND_USE_SRAM
#ifdef  CONFIG_SPINAND_USE_SRAM
#define CONFIG_SPINAND_SRAM_ADDR    0x9e800000
#endif
#define SPI_NAND_DIRECT_MAP         0x9dff0000

#define SPINAND_CLKSRC_REG          ((volatile u32 *)(0x9c000000 + (4*32 + 13)*4))
#define SPINAND_SET_CLKSRC(a)       (*SPINAND_CLKSRC_REG = (0x001e0000+(((a)&0x0f)<<1)))
#define SPINAND_GET_CLKSRC()        (((*SPINAND_CLKSRC_REG)>>1)&0x0f)

/*
 *  spi nand functional related configs
 */
#define CONFIG_SPINAND_CLK_DIV               (1)
#define CONFIG_SPINAND_CLK_SRC               (14)
#define CONFIG_SPINAND_READ_BITMODE          SPINAND_4BIT_MODE
#define CONFIG_SPINAND_WRITE_BITMODE         SPINAND_4BIT_MODE
#define CONFIG_SPINAND_BUF_SZ                (8 << 10)
#define CONFIG_SPINAND_TIMEOUT               (100)   /* unit: ms */
#define CONFIG_SPINAND_READ_TIMING_SEL       (2)
#define CONFIG_SPINAND_TRSMODE               SPINAND_TRS_DMA
#define CONFIG_SPINAND_TRSMODE_RAW           SPINAND_TRS_DMA

#define SPINAND_DEBUG_ON
#ifdef SPINAND_DEBUG_ON
#define TAG "[SPI-NAND] "
#define SPINAND_LOGE(fmt, ...) printk(KERN_ERR TAG fmt,##__VA_ARGS__)
#define SPINAND_LOGW(fmt, ...) printk(KERN_WARNING TAG fmt,##__VA_ARGS__)
#define SPINAND_LOGI(fmt, ...) printk(KERN_INFO TAG fmt,##__VA_ARGS__)
#define SPINAND_LOGD(fmt, ...) printk(KERN_DEBUG TAG fmt,##__VA_ARGS__)
#else
#define SPINAND_LOGE(fmt, ...)  do{}while(0)
#define SPINAND_LOGW(fmt, ...)  do{}while(0)
#define SPINAND_LOGI(fmt, ...)  do{}while(0)
#define SPINAND_LOGD(fmt, ...)  do{}while(0)
#endif

/*
 *  spi nand vendor ids
 */
#define VID_GD      0xC8
#define VID_WINBOND 0xEF
#define VID_TOSHIBA 0x98
#define VID_PHISON  0x6B
#define VID_ETRON   0xD5
#define VID_MXIC    0xC2
#define VID_ESMT    0xC8
#define VID_ISSI    0xC8
#define VID_MICRON  0x2C
#define VID_XTX         0x0B
#define VID_FORESEE     0xCD

/*
 *  SPINAND_OPT_* are driver private options for spi nand device
 */

/*
 *  device has two planes.
 *  for readcache and programload cmds, the bit12 of addr is the plane selector.
 */
#define SPINAND_OPT_HAS_TWO_PLANE       0x00000001

/*
 *  the bit0 of feature(0xB0) is used as QE(Quad Enable) bit. (ie. MXIC)
 */
#define SPINAND_OPT_HAS_QE_BIT          0x00000002

/*
 *  unsupport 4bit mode program load cmd (0x32). (ie. toshiba)
 */
#define SPINAND_OPT_NO_4BIT_PROGRAM     0x00000004

/*
 *  unsupport 4bit mode read cache cmd (0x6B). (ie. FORESEE)
 */
#define SPINAND_OPT_NO_4BIT_READ        0x00000008

/*
 *  unsupport 2bit mode read cache cmd (0x3B).
 */
#define SPINAND_OPT_NO_2BIT_READ        0x00000010

/*
 *  the bit0 of feature(0xB0) is used as CONTI_RD bit. (ie. micron 4G spi-nand)
 *  the CONTI_RD is used to enable/disable continue read.
 */
#define SPINAND_OPT_HAS_CONTI_RD        0x00000020

/*
 * the bit3 of feature(0xB0) is used as BUF bit. (ie. winbond)
 * when this bit is set, it uses buffer read mode.
 * when this bit isn't set, it uses continuous read mode.
 */
#define SPINAND_OPT_HAS_BUF_BIT         0x00000040

/*
 * the bit4 of feature(0x90) is used as ECC_EN bit. (ie. FORESEE)
 * generally, the ECC_EN should be the bit4 of feature(0xB0).
 */
#define SPINAND_OPT_ECCEN_IN_F90_4      0x00000080

/*
 * the bit[31:24] is the wanted feature(0xD0) value.
 */
#define SPINAND_OPT_HAS_FD0_VALUE       0x00000100

/*
 * some devices have multiple dies,
 * use the following macros to set/get die number.
 */
#define SPINAND_OPT_SET_DIENUM(n)       (((n-1)&0x0f)<<9)
#define SPINAND_OPT_GET_DIENUM(x)       ((((x)>>9)&0x0f)+1)

/*
 *  SPINAND_CMD_*  are spi nand cmds.
 */
#define SPINAND_CMD_RESET            (0xff)
#define SPINAND_CMD_READID           (0x9f)
#define SPINAND_CMD_GETFEATURES      (0x0f)
#define SPINAND_CMD_SETFEATURES      (0x1f)
#define SPINAND_CMD_BLKERASE         (0xd8)
#define SPINAND_CMD_PAGE2CACHE       (0x13)
#define SPINAND_CMD_PAGEREAD         (0x03)
#define SPINAND_CMD_PAGEREAD_X2      (0x3b)
#define SPINAND_CMD_PAGEREAD_X4      (0x6b)
#define SPINAND_CMD_PAGEREAD_DUAL    (0xbb)
#define SPINAND_CMD_PAGEREAD_QUAD    (0xeb)
#define SPINAND_CMD_WRITEENABLE      (0x06)
#define SPINAND_CMD_WRITEDISABLE     (0x04)
#define SPINAND_CMD_PROGLOAD         (0x02)
#define SPINAND_CMD_PROGLOAD_X4      (0x32)
#define SPINAND_CMD_PROGEXEC         (0x10)
#define SPINAND_CMD_DIE_SELECT       (0xc2)

/*
 *  macros for spi_ctrl register
 */
#define SPINAND_BUSY_MASK            (1<<31)
#define SPINAND_AUTOMODE_EN          (1<<28)
#define SPINAND_AUTOCMD_EN           (1<<27)
#define SPINAND_SEL_CHIP_B           (1<<25)
#define SPINAND_SEL_CHIP_A           (1<<24)
#define SPINAND_AUTOWEL_EN           (1<<19)
#define SPINAND_SCK_DIV(x)           (((x)&0x07)<<16)
#define SPINAND_USR_CMD(x)           (((x)&0xff)<<8)
#define SPINAND_CTRL_EN              (1<<7)
#define SPINAND_USRCMD_DATASZ(x)     (((x)&0x07)<<4)   //0~3 are allowed
#define SPINAND_READ_MODE            (0<<2)            //read from device
#define SPINAND_WRITE_MODE           (1<<2)            //write to device
#define SPINAND_USRCMD_ADDRSZ(x)     (((x)&0x07)<<0)   //0~3 are allowed

/*
 *  macros for spi_timing register
 */
#define SPINAND_CS_SH_CYC(x)         (((x)&0x3f)<<22)
#define SPINAND_CD_DISACTIVE_CYC(x)  (((x)&0x3f)<<16)
#define SPINAND_READ_TIMING(x)       (((x)&0x07)<<1)  //0~7 are allowed
#define SPINAND_WRITE_TIMING(x)      (((x)&0x01))     //0~1 are allowed

/*
 *  macros for spi_status register
 */
#define SPINAND_STATUS_MASK          (0xff)

/*
 *  macros for spi_auto_cfg register
 */
#define SPINAND_USR_READCACHE_CMD(x) (((x)&0xff)<<24)
#define SPINAND_CONTINUE_MODE_EN     (1<<23)//enable winbon read continue mode
#define SPINAND_USR_CMD_TRIGGER      (1<<21)
#define SPINAND_USR_READCACHE_EN     (1<<20)
#define SPINAND_CHECK_OIP_EN         (1<<19)
#define SPINAND_AUTO_RDSR_EN         (1<<18)
#define SPINAND_DMA_OWNER_MASK       (1<<17)
#define SPINAND_DMA_TRIGGER          (1<<17)
#define SPINAND_DUMMY_OUT            (1<<16) //SPI_DQ is ouput in dummy cycles
#define SPINAND_USR_PRGMLOAD_CMD(x)  (((x)&0xff)<<8)
#define SPINAND_AUTOWEL_BF_PRGMEXEC  (1<<2)
#define SPINAND_AUTOWEL_BF_PRGMLOAD  (1<<1)
#define SPINAND_USR_PRGMLOAD_EN      (1<<0)

/*
 *  macros for spi_cfg0 register
 */
#define SPINAND_LITTLE_ENDIAN        (1<<23)
#define SPINAND_DATA64_EN            (1<<20)
#define SPINAND_TRS_MODE             (1<<19)
#define SPINAND_SCL_IDLE_LOW         (1<<17)
#define SPINAND_DATA_LEN(x)          (((x)&0xffff)<<0)

/*
 *  macros for spi_cfg1, spi_cfg2 register
 *  spi_cfg1 is used in pio(including auto mode) and dma mode.
 *  spi_cfg2 is used in memory access mode.
 */
#define SPINAND_DUMMY_CYCLES(x)      (((x)&0x3f)<<24)
/*
 *  bit mode selector:
 *     0 - no need tranfer
 *     1 - 1 bit mode
 *     2 - 2 bit mode
 *     3 - 4 bit mode
 */
#define SPINAND_DATA_BITMODE(x)      (((x)&0x03)<<20)
#define SPINAND_ADDR_BITMODE(x)      (((x)&0x03)<<18)
#define SPINAND_CMD_BITMODE(x)       (((x)&0x03)<<16)
/*
 *  cmd/addr/dataout DQ selector:
 *     0 - disabled
 *     1 - select DQ0
 *     2 - select DQ0,DQ1
 *     3 - select DQ0,DQ1,DQ2,DQ3
 */
#define SPINAND_DATAOUT_DQ(x)        (((x)&0x03)<<4)
#define SPINAND_ADDR_DQ(x)           (((x)&0x03)<<2)
#define SPINAND_CMD_DQ(x)            (((x)&0x03)<<0)
/*
 *  datain DQ selsector:
 *     0 - disabled
 *     1 - select DQ0
 *     2 - select DQ1
 */
#define SPINAND_DATAIN_DQ(x)         (((x)&0x03)<<6)

/*
 *  macros for spi_bch register
 */
#define SPINAND_BCH_DATA_LEN(x)      (((x)&0xff)<<8)
#define SPINAND_BCH_1K_MODE          (1<<6)
#define SPINAND_BCH_512B_MODE        (0<<6)
#define SPINAND_BCH_ALIGN_32B        (0<<5)
#define SPINAND_BCH_ALIGN_16B        (1<<5)
#define SPINAND_BCH_AUTO_EN          (1<<4)
#define SPINAND_BCH_BLOCKS(x)        (((x)&0x0f)<<0) //0~15 means 1~16 blocks

/*
 *  macros for spi_intr_msk,spi_intr_sts register
 */
#define SPINAND_PIO_DONE_MASK        (1<<2)
#define SPINAND_DMA_DONE_MASK        (1<<1)
#define SPINAND_BUF_FULL_MASK        (1<<0)

/*
 *  macros for spi_page_size register
 */
#define SPINAND_DEV_ECC_EN           (1<<15)
#define SPINAND_SPARE_SIZE(x)        (((x)&0x7ff)<<4)  //0~2047 are allowed.
#define SPINAND_PAGE_SIZE(x)         (((x)&0x07)<<0)   //0~7 means 1~8KB

enum SPINAND_TRSMODE {
	SPINAND_TRS_PIO,
	SPINAND_TRS_PIO_AUTO,
	SPINAND_TRS_DMA,
	SPINAND_TRS_DMA_AUTOBCH,
	SPINAND_TRS_MAX,
};

enum SPINAND_BIT_MODE {
	SPINAND_1BIT_MODE,
	SPINAND_2BIT_MODE,
	SPINAND_4BIT_MODE,
	SPINAND_DUAL_MODE,
	SPINAND_QUAD_MODE,
};

/*block erase status */
#define ERASE_STATUS            0x04

/*protect status */
#define PROTECT_STATUS          0x38

/*
 *  spi nand device feature address
 */
#define DEVICE_PROTECTION_ADDR  0xA0
#define DEVICE_FEATURE_ADDR     0xB0
#define DEVICE_STATUS_ADDR      0xC0

/* spi nand regs */
struct sp_spinand_regs {
	unsigned int spi_ctrl;       // 87.0
	unsigned int spi_timing;     // 87.1
	unsigned int spi_page_addr;  // 87.2
	unsigned int spi_data;       // 87.3
	unsigned int spi_status;     // 87.4
	unsigned int spi_auto_cfg;   // 87.5
	unsigned int spi_cfg[3];     // 87.6
	unsigned int spi_data_64;    // 87.9
	unsigned int spi_buf_addr;   // 87.10
	unsigned int spi_statu_2;    // 87.11
	unsigned int spi_err_status; // 87.12
	unsigned int mem_data_addr;  // 87.13
	unsigned int mem_parity_addr;// 87.14
	unsigned int spi_col_addr;   // 87.15
	unsigned int spi_bch;        // 87.16
	unsigned int spi_intr_msk;   // 87.17
	unsigned int spi_intr_sts;   // 87.18
	unsigned int spi_page_size;  // 87.19
};

struct sp_spinand_info {
	void __iomem *regs;
	void __iomem *bch_regs;
	struct mtd_info *mtd;
	struct nand_chip nand;
	const char *dev_name;

	struct {
		u32 idx;
		u32 size;
		u8 *virt;
		dma_addr_t phys;
		u8 *virt_base;
	} buff;

	struct nand_ecclayout ecc_layout;
	u32 id;                /* Vendor ID for 6B/EB Winbond or GigaDevice */
	u32 cmd;               /* current command */
	u32 row;               /* row address for current cmd */
	u32 col;               /* column address for current cmd */
	u32 plane_sel_mode;
	u32 page_size;
	u32 oob_size;

	u32 chip_num;
	u32 cur_chip;

	u32 cr0;
	u32 ecc_sts;
	u32 parity_sector_size;

	u8 spi_clk_div;
	u8 read_bitmode;    /* bit mode in read case,refer to SPINAND_BIT_MODE*/
	u8 write_bitmode;   /* bit mode in write case,refer to SPINAND_BIT_MODE*/
	u8 trs_mode;       /* refer to SPINAND_TRSMODE*/
	u8 raw_trs_mode;   /* used in raw data access,refer to SPINAND_TRSMODE*/
	u8 dev_protection; /* protection value by reading feature(0xA0) */

	int cac;      /* col address cycles, unused for spi-nand */
	int rac;      /* row address cycles, unused for spi-nand */
};

struct sp_spinand_info* get_spinand_info(void);

#endif /* __SP_SPINAND_H */

