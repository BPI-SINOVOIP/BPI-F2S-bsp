/******************************************************************************
*                          Include File
*******************************************************************************/
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#include <linux/mmc/sd.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>

#include <linux/miscdevice.h>
#include <asm/cacheflush.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>

#include <asm-generic/io.h>
#include <asm/bitops.h>
#include "spemmc.h"

#ifdef SP_MMC_SUPPORT_DDR_MODE
#define SP_MMC_DDR_READ_CRC_STATUS_DLY 0
#define SP_MMC_DDR_WRITE_DLY 1
#endif

/******************************************************************************
*                          MACRO Function Define
*******************************************************************************/
#define SPEMMC_MAX_CLOCK  CLOCK_52M     /* Max supported SD Card frequency */
#define SPEMMCV2_SDC_NAME "sp7021-emmc"
#define MAX_SDDEVICES   2
#define SPEMMC_DEVICE_MASK 1
#define SPEMMC_READ_DELAY  2		/* delay for sampling data */
#define SPEMMC_WRITE_DELAY 1		/* delay for output data   */
#define DUMMY_COCKS_AFTER_DATA     8

#define MAX_DLY_CLK     7 		/* max  delay clocks */
#define ENABLE_TIMING_TUNING 0

/* log levels */
#define MMC_LOGLEVEL_FATAL		0x01
#define MMC_LOGLEVEL_ERROR		0x02
#define MMC_LOGLEVEL_DEBUG		0x04
#define MMC_LOGLEVEL_IF 		0x10
#define MMC_LOGLEVEL_PK 		0x20
#define MMC_LOGLEVEL_COUNTER	0x100
#define MMC_LOGLEVEL_WAITTIME	0x200
#define MMC_LOGLEVEL_DUMPREG	0x1000
#define MMC_LOGLEVEL_MINI		0x2000

/* enable fatal error messages temporarily */
static u32 loglevel = 0x003;

