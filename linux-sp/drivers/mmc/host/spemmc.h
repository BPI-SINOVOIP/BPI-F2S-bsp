#ifndef _SPEMMCV2_H_
#define _SPEMMCV2_H_
////////////////////////////////////////////////////////////////
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/mmc/mmc.h>
#include <mach/irqs.h>
#include <linux/clk.h>

/////////////////////////////////////////////////////////////////
#define SP_MMC_SUPPORT_DDR_MODE
#define SPEMMC_SUCCESS			0x00000000
#define SPEMMC_FAIL				0x00000001
#define SPEMMC_RSP_TIMEOUT		0x00000002
#define SPEMMC_CRC_ERROR			0x00000004
#define SPEMMC_COUNTER_ERROR		0x00000008
#define SPEMMC_DMA_FAIL			0x00000010
#define SPEMMC_TIMEOUT			0x00000020
#define SPEMMC_CRC_TIMEOUT		0x00000040
#define SPEMMC_CARD_NOT_EXIST		0x00000080
#define SPEMMC_COMPLETE_INIT		0x80000000

#define SDCARD_STATUS_NOTEXIST 			0
#define SDCARD_STATUS_EXIST 			1
#define SDCARD_STATUS_TESTNOTEXIST 		2
#define SDCARD_STATUS_TESTEXIST 		3

//1.17[14]
#define CARD_MODE_SD	0
#define CARD_MODE_MMC	1

//to delete
#define EMMC_1BIT_MODE 			0
#define EMMC_4BIT_MODE 			1

#define SPRSP_TYPE_NORSP		0
#define SPRSP_TYPE_R1  			1
#define SPRSP_TYPE_R1B 			11
#define SPRSP_TYPE_R2  			2
#define SPRSP_TYPE_R3  			3
#define SPRSP_TYPE_R4  			4
#define SPRSP_TYPE_R5  			5
#define SPRSP_TYPE_R6  			6
#define SPRSP_TYPE_R7 			7

#define SP_DMA_DRAM      		1
#define SP_DMA_FLASH     		2

#define SP_DMA_DST_SHIFT		4
#define SP_DMA_SRT_SHIFT		0

#define SP_BUS_WIDTH_1BIT 0
#define SP_BUS_WIDTH_4BIT 1
#define SP_BUS_WIDTH_8BIT (1<<6)
#define CLOCK_200K	200000
#define CLOCK_300K	300000
#define CLOCK_375K	375000
#define CLOCK_400K	400000
#define CLOCK_1M	1000000
#define CLOCK_5M	5000000
#define CLOCK_10M	10000000
#define CLOCK_12M	12000000
#define CLOCK_15M	15000000
#define CLOCK_20M	20000000
#define CLOCK_25M	25000000
#define CLOCK_27M	27000000
#define CLOCK_30M	30000000
#define CLOCK_35M	35000000
#define CLOCK_45M	45000000
#define CLOCK_48M	48000000
#define CLOCK_50M	50000000
#define CLOCK_52M	52000000

#define CLOCK_240M  240000000
#define CLOCK_270M  270000000
#define CLOCK_202M  202500000

/* kernel set max read timeout to 100ms,
 * mult  to  Improve compatibility for some unstandard card
 */
#define READ_TIMEOUT_MULT     10


/* kernel set max read timeout to 3s,
 * mult  to  Improve compatibility for some unstandard emmc
 */
#define EMMC_WRITE_TIMEOUT_MULT     5

/******************************************************************************
*                          MACRO Function Define
*******************************************************************************/

#ifndef REG
//#define REG(x) (*(volatile unsigned long *)(x))
#define REG(x) (*(volatile unsigned int *)(x))
#endif

#define get_val(addr)		REG(addr)
#define set_val(addr, val)	REG(addr) = (val)

#ifndef set_bit
#define set_bit(addr, val)	set_val((addr), (get_val(addr) | (val)))
#endif

#ifndef clear_bit
#define clear_bit(addr, val)	set_val((addr), (get_val(addr) & ~(val)))
#endif

#define SP_MEDIA_NONE			0
#define SP_MEDIA_SMC			1
#define SP_MEDIA_RESERVED1 		2
#define SP_MEDIA_CF				3
#define SP_MEDIA_SPI			4
#define SP_MEDIA_RESERVED2		5
#define SP_MEDIA_SD				6
#define SP_MEDIA_MEMORY_STICK	7

