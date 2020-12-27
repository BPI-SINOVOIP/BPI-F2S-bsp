/**
 * (C) Copyright 2019 Sunplus Technology. <http://www.sunplus.com/>
 *
 * Sunplus SD host controller v2.0
 */
#ifndef __SPSDC_H__
#define __SPSDC_H__

#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>

#define SPSDC_SYS_CLK	222750000
#define SPSDC_MIN_CLK	400000
#define SPSDC_MAX_CLK	111375000
#define SPSDC_MAX_BLK_COUNT 65535



#define __rsvd_regs(l) __append_suffix(l, __COUNTER__)
#define __append_suffix(l, s) _append_suffix(l, s)
#define _append_suffix(l, s) reserved##s[l]


/* #define SPMMC_SUPPORT_VOLTAGE_1V8 */
/* #define SPMMC_EMMC_VCCQ_1V8 */

#define SPMMC_SUPPORT_VOLTAGE_1V8
//#define SPMMC_SDIO_1V8

struct spsdc_regs {
#define SPSDC_MEDIA_NONE 0
#define SPSDC_MEDIA_SD 6
#define SPSDC_MEDIA_MS 7

	/* g0*/
	/* g0.0 card_mediatype_srcdst*/

#define SPSDC_dmadst_w03              8
#define SPSDC_dmasrc_w03              4
#define SPSDC_MediaType_w03           0
#define SPSDC_DMA_READ                0x12
#define SPSDC_DMA_WRITE               0x21		
	u32 card_mediatype_srcdst;
	
	
	u32 card_cpu_page_cnt;
	u32 sdram_sector_0_size;
	u32 dma_base_addr;
	
	
	/* g0.4 hw_dma_ctrl*/	
#define SPSDC_HW_SD_CMD13_RCA_w16     16
#define SPSDC_HW_BLOCK_NUM_w02        12
#define SPSDC_dmastart_w01            11
#define SPSDC_dmaidle_w01             10
#define SPSDC_HW_DMA_RST_w01          9
#define SPSDC_STOP_DMA_FLAG_w01       8
#define SPSDC_HW_SD_CMD13_EN_w01      6
#define SPSDC_HW_SD_DMA_TYPE_w02      4
#define SPSDC_HW_SD_HCSD_EN_w01       3
#define SPSDC_HW_DMA_EN_w01           1
	u32 hw_dma_ctrl;
	
	
	u32 card_gclk_disable;
#define SPSDC_MAX_DMA_MEMORY_SECTORS 8 /* support up to 8 fragmented memory blocks */
	u32 sdram_sector_1_addr;
	u32 sdram_sector_1_size;
	u32 sdram_sector_2_addr;
	u32 sdram_sector_2_size;
	u32 sdram_sector_3_addr;
	u32 sdram_sector_3_size;
	u32 sdram_sector_4_addr;
	u32 sdram_sector_4_size;
	u32 sdram_sector_5_addr;
	u32 sdram_sector_5_size;
	u32 sdram_sector_6_addr;
	u32 sdram_sector_6_size;
	u32 sdram_sector_7_addr;
	u32 sdram_sector_7_size;
	u32 sdram_sector_cnt;
	u32 dma_hw_page_addr0;
	u32 dma_hw_page_addr1;
	u32 dma_hw_page_addr2;
	u32 dma_hw_page_addr3;
	u32 dma_hw_page_num0;
	u32 dma_hw_page_num1;
	u32 dma_hw_page_num2;
	u32 dma_hw_page_num3;
	u32 dma_hw_wait_num;
	u32 dma_hw_delay_num;
	u32 dma_debug;

	/* g1*/
	u32 boot_ctrl;
	
	/* g1.1 sd_vol_ctrl*/		
#define SPSDC_SWITCH_VOLTAGE_1V8_FINISH		1
#define SPSDC_SWITCH_VOLTAGE_1V8_ERROR		2
#define SPSDC_SWITCH_VOLTAGE_1V8_TIMEOUT	3

#define SPSDC_vol_result_w01          4
#define SPSDC_hw_set_vol_w01          3
#define SPSDC_sw_set_vol_w01          2
#define SPSDC_vol_tmr_w02             0
	u32 sd_vol_ctrl;
	
