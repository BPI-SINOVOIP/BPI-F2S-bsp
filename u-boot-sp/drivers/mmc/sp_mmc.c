/*
 * This is a driver for the SDHC controller found in Sunplus Gemini SoC
 *
 * Enable CONFIG_MMC_TRACE in uboot configs to trace SD card requests
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <dm.h>
#include <malloc.h>
#include "sp_mmc.h"

#define MAX_SDDEVICES   2

#define SPMMC_CLK_SRC CLOCK_270M    /* Host controller's clk source */
#define SPMMC_ZEBU_CLK_SRC CLOCK_202M    /* Host controller's clk source */

#define SPMMC_MAX_CLK CLOCK_25M     /* Max supported SD Card frequency */
#define SPEMMC_MAX_CLK CLOCK_45M     /* Max supported emmc Card frequency */

#define MAX_DLY_CLK  7


/*
 * Freq. for identification mode(SD Spec): 100~400kHz
 * U-Boot uses the host's minimal declared freq. directly
 * Set it to ~150kHz for now
 */
#define SPMMC_MIN_CLK (((SPMMC_CLK_SRC / (0xFFF + 1)) > CLOCK_150K) \
		? (SPMMC_CLK_SRC / (0xFFF + 1)) \
		: CLOCK_150K)
#define SPEMMC_MIN_CLK  CLOCK_200K

#define SP_MMC_MAX_RSP_LEN 16
#define SP_MMC_SWAP32(x)		((((x) & 0x000000ff) << 24) | \
				 (((x) & 0x0000ff00) <<  8) | \
				 (((x) & 0x00ff0000) >>  8) | \
				 (((x) & 0xff000000) >> 24)   \
				)



// #define sp_sd_trace()	printf(KERN_ERR "[SD TRACe]: %s:%d\n", __FILE__, __LINE__)
#define sp_sd_trace()	do {}  while (0)

/* Disabled fatal error messages temporarily */
static u32 loglevel = 0x003;
/* static u32 loglevel = 0x001; */
/* static u32 loglevel = 0x033; */
/* static u32 loglevel = 0xefff; */
/* static u32 loglevel = 0xffff; */

#define MMC_LOGLEVEL_FATAL		0x01
#define MMC_LOGLEVEL_ERROR		0x02
#define MMC_LOGLEVEL_DEBUG		0x04


#define MMC_LOGLEVEL_IF 		0x10
#define MMC_LOGLEVEL_PK 		0x20

#define MMC_LOGLEVEL_COUNTER	0x100
#define MMC_LOGLEVEL_WAITTIME	0x200

#define MMC_LOGLEVEL_DUMPREG	0x1000
#define MMC_LOGLEVEL_MINI		0x2000