#define SP_HW_DMA_MEMORY_SECTORS	8 /*  supports up to 8 fragmented memory blocks */

#define SP_LENMODE_SEND_STOP 				0
#define SP_LENMODE_NOT_SEND_STOP 			1
#define SP_TRANSACTION_MODE_CMD_RSP			0
#define SP_TRANSACTION_MODE_WRITE_DATA		1
#define SP_TRANSACTION_MODE_READ_DATA		2
#define SP_AUTORSP_NO_RX_RESPONSE 			0
#define SP_AUTORSP_ATUO_RX_RESPONSE 		1

#define SP_SD_RSP_TYPE_LEN_48_BITS		0
#define SP_SD_RSP_TYPE_LEN_136_BITS		1
#define SP_SD_MODE   0
#define SP_MMC_MODE  1
#define SP_EMMC_CARD    0
#define SP_SDIO_CARD  1

#define SP_SDSTATUS_DUMMY_READY 					BIT(0)
#define SP_SDSTATUS_RSP_BUF_FULL					BIT(1)
#define SP_SDSTATUS_TX_DATA_BUF_EMPTY				BIT(2)
#define SP_SDSTATUS_RX_DATA_BUF_FULL				BIT(3)
#define SP_SDSTATUS_CMD_PIN_STATUS					BIT(4)
#define SP_SDSTATUS_DAT0_PIN_STATUS					BIT(5)
#define SP_SDSTATUS_WAIT_RSP_TIMEOUT				BIT(6)
#define SP_SDSTATUS_WAIT_CARD_CRC_CHECK_TIMEOUT		BIT(7)
#define SP_SDSTATUS_WAIT_STB_TIMEOUT				BIT(8)
#define SP_SDSTATUS_RSP_CRC7_ERROR					BIT(9)
#define SP_SDSTATUS_CRC_TOKEN_CHECK_ERROR			BIT(10)
#define SP_SDSTATUS_RDATA_CRC16_ERROR				BIT(11)
#define SP_SDSTATUS_SUSPEND_STATE_READY				BIT(12)
#define SP_SDSTATUS_BUSY_CYCLE						BIT(13)
#define SP_SDSTATUS_SD_DATA1						BIT(14)
#define SP_SDSTATUS_SD_SENSE						BIT(15)

#define WRITE_CRC_TOKEN_CORRECT		2
#define WRITE_CRC_TOKEN_ERROR		5
#define SDSTATE_NEW_FINISH_IDLE 		BIT(6)
#define SDSTATE_NEW_ERROR_TIMEOUT 		BIT(5)

#define SP_DMA_TYPE_SINGLEBLOCK_CMD	1
#define SP_DMA_TYPE_MULTIBLOCK_CMD		2

#define SP_SD_CMDBUF_SIZE 5

#define SP_SD_RXDATTMR_MAX	((1 << 29) - 1)

#define MS_RDDATA_SIZE 4

#define SP_MS_WD_DATA_SIZE 16

#define SP_DMA_NORMAL_STATE			0
#define SP_RESET_DMA_OPERATION		1

#define SP_DMA_BLOCK_MODE_PAGE_LEVEL	0
#define SP_DMA_BLOCK_MODE_BLOCK_LEVEL	1

#define SP_DMA_HW_PAGE_NUM_SIZE 4


typedef volatile unsigned int dev_reg32;
typedef volatile unsigned char dev_reg8;

