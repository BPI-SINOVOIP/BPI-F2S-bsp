#ifndef _SPSOC_UTIL_H
#define _SPSOC_UTIL_H

#include <linux/slab.h>		/* for kmalloc and kfree */
#include <asm/io.h>		/* for virt_to_phys */
#include <linux/mm.h>
#include <mach/io_map.h>
#include "types.h"

extern void __iomem *audio_base;

/**********************************************************
 * 			BASE
 **********************************************************/
#define IIC_BASEADDR	0xe0004000
#define REG_BASEADDR	0x9C000000//0x40000000
#define DRAM_BASEADDR	0x10000000	// 0x0010_0000 ~ 0x3fff_ffff
#define FPGA_BASEADDR	0xF8000000

#define REG(g, i)	((void __iomem *)VA_IOB_ADDR(((g) * 32 + (i)) * 4))


/**********************************************************
 *
 **********************************************************/
#define AUD_RATES	(SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_64000|\
	                 SNDRV_PCM_RATE_64000|SNDRV_PCM_RATE_88200|SNDRV_PCM_RATE_96000|\
	                 SNDRV_PCM_RATE_176400|SNDRV_PCM_RATE_192000)

#define AUD_RATES_C	(SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000)


/**********************************************************
 *
 **********************************************************/
#define AUD_ERR(fmt, arg...)		printk(KERN_ERR "[alsa] " fmt, ##arg)
#define AUD_WARNING(fmt, arg...)	printk(KERN_WARNING "[alsa] " fmt, ##arg)
#define AUD_NOTICE(fmt, arg...)	  	printk(KERN_NOTICE "[alsa] " fmt, ##arg)
#define AUD_INFO(fmt, arg...)	    	printk(KERN_INFO "[alsa] " fmt, ##arg)//((void)0)
#define AUD_DEBUG(fmt, arg...)    	((void)0)//printk(KERN_DEBUG "[alsa] " fmt, ##arg)

/**********************************************************
 *
 **********************************************************/
#define ADCP_WAIT_INIT_DONE()  \
        while( HWREG_R(adcp_init_ctrl) & 0x1000){}

/**********************************************************
 * 			Register Definition
 **********************************************************/
typedef struct
{
    	// GROUP 000 : MOON0
    	UINT32  stamp                                 ; // 00  (ADDR : 0x9C00_0000)
    	UINT32  clken0                                ; // 01  (ADDR : 0x9C00_0004)
    	UINT32  clken1                                ; // 02  (ADDR : 0x9C00_0008)
    	UINT32  clken2                                ; // 03  (ADDR : 0x9C00_000C)
    	UINT32  clken3                                ; // 04  (ADDR : 0x9C00_0010)
    	UINT32  clken4                                ; // 05  (ADDR : 0x9C00_0014)
    	UINT32  clken5                                ; // 06  (ADDR : 0x9C00_0018)
    	UINT32  clken6                                ; // 07  (ADDR : 0x9C00_001C)
    	UINT32  clken7                                ; // 08  (ADDR : 0x9C00_0020)
    	UINT32  clken8                                ; // 09  (ADDR : 0x9C00_0024)
    	UINT32  clken9                                ; // 10  (ADDR : 0x9C00_0028)
    	UINT32  gclken0                               ; // 11  (ADDR : 0x9C00_002C)
    	UINT32  gclken1                               ; // 12  (ADDR : 0x9C00_0030)
    	UINT32  gclken2                               ; // 13  (ADDR : 0x9C00_0034)
    	UINT32  gclken3                               ; // 14  (ADDR : 0x9C00_0038)
    	UINT32  gclken4                               ; // 15  (ADDR : 0x9C00_003C)
    	UINT32  gclken5                               ; // 16  (ADDR : 0x9C00_0040)
    	UINT32  gclken6                               ; // 17  (ADDR : 0x9C00_0044)
    	UINT32  gclken7                               ; // 18  (ADDR : 0x9C00_0048)
    	UINT32  gclken8                               ; // 19  (ADDR : 0x9C00_004C)
    	UINT32  gclken9                               ; // 20  (ADDR : 0x9C00_0050)
    	UINT32  reset0                                ; // 21  (ADDR : 0x9C00_0054)
    	UINT32  reset1                                ; // 22  (ADDR : 0x9C00_0058)
    	UINT32  reset2                                ; // 23  (ADDR : 0x9C00_005C)
    	UINT32  reset3                                ; // 24  (ADDR : 0x9C00_0060)
    	UINT32  reset4                                ; // 25  (ADDR : 0x9C00_0064)
    	UINT32  reset5                                ; // 26  (ADDR : 0x9C00_0068)
    	UINT32  reset6                                ; // 27  (ADDR : 0x9C00_006C)
    	UINT32  reset7                                ; // 28  (ADDR : 0x9C00_0070)
    	UINT32  reset8                                ; // 29  (ADDR : 0x9C00_0074)
    	UINT32  reset9                                ; // 30  (ADDR : 0x9C00_0078)
    	UINT32  hw_cfg                                ; // 31  (ADDR : 0x9C00_007C)
} RegisterFile_G0;

