/*
 * ALSA SoC AUD3502 codec driver
 *
 * Author:	 <@sunplus.com>
 *
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include "spsoc_util.h"

#define AUD_FORMATS	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)|(SNDRV_PCM_FMTBIT_S24_3BE )

#if 0
/*================================================================
 *						codec driver
 *===============================================================*/
static int aud_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_dai_hw_params(struct snd_pcm_substream *substream,
	                     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}
#else
/*================================================================
 *						codec driver
 *===============================================================*/
static int aud_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_dai_hw_params(struct snd_pcm_substream *substream,
	                     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int rate = params_rate(params);

	AUD_INFO("%s IN, rate %d\n", __func__, rate);
	return 0;
}

#endif
static const struct snd_soc_dai_ops aud_dai_ops = {
	.startup 	= aud_dai_startup,
	.hw_params	= aud_dai_hw_params,
};

static struct snd_soc_dai_driver audcodec_dai[] = {
	{
		.name = "aud-codec-dai",
		.playback = {
			.stream_name 	= "I2S Playback",
			.channels_min 	= 2,
			.channels_max 	= 10,
			.rates 		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},
		.capture = {
			.stream_name 	= "I2S Capture",
			.channels_min	= 2,
			.channels_max 	= 8,
			.rates		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},
		.ops = &aud_dai_ops,
	},
	{
		.name = "aud-codec-tdm-dai",
		#if 1
		.playback = {
			.stream_name 	= "TDM playback",
			.channels_min 	= 2,
			.channels_max 	= 12,
			.rates 		= AUD_RATES_C,
			.formats 	= AUD_FORMATS,
		},
		#endif
		.capture = {
			.stream_name 	= "TDM Capture",
			.channels_min	= 2,
			.channels_max 	= 8,
			.rates 		= AUD_RATES_C,
			.formats 	= AUD_FORMATS,
		 },
		.ops = &aud_dai_ops,
	},
  	{
		.name = "aud-codec-pdm-dai",
		.capture = {
			.stream_name 	= "PDM Capture",
			.channels_min 	= 2,
			.channels_max 	= 8,
			.rates 		= AUD_RATES_C,
			.formats 	= AUD_FORMATS,
		},
		.ops = &aud_dai_ops,
	},
	{
		.name = "aud-spdif-dai",
		.playback = {
			.stream_name 	= "spdif Playback",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},
		.capture = {
			.stream_name 	= "spdif Capture",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},
		.ops = &aud_dai_ops,
	},
	{
		.name = "aud-i2s-hdmi-dai",
		.playback = {
			.stream_name 	= "i2s hdmi Playback",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},		
		.ops = &aud_dai_ops,
	},
	{
		.name = "aud-spdif-hdmi-dai",
		.playback = {
			.stream_name 	= "spdif hdmi Playback",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 		= AUD_RATES,
			.formats 	= AUD_FORMATS,
		},		
		.ops = &aud_dai_ops,
	},
};
#if 0
static int aud_probe(struct snd_soc_codec *codec)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_suspend(struct snd_soc_codec *codec)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_resume(struct snd_soc_codec *codec)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_remove(struct snd_soc_codec *codec)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_set_sysclk(struct snd_soc_codec *codec, int clk_id,
			                    int source, unsigned int freq, int dir)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

static int aud_set_pll(struct snd_soc_codec *codec, int pll_id, int source, unsigned int freq_in, unsigned int freq_out)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}
 #endif
static const DECLARE_TLV_DB_SCALE(volume_tlv, -6000, 0, 1);
static const char *cpm0_5_out[] = {"pcm0","pcm5"};

static const struct soc_enum cpm0_5_out_enum = SOC_ENUM_DOUBLE(reg_aud_grm_gain_control_8, 16,17, 2, cpm0_5_out);