#if 1
#define FATAL(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_FATAL)) \
		printk(KERN_ERR "[eMMC FATAL]: %s: " fmt, __func__ , ## args)

#define EPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_ERROR)) \
		printk(KERN_ERR "[eMMC ERROR]: %s: " fmt, __func__ , ## args)

#define DPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_DEBUG)) \
		printk(KERN_INFO "[eMMC DBG]: %s: " fmt, __func__ , ## args)

#define IFPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_IF)) \
		printk(KERN_NOTICE "[eMMC IF]: %s:" fmt, __func__, ## args)

#define pk(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_PK)) \
		printk(KERN_NOTICE "[eMMC PK]: %s: " fmt, __func__ , ## args)

#define CPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_COUNTER)) \
		printk(KERN_INFO "[eMMC COUNTER]: %s: " fmt, __func__ , ## args)

#define WPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_WAITTIME)) \
		printk(KERN_INFO "[eMMC WAITTIME]: %s: " fmt, __func__ , ## args)

#define REGPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_DUMPREG)) \
		printk(KERN_INFO "[eMMC REG]: %s: " fmt, __func__ , ## args)

#define MPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_MINI)) \
		printk(KERN_INFO "[eMMC]: %s: " fmt, __func__ , ## args)

#else

#define FATAL(fmt, args...)
#define EPRINTK(fmt, args...)
#define DPRINTK(fmt, args...)
#define IFPRINTK(fmt, args...)
#define pk(fmt, args...)
#define CPRINTK(fmt, args...)
#define WPRINTK(fmt, args...)
#define REGPRINTK(fmt, args...)
#define MPRINTK(fmt, args...)

#endif

#define IS_DMA_ADDR_2BYTE_ALIGNED(x)  (!((x) & 0x1))


/******************************************************************************
*                          Global Variables
*******************************************************************************/

const static spemmc_dridata_t spemmc_driv_data[] = {
	{.id = 0,},
};

static const struct of_device_id spemmc_of_id[] = {
	{
		.compatible = "sunplus,sp7021-emmc",
		.data = &spemmc_driv_data[0],
	},
	{}

};
MODULE_DEVICE_TABLE(of, spemmc_of_id);

static int dma_mode = 1;
module_param(dma_mode, int, 0664);


/******************************************************************************
*                         Function Prototype
*******************************************************************************/
static int spemmc_get_dma_dir(SPEMMCHOST *, struct mmc_data *);
static void sphe_mmc_finish_request(SPEMMCHOST *, struct mmc_request *);
static void spemmc_set_cmd(SPEMMCHOST *host, struct mmc_request *mrq);
static void spemmc_set_data_info(SPEMMCHOST *host, struct mmc_data * data);
static void spemmc_prepare_cmd_rsp(SPEMMCHOST *host);

static inline bool is_crc_token_valid(SPEMMCHOST *host)
{
	return (host->base->sdcrdcrc == 0x2 || host->base->sdcrdcrc == 0x5);
}

#define REG_BASE 0x9c000000
#define RF_GRP(_grp, _reg) ((((_grp) * 32 + (_reg)) * 4) + REG_BASE)
#define RF_MASK_V(_mask, _val) (((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask) (((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask) (((_mask) << 16) | 0)

static int reset_controller(SPEMMCHOST *host)
{
	void __iomem  *reg  = ioremap_nocache(RF_GRP(0, 24), 4);
	if (!reg) {
		EPRINTK("ioremap for reset failed!\n");
		return -ENOMEM;
	}
	writel(RF_MASK_V_SET(1<<14), reg);
	mdelay(10);
	writel(RF_MASK_V_CLR(1<<14), reg);
	iounmap(reg);
	return 0;
}

static inline int emmc_get_in_clock(SPEMMCHOST *host)
{
	return clk_get_rate(host->clk);
}

static int Sd_Bus_Reset_Channel(SPEMMCHOST *host)
{
	/*reset Central FIFO*/
	host->base->hw_dma_rst = 1;
	/* Wait for channel reset to complete */
	while (host->base->hwsd_sm & SP_EMMC_HW_DMA_ERROR) {
	}

	return SPEMMC_SUCCESS;
}

static int Reset_Controller(SPEMMCHOST *host)
{
	DPRINTK("controller reset\n");
	EMMC_RST_seq(host->base);
	return Sd_Bus_Reset_Channel(host);
}

static void spemmc_controller_init(SPEMMCHOST *host)
{
	host->base->sdddrmode = 0;
	host->base->sdpiomode = 1;
	host->base->rx4b_en = 1;
	host->base->sdiomode = SP_EMMC_CARD;

	host->base->sdrsptmr = 0x7ff;
	host->base->sdrsptmren = 1;
	host->base->sdcrctmr = 0x7ff;
	host->base->sdcrctmren = 1;
	host->base->sdmmcmode = SP_MMC_MODE;
	host->base->sd_rxdattmr = SP_EMMC_RXDATTMR_MAX;
	host->base->mediatype = 6;
}
/*
 * Set SD card clock divider value based on the required clock in HZ
 * TODO: Linux passes clock as 0, look into it
 */
static void spemmc_set_ac_timing(SPEMMCHOST *host, struct mmc_ios *ios)
{
	struct mmc_host *mmc = host->mmc;
	uint clkrt, sys_clk, act_clock;
	uint rd_dly = SPEMMC_READ_DELAY, wr_dly = SPEMMC_WRITE_DELAY;
	uint clock = ios->clock;
	/* Check if requested frequency is above/below supported range */
	if (clock < mmc->f_min)
		clock = mmc->f_min;
	else if (clock > mmc->f_max)
		clock = mmc->f_max;

	sys_clk = emmc_get_in_clock(host);
	clkrt = (sys_clk / clock) - 1;

	/* Calculate the actual clock for the divider used */
	act_clock = sys_clk / (clkrt + 1);
	if (act_clock > clock)
		clkrt++;
	/* printk("sys_clk =%u, act_clock=%u, clkrt = %u\n", sys_clk, act_clock, clkrt); */
	/* check clock divider boundary and correct it */
	if (clkrt > 0xFFF)
		clkrt = 0xFFF;

	host->base->sdfqsel = clkrt;

	/* Delay 4 msecs for now (wait till clk stabilizes?) */
	mdelay(4);

	if (ios->timing != MMC_TIMING_LEGACY) {
		host->base->sd_high_speed_en = 1;
		if ((ios->timing == MMC_TIMING_UHS_DDR50) || (ios->timing == MMC_TIMING_MMC_DDR52)) {
			host->base->sdddrmode = 1;
		}
	} else {
		host->base->sd_high_speed_en = 0;
	}

	/* Write delay: Controls CMD, DATA signals timing to SD Card */
	host->base->sd_wr_dly_sel = wr_dly;
	/* Read delay: Controls timing to sample SD card's CMD, DATA signals */
	host->base->sd_rd_dly_sel = rd_dly;
	#ifdef SP_MMC_SUPPORT_DDR_MODE
	host->ddr_rd_crc_token_dly = SP_MMC_DDR_READ_CRC_STATUS_DLY;
	host->ddr_wr_data_dly = SP_MMC_DDR_WRITE_DLY;
	#endif

	return;
}

/* Sets bus width accordingly */
static void spemmc_set_bus_width(SPEMMCHOST *host, u32 bus_width)
{
	switch (bus_width) {
	case MMC_BUS_WIDTH_8:
		host->base->sddatawd = 0;
		host->base->mmc8_en = 1;
		break;
	case MMC_BUS_WIDTH_4:
		host->base->sddatawd = 1;
		host->base->mmc8_en = 0;
		break;
	case MMC_BUS_WIDTH_1:
		host->base->sddatawd = 0;
		host->base->mmc8_en = 0;
		break;
	default:
		EPRINTK("unknown bus wide %d\n", bus_width);
		break;
	}

	return;
}

static void spemmc_trigger_sdstate(SPEMMCHOST *host)
{
	host->base->sdctrl0 = 1;   /* Start transaction */
}


#define SP_EMMC_WAIT_RSP_BUFF_FULL(host) \
	do { \
		while (1) { \
			if ((host)->base->sdstatus & SP_SDSTATUS_RSP_BUF_FULL) \
			break; \
			if ((host)->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) \
			return; \
		} \
	} while (0)

#define SP_MMC_SWAP32(x)		((((x) & 0x000000ff) << 24) | \
				 (((x) & 0x0000ff00) <<  8) | \
				 (((x) & 0x00ff0000) >>  8) | \
				 (((x) & 0xff000000) >> 24)   \
				)

/*
 * Receive 136 bits response, and pass it back to Linux
 * Used for cmd+rsp and normal dma requests
 * If error occurs, stop receiving response and return
 * Note: Host doesn't support Response R2 CRC error check
 */
static void spemmc_get_rsp_136(SPEMMCHOST *host)
{
	unsigned int val[2];
	struct mmc_command *cmd = host->mrq->cmd;
	unsigned char rspBuf[17] = {0}; /* Used to store 17 bytes(136 bits) or 6 bytes(48 bits) response */

	/*  read R2 response in 3 times, each time read 6 bytes */
	SP_EMMC_WAIT_RSP_BUFF_FULL(host);
		/*
		 * Store received response buffer data.
		 * Function runs to here only if no error occurs
		 */
	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[0] = (val[0] >> 16) & 0xff;  /* skip the first byte */
	rspBuf[1] = (val[0] >> 8) & 0xff;
	rspBuf[2] = (val[0] >> 0) & 0xff;
	rspBuf[3] = (val[1] >> 8) & 0xff;
	rspBuf[4] = (val[1] >> 0) & 0xff;

	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[5] = (val[0] >> 24) & 0xff;
	rspBuf[6] = (val[0] >> 16) & 0xff;
	rspBuf[7] = (val[0] >> 8) & 0xff;
	rspBuf[8] = (val[0] >> 0) & 0xff;
	rspBuf[9] = (val[1] >> 8) & 0xff;
	rspBuf[10] = (val[1] >> 0) & 0xff;

	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[11] = (val[0] >> 24) & 0xff;
	rspBuf[12] = (val[0] >> 16) & 0xff;
	rspBuf[13] = (val[0] >> 8) & 0xff;
	rspBuf[14] = (val[0] >> 0) & 0xff;
	rspBuf[15] = (val[1] >> 8) & 0xff;


	/*
	 * Wait till host controller becomes idle or error occurs
	 * The host may be busy sending 8 clk cycles for the end of a request
	 */
	while (1) {
		if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
			break;
		if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
			return;
	}

	/*
	 * Pass response back to Linux
	 * Function runs to here only if no error occurs
	 */
	cmd->resp[0] = SP_MMC_SWAP32(*((unsigned int *)(rspBuf)));
	cmd->resp[1] = SP_MMC_SWAP32(*((unsigned int *)(rspBuf + 4)));
	cmd->resp[2] = SP_MMC_SWAP32(*((unsigned int *)(rspBuf + 8)));
	cmd->resp[3] = SP_MMC_SWAP32(*((unsigned int *)(rspBuf + 12)));

	return;
}

/*
 * Receive 48 bits response, and pass it back to Linux
 * Used for cmd+rsp and normal dma requests
 * If error occurs, stop receiving response and return
 */
static void spemmc_get_rsp_48(SPEMMCHOST *host)
{
	struct mmc_command *cmd = host->mrq->cmd;
	unsigned char rspBuf[6] = {0}; /* Used to store 6 bytes(48 bits) response */

	/* Wait till host controller becomes idle or error occurs */
	while (1) {
		if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
			break;
		if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
			return;
	}
	/*
	 * Store received response buffer data
	 * Function runs to here only if no error occurs
	 */
	rspBuf[0] = host->base->sd_rspbuf0;
	rspBuf[1] = host->base->sd_rspbuf1;
	rspBuf[2] = host->base->sd_rspbuf2;
	rspBuf[3] = host->base->sd_rspbuf3;
	rspBuf[4] = host->base->sd_rspbuf4;
	rspBuf[5] = host->base->sd_rspbuf5;

	/* Pass response back to Linux */
	cmd->resp[0] = (rspBuf[1] << 24) | (rspBuf[2] << 16) | (rspBuf[3] << 8) | rspBuf[4];
	cmd->resp[1] = rspBuf[5] << 24;

	return;
}

/*
 * Retrieve response for cmd+rsp and normal dma request
 * This function makes sure host returns to it's sdstate_new idle or error state
 * Note: Error handling should be performed afterwards
 */
static void spemmc_get_rsp(SPEMMCHOST *host)
{
	struct mmc_command *cmd = host->mrq->cmd;

	if (cmd->flags & MMC_RSP_136)
		spemmc_get_rsp_136(host);
	else
		spemmc_get_rsp_48(host);

	return;
}

#if 0
ETIMEDOUT    Card took too long to respond
EILSEQ       Basic format problem with the received or sent data
(e.g. CRC check failed, incorrect opcode in response or bad end bit)
#endif

static void spemmc_check_sdstatus_errors(SPEMMCHOST *host)
{
	if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) {
		DPRINTK("cmd %d error with sdstate = %x, sdstatus = %x\n",
			host->mrq->cmd->opcode, host->base->sd_state, host->base->sdstatus);
		/* Response related errors */
		if (host->base->sdstatus & SP_SDSTATUS_WAIT_RSP_TIMEOUT)
			host->mrq->cmd->error = -ETIMEDOUT;
		if (host->base->sdstatus & SP_SDSTATUS_RSP_CRC7_ERROR)
			host->mrq->cmd->error = -EILSEQ;

		/* Only check the below error flags if data transaction is involved */
		if(host->mrq->data) {
			/* Data transaction related errors */
			if (host->base->sdstatus & SP_SDSTATUS_WAIT_STB_TIMEOUT)
				host->mrq->data->error = -ETIMEDOUT;
			if (host->base->sdstatus & SP_SDSTATUS_WAIT_CARD_CRC_CHECK_TIMEOUT)
				host->mrq->data->error = -ETIMEDOUT;

			if (host->base->sdstatus & SP_SDSTATUS_CRC_TOKEN_CHECK_ERROR)
				host->mrq->data->error = -EILSEQ;
			if (host->base->sdstatus & SP_SDSTATUS_RDATA_CRC16_ERROR)
				host->mrq->data->error = -EILSEQ;

			/* Reset PBus channel */
			Sd_Bus_Reset_Channel(host);
		}
	}

	return;
}

/*
 * Receive 48 bits response, and pass it back to kernel
 * Used for interrupt transactions (don't need to wait sdstate_new to become idle)
 */
static void spemmc_get_response_48(SPEMMCHOST *host)
{
	unsigned char rspBuf[6] = {0}; /* Used to store 6 bytes(48 bits) response */

	/* Store received response buffer data */
	rspBuf[0] = host->base->sd_rspbuf0;
	rspBuf[1] = host->base->sd_rspbuf1;
	rspBuf[2] = host->base->sd_rspbuf2;
	rspBuf[3] = host->base->sd_rspbuf3;
	rspBuf[4] = host->base->sd_rspbuf4;
	rspBuf[5] = host->base->sd_rspbuf5;

	/* Pass response back to kernel */
	host->mrq->cmd->resp[0] = (rspBuf[1] << 24) | (rspBuf[2] << 16) | (rspBuf[3] << 8) | rspBuf[4];
	host->mrq->cmd->resp[1] = rspBuf[5] << 24;

	return;
}


void dump_emmc_all_regs(SPEMMCHOST *host)
{
	volatile unsigned int *reg = (volatile unsigned int *)host->base;
	int i, j;
	for (i =  0; i < 5; i++){
		for (j =  0; j < 32; j++){
			printk("g%d.%d = 0x%08x\n", i, j, *reg);
			reg++;
		}
	}
}

static void spemmc_irq_normDMA(SPEMMCHOST *host)
{
	struct mmc_data *data = host->mrq->data;
	struct mmc_command *stop = host->mrq->stop;

	/* Get response */
	spemmc_get_response_48(host);
	/* Check error flags */
	spemmc_check_sdstatus_errors(host);

	if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE) {
		data->bytes_xfered = data->blocks * data->blksz;
	} else {
		EPRINTK("normal DMA error!\n");
		Reset_Controller(host);
		data->bytes_xfered = 0;
	}

	if (stop) {
		/* Configure Group SD Registers */
		host->base->sd_cmdbuf[3] = (u8)(stop->opcode | 0x40);	/* add start bit, according to spec, command format */
		host->base->sd_cmdbuf[2] = (u8)((stop->arg >> 24) & 0x000000ff);
		host->base->sd_cmdbuf[1] = (u8)((stop->arg >> 16) & 0x000000ff);
		host->base->sd_cmdbuf[0] = (u8)((stop->arg >>  8) & 0x000000ff);
		host->base->sd_cmdbuf[4] = (u8)((stop->arg >>  0) & 0x000000ff);
		host->base->sd_trans_mode = 0x0;
		host->base->sdautorsp = 1;
		host->base->sdcmddummy = 1;
		host->base->sdrspchk_en = 0x1;
		host->base->sdrsptype = 0x0;

		/* Configure SD INT reg (Disable them) */
		host->base->hwdmacmpen = 0;
		host->base->sdcmpen = 0x0;

		host->base->sdctrl0 = 1;   /* Start transaction */
		while (1) {
			if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
				break;
			if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
				break;
		}
	}

	host->base->sd_cmp_clr = 1;

	return;
}

static void spemmc_irq_cmd_rsp(SPEMMCHOST *host)
{
	/* Get response */
	spemmc_get_response_48(host);
	/* Check error flags */
	spemmc_check_sdstatus_errors(host);

	host->base->sd_cmp_clr = 1;

	return;
}

/* Interrupt Handler */
irqreturn_t spemmc_irq(int irq, void *dev_id)
{
	struct mmc_host *mmc = dev_id;

	SPEMMCHOST *host = (SPEMMCHOST *)mmc_priv(mmc);
	/*ignore unexpected irq */
	if(!host->base->sd_cmp &&
			!host->base->hw_dma_en)
	{
		printk("!!!!!spemmc_irq:unknow int\n");
		return IRQ_HANDLED;
	}

	if (host->mrq->data != NULL) {
		spemmc_irq_normDMA(host);
	} else { /* Cmd + Rsp(48 bits) IRQ */
		spemmc_irq_cmd_rsp(host);
	}
	/* disbale interrupt to workaround unexpected irq*/
	host->base->hwdmacmpen = 0;
	host->base->sdcmpen = 0;
	sphe_mmc_finish_request(host, host->mrq);

	return IRQ_HANDLED;
}

/*
 * 1. Releases semaphore for mmc_request
 * 2. Notifies kernel that mmc_request is done
 */
static void sphe_mmc_finish_request(SPEMMCHOST *host, struct mmc_request *mrq)
{
	if (mrq->data) {
		/*
		 * The original sg_len may differ after dma_map_sg function callback.
		 * When executing dma_unmap_sg, the memory segment count value returned
		 * by dma_map_sg should be used instead (value is stored in host->dma_sgcount)
		 */
		dma_unmap_sg(mmc_dev(host->mmc), mrq->data->sg,
				host->dma_sgcount,
				spemmc_get_dma_dir(host, mrq->data));

#ifdef SP_MMC_SUPPORT_DDR_MODE
		if ((mrq->data->flags & MMC_DATA_WRITE) && host->base->sdddrmode) {
			host->base->sd_rd_dly_sel =host->rddly;
			host->base->sd_wr_dly_sel =host->wrdly;
		}
#endif
		if(mrq->data->error && -EINVAL != mrq->data->error) {
			#if (ENABLE_TIMING_TUNING == 1)
			/* tune next data request timing */
			host->need_tune_dat_timing = 1;
			#endif
			DPRINTK("data err(%d)\n", mrq->data->error);
		}
	}

	if(mrq->cmd->error) {
		#if (ENABLE_TIMING_TUNING == 1)
		/* tune next cmd request timing */
		host->need_tune_cmd_timing = 1;
		#endif
		DPRINTK("cmd err(%d)\n",mrq->cmd->error);
	}

	host->mrq = NULL;

	up(&host->req_sem);
	mmc_request_done(host->mmc, mrq);
}

static int spemmc_get_dma_dir(SPEMMCHOST *host, struct mmc_data *data)
{
	if (data->flags & MMC_DATA_WRITE)
		return DMA_TO_DEVICE;
	else
		return DMA_FROM_DEVICE;
}

/* Get timeout_ns from kernel, and set it to HW DMA's register */
static inline void spemmc_set_data_timeout(SPEMMCHOST *host)
{
	unsigned int timeout_clks, cycle_ns;

	cycle_ns = (1000000000 * (1 + host->base->sdfqsel))  / emmc_get_in_clock(host);
	timeout_clks = host->mrq->data->timeout_ns / cycle_ns;
	timeout_clks +=  host->mrq->data->timeout_clks;

	/*  kernel set max read timeout to for SDHC 100ms, */
	/*  mult 10 to  Improve compatibility for some unstandard card */
	if (host->mrq->data->flags & MMC_DATA_READ) {
		timeout_clks *= READ_TIMEOUT_MULT;
	}
	else {
		timeout_clks *= EMMC_WRITE_TIMEOUT_MULT;
	}
	host->base->sd_rxdattmr = timeout_clks;
}

/*
 * DMA transfer mode, used for all other data transfer commands besides read/write block commands (cmd17, 18, 24, 25)
 * Due to host limitations, this kind of DMA transfer mode only supports 1 consecutive memory area
 */
static void spemmc_proc_normDMA(SPEMMCHOST *host)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_data *data = mrq->data;
	spemmc_prepare_cmd_rsp(host);
	spemmc_set_data_info(host, data);

	if (!data->error) {
		/* Configure SD INT reg */
		/* Disable HW DMA data transfer complete interrupt (when using sdcmpen) */
		host->base->hwdmacmpen = 0;
		host->base->sdcmpen = 0x1;
		/* Start Transaction */
		host->base->sdctrl0 = 1;
	}
	return;
}

/* Send data in @buf out with pio.
  * @host: the host
  * @buf: data buf
  * @len data len
  *
  * return: the count of transfered data, or -1 if error occured.
  */
static int spemmc_pio_tx(SPEMMCHOST *host, char *buf, int len)
{
	int count = 0;
	while (len > 0) {
		host->base->sd_piodatatx = *(u32 *)buf;
		buf += 4;
		len -= 4;
		count += 4;
		while (1) {
			if (host->base->sdstatus & SP_SDSTATUS_TX_DATA_BUF_EMPTY)
				break;
			if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
				return count;
			if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) {
				spemmc_check_sdstatus_errors(host);
				return -1;
			}
		}
	}
	return count;
}

/* Receive data to @buf with pio.
  * @host: the host
  * @buf: data buf
  * @len data len
  *
  * return: the count of transfered data, or -1 if error occured.
  */
static int spemmc_pio_rx(SPEMMCHOST *host, char *buf, int len)
{
	int count = 0;
	while (len > 0) {
		while (1) {
			if (host->base->sdstatus & SP_SDSTATUS_RX_DATA_BUF_FULL)
				break;
			if (host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
				return count;
			if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) {
				spemmc_check_sdstatus_errors(host);
				return -1;
			}
		}
		*(u32 *)buf = host->base->sd_piodatarx;
		buf += 4;
		len -= 4;
		count += 4;
	}
	return count;

}

static void spemmc_proc_pio(SPEMMCHOST *host)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_data *data = mrq->data;
	struct scatterlist *sg;
	int nents = sg_nents(data->sg);
	int write = data->flags & MMC_DATA_WRITE;
	int i;

	spemmc_prepare_cmd_rsp(host);
	EMMC_PAGE_NUM_SET(host->base, data->blocks);
	if (!write) {
		host->base->sdcmddummy = 0;
		host->base->sdautorsp = 0;
		host->base->sd_trans_mode = 2;
	} else {
		host->base->sdcmddummy = 1;
		host->base->sdautorsp = 1;
		host->base->sd_trans_mode = 1;
	}
	if (host->mrq->stop)
		host->base->sd_len_mode = 0;
	else
		host->base->sd_len_mode = 1;

	host->base->sdpiomode = 1;
	SDDATALEN_SET(host->base, data->blksz);

	/* Start Transaction */
	host->base->sdctrl0 = 1;

	for_each_sg(data->sg, sg, nents, i) {
		if (write) {
			if (-1 == spemmc_pio_tx(host, (char *)sg_virt(sg), sg->length))
				return;
		} else {
			if (-1 == spemmc_pio_rx(host, (char *)sg_virt(sg), sg->length))
				return;
		}
	}
	spemmc_get_response_48(host);
	data->bytes_xfered = data->blocks * data->blksz;
}

static void spemmc_set_cmd(SPEMMCHOST *host, struct mmc_request *mrq)
{
	host->base->sd_cmdbuf[3] = (u8)(mrq->cmd->opcode | 0x40);	/* add start bit, according to spec, command format */
	host->base->sd_cmdbuf[2] = (u8)((mrq->cmd->arg >> 24) & 0x000000ff);
	host->base->sd_cmdbuf[1] = (u8)((mrq->cmd->arg >> 16) & 0x000000ff);
	host->base->sd_cmdbuf[0] = (u8)((mrq->cmd->arg >>  8) & 0x000000ff);
	host->base->sd_cmdbuf[4] = (u8)((mrq->cmd->arg >>  0) & 0x000000ff);
}

/* Prepare Command + Response commands (with no data), polling mode */
static void spemmc_prepare_cmd_rsp(SPEMMCHOST *host)
{
	struct mmc_request *mrq = host->mrq;

	/* Reset */
	Reset_Controller(host);

	/* Configure Group SD Registers */
	spemmc_set_cmd(host, mrq);
	host->base->sd_trans_mode = 0x0;
	host->base->sdautorsp = 1;
	host->base->sdcmddummy = 1;

	/*
	 * Currently, host is not capable of checking Response R2's CRC7
	 * Because of this, enable response crc7 check only for 48 bit response commands
	 */
	if (mrq->cmd->flags & MMC_RSP_CRC && !(mrq->cmd->flags & MMC_RSP_136))
		host->base->sdrspchk_en = 0x1;
	else
		host->base->sdrspchk_en = 0x0;

	if (mrq->cmd->flags & MMC_RSP_136)
		host->base->sdrsptype = 0x1;
	else
		host->base->sdrsptype = 0x0;

	/* Configure SD INT reg (Disable them) */
	host->base->hwdmacmpen = 0;
	host->base->sdcmpen = 0x0;

	return;
}

static void spemmc_set_data_info(SPEMMCHOST *host, struct mmc_data * data)
{
	int i, count = dma_map_sg(mmc_dev(host->mmc), data->sg,
							  data->sg_len,
							  spemmc_get_dma_dir(host, data));
	struct scatterlist *sg;
	unsigned int hw_address[SP_HW_DMA_MEMORY_SECTORS] = {0}, hw_len[SP_HW_DMA_MEMORY_SECTORS] = {0};

	/* Store the dma_mapped memory segment count, it will be used when calling dma_unmap_sg */
	host->dma_sgcount = count;

	/* retreive physical memory address & size of the fragmented memory blocks */
	for_each_sg(data->sg, sg, count, i) {
		hw_address[i] = sg_dma_address(sg);
		hw_len[i] = sg_dma_len(sg);
		if(unlikely(!IS_DMA_ADDR_2BYTE_ALIGNED(hw_address[i]))) {
			EPRINTK("dma addr is not 2 bytes aligned!\n");
			data->error = -EINVAL;
			return;
		}
	}

	/* Due to host limitations, normal DMA transfer mode only supports 1 consecutive physical memory area */
	if (count == 1) {
		DPRINTK("page num = %d\n", data->blocks);
		EMMC_PAGE_NUM_SET(host->base, data->blocks);
		if (data->flags & MMC_DATA_READ) {
			host->base->sdcmddummy = 0;
			host->base->sdautorsp = 0;
			host->base->sd_trans_mode = 2;
		} else {
			host->base->sdcmddummy = 1;
			host->base->sdautorsp = 1;
			host->base->sd_trans_mode = 1;
		}
		host->base->sd_len_mode = 1;
		#if 0
		if (host->mrq->stop)
			host->base->sd_len_mode = 0;
		else
			host->base->sd_len_mode = 1;
		#endif

		host->base->sdpiomode = 0;
		host->base->hw_dma_en = 0;
		SDDATALEN_SET(host->base, data->blksz);

		/* Configure Group DMA Registers */
		if (data->flags & MMC_DATA_WRITE) {
			host->base->dmadst = 0x2;
			host->base->dmasrc = 0x1;
		} else {
			host->base->dmadst = 0x1;
			host->base->dmasrc = 0x2;
		}
		SET_DMA_BASE_ADDR(host->base, hw_address[0]);

#ifdef SP_MMC_SUPPORT_DDR_MODE
		if ((data->flags & MMC_DATA_WRITE) && host->base->sdddrmode) {
			host->wrdly = host->base->sd_wr_dly_sel;
			host->rddly = host->base->sd_rd_dly_sel;
			host->base->sd_rd_dly_sel = host->ddr_rd_crc_token_dly;
			host->base->sd_wr_dly_sel = host->ddr_wr_data_dly;
		}
#endif
	} else {
		/* Should be implemented to fallback and use PIO transfer mode in the future */
		EPRINTK("SD Card DMA memory segment > 1, not supported!\n");
		data->error = -EINVAL;
	}
	return;
}

/* Process Command + Response commands (with no data) , interrupt mode */
static void spemmc_mmc_proc_cmd_rsp_intr(SPEMMCHOST *host)
{
	struct mmc_request *mrq = host->mrq;

	DPRINTK("Process Command & Response (No Data)\n");
	/* Reset */
	Reset_Controller(host);

	/* Configure Group SD Registers */
	spemmc_set_cmd(host, mrq);

	host->base->sd_trans_mode = 0x0;
	host->base->sdautorsp = 1;
	host->base->sdcmddummy = 1;

	/* Currently, host is not capable of checking Response R2's CRC7
	   Because of this, enable response crc7 check only for 48 bit response commands
	 */
	if (mrq->cmd->flags & MMC_RSP_CRC && !(mrq->cmd->flags & MMC_RSP_136))
		host->base->sdrspchk_en = 0x1;
	else
		host->base->sdrspchk_en = 0x0;

	if (mrq->cmd->flags & MMC_RSP_136)
		host->base->sdrsptype = 0x1;
	else
		host->base->sdrsptype = 0x0;

	/* Configure SD INT reg */
	/* Disable HW DMA data transfer complete interrupt (when using sdcmpen) */
	host->base->hwdmacmpen = 0;
	host->base->sdcmpen = 0x1;

	/* Start Transaction */
	host->base->sdctrl0 = 1;

	return;
}

/* Process Command + No Response commands (with no data) */
static void spemmc_mmc_proc_cmd(SPEMMCHOST *host)
{
	struct mmc_request *mrq = host->mrq;

	/* Reset */
	Reset_Controller(host);

	/* Configure Group SD Registers */
	spemmc_set_cmd(host, mrq);

	host->base->sd_trans_mode = 0x0;
	host->base->sdautorsp = 0;
	host->base->sdcmddummy = 1;

	/* Configure SD INT reg */
	/* Disable HW DMA data transfer complete interrupt (when using sdcmpen) */
	host->base->hwdmacmpen = 0;
	host->base->sdcmpen = 0x0;

	/* Start Transaction */
	host->base->sdctrl0 = 1;

	while((host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE) != 0x40) {
		/* printk("Waiting! sd_hw_state : 0x%x   LMST_SM:0x%x   Data In Counter :%u Data Out Counter: %u\n", host->base->hwsd_sm, host->base->lmst_sm, host->base->incnt, host->base->outcnt); */
		/* printk("sd status:0x%x, state:0x%x, state new:0x%x\n", host->base->sdstatus, host->base->sdstate, host->base->sdstate_new); */
		/* printk("Waiting\n"); */
	}

	sphe_mmc_finish_request(host, host->mrq);
}

/* For mmc_requests
 * ================ Data transfer requests ===========================
 * 1. Data transfer requests : use interrupt mode
 * ================= Non data transfer requests =======================
 * 1. Command + Response (48 bit response) requests : use interrupt mode
 * 2. Command + Response (136 bit response) requests : use polling mode
 * 3. Command (no response) requests : use polling mode
 */
void spemmc_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	SPEMMCHOST *host = mmc_priv(mmc);
	int retry_count = (host->need_tune_cmd_timing || host->need_tune_dat_timing) ? MAX_DLY_CLK : 0;

	DPRINTK("\n<-- cmd:%d, arg:0x%08x, data len:%d, stop:%s\n",
		mrq->cmd->opcode, mrq->cmd->arg,
		mrq->data ? (mrq->data->blocks * mrq->data->blksz) : 0,
		mrq->stop ? "true" : "false");

	/*
	 * The below semaphore is released when "sphe_mmc_finish_request" is called
	 * TODO: Observe if the below semaphore is necessary
	 */
	down(&host->req_sem);
	host->mrq = mrq;

	if (mrq->data == NULL) {
		if (host->mrq->cmd->flags & MMC_RSP_PRESENT) {
			if (unlikely(host->need_tune_cmd_timing || host->mrq->cmd->flags & MMC_RSP_136)) {
				do {
					spemmc_prepare_cmd_rsp(host);
					spemmc_trigger_sdstate(host);
					spemmc_get_rsp(host); /* Makes sure host returns to an idle or error state */
					spemmc_check_sdstatus_errors(host);
					if (-EILSEQ == mrq->cmd->error) {
						host->base->sd_rd_dly_sel++;
					} else if (-ETIMEDOUT == mrq->cmd->error) {
						host->base->sd_wr_dly_sel++;
					} else {
						break;
					}
				} while(retry_count--);
				if (!mrq->cmd->error)
					host->need_tune_cmd_timing = 0;
				sphe_mmc_finish_request(host, host->mrq);
			} else {
				spemmc_mmc_proc_cmd_rsp_intr(host);
			}
		} else {
			spemmc_mmc_proc_cmd(host);
		}
	} else {
		if (unlikely(host->need_tune_dat_timing)) {
			do {
				spemmc_prepare_cmd_rsp(host);
				spemmc_set_data_info(host, mrq->data);
				if(-EINVAL == mrq->data->error) {
					break;
				}
				spemmc_trigger_sdstate(host);
				spemmc_get_rsp(host); /* Makes sure host returns to an idle or error state */
				spemmc_check_sdstatus_errors(host);
				if (-EILSEQ == mrq->data->error) {
					if (mrq->data->flags & MMC_DATA_WRITE) {
						if (is_crc_token_valid(host))
							host->base->sd_wr_dly_sel++;
						else
							host->base->sd_rd_dly_sel++;
					} else {
						host->base->sd_rd_dly_sel++;
					}
				} else if (-ETIMEDOUT == mrq->data->error) {
					host->base->sd_wr_dly_sel++;
				} else {
					break;
				}
				#ifdef SP_MMC_SUPPORT_DDR_MODE
				if ((mrq->data->flags & MMC_DATA_WRITE) && host->base->sdddrmode) {
					host->ddr_rd_crc_token_dly = host->base->sd_rd_dly_sel;
					host->ddr_wr_data_dly = host->base->sd_wr_dly_sel;
				}
				#endif
			} while(retry_count--);
			if (!mrq->data->error)
				host->need_tune_dat_timing = 0;
			sphe_mmc_finish_request(host, host->mrq);
		} else if (likely(dma_mode)){
			spemmc_proc_normDMA(host);
			if(-EINVAL == host->mrq->data->error) { /*  para is not correct return */
				sphe_mmc_finish_request(host, host->mrq);
			}
		} else {
			spemmc_proc_pio(host);
			host->mrq = NULL;
			up(&host->req_sem);
			mmc_request_done(host->mmc, mrq);
		}
	}
}

/* set_ios -
 * 1) Set/Disable clock
 * 2) Power on/off to offer SD card or not
 * 3) Set SD Card Bus width to 1 or 4
 */
void spemmc_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	SPEMMCHOST *host = (SPEMMCHOST *)mmc_priv(mmc);

	IFPRINTK("\n<----- sd%d: clk:%d, buswidth:%d(2^n), bus_mode:%d, powermode:%d(0:off, 1:up, 2:on), timing:%d\n",
		host->id, ios->clock, ios->bus_width, ios->bus_mode, ios->power_mode, ios->timing);

	down(&host->req_sem);

	/* TODO: Cleanup power_mode functions */
	switch (ios->power_mode) {
	/* power off->up->on */
	case MMC_POWER_ON:
		DPRINTK("set MMC_POWER_ON\n");
		spemmc_controller_init(host);
		break;
	case MMC_POWER_UP:
		/* Set default control register */
		DPRINTK("set MMC_POWER_UP\n");
		Reset_Controller(host);
		break;
	case MMC_POWER_OFF:
		Reset_Controller(host);
		DPRINTK("set MMC_POWER_OFF\n");
		break;
	}

	spemmc_set_ac_timing(host, ios);
	spemmc_set_bus_width(host, ios->bus_width);
	host->need_tune_cmd_timing = 0;
	host->need_tune_dat_timing = 0;

	up(&host->req_sem);
	IFPRINTK("----- \n\n");

	return;
}

int spemmc_mmc_read_only(struct mmc_host *mmc)
{
	/* return  > 0 :support read only */
	/*         < 0 :not support RO */
	/*         = 0 :no action */
	return -ENOSYS;
}


/*
 * Return values for the get_cd callback should be:
 *   0 for a absent card
 *   1 for a present card
 *   -ENOSYS when not supported (equal to NULL callback)
 *   or a negative errno value when something bad happened
 */
int spemmc_mmc_card_detect(struct mmc_host *mmc)
{
	// SPEMMCHOST *host = mmc_priv(mmc);

	return 1;
}

static void spemmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	SPEMMCHOST *host = mmc_priv(mmc);
	if (enable) {
		host->base->sdio_int_en = 1;
	} else {
		host->base->sdio_int_en = 0;
	}
}