typedef struct  spemmc_general_regs{
	/*g0.0*/
	union {
		struct {
			dev_reg32 mediatype:3;
			dev_reg32 reserved0:1;
			dev_reg32 dmasrc:3;
			dev_reg32 reserved1:1;
			dev_reg32 dmadst:3;
			dev_reg32 reserved2:21;
		};
		dev_reg32 medatype_dma_src_dst;
	};
	/*g0.1*/
	dev_reg32 card_ctl_page_cnt:16;
	dev_reg32 reserved3:16;

	/* g0.2 */
	dev_reg32 sdram_sector_0_size:16;
	dev_reg32 reserved4:1;
	/* g0.3 */
	dev_reg32 dma_base_addr;
	/* g0.4 */
	union {
		struct {
			dev_reg32 reserved5:1;
			dev_reg32 hw_dma_en:1;
			dev_reg32 reserved6:1;
			dev_reg32 hw_sd_hcsd_en:1;
			dev_reg32 hw_sd_dma_type:2;
			dev_reg32 hw_sd_cmd13_en:1;
			dev_reg32 reserved7:1;
			dev_reg32 stop_dma_flag:1;
			dev_reg32 hw_dma_rst:1;
			dev_reg32 dmaidle:1;
			dev_reg32 dmastart:1;
			dev_reg32 hw_block_num:2;
			dev_reg32 reserved8:2;
			dev_reg32 hw_cmd13_rca:16;
		};
		dev_reg32 hw_dma_ctl;
	};
	/* g0.5 */
	union {
		struct {
			dev_reg32 reg_sd_ctl_free:1;			/*  0 */
			dev_reg32 reg_sd_free:1;				/*  1 */
			dev_reg32 reg_ms_ctl_free:1;			/*  2 */
			dev_reg32 reg_ms_free:1;				/*  3 */
			dev_reg32 reg_dma_fifo_free:1;			/*  4 */
			dev_reg32 reg_dma_ctl_free:1;			/*  5 */
			dev_reg32 reg_hwdma_page_free:1;		/*  6 */
			dev_reg32 reg_hw_dma_free:1;			/*  7 */
			dev_reg32 reg_sd_hwdma_free:1;			/*  8 */
			dev_reg32 reg_ms_hwdma_free:1;			/*  9 */
			dev_reg32 reg_dma_reg_free:1;			/*  10 */
			dev_reg32 reg_card_reg_free:1;			/*  11 */
			dev_reg32 reserved9:20;
		};
		dev_reg32 card_gclk_disable;
	};

	/* g0.6 ~ g0.19*/
	struct {
		dev_reg32 dram_sector_addr;
		dev_reg32 sdram_sector_size:16;
		dev_reg32 reserved10:16;
	} dma_addr_info[7];

	/* g0.20 */
	union {
		struct {
			dev_reg32 dram_sector_cnt:3;			/*  2:00 ro */
			dev_reg32 hw_block_cnt:2;				/*  04:03 ro */
			dev_reg32 reserved11:11;				/*  15:05 ro */
			dev_reg32 hw_page_cnt:16;				/*  31:16 ro  */
		};
		dev_reg32 sdram_sector_block_cnt;
	};
	/* g0.20 ~ g0.28 */
	dev_reg32 dma_hw_page_addr[4];
	dev_reg32 dma_hw_page_num[4];

	/* g0.29 */
	dev_reg32 hw_wait_num;

	/* g0.30 */
	dev_reg32 hw_delay_num:16;
	dev_reg32 reserved12:16;

	/* g0.31 */
	union {
		struct {
			dev_reg32 incnt:11;
			dev_reg32 outcnt:11;
			dev_reg32 dma_sm:3;
			dev_reg32 reserved13:7;
		};
		dev_reg32 dma_debug;
	};

	/* g1.0 */
	union {
		struct {
			dev_reg32 boot_ack_en:1;
			dev_reg32 boot_ack_tmr:1;
			dev_reg32 boot_data_tmr:1;
			dev_reg32 fast_boot:1;
			dev_reg32 boot_mode:1;
			dev_reg32 bootack:3;
			dev_reg32 reserved14:24;
		};
		dev_reg32 boot_ctl;
	};

	/* g1.1 */
	union {
		struct {
			dev_reg32 vol_tmr:2;
			dev_reg32 sw_set_vol:1;
			dev_reg32 hw_set_vol:1;
			dev_reg32 vol_result:2;
			dev_reg32 reserved15:26;
		};
		dev_reg32 sd_vol_ctrl;
	};
	/* g1.2 */
	union {
		struct {
			dev_reg32 sdcmpen:1;
			dev_reg32 sd_cmp:1;			/* 1 */
			dev_reg32 sd_cmp_clr:1;		/* 2 */
			dev_reg32 sdio_int_en:1;	/* 3 */
			dev_reg32 sdio_int:1;		/* 4 */
			dev_reg32 sdio_int_clr:1;	/* 5 */
			dev_reg32 detect_int_en:1;  /* 6 */
			dev_reg32 detect_int:1;		/* 7 */
			dev_reg32 detect_int_clr:1; /* 8 */
			dev_reg32 hwdmacmpen:1;		/* 9 */
			dev_reg32 hw_dma_cmp:1;		/* 10 */
			dev_reg32 hwdmacmpclr:1;	/* 11 */
			dev_reg32 reserved16:20;	/* 31:12 */
		};
		dev_reg32 sd_int;
	};

	/* g1.3 */
	dev_reg32 sd_page_num:16;
	dev_reg32 reserved17:16;
	/* g1.4 */
	union {
		struct {
			dev_reg32 sdpiomode:1;
			dev_reg32 sdddrmode:1;
			dev_reg32 sd_len_mode:1;
			dev_reg32 first_dat_hcyc:1;
			dev_reg32 sd_trans_mode:2;
			dev_reg32 sdautorsp:1;
			dev_reg32 sdcmddummy:1;
			dev_reg32 sdrspchk_en:1;
			dev_reg32 sdiomode:1;
			dev_reg32 sdmmcmode:1;
			dev_reg32 sddatawd:1;
			dev_reg32 sdrsptmren:1;
			dev_reg32 sdcrctmren:1;
			dev_reg32 rx4b_en:1;
			dev_reg32 sdrsptype:1;
			dev_reg32 detect_tmr:2;
			dev_reg32 mmc8_en:1;
			dev_reg32 selci:1;
			dev_reg32 sdfqsel:12;
		};
		dev_reg32 sd_config0;
	};

	/* g1.5 */
	union {
		struct {
			dev_reg32 rwc:1;
			dev_reg32 s4mi:1;
			dev_reg32 resu:1;
			dev_reg32 sus_req:1;
			dev_reg32 con_req:1;
			dev_reg32 sus_data_flag:1;
			dev_reg32 int_multi_trig:1;
			dev_reg32 reserved18:25;
		};
		dev_reg32 sdio_ctrl;
	};

	/* g1.6 */
	union {
		struct {
			dev_reg32 sdrst:1;
			dev_reg32 sdcrcrst:1;
			dev_reg32 sdiorst:1;
			dev_reg32 reserved19:29;
		};
		dev_reg32 sd_rst;
	};

	/* g1.7 */
	union {
		struct {
			dev_reg32 sdctrl0:1;
			dev_reg32 sdctrl1:1;
			dev_reg32 sdioctrl:1;
			dev_reg32 emmcctrl:1;
			dev_reg32 reserved20:28;
		} ;
		dev_reg32 sd_ctrl;
	};
	/* g1.8 */
	union {
		struct {
			dev_reg32 sdstatus:19;
			dev_reg32 reserved21:13;
		};
		dev_reg32 sd_status;
	};
	/* g1.9 */
	union {
		struct {
			dev_reg32 sdstate:3;
			dev_reg32 reserved22:1;
			dev_reg32 sdcrdcrc:3;
			dev_reg32 reserved23:1;
			dev_reg32 sdstate_new:7;
			dev_reg32 reserved24:17;
		};
		dev_reg32 sd_state;
	};

	/* g1.10 */
	union {
		struct {
			dev_reg32 hwsd_sm:10;
			dev_reg32 reserved25:22;
		};
		dev_reg32 sd_hw_state;
#define SP_EMMC_HW_DMA_ERROR				BIT(6)
#define SP_EMMC_HW_DMA_DONE				BIT(7)
	};

	/* g1.11 */
	union {
		struct {
			dev_reg32 sddatalen:11;
			dev_reg32 reserved26:21;
		};
		dev_reg32 sd_blocksize;
	};

	/* g1.12 */
	union {
		struct {
			dev_reg32 tx_dummy_num:9;
			dev_reg32 sdcrctmr:11;
			dev_reg32 sdrsptmr:11;
			dev_reg32 sd_high_speed_en:1;
		};
		dev_reg32 sd_config1;
	};

	/* g1.13 */
	union {
		struct {
			dev_reg32 sd_clk_dly_sel:3;
			dev_reg32 reserved27:1;
			dev_reg32 sd_wr_dly_sel:3;
			dev_reg32 reserved28:1;
			dev_reg32 sd_rd_dly_sel:3;
			dev_reg32 reserved29:21;
		};
		dev_reg32 sd_timing_config;
	};

	/* g1.14 */
	dev_reg32 sd_rxdattmr:29;
#define SP_EMMC_RXDATTMR_MAX	((1 << 29) - 1)
	dev_reg32 reserved30:3;

	/* g1.15 */
	dev_reg32 sd_piodatatx;

	/* g1.16 */
	dev_reg32 sd_piodatarx;

	/* g1.17 */
	/* g1.18 */
	dev_reg8 sd_cmdbuf[5];
	dev_reg8 reserved32[3];
	/* g1.19 - g1.20 */
	union {
		struct  {
			dev_reg8 sd_rspbuf3;
			dev_reg8 sd_rspbuf2;
			dev_reg8 sd_rspbuf1;
			dev_reg8 sd_rspbuf0;
			dev_reg8 sd_rspbuf5;
			dev_reg8 sd_rspbuf4;
			dev_reg8 sd_rspbuf_reserved[2];
		};
		struct {
			dev_reg32 sd_rspbuf[2];
		};
	};
	/*  unused sd control regs */
	dev_reg32 reserved34[11];
	/* ms card related regs */
	dev_reg32 ms_regs[32];
} EMMCREG;