static const struct snd_kcontrol_new aud_snd_controls[] = {
	/* master gains */
	SOC_SINGLE_TLV("Master Playback Volume",reg_aud_grm_master_gain, 	8, 	2<<22,	0, 	volume_tlv),
	/* Playback gains */
	SOC_DOUBLE_TLV("A0 Playback Volume",	reg_aud_grm_gain_control_5,  	0,  	8, 	0x80, 	0, 	volume_tlv),
	SOC_DOUBLE_TLV("A1 Playback Volume", 	reg_aud_grm_gain_control_5, 	16, 	24, 	0x80, 	0, 	volume_tlv),

	SOC_DOUBLE_TLV("A2 Playback Volume", 	reg_aud_grm_gain_control_6,  	0,  	8, 	0x80, 	0, 	volume_tlv),
	SOC_DOUBLE_TLV("A3 Playback Volume", 	reg_aud_grm_gain_control_6, 	16, 	24, 	0x80, 	0, 	volume_tlv),
	
	SOC_SINGLE_TLV("A4 Playback Volume 1", 	reg_aud_grm_gain_control_7, 	14, 	2<<16, 	0, 	volume_tlv),
	SOC_SINGLE_TLV("A4 Playback Volume 2", 	reg_aud_grm_gain_control_8, 	14, 	2<<16, 	0, 	volume_tlv),
    	SOC_DOUBLE_TLV("A20 Playback Volume", 	reg_aud_grm_gain_control_10,  	0,	8, 	0x80, 	0, 	volume_tlv),

	/* Mux */
	SOC_ENUM("pcm0 pcm5 Output", cpm0_5_out_enum),

	/* Mix */
	SOC_SINGLE("Mix0",reg_aud_grm_mix_control_0,0,1,0),
	SOC_SINGLE("Mix1",reg_aud_grm_mix_control_0,1,1,0),
	SOC_SINGLE("Mix2",reg_aud_grm_mix_control_0,2,1,0),
	SOC_SINGLE("Mix3",reg_aud_grm_mix_control_0,3,1,0),
	SOC_SINGLE("Mix4",reg_aud_grm_mix_control_0,4,1,0),
	SOC_SINGLE("Mix5",reg_aud_grm_mix_control_0,5,1,0),
	SOC_SINGLE("Mix6",reg_aud_grm_mix_control_0,6,1,0),
	SOC_SINGLE("Mix7",reg_aud_grm_mix_control_0,7,1,0),
	SOC_SINGLE("Mix8",reg_aud_grm_mix_control_0,8,1,0),
	SOC_SINGLE("Mix9",reg_aud_grm_mix_control_0,9,1,0),
	SOC_SINGLE("Mix30",reg_aud_grm_mix_control_0,10,1,0),
	SOC_SINGLE("Mix36",reg_aud_grm_mix_control_0,11,1,0),
	SOC_SINGLE("Mix46",reg_aud_grm_mix_control_0,12,1,0),
	SOC_SINGLE("Mix52",reg_aud_grm_mix_control_0,13,1,0),
	SOC_SINGLE("Mix72",reg_aud_grm_mix_control_0,14,1,0),

	SOC_SINGLE("Mix10",reg_aud_grm_mix_control_0,16,1,0),
	SOC_SINGLE("Mix11",reg_aud_grm_mix_control_0,17,1,0),
	SOC_SINGLE("Mix12",reg_aud_grm_mix_control_0,18,1,0),
	SOC_SINGLE("Mix13",reg_aud_grm_mix_control_0,19,1,0),
	SOC_SINGLE("Mix14",reg_aud_grm_mix_control_0,20,1,0),
	SOC_SINGLE("Mix15",reg_aud_grm_mix_control_0,21,1,0),
	SOC_SINGLE("Mix16",reg_aud_grm_mix_control_0,22,1,0),
	SOC_SINGLE("Mix17",reg_aud_grm_mix_control_0,23,1,0),
	SOC_SINGLE("Mix18",reg_aud_grm_mix_control_0,24,1,0),
	SOC_SINGLE("Mix19",reg_aud_grm_mix_control_0,25,1,0),

	SOC_SINGLE("Mix31",reg_aud_grm_mix_control_0,26,1,0),
	SOC_SINGLE("Mix37",reg_aud_grm_mix_control_0,27,1,0),
	SOC_SINGLE("Mix47",reg_aud_grm_mix_control_0,28,1,0),
	SOC_SINGLE("Mix53",reg_aud_grm_mix_control_0,29,1,0),

	SOC_SINGLE("Mix26",reg_aud_grm_mix_control_1,0,1,0),
	SOC_SINGLE("Mix27",reg_aud_grm_mix_control_1,1,1,0),
	SOC_SINGLE("Mix28",reg_aud_grm_mix_control_1,2,1,0),
	SOC_SINGLE("Mix29",reg_aud_grm_mix_control_1,3,1,0),
	SOC_SINGLE("Mix73",reg_aud_grm_mix_control_1,4,1,0),
	SOC_SINGLE("Mix74",reg_aud_grm_mix_control_1,5,1,0),
	SOC_SINGLE("Mix39",reg_aud_grm_mix_control_1,6,1,0),
	SOC_SINGLE("Mix32",reg_aud_grm_mix_control_1,7,1,0),
  	SOC_SINGLE("Mix33",reg_aud_grm_mix_control_1,8,1,0),
	SOC_SINGLE("Mix34",reg_aud_grm_mix_control_1,9,1,0),
	SOC_SINGLE("Mix35",reg_aud_grm_mix_control_1,10,1,0),
	SOC_SINGLE("Mix75",reg_aud_grm_mix_control_1,11,1,0),
	SOC_SINGLE("Mix76",reg_aud_grm_mix_control_1,12,1,0),
	SOC_SINGLE("Mix40",reg_aud_grm_mix_control_1,13,1,0),
	SOC_SINGLE("Mix41",reg_aud_grm_mix_control_1,14,1,0),

	SOC_SINGLE("Mix42",reg_aud_grm_mix_control_1,16,1,0),
	SOC_SINGLE("Mix43",reg_aud_grm_mix_control_1,17,1,0),
	SOC_SINGLE("Mix44",reg_aud_grm_mix_control_1,18,1,0),
	SOC_SINGLE("Mix45",reg_aud_grm_mix_control_1,19,1,0),
	SOC_SINGLE("Mix77",reg_aud_grm_mix_control_1,20,1,0),
	SOC_SINGLE("Mix78",reg_aud_grm_mix_control_1,21,1,0),
	SOC_SINGLE("Mix56",reg_aud_grm_mix_control_1,22,1,0),
	SOC_SINGLE("Mix48",reg_aud_grm_mix_control_1,23,1,0),
	SOC_SINGLE("Mix49",reg_aud_grm_mix_control_1,24,1,0),
	SOC_SINGLE("Mix50",reg_aud_grm_mix_control_1,25,1,0),
	SOC_SINGLE("Mix51",reg_aud_grm_mix_control_1,26,1,0),
	SOC_SINGLE("Mix79",reg_aud_grm_mix_control_1,27,1,0),
	SOC_SINGLE("Mix80",reg_aud_grm_mix_control_1,28,1,0),
	SOC_SINGLE("Mix57",reg_aud_grm_mix_control_1,29,1,0),
	SOC_SINGLE("Mix55",reg_aud_grm_mix_control_1,30,1,0),

	SOC_SINGLE("Mix58",reg_aud_grm_mix_control_2,0,1,0),
	SOC_SINGLE("Mix59",reg_aud_grm_mix_control_2,1,1,0),
	SOC_SINGLE("Mix60",reg_aud_grm_mix_control_2,2,1,0),
	SOC_SINGLE("Mix61",reg_aud_grm_mix_control_2,3,1,0),
	SOC_SINGLE("Mix62",reg_aud_grm_mix_control_2,4,1,0),
	SOC_SINGLE("Mix63",reg_aud_grm_mix_control_2,5,1,0),
	SOC_SINGLE("Mix64",reg_aud_grm_mix_control_2,6,1,0),
	SOC_SINGLE("Mix65",reg_aud_grm_mix_control_2,7,1,0),
	SOC_SINGLE("Mix66",reg_aud_grm_mix_control_2,8,1,0),
	SOC_SINGLE("Mix67",reg_aud_grm_mix_control_2,9,1,0),
	SOC_SINGLE("Mix68",reg_aud_grm_mix_control_2,10,1,0),
	SOC_SINGLE("Mix69",reg_aud_grm_mix_control_2,11,1,0),
	SOC_SINGLE("Mix70",reg_aud_grm_mix_control_2,12,1,0),
	SOC_SINGLE("Mix71",reg_aud_grm_mix_control_2,13,1,0),

	SOC_SINGLE("Mix20",reg_aud_grm_mix_control_2,16,1,0),
	SOC_SINGLE("Mix21",reg_aud_grm_mix_control_2,17,1,0),
	SOC_SINGLE("Mix22",reg_aud_grm_mix_control_2,18,1,0),
	SOC_SINGLE("Mix23",reg_aud_grm_mix_control_2,19,1,0),
	SOC_SINGLE("Mix24",reg_aud_grm_mix_control_2,20,1,0),
	SOC_SINGLE("Mix25",reg_aud_grm_mix_control_2,21,1,0),
	SOC_SINGLE("Mix38",reg_aud_grm_mix_control_2,22,1,0),
	SOC_SINGLE("Mix54",reg_aud_grm_mix_control_2,23,1,0),
	SOC_SINGLE("Mix81",reg_aud_grm_mix_control_2,24,1,0),
	SOC_SINGLE("Mix82",reg_aud_grm_mix_control_2,25,1,0),
	SOC_SINGLE("Mix83",reg_aud_grm_mix_control_2,26,1,0),
	SOC_SINGLE("Mix84",reg_aud_grm_mix_control_2,27,1,0),

	//SOC_SINGLE_TLV()
};


