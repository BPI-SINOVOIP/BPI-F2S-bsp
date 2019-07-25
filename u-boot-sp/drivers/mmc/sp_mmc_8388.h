#ifndef __SPMMC_H__
#define __SPMMC_H__
#include "sp_mmc_regs_8388.h"

#ifdef CONFIG_MMC_SP_EMMC
#define EMMC_SLOT_ID            0
#else
#define EMMC_SLOT_ID            (-1)
#endif

#define SPMMC_DEVICE_TYPE_EMMC	0
#define SPMMC_DEVICE_TYPE_SD	1

#define SPMMC_SUCCESS			0x00000000
#define SPMMC_FAIL				0x00000001
#define SPMMC_RSP_TIMEOUT		0x00000002
#define SPMMC_CRC_ERROR			0x00000004
#define SPMMC_COUNTER_ERROR		0x00000008
#define SPMMC_DMA_FAIL			0x00000010
#define SPMMC_TIMEOUT			0x00000020
#define SPMMC_CRC_TIMEOUT		0x00000040
#define SPMMC_CARD_NOT_EXIST	0x00000080
#define SPMMC_COMPLETE_INIT		0x80000000

#define CLOCK_150K	150000
#define CLOCK_200K	200000
#define CLOCK_300K	300000
#define CLOCK_375K	375000
#define CLOCK_400K	400000
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

#define CLOCK_180M  180000000
#define CLOCK_240M  240000000
#define CLOCK_270M  270000000

typedef struct sp_mmc_dev_info {
	uint	id;
	uint	type;	
	uint	version;
#define SP_MMC_VER_Q610	 1
#define SP_MMC_VER_Q628	 2

	int (*set_clock)(struct sp_mmc_dev_info *dev); 
	int (*set_pinmux)(struct sp_mmc_dev_info *info);
} sp_mmc_dev_info;

typedef struct sp_mmc_host {
	union {
		volatile SDREG		*base;
		volatile EMMCREG	*ebase;
	};

	void				*ops;
	struct mmc_config	cfg;
	uint				rddly;
	uint				wrdly;
	sp_mmc_dev_info		dev_info;
} sp_mmc_host;

typedef struct sp_mmc_hw_ops {
	int	 (*hw_init)			(sp_mmc_host *host);
	int (*set_clock)		(sp_mmc_host *host, uint div);
	int (*highspeed_en)		(sp_mmc_host *host, bool en);
	int (*tunel_read_dly)	(sp_mmc_host *host, uint dly);
	int (*tunel_write_dly) 	(sp_mmc_host *host, uint dly);
	int (*tunel_clock_dly) 	(sp_mmc_host *host, uint dly);
	int (*set_bus_width)	(sp_mmc_host *host, uint bus_width);
	int (*set_bus_timing)	(sp_mmc_host *host, uint timing);
	int (*set_cmd)			(sp_mmc_host *host, struct mmc_cmd *cmd);
	int (*get_response)		(sp_mmc_host *host, struct mmc_cmd *cmd);
	int (*set_data_info)	(sp_mmc_host *host, struct mmc_cmd *cmd, struct mmc_data *data);
	int (*trigger)			(sp_mmc_host *host);
	int (*reset_mmc)		(sp_mmc_host *host);
	int (*reset_dma)		(sp_mmc_host *host);
	int (*reset_sdio)		(sp_mmc_host *host);
	int (*wait_data_timeout)(sp_mmc_host *host, uint timeout);
	int (*check_finish)		(sp_mmc_host *host); /* chech whether transfer is finish */
	int	(*tx_dummy)			(sp_mmc_host *host);
	int (*check_error)		(sp_mmc_host *host, bool with_data);
} sp_mmc_hw_ops;

struct sp_mmc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

#define SD_PAGE_NUM_SET(base, x)		(base->sd_page_num = ((x)-1))

#define DMASIZE_SET(base, x)			(base->dmasize = ((x)-1))
#define SDDATALEN_SET(base, x)			(base->sddatalen = ((x)-1))

#define SET_HW_DMA_BASE_ADDR(base, addr)  do{ \
	base->dma_base_addr_low = ((u32)(addr) & 0xffff); \
	base->dma_base_addr_high =  (((u32)(addr) >> 16) & 0xffff); \
}while(0)

#define BLOCK0_DMA_PARA_SET(base, pageIdx, nrPages)  do { \
	base->block0_hw_page_addr_low = ((pageIdx) & 0x0000ffff); \
	base->block0_hw_page_addr_high = (((pageIdx) >> 16) & 0x0000ffff); \
	base->dma_hw_page_num[0] = ((nrPages) - 1); \
}while(0)

#define SD_RST_seq(base) \
do { \
	base->sdrst = 1; \
	base->sdcrcrst = 1; \
	base->stop_dma_flag = 1; \
	base->hw_dma_rst = 1; \
	base->dmaidle = SP_DMA_NORMAL_STATE; \
	base->dmaidle = SP_RESET_DMA_OPERATION; \
	base->dmaidle = SP_DMA_NORMAL_STATE; \
}while(0)



#define SP_MMC_ASSERT(con) \
do { \
	int ret = (con); \
	if (ret) { \
		return ret; \
	} \
} while (0)

#endif /* #ifndef __SPMMC_H__ */
