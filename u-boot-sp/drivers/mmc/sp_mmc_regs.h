#ifndef __SP_MMC_REGS_H__
#define __SP_MMC_REGS_H__
#include <linux/types.h>

typedef volatile uint dev_reg32;
typedef volatile unchar dev_reg8;

#define SP_SD_HW_DMA_ERROR				BIT(6)
#define SP_SD_HW_DMA_DONE				BIT(7)

/* Controller send CMD timeout */
#define	HWSD_W_SM_CMD24_TIMEOUT	0x240
#define	HWSD_W_SM_CMD25_TIMEOUT	0x241
#define	HWSD_W_SM_CMD12_TIMEOUT	0x242
#define	HWSD_W_SM_CMD13_TIMEOUT	0x24A
#define	HWSD_SM_CMD17_TIMEOUT 	0x40
#define	HWSD_SM_CMD18_TIMEOUT 	0x41
#define	HWSD_SM_CMD12_TIMEOUT	0x42
#define	HWSD_SM_CMD13_TIMEOUT	0x4A

/* Controller receive response timeout */
#define	HWSD_W_SM_CMD24_RSP_TIMEOUT	0x252
#define	HWSD_W_SM_CMD25_RSP_TIMEOUT	0x253
#define	HWSD_W_SM_CMD12_RSP_TIMEOUT	0x243
#define	HWSD_W_SM_CMD13_RSP_TIMEOUT	0x24B
#define	HWSD_SM_CMD13_RSP_TIMEOUT	0x4B
#define	HWSD_SM_CMD12_RSP_TIMEOUT	0x43


#define	HWSD_W_SM_RXCRC_TIMEOUT	0x254	/* Read card crc timeout */
#define	HWSD_W_SM_RXCRC_ERR	0x255	/* Read card crc error */
#define	HWSD_W_SM_RXCRC_TX_TIMEOUT	0x256	/* Timeout occurs after reading card crc */

/* Response of CMD12 command index error */
#define	HWSD_W_SM_CMD12_CMD_ERR	0x246
#define	HWSD_SM_CMD12_CMD_ERR	0x46

/* Response of CMD12 CRC error */
#define	HWSD_W_SM_CMD12_CRCERR	0x244
#define	HWSD_SM_CMD12_CRCERR	0x44

/* Resposne of CMD13 CRC error */
#define	HWSD_W_SM_CMD13_CRCERR	0x24C
#define	HWSD_SM_CMD13_CRCERR	0x4C

/* Response of CMD13 command index error */
#define	HWSD_W_SM_CMD13_CMD_ERR	0x24E
#define	HWSD_SM_CMD13_CMD_ERR	0x4E

/* Response of CMD13 indicates device is busy */
#define	HWSD_W_SM_CMD13_BUSY_ERR	0x24F
#define	HWSD_SM_CMD13_BUSY_ERR	0x4F

#define	HWSD_W_SM_CMD13_ST_TX_TIMEOUT	0x257	/* Controller send CMD13 timeout */
#define	HWSD_W_SM_CRC_ERR	0x249	/* TX data CRC16 check error */
#define	HWSD_SM_CRC_ERR	0x49	/* RX data CRC16 check error */
#define	HWSD_W_SM_DMA_TIMEOUT	0x247	/* TX data timeout */
#define	HWSD_SM_DMA_TIMEOUT	0x47	/* RX data timeout */
#define	HWSD_SM_CMD18_BUSY_TIMEOUT	0x52	/* RX next page data timeout during multi-block read */

/* Reset Pbus channel timeout */
#define	HWSD_W_SM_RST_CHAN_TIMEOUT	0x258
#define	HWSD_SM_RST_CHAN_TIMEOUT	0x53