typedef struct
{
    	// Group 001 : MOON1
    	UINT32  rf_sft_cfg0                           ; // 00  (ADDR : 0x9C00_0080)
    	UINT32  rf_sft_cfg1                           ; // 01  (ADDR : 0x9C00_0084)
    	UINT32  rf_sft_cfg2                           ; // 02  (ADDR : 0x9C00_0088)
    	UINT32  rf_sft_cfg3                           ; // 03  (ADDR : 0x9C00_008C)
    	UINT32  rf_sft_cfg4                           ; // 04  (ADDR : 0x9C00_0090)
    	UINT32  rf_sft_cfg5                           ; // 05  (ADDR : 0x9C00_0094)
    	UINT32  rf_sft_cfg6                           ; // 06  (ADDR : 0x9C00_0098)
    	UINT32  rf_sft_cfg7                           ; // 07  (ADDR : 0x9C00_009C)
    	UINT32  rf_sft_cfg8                           ; // 08  (ADDR : 0x9C00_00A0)
    	UINT32  rf_sft_cfg9                           ; // 09  (ADDR : 0x9C00_00A4)
    	UINT32  rf_sft_cfg10                          ; // 10  (ADDR : 0x9C00_00A8)
    	UINT32  rf_sft_cfg11                          ; // 11  (ADDR : 0x9C00_00AC)
    	UINT32  rf_sft_cfg12                          ; // 12  (ADDR : 0x9C00_00B0)
    	UINT32  rf_sft_cfg13                          ; // 13  (ADDR : 0x9C00_00B4)
    	UINT32  rf_sft_cfg14                          ; // 14  (ADDR : 0x9C00_00B8)
    	UINT32  rf_sft_cfg15                          ; // 15  (ADDR : 0x9C00_00BC)
    	UINT32  rf_sft_cfg16                          ; // 16  (ADDR : 0x9C00_00C0)
    	UINT32  rf_sft_cfg17                          ; // 17  (ADDR : 0x9C00_00C4)
    	UINT32  rf_sft_cfg18                          ; // 18  (ADDR : 0x9C00_00C8)
    	UINT32  rf_sft_cfg19                          ; // 19  (ADDR : 0x9C00_00CC)
    	UINT32  rf_sft_cfg20                          ; // 20  (ADDR : 0x9C00_00D0)
    	UINT32  rf_sft_cfg21                          ; // 21  (ADDR : 0x9C00_00D4)
    	UINT32  rf_sft_cfg22                          ; // 22  (ADDR : 0x9C00_00D8)
    	UINT32  rf_sft_cfg23                          ; // 23  (ADDR : 0x9C00_00DC)
    	UINT32  rf_sft_cfg24                          ; // 24  (ADDR : 0x9C00_00E0)
    	UINT32  rf_sft_cfg25                          ; // 25  (ADDR : 0x9C00_00E4)
    	UINT32  rf_sft_cfg26                          ; // 26  (ADDR : 0x9C00_00E8)
    	UINT32  rf_sft_cfg27                          ; // 27  (ADDR : 0x9C00_00EC)
    	UINT32  rf_sft_cfg28                          ; // 28  (ADDR : 0x9C00_00F0)
    	UINT32  rf_sft_cfg29                          ; // 29  (ADDR : 0x9C00_00F4)
    	UINT32  rf_sft_cfg30                          ; // 30  (ADDR : 0x9C00_00F8)
    	UINT32  rf_sft_cfg31                          ; // 31  (ADDR : 0x9C00_00FC)
} RegisterFile_G1;

typedef struct
{
    	// Group 002 : Reserved
    	UINT32  G002_RESERVED[64]                     ; //     (ADDR : 0x9C00_0100) ~ (ADDR : 0x9C00_017C)
} RegisterFile_G2;

typedef struct
{
    	// Group 004 : PAD_CTL
    	UINT32  rf_pad_ctl0                           ; // 00  (ADDR : 0x9C00_0200)
    	UINT32  rf_pad_ctl1                           ; // 01  (ADDR : 0x9C00_0204)
    	UINT32  rf_pad_ctl2                           ; // 02  (ADDR : 0x9C00_0208)
    	UINT32  rf_pad_ctl3                           ; // 03  (ADDR : 0x9C00_020C)
    	UINT32  rf_pad_ctl4                           ; // 04  (ADDR : 0x9C00_0210)
    	UINT32  rf_pad_ctl5                           ; // 05  (ADDR : 0x9C00_0214)
    	UINT32  rf_pad_ctl6                           ; // 06  (ADDR : 0x9C00_0218)
    	UINT32  rf_pad_ctl7                           ; // 07  (ADDR : 0x9C00_021C)
    	UINT32  rf_pad_ctl8                           ; // 08  (ADDR : 0x9C00_0220)
    	UINT32  rf_pad_ctl9                           ; // 09  (ADDR : 0x9C00_0224)
    	UINT32  rf_pad_ctl10                          ; // 10  (ADDR : 0x9C00_0228)
    	UINT32  rf_pad_ctl11                          ; // 11  (ADDR : 0x9C00_022C)
    	UINT32  rf_pad_ctl12                          ; // 12  (ADDR : 0x9C00_0230)
    	UINT32  rf_pad_ctl13                          ; // 13  (ADDR : 0x9C00_0234)
    	UINT32  rf_pad_ctl14                          ; // 14  (ADDR : 0x9C00_0238)
    	UINT32  rf_pad_ctl15                          ; // 15  (ADDR : 0x9C00_023C)
    	UINT32  rf_pad_ctl16                          ; // 16  (ADDR : 0x9C00_0240)
    	UINT32  rf_pad_ctl17                          ; // 17  (ADDR : 0x9C00_0244)
    	UINT32  rf_pad_ctl18                          ; // 18  (ADDR : 0x9C00_0248)
    	UINT32  rf_pad_ctl19                          ; // 19  (ADDR : 0x9C00_024C)
    	UINT32  rf_pad_ctl20                          ; // 20  (ADDR : 0x9C00_0250)
    	UINT32  rf_pad_ctl21                          ; // 21  (ADDR : 0x9C00_0254)
    	UINT32  rf_pad_ctl22                          ; // 22  (ADDR : 0x9C00_0258)
    	UINT32  rf_pad_ctl23                          ; // 23  (ADDR : 0x9C00_025C)
    	UINT32  rf_pad_ctl24                          ; // 24  (ADDR : 0x9C00_0260)
    	UINT32  gpio_first[6]                         ;
    	UINT32  rf_pad_ctl31                          ; // 31  (ADDR : 0x9C00_027C)
} RegisterFile_G4;

typedef struct
{
    	UINT32  G382_RESERVED[32]                     ; //     (ADDR : 0x9C00_BF00) ~ (ADDR : 0x9C00_BF80)
} RegisterFile_G382;