static unsigned int audreg_read(struct snd_soc_component *component, unsigned int reg)
{
	int val, addr_g, addr_i, addr;
	  
	//AUD_INFO("r*audio_base=%08x\n", audio_base);
	addr_i = reg % 100;
	addr_g = (reg - addr_i)/100 - 60;
	addr = audio_base + addr_g*32*4 + addr_i*4;
	val = (*(volatile unsigned int *)(addr));
	//SYNCHRONIZE_IO;
    
    	//AUD_INFO("*val=%08x\n", val);
	return val;
	//return HWREG_R(reg);
}

static int audreg_write(struct snd_soc_component *component, unsigned int reg,unsigned int value)
{
	int  addr_g, addr_i, addr;
	  
	//AUD_INFO("w*audio_base=%08x, val = 0x%x\n", audio_base, value);
	addr_i = reg % 100;
	addr_g = (reg - addr_i)/100 - 60;
	addr = audio_base + addr_g*32*4 + addr_i*4;
	(*(volatile unsigned int *)(addr)) = value;
	//SYNCHRONIZE_IO;
	  
	//AUD_INFO("val=%08x\n", (*(volatile unsigned int *)(addr)));
	//HWREG_W(reg,value);
	return 0;
}

#if 0
static struct snd_soc_codec_driver soc_codec_dev_aud = {
	.probe		= aud_probe,
	.remove		= aud_remove,
	.suspend	= aud_suspend,
	.resume		= aud_resume,
	.set_sysclk	= aud_set_sysclk,
	.set_pll	= aud_set_pll,
	.read		= audreg_read,  //changed core func !
	.write		= audreg_write,

