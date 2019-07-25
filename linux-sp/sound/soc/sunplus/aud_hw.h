#ifndef __AUD_HW_H__
#define __AUD_HW_H__

//#include "types.h"
#include "spsoc_util.h"
#include "audclk.h"


#define	AUD_SET_EXT_DAC_XCK(x)		HWREG_W(aud_ext_dac_xck_cfg, (x))
#define	AUD_SET_EXT_DAC_BCK(x)		HWREG_W(aud_ext_dac_bck_cfg, (x))
#define	AUD_SET_INT_DAC_XCK(x)		HWREG_W(aud_int_dac_xck_cfg, (x))
#define	AUD_SET_INT_DAC_BCK(x)		HWREG_W(aud_int_dac_bck_cfg, (x))
#define	AUD_SET_IEC0_BCLK(x)		HWREG_W(aud_iec0_bclk_cfg, (x))
#define	AUD_SET_IEC1_BCLK(x)		WREG_W(aud_iec1_bclk_cfg, (x))
#define	AUD_SET_EXT_ADC_XCK(x)		HWREG_W(aud_ext_adc_xck_cfg, (x))
#define	AUD_SET_EXT_ADC_BCK(x)		HWREG_W(aud_ext_adc_bck_cfg, (x))
#define	AUD_SET_INT_ADC_XCK(x)		HWREG_W(aud_int_adc_xck_cfg, (x))
#define	AUD_SET_HDMI_TX_XCK(x)		HWREG_W(aud_hdmi_tx_mclk, (x))
#define	AUD_SET_HDMI_TX_BCK(x)		HWREG_W(aud_hdmi_tx_bclk, (x))

#define	AUD_GET_EXT_DAC_XCK()		(HWREG_R(aud_ext_dac_xck_cfg))
#define	AUD_GET_EXT_DAC_BCK()		(HWREG_R(aud_ext_dac_bck_cfg))
#define	AUD_GET_INT_DAC_XCK()		(HWREG_R(aud_int_dac_xck_cfg))
#define	AUD_GET_INT_DAC_BCK()		(HWREG_R(aud_int_dac_bck_cfg))
#define	AUD_GET_EXT_ADC_XCK()		(HWREG_R(aud_ext_adc_xck_cfg))
#define	AUD_GET_EXT_ADC_BCK()		(HWREG_R(aud_ext_adc_bck_cfg))

#define	AUD_ENABLE_XCK_CLK()		HWREG_W(aud_ext_dac_xck_cfg, HWREG_R(aud_ext_dac_xck_cfg)|AUDCLK_XCK_ENABLE)
#define	AUD_DISABLE_XCK_CLK()		HWREG_W(aud_ext_dac_xck_cfg, HWREG_R(aud_ext_dac_xck_cfg)&(~AUDCLK_XCK_ENABLE))


typedef enum _AUD_ChannelIdx_e
{
	CHANNEL_LRF 	= (0x1<<0),		/*!< Front Left */
//	CHANNEL_RF			= (0x1<<1),		/*!< Front Right */
	CHANNEL_LRS 	= (0x1<<2),		/*!< Surround Left */
//	CHANNEL_RS 	= (0x1<<3),		/*!< Surround Right */
	CHANNEL_CENTER 	= (0x1<<4),		/*!< Center */
	CHANNEL_LEF 	= (0x1<<5),		/*!< Subwoofer */
	CHANNEL_LINK 	= 0xffff,		/*!< 5.1ch link */
} AUD_ChannelIdx_e;

typedef enum _AUD_FIFOIdx_e
{
	FIFO_A0 = 0,		//Lf Rf
	FIFO_A1 = 1,		//Ls Rs
	FIFO_A2 = 2,		//C Lfe
	FIFO_A3 = 3,		//Lb Rb
	FIFO_A4 = 4,		//Lm Rm
	FIFO_A5 = 5,		//spdif
	FIFO_A6 = 6,		//hdmi
	FIFO_PCM_LINK = 0xffff,
} AUD_FIFOIdx_e;

INT32 AUD_Set_DacAnalogGain( AUD_ChannelIdx_e tag, int pgaGain);
void AUDHW_Set_PLL(void);
void AUDHW_pin_mx(void);
void AUDHW_clk_cfg(void);
void AUDHW_Mixer_Setting(void);
void AUDHW_int_dac_adc_Setting(void);
void AUDHW_SystemInit(void);
void AUD_hw_free(void);
void snd_aud_config( void);
void AUD_Set_PLL(unsigned int SAMPLE_RATE);
#if 0
void aud_clk_cfg(unsigned int SAMPLE_RATE);
#endif
void internal_dac_setting( unsigned int fs );
void pcm_format_setting( void );
void plla_clk_setting(int index);
void AUDHW_Init_PLLA(void);

#endif