typedef struct
{
    	// Group 060 : AUD
    	UINT32  audif_ctrl                            ; // 00
    	UINT32  aud_enable                            ; // 01
    	UINT32  pcm_cfg                               ; // 02
    	UINT32  i2s_mute_flag_ctrl                    ; // 03
    	UINT32  ext_adc_cfg                           ; // 04
    	UINT32  int_dac_ctrl0                         ; // 05
    	UINT32  int_adc_ctrl                          ; // 06
    	UINT32  adc_in_path_switch                    ; // 07
    	UINT32  int_adc_dac_cfg                       ; // 08
    	UINT32  G060_reserved_9                       ; // 09
    	UINT32  iec_cfg                               ; // 10
    	UINT32  iec0_valid_out                        ; // 11
    	UINT32  iec0_par0_out                         ; // 12
    	UINT32  iec0_par1_out                         ; // 13
    	UINT32  iec1_valid_out                        ; // 14
    	UINT32  iec1_par0_out                         ; // 15
    	UINT32  iec1_par1_out                         ; // 16
    	UINT32  iec0_rx_debug_info                    ; // 17
    	UINT32  iec0_valid_in                         ; // 18
    	UINT32  iec0_par0_in                          ; // 19
    	UINT32  iec0_par1_in                          ; // 20
    	UINT32  iec1_rx_debug_info                    ; // 21
    	UINT32  iec1_valid_in                         ; // 22
    	UINT32  iec1_par0_in                          ; // 23
    	UINT32  iec1_par1_in                          ; // 24
    	UINT32  iec2_rx_debug_info                    ; // 25
    	UINT32  iec2_valid_in                         ; // 26
    	UINT32  iec2_par0_in                          ; // 27
    	UINT32  iec2_par1_in                          ; // 28
    	UINT32  G060_reserved_29                      ; // 29
    	UINT32  iec_tx_user_wdata                     ; // 30
    	UINT32  iec_tx_user_ctrl                      ; // 31

    	// Group 061 : AUD
    	UINT32  adcp_ch_enable                        ; // 00, ADCPRC Configuration Group 1
    	UINT32  adcp_fubypass                         ; // 01, ADCPRC Configuration Group 2
    	UINT32  adcp_mode_ctrl                        ; // 02, ADCPRC Mode Control
    	UINT32  adcp_init_ctrl                        ; // 03, ADCP Initialization Control
    	UINT32  adcp_coeff_din                        ; // 04, Coefficient Data Input
    	UINT32  adcp_agc_cfg                          ; // 05, ADCPRC AGC Configuration of Ch0/1
    	UINT32  adcp_agc_cfg2                         ; // 06, ADCPRC AGC Configuration of Ch2/3
    	UINT32  adcp_gain_0                           ; // 07, ADCP System Gain1
    	UINT32  adcp_gain_1                           ; // 08, ADCP System Gain2
    	UINT32  adcp_gain_2                           ; // 09, ADCP System Gain3
    	UINT32  adcp_gain_3                           ; // 10, ADCP System Gain4
    	UINT32  adcp_risc_gain                        ; // 11, ADCP RISC Gain
    	UINT32  adcp_mic_l                            ; // 12, ADCPRC Microphone - in Left Channel Data
    	UINT32  adcp_mic_r                            ; // 13, ADCPRC Microphone - in Right Channel Data
    	UINT32  adcp_agc_gain                         ; // 14, ADCPRC AGC Gain
    	UINT32  G061_reserved_15                      ; // 15, Reserved
    	UINT32  aud_apt_mode                          ; // 16, Audio Playback Timer Mode
    	UINT32  aud_apt_data                          ; // 17, Audio Playback Timer
    	UINT32  aud_apt_parameter                     ; // 18, Audio Playback Timer Parameter
    	UINT32  G061_reserved_19                      ; // 19, Reserved
    	UINT32  aud_audhwya                           ; // 20, DRAM Base Address Offset
    	UINT32  aud_inc_0                             ; // 21, DMA Counter Increment/Decrement
    	UINT32  aud_delta_0                           ; // 22, Delta Value
    	UINT32  aud_fifo_enable                       ; // 23, Audio FIFO Enable
    	UINT32  aud_fifo_mode                         ; // 24, FIFO Mode Control
    	UINT32  aud_fifo_support                      ; // 25, Supported FIFOs ( Debug Function )
    	UINT32  aud_fifo_reset                        ; // 26, Host FIFO Reset
    	UINT32  aud_chk_ctrl                          ; // 27, Checksum Control ( Debug Function )
    	UINT32  aud_checksum_data                     ; // 28, Checksum Data ( Debug Function )
    	UINT32  aud_chk_tcnt                          ; // 29, Target Count of Checksum ( Debug Function )
    	UINT32  aud_embedded_input_ctrl               ; // 30, Embedded Input Control ( Debug Function )
    	UINT32  aud_misc_ctrl                         ; // 31, Miscellaneous Control

    	// Group 062 : AUD
    	UINT32  aud_ext_dac_xck_cfg                   ; // 00
    	UINT32  aud_ext_dac_bck_cfg                   ; // 01
    	UINT32  aud_iec0_bclk_cfg                     ; // 02
    	UINT32  aud_ext_adc_xck_cfg                   ; // 03
    	UINT32  aud_ext_adc_bck_cfg                   ; // 04
    	UINT32  aud_int_adc_xck_cfg                   ; // 05
    	UINT32  G062_reserved_6                       ; // 06
    	UINT32  aud_int_dac_xck_cfg                   ; // 07
    	UINT32  aud_int_dac_bck_cfg                   ; // 08
    	UINT32  aud_iec1_bclk_cfg                     ; // 09
    	UINT32  G062_reserved_10                      ; // 10
    	UINT32  aud_pcm_iec_bclk_cfg                  ; // 11
    	UINT32  aud_xck_osr104_cfg                    ; // 12
    	UINT32  aud_hdmi_tx_mclk_cfg                  ; // 13
    	UINT32  aud_hdmi_tx_bck_cfg                   ; // 14
    	UINT32  hdmi_tx_i2s_cfg                       ; // 15
    	UINT32  hdmi_rx_i2s_cfg                       ; // 16
    	UINT32  aud_aadc_agc01_cfg0                   ; // 17
    	UINT32  aud_aadc_agc01_cfg1                   ; // 18
    	UINT32  aud_aadc_agc01_cfg2                   ; // 19
    	UINT32  aud_aadc_agc01_cfg3                   ; // 20
    	UINT32  int_adc_ctrl3                      	  ; // 21
    	UINT32  int_adc_ctrl2                         ; // 22
    	UINT32  int_dac_ctrl2                         ; // 23
    	UINT32  G062_reserved_24                      ; // 24
    	UINT32  int_dac_ctrl1                         ; // 25
    	UINT32  G062_reserved_26                      ; // 26
    	UINT32  G062_reserved_27                      ; // 27
    	UINT32  G062_reserved_28                      ; // 28
    	UINT32  G062_reserved_29                      ; // 29
   	UINT32  G062_reserved_30                      ; // 30
    	UINT32  G062_reserved_31                      ; // 31

    	// Group 063 : AUD
    	UINT32  aud_bt_ifx_cfg                        ; // 00
    	UINT32  aud_bt_i2s_cfg                        ; // 01
    	UINT32  aud_bt_xck_cfg                        ; // 02
    	UINT32  aud_bt_bck_cfg                        ; // 03
    	UINT32  aud_bt_sync_cfg                       ; // 04
    	UINT32  G063_reserved_5                       ; // 05
    	UINT32  G063_reserved_6                       ; // 06
    	UINT32  G063_reserved_7                       ; // 07
    	UINT32  aud_pwm_xck_cfg                       ; // 08
    	UINT32  aud_pwm_bck_cfg                       ; // 09
    	UINT32  G063_reserved_10                      ; // 10
    	UINT32  G063_reserved_11                      ; // 11
    	UINT32  G063_reserved_12                      ; // 12
    	UINT32  G063_reserved_13                      ; // 13
    	UINT32  G063_reserved_14                      ; // 14
    	UINT32  G063_reserved_15                      ; // 15
    	UINT32  G063_reserved_16                      ; // 16
    	UINT32  G063_reserved_17                      ; // 17
    	UINT32  G063_reserved_18                      ; // 18
    	UINT32  G063_reserved_19                      ; // 19
    	UINT32  aud_aadc_agc2_cfg0                    ; // 20
    	UINT32  aud_aadc_agc2_cfg1                    ; // 21
    	UINT32  aud_aadc_agc2_cfg2                    ; // 22
    	UINT32  aud_aadc_agc2_cfg3                    ; // 23
    	UINT32  aud_opt_test_pat                      ; // 24
    	UINT32  aud_sys_status0                       ; // 25
    	UINT32  aud_sys_status1                       ; // 26
    	UINT32  int_adc_ctrl1                         ; // 27
    	UINT32  bt_mute_flag                          ; // 28
    	UINT32  cdrpll_losd_ctrl                      ; // 29
    	UINT32  G063_reserved_30                      ; // 30
    	UINT32  other_config                          ; // 31

    	// Group 064 : AUD
    	UINT32  aud_a0_base                           ; // 00
    	UINT32  aud_a0_length                         ; // 01
    	UINT32  aud_a0_ptr                            ; // 02
    	UINT32  aud_a0_cnt                            ; // 03
    	UINT32  aud_a1_base                           ; // 04
    	UINT32  aud_a1_length                         ; // 05
    	UINT32  aud_a1_ptr                            ; // 06
    	UINT32  aud_a1_cnt                            ; // 07
    	UINT32  aud_a2_base                           ; // 08
    	UINT32  aud_a2_length                         ; // 09
    	UINT32  aud_a2_ptr                            ; // 10
    	UINT32  aud_a2_cnt                            ; // 11
    	UINT32  aud_a3_base                           ; // 12
    	UINT32  aud_a3_length                         ; // 13
    	UINT32  aud_a3_ptr                            ; // 14
    	UINT32  aud_a3_cnt                            ; // 15
    	UINT32  aud_a4_base                           ; // 16
    	UINT32  aud_a4_length                         ; // 17
    	UINT32  aud_a4_ptr                            ; // 18
    	UINT32  aud_a4_cnt                            ; // 19
    	UINT32  aud_a5_base                           ; // 20
    	UINT32  aud_a5_length                         ; // 21
    	UINT32  aud_a5_ptr                            ; // 22
    	UINT32  aud_a5_cnt                            ; // 23
    	UINT32  aud_a6_base                           ; // 24
    	UINT32  aud_a6_length                         ; // 25
    	UINT32  aud_a6_ptr                            ; // 26
    	UINT32  aud_a6_cnt                            ; // 27
    	UINT32  aud_a7_base                           ; // 28
    	UINT32  aud_a7_length                         ; // 29
    	UINT32  aud_a7_ptr                            ; // 30
    	UINT32  aud_a7_cnt                            ; // 31

    	// Group 065 : AUD
    	UINT32  aud_a8_base                           ; // 00
    	UINT32  aud_a8_length                         ; // 01
    	UINT32  aud_a8_ptr                            ; // 02
    	UINT32  aud_a8_cnt                            ; // 03
    	UINT32  aud_a9_base                           ; // 04
    	UINT32  aud_a9_length                         ; // 05
    	UINT32  aud_a9_ptr                            ; // 06
    	UINT32  aud_a9_cnt                            ; // 07
    	UINT32  aud_a10_base                          ; // 08
    	UINT32  aud_a10_length                        ; // 09
    	UINT32  aud_a10_ptr                           ; // 10
    	UINT32  aud_a10_cnt                           ; // 11
    	UINT32  aud_a11_base                          ; // 12
    	UINT32  aud_a11_length                        ; // 13
    	UINT32  aud_a11_ptr                           ; // 14
    	UINT32  aud_a11_cnt                           ; // 15
    	UINT32  aud_a12_base                          ; // 16
    	UINT32  aud_a12_length                        ; // 17
    	UINT32  aud_a12_ptr                           ; // 18
    	UINT32  aud_a12_cnt                           ; // 19
    	UINT32  aud_a13_base                          ; // 20
    	UINT32  aud_a13_length                        ; // 21
    	UINT32  aud_a13_ptr                           ; // 22
    	UINT32  aud_a13_cnt                           ; // 23
    	UINT32  aud_a14_base                          ; // 24
    	UINT32  aud_a14_length                        ; // 25
    	UINT32  aud_a14_ptr                           ; // 26
    	UINT32  aud_a14_cnt                           ; // 27
    	UINT32  aud_a15_base                          ; // 28
    	UINT32  aud_a15_length                        ; // 29
    	UINT32  aud_a15_ptr                           ; // 30
    	UINT32  aud_a15_cnt                           ; // 31


    	// Group 066 : AUD
    	UINT32  aud_a16_base                          ; // 00
    	UINT32  aud_a16_length                        ; // 01
    	UINT32  aud_a16_ptr                           ; // 02
    	UINT32  aud_a16_cnt                           ; // 03
    	UINT32  aud_a17_base                          ; // 04
    	UINT32  aud_a17_length                        ; // 05
    	UINT32  aud_a17_ptr                           ; // 06
    	UINT32  aud_a17_cnt                           ; // 07
    	UINT32  aud_a18_base                          ; // 08
    	UINT32  aud_a18_length                        ; // 09
    	UINT32  aud_a18_ptr                           ; // 10
    	UINT32  aud_a18_cnt                           ; // 11
    	UINT32  aud_a19_base                          ; // 12
    	UINT32  aud_a19_length                        ; // 13
    	UINT32  aud_a19_ptr                           ; // 14
    	UINT32  aud_a19_cnt                           ; // 15
    	UINT32  aud_a20_base                          ; // 16
    	UINT32  aud_a20_length                        ; // 17
    	UINT32  aud_a20_ptr                           ; // 18
    	UINT32  aud_a20_cnt                           ; // 19
    	UINT32  aud_a21_base                          ; // 20
    	UINT32  aud_a21_length                        ; // 21
    	UINT32  aud_a21_ptr                           ; // 22
    	UINT32  aud_a21_cnt                           ; // 23
 #if 0
    	UINT32  G066_reserved_24                      ; // 24
    	UINT32  G066_reserved_25                      ; // 25
    	UINT32  G066_reserved_26                      ; // 26
    	UINT32  G066_reserved_27                      ; // 27
    	UINT32  G066_reserved_28                      ; // 28
    	UINT32  G066_reserved_29                      ; // 29
    	UINT32  G066_reserved_30                      ; // 30
    	UINT32  G066_reserved_31                      ; // 31
 #else
    	UINT32  aud_a22_base                          ; // 24
    	UINT32  aud_a22_length                        ; // 25
    	UINT32  aud_a22_ptr                           ; // 26
    	UINT32  aud_a22_cnt                           ; // 27
    	UINT32  aud_a23_base                          ; // 28
    	UINT32  aud_a23_length                        ; // 29
    	UINT32  aud_a23_ptr                           ; // 30
    	UINT32  aud_a23_cnt                           ; // 31
 #endif

    	// Group 067 : AUD
    	UINT32  aud_grm_master_gain                   ; // 00 Gain Control
    	UINT32  aud_grm_gain_control_0                ; // 01 Gain Control
    	UINT32  aud_grm_gain_control_1                ; // 02 Gain Control
    	UINT32  aud_grm_gain_control_2                ; // 03 Gain Control
    	UINT32  aud_grm_gain_control_3                ; // 04 Gain Control
    	UINT32  aud_grm_gain_control_4                ; // 05 Gain Control
    	UINT32  aud_grm_mix_control_0                 ; // 06 Mixer Setting
    	UINT32  aud_grm_mix_control_1                 ; // 07 Mixer Setting
    	UINT32  aud_grm_mix_control_2                 ; // 08 Mixer Setting
    	UINT32  aud_grm_switch_0                      ; // 09 Channel Switch
    	UINT32  aud_grm_switch_1                      ; // 10 Channel Switch
    	UINT32  aud_grm_switch_int                    ; // 11 Channel Switch
    	UINT32  aud_grm_delta_volume                  ; // 12 Gain Update
    	UINT32  aud_grm_delta_ramp_pcm                ; // 13 Gain Update
    	UINT32  aud_grm_delta_ramp_risc               ; // 14 Gain Update
    	UINT32  aud_grm_delta_ramp_linein             ; // 15 Gain Update
    	UINT32  aud_grm_other                         ; // 16 Other Setting
    	UINT32  aud_grm_gain_control_5                ; // 17 Gain Control
    	UINT32  aud_grm_gain_control_6                ; // 18 Gain Control
    	UINT32  aud_grm_gain_control_7                ; // 19 Gain Control
    	UINT32  aud_grm_gain_control_8                ; // 20 Gain Control
    	UINT32  aud_grm_fifo_eflag                    ; // 21 FIFO Error Flag
    	UINT32  G067_reserved_22                      ; // 22
    	UINT32  G067_reserved_23                      ; // 23
    	UINT32  aud_grm_switch_hdmi_tx                ; // 24 AUD_GRM_SWITCH_HDMI_TX
    	UINT32  G067_reserved_25                      ; // 25
    	UINT32  G067_reserved_26                      ; // 26
    	UINT32  G067_reserved_27                      ; // 27
    	UINT32  G067_reserved_28                      ; // 28
    	UINT32  G067_reserved_29                      ; // 29
    	UINT32  G067_reserved_30                      ; // 30
    	UINT32  G067_reserved_31                      ; // 31

    	// Group 068 : AUD
    	UINT32  G068_AUD[32]                          ;

    	// Group 069 : Reserved
    	UINT32  G069_reserved_00                      ; // 00
    	UINT32  G069_reserved_01                      ; // 01
    	UINT32  G069_reserved_02                      ; // 02
    	UINT32  G069_reserved_03                      ; // 03
    	UINT32  G069_reserved_04                      ; // 04
    	UINT32  G069_reserved_05                      ; // 05
    	UINT32  G069_reserved_06                      ; // 06
    	UINT32  G069_reserved_07                      ; // 07
    	UINT32  G069_reserved_08                      ; // 08
    	UINT32  G069_reserved_09                      ; // 09
    	UINT32  G069_reserved_10                      ; // 10
    	UINT32  G069_reserved_11                      ; // 11
    	UINT32  G069_reserved_12                      ; // 12
    	UINT32  G069_reserved_13                      ; // 13
    	UINT32  G069_reserved_14                      ; // 14
    	UINT32  I2S_PWM_CONTROL_1                     ; // 15
    	UINT32  I2S_PWM_CONTROL_2                     ; // 16
    	UINT32  I2S_PWM_CONTROL_3                     ; // 17
    	UINT32  I2S_PWM_CONTROL_4                     ; // 18
    	UINT32  CLASSD_MOS_CONTROL                    ; // 19
    	UINT32  G069_reserved_20                      ; // 20
    	UINT32  G069_reserved_21                      ; // 21
    	UINT32  G069_reserved_22                      ; // 22
    	UINT32  G069_reserved_23                      ; // 23
    	UINT32  G069_reserved_24                      ; // 24
    	UINT32  G069_reserved_25                      ; // 25
    	UINT32  G069_reserved_26                      ; // 26
    	UINT32  G069_reserved_27                      ; // 27
    	UINT32  G069_reserved_28                      ; // 28
    	UINT32  G069_reserved_29                      ; // 29
    	UINT32  G069_reserved_30                      ; // 30
    	UINT32  G069_reserved_31                      ; // 31

    	// Group 070 : Reserved
    	UINT32  G070_RESERVED[32]                     ;

    	// Group 071 : Reserved
#if 0
    	UINT32  G071_RESERVED[32]                     ;
#else
    	UINT32  aud_a24_base                          ; // 00
    	UINT32  aud_a24_length                        ; // 01
    	UINT32  aud_a24_ptr                           ; // 02
    	UINT32  aud_a24_cnt                           ; // 03
    	UINT32  aud_a25_base                          ; // 04
    	UINT32  aud_a25_length                        ; // 05
    	UINT32  aud_a25_ptr                           ; // 06
    	UINT32  aud_a25_cnt                           ; // 07
    	UINT32  aud_a26_base                          ; // 08
    	UINT32  aud_a26_length                        ; // 09
    	UINT32  aud_a26_ptr                           ; // 10
    	UINT32  aud_a26_cnt                           ; // 11
    	UINT32  aud_a27_base                          ; // 12
    	UINT32  aud_a27_length                        ; // 13
    	UINT32  aud_a27_ptr                           ; // 14
    	UINT32  aud_a27_cnt                           ; // 15
    	UINT32  aud_a28_base                          ; // 16
    	UINT32  aud_a28_length                        ; // 17
    	UINT32  aud_a28_ptr                           ; // 18
    	UINT32  aud_a28_cnt                           ; // 19
    	UINT32  aud_a29_base                          ; // 20
    	UINT32  aud_a29_length                        ; // 21
    	UINT32  aud_a29_ptr                           ; // 22
    	UINT32  aud_a29_cnt                           ; // 23
    	UINT32  G071_reserved_24                      ; // 24
    	UINT32  G071_reserved_25                      ; // 25
    	UINT32  G071_reserved_26                      ; // 26
    	UINT32  G071_reserved_27                      ; // 27
    	UINT32  G071_reserved_28                      ; // 28
    	UINT32  G071_reserved_29                      ; // 29
    	UINT32  G071_reserved_30                      ; // 30
    	UINT32  G071_reserved_31                      ; // 31
#endif

    	// Group 072 : Reserved
    	UINT32  tdm_rx_cfg0                           ;//0
    	UINT32  tdm_rx_cfg1                           ;//1
    	UINT32  tdm_rx_cfg2                           ;//2
    	UINT32  tdm_rx_cfg3                           ;//3
    	UINT32  G72_reserved_4                        ;//4
    	UINT32  G72_reserved_5                        ;//5
    	UINT32  tdm_tx_cfg0                           ;//6
    	UINT32  tdm_tx_cfg1                           ;//7
    	UINT32  tdm_tx_cfg2                           ;//8
    	UINT32  tdm_tx_cfg3                           ;//9
    	UINT32  tdm_tx_cfg4                           ;//10
    	UINT32  G72_reserved_11                       ;//11
    	UINT32  G72_reserved_12                       ;//12
    	UINT32  G72_reserved_13                       ;//13
    	UINT32  pdm_rx_cfg0                           ;//14 
    	UINT32  pdm_rx_cfg1                           ;//15
    	UINT32  pdm_rx_cfg2                           ;//16
    	UINT32  pdm_rx_cfg3                           ;//17
    	UINT32  pdm_rx_cfg4                           ;//18
    	UINT32  pdm_rx_cfg5                           ;//19
    	UINT32  G72_reserved_20                       ;//20
    	UINT32  G72_reserved_21                       ;//21
    	UINT32  tdm_tx_xck_cfg                        ;//22
    	UINT32  tdm_tx_bck_cfg                        ;//23
    	UINT32  tdm_rx_xck_cfg                        ;//24
    	UINT32  tdm_rx_bck_cfg                        ;//25 TDM_RX_BCK_CFG
    	UINT32  pdm_rx_xck_cfg                        ;//26
    	UINT32  pdm_rx_bck_cfg                        ;//27
    	UINT32  G72_reserved_28                       ;//28
    	UINT32  G72_reserved_29                       ;//29
    	UINT32  tdmpdm_tx_sel                         ;//30
    	UINT32  G72_reserved_31                       ;//31
} RegisterFile_Audio;