	/* g1.2 sd_int*/	
#define SPSDC_SDINT_SDCMPEN	BIT(0)
#define SPSDC_SDINT_SDCMP	BIT(1)
#define SPSDC_SDINT_SDIOEN	BIT(3)
#define SPSDC_SDINT_SDIO	BIT(4)

#define SPSDC_hwdmacmpclr_w01         11
#define SPSDC_HW_DMA_CMP_w01          10
#define SPSDC_hwdmacmpen_w01          9
#define SPSDC_DETECT_INT_CLR_w01      8
#define SPSDC_DETECT_INT_w01          7
#define SPSDC_DETECT_INT_EN_w01       6
#define SPSDC_sdio_init_clr_w01       5
#define SPSDC_sdio_init_w01           4
#define SPSDC_sdio_init_en_w01        3
#define SPSDC_sd_cmp_clr_w01          2
#define SPSDC_sd_cmp_w01              1
#define SPSDC_sdcmpen_w01             0
	u32 sd_int;
	u32 sd_page_num;
	
#define SPSDC_MODE_SDIO	2
#define SPSDC_MODE_EMMC	1
#define SPSDC_MODE_SD	0

	/* g1.4 sd_config0*/
#define SPSDC_sdfqsel_w12             20
#define SPSDC_selci_w01               19
#define SPSDC_mmc8_en_w01             18
#define SPSDC_detect_tmr_w02          16
#define SPSDC_sdrsptype_w01           15
#define SPSDC_rx4_en_w01              14
#define SPSDC_sdcrctmren_w01          13
#define SPSDC_sdrsptmren_w01          12
#define SPSDC_sddatawd_w01            11
#define SPSDC_sdmmcmode_w01           10
#define SPSDC_sdiomode_w01            9
#define SPSDC_sdrspchk_w01            8
#define SPSDC_sdcmddummy_w01          7
#define SPSDC_sdautorsp_w01           6
#define SPSDC_trans_mode_w02          4
#define SPSDC_ddr_rx_first_hcyc_w01   3
#define SPSDC_sd_len_mode_w01         2
#define SPSDC_sdddrmode_w01           1
#define SPSDC_sdpiomode_w01           0
	u32 sd_config0;

	/* g1.5 sdio_ctrl*/	
#define SPSDC_INT_MULTI_TRIG_w01      6
#define SPSDC_SUS_DATA_FLAG_w01       4
#define SPSDC_CON_REQ_w01             3
#define SPSDC_RESU_w01                2
#define SPSDC_S4MI_w01                1
#define SPSDC_RWC_w01                 0
	u32 sdio_ctrl;
	
	/* g1.6 sd_rst*/	
#define SPSDC_sdiorst_w01             2
#define SPSDC_sdcrcrst_w01            1
#define SPSDC_sdrst_w01               0		
	u32 sd_rst;

	/* g1.7 sd_ctrl*/	
#define SPSDC_emmcctrl_w01            3
#define SPSDC_sdioctrl_w01            2
#define SPSDC_sdctrl1_w01             1
#define SPSDC_sdctrl0_w01             0		
	u32 sd_ctrl;
	
#define SPSDC_SDSTATUS_DUMMY_READY			BIT(0)
#define SPSDC_SDSTATUS_RSP_BUF_FULL			BIT(1)
#define SPSDC_SDSTATUS_TX_DATA_BUF_EMPTY		BIT(2)
#define SPSDC_SDSTATUS_RX_DATA_BUF_FULL			BIT(3)
#define SPSDC_SDSTATUS_CMD_PIN_STATUS			BIT(4)
#define SPSDC_SDSTATUS_DAT0_PIN_STATUS			BIT(5)
#define SPSDC_SDSTATUS_RSP_TIMEOUT			BIT(6)
#define SPSDC_SDSTATUS_CARD_CRC_CHECK_TIMEOUT		BIT(7)
#define SPSDC_SDSTATUS_STB_TIMEOUT			BIT(8)
#define SPSDC_SDSTATUS_RSP_CRC7_ERROR			BIT(9)
#define SPSDC_SDSTATUS_CRC_TOKEN_CHECK_ERROR		BIT(10)
#define SPSDC_SDSTATUS_RDATA_CRC16_ERROR		BIT(11)
#define SPSDC_SDSTATUS_SUSPEND_STATE_READY		BIT(12)
#define SPSDC_SDSTATUS_BUSY_CYCLE			BIT(13)
#define SPSDC_SDSTATUS_DAT1_PIN_STATUS			BIT(14)
#define SPSDC_SDSTATUS_SD_SENSE_STATUS			BIT(15)
#define SPSDC_SDSTATUS_BOOT_ACK_TIMEOUT			BIT(16)
#define SPSDC_SDSTATUS_BOOT_DATA_TIMEOUT		BIT(17)
#define SPSDC_SDSTATUS_BOOT_ACK_ERROR			BIT(18)
	u32 sd_status;