	.component_driver = {
		.controls	= aud_snd_controls,
		.num_controls	=  ARRAY_SIZE(aud_snd_controls),
	},
};
#endif
static const struct snd_soc_component_driver soc_codec_dev_aud = {
	//.name = "aud-codec",
	.read		= audreg_read,  //changed core func !
	.write		= audreg_write,
	.controls	= aud_snd_controls,
	.num_controls	= ARRAY_SIZE(aud_snd_controls),
};

static int aud_codec_probe(struct platform_device *pdev)
{
	int ret = 0;
	AUD_INFO("%s IN\n", __func__);

	ret = devm_snd_soc_register_component(&pdev->dev, &soc_codec_dev_aud,
				  	      audcodec_dai, ARRAY_SIZE(audcodec_dai));
		                     
	return ret;
}

static int aud_codec_remove(struct platform_device *pdev)
{
	//snd_soc_unregister_codec(&pdev->dev);

	return 0;
}

static struct platform_driver aud_codec_driver = {
	.probe	= aud_codec_probe,
	.remove	= aud_codec_remove,
	.driver	= {
		.name	= "aud-codec",
		.owner	= THIS_MODULE,
	},
};


static struct platform_device *spsoc_pcm_device;
static int __init aud_modinit(void)
{
	int ret = platform_driver_register(&aud_codec_driver);

	spsoc_pcm_device = platform_device_alloc("aud-codec", -1);
	if (!spsoc_pcm_device)
		return -ENOMEM;

	AUD_INFO("%s IN, create soc_card\n", __func__);

	ret = platform_device_add(spsoc_pcm_device);
	if (ret)
		platform_device_put(spsoc_pcm_device);

	return ret;
}
module_init(aud_modinit);

static void __exit aud_modexit(void)
{
	platform_driver_unregister(&aud_codec_driver);
}
module_exit(aud_modexit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC AUD codec driver");
MODULE_AUTHOR("Sunplus DSP");