typedef struct {
	unsigned int ch0_addr; // 0x9c10_6000
	unsigned int ch0_ctrl; // 0x9c10_6004
	unsigned int ch1_addr; // 0x9c10_6008
	unsigned int ch1_ctrl; // 0x9c10_600c
	unsigned int ch2_addr; // 0x9c10_6010
	unsigned int ch2_ctrl; // 0x9c10_6014
	unsigned int ch3_addr; // 0x9c10_6018
	unsigned int ch3_ctrl; // 0x9c10_601c
	unsigned int ch4_addr; // 0x9c10_6020
	unsigned int ch4_ctrl; // 0x9c10_6024
	unsigned int ch5_addr; // 0x9c10_6028
	unsigned int ch5_ctrl; // 0x9c10_602c
	unsigned int io_ctrl ; // 0x9c10_6030
	unsigned int io_tctl ; // 0x9c10_6034
	unsigned int macro_c0; // 0x9c10_6038
	unsigned int macro_c1; // 0x9c10_603c
	unsigned int macro_c2; // 0x9c10_6040
	unsigned int io_tpctl; // 0x9c10_6044
	unsigned int io_rpctl; // 0x9c10_6048
	unsigned int boot_ra ; // 0x9c10_604c
	unsigned int io_tdat0; // 0x9c10_6050
	unsigned int reserved[11];
}RegisterFile_Fbio;