#define EMMC_RST_seq(base) 				do { \
		base->sdrst = 1; \
		base->sdcrcrst = 1; \
		base->stop_dma_flag = 1; \
		base->hw_dma_rst = 1; \
		base->dmaidle = SP_DMA_NORMAL_STATE; \
		base->dmaidle = SP_RESET_DMA_OPERATION; \
		base->dmaidle = SP_DMA_NORMAL_STATE; \
	}while(0)

#define BLOCK0_DMA_PARA_SET(base, pageIdx, nrPages)  do { \
		base->block0_hw_page_addr_low = ((pageIdx) & 0x0000ffff); \
		base->block0_hw_page_addr_high = (((pageIdx) >> 16) & 0x0000ffff); \
		base->dma_hw_page_num[0] = ((nrPages) - 1); \
    }while(0)

#define BLOCK1_DMA_PARA_SET(base, pageIdx, nrPages)  do{ \
		base->block1_hw_page_addr_low = ((pageIdx) & 0x0000ffff); \
		base->block1_hw_page_addr_high = (((pageIdx) >> 16) & 0x0000ffff); \
		base->dma_hw_page_num[1] = ((nrPages) - 1); \
    }while(0)

#define BLOCK2_DMA_PARA_SET(base, pageIdx, nrPages)  do{ \
		base->block2_hw_page_addr_low = ((pageIdx) & 0x0000ffff); \
		base->block2_hw_page_addr_high = (((pageIdx) >> 16) & 0x0000ffff); \
		base->dma_hw_page_num[2] = ((nrPages) - 1); \
    }while(0)