	/* g1.9 sd_state*/
#define SPSDC_SDSTATE_IDLE	(0x0)
#define SPSDC_SDSTATE_TXDUMMY	(0x1)
#define SPSDC_SDSTATE_TXCMD	(0x2)
#define SPSDC_SDSTATE_RXRSP	(0x3)
#define SPSDC_SDSTATE_TXDATA	(0x4)
#define SPSDC_SDSTATE_RXCRC	(0x5)
#define SPSDC_SDSTATE_RXDATA	(0x6)
#define SPSDC_SDSTATE_MASK	(0x7)
#define SPSDC_SDSTATE_BADCRC	(0x5)
#define SPSDC_SDSTATE_ERROR	BIT(13)
#define SPSDC_SDSTATE_FINISH	BIT(14)
#define SPSDC_sdstate_new_w01         8
#define SPSDC_sdcrdcrc_w03            4
#define SPSDC_sdstate_w03             0	
	u32 sd_state;


	u32 sd_hw_state;
	u32 sd_blocksize;
	
	/* g1.12 sd_config1*/
#define SPSDC_sdhigh_speed_en_w01     31
#define SPSDC_sdrsptmr_w11            20
#define SPSDC_sdcrctmr_w11            9
#define SPSDC_TX_DUMMY_NUM_w09        0		
	u32 sd_config1;

	/* g1.13 sd_timing_config0*/
#define SPSDC_sd_rd_crc_dly_sel_w03   20
#define SPSDC_sd_rd_dat_dly_sel_w03   16
#define SPSDC_sd_rd_rsp_dly_sel_w03   12
#define SPSDC_sd_wr_cmd_dly_sel_w03   8
#define SPSDC_sd_wr_dat_dly_sel_w03   4
#define SPSDC_sd_clk_dly_sel_w03      0		
	u32 sd_timing_config0;

	
	u32 sd_rx_data_tmr;
	u32 sd_piodatatx;
	u32 sd_piodatarx;
	u32 sd_cmdbuf0_3;
	u32 sd_cmdbuf4;
	u32 sd_rspbuf0_3;
	u32 sd_rspbuf4_5;

	u32 __rsvd_regs(32);
};

struct spsdc_tuning_info {
	int enable_tuning;
	int need_tuning;
#define SPSDC_MAX_RETRIES (8 * 8)
	int retried; /* how many times has been retried */

	u32 rd_crc_dly:3;
	u32 rd_dat_dly:3;
	u32 rd_rsp_dly:3;
	u32 wr_cmd_dly:3;
	u32 wr_dat_dly:3;
	u32 clk_dly:3;
		
};

struct spsdc_compatible {
	int mode; /* SD/SDIO/eMMC */
};


struct spsdc_host {
	struct spsdc_regs *base;
	struct clk *clk;
	struct reset_control *rstc;
	int mode; /* SD/SDIO/eMMC */
	spinlock_t lock; /* controller lock */
	struct mutex mrq_lock;
	/* tasklet used to handle error then finish the request */
	struct tasklet_struct tsklet_finish_req;
	struct mmc_host *mmc;
	struct mmc_request *mrq; /* current mrq */

	int irq;
	int use_int; /* should raise irq when done */
	int power_state; /* current power state: off/up/on */
	int ddr_enabled;
	int signal_voltage;

#define SPSDC_DMA_MODE 0
#define SPSDC_PIO_MODE 1
	int dmapio_mode;
	/* for purpose of reducing context switch, only when transfer data that
	   length is greater than `dma_int_threshold' should use interrupt */
	int dma_int_threshold;
	int dma_use_int; /* should raise irq when dma done */
	struct sg_mapping_iter sg_miter; /* for pio mode to access sglist */
	struct spsdc_tuning_info tuning_info;
};

#endif /* #ifndef __SPSDC_H__ */