//group 67
#define reg_aud_grm_master_gain		6700
#define reg_aud_grm_gain_control_0	6701
#define reg_aud_grm_gain_control_1	6702
#define reg_aud_grm_gain_control_2	6703
#define reg_aud_grm_gain_control_3	6704
#define reg_aud_grm_gain_control_4	6705
#define reg_aud_grm_mix_control_0	6706
#define reg_aud_grm_mix_control_1	6707
#define reg_aud_grm_mix_control_2	6708
#define reg_aud_grm_switch_0		6709
#define reg_aud_grm_switch_1		6710
#define reg_aud_grm_switch_int		6711
#define reg_aud_grm_delta_volume	6712
#define reg_aud_grm_delta_ramp_pcm	6713
#define reg_aud_grm_delta_ramp_risc	6714
#define reg_aud_grm_delta_ramp_linein	6715
#define reg_aud_grm_other		6716
#define reg_aud_grm_gain_control_5	6717
#define reg_aud_grm_gain_control_6	6718
#define reg_aud_grm_gain_control_7	6719
#define reg_aud_grm_gain_control_8	6720
#define reg_aud_grm_fifo_eflag		6721
#define reg_iec_tx_interfacegain	6722
#define reg_i2s_tx_interfacegain	6723
#define reg_aud_grm_switch_hdmi		6724
#define reg_aud_grm_switch_hdmi_tx	6724
#define reg_aud_grm_gain_control_10	6725

