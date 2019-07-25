/*
 * The TDM driver for Sunplus QAC628
 *
 * Copyright (C) 2018 Sunplus Technology Inc., All rights reseerved.
 *
 * Author: Sunplus
 *
 * This program is free software; you can redistribute is and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Note that this is a complete rewrite of ZTE driver code.
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/vmalloc.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/fs.h>

#include "spsoc_pcm.h"
#include "spsoc_util.h"
#include "aud_hw.h"
#include "spdif.h"


// PLLA Registers
#define PLLACTL0                          0x001C
#define PLLACTL1                          0x0020
#define PLLACTL2                          0x0024
#define PLLACTL3                          0x0028
#define PLLACTL4                          0x002C


// Audio Registers
//#define Audio_Interface_Control           0x0000
#define FIFO_RST_B                        BIT(0)

//#define Audio_FIFO_Request_Enable         0x0004
//#define TDM_PDM_IN_ENABLE                 BIT(12)
//#define PCM_ENABLE                        BIT(0)
//#define PCM_ENABLE                        BIT(0)

//#define Audio_DRAM_Base_Address_Offset    0x00D0
//#define Audio_Inc_0                       0x00D4
//#define Audio_Delta_0                     0x00D8

//#define Audio_FIFO_Enable                 0x00DC
#define TDM_PDM_RX3                       BIT(25)
#define TDM_PDM_RX2                       BIT(24)
#define TDM_PDM_RX1                       BIT(23)
#define TDM_PDM_RX0                       BIT(22)
#define Main_PCM5                         BIT(20)
#define Main_PCM4                         BIT(4)
#define Main_PCM3                         BIT(3)
#define Main_PCM2                         BIT(2)
#define Main_PCM1                         BIT(1)
#define Main_PCM0                         BIT(0)

//#define Host_FIFO_Reset                   0x00E8
#define HOST_FIFO_RST                     GENMASK(29, 0)
#define HOST_FIFO_RST_SHIFT               0

//#define AUD_A22_Base                      0x0360
//#define AUD_A22_Length                    0x0364
//#define AUD_A23_Base                      0x0370
//#define AUD_A23_Length                    0x0374
//#define AUD_A23_1                         0x0378
//#define AUD_A23_2                         0x037C
//#define AUD_A24_Base                      0x0580
//#define AUD_A24_Length                    0x0584
//#define AUD_A25_Base                      0x0590
//#define AUD_A25_Length                    0x0594

//#define TDM_RXCFG0                        0x0600
#define RX_CFG_EDGE                       BIT(12)       // send data at rising edge, sample data at falling edge
#define TDM_RX_SLAVE_ENABLE               BIT(8)
#define RX_PATH0_PATH1_SELECT             BIT(4)
#define TDM_RX_ENABLE                     BIT(0)

//#define TDM_RXCFG1                        0x0604
#define RX_SLOT_DATA_DELAY_MASK           GENMASK(21, 20)
#define RX_SLOT_DATA_DELAY_SHIFT          20
#define RX_LSB_FIRST_EN                   BIT(8)
#define RX_WORD_R_JUSTIFY                 BIT(8)
#define RX_SLOT_R_JUSTIFY                 BIT(4)
#define RX_DOUBLE_DATA_ENABLE             BIT(0)

//#define TDM_RXCFG2                        0x0608
#define RX_FSYNC_HI_WIDTH_MASK            GENMASK(25, 16)
#define RX_FSYNC_HI_WIDTH_SHIFT           16
#define RX_INCR_FSYNC_BY_HALF_CYC         BIT(12)
#define RX_FSYNC_SUPPRESS_EN              BIT(8)
#define RX_FSYNC_START_AS_LOW_EN          BIT(4)
#define RX_FSYNC_HALF_PERIOD_EN           BIT(0)

//#define TDM_RXCFG3                        0x060C
#define RX_BIT_PER_WORD_MASK              GENMASK(17, 12)
#define RX_BIT_PER_WORD_SHIFT             12
#define RX_BIT_PER_SLOT_SEL_MASK          GENMASK(9, 8)
#define RX_BIT_PER_SLOT_SEL_SHIFT         8
#define RX_TOL_CH_NUM_MASK                GENMASK(4, 0)
#define RX_TOL_CH_NUM_SHIFT               0

//#define TDM_RX_TOTAL_SLOT_NUMBER          0x0610
#define TOL_SLOT_PER_FRM_RXPATH1_MASK     GENMASK(13, 8)
#define TOL_SLOT_PER_FRM_RXPATH1_SHIFT    8
#define TOL_SLOT_PER_FRM_RXPATH0_MASK     GENMASK(5, 0)
#define TOL_SLOT_PER_FRM_RXPATH0_SHIFT    0

//#define TDM_RX_ACTIVE_SLOT_NUMBER         0x0614
#define ACT_SLOT_PER_FRM_RXPATH1_MASK     GENMASK(13, 8)
#define ACT_SLOT_PER_FRM_RXPATH1_SHIFT    8
#define ACT_SLOT_PER_FRM_RXPATH0_MASK     GENMASK(5, 0)
#define ACT_SLOT_PER_FRM_RXPATH0_SHIFT    0

//#define TDM_TXCFG0                        0x0618
#define TDM_TX_MUTE_ENABLE                BIT(16)
#define TX_CFG_EDGE                       BIT(12)       // send data at rising edge, sample data at falling edge
#define TDM_TX_SLAVE_ENABLE               BIT(8)
#define TX_PATH_SELECT_MASK               GENMASK(5, 4)
#define TX_PATH_SELECT_SHIFT              4
#define TDM_TX_ENABLE                     BIT(0)

//#define TDM_TXCFG1                        0x061C
#define TX_SLOT_DATA_DELAY_MASK           GENMASK(21, 20)
#define TX_SLOT_DATA_DELAY_SHIFT          20
#define TX_LSB_FIRST_EN                   BIT(8)
#define TX_WORD_R_JUSTIFY                 BIT(8)
#define TX_SLOT_R_JUSTIFY                 BIT(4)
#define TX_DOUBLE_DATA_ENABLE             BIT(0)

//#define TDM_TXCFG2                        0x0620
#define TX_FSYNC_HI_WIDTH_MASK            GENMASK(25, 16)
#define TX_FSYNC_HI_WIDTH_SHIFT           16
#define TX_INCR_FSYNC_BY_HALF_CYC         BIT(12)
#define TX_FSYNC_SUPPRESS_EN              BIT(8)
#define TX_FSYNC_START_AS_LOW_EN          BIT(4)
#define TX_FSYNC_HALF_PERIOD_EN           BIT(0)

//#define TDM_TXCFG3                        0x0624
#define TX_BIT_PER_WORD_MASK              GENMASK(17, 12)
#define TX_BIT_PER_WORD_SHIFT             12
#define TX_BIT_PER_SLOT_SEL_MASK          GENMASK(9, 8)
#define TX_BIT_PER_SLOT_SEL_SHIFT         8
#define TX_TOL_CH_NUM_MASK                GENMASK(4, 0)
#define TX_TOL_CH_NUM_SHIFT               0

//#define TDM_TXCFG4                        0x0628
#define TOL_SLOT_PER_FRM_TXPATH2_MASK     GENMASK(21, 16)
#define TOL_SLOT_PER_FRM_TXPATH2_SHIFT    16
#define TOL_SLOT_PER_FRM_TXPATH1_MASK     GENMASK(13, 8)
#define TOL_SLOT_PER_FRM_TXPATH1_SHIFT    8
#define TOL_SLOT_PER_FRM_TXPATH0_MASK     GENMASK(5, 0)
#define TOL_SLOT_PER_FRM_TXPATH0_SHIFT    0

//#define TDM_TX_ACTIVE_SLOT_NUMBER         0x062C
#define ACT_SLOT_PER_FRM_TXPATH2_MASK     GENMASK(21, 16)
#define ACT_SLOT_PER_FRM_TXPATH2_SHIFT    16
#define ACT_SLOT_PER_FRM_TXPATH1_MASK     GENMASK(13, 8)
#define ACT_SLOT_PER_FRM_TXPATH1_SHIFT    8
#define ACT_SLOT_PER_FRM_TXPATH0_MASK     GENMASK(5, 0)
#define ACT_SLOT_PER_FRM_TXPATH0_SHIFT    0

//#define TDM_TX_MUTE_FLAG                  0x0630
#define TDM_TX_Mute_Flag                  BIT(0)

//#define TDM_TX_XCK_CFG                    0x0658
//#define TDM_TX_BCK_CFG                    0x065C
//#define TDM_RX_XCK_CFG                    0x0660
//#define TDM_RX_BCK_CFG                    0x0664    //G72.25
//#define TDM_PDM_TX_SEL                    0x0678


struct sp_tdm_info {
    	struct snd_dmaengine_dai_dma_data   dma_playback;
    	struct snd_dmaengine_dai_dma_data   dma_capture;
    	resource_size_t     phy_addr;
    	resource_size_t     vir_addr;
    	void __iomem        *reg_base;
    	void __iomem        *plla_reg_base;
    	int                 master;
    	struct device       *dev;
};

void aud_tdm_clk_cfg(int pll_id, int source, unsigned int SAMPLE_RATE)
{
    	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;//(volatile RegisterFile_Audio*)REG(60,0);
        int capture = (source == SNDRV_PCM_STREAM_CAPTURE);
    	AUD_INFO("%s rate %d capture %d\n", __func__, SAMPLE_RATE, capture);
        //147M settings
    	if (SAMPLE_RATE == 32000) {
    		if (capture){
    			regs0->tdm_rx_xck_cfg = 0x6981;
    	  		regs0->tdm_rx_bck_cfg = 0x6001;
    		}else{
    			regs0->tdm_tx_xck_cfg = 0x6981;
    	  		regs0->tdm_tx_bck_cfg = 0x6001;
    		}    	  	    	  	
    	}else if((SAMPLE_RATE == 44100) || (SAMPLE_RATE == 64000) || (SAMPLE_RATE == 48000)) {
    		if (capture){
    			regs0->tdm_rx_xck_cfg = 0x6883;
    	  		regs0->tdm_rx_bck_cfg = 0x6001;
    		}else{
    			regs0->tdm_tx_xck_cfg = 0x6883;
    	  		regs0->tdm_tx_bck_cfg = 0x6001;
    	  	
    	  		regs0->aud_ext_dac_xck_cfg  = 0x6883;   //PLLA, 256FS //??? need to check
    	  		regs0->aud_ext_dac_bck_cfg  = 0x6003;   //64FS //??? need to check
    		}        	
    	}else if((SAMPLE_RATE == 88200) || (SAMPLE_RATE == 96000) || (SAMPLE_RATE == 128000)) {
    		if (capture){
    			regs0->tdm_rx_xck_cfg = 0x6881;
    	  		regs0->tdm_rx_bck_cfg = 0x6001; 
    		}else{
    			regs0->tdm_tx_xck_cfg = 0x6881;
    	  		regs0->tdm_tx_bck_cfg = 0x6001;
    		}    	  	       
    	}else if((SAMPLE_RATE == 176400) || (SAMPLE_RATE == 192000)) {
    		if (capture){
    			regs0->tdm_rx_xck_cfg = 0x6881;
    	  		regs0->tdm_rx_bck_cfg = 0x6000;
    		}else{
    			regs0->tdm_tx_xck_cfg = 0x6881;
    	  		regs0->tdm_tx_bck_cfg = 0x6001;
    		}    	  	    	  	
    	}else{
    	  	regs0->tdm_tx_xck_cfg = 0;
    	  	regs0->tdm_tx_bck_cfg = 0;
    	  	regs0->tdm_rx_xck_cfg = 0;
    	  	regs0->tdm_rx_bck_cfg = 0;
    	  	regs0->aud_ext_dac_xck_cfg  = 0;   //PLLA, 256FS //??? need to check
    	  	regs0->aud_ext_dac_bck_cfg  = 0;   //64FS //??? need to check
    	}    	
}

static void sp_tdm_tx_en(bool on)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
    	unsigned long val;

    	val =  regs0->tdm_tx_cfg0;
    	if (on){
        	val |= TDM_TX_ENABLE;
        	regs0->tdmpdm_tx_sel = 0x01;
    	}else
        	val &= ~(TDM_TX_ENABLE);
    	regs0->tdm_tx_cfg0 = val;
    
    	AUD_INFO("tdm_tx_cfg0 0x%x\n", regs0->tdm_tx_cfg0);
}

static void sp_tdm_rx_en(bool on)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
    	unsigned long val;

    	val = regs0->tdm_rx_cfg0;
    	if (on){
        	val |= TDM_RX_ENABLE;
        	regs0->tdmpdm_tx_sel = 0x01;
    	}else
        	val &= ~(TDM_RX_ENABLE);
    	regs0->tdm_rx_cfg0 = val;
    
    	AUD_INFO("tdm_rx_cfg0 0x%x\n", regs0->tdm_rx_cfg0);
}

static void sp_tdm_tx_dma_en(bool on)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
    	unsigned long val;

    	val = regs0->aud_fifo_enable;
    	if (on){
    	  	if ((val&(Main_PCM5 | Main_PCM4 | Main_PCM3 | Main_PCM2 | Main_PCM1 | Main_PCM0)) != 0)
    	  	  	return;
        	val |= (Main_PCM5 | Main_PCM4 | Main_PCM3 | Main_PCM2 | Main_PCM1 | Main_PCM0);
    	}else
        	val &= ~(Main_PCM5 | Main_PCM4 | Main_PCM3 | Main_PCM2 | Main_PCM1 | Main_PCM0);
    	regs0->aud_fifo_enable = val;
    
    	AUD_INFO("aud_fifo_enable 0x%x\n", regs0->aud_fifo_enable);
#if 1
    	if (on){
    	  	//val = (Main_PCM5 | Main_PCM4 | Main_PCM3 | Main_PCM2 | Main_PCM1 | Main_PCM0);
        	regs0->aud_fifo_reset = val;
        	//AUD_INFO("aud_fifo_reset 0x%x\n", val);
        	while((regs0->aud_fifo_reset&val));
    	}
#endif
    	val = regs0->aud_enable;
    	if (on)
        	val |= aud_enable_i2stdm_p;
    	else
        	val &= (~aud_enable_i2stdm_p);
    	regs0->aud_enable = val;
    
    	AUD_INFO("aud_enable 0x%x\n", regs0->aud_enable);
}

static void sp_tdm_rx_dma_en(bool on)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
    	unsigned long val;

    	val = regs0->aud_fifo_enable;
    	if (on)
        	val |= (TDM_PDM_RX3 | TDM_PDM_RX2 | TDM_PDM_RX1 | TDM_PDM_RX0);
    	else
        	val &= ~(TDM_PDM_RX3 | TDM_PDM_RX2 | TDM_PDM_RX1 | TDM_PDM_RX0);
    	regs0->aud_fifo_enable = val;
    
    	AUD_INFO("aud_fifo_enable 0x%x\n", regs0->aud_fifo_enable);
#if 1
    	if (on){      
        	//val = (TDM_PDM_RX3 | TDM_PDM_RX2 | TDM_PDM_RX1 | TDM_PDM_RX0);
        	regs0->aud_fifo_reset = val;
        
        	//AUD_INFO("aud_fifo_reset 0x%x\n", regs0->aud_fifo_reset);
        	while((regs0->aud_fifo_reset&val));               
    	}
#endif
    	val = regs0->aud_enable;
    	if (on)
        	val |= aud_enable_tdmpdm_c;
    	else
        	val &= (~aud_enable_tdmpdm_c);
    	regs0->aud_enable = val;
    
    	AUD_INFO("aud_enable 0x%x\n", regs0->aud_enable);
}

#define SP_TDM_RATES    SNDRV_PCM_RATE_44100//(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)

#define SP_TDM_FMTBIT \
        (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FORMAT_MU_LAW | \
         SNDRV_PCM_FORMAT_A_LAW)

static int sp_tdm_dai_probe(struct snd_soc_dai *dai)
{
    	struct sp_tdm_info *sp_tdm = dev_get_drvdata(dai->dev);

    	AUD_INFO("%s IN\n", __func__ );

    	snd_soc_dai_set_drvdata(dai, sp_tdm);
    	sp_tdm->phy_addr = aud_param.fifoInfo.pcmtx_physAddrBase;
    	sp_tdm->dma_playback.addr = aud_param.fifoInfo.pcmtx_physAddrBase;
    	sp_tdm->dma_playback.maxburst = 16;
    	sp_tdm->dma_capture.addr = aud_param.fifoInfo.mic_physAddrBase;
    	sp_tdm->dma_capture.maxburst = 16;
    	snd_soc_dai_init_dma_data(dai, &sp_tdm->dma_playback,
                              &sp_tdm->dma_capture);
    	AUD_INFO("%s, phy_addr=%08x\n", __func__, sp_tdm->phy_addr);
    	return 0;
}

static int sp_tdm_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
    	struct sp_tdm_info *tdm = snd_soc_dai_get_drvdata(cpu_dai);
    	unsigned long val;

    	AUD_INFO("%s IN, fmt=0x%x\n", __func__, fmt);

    	switch (fmt) {
    		case SND_SOC_DAIFMT_CBM_CFM: // TX/RX master
        		tdm->master = 1;
        		val = regs0->tdm_tx_cfg0;

        		val = regs0->tdm_rx_cfg0;
        		val |= RX_PATH0_PATH1_SELECT;
        		val &= ~TDM_RX_SLAVE_ENABLE;
        		regs0->tdm_rx_cfg0 = val;
        		break;

    		case SND_SOC_DAIFMT_CBS_CFS: // TX/RX slave
        		tdm->master = 0;
        		val = regs0->tdm_tx_cfg0;

        		val = regs0->tdm_rx_cfg0;
        		val |= (TDM_RX_SLAVE_ENABLE | RX_PATH0_PATH1_SELECT);
        		regs0->tdm_rx_cfg0 = val;
        		break;

    		default:
        		dev_err(cpu_dai->dev, "Unknown master/slave format\n");
        		return -EINVAL;
    	}
    	AUD_INFO("master tdm_rx_cfg0 0x%x\n", val);
    	return 0;
}

static int sp_tdm_hw_params(struct snd_pcm_substream *substream,
                            struct snd_pcm_hw_params *params,
                            struct snd_soc_dai *socdai)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
	struct snd_pcm_runtime *runtime = substream->runtime;
    	struct sp_tdm_info *tdm = snd_soc_dai_get_drvdata(socdai);
    	struct snd_dmaengine_dai_dma_data *dma_data;
    	unsigned int ts_width;
    	unsigned int wd_width;
    	unsigned int ch_num = 32;
    	unsigned int ret = 0;
    	unsigned long val;

    	AUD_INFO("%s IN\n", __func__ );
    	AUD_INFO("%s, stream=%d, tdm->dma_capture=0x%08x\n", __func__, substream->stream, tdm->dma_capture);
    
    	dma_data = snd_soc_dai_get_dma_data(socdai, substream);
    	dma_data->addr_width = ch_num >> 3;
    	ch_num = params_channels(params);
    	AUD_INFO("%s, params=%x\n", __func__, params_format(params));
    	AUD_INFO("AUD_FORMATS=0x%x\n", (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE));

    	switch (params_format(params)) {
    		case SNDRV_PCM_FORMAT_S8:
        		wd_width = 8;
        		break;
    		case SNDRV_PCM_FORMAT_S24_3LE:
    		case SNDRV_PCM_FORMAT_S24_LE:
    		case SNDRV_PCM_FORMAT_S24_3BE:
        		wd_width = 24;
        		break;
    		case SNDRV_PCM_FORMAT_S16_LE:
        		wd_width = 16;
        		break;
    		case SNDRV_PCM_FORMAT_S32_LE:
        		wd_width = 24;
        		break;
    		case SNDRV_PCM_FORMAT_MU_LAW:
    		case SNDRV_PCM_FORMAT_A_LAW:
    		default:
        		ts_width = 0x00;
        		dev_err(socdai->dev, "Unknown data format\n");
        		return -EINVAL;
    	}

    	AUD_INFO("ts_width=%d\n", runtime->frame_bits);
    	//wd_width = 0x14;
    	ts_width = 0;
    	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
        	regs0->tdm_rx_cfg1 = 0x00100000;// slot delay = 1T
        	regs0->tdm_rx_cfg2 = 0x00010000;// FSYNC_HI_WIDTH = 1
        	val = (wd_width<<12)|(ts_width<<8)| ch_num;// bit# per word = 20, bit# per slot = 24, slot# per frame = 8
        	//val = 0x10008;
        	regs0->tdm_rx_cfg3 = val;
        
        	AUD_INFO("tdm_rx_cfg1 0x%x\n", regs0->tdm_rx_cfg1);        
       		AUD_INFO("tdm_rx_cfg2 0x%x\n", regs0->tdm_rx_cfg2);        
        	AUD_INFO("tdm_rx_cfg3 0x%x\n", regs0->tdm_rx_cfg3);
    	}else{
    	  	regs0->tdm_tx_cfg1 = 0x00000000;// slot delay = 1T
        	regs0->tdm_tx_cfg2 = 0x00010000;// FSYNC_HI_WIDTH = 1
        	val = (wd_width<<12)|(ts_width<<8)|ch_num;// bit# per word = 20, bit# per slot = 24, slot# per frame = 8
        	//val = 0x10008;
        	regs0->tdm_tx_cfg3 = val;
        
        	AUD_INFO("tdm_tx_cfg1 0x%x\n", regs0->tdm_tx_cfg1);        
        	AUD_INFO("tdm_tx_cfg2 0x%x\n", regs0->tdm_tx_cfg2);        
        	AUD_INFO("tdm_tx_cfg3 0x%x\n", regs0->tdm_tx_cfg3);
        	sp_tdm_tx_dma_en(true);//Need to add here
    	}

    	return ret;
}

static int sp_tdm_trigger(struct snd_pcm_substream *substream, int cmd,
                          struct snd_soc_dai *dai)
{
    	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);
    	struct sp_tdm_info *sp_tdm = dev_get_drvdata(dai->dev);
    	//unsigned int val;
    	int ret = 0;

    	AUD_INFO("%s IN, cmd=%d, capture=%d\n", __func__, cmd, capture);

    	switch (cmd) {
    		case SNDRV_PCM_TRIGGER_START:
        		if (capture) {
            			//val = sp_tdm_readl(sp_tdm, Host_FIFO_Reset);
            			//val |= (TDM_PDM_RX3 | TDM_PDM_RX2 | TDM_PDM_RX1 | TDM_PDM_RX0);
            			//sp_tdm_writel(sp_tdm, Host_FIFO_Reset, val);
         
            			sp_tdm_rx_dma_en(true);
        		} //else {
            			//val = sp_tdm_readl(sp_tdm, Host_FIFO_Reset);
            			//val |= Main_PCM5;
            			//sp_tdm_writel(sp_tdm, Host_FIFO_Reset, val);

            		//	sp_tdm_tx_dma_en(true);
        		//}
        		break;

    		case SNDRV_PCM_TRIGGER_RESUME:
    		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        		if (capture)
            			sp_tdm_rx_dma_en(true);
        		//else
            		//	sp_tdm_tx_dma_en(true);
        		break;

    		case SNDRV_PCM_TRIGGER_STOP:
        		if (capture)
            			sp_tdm_rx_dma_en(false);
        		//else
            		//	sp_tdm_tx_dma_en(false);
        		break;

    		case SNDRV_PCM_TRIGGER_SUSPEND:
    		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        		if (capture)
            			sp_tdm_rx_dma_en(false);
        		//else
            		//	sp_tdm_tx_dma_en(false);
        		break;

    		default:
        		ret = -EINVAL;
        		break;
    	}
    
    	return ret;
}

static int sp_tdm_startup(struct snd_pcm_substream *substream,
                          struct snd_soc_dai *dai)
{
    	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);
    	struct sp_tdm_info *sp_tdm = dev_get_drvdata(dai->dev);
    	//int ret;

    	AUD_INFO("%s IN, operation c or p %d\n", __func__, capture);

    	//aud_tdm_clk_cfg(41100);
    	if (capture)
        	sp_tdm_rx_en(true);
    	else
        	sp_tdm_tx_en(true);

    	return 0;
}

static void sp_tdm_shutdown(struct snd_pcm_substream *substream,
                            struct snd_soc_dai *dai)
{
    	struct sp_tdm_info *sp_tdm = dev_get_drvdata(dai->dev);
    	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);

    	AUD_INFO("%s IN\n", __func__ );
    	if (capture)
        	sp_tdm_rx_en(false);
    	else
        	sp_tdm_tx_en(false);
    	aud_tdm_clk_cfg(0, 0, 0);
}

static int sp_tdm_set_pll(struct snd_soc_dai *dai, int pll_id, int source,unsigned int freq_in, unsigned int freq_out)
{
    	struct sp_tdm_info *sp_tdm = dev_get_drvdata(dai->dev);

    	AUD_INFO("%s IN, freq_out=%d\n", __func__, freq_out);

    	aud_tdm_clk_cfg(pll_id, source, freq_out);
    	return 0;
}

static struct snd_soc_dai_ops sp_tdm_dai_ops = {
    	.trigger	= sp_tdm_trigger,
    	.hw_params  	= sp_tdm_hw_params,
    	.set_fmt    	= sp_tdm_set_fmt,
    	.set_pll    	= sp_tdm_set_pll,
    	.startup    	= sp_tdm_startup,
    	.shutdown  	= sp_tdm_shutdown,
};

static const struct snd_soc_component_driver sp_tdm_component = {
    	.name       = "spsoc-tdm-driver-dai",
};

static void sp_tdm_init_state(struct sp_tdm_info *tdm)
{
	volatile RegisterFile_Audio * regs0 = (volatile RegisterFile_Audio*)audio_base;
	int val;
	  
    	regs0->tdm_rx_cfg0 	= (0x0<<12);
    	regs0->tdm_rx_cfg1 	= 0x00100000;
    	regs0->tdm_rx_cfg2 	= 0x00010000;
    	regs0->tdm_rx_cfg3 	= 0x00014008;
    	regs0->tdm_tx_cfg0 	= (0x2<<4);
    	regs0->tdm_tx_cfg1 	= 0x00100000;
    	regs0->tdm_tx_cfg2 	= 0x00010000;
    	regs0->tdm_tx_cfg3 	= 0x0001400C;
    	regs0->tdmpdm_tx_sel 	= 0x01;
    
}

#define AUD_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S24_3BE )
static struct snd_soc_dai_driver sp_tdm_dai = {
    	.name   = "spsoc-tdm-driver-dai",
    	//.id   = 0,
    	.probe	= sp_tdm_dai_probe,
    	.capture= {
        	.channels_min 	= 2,
        	.channels_max 	= 8,
        	.rates		= AUD_RATES_C,//SP_TDM_RATES,  //AUD_RATES_C
        	.formats      	= AUD_FORMATS,//SP_TDM_FMTBIT, //AUD_FORMATS
    	},
    	.playback	= {
        	.channels_min 	= 2,
        	.channels_max 	= 12,
        	.rates        	= AUD_RATES_C,
        	.formats      	= AUD_FORMATS,
    	},
    	.ops	= &sp_tdm_dai_ops
};

static int sp_tdm_probe(struct platform_device *pdev)
{
    	struct device *dev = &pdev->dev;
    	struct sp_tdm_info *sp_tdm;
    	int ret;

    	AUD_INFO("%s IN\n", __func__);

    	sp_tdm = devm_kzalloc(&pdev->dev, sizeof(*sp_tdm), GFP_KERNEL);
    	if (!sp_tdm)
        	return -ENOMEM;

    	sp_tdm->dev = dev;

    	sp_tdm->reg_base = audio_base;

    	AUD_NOTICE("%s, reg_base=%08x\n", __func__, sp_tdm->reg_base);

    	sp_tdm_init_state(sp_tdm);
    	platform_set_drvdata(pdev, sp_tdm);

    	ret = devm_snd_soc_register_component(&pdev->dev,
                                              &sp_tdm_component,
                                              &sp_tdm_dai,1);
    	if (ret) {
        	dev_err(&pdev->dev, "Register DAI failed: %d\n", ret);
        	return ret;
    	}

    	return ret;
}

static struct platform_driver spsoc_tdm_driver = {
    	.probe	= sp_tdm_probe,
    	.driver = {
        	.name	= "spsoc-tdm-driver-dai",
        	.owner 	= THIS_MODULE,
    	},
};
#if 0
module_platform_driver(spsoc_tdm_driver);
#else
static struct platform_device *spsoc_tdm_device;

static int __init snd_spsoc_tdm_init(void)
{
    	int ret = 0;

    	ret = platform_driver_register(&spsoc_tdm_driver);
    	AUD_INFO("%s IN, create soc_tdm\n", __func__);

    	spsoc_tdm_device = platform_device_alloc("spsoc-tdm-driver-dai", -1);
    	if (!spsoc_tdm_device)
        	return -ENOMEM;

    	ret = platform_device_add(spsoc_tdm_device);
    	if (ret)
        	platform_device_put(spsoc_tdm_device);

    	return ret;
}
module_init(snd_spsoc_tdm_init);

static void __exit snd_spsoc_tdm_exit(void)
{
    	platform_driver_unregister(&spsoc_tdm_driver);
}
module_exit(snd_spsoc_tdm_exit);
#endif

MODULE_AUTHOR("Sunplus Technology Inc.");
MODULE_DESCRIPTION("Sunplus TDM DAI driver");
MODULE_LICENSE("GPL");