const struct mmc_host_ops spemmc_sdc_ops = {
	.request = spemmc_mmc_request,
	.set_ios = spemmc_mmc_set_ios,
	.get_ro = spemmc_mmc_read_only,
	.get_cd = spemmc_mmc_card_detect,
	.enable_sdio_irq = spemmc_enable_sdio_irq,
};


static uint get_max_sd_freq(SPEMMCHOST *host)
{
	uint max_freq = SPEMMC_MAX_CLOCK;

	/* fix me read from device tree */
	max_freq = SPEMMC_MAX_CLOCK;

	printk("[eMMC] Slot %d, actually use max_freq #%d M\n", host->id, max_freq/CLOCK_1M);
	return max_freq;
}

int spemmc_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mmc_host *mmc;
	struct resource *resource;
	SPEMMCHOST *host;
	spemmc_dridata_t *priv = NULL;

	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(spemmc_of_id, pdev->dev.of_node);
		if (match)
			priv = (spemmc_dridata_t *)match->data;
	}

	mmc = mmc_alloc_host(sizeof(SPEMMCHOST), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_free_host;
	}

	host = (SPEMMCHOST *)mmc_priv(mmc);
	host->mmc = mmc;
	host->pdev = pdev;

	if (priv)
		host->id  = priv->id;
	printk("sd slot id:%d\n", host->id);

	host->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(host->clk)) {
		EPRINTK("Can not find clock source!\n");
		ret = PTR_ERR(host->clk);
		goto probe_free_host;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(resource)) {
		EPRINTK("get sd %d register resource fail\n", host->id);
		ret = PTR_ERR(resource);
		goto probe_free_host;
	}

	if ((resource->end - resource->start + 1) < sizeof(EMMCREG)) {
		EPRINTK("register size not right e:%d:r:%d\n",
			resource->end - resource->start + 1, sizeof(EMMCREG));
		ret = -EINVAL;
		goto probe_free_host;
	}

	host->base = devm_ioremap_resource(&pdev->dev, resource);
	if (IS_ERR((void *)host->base)) {
		EPRINTK("devm_ioremap_resource fail\n");
		ret = PTR_ERR((void *)host->base);
		goto probe_free_host;
	}

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq <= 0) {
		EPRINTK("get sd %d irq resource fail\n", host->id);
		ret = -EINVAL;
		goto probe_free_host;
	}
	if (request_irq(host->irq, spemmc_irq, IRQF_SHARED, dev_name(&pdev->dev), mmc)) {
		EPRINTK("Failed to request sd card interrupt.\n");
		ret = -ENOENT;
		goto probe_free_host;
	}
	DPRINTK("SD card driver probe, sd %d, base:0x%x, reg size:%d, irq: %d\n",
		host->id, resource->start, resource->end - resource->start, host->irq);

	ret = clk_prepare(host->clk);
	if (ret)
		goto probe_free_host;
	ret = clk_enable(host->clk);
	if (ret)
		goto probe_clk_unprepare;

	host->wrdly = host->base->sd_wr_dly_sel;
	host->rddly = host->base->sd_rd_dly_sel;

	sema_init(&host->req_sem, 1);

	mmc->ops = &spemmc_sdc_ops;

	/*
	 * freq_divisor[11:10] = sdfreq[1:0]
	 * freq_divisor[9:0] = sdfqsel[9:0]
	 */
	mmc->f_min = emmc_get_in_clock(host) / (0x0FFF + 1);
	mmc->f_max = get_max_sd_freq(host);

	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	mmc->max_seg_size = 65536 * 512;            /* Size per segment is limited via host controller's
	                                               ((sdram_sector_#_size max value) * 512) */
	/* Host controller supports up to "SP_HW_DMA_MEMORY_SECTORS", a.k.a. max scattered memory segments per request */
	mmc->max_segs = 1;
	mmc->max_req_size = 65536 * 512;			/* Decided by hw_page_num0 * SDHC's blk size */
	mmc->max_blk_size = 512;                   /* Limited by host's dma_size & data_length max value, set it to 512 bytes for now */
	mmc->max_blk_count = 65536;                 /* Limited by sdram_sector_#_size max value */
	mmc->caps =  MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE;
	mmc->caps |= MMC_CAP_3_3V_DDR;
	mmc->caps2 = MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD;

	dev_set_drvdata(&pdev->dev, mmc);
	/* FIXME: change to  make use of reset_control_assert/deassert
	  * when some day reset driver is ready.
	  */
	if (reset_controller(host))
		printk(KERN_WARNING "Reset card controller failed!\n");

	mmc_add_host(mmc);

	return 0;