#if 0
//group 00
#define reset_0				0016

//group 01
#define sft_cfg_0            		0100
#define sft_cfg_1            		0101
#define sft_cfg_2            		0102
#define sft_cfg_3            		0103
#define sft_cfg_4            		0104
#define sft_cfg_5            		0105
#define sft_cfg_6            		0106
#define sft_cfg_7            		0107
#define sft_cfg_8            		0108
#define sft_cfg_9            		0109
#define sft_cfg_10           		0110
#define sft_cfg_11           		0111
#define sft_cfg_12           		0112
#define sft_cfg_13           		0113
#define sft_cfg_14           		0114
#define sft_cfg_15           		0115
#define sft_cfg_16           		0116
#define sft_cfg_17           		0117
#define sft_cfg_18           		0118
#define sft_cfg_19           		0119
#define sft_cfg_20           		0120
#define sft_cfg_21           		0121
#define sft_cfg_22           		0122
#define sft_cfg_23           		0123
#define sft_cfg_24           		0124
#define sft_cfg_25           		0125
#define sft_cfg_26           		0126
#define sft_cfg_27           		0127
#define sft_cfg_28           		0128
#define sft_cfg_29           		0129
#define sft_cfg_30           		0130
#define sft_cfg_31           		0131

// Group 004 : PAD_CTL
#define rf_pad_ctl0         		0400
#define rf_pad_ctl1         		0401
#define rf_pad_ctl2         		0402
#define rf_pad_ctl3         		0403
#define rf_pad_ctl4         		0404
#define rf_pad_ctl5         		0405
#define rf_pad_ctl6         		0406
#define rf_pad_ctl7         		0407
#define rf_pad_ctl8         		408
#define rf_pad_ctl9         		409
#define rf_pad_ctl10        		0410
#define rf_pad_ctl11        		0411
#define rf_pad_ctl12        		0412
#define rf_pad_ctl13        		0413
#define rf_pad_ctl14        		0414
#define rf_pad_ctl15        		0415
#define rf_pad_ctl16        		0416
#define rf_pad_ctl17        		0417
#define rf_pad_ctl18        		0418
#define rf_pad_ctl19        		0419
#define rf_pad_ctl20        		0420
#define rf_pad_ctl21        		0421
#define rf_pad_ctl22        		0422
#define rf_pad_ctl23        		0423
#define rf_pad_ctl24        		0424
#define gpio_first_0        		0425
#define gpio_first_1        		0426
#define gpio_first_2        		0427
#define gpio_first_3        		0428
#define gpio_first_4        		0429
#define gpio_first_5        		0430
#define rf_pad_ctl31        		0431




