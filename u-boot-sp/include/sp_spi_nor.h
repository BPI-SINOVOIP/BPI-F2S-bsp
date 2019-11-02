/*
 * (C) Copyright 2017
 * Sunplus Technology
 * Henry Liou<henry.liou@sunplus.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SP_SPI_NOR_H
#define __SP_SPI_NOR_H

typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef unsigned char BYTE;


//#define ALL_DEBUG
#ifdef ALL_DEBUG
#define diag_printf(fmt, arg...)        printf(fmt, ##arg)
#else
#define diag_printf(s...)               ((void)0)
#endif

//#define DEBUG
#ifdef DEBUG
#define msg_printf(fmt, arg...)         printf(fmt, ##arg)
#else
#define msg_printf(s...)                ((void)0)
#endif


#define SP_SPINOR_DMA                   1

#define CFG_BUFF_MAX                    (18 << 10)
#define CONFIG_SRAM_BASE                0x9e800000

#define CMD_BUF_LEN                     4

#define RF_MASK_V(_mask, _val)          (((_mask) << 16) | (_val))

//spi_ctrl
#define SPI_CTRL_BUSY                   (1<<31)
#define CUST_CMD(x)                     (x<<8)
#define CLEAR_CUST_CMD                  (~0xffff)

enum SPI_CLOCK_SEL {
	SPI_CLK_D_2 = (1)<<16,
	SPI_CLK_D_4 = (2)<<16,
	SPI_CLK_D_6 = (3)<<16,
	SPI_CLK_D_8 = (4)<<16,
	SPI_CLK_D_16 = (5)<<16,
	SPI_CLK_D_24 = (6)<<16,
	SPI_CLK_D_32 = (7)<<16
};

enum SPI_CHIP_SEL {
	A_CHIP = 1<<24,
	B_CHIP = 1<<25
};

enum SPI_PIO_ADDRESS_BYTE {
	ADDR_0B = 0,
	ADDR_1B = 1,
	ADDR_2B = 2,
	ADDR_3B = 3
};

enum SPI_PIO_CMD {
	READ  = 0<<2,
	WRITE = 1<<2
};

enum SPI_PIO_DATA_BYTE {
	BYTE_0 = 0<<4,
	BYTE_1 = 1<<4,
	BYTE_2 = 2<<4,
	BYTE_3 = 3<<4,
	BYTE_4 = 4<<4
};

//spi_cfg1/2
#define SPI_DUMMY_CYC(x)                (x<<24)
#define DUMMY_CYCLE(x)                  (x)

enum SPI_CMD_BIT {
	SPI_CMD_NO = (0)<<16,
	SPI_CMD_1b = (1)<<16,
	SPI_CMD_2b = (2)<<16,
	SPI_CMD_4b = (3)<<16
};

enum SPI_ADDR_BIT {
	SPI_ADDR_NO = (0)<<18,
	SPI_ADDR_1b = (1)<<18,
	SPI_ADDR_2b = (2)<<18,
	SPI_ADDR_4b = (3)<<18
};

enum SPI_DATA_BIT {
	SPI_DATA_NO = (0)<<20,
	SPI_DATA_1b = (1)<<20,
	SPI_DATA_2b = (2)<<20,
	SPI_DATA_4b = (3)<<20
};

enum SPI_ENHANCE_BIT {
	SPI_ENHANCE_NO = (0)<<22,
	SPI_ENHANCE_1b = (1)<<22,
	SPI_ENHANCE_2b = (2)<<22,
	SPI_ENHANCE_4b = (3)<<22
};

enum SPI_CMD_OEN_BIT {
	SPI_CMD_OEN_NO = 0,
	SPI_CMD_OEN_1b = 1,
	SPI_CMD_OEN_2b = 2,
	SPI_CMD_OEN_4b = 3
};

enum SPI_ADDR_OEN_BIT {
	SPI_ADDR_OEN_NO = (0)<<2,
	SPI_ADDR_OEN_1b = (1)<<2,
	SPI_ADDR_OEN_2b = (2)<<2,
	SPI_ADDR_OEN_4b = (3)<<2
};

enum SPI_DATA_OEN_BIT {
	SPI_DATA_OEN_NO = (0)<<4,
	SPI_DATA_OEN_1b = (1)<<4,
	SPI_DATA_OEN_2b = (2)<<4,
	SPI_DATA_OEN_4b = (3)<<4
};

enum SPI_DATA_IEN_BIT {
	SPI_DATA_IEN_NO  = (0)<<6,
	SPI_DATA_IEN_DQ0 = (1)<<6,
	SPI_DATA_IEN_DQ1 = (2)<<6
};

//spi_autocfg
#define USER_DEFINED_READ(x)            (x<<24)
#define USER_DEFINED_READ_EN            (1<<20)
#define DMA_TRIGGER                     (1<<17)
#define AUTO_RDSR_EN                    (1<<18)
#define PIO_TRIGGER                     (1<<21)
#define PREFETCH_ENABLE                 (1<<22)
#define PREFETCH_DISABLE                (~(1<<22))

//spi_cfg0
#define ENHANCE_DATA(x)                 (x<<24)
#define CLEAR_ENHANCE_DATA              (~(0xff<<24))
#define DATA64_EN                       (1<<20)
#define DATA64_DIS                      (~(1<<20))
#define CLEAR_DATA64_LEN                (~0xffff)
#define SPI_DATA64_MAX_LEN              ((1<<16)-1)

//spi_buf_addr
#define DATA64_READ_ADDR(x)             (x<<16)
#define DATA64_WRITE_ADDR(x)            (x)

//spi_status_2
#define SPI_SRAM_ST                     (0x7<<13)

#define CMD_FAST_READ                   (0xb)
#define CMD_READ_STATUS                 (0x5)
#define SPI_TIMEOUT                     450

typedef struct {
	UINT8 enhance_en;
	UINT8 enhance_bit;
	UINT8 enhance_bit_mode;
	UINT8 enhance_data;
} SPI_ENHANCE;

enum ERASE_TYPE {
	SECTOR = 0,
	BLOCK = 1,
	CHIP = 2
};

enum SPI_USEABLE_DQ {
	DQ0 = 1<<20,
	DQ1 = 1<<21,
	DQ2 = 1<<22,
	DQ3 = 1<<23
};

enum SPI_CMD_IO_BIT {
	CMD_0 = 0,
	CMD_1 = 1,
	CMD_2 = 2,
	CMD_4 = 4
};

enum SPI_ADDR_IO_BIT {
	ADDR_0 = 0,
	ADDR_1 = 1,
	ADDR_2 = 2,
	ADDR_4 = 4
};

enum SPI_DATA_IO_BIT {
	DATA_0 = 0,
	DATA_1 = 1,
	DATA_2 = 2,
	DATA_4 = 4
};

enum SPI_DMA_MODE {
	DMA_OFF = 0,
	DMA_ON = 1
};

enum SPI_SRAM_STATUS {
	SRAM_CONFLICT = 0,
	SRAM_EMPTY = 1,
	SRAM_FULL = 2
};

enum SPI_INTR_STATUS {
	BUFFER_ENOUGH = 1,
	DMA_DONE = 2,
	PIO_DONE = 4
};

typedef struct {
	// Group 022: SPI_FLASH
	UINT32 spi_ctrl;
	UINT32 spi_timing;
	UINT32 spi_page_addr;
	UINT32 spi_data;
	UINT32 spi_status;
	UINT32 spi_auto_cfg;
	UINT32 spi_cfg0;
	UINT32 spi_cfg1;
	UINT32 spi_cfg2;
	UINT32 spi_data64;
	UINT32 spi_buf_addr;
	UINT32 spi_status_2;
	UINT32 spi_err_status;
	UINT32 spi_mem_data_addr;
	UINT32 spi_mem_parity_addr;
	UINT32 spi_col_addr;
	UINT32 spi_bch;
	UINT32 spi_intr_msk;
	UINT32 spi_intr_sts;
	UINT32 spi_page_size;
	UINT32 G22_RESERVED[12];
} sp_spi_nor_regs;

struct sp_spi_nor_platdata {
	struct sp_spi_nor_regs *regs;
	unsigned int clock;
	unsigned int mode;
	unsigned int chipsel;
};

#if (SP_SPINOR_DMA)
struct spinorbufdesc {
	uint32_t idx;
	uint32_t size;
	dma_addr_t phys;
} __aligned(ARCH_DMA_MINALIGN);
#endif

struct sp_spi_nor_priv {
	struct sp_spi_nor_regs *regs;
	unsigned int clock;
	unsigned int mode;
	unsigned int chipsel;
#if (SP_SPINOR_DMA)
	struct spinorbufdesc wchain;
	struct spinorbufdesc rchain;
	UINT8 w_buf[CFG_BUFF_MAX] __aligned(ARCH_DMA_MINALIGN);
	UINT8 r_buf[CFG_BUFF_MAX] __aligned(ARCH_DMA_MINALIGN);
#endif
};

#endif /* __SP_SPI_NOR_H */