#define BLOCK3_DMA_PARA_SET(base, pageIdx, nrPages)  do{ \
		base->block3_hw_page_addr_low = ((pageIdx) & 0x0000ffff); \
		base->block3_hw_page_addr_high = (((pageIdx) >> 16) & 0x0000ffff); \
		base->dma_hw_page_num[3] = ((nrPages) - 1); \
    }while(0)

#define SET_DMA_BASE_ADDR(base, addr)  base->dma_base_addr = (u32)(addr)

#define DMA_RESET(base)		do{ \
			base->dmaidle = SP_RESET_DMA_OPERATION; \
			base->dmaidle = SP_DMA_NORMAL_STATE; \
			base->hw_dma_rst = 1; \
			base->dma_hw_page_num[0] = 0; \
			base->dma_hw_page_num[1] = 0; \
			base->dma_hw_page_num[2] = 0; \
			base->dma_hw_page_num[3] = 0; \
			base->hw_block_num = 0; \
		}while(0)

#define SDDATALEN_SET(base, x)			(base->sddatalen = ((x)-1))
#define SDDATALEN_GET(base)				(base->sddatalen + 1)
#define HWEMMC_TIMEOUT(base)				((base->sdstatus >> 7) & 0x01)

#define EMMC_PAGE_NUM_SET(base, x)		(base->sd_page_num = ((x)-1))
#define EMMC_PAGE_NUM_GET(base)			(base->sd_page_num +1)

typedef struct spsdhost {
	volatile struct spemmc_general_regs *base;
	struct clk *clk;
	uint id;
	char *name;
	struct mmc_host *mmc;
	struct mmc_request *mrq;
	struct platform_device *pdev;
	struct semaphore req_sem;

	int dma_sgcount;   /* Used to store dma_mapped sg count */
	int irq;
	uint InsertGPIO;
	int cd_sta;

	uint wrdly;
	uint rddly;
#ifdef SP_MMC_SUPPORT_DDR_MODE
	uint ddr_rd_crc_token_dly;
	uint ddr_wr_data_dly;
#endif
	int need_tune_cmd_timing;
	int need_tune_dat_timing;
}SPEMMCHOST;

typedef struct spemmc_dridata {
	uint id;
} spemmc_dridata_t;

#endif //#ifndef _SPEMMCV2_H_