//group 20
#define dpll2_ctrl			2016
#define dpll2_remainder			2017
#define dpll2_denominator		2018
#define dpll2_divider			2019
#define dpll3_ctrl			2024
#define dpll3_remainder			2025
#define dpll3_denominator		2026
#define dpll3_divider			2027

//group 56
#define dsp_data_page			5625

//group 57
#define rdif_ctrl			5700
#define dsp_status			5704
#define rdif_oh_ctrl			5706

//group 58
#define dsp_dma_status			5809

//group 60
#define audif_ctrl			6000
#define aud_enable			6001
#define pcm_cfg				6002
#define i2s_mute_flag_ctrl		6003
#define ext_adc_cfg			6004
#define int_dac_ctrl0			6005
#define int_adc_ctrl0			6006
#define adc_in_path_switch		6007
#define int_adc_dac_cfg			6008	//[31:16] ADC, [15:0]DAC

#define iec_cfg				6010
#define iec0_valid_out			6011
#define iec0_par0_out			6012
#define iec0_par1_out			6013
#define iec1_valid_out			6014
#define iec1_par0_out			6015
#define iec1_par1_out			6016
#define iec_tx_user_wdata		6030
#define iec_tx_user_ctrl		6031

//group 61
#define adcp_ch_enable			6100
#define adcp_fubypass			6101
#define adcp_mode_ctrl			6102
#define adcp_init_ctrl			6103
#define adcp_coeff_din			6104
#define adcp_agc_cfg			6105
#define adcp_agc_cfg2			6106
#define adcp_gain_0			6107
#define adcp_gain_1			6108
#define adcp_gain_2			6109
#define adcp_gain_3			6110
#define adcp_risc_gain			6111
#define adcp_mic_l			6112

#define aud_apt_mode			6116
#define aud_apt_data			6117
#define aud_apt_parameter		6118
#define aud_audhwya2			6119
#define aud_audhwya			6120
#define aud_inc_0			6121
#define aud_delta_0			6122
#define aud_fifo_enable			6123
#define aud_fifo_mode			6124
#define aud_fifo_support		6125
#define aud_fifo_reset			6126
#define aud_new_pts			6128
#define aud_new_pts_ptr			6129
#define aud_input_cfg			6130
#define aud_misc_ctrl			6131

//group 62
#define aud_ext_dac_xck_cfg		6200
#define aud_ext_dac_bck_cfg		6201
#define aud_iec0_bclk_cfg		6202
#define aud_ext_adc_xck_cfg		6203
#define aud_ext_adc_bck_cfg		6204
#define aud_int_adc_xck_cfg		6205
#define aud_int_dac_xck_cfg		6207
#define aud_int_dac_bck_cfg		6208
#define aud_iec1_bclk_cfg		6209
#define aud_pcm_iec_bclk_cfg 		6211
#define aud_xck_osr104_cfg		6212
#define aud_hdmi_tx_mclk		6213
#define aud_hdmi_tx_bclk		6214
#define hdmi_tx_pcm_cfg			6215
#define hdmi_rx_cfg			6216
#define aud_aadc_agc_cfg0 		6217				  /* <17> [dv: 0xf007f007] [des: DAGC0/1 Config0] */
#define aud_aadc_agc_cfg1 		6218				  /* <18> [dv: 0x80FF80FF] [des: DAGC0/1 Config1] */
#define aud_aadc_agc_cfg2 		6219					  /* <19> [dv: 0x00F000F0] [des: DAGC0/1 Config2] */
#define aud_aadc_agc_cfg3 		6220			  /* <20> [dv: 0x08000800] [des: DAGC0/1 Config3] */
#define int_adc_ctrl3			6221				  /* <21> [dv: 0x3f245c1e] [des: Internal ADC Config 3] */
#define int_adc_ctrl2			6222			  /* <22> [dv: 0x06de1e1e] [des: Internal ADC Config 2] */
#define int_dac_ctrl2 			6223				  /* <23> [dv: 0x1e1e1e1e] [des: Internal DAC Control 2] */
#define g62addr24_reserved		6224				  /* <24> [dv: 0x00000000] [des: Reserved] */
#define int_dac_ctrl1			6225
#define aud_force_cken			6226
#define pcm_iec_par0_out		6228
#define pcm_iec_par1_out		6229
#define dmactrl_cnt_inc_1		6230
#define dmactrl_cnt_delta_1		6231