probe_clk_unprepare:
	EPRINTK("unable to enable controller clock\n");
	clk_unprepare(host->clk);
probe_free_host:
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

int spemmc_drv_remove(struct platform_device *dev)
{
	struct mmc_host *mmc;
	SPEMMCHOST *host;

	DPRINTK("Remove sd card\n");
	mmc = platform_get_drvdata(dev);
	if (!mmc)
		return -EINVAL;

	host = (SPEMMCHOST *)mmc_priv(mmc);
	mmc_remove_host(mmc);
	free_irq(host->irq, mmc);
	clk_disable(host->clk);
	clk_unprepare(host->clk);
	platform_set_drvdata(dev, NULL);
	mmc_free_host(mmc);

	return 0;
}


int spemmc_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	struct mmc_host *mmc;
	SPEMMCHOST *host;

	mmc = platform_get_drvdata(dev);
	if (!mmc)
		return -EINVAL;

	host = (SPEMMCHOST *)mmc_priv(mmc);

	printk("Sunplus SD %d driver suspend.\n", host->id);
	down(&host->req_sem);
	up(&host->req_sem);

	return 0;
}

int spemmc_drv_resume(struct platform_device *dev)
{

#ifdef CONFIG_PM
	struct mmc_host *mmc;
	SPEMMCHOST *host;

	mmc = platform_get_drvdata(dev);
	if (!mmc)
		return -EINVAL;

	host = (SPEMMCHOST *)mmc_priv(mmc);


	printk("Sunplus SD%d driver resume.\n", host->id);
#endif

	return 0;
}
#ifdef CONFIG_PM