#define FATAL(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_FATAL)) \
	printf(KERN_ERR "[SD FATAL]: %s: " fmt, __func__ , ## args)

#define EPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_ERROR)) \
	printf("[SD ERROR]: %s: " fmt, __func__ , ## args)

#define DPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_DEBUG)) \
	printf("[SD DBG]: %s: " fmt, __func__ , ## args)

#define IFPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_IF)) \
	printf("[SD IF]: %s:" fmt, __func__, ## args)

#define pk(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_PK)) \
	printf("[SD PK]: %s: " fmt, __func__ , ## args)

#define CPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_COUNTER)) \
	printf("[SD COUNTER]: %s: " fmt, __func__ , ## args)

#define WPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_WAITTIME)) \
	printf("[SD WAITTIME]: %s: " fmt, __func__ , ## args)

#define REGPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_DUMPREG)) \
	printf("[SD REG]: %s: " fmt, __func__ , ## args)

#define MPRINTK(fmt, args...) if(unlikely(loglevel & MMC_LOGLEVEL_MINI)) \
	printf("[SD]: %s: " fmt, __func__ , ## args)


/* Function declaration */
static int Sd_Bus_Reset_Channel(struct sp_mmc_host *host);
static int Reset_Controller(struct sp_mmc_host *host);
static void sp_mmc_prep_cmd_rsp(struct sp_mmc_host *host, struct mmc_cmd *cmd);
static void sp_mmc_prep_data_info(struct sp_mmc_host *host, struct mmc_cmd *cmd, struct mmc_data *data);
static void sp_mmc_wait_sdstate_new(struct sp_mmc_host *host);
static int sp_mmc_read_data_pio(sp_mmc_host *host,  struct mmc_data *data);

#if defined(CONFIG_SP_PNG_DECODER)
int sp_png_dec_run(void);
#endif

#ifdef SP_EMMC_DEBUG
void dump_emmc_all_regs(sp_mmc_host *host)
{
	volatile unsigned int *reg = (volatile unsigned int *)host->ebase;
	int i, j;
	printf("### dump emmc controller registers start ###");
	for (i =  0; i < 3; i++){
		for (j =  0; j < 32; j++){
			printf("g%d.%d = 0x%08x\n", i, j, *reg);
			reg++;
		}
	}
	printf("### dump emmc controller registers end ###");
}
#endif

static void sp_mmc_dcache_flush_invalidate(struct mmc_data *data, unsigned int phys_start, unsigned int data_len)
{
	unsigned int phys_end = phys_start + data_len;

	phys_end = roundup(phys_end, ARCH_DMA_MINALIGN);
	/*
	 * MMC_DATA_WRITE: memory -> SD Card, flush data to memory
	 * MMC_DATA_READ: SD Card -> memory, invalidate dcache contents
	 */
	if (data->flags & MMC_DATA_WRITE) {
		flush_dcache_range((ulong)phys_start, (ulong)phys_end);
	}
	else
#ifdef CONFIG_SP_NAND_READ_HELP_FLUSH_BUF
	{
		/* avoid data loss in unaligned buffer */
		if ((phys_start & ARCH_DMA_MINALIGN) || ((phys_start + data_len) & ARCH_DMA_MINALIGN))
			flush_dcache_range((ulong)phys_start, (ulong)phys_end);

		invalidate_dcache_range((ulong)phys_start, (ulong)phys_end);
	}
#else
	invalidate_dcache_range((ulong)phys_start, (ulong)phys_end);
#endif

	return;
}




/*
 * Receive 48 bits response, and pass it back to U-Boot
 * Used for cmd+rsp and normal dma requests
 * If error occurs, stop receiving response and return
 */
static void sp_mmc_get_rsp_48(struct sp_mmc_host *host, struct mmc_cmd *cmd)
{
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
	rspBuf[0] = host->base->sdrspbuf0;
	rspBuf[1] = host->base->sdrspbuf1;
	rspBuf[2] = host->base->sdrspbuf2;
	rspBuf[3] = host->base->sdrspbuf3;
	rspBuf[4] = host->base->sdrspbuf4;
	rspBuf[5] = host->base->sdrspbuf5;

	/* Pass response back to U-Boot */
	cmd->response[0] = (rspBuf[1] << 24) | (rspBuf[2] << 16) | (rspBuf[3] << 8) | rspBuf[4];
	cmd->response[1] = rspBuf[5] << 24;

	return;
}


#define SD_WAIT_REPONSE_BUFFER_FULL(host) \
do { \
	while (1) { \
		if (host->base->sdstatus & SP_SDSTATUS_RSP_BUF_FULL) \
			break;	 \
		if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) \
			return;  \
	} \
} while (0)

/*
 * Receive 136 bits response, and pass it back to U-Boot
 * Used for cmd+rsp and normal dma requests
 * If error occurs, stop receiving response and return
 * Note: Host doesn't support Response R2 CRC error check
 */
static void sp_mmc_get_rsp_136(struct sp_mmc_host *host, struct mmc_cmd *cmd)
{
	unchar *rspBuf = (unchar *)cmd->response;
	uint val[2];
	uint i;
	SD_WAIT_REPONSE_BUFFER_FULL(host);

	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[0] = (val[0] >> 16) & 0xff;
	rspBuf[1] = (val[0] >> 8) & 0xff;
	rspBuf[2] = (val[0] >> 0) & 0xff;
	rspBuf[3] = (val[1] >> 8) & 0xff;
	rspBuf[4] = (val[1] >> 0) & 0xff;

	SD_WAIT_REPONSE_BUFFER_FULL(host);
	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[5] = (val[0] >> 24) & 0xff;
	rspBuf[6] = (val[0] >> 16) & 0xff;
	rspBuf[7] = (val[0] >> 8) & 0xff;
	rspBuf[8] = (val[0] >> 0) & 0xff;
	rspBuf[9] = (val[1] >> 8) & 0xff;
	rspBuf[10] = (val[1] >> 0) & 0xff;

	SD_WAIT_REPONSE_BUFFER_FULL(host);
	val[0] = host->base->sd_rspbuf[0];
	val[1] = host->base->sd_rspbuf[1];
	rspBuf[11] = (val[0] >> 24) & 0xff;
	rspBuf[12] = (val[0] >> 16) & 0xff;
	rspBuf[13] = (val[0] >> 8) & 0xff;
	rspBuf[14] = (val[0] >> 0) & 0xff;
	rspBuf[15] = (val[1] >> 8) & 0xff;

	for (i = 0; i < SP_MMC_MAX_RSP_LEN/sizeof(cmd->response[0]); i++ ) {
		cmd->response[i] = SP_MMC_SWAP32(cmd->response[i]);
	}

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
}

/*
 * Retrieve response for cmd+rsp and normal dma requests
 * This function makes sure host returns to it's sdstate_new idle or error state
 * Note: Error handling should be performed afterwards
 */
static void sp_mmc_get_rsp(struct sp_mmc_host *host, struct mmc_cmd *cmd)
{
	struct sp_mmc_hw_ops *ops = host->ops;

	sp_sd_trace();
	ops->get_response(host, cmd);

	return;
}


/* Set SD card clock divider value based on the required clock in HZ */
static void sp_mmc_set_clock(struct mmc *mmc, uint clock)
{
	struct sp_mmc_host *host = mmc->priv;
	struct sp_mmc_hw_ops *ops = host->ops;
	uint clkrt, sys_clk;
	/* uint act_clock; */
	uint wr_dly;
	if (clock < mmc->cfg->f_min)
		clock = mmc->cfg->f_min;
	if (clock > mmc->cfg->f_max)
		clock = mmc->cfg->f_max;

	if (SP_MMC_VER_Q628 == host->dev_info.version)
		sys_clk = SPMMC_ZEBU_CLK_SRC;
	else
		sys_clk = SPMMC_CLK_SRC;
	clkrt = (sys_clk / clock) - 1;

	/* Calculate the actual clock for the divider used */
	/* act_clock = sys_clk / (clkrt + 1); */
	/* printf("sys_clk =%u, act_clock=%u, clkrt = %u\n", sys_clk, act_clock, clkrt); */
	/* check clock divider boundary and correct it */
	if (clkrt > 0xFFF)
		clkrt = 0xFFF;

	ops->set_clock(host, clkrt);
	/*
	 *Host adjusts the data sampling edge and send edge depending on the speed mode used.
	 * read delay:
	 * default speed controller sample data at fall edge, card send data at fall edge
	 * high speed controller sample data at rise edge, card send data at rise edge
	 * tunel (clkrt + 1)/2 clks to ensure controller sample In the middle of the data.
	 * write delay:
	 *  default speed controller send data at fall edge, card sample data at rise edge
	 * high speed controller send data at rise edge, card sample data at rise edge
	 * so we need to tunel write delay (clkrt + 1)/2  clks at high speed to ensure card sample right data
	 */
	wr_dly = (clkrt + 1) / 2 > MAX_DLY_CLK ? MAX_DLY_CLK:(clkrt + 1) / 2;
	host->timing_info.wr_dat_dly = wr_dly;
	host->timing_info.wr_cmd_dly = wr_dly;

	/* clock >  25Mhz : High Speed Mode
	 *       <= 25Mhz : Default Speed Mode
	 */
	if (mmc->clock > 25000000) {
		ops->highspeed_en(host, true);
	} else {
		ops->highspeed_en(host, false);
	}
	/* Write delay: Controls CMD, DATA signals timing to SD Card */
	ops->tunel_write_dly(host, &host->timing_info);
}


static int Reset_Controller(struct sp_mmc_host *host)
{
	sp_mmc_hw_ops *ops = host->ops;
	return ops->reset_mmc(host);
}


static int Sd_Bus_Reset_Channel(struct sp_mmc_host *host)
{
	sp_mmc_hw_ops *ops = host->ops;
	return ops->reset_dma(host);
}

static void sp_mmc_prep_cmd_rsp(struct sp_mmc_host *host, struct mmc_cmd *cmd)
{
	sp_mmc_hw_ops *ops = host->ops;
	/* printk("Process Command & Response (No Data)\n"); */
	/* Reset */
	Reset_Controller(host);
	ops->set_cmd(host, cmd);

	return;
}



static void sp_mmc_check_sdstatus_errors(struct sp_mmc_host *host, struct mmc_data *data, int *ret)
{
	sp_mmc_hw_ops *ops = host->ops;
	int val;
	val = ops->check_error(host, data ? true : false);
	if (ret) {
		*ret = val;
		ops->tunel_read_dly(host, &host->timing_info);
		ops->tunel_write_dly(host, &host->timing_info);
	}
	return;
}

/*
 * DMA transfer mode, used for all other data transfer commands besides read/write block commands (cmd17, 18, 24, 25)
 * Due to host limitations, this kind of DMA transfer mode only supports 1 consecution memory area
 */
static void sp_mmc_prep_data_info(struct sp_mmc_host *host, struct mmc_cmd *cmd, struct mmc_data *data)
{
	unsigned int hw_address = 0, hw_len = 0;
	sp_mmc_hw_ops *ops = host->ops;

	/* retreive physical memory address & size of the fragmented memory blocks */
	if (data->flags & MMC_DATA_WRITE)
		hw_address = (unsigned int) data->src;
	else
		hw_address = (unsigned int) data->dest;

	hw_len = data->blocksize * data->blocks;
	DPRINTK("block size = %d, blks = %d, hw_len %u\n", data->blocksize, data->blocks, hw_len);
	ops->set_data_info(host, cmd, data);
	sp_mmc_dcache_flush_invalidate(data, hw_address, hw_len);

	return;
}


static void sp_mmc_trigger_sdstate(struct sp_mmc_host *host)
{
	sp_mmc_hw_ops *ops = host->ops;

	ops->trigger(host);
}

static inline void sp_mmc_txdummy(struct sp_mmc_host *host)
{
	sp_mmc_hw_ops *ops = host->ops;
	ops->tx_dummy(host);
}

static void sp_mmc_wait_sdstate_new(struct sp_mmc_host *host)
{
	/* Wait transaction to finish (either done or error occured) */
	sp_mmc_hw_ops *ops = host->ops;
	sp_sd_trace();
	while (1) {
		sp_sd_trace();
		if(ops->check_finish(host)) {
			break;
		}
	}
	sp_sd_trace();
}


void send_stop_cmd(struct sp_mmc_host *host)
{
	struct mmc_cmd stop = {};
	stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	stop.cmdarg = 0;
	stop.resp_type = MMC_RSP_R1b;

	sp_mmc_prep_cmd_rsp(host, &stop);
	sp_mmc_trigger_sdstate(host);
	sp_mmc_get_rsp(host, &stop); /* Makes sure host returns to a idle or error state */
	/* sp_mmc_check_sdstatus_errors(host, NULL, NULL); */
}

/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
/*
 * For data transfer requests
 * 1. CMD 17, 18, 24, 25 : Use HW DMA
 * 2. Other data transfer commands : Use normal DMA
 */
#ifdef CONFIG_DM_MMC
	static int
sp_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	sp_sd_trace();
	struct mmc *mmc = mmc_get_mmc_dev(dev);
#else
static int
sp_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	sp_sd_trace();
#endif
	struct sp_mmc_host *host = mmc->priv;

	int ret = 0; /* Return 0 means success, returning other stuff means error */
	int i = 0;

	DPRINTK("cmd %d with data %p\n", cmd->cmdidx, data);
	host->current_cmd = cmd;
	/* hw dma auto send cmd12  */
	if (MMC_CMD_STOP_TRANSMISSION == cmd->cmdidx)
		return 0;

	for (i = 0; i < CMD_MAX_RETRY; ++i)
	{
		sp_sd_trace();
		/* post process send command requests, trigger transaction */
		if (data == NULL) {
			sp_mmc_prep_cmd_rsp(host, cmd);
			sp_mmc_trigger_sdstate(host);
			if (cmd->resp_type & MMC_RSP_PRESENT) {
				sp_mmc_get_rsp(host, cmd); /* Makes sure host returns to a idle or error state */
			} else {
				sp_sd_trace();
				sp_mmc_wait_sdstate_new(host);
				sp_sd_trace();
			}

			sp_sd_trace();
			sp_mmc_check_sdstatus_errors(host, data, &ret);
		} else {
			sp_sd_trace();
			sp_mmc_prep_data_info(host, cmd, data);
			sp_mmc_trigger_sdstate(host);

			/* Host's "read data start bit timeout counter" is broken, use
			 * use software to set "the whole transaction's timeout" instead
			 */

			if(SP_MMC_PIO_MODE == host->dmapio_mode) {
				sp_mmc_read_data_pio(host, data);
			}

			sp_mmc_get_rsp(host, cmd); /* Makes sure host returns to a idle or error state */
			sp_mmc_check_sdstatus_errors(host, data, &ret);
		}

		if (!ret) {
			break;
		} else {
			if ((MMC_CMD_READ_MULTIPLE_BLOCK == cmd->cmdidx)
			    || (MMC_CMD_WRITE_MULTIPLE_BLOCK == cmd->cmdidx)) {
				send_stop_cmd(host);
			}
			/* MMC_CMD_SEND_OP_COND response timeout need to re-init */
			if (cmd->cmdidx == MMC_CMD_SEND_OP_COND)
				break;
		}
	}

	return ret;
}

	/* Set buswidth or clock as indicated by the GENERIC_MMC framework */
#ifdef CONFIG_DM_MMC
static int sp_mmc_set_ios(struct udevice *dev)
{
	sp_sd_trace();
	struct mmc *mmc = mmc_get_mmc_dev(dev);
#else
static int sp_mmc_set_ios(struct mmc *mmc)
{
	sp_sd_trace();
#endif
	struct sp_mmc_host *host = mmc->priv;
	struct sp_mmc_hw_ops *ops = host->ops;
	/* Set clock speed */
	if (mmc->clock)
		sp_mmc_set_clock(mmc, mmc->clock);

	ops->set_bus_width(host, mmc->bus_width);
	ops->set_sdddr_mode(host, mmc->ddr_mode);

	return 0;
}

/*
 * Card detection, return 1 to indicate card is plugged in
 *                 return 0 to indicate card in not plugged in
 */
#ifdef CONFIG_DM_MMC
static int sp_mmc_getcd(struct udevice *dev)
{
	sp_sd_trace();
	struct mmc *mmc = mmc_get_mmc_dev(dev);
#else
static int sp_mmc_getcd(struct mmc *mmc)
{
	sp_sd_trace();
#endif
	struct sp_mmc_host *priv_data = mmc->priv;

	/* Names of card detection GPIO pin # configs found in runtime.cfg */
	if(EMMC_SLOT_ID == priv_data->dev_info.id)
	{
		return 1; /* emmc aways inserted */
	}
	else {
		return 1; /* fix me  */
	}
}


#ifdef CONFIG_DM_MMC
const struct dm_mmc_ops sp_mmc_ops = {
	.send_cmd	= sp_mmc_send_cmd,
	.set_ios	= sp_mmc_set_ios,
	.get_cd		= sp_mmc_getcd,
};
#else

#endif


static int sp_mmc_ofdata_to_platdata(struct udevice *dev)
{
	sp_sd_trace();
	struct sp_mmc_host *priv = dev_get_priv(dev);

	priv->base = (volatile SDREG *)devfdt_get_addr(dev);
	IFPRINTK("base:%p\n", priv->base);

	return 0;
}

static int sp_mmc_bind(struct udevice *dev)
{
	sp_sd_trace();
	struct sp_mmc_plat *plat = dev_get_platdata(dev);

	return mmc_bind(dev, &plat->mmc, &plat->cfg);
}


/* hw related ops */
/* sd controller ops */
/* Initialize MMC/SD controller */
static int sp_sd_hw_init(sp_mmc_host *host)
{
	host->base->sdddrmode = 0;
	host->base->sdiomode = 0;

	host->base->sdrsptmr = 0x7ff;
	host->base->sdcrctmr = 0x7ff;
	host->base->sdcrctmren = 1;
	host->base->sdrsptmren = 1;
	host->base->sdmmcmode = 1;
	host->base->sdrxdattmr_sel = SP_SD_RXDATTMR_MAX;
	host->base->mediatype = 6;

	return 0;
}

static int sp_sd_hw_set_clock(struct sp_mmc_host *host, uint div)
{
	host->base->sdfqsel = div & 0xFFF;
	/* Delay 4 msecs for now (wait till clk stabilizes?) */
	mdelay(4);

	return 0;
}

static int sp_sd_hw_tunel_read_dly (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	host->base->sd_rd_dly_sel = dly->rd_dly;
	return 0;
}

static int sp_sd_hw_tunel_write_dly  (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	host->base->sd_wr_dly_sel = dly->wr_dly;
	return 0;
}

static int sp_sd_hw_tunel_clock_dly  (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	host->base->sd_clk_dly_sel = dly->clk_dly;
	return 0;
}

int sp_sd_hw_highspeed_en (sp_mmc_host *host, bool en)
{
	if (en)
		host->base->sd_high_speed_en = 1;
	else
		host->base->sd_high_speed_en = 0;

	return 0;
}

int sp_sd_hw_set_bus_width (sp_mmc_host *host, uint bus_width)
{
	/* Set the bus width */
	if (bus_width == 4) {
		host->base->sddatawd = 1;
		host->base->mmc8_en = 0;
		/* printf("sd 4bit mode\n"); */
	} else if(bus_width == 8) {
		host->base->sddatawd = 0;
		host->base->mmc8_en = 1;
		/* printf("sd 8bit mode\n"); */
	} else {
		/* printf("sd 1bit mode\n"); */
		host->base->sddatawd = 0;
		host->base->mmc8_en = 0;
	}

	return 0;
}

int sp_sd_hw_set_sdddr_mode (sp_mmc_host *host, int timing)
{
	return 0;
}

int sp_sd_hw_set_cmd (sp_mmc_host *host, struct mmc_cmd *cmd)
{
	/* printf("Configuring registers\n"); */
	/* Configure Group SD Registers */
	host->base->sd_cmdbuf[0] = (u8)(cmd->cmdidx | 0x40);	/* add start bit, according to spec, command format */
	host->base->sd_cmdbuf[1] = (u8)((cmd->cmdarg >> 24) & 0x000000ff);
	host->base->sd_cmdbuf[2] = (u8)((cmd->cmdarg >> 16) & 0x000000ff);
	host->base->sd_cmdbuf[3] = (u8)((cmd->cmdarg >>  8) & 0x000000ff);
	host->base->sd_cmdbuf[4] = (u8)((cmd->cmdarg >>  0) & 0x000000ff);

	/* Configure SD INT reg (Disable them) */
	host->base->dmacmpen_interrupt = 0;
	host->base->sdcmpen = 0x0;
	host->base->sd_trans_mode = 0x0;
	host->base->sdcmddummy = 1;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		host->base->sdautorsp = 1;
	}
	else {
		host->base->sdautorsp = 0;
		return 0;
	}

	/*
	 * Currently, host is not capable of checking Response R2's CRC7
	 * Because of this, enable response crc7 check only for 48 bit response commands
	 */
	if (cmd->resp_type & MMC_RSP_CRC && !(cmd->resp_type & MMC_RSP_136))
		host->base->sdrspchk_en = 0x1;
	else
		host->base->sdrspchk_en = 0x0;

	if (cmd->resp_type & MMC_RSP_136)
		host->base->sdrsptype = 0x1;
	else
		host->base->sdrsptype = 0x0;

	return 0;
}


int sp_sd_hw_get_reseponse (sp_mmc_host *host, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136)
		sp_mmc_get_rsp_136(host, cmd);
	else
		sp_mmc_get_rsp_48(host, cmd);

	return 0;
}



int sp_sd_hw_set_data_info (sp_mmc_host *host, struct mmc_cmd *cmd, struct mmc_data *data)
{
	unsigned int hw_address;
	/* Reset */
	Reset_Controller(host);
	/* Configure Group SD Registers */
	host->base->sd_cmdbuf[0] = (u8)(cmd->cmdidx | 0x40);	/* add start bit, according to spec, command format */
	host->base->sd_cmdbuf[1] = (u8)((cmd->cmdarg >> 24) & 0x000000ff);
	host->base->sd_cmdbuf[2] = (u8)((cmd->cmdarg >> 16) & 0x000000ff);
	host->base->sd_cmdbuf[3] = (u8)((cmd->cmdarg >>  8) & 0x000000ff);
	host->base->sd_cmdbuf[4] = (u8)((cmd->cmdarg >>  0) & 0x000000ff);

	SD_PAGE_NUM_SET(host->base, data->blocks);
	if (cmd->resp_type & MMC_RSP_CRC && !(cmd->resp_type & MMC_RSP_136))
		host->base->sdrspchk_en = 0x1;
	else
		host->base->sdrspchk_en = 0x0;

	if (data->flags & MMC_DATA_READ) {
		host->base->sdcmddummy = 0;
		host->base->sdautorsp = 0;
		host->base->sd_trans_mode = 2;
	} else {
		host->base->sdcmddummy = 1;
		host->base->sdautorsp = 1;
		host->base->sd_trans_mode = 1;
	}

	if ((MMC_CMD_READ_MULTIPLE_BLOCK == cmd->cmdidx) || (MMC_CMD_WRITE_MULTIPLE_BLOCK == cmd->cmdidx))
		host->base->sd_len_mode = 0;
	else
		host->base->sd_len_mode = 1;
	host->base->sdpiomode = 0;
	host->base->sdcrctmren = 1;
	host->base->sdrsptmren = 1;
	host->base->hw_dma_en = 0;
	/* Set response type */
	if(cmd->resp_type & MMC_RSP_136)
		host->base->sdrsptype = 0x1;
	else
		host->base->sdrsptype = 0x0;

	SDDATALEN_SET(host->base, data->blocksize);

	/* Configure Group DMA Registers */
	if (data->flags & MMC_DATA_WRITE) {
		host->base->dmadst = 0x2;
		host->base->dmasrc = 0x1;
		hw_address = (unsigned int) data->src;
	} else {
		host->base->dmadst = 0x1;
		host->base->dmasrc = 0x2;
		hw_address = (unsigned int) data->dest;
	}
	DMASIZE_SET(host->base, data->blocksize);
	/* printf("hw_address 0x%x\n", hw_address); */
	SET_HW_DMA_BASE_ADDR(host->base, hw_address);

	/*
	 * Configure SD INT reg
	 * Disable HW DMA data transfer complete interrupt (when using sdcmpen)
	 */
	host->base->dmacmpen_interrupt = 0;
	host->base->sdcmpen = 0x0;

	return 0;
}

int sp_sd_hw_trigger (sp_mmc_host *host)
{
	host->base->sdctrl_trigger_cmd = 1;   /* Start transaction */
	return 0;
}

int sp_sd_hw_reset_mmc (sp_mmc_host *host)
{
	sp_sd_trace();
	SD_RST_seq(host->base);
	sp_sd_trace();

	return 0;
}

int sp_sd_hw_reset_dma (sp_mmc_host *host)
{
	host->base->rst_cnad = 1;
	/*reset Central FIFO*/
	/* Wait for channel reset to complete */
	while (host->base->rst_cnad == 1) {
		sp_sd_trace();
		udelay(1);
	}


	return 0;
}

int sp_sd_hw_reset_sdio (sp_mmc_host *host)
{
	return 0;
}

int sp_sd_hw_wait_data_timeout (sp_mmc_host *host, uint timeout)
{
	return 0;
}

int sp_sd_hw_check_finish (sp_mmc_host *host)
{
	/* printf("sdstate_new 0x%x\n", host->base->sdstate_new); */
	if ((host->base->sdstate_new & SDSTATE_NEW_FINISH_IDLE) == 0x40)
			return 1;
	if ((host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) == 0x20)
			return 1;

	return 0;
}

int	sp_sd_hw_tx_dummy (sp_mmc_host *host)
{
	return 0;
}

int sp_sd_hw_check_error (sp_mmc_host *host, bool with_data)
{
	int ret = 0;

	if (host->base->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) {
		/* Response related errors */
		if (host->base->sdstatus & SP_SDSTATUS_WAIT_RSP_TIMEOUT) {
			host->timing_info.wr_dly++;
			ret = -ETIMEDOUT;
		} else if (host->base->sdstatus & SP_SDSTATUS_RSP_CRC7_ERROR) {
			host->timing_info.rd_dly++;
			ret = -EILSEQ;
		} else if (with_data) {
			/* Data transaction related errors */
			if (host->base->sdstatus & SP_SDSTATUS_WAIT_STB_TIMEOUT) {
				host->timing_info.rd_dly++;
				ret = -ETIMEDOUT;
			} else if (host->base->sdstatus & SP_SDSTATUS_WAIT_CARD_CRC_CHECK_TIMEOUT) {
				host->timing_info.rd_dly++;
				ret = -ETIMEDOUT;
			} else if (host->base->sdstatus & SP_SDSTATUS_CRC_TOKEN_CHECK_ERROR) {
				host->timing_info.wr_dly++;
				ret = -EILSEQ;
			} else if (host->base->sdstatus & SP_SDSTATUS_RDATA_CRC16_ERROR) {
				host->timing_info.rd_dly++;
				ret = -EILSEQ;
			}

			Sd_Bus_Reset_Channel(host);
		}

		/*
		 * By now, ret should be set to a known error case, the below code
		 * snippet warns user a unknown error occurred
		 */
		if ((ret != -EILSEQ) && (ret != -ETIMEDOUT)) {
			EPRINTK("[SDCard] Unknown error occurred!\n");
			ret = -EILSEQ;
		}
	}

	return ret;
}

/* emmc */
/* Initialize eMMC controller */
static int sp_emmc_hw_init(sp_mmc_host *host)
{
	sp_sd_trace();
	volatile EMMCREG *base = host->ebase;

	base->sdddrmode = 0;
	base->sdiomode = 0;

	base->sdrsptmr = 0xff;
	base->sdcrctmr = 0x7ff;
	base->sdcrctmren = 1;
	base->sdrsptmren = 1;
	base->sdmmcmode = 1;
	base->sd_rxdattmr = SP_EMMC_RXDATTMR_MAX;
	base->mediatype = 6;
	mdelay(10);

	return 0;
}

static int sp_emmc_hw_set_clock(struct sp_mmc_host *host, uint div)
{
	host->ebase->sdfqsel = div;
	mdelay(4);
	/* clock >  25Mhz : High Speed Mode
	 *       <= 25Mhz : Default Speed Mode
	 */
	return 0;
}


static int sp_emmc_hw_tunel_read_dly (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	host->ebase->sd_rd_rsp_dly_sel = dly->rd_rsp_dly;
	host->ebase->sd_rd_dat_dly_sel = dly->rd_dat_dly;
	host->ebase->sd_rd_crc_dly_sel = dly->rd_crc_dly;
	return 0;
}

static int sp_emmc_hw_tunel_write_dly  (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	host->ebase->sd_wr_dat_dly_sel = dly->wr_dat_dly;
	host->ebase->sd_wr_cmd_dly_sel = dly->wr_cmd_dly;
	return 0;
}

static int sp_emmc_hw_tunel_clock_dly (sp_mmc_host *host, sp_mmc_timing_info *dly)
{
	sp_sd_trace();
	host->ebase->sd_clk_dly_sel = dly->clk_dly;
	return 0;
}



int sp_emmc_hw_highspeed_en (sp_mmc_host *host, bool en)
{
	sp_sd_trace();
	if (en)
		host->ebase->sd_high_speed_en = 1;
	else
		host->ebase->sd_high_speed_en = 0;

	return 0;
}

int sp_emmc_hw_set_bus_width (sp_mmc_host *host, uint bus_width)
{
	sp_sd_trace();
	/* Set the bus width */
	if (bus_width == 4) {
		host->ebase->sddatawd = 1;
		host->ebase->mmc8_en = 0;
		/* printf("sd 4bit mode\n"); */
	} else if(bus_width == 8) {
		host->ebase->sddatawd = 0;
		host->ebase->mmc8_en = 1;
		/* printf("sd 8bit mode\n"); */
	} else {
		/* printf("sd 1bit mode\n"); */
		host->ebase->sddatawd = 0;
		host->ebase->mmc8_en = 0;
	}

	return 0;
}

int sp_emmc_hw_set_sdddr_mode (sp_mmc_host *host, int ddrmode)
{
	sp_sd_trace();
	if (ddrmode)
		host->ebase->sdddrmode = 1;
	else
		host->ebase->sdddrmode = 0;

	return 0;
}

int sp_emmc_hw_set_cmd (sp_mmc_host *host, struct mmc_cmd *cmd)
{
	sp_sd_trace();
	/* printf("Configuring registers\n"); */
	/* Configure Group SD Registers */
	host->ebase->sd_cmdbuf[3] = (u8)(cmd->cmdidx | 0x40);	/* add start bit, according to spec, command format */
	host->ebase->sd_cmdbuf[2] = (u8)((cmd->cmdarg >> 24) & 0xff);
	host->ebase->sd_cmdbuf[1] = (u8)((cmd->cmdarg >> 16) & 0xff);
	host->ebase->sd_cmdbuf[0] = (u8)((cmd->cmdarg >>  8) & 0xff);
	host->ebase->sd_cmdbuf[4] = (u8)((cmd->cmdarg >>  0) & 0xff);

	/* Configure SD INT reg (Disable them) */
	host->ebase->hwdmacmpen = 0;
	host->ebase->sdcmpen = 0x0;
	host->ebase->sd_trans_mode = 0x0;
	host->ebase->sdcmddummy = 1;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		host->ebase->sdautorsp = 1;
	}
	else {
		host->ebase->sdautorsp = 0;
		return 0;
	}

	/*
	 * Currently, host is not capable of checking Response R2's CRC7
	 * Because of this, enable response crc7 check only for 48 bit response commands
	 */
	if (cmd->resp_type & MMC_RSP_CRC && !(cmd->resp_type & MMC_RSP_136))
		host->ebase->sdrspchk_en = 0x1;
	else
		host->ebase->sdrspchk_en = 0x0;

	if (cmd->resp_type & MMC_RSP_136)
		host->ebase->sdrsptype = 0x1;
	else
		host->ebase->sdrsptype = 0x0;

	return 0;
}


int sp_emmc_hw_get_reseponse (sp_mmc_host *host, struct mmc_cmd *cmd)
{
	sp_sd_trace();
	unchar *rspBuf = (unchar *)cmd->response;
	int i;

	while (1) {
		if (host->ebase->sdstatus & SP_SDSTATUS_RSP_BUF_FULL)
			break;	/* Wait until response buffer full */

		if (host->ebase->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
			break;

		if (host->ebase->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
			return -EIO;
	}

	if (cmd->resp_type & MMC_RSP_136) {
		uint val[2];
		val[0] = host->ebase->sd_rspbuf[0];
		val[1] = host->ebase->sd_rspbuf[1];
		rspBuf[0] = (val[0] >> 16) & 0xff;
		rspBuf[1] = (val[0] >> 8) & 0xff;
		rspBuf[2] = (val[0] >> 0) & 0xff;
		rspBuf[3] = (val[1] >> 8) & 0xff;
		rspBuf[4] = (val[1] >> 0) & 0xff;

		val[0] = host->ebase->sd_rspbuf[0];
		val[1] = host->ebase->sd_rspbuf[1];
		rspBuf[5] = (val[0] >> 24) & 0xff;
		rspBuf[6] = (val[0] >> 16) & 0xff;
		rspBuf[7] = (val[0] >> 8) & 0xff;
		rspBuf[8] = (val[0] >> 0) & 0xff;
		rspBuf[9] = (val[1] >> 8) & 0xff;
		rspBuf[10] = (val[1] >> 0) & 0xff;

		val[0] = host->ebase->sd_rspbuf[0];
		val[1] = host->ebase->sd_rspbuf[1];
		rspBuf[11] = (val[0] >> 24) & 0xff;
		rspBuf[12] = (val[0] >> 16) & 0xff;
		rspBuf[13] = (val[0] >> 8) & 0xff;
		rspBuf[14] = (val[0] >> 0) & 0xff;
		rspBuf[15] = (val[1] >> 8) & 0xff;

		for (i = 0; i < SP_MMC_MAX_RSP_LEN/sizeof(cmd->response[0]); i++ ) {
			cmd->response[i] = SP_MMC_SWAP32(cmd->response[i]);
		}
	}
	else {
		rspBuf[0] = host->ebase->sd_rspbuf0;
		rspBuf[0] = host->ebase->sd_rspbuf1;
		rspBuf[1] = host->ebase->sd_rspbuf2;
		rspBuf[2] = host->ebase->sd_rspbuf3;
		rspBuf[3] = host->ebase->sd_rspbuf4;
		rspBuf[4] = host->ebase->sd_rspbuf5;

		cmd->response[0] = SP_MMC_SWAP32(cmd->response[0]);
	}

	while (1) {
		if (host->ebase->sdstate_new & SDSTATE_NEW_FINISH_IDLE)
			break;

		if (host->ebase->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
			return -EIO;
	}

	return 0;
}

static int sp_mmc_read_data_pio (sp_mmc_host *host,  struct mmc_data *data)
{
	unsigned int need_read_count;
	unsigned int* pTmp_buf;

	if (data->flags & MMC_DATA_WRITE) {
		pTmp_buf = (unsigned int *) data->src;
	} else {
		pTmp_buf = (unsigned int *) data->dest;
	}

	need_read_count = data->blocks * data->blocksize;
	while(need_read_count > 0) {
		while (1) {
			if (data->flags & MMC_DATA_READ) {
				if ((host->ebase->sdstatus & SP_SDSTATUS_RX_DATA_BUF_FULL) == SP_SDSTATUS_RX_DATA_BUF_FULL) {
					break;
				}
			}
			else {
				if ((host->ebase->sdstatus & SP_SDSTATUS_TX_DATA_BUF_EMPTY) == SP_SDSTATUS_TX_DATA_BUF_EMPTY) {
					break;
				}
			}

			if (host->ebase->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT)
				return -EIO;
		}

		if (data->flags & MMC_DATA_WRITE)
			host->ebase->sd_piodatatx = *pTmp_buf;
		else
			*pTmp_buf = host->ebase->sd_piodatarx;

		pTmp_buf++;
		need_read_count -= 4;
	}

	return 0;
}

int sp_emmc_hw_set_data_info (sp_mmc_host *host, struct mmc_cmd *cmd, struct mmc_data *data)
{
	sp_sd_trace();
	unsigned int hw_address;
	/* Reset */
	Reset_Controller(host);
	Sd_Bus_Reset_Channel(host);

	/* Configure Group SD Registers */
	host->ebase->sd_cmdbuf[3] = (u8)(cmd->cmdidx | 0x40);	/* add start bit, according to spec, command format */
	host->ebase->sd_cmdbuf[2] = (u8)((cmd->cmdarg >> 24) & 0x000000ff);
	host->ebase->sd_cmdbuf[1] = (u8)((cmd->cmdarg >> 16) & 0x000000ff);
	host->ebase->sd_cmdbuf[0] = (u8)((cmd->cmdarg >>  8) & 0x000000ff);
	host->ebase->sd_cmdbuf[4] = (u8)((cmd->cmdarg >>  0) & 0x000000ff);


	if (cmd->resp_type & MMC_RSP_CRC && !(cmd->resp_type & MMC_RSP_136))
		host->ebase->sdrspchk_en = 0x1;
	else
		host->ebase->sdrspchk_en = 0x0;


	if (data->flags & MMC_DATA_READ) {
		host->ebase->sdcmddummy = 0;
		host->ebase->sdautorsp = 0;
		host->ebase->sd_trans_mode = 2;
	} else {
		host->ebase->sdcmddummy = 1;
		host->ebase->sdautorsp = 1;
		host->ebase->sd_trans_mode = 1;
	}

	if ((MMC_CMD_READ_MULTIPLE_BLOCK == cmd->cmdidx) || (MMC_CMD_WRITE_MULTIPLE_BLOCK == cmd->cmdidx))
		host->ebase->sd_len_mode = 0;
	else
		host->ebase->sd_len_mode = 1;

	host->ebase->sdcrctmren = 1;
	host->ebase->sdrsptmren = 1;
	host->ebase->hw_dma_en = 0;
	/* Set response type */
	if(cmd->resp_type & MMC_RSP_136)
		host->ebase->sdrsptype = 0x1;
	else
		host->ebase->sdrsptype = 0x0;

	SD_PAGE_NUM_SET(host->ebase, data->blocks);
	SDDATALEN_SET(host->ebase, data->blocksize);
	if(SP_MMC_PIO_MODE == host->dmapio_mode) {
		host->ebase->sdpiomode = 1;
		host->ebase->rx4_en = 1;
	}
	else {
		host->ebase->sdpiomode = 0;
#define SP_MMC_DMA_DEVICE		2ul
#define SP_MMC_DMADST_SHIFT		8
#define SP_MMC_DMA_HOST			1ul
#define SP_MMC_DMASRC_SHIFT		4
#define SP_MMC_DMA_TO_DEVICE	((SP_MMC_DMA_DEVICE << SP_MMC_DMADST_SHIFT) | (SP_MMC_DMA_HOST << SP_MMC_DMASRC_SHIFT))
#define SP_MMC_DMA_FROM_DEVICE  ((SP_MMC_DMA_HOST << SP_MMC_DMADST_SHIFT) | (SP_MMC_DMA_DEVICE << SP_MMC_DMASRC_SHIFT))
#define SP_MMC_DMA_MASK			((0x3ul<< SP_MMC_DMADST_SHIFT) | (0x03ul << SP_MMC_DMASRC_SHIFT))

		host->ebase->medatype_dma_src_dst &= ~SP_MMC_DMA_MASK;
		/* Configure Group DMA Registers */
		if (data->flags & MMC_DATA_WRITE) {
			host->ebase->medatype_dma_src_dst |= SP_MMC_DMA_TO_DEVICE;
#if 0
			host->ebase->dmadst = 0x2;
			host->ebase->dmasrc = 0x1;
#endif
			hw_address = (unsigned int) data->src;
		} else {
			host->ebase->medatype_dma_src_dst |= SP_MMC_DMA_FROM_DEVICE;
#if 0
			host->ebase->dmadst = 0x1;
			host->ebase->dmasrc = 0x2;
#endif
			hw_address = (unsigned int) data->dest;
		}
		host->ebase->dma_base_addr = hw_address;
		SP_MMC_SECTOR_NUM_SET(host->ebase->sdram_sector_0_size, data->blocks);
	}
	/*
	 * Configure SD INT reg
	 * Disable HW DMA data transfer complete interrupt (when using sdcmpen)
	 */
	host->ebase->hwdmacmpen = 0;
	host->ebase->sdcmpen = 0x0;

	return 0;
}

int sp_emmc_hw_trigger (sp_mmc_host *host)
{
	sp_sd_trace();
	host->ebase->sdctrl0 = 1;
	return 0;
}

int sp_emmc_hw_reset_mmc (sp_mmc_host *host)
{
	sp_sd_trace();
	SD_RST_seq(host->ebase);
	sp_sd_trace();

	return 0;
}

int sp_emmc_hw_reset_dma (sp_mmc_host *host)
{
	sp_sd_trace();
	host->ebase->hw_dma_rst = 1;
	/*reset Central FIFO*/
	/* Wait for channel reset to complete */
	while (host->ebase->hwsd_sm & SP_SD_HW_DMA_ERROR) {
		sp_sd_trace();
		udelay(1);
	}

	return 0;
}

int sp_emmc_hw_reset_sdio (sp_mmc_host *host)
{
	sp_sd_trace();
	return 0;
}

int sp_emmc_hw_wait_data_timeout(sp_mmc_host *host, uint timeout)
{
	sp_sd_trace();
	return 0;
}

int sp_emmc_hw_check_finish (sp_mmc_host *host)
{
	sp_sd_trace();
	if ((host->ebase->sdstate_new & SDSTATE_NEW_FINISH_IDLE) == 0x40)
		return 1;
	if ((host->ebase->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) == 0x20)
		return 1;

	return 0;
}

int	sp_emmc_hw_tx_dummy (sp_mmc_host *host)
{
	sp_sd_trace();
	host->ebase->sdctrl1 = 1;
	return 0;
}

int sp_emmc_hw_check_error (sp_mmc_host *host, bool with_data)
{
	sp_sd_trace();
	int ret = 0;

	if (host->ebase->sdstate_new & SDSTATE_NEW_ERROR_TIMEOUT) {
		/* Response related errors */
		if (host->ebase->sdstatus & SP_SDSTATUS_WAIT_RSP_TIMEOUT) {
			host->timing_info.wr_cmd_dly++;
			ret = -ETIMEDOUT;
		} else if (host->ebase->sdstatus & SP_SDSTATUS_RSP_CRC7_ERROR) {
			host->timing_info.rd_rsp_dly++;
			ret = -EILSEQ;
		} else if (with_data) {
			/* Data transaction related errors */
			if (host->ebase->sdstatus & SP_SDSTATUS_WAIT_STB_TIMEOUT) {
				host->timing_info.rd_dat_dly++;
				ret = -ETIMEDOUT;
			} else if (host->ebase->sdstatus & SP_SDSTATUS_WAIT_CARD_CRC_CHECK_TIMEOUT) {
				host->timing_info.rd_dat_dly++;
				ret = -ETIMEDOUT;
			} else if (host->ebase->sdstatus & SP_SDSTATUS_CRC_TOKEN_CHECK_ERROR) {
				host->timing_info.wr_dat_dly++;
				ret = -EILSEQ;
			} else if (host->ebase->sdstatus & SP_SDSTATUS_RDATA_CRC16_ERROR) {
				host->timing_info.rd_dat_dly++;
				ret = -EILSEQ;
			}

			Sd_Bus_Reset_Channel(host);
		}

		/*
		 * By now, ret should be set to a known error case, the below code
		 * snippet warns user a unknown error occurred
		 */
		if ((ret != -EILSEQ) && (ret != -ETIMEDOUT)) {
			EPRINTK("[SDCard] Unknown error occurred!\n");
			ret = -EILSEQ;
		}
	}

	return ret;
}



sp_mmc_hw_ops sd_hw_ops = {
	.hw_init			= sp_sd_hw_init,
	.set_clock			= sp_sd_hw_set_clock,
	.highspeed_en		= sp_sd_hw_highspeed_en,
	.tunel_read_dly		= sp_sd_hw_tunel_read_dly,
	.tunel_write_dly	= sp_sd_hw_tunel_write_dly,
	.tunel_clock_dly	= sp_sd_hw_tunel_clock_dly,
	.set_bus_width		= sp_sd_hw_set_bus_width,
	.set_sdddr_mode		= sp_sd_hw_set_sdddr_mode,
	.set_cmd			= sp_sd_hw_set_cmd,
	.get_response		= sp_sd_hw_get_reseponse,
	.set_data_info		= sp_sd_hw_set_data_info,
	.trigger			= sp_sd_hw_trigger,
	.reset_mmc			= sp_sd_hw_reset_mmc,
	.reset_dma			= sp_sd_hw_reset_dma,
	.reset_sdio			= sp_sd_hw_reset_sdio,
	.wait_data_timeout	= sp_sd_hw_wait_data_timeout,
	.check_finish		= sp_sd_hw_check_finish,
	.tx_dummy			= sp_sd_hw_tx_dummy,
	.check_error		= sp_sd_hw_check_error,
};

sp_mmc_hw_ops emmc_hw_ops = {
	.hw_init			= sp_emmc_hw_init,
	.set_clock			= sp_emmc_hw_set_clock,
	.highspeed_en		= sp_emmc_hw_highspeed_en,
	.tunel_read_dly		= sp_emmc_hw_tunel_read_dly,
	.tunel_write_dly	= sp_emmc_hw_tunel_write_dly,
	.tunel_clock_dly	= sp_emmc_hw_tunel_clock_dly,
	.set_bus_width		= sp_emmc_hw_set_bus_width,
	.set_sdddr_mode		= sp_emmc_hw_set_sdddr_mode,
	.set_cmd			= sp_emmc_hw_set_cmd,
	.get_response		= sp_emmc_hw_get_reseponse,
	.set_data_info		= sp_emmc_hw_set_data_info,
	.trigger			= sp_emmc_hw_trigger,
	.reset_mmc			= sp_emmc_hw_reset_mmc,
	.reset_dma			= sp_emmc_hw_reset_dma,
	.reset_sdio			= sp_emmc_hw_reset_sdio,
	.wait_data_timeout	= sp_emmc_hw_wait_data_timeout,
	.check_finish		= sp_emmc_hw_check_finish,
	.tx_dummy			= sp_emmc_hw_tx_dummy,
	.check_error		= sp_emmc_hw_check_error,
};




static int sp_mmc_probe(struct udevice *dev)
{
	sp_sd_trace();
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sp_mmc_plat *plat = dev_get_platdata(dev);
	struct mmc_config *cfg = &plat->cfg;
	struct sp_mmc_host *host = dev_get_priv(dev);
	sp_mmc_hw_ops *ops;

	IFPRINTK("base addr:%p\n", host->base);
	sp_sd_trace();
	upriv->mmc = &plat->mmc;
	upriv->mmc->priv = host;

	host->dev_info = *((sp_mmc_dev_info *)dev_get_driver_data(dev));
	IFPRINTK("dev_info.id = %d\n", host->dev_info.id);
	IFPRINTK("host type: %s\n", (host->dev_info.type == SPMMC_DEVICE_TYPE_EMMC) ? "EMMC":"SD");
	IFPRINTK("version type: %s\n", (host->dev_info.version == SP_MMC_VER_Q628) ? "Q628" : "Q610");
	/* set pinmux on */
	if (host->dev_info.set_pinmux) {
		if (host->dev_info.set_pinmux(&host->dev_info)) {
			return -ENODEV;
		}
	}

	if (host->dev_info.set_clock) {
		if (host->dev_info.set_clock(&host->dev_info)) {
			return -ENODEV;
		}
	}

	if(SPMMC_DEVICE_TYPE_EMMC == host->dev_info.type) {
		cfg->host_caps	=  MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
		cfg->voltages	= MMC_VDD_32_33 | MMC_VDD_33_34;
		cfg->f_min		= SPEMMC_MIN_CLK;
		cfg->f_max		= SPEMMC_MAX_CLK;
		/* Limited by sdram_sector_#_size max value */
		cfg->b_max		= CONFIG_SYS_MMC_MAX_BLK_COUNT;
		cfg->name		= "emmc";
		ops = &emmc_hw_ops;
		/* cfg->host_caps |= MMC_MODE_DDR_52MHz; */
		host->dmapio_mode = SP_MMC_DMA_MODE;
	}
	else {
		cfg->host_caps	=  MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
		cfg->voltages	= MMC_VDD_32_33 | MMC_VDD_33_34;
		cfg->f_min		= SPMMC_MIN_CLK;
		cfg->f_max		= SPMMC_MAX_CLK;
		/* Limited by sdram_sector_#_size max value */
		cfg->b_max		= CONFIG_SYS_MMC_MAX_BLK_COUNT;
		cfg->name		= "sd";

		ops = &sd_hw_ops;
		host->dmapio_mode = SP_MMC_DMA_MODE;
	}
	host->ops = ops;
	sp_sd_trace();

	sp_sd_trace();

#ifdef CONFIG_DM_MMC
	if (ops && ops->hw_init)
		return ops->hw_init(mmc_get_mmc_dev(dev)->priv);
#endif

	return 0;
}



int sp_print_mmcinfo(struct mmc *mmc)
{
	struct sp_mmc_host *host = mmc->priv;
	printf("use %s mode\n", (host->dmapio_mode == SP_MMC_PIO_MODE) ? "PIO":"DMA" );
	return 0;
}


int sp_mmc_set_dmapio(struct mmc *mmc, uint val)
{
	struct sp_mmc_host *host = mmc->priv;
	host->dmapio_mode = val;
	return 0;
}


#define REG_BASE           0x9c000000
#define RF_GRP(_grp, _reg) ((((_grp) * 32 + (_reg)) * 4) + REG_BASE)
#define RF_MASK_V(_mask, _val)       (((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask)         (((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask)         (((_mask) << 16) | 0)


struct moon1_regs {
	unsigned int sft_cfg[32];
};
#define MOON1_REG ((volatile struct moon1_regs *)RF_GRP(1, 0))


int sd_set_pinmux(struct sp_mmc_dev_info *info)
{
	MOON1_REG->sft_cfg[1] = RF_MASK_V(1 << 6, 1 << 6);
	return 0;
}

int emmc_set_pinmux(struct sp_mmc_dev_info *info)
{
	/* disable spi nor pimux   */
	MOON1_REG->sft_cfg[1] = RF_MASK_V_CLR(0xf);
	/* disable spi nand pinmux  */
	MOON1_REG->sft_cfg[1] = RF_MASK_V_CLR(1 << 4);
	/* enable emmc pinmux */
	MOON1_REG->sft_cfg[1] = RF_MASK_V(1 << 5, 1 << 5);
	return 0;
}

static sp_mmc_dev_info q628_dev_info[] = {
	{
		.id = 0,
		.type = SPMMC_DEVICE_TYPE_EMMC,
		.version = SP_MMC_VER_Q628,
		.set_pinmux = emmc_set_pinmux,
	},

	{
		.id = 1,
		.type = SPMMC_DEVICE_TYPE_SD,
		.version = SP_MMC_VER_Q628,
		.set_pinmux = sd_set_pinmux,
	},
};


static const struct udevice_id sunplus_mmc_ids[] = {
	{
		.compatible	= "sunplus,sunplus-q628-sd",
		.data		= (ulong)&q628_dev_info[1],
	},
#ifndef CONFIG_SP_SPINAND
	{
		.compatible	= "sunplus,sunplus-q628-emmc",
		.data		= (ulong)&q628_dev_info[0],
	},
#endif
	{
	}
};


U_BOOT_DRIVER(sd_sunplus) ={
	.name						= "mmc_sunplus",
	.id							= UCLASS_MMC,
	.of_match					= sunplus_mmc_ids,
	.ofdata_to_platdata			= sp_mmc_ofdata_to_platdata,
	.platdata_auto_alloc_size	= sizeof(struct sp_mmc_plat),
	.priv_auto_alloc_size		= sizeof(struct sp_mmc_host),
	.bind						= sp_mmc_bind,
	.probe						= sp_mmc_probe,
	.ops						= &sp_mmc_ops,
};