//group 63
#define bt_ifx_cfg			6300
#define bt_i2s_cfg			6301
#define bt_xck_cfg			6302
#define bt_bck_cfg			6303
#define bt_sync_cfg			6304

#define adac_pga_gain_0l_ctrl 		6314		  /* <14> [dv: 0x00000000] [des: ADAC_PGA_GAIN_0L_CTRL] */
#define adac_pga_gain_0r_ctrl		6315		  /* <15> [dv: 0x00000000] [des: ADAC_PGA_GAIN_0R_CTRL] */
#define adac_pga_gain_1l_ctrl		6316		  /* <16> [dv: 0x00000000] [des: ADAC_PGA_GAIN_1L_CTRL] */
#define adac_pga_gain_1r_ctrl		6317		  /* <17> [dv: 0x00000000] [des: ADAC_PGA_GAIN_1R_CTRL] */
#define adac_pga_gain_2r_ctrl 		6318
#define int_adc_ctrl1			6327


#define aud_other_ctl			6331

//group 64
#define aud_a0_base			6400
#define aud_a0_length			6401
#define aud_a0_ptr			6402
#define aud_a0_cnt			6403
#define aud_a1_base			6404
#define aud_a1_length			6405
#define aud_a1_ptr			6406
#define aud_a1_cnt			6407
#define aud_a2_base			6408
#define aud_a2_length			6409
#define aud_a2_ptr			6410
#define aud_a2_cnt			6411
#define aud_a3_base			6412
#define aud_a3_length			6413
#define aud_a3_ptr			6414
#define aud_a3_cnt			6415
#define aud_a4_base			6416
#define aud_a4_length			6417
#define aud_a4_ptr			6418
#define aud_a4_cnt			6419
#define aud_a5_base			6420
#define aud_a5_length			6421
#define aud_a5_ptr			6422
#define aud_a5_cnt			6423
#define aud_a6_base			6424
#define aud_a6_length			6425
#define aud_a6_ptr			6426
#define aud_a6_cnt			6427
#define aud_a7_base			6428
#define aud_a7_length			6429
#define aud_a7_ptr			6430
#define aud_a7_cnt			6431

//group 65
#define aud_a9_base			6504
#define aud_a9_length			6505
#define aud_a9_ptr			6506
#define aud_a9_cnt			6507
#define aud_a10_base			6508
#define aud_a10_length			6509
#define aud_a10_ptr			6510
#define aud_a10_cnt			6511
#define aud_a11_base			6512
#define aud_a11_length			6513
#define aud_a11_ptr			6514
#define aud_a11_cnt			6515
#define aud_a12_base			6516
#define aud_a12_length			6517
#define aud_a12_ptr			6518
#define aud_a12_cnt			6519
#define aud_a13_base			6520
#define aud_a13_length			6521
#define aud_a13_ptr			6522
#define aud_a13_cnt			6523
#define aud_a14_base			6524
#define aud_a14_length			6525
#define aud_a14_ptr			6526
#define aud_a14_cnt			6527

//group 66
#define aud_a16_base	 		6600
#define aud_a16_length			6601
#define aud_a16_ptr			6602
#define aud_a16_cnt			6603
#define aud_a17_base	  		6604
#define aud_a17_length			6605
#define aud_a17_ptr			6606
#define aud_a17_cnt			6607
#define aud_a18_base	   		6608
#define aud_a18_length			6609
#define aud_a18_ptr			6610
#define aud_a18_cnt			6611
#define aud_a19_base	    		6612
#define aud_a19_length			6613
#define aud_a19_ptr			6614
#define aud_a19_cnt			6615

//group 67
#define aud_grm_master_gain		6700
#define aud_grm_gain_control_0		6701
#define aud_grm_gain_control_1		6702
#define aud_grm_gain_control_2		6703
#define aud_grm_gain_control_3		6704
#define aud_grm_gain_control_4		6705
#define aud_grm_mix_control_0		6706
#define aud_grm_mix_control_1		6707
#define aud_grm_mix_control_2		6708
#define aud_grm_switch_0		6709
#define aud_grm_switch_1		6710
#define aud_grm_switch_int		6711
#define aud_grm_delta_volume		6712
#define aud_grm_delta_ramp_pcm		6713
#define aud_grm_delta_ramp_risc		6714
#define aud_grm_delta_ramp_linein	6715
#define aud_grm_other			6716
#define aud_grm_gain_control_5		6717
#define aud_grm_gain_control_6		6718
#define aud_grm_gain_control_7		6719
#define aud_grm_gain_control_8		6720
#define aud_grm_fifo_eflag		6721

#define iec_tx_interfacegain		6722
#define i2s_tx_interfacegain		6723
#define aud_grm_switch_hdmi		6724

#define aud_grm_switch_hdmi_tx		6724

//GROUP 49
#define G069_reserved_00            	6900
#endif

/**********************************************************
 * 			Function Prototype
 **********************************************************/
UINT32 HWREG_R(UINT32 reg_name);
void HWREG_W(UINT32 reg_name, UINT32 val);
void delay_ms(UINT32 ms_count);
void ADCP_INITIAL_BUFFER(UINT32 chan, UINT32 Fn);
void ADCP_INITIAL_COEFF_INDEX(UINT32 chan, UINT32 Fn, UINT32 coeff_index, UINT32 ena_inc);
void ADCP_SET_MODE(UINT32 up_ratio, UINT32 echo_data_mode, UINT32 dw_ratio );


#endif /* _SPSOC_UTIL_H */