int spemmc_pm_suspend(struct device *dev)
{
	struct mmc_host *mmc;
	SPEMMCHOST *host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	if (!mmc)
		return -EINVAL;

	host = (SPEMMCHOST *)mmc_priv(mmc);

	printk("Sunplus SD %d driver suspend.\n", host->id);

	down(&host->req_sem);
	up(&host->req_sem);

	return 0;
}

int spemmc_pm_resume(struct device *dev)
{
	struct mmc_host *mmc;
	SPEMMCHOST *host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	if (!mmc)
		return -EINVAL;

	host = (SPEMMCHOST *)mmc_priv(mmc);

	printk("Sunplus SD%d driver resume.\n", host->id);
	return 0;
}

const struct dev_pm_ops sphe_emmc_pm_ops = {
	.suspend	= spemmc_pm_suspend,
	.resume		= spemmc_pm_resume,
};
#endif



struct platform_driver spemmc_driver_sdc = {
	.probe = spemmc_drv_probe,
	.remove = spemmc_drv_remove,
	.shutdown = NULL,
	.suspend = spemmc_drv_suspend,
	.resume = spemmc_drv_resume,
	.driver = {
		.name = SPEMMCV2_SDC_NAME,
		.owner = THIS_MODULE,
		.of_match_table = spemmc_of_id,
#ifdef CONFIG_PM
		.pm= &sphe_emmc_pm_ops,
#endif
	}
};

module_platform_driver(spemmc_driver_sdc);

MODULE_AUTHOR("SPHE B1");
MODULE_DESCRIPTION("SPHE MMC/SD Card Interface Driver");
MODULE_LICENSE("GPL v2");