typedef struct spsd_general_regs {
	/*  group 0 */
	dev_reg32 mediatype:3;				/* 0. lower bits first, mmc set 6?? */
	dev_reg32 reserved_card_mediatype:29;
#define SP_MEDIA_NONE			0
#define SP_MEDIA_SMC			1
#define SP_MEDIA_RESERVED1 		2
#define SP_MEDIA_CF				3
#define SP_MEDIA_SPI			4
#define SP_MEDIA_RESERVED2		5
#define SP_MEDIA_SD				6
#define SP_MEDIA_MEMORY_STICK	7

	/* for the register that take whole bits(0-15, ignore 16-31), just define the field name */
	dev_reg32 reserved0;				/* 1. 32 bits */
	dev_reg32 cpu_page_cnt;			/* 2. 16 bits */
	dev_reg32 card_ctl_page_cnt;		/* 3. 16 bits */
	dev_reg32 sdram_sector_0_size;		/* 4. 16 bits */
	dev_reg32 ring_buffer_on;			/* 5. 1 bit valid */
	dev_reg32 CARD_GCLK_DISABLE;		/* 6. 12 bits */
	dev_reg32 SDRAM_SECTOR_1_ADDR;		/* 7. 32 bits */
	dev_reg32 SDRAM_SECTOR_1_SIZE;		/* 8. 16 bits */
	dev_reg32 SDRAM_SECTOR_2_ADDR;		/* 9. 32 bits */
	dev_reg32 SDRAM_SECTOR_2_SIZE;		/* 10.16 bits */
	dev_reg32 SDRAM_SECTOR_3_ADDR;		/* 11.32 bits */
	dev_reg32 SDRAM_SECTOR_3_SIZE;		/* 12.16 bits */
	dev_reg32 SDRAM_SECTOR_4_ADDR;		/* 13.32 bits */
	dev_reg32 SDRAM_SECTOR_4_SIZE;		/* 14.16 bits */
	dev_reg32 SDRAM_SECTOR_5_ADDR;		/* 15.32 bits */
	dev_reg32 SDRAM_SECTOR_5_SIZE;		/* 16.16 bits */
	dev_reg32 SDRAM_SECTOR_6_ADDR;		/* 17.32 bits */
	dev_reg32 SDRAM_SECTOR_6_SIZE;		/* 18.16 bits */
	dev_reg32 SDRAM_SECTOR_7_ADDR;		/* 19.32 bits */
	dev_reg32 SDRAM_SECTOR_7_SIZE;		/* 20.16 bits */
	dev_reg32 sdram_sector_cnt;		/* 21.3 bits */
#define SP_HW_DMA_MEMORY_SECTORS	8 /*  supports up to 8 fragmented memory blocks */
	dev_reg32 reserved[10];			/* 22-31. */

	/*  group 1 */
	dev_reg32 reserved_group1[11];			/* 0-10. */

	dev_reg32 vol_tmr:2;				/* 11. */
	dev_reg32 sw_set_vol:1;
	dev_reg32 hw_set_vol:1;
	dev_reg32 vol_result:2;
	dev_reg32 Reserved_sd_vol_ctrl:26;

	dev_reg32 sdcmpen:1;				/* 12. */
	dev_reg32 sd_cmp:1;
	dev_reg32 sd_cmp_clr:1;
	dev_reg32 reserved0_sd_int:1;
	dev_reg32 sdio_int_en:1;
	dev_reg32 sdio_int:1;
	dev_reg32 sdio_int_clr:1;
	dev_reg32 reserved1_sd_int:25;

	dev_reg32 sd_page_num;				/* 13. 16 bits */

	dev_reg32 sdpiomode:1;				/* 14. */
	dev_reg32 sdddrmode:1;
	dev_reg32 sd_len_mode:1;
#define SP_LENMODE_SEND_STOP 				0
#define SP_LENMODE_NOT_SEND_STOP 			1
	dev_reg32 reserved0_sd_config0:1;
	dev_reg32 sd_trans_mode:2;
#define SP_TRANSACTION_MODE_CMD_RSP			0
#define SP_TRANSACTION_MODE_WRITE_DATA		1
#define SP_TRANSACTION_MODE_READ_DATA		2
	dev_reg32 sdautorsp:1; 				/* sdautorsp should be enabled except CMD0(idle, norsp) and read command(rx data, including acmd51, acmd13, ...) */
#define SP_AUTORSP_NO_RX_RESPONSE 			0
#define SP_AUTORSP_ATUO_RX_RESPONSE 		1
	dev_reg32 sdcmddummy:1; 				/* sdcmddummy should be enabled except read command(rx data, including acmd51, acmd13, ...) */
	dev_reg32 sdrspchk_en:1;
	dev_reg32 reserved1_sd_config0:23;

	dev_reg32 rwc:1;					/* 15. */
	dev_reg32 s4mi:1;
	dev_reg32 resu:1;
	dev_reg32 sus_req:1;
	dev_reg32 con_req:1;
	dev_reg32 sus_data_flag:1;
	dev_reg32 reserved_sdio_ctrl:26;

	dev_reg32 sdrst:1;					/* 16. */
	dev_reg32 sdcrcrst:1;
	dev_reg32 sdiorst:1;
	dev_reg32 reserved_sd_rst:29;

	dev_reg32 sdfqsel:12;				/* 17. */
	dev_reg32 sddatawd:1;				/* bus width:0:1bit   1:4bit */
	dev_reg32 sdrsptype:1;
#define SP_SD_RSP_TYPE_LEN_48_BITS		0
#define SP_SD_RSP_TYPE_LEN_136_BITS		1
	dev_reg32 sdrsptmren:1;
	dev_reg32 sdcrctmren:1;
	dev_reg32 sdmmcmode:1;
	dev_reg32 selci:1;
	dev_reg32 mmc8_en:1;
	dev_reg32 rx4b_en:1;
	dev_reg32 sdiomode:1;
	dev_reg32 reserved_sd_config:11;

	dev_reg32 sdctrl_trigger_cmd:1;	/* 18. */
	dev_reg32 sdctrl_trigger_dummy:1;
	dev_reg32 reserved_sd_ctrl:30;

	dev_reg32 sdstatus:14;				/* 19. */
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

	dev_reg32 reserved_sd_status:18;

	dev_reg32 sdstate:3;				/* 20. */
	dev_reg32 reserved0_sd_state:1;
	dev_reg32 sdcrdcrc:3;
#define WRITE_CRC_TOKEN_CORRECT		2
#define WRITE_CRC_TOKEN_ERROR		5
	dev_reg32 reserved1_sd_state:1;
	dev_reg32 sdstate_new:7;
#define SDSTATE_NEW_FINISH_IDLE 		BIT(6)
#define SDSTATE_NEW_ERROR_TIMEOUT 		BIT(5)
	dev_reg32 reserved2_sd_state:17;

	dev_reg32 sddatalen:11;			/* 21. */
	dev_reg32 reserved_sd_blocksize:21;

	dev_reg32 hw_sd_hcsd_en:1;			/* 22. */
	dev_reg32 hw_sd_dma_type:2;
#define SP_DMA_TYPE_SINGLEBLOCK_CMD	1
#define SP_DMA_TYPE_MULTIBLOCK_CMD		2
	dev_reg32 hw_sd_cmd13_en:1;
	dev_reg32 hwsd_stb_en:1;			/* ?? */
	dev_reg32 cmd13_rsp_cnt:11;
	dev_reg32 reserved_sd_hwdma_config:16;

	dev_reg32 sdrsptmr:11;				/* 23. */
	dev_reg32 sd_high_speed_en:1;
	dev_reg32 sd_wr_dly_sel:3;
	dev_reg32 reserved_sd_timing_config0:17;

	dev_reg32 sdcrctmr:11;				/* 24. */
	dev_reg32 reserved_sdcrctmr:2;
	dev_reg32 sd_rd_dly_sel:3;
	dev_reg32 reserved_sd_timing_config1:16;

	dev_reg32 sdpiodatatx;				/* 25. 16 bits, the write buffer always 2 bytes. */
	dev_reg32 sdpiodatarx;				/* 26. when rx4b_en=1, read 4 bytes from this register. When rx4b_en=0, read 2 bytes. Little endian. */
#define SP_SD_CMDBUF_SIZE 5
	dev_reg32 sd_cmdbuf[SP_SD_CMDBUF_SIZE];	/* 27-31. 8 bits. the first bit is the start bit. So the command should be cmd|0x40 */
	/* total 5 bytes, and the controller will append the crc7. Overall 6 bytes (48 bits) for the command */

	/*  group 2 */
	union {
		struct {
			dev_reg32 sdrspbuf3:8;				/* 0. */
			dev_reg32 sdrspbuf2:8;
			dev_reg32 sdrspbuf1:8;
			dev_reg32 sdrspbuf0:8;

			dev_reg32 sdrspbuf5:8;				/* 1. */
			dev_reg32 sdrspbuf4:8;
			dev_reg32 reserved_sd_rspbuf:16;
		};
		dev_reg32 sd_rspbuf[2];
	};

	dev_reg32 sdcrc16evenbuf0:16;		/* 2. */
	dev_reg32 sdcrc16evenbuf4:16;

	dev_reg32 sdcrc16evenbuf1:16;		/* 3. */
	dev_reg32 sdcrc16evenbuf5:16;

	dev_reg32 sdcrc16evenbuf2:16;		/* 4. */
	dev_reg32 sdcrc16evenbuf6:16;

	dev_reg32 sdcrc16evenbuf3:16;		/* 5. */
	dev_reg32 sdcrc16evenbuf7:16;

	dev_reg32 reserved0_sd_crc7buf:1;	/* 6. */
	dev_reg32 sdcrc7buf:7;
	dev_reg32 reserved1_sd_crc7buf:24;

	dev_reg32 sdcrc16buf0:16;			/* 7. */
	dev_reg32 sdcrc16buf4:16;

	dev_reg32 hwsd_sm:10;				/* 8. */
	dev_reg32 reserved_sd_hw_state:22;

	dev_reg32 sdcrc16buf1:16;			/* 9. */
	dev_reg32 sdcrc16buf5:16;

	dev_reg32 hwsd_cmd13_rca:16;		/* 10. */
	dev_reg32 reserved_sd_hw_cmd13_rca:16;

	dev_reg32 sdcrc16buf2:16;			/* 11. */
	dev_reg32 sdcrc16buf6:16;

	dev_reg32 tx_dummy_num:9;			/* 12. */
	dev_reg32 reserved_sd_tx_dummy_num:23;

	dev_reg32 sdcrc16buf3:16;			/* 13. */
	dev_reg32 sdcrc16buf7:16;

	dev_reg32 sd_clk_dly_sel:3;			/* 14. */
#define SP_SD_RXDATTMR_MAX	((1 << 29) - 1)
	dev_reg32 sdrxdattmr_sel:29;

	dev_reg32 reserved_reserved_2_15;		/* 15. */

	dev_reg32 mspiomode:1;				/* 16. */
	dev_reg32 reserved0_ms_piodmarst:3;
	dev_reg32 msreset:1;
	dev_reg32 mscrcrst:1;
	dev_reg32 msclrerr:1;
	dev_reg32 reserved1_ms_piodmarst:25;

	/* the following ms register not yet use in sd. */
	dev_reg32 mscommand:4;					/* 17. */
	dev_reg32 datasize:4;
	dev_reg32 reserved_ms_cmd:24;

	dev_reg32 reserved_reserved_2_18;		/* 18. */

	dev_reg32 hwms_sm:10;
	dev_reg32 reserved_hw_hw_state:22;		/* 19. */

	dev_reg32 msspeed:8;					/* 20. */
	dev_reg32 msdatwd:1;
	dev_reg32 msspeed8:1;
	dev_reg32 mstype:1;
	dev_reg32 ms_rdy_chk3_en:1;
	dev_reg32 ms_wdat_sel:3;
	dev_reg32 reserved_ms_modespeed:17;

	dev_reg32 ms_busy_rdy_timer:5;			/* 21. */
	dev_reg32 reserved0_ms_timeout:3;
	dev_reg32 ms_clk_dly_sel:3;
	dev_reg32 ms_rdat_dly_sel:3;
	dev_reg32 reserved_ms_timeout:18;

	dev_reg32 msstate:8;					/* 22. */
	dev_reg32 reserved_ms_state:24;

	dev_reg32 reserved0_ms_status:1;		/* 23. */
	dev_reg32 mserror_busy_rdy_timeout:1;
	dev_reg32 msbs:1;
	dev_reg32 reserved1_ms_status:1;
	dev_reg32 ms_data_in:4;
	dev_reg32 reserved2_ms_status:24;

#define MS_RDDATA_SIZE 4
	dev_reg32 ms_rddata[MS_RDDATA_SIZE];	/* 24-27. 8 bits */

	dev_reg32 mscrc16buf_low:8;				/* 28. */
	dev_reg32 reserved_ms_crcbuf0:24;

	dev_reg32 mscrc16buf_high:8;			/* 29. */
	dev_reg32 reserved_ms_crcbuf1:24;

	dev_reg32 mserror:1;					/* 30. */
	dev_reg32 mscrc16cor:1;
	dev_reg32 reserved_mccrcerror:30;

	dev_reg32 mspiordy:1;					/* 31. */
	dev_reg32 reserved_ms_piordy:31;

	/*  group 3 */
#define SP_MS_WD_DATA_SIZE 16
	dev_reg32 ms_wd_data[SP_MS_WD_DATA_SIZE];
	dev_reg32 reserved_ms[16];

	/*  group 4 */
	dev_reg32 dmadata:16;					/* 0. */
	dev_reg32 reserved_dma_data:16;

	dev_reg32 dmasrc:3;					/* 1. */
	dev_reg32 reserved0_dma_data:1;
	dev_reg32 dmadst:3;
	dev_reg32 reserved1_dma_data:25;

	dev_reg32 dmasize:11;					/* 2. */
	dev_reg32 reserved_dma_size:21;

	dev_reg32 stop_dma_flag:1;				/* 3. */
	dev_reg32 hw_dma_rst:1;
	dev_reg32 rst_cnad:1;					/* When set this bit to 1, wait it change back to 0 */
	dev_reg32 reserved_dma_hw_stop_rst:29;
	dev_reg32 dmaidle:1;					/* 4. */
#define SP_DMA_NORMAL_STATE			0
#define SP_RESET_DMA_OPERATION		1
	dev_reg32 reserved_dma_ctrl:31;

	dev_reg32 dma_base_addr_low:16;		/* 5. */
	dev_reg32 reserved_dma_base_addr_low:16;

	dev_reg32 dma_base_addr_high:16;		/* 6. */
	dev_reg32 reserved_dma_base_addr_high:16;

	dev_reg32 hw_dma_en:1;					/* 7. */
	dev_reg32 hw_block_mode_en:1;
#define SP_DMA_BLOCK_MODE_PAGE_LEVEL	0
#define SP_DMA_BLOCK_MODE_BLOCK_LEVEL	1

	dev_reg32 reserved_dma_hw_en:30;


	dev_reg32 block0_hw_page_addr_low:16;	/* 8. */
	dev_reg32 reserved_block0_hw_page_addr_low:16;

	dev_reg32 block0_hw_page_addr_high:16;	/* 9. */
	dev_reg32 reserved_block0_hw_page_addr_high:16;

	dev_reg32 block1_hw_page_addr_low:16;	/* 10. */
	dev_reg32 reserved_block1_hw_page_addr_low:16;

	dev_reg32 block1_hw_page_addr_high:16;	/* 11. */
	dev_reg32 reserved_block1_hw_page_addr_high:16;

	dev_reg32 block2_hw_page_addr_low:16;	/* 12. */
	dev_reg32 reserved_block2_hw_page_addr_low:16;

	dev_reg32 block2_hw_page_addr_high:16;	/* 13. */
	dev_reg32 reserved_block2_hw_page_addr_high:16;

	dev_reg32 block3_hw_page_addr_low:16;	/* 14. */
	dev_reg32 reserved_block3_hw_page_addr_low:16;

	dev_reg32 block3_hw_page_addr_high:16;	/* 15. */
	dev_reg32 reserved_block3_hw_page_addr_high:16;

#define SP_DMA_HW_PAGE_NUM_SIZE 4
	dev_reg32 dma_hw_page_num[SP_DMA_HW_PAGE_NUM_SIZE];	/* 16-19. 16 bits the actual page number is x+1 */

	dev_reg32 hw_block_num:2;				/* 20. */
	dev_reg32 hw_blcok_cnt:2;
	dev_reg32 reserved_dma_hw_block_num:28;

	dev_reg32 dmastart:1;					/* 21. */
	dev_reg32 reserved_dma_start:31;

	dev_reg32 hw_page_cnt;					/* 22. 16 bits */

	dev_reg32 dma_cmp:1;					/* 23. */
	dev_reg32 reserved_dma_cmp:31;

	dev_reg32 dmacmpen_interrupt:1;		/* 24. */
	dev_reg32 reserved_dma_int_en:31;

	dev_reg32 reserved_reserved_4_25;		/* 25. */
	dev_reg32 hw_wait_num_low;				/* 26. 16 bits */
	dev_reg32 hw_wait_num_high;			/* 27. 16 bits */
	dev_reg32 hw_delay_num;				/* 28. 16 bits */

	dev_reg32 incnt:11;					/* 29. */
	dev_reg32 outcnt:11;
	dev_reg32 lmst_sm:2;
	dev_reg32 reserved_dma_debug:8;

} SDREG;


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
	dev_reg32 reserved4:16;
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
			dev_reg32 resume_boot:1;
			dev_reg32 reserved14:7;
			dev_reg32 stop_page_num:16;
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
			dev_reg32 rx4_en:1;
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
			dev_reg32 sd_wr_dat_dly_sel:3;
			dev_reg32 reserved28:1;
			dev_reg32 sd_wr_cmd_dly_sel:3;
			dev_reg32 reserved29:1;
			dev_reg32 sd_rd_rsp_dly_sel:3;
			dev_reg32 reserved29_1:1;
			dev_reg32 sd_rd_dat_dly_sel:3;
			dev_reg32 reserved29_2:1;
			dev_reg32 sd_rd_crc_dly_sel:3;
			dev_reg32 reserved29_3:9;
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

#endif /* __SP_MMC_REGS_H__ */
