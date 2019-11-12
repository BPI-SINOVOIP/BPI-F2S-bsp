/*----------------------------------------------------------------------------*
 *					INCLUDE DECLARATIONS
 *---------------------------------------------------------------------------*/
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <mach/hdmitx.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#ifdef CONFIG_PM_RUNTIME_HDMITX
#include <linux/pm_runtime.h>
#endif
#include "include/hal_hdmitx.h"
#include <media/sp-disp/display.h> //#ifdef TIMING_SYNC_720P60
/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/
/*about print msg*/
#ifdef _HDMITX_ERR_MSG_
#define HDMITX_ERR(fmt, args...) printk(KERN_ERR fmt, ##args)
#else
#define HDMITX_ERR(fmt, args...)
#endif

#ifdef _HDMITX_WARNING_MSG_
#define HDMITX_WARNING(fmt, args...) printk(KERN_WARNING fmt, ##args)
#else
#define HDMITX_WARNING(fmt, args...)
#endif

#ifdef _HDMITX_INFO_MSG_
#define HDMITX_INFO(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define HDMITX_INFO(fmt, args...)
#endif

#ifdef _HDMITX_DBG_MSG_
#define HDMITX_DBG(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#else
#define HDMITX_DBG(fmt, args...)
#endif

/*----------------------------------------------------------------------------*
 *					DATA TYPES
 *---------------------------------------------------------------------------*/
enum hdmitx_fsm {
	FSM_INIT,
	FSM_HPD,
	FSM_RSEN,
	FSM_HDCP
};

struct hdmitx_video_attribute {
	enum hdmitx_timing timing;
	enum hdmitx_color_depth color_depth;
	enum hdmitx_color_space_conversion conversion;
};

struct hdmitx_audio_attribute {
	enum hdmitx_audio_channel chl;
	enum hdmitx_audio_type type;
	enum hdmitx_audio_sample_size sample_size;
	enum hdmitx_audio_layout layout;
	enum hdmitx_audio_sample_rate sample_rate;
};

struct hdmitx_config {
	enum hdmitx_mode mode;
	struct hdmitx_video_attribute video;
	struct hdmitx_audio_attribute audio;
};

/*----------------------------------------------------------------------------*
 *					GLOBAL VARIABLES
 *---------------------------------------------------------------------------*/
// device driver functions
static int hdmitx_fops_open(struct inode *inode, struct file *pfile);
static int hdmitx_fops_release(struct inode *inode, struct file *pfile);
static long hdmitx_fops_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg);

// platform driver functions
static int hdmitx_probe(struct platform_device *dev);
static int hdmitx_remove(struct platform_device *dev);
static int hdmitx_suspend(struct platform_device *dev,  pm_message_t state);
static int hdmitx_resume(struct platform_device *dev);

#ifdef CONFIG_PM_RUNTIME_HDMITX
// runtime power management functions
static int sp_hdmitx_runtime_suspend(struct device *dev);
static int sp_hdmitx_runtime_resume(struct device *dev);
#endif

// device id
static struct of_device_id g_hdmitx_ids[] = {
	{.compatible = "sunplus,sp7021-hdmitx"},
};

// device driver operation functions
static const struct file_operations g_hdmitx_fops = {
	.owner          = THIS_MODULE,
	.open           = hdmitx_fops_open,
	.release        = hdmitx_fops_release,
	.unlocked_ioctl = hdmitx_fops_ioctl,
	// .mmap           = hdmitx_mmap,
};

static struct miscdevice g_hdmitx_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "hdmitx",
	.fops  = &g_hdmitx_fops,
};

// platform device
// static struct platform_device g_hdmitx_device = {
// 	.name = "hdmitx",
// 	.id   = -1,
// };

#ifdef CONFIG_PM_RUNTIME_HDMITX
static const struct dev_pm_ops sp7021_hdmitx_pm_ops = {
	.runtime_suspend    = sp_hdmitx_runtime_suspend,
	.runtime_resume     = sp_hdmitx_runtime_resume,
};

#define sp_hdmitx_pm_ops (&sp7021_hdmitx_pm_ops)
#endif

// platform driver
static struct platform_driver g_hdmitx_driver = {
	.probe    = hdmitx_probe,
	.remove   = hdmitx_remove,
	.suspend  = hdmitx_suspend,
	.resume   = hdmitx_resume,
	// .shutdown = hdmitx_shotdown,
	.driver   = {
		.name           = "hdmitx",
		.of_match_table = of_match_ptr(g_hdmitx_ids),
		.owner          = THIS_MODULE,
#ifdef CONFIG_PM_RUNTIME_HDMITX
		.pm		= sp_hdmitx_pm_ops,
#endif			
	},
};
#ifdef TIMING_SYNC_720P60
static int __init sp_hdmitx_init(void)
{
	return platform_driver_register(&g_hdmitx_driver);
}
fs_initcall(sp_hdmitx_init);

static void __exit sp_hdmitx_exit(void)
{
	platform_driver_unregister(&g_hdmitx_driver);
}
module_exit(sp_hdmitx_exit);
#else
module_platform_driver(g_hdmitx_driver);
#endif

static struct task_struct *g_hdmitx_task;
static struct mutex g_hdmitx_mutex;
static enum hdmitx_fsm g_hdmitx_state = FSM_INIT;
static unsigned char g_hpd_in = FALSE;
static unsigned char g_rx_ready = FALSE;
static struct hdmitx_config g_cur_hdmi_cfg;
static struct hdmitx_config g_new_hdmi_cfg;
static unsigned char edid[EDID_CAPACITY];
static unsigned int edid_data_ofs;
#ifdef CONFIG_EDID_READ
static unsigned char edid_read_timeout = FALSE;
#endif
#ifdef MODE_CHANGE
static unsigned char g_hdmitx_mode = 0;	/* 0 : DVI mode, 1 : HDMI mode */
module_param(g_hdmitx_mode, byte, 0644);
#endif

typedef struct {
	void __iomem *moon4base;
	void __iomem *moon5base;
	void __iomem *moon1base;
	void __iomem *hdmitxbase;	
	struct miscdevice *hdmitx_misc;
	struct device *dev;
	struct clk *clk;
	struct reset_control *rstc;
} sp_hdmitx_t;

static sp_hdmitx_t *sp_hdmitx;

/*----------------------------------------------------------------------------*
 *					EXTERNAL DECLARATIONS
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *					FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/
static unsigned char get_hpd_in(void)
{
#ifdef CONFIG_HPD_DETECTION
	return g_hpd_in;
#else
	return TRUE;
#endif
}

static unsigned char get_rx_ready(void)
{
	return g_rx_ready;
}

#ifdef MODE_CHANGE
static unsigned char get_hdmitx_mode(void)
{
	return g_hdmitx_mode;
}
#endif

static void set_hdmi_mode(enum hdmitx_mode mode)
{
	unsigned char is_hdmi;

	is_hdmi = (mode == HDMITX_MODE_HDMI) ? TRUE : FALSE;

	hal_hdmitx_set_hdmi_mode(is_hdmi);
}

static void config_video(struct hdmitx_video_attribute *video)
{
	struct hal_hdmitx_video_attribute attr;

	if (!video) {
		return;
	}

	hal_hdmitx_get_video(&attr);

	attr.timing      = (enum hal_hdmitx_timing) video->timing;
	attr.color_depth = (enum hal_hdmitx_color_depth) video->color_depth;

	switch (video->conversion) {
		case HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_FULL_RGB:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_RGB:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_FULL_RGB:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_YUV422_TO_LIMITED_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_RGB:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_LIMITED_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_LIMITED_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_RGB_TO_FULL_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_LIMITED_RGB:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_RGB:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV444_TO_FULL_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV444;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_LIMITED_RGB:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_RGB:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_YUV444:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_YUV444;
			break;
		case HDMITX_COLOR_SPACE_CONV_FULL_YUV422_TO_FULL_YUV422:
			attr.input_range  = QUANTIZATION_RANGE_FULL;
			attr.output_range = QUANTIZATION_RANGE_FULL;
			attr.input_fmt    = PIXEL_FORMAT_YUV422;
			attr.output_fmt   = PIXEL_FORMAT_YUV422;
			break;
		case HDMITX_COLOR_SPACE_CONV_LIMITED_RGB_TO_LIMITED_RGB:
		default:
			attr.input_range  = QUANTIZATION_RANGE_LIMITED;
			attr.output_range = QUANTIZATION_RANGE_LIMITED;
			attr.input_fmt    = PIXEL_FORMAT_RGB;
			attr.output_fmt   = PIXEL_FORMAT_RGB;
			break;
	}

	hal_hdmitx_config_video(&attr);
}

static void config_audio(struct hdmitx_audio_attribute *audio)
{
	struct hal_hdmitx_audio_attribute attr;

	if (!audio)	{
		return;
	}

	hal_hdmitx_get_audio(&attr);

	attr.chl         = (enum hal_hdmitx_audio_channel) audio->chl;
	attr.type        = (enum hal_hdmitx_audio_type) audio->type;
	attr.sample_size = (enum hal_hdmitx_audio_sample_size) audio->sample_size;
	attr.layout      = (enum hal_hdmitx_audio_layout) audio->layout;
	attr.sample_rate = (enum hal_hdmitx_audio_sample_rate) audio->sample_rate;

	hal_hdmitx_config_audio(&attr);
}

static void start(void)
{
	hal_hdmitx_start(sp_hdmitx->moon4base, sp_hdmitx->moon5base, sp_hdmitx->hdmitxbase);
}

static void stop(void)
{
	hal_hdmitx_stop(sp_hdmitx->hdmitxbase);
}

#ifdef CONFIG_EDID_READ
static void read_edid(void)
{
	unsigned int timeout = EDID_TIMEOUT;
	
	edid_data_ofs = 0;
	hal_hdmitx_ddc_cmd(HDMITX_DDC_CMD_CLEAR_FIFO, edid_data_ofs, sp_hdmitx->hdmitxbase);
	
	do {
		if (hal_hdmitx_get_ddc_status(DDC_STUS_CMD_DONE, sp_hdmitx->hdmitxbase)) {
			hal_hdmitx_ddc_cmd(HDMITX_DDC_CMD_SEQ_READ, edid_data_ofs, sp_hdmitx->hdmitxbase);			
		}
		
		mdelay(10);
		if (timeout-- == 0) {
			edid_read_timeout = TRUE;
			g_cur_hdmi_cfg.mode = HDMITX_MODE_HDMI;
	#ifdef MODE_CHANGE
			g_hdmitx_mode = 1;
	#endif
			HDMITX_WARNING("EDID read timeout\n");
			break;
		}
	} while(edid_data_ofs != EDID_CAPACITY);
}

static void parse_edid(void)
{
	unsigned int i, j;
	unsigned sum = 0;
	unsigned int data, pre_data, d;
	char name[13];
	unsigned int audio = FALSE,  yuv444 = FALSE, yuv422 = FALSE;
	unsigned int rgb_limit = FALSE, yuv_limit = FALSE;
	
	HDMITX_DBG("\n");
	for (i = 0; i < EDID_CAPACITY; i += DDC_FIFO_CAPACITY) {
		HDMITX_DBG("EDID[%03u] ~ EDID[%03u] : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
			                          i, i + 7,
			                         edid[i],  edid[i+1],  edid[i+2],  edid[i+3],
			                       edid[i+4],  edid[i+5],  edid[i+6],  edid[i+7]);

		HDMITX_DBG("EDID[%03u] ~ EDID[%03u] : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
			                          i + 8, i + DDC_FIFO_CAPACITY - 1,
			                       edid[i+8],  edid[i+9], edid[i+10], edid[i+11],
			                      edid[i+12], edid[i+13], edid[i+14], edid[i+15]);	

		for (j = i; j < (i + DDC_FIFO_CAPACITY); j++) {
			sum = sum + edid[j];
		}

		if (i == 112) {
			if (edid[i + DDC_FIFO_CAPACITY - 2] == 0) {
				break;
			}
		}
	}
	HDMITX_DBG("Checksum = 0x%04X\n", sum);

	//monitor
	i = 72;
	while (i < 126) {
		if (edid[i+3] == 0xFC) {
			for (i += 5, j = 0; j < 13; i++, j++) {
				if (edid[i] != 0x0A) {
					name[j] = edid[i];
				} else {
					name[j] = 0;;
				}
			}

			break;
		}
		else {
			i += 18;
		}
	}
	HDMITX_DBG("Monitor %s", name);
	
	//mode
	if (edid[126] == 0) {
		g_cur_hdmi_cfg.mode = HDMITX_MODE_DVI;
	} else if (edid[126] == 1) {
		g_cur_hdmi_cfg.mode = HDMITX_MODE_HDMI;
	}

	if (g_cur_hdmi_cfg.mode == HDMITX_MODE_DVI) {
		//timing
		data = (edid[61] & 0xF0);
		data = ((data << 4) | edid[59]);
		HDMITX_DBG("DVI Max. Timing : %up\n", data);

	#if 0
		if (data <= 480) {
			g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_480P;
		} else if (data <= 576) {
			g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_576P;
		} else if (data <= 720) {
			g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_720P60;
		} else {
			g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_1080P60;
		}
	#endif	
	}
	else if (g_cur_hdmi_cfg.mode == HDMITX_MODE_HDMI) {
		d = edid[130];
		
		if (edid[131] & 0x40) {
			audio = TRUE;
		}
		
		if (edid[131] & 0x20) {
			yuv444 = TRUE;
		}
		
		if (edid[131] & 0x10) {
			yuv422 = TRUE;
		}

		i = 132;
		
		//data blocks from byte 132 to byte (128+d-1)
		if (d != 4) {
			while (i < (128 + d - 1)) {
				data = edid[i];
				
				if (((data & 0xE0) >> 5) == EDID_USE_EXTENDED_TAG) {
					i++;
					
					if (edid[i+1] == EDID_VIDEO_CAPABILITY_DATA_BLOCK) {
						if (!(edid[i+4] & 0x80)) {
							yuv_limit = TRUE;
						}

						if (!(edid[i+4] & 0x40)) {
							rgb_limit = TRUE;
						}
					} else {
						yuv_limit = TRUE;
						rgb_limit = TRUE;
					}
				} else {
					yuv_limit = TRUE;
					rgb_limit = TRUE;
				}

				i = i + (data & 0x1F) + 1;
			}
		}


	#if 0
		if (g_cur_hdmi_cfg.video.conversion == HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_FULL_RGB) {
			if (rgb_limit == TRUE) {
				g_cur_hdmi_cfg.video.conversion = HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB;
			}
		} else if (g_cur_hdmi_cfg.video.conversion == HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV444) {
			if (yuv444 == FALSE) {	
				g_cur_hdmi_cfg.video.conversion = HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB;
			}
		} else if (g_cur_hdmi_cfg.video.conversion == HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_YUV422) {
			if (yuv422 == FALSE) {	
				g_cur_hdmi_cfg.video.conversion = HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB;
			}
		}
	#endif
	
		//detailed timing descriptors
		if (d != 0) {
			pre_data = 0;
			while ((i < (EDID_CAPACITY - 1)) && (edid[i] != 0)) {
				//timing
				data = (edid[i+7] & 0xF0);
				data = ((data << 4) | edid[i+5]);

				if (data > pre_data) {
					pre_data = data;	
				}

				i += 18;
			}

			HDMITX_DBG("HDMI Max. Timing : %up\n", pre_data);

	#if 0	
			if (pre_data <= 480) {
				g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_480P;
			} else if (pre_data <= 576) {
				g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_576P;
			} else if (pre_data <= 720) {
				g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_720P60;
			} else {
				g_cur_hdmi_cfg.video.timing = HDMITX_TIMING_1080P60;
			}
	#endif		
		}
	}
}
#endif

static void process_hpd_state(void)
{
#ifdef CONFIG_PM_RUNTIME_HDMITX
	int ret;
	static int runtime_put = FALSE;
	static int runtime_put1 = TRUE;
#endif

	HDMITX_DBG("HPD State\n");

	if (get_hpd_in()) {
#ifdef CONFIG_PM_RUNTIME_HDMITX
		if (runtime_put1 == TRUE) { 
			runtime_put = TRUE;

			ret = pm_runtime_get_sync(sp_hdmitx->dev);
			if (ret < 0) {
				pm_runtime_mark_last_busy(sp_hdmitx->dev);
			}
		}
#endif

#ifdef CONFIG_EDID_READ
		// send AV mute

		edid_read_timeout = FALSE;
		// read EDID
		read_edid();
		
		// parser EDID
		if (edid_read_timeout == FALSE)
			parse_edid();
#else
		// send AV mute
#endif

		// update state
		mutex_lock(&g_hdmitx_mutex);
		g_hdmitx_state = FSM_RSEN;
		mutex_unlock(&g_hdmitx_mutex);
	} else {
#ifdef CONFIG_PM_RUNTIME_HDMITX
		if (runtime_put == TRUE) {
			pm_runtime_put(sp_hdmitx->dev);
			runtime_put = FALSE;
			runtime_put1 = TRUE;
		}
		
		msleep(2000);

		if (runtime_put1 == TRUE) {
			runtime_put = TRUE ;
			runtime_put1 = FALSE;
			
			ret = pm_runtime_get_sync(sp_hdmitx->dev);
			if (ret < 0) {
				pm_runtime_mark_last_busy(sp_hdmitx->dev);
			}
		}
#endif
	}	
}

static void process_rsen_state(void)
{
	HDMITX_DBG("RSEN State\n");

	mutex_lock(&g_hdmitx_mutex);
	if (get_hpd_in() && get_rx_ready()) {
		// apply A/V configuration
		stop();
		set_hdmi_mode(g_cur_hdmi_cfg.mode);
		config_video(&g_cur_hdmi_cfg.video);
		config_audio(&g_cur_hdmi_cfg.audio);
		start();
		// update state
		g_hdmitx_state = FSM_HDCP;
	} else {
		// update state
		if (!get_hpd_in()) {

			HDMITX_DBG("hpd out\n");
			g_hdmitx_state = FSM_HPD;
		} else {

			HDMITX_DBG("rsen out\n");
			g_hdmitx_state = FSM_RSEN;
		}
	}
	mutex_unlock(&g_hdmitx_mutex);
}

static void process_hdcp_state(void)
{
	HDMITX_DBG("HDCP State\n");

#ifdef MODE_CHANGE
	if (get_hpd_in() && get_rx_ready() && (get_hdmitx_mode() == g_cur_hdmi_cfg.mode)) {
#else
	if (get_hpd_in() && get_rx_ready()) {
#endif
#ifdef HDCP_AUTH
		// auth.
		// clear AV mute
#else
		// clear AV mute
#endif
	} else {
		mutex_lock(&g_hdmitx_mutex);
		if (!get_hpd_in()) {
			HDMITX_DBG("hpd out\n");
			g_hdmitx_state = FSM_HPD;
		} else {
#ifdef MODE_CHANGE
			if (g_cur_hdmi_cfg.mode != get_hdmitx_mode()) {
				g_cur_hdmi_cfg.mode = get_hdmitx_mode();

				if (get_hdmitx_mode() == HDMITX_MODE_DVI)
					HDMITX_INFO("change to DVI mode\n");
				else if (get_hdmitx_mode() == HDMITX_MODE_HDMI)
					HDMITX_INFO("change to HDMI mode\n");
			}
			else
#endif
				HDMITX_DBG("rsen out\n");

			g_hdmitx_state = FSM_RSEN;
		}
		mutex_unlock(&g_hdmitx_mutex);
	}
}

static irqreturn_t hdmitx_irq_handler(int irq, void *data)
{
	unsigned int cnt;
	
	if (hal_hdmitx_get_interrupt0_status(INTERRUPT0_HDP, sp_hdmitx->hdmitxbase)) {

		if (hal_hdmitx_get_system_status(SYSTEM_STUS_HPD_IN, sp_hdmitx->hdmitxbase)) {
			g_hpd_in = TRUE;
			HDMITX_INFO("HDMI plug in\n");
		} else {
			g_hpd_in = FALSE;
			HDMITX_INFO("HDMI plug out\n");
		}

		hal_hdmitx_clear_interrupt0_status(INTERRUPT0_HDP, sp_hdmitx->hdmitxbase);
	}

	if (hal_hdmitx_get_interrupt0_status(INTERRUPT0_RSEN, sp_hdmitx->hdmitxbase)) {

		if (hal_hdmitx_get_system_status(SYSTEM_STUS_RSEN_IN, sp_hdmitx->hdmitxbase)) {
			g_rx_ready = TRUE;
			HDMITX_INFO("HDMI rsen in\n");
		} else {
			g_rx_ready = FALSE;
			HDMITX_INFO("HDMI rsen out\n");
		}

		hal_hdmitx_clear_interrupt0_status(INTERRUPT0_RSEN, sp_hdmitx->hdmitxbase);
	}
	
	if (hal_hdmitx_get_interrupt1_status(INTERRUPT1_DDC_FIFO_FULL, sp_hdmitx->hdmitxbase)) {
		
		if (hal_hdmitx_get_ddc_status(DDC_STUS_FIFO_FULL, sp_hdmitx->hdmitxbase)) {
			edid_data_ofs += hal_hdmitx_get_fifodata_cnt(sp_hdmitx->hdmitxbase);
			for (cnt = edid_data_ofs - DDC_FIFO_CAPACITY; cnt < edid_data_ofs; cnt++) {
				edid[cnt] = hal_hdmitx_get_edid(sp_hdmitx->hdmitxbase);
			}
		}

		hal_hdmitx_clear_interrupt1_status(INTERRUPT1_DDC_FIFO_FULL, sp_hdmitx->hdmitxbase);
	}	
	
	return IRQ_HANDLED;
}

static int hdmitx_state_handler(void *data)
{
	while (!kthread_should_stop()) {

		switch (g_hdmitx_state) {
			case FSM_HPD:
				process_hpd_state();
				break;
			case FSM_RSEN:
				process_rsen_state();
				break;
			case FSM_HDCP:
				process_hdcp_state();
				break;
			default:
				break;
		}

		msleep(1);
	}

	return 0;
}

void hdmitx_set_timming(enum hdmitx_timing timing)
{
	mutex_lock(&g_hdmitx_mutex);
	g_new_hdmi_cfg.video.timing = timing;
	mutex_unlock(&g_hdmitx_mutex);
}

void hdmitx_get_timming(enum hdmitx_timing *timing)
{
	mutex_lock(&g_hdmitx_mutex);
	*timing = g_cur_hdmi_cfg.video.timing;
	mutex_unlock(&g_hdmitx_mutex);
}

void hdmitx_set_color_depth(enum hdmitx_color_depth color_depth)
{
	mutex_lock(&g_hdmitx_mutex);
	g_new_hdmi_cfg.video.color_depth = color_depth;
	mutex_unlock(&g_hdmitx_mutex);
}

void hdmitx_get_color_depth(enum hdmitx_color_depth *color_depth)
{
	mutex_lock(&g_hdmitx_mutex);
	*color_depth = g_cur_hdmi_cfg.video.color_depth;
	mutex_unlock(&g_hdmitx_mutex);
}

unsigned char hdmitx_get_rx_ready(void)
{
	return get_rx_ready();
}

int hdmitx_enable_display(int enforced)
{
	int err = 0;

	if (get_rx_ready() || enforced == 1) {

		mutex_lock(&g_hdmitx_mutex);
		memcpy(&g_cur_hdmi_cfg, &g_new_hdmi_cfg, sizeof(struct hdmitx_config));
		g_hdmitx_state = FSM_RSEN;
		mutex_unlock(&g_hdmitx_mutex);
	} else {
		err = -1;
	}

	return err;
}

void hdmitx_disable_display(void)
{
	stop();
}

void hdmitx_enable_pattern(void)
{
	hal_hdmitx_enable_pattern(sp_hdmitx->hdmitxbase);
}

void hdmitx_disable_pattern(void)
{
	hal_hdmitx_disable_pattern(sp_hdmitx->hdmitxbase);
}

EXPORT_SYMBOL(hdmitx_enable_display);
EXPORT_SYMBOL(hdmitx_disable_display);
EXPORT_SYMBOL(hdmitx_set_timming);

static int hdmitx_fops_open(struct inode *inode, struct file *pfile)
{
	int minor, err = 0;

	minor = iminor(inode);

	if (minor != g_hdmitx_misc.minor) {
		HDMITX_ERR("invalid inode\n");
		pfile->private_data = NULL;
		err = -1;
	}

	return err;
}

static int hdmitx_fops_release(struct inode *inode, struct file *pfile)
{
	return 0;
}

static long hdmitx_fops_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	enum hdmitx_timing timing;
	enum hdmitx_color_depth color_depth;
	unsigned char rx_ready, enable;

	switch (cmd) {

		case HDMITXIO_SET_TIMING:
			if (copy_from_user((void*) &timing, (const void __user *) arg, sizeof(enum hdmitx_timing))) {
				err = -1;
			} else {
				hdmitx_set_timming(timing);
			}
			break;
		case HDMITXIO_GET_TIMING:
			hdmitx_get_timming(&timing);
			if (copy_to_user((void __user *) arg, (const void *) &timing, sizeof(enum hdmitx_timing))) {
				err = -1;
			}
			break;
		case HDMITXIO_SET_COLOR_DEPTH:
			if (copy_from_user((void*) &color_depth, (const void __user *) arg, sizeof(enum hdmitx_color_depth))) {
				err = -1;
			} else {
				hdmitx_set_color_depth(color_depth);
			}
			break;
		case HDMITXIO_GET_COLOR_DEPTH:
			hdmitx_get_color_depth(&color_depth);
			if (copy_to_user((void __user *) arg, (const void *) &color_depth, sizeof(enum hdmitx_color_depth))) {
				err = -1;
			}
			break;
		case HDMITXIO_GET_RX_READY:
			rx_ready = hdmitx_get_rx_ready();
			if (copy_to_user((void __user *) arg, (const void *) &rx_ready, sizeof(unsigned char))) {
				err = -1;
			}
			break;
		case HDMITXIO_DISPLAY:
			if (copy_from_user((void*) &enable, (const void __user *) arg, sizeof(unsigned char))) {
				err = -1;
			} else {
				if (enable) {
					hdmitx_enable_display(0);
				} else {
					hdmitx_disable_display();
				}
			}
			break;
		case HDMITXIO_PTG:
			if (copy_from_user((void*) &enable, (const void __user *) arg, sizeof(unsigned char))) {
				err = -1;
			} else {
				if (enable) {
					hdmitx_enable_pattern();
				} else {
					hdmitx_disable_pattern();
				}
			}
			break;
		default:
			HDMITX_ERR("Invalid hdmitx ioctl commnad\n");
			err = -1;
			break;
	}

	return -1;
}

static int hdmitx_probe(struct platform_device *pdev)
{
	int err, irq;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match) {
		return -EINVAL;
	}

	sp_hdmitx = (sp_hdmitx_t *)devm_kzalloc(&pdev->dev, sizeof(sp_hdmitx_t), GFP_KERNEL);
	if (!sp_hdmitx) {
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sp_hdmitx->hdmitxbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(sp_hdmitx->hdmitxbase)) {
		return PTR_ERR(sp_hdmitx->hdmitxbase);
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	sp_hdmitx->moon4base = devm_ioremap_resource(dev, res);
	if (IS_ERR(sp_hdmitx->moon4base)) {
		return PTR_ERR(sp_hdmitx->moon4base);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	sp_hdmitx->moon5base = devm_ioremap_resource(dev, res);
	if (IS_ERR(sp_hdmitx->moon5base)) {
		return PTR_ERR(sp_hdmitx->moon5base);
	}	

//	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
//	sp_hdmitx->moon1base = devm_ioremap_resource(dev, res);
//	if (IS_ERR(sp_hdmitx->moon1base)) {
//		return PTR_ERR(sp_hdmitx->moon1base);
//	}

	sp_hdmitx->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(sp_hdmitx->clk)) {
		return PTR_ERR(sp_hdmitx->clk);
	}

	ret = clk_prepare(sp_hdmitx->clk);
	if (ret < 0) {
		dev_err(dev, "failed to prepare clk: %d\n", ret);
		return ret;
	}
	clk_enable(sp_hdmitx->clk);
	
	sp_hdmitx->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(sp_hdmitx->rstc)) {
		dev_err(&pdev->dev, "Failed to retrieve reset controller!\n");
		return PTR_ERR(sp_hdmitx->rstc);
	}

	ret = reset_control_deassert(sp_hdmitx->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to deassert reset line (err = %d)!\n", ret);
		return -ENODEV;
	}

	HDMITX_INFO("HDMITX installed\n");

	/*initialize hardware settings*/	
	hal_hdmitx_init(sp_hdmitx->moon1base, sp_hdmitx->hdmitxbase);

	/*initialize software settings*/
	// reset hdmi config
#ifdef CONFIG_HDMI_MODE
	g_cur_hdmi_cfg.mode = HDMITX_MODE_HDMI;
	#ifdef MODE_CHANGE
	g_hdmitx_mode = 1;
	#endif
#endif

#ifdef CONFIG_DVI_MODE
	g_cur_hdmi_cfg.mode = HDMITX_MODE_DVI;
	#ifdef MODE_CHANGE
	g_hdmitx_mode = 0;
	#endif
#endif

	g_cur_hdmi_cfg.video.timing      = HDMITX_TIMING_480P;
	g_cur_hdmi_cfg.video.color_depth = HDMITX_COLOR_DEPTH_24BITS;
	g_cur_hdmi_cfg.video.conversion  = HDMITX_COLOR_SPACE_CONV_LIMITED_YUV444_TO_LIMITED_RGB;
	g_cur_hdmi_cfg.audio.chl         = HDMITX_AUDIO_CHL_I2S;
	g_cur_hdmi_cfg.audio.type        = HDMITX_AUDIO_TYPE_LPCM;
	g_cur_hdmi_cfg.audio.sample_size = HDMITX_AUDIO_SAMPLE_SIZE_16BITS;
	g_cur_hdmi_cfg.audio.layout      = HDMITX_AUDIO_LAYOUT_2CH;
	g_cur_hdmi_cfg.audio.sample_rate = HDMITX_AUDIO_SAMPLE_RATE_48000HZ;
	memcpy(&g_new_hdmi_cfg, &g_cur_hdmi_cfg, sizeof(struct hdmitx_config));

	// request irq
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		HDMITX_ERR("platform_get_irq failed\n");
		return -ENODEV;
	}

	err = devm_request_irq(&pdev->dev, irq, hdmitx_irq_handler, IRQF_TRIGGER_HIGH, "hdmitx irq", pdev);
	if (err) {
		HDMITX_ERR("devm_request_irq failed: %d\n", err);
		return err;
	}
	
	// create thread
	g_hdmitx_task = kthread_run(hdmitx_state_handler, NULL, "hdmitx_task");
	if (IS_ERR(g_hdmitx_task)) {
		err = PTR_ERR(g_hdmitx_task);
		HDMITX_ERR("kthread_run failed: %d\n", err);
		return err;
	}

	// registry device
	sp_hdmitx->dev = dev;
	sp_hdmitx->hdmitx_misc = &g_hdmitx_misc;
	err = misc_register(&g_hdmitx_misc);
	if (err) {
		HDMITX_ERR("misc_register failed: %d\n", err);
		return err;
	}
	
	// init mutex
	mutex_init(&g_hdmitx_mutex);

	//
	wake_up_process(g_hdmitx_task);
	g_hdmitx_state = FSM_HPD;

	platform_set_drvdata(pdev, sp_hdmitx->hdmitx_misc);

#ifdef CONFIG_PM_RUNTIME_HDMITX
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif

	return 0;
}

static int hdmitx_remove(struct platform_device *pdev)
{
	struct miscdevice *misc = platform_get_drvdata(pdev);
	int err = 0;

	/*deinitialize hardware settings*/
	hal_hdmitx_deinit(sp_hdmitx->hdmitxbase);
	
	/*deinitialize software settings*/
	if (g_hdmitx_task) {
		err = kthread_stop(g_hdmitx_task);
		if (err) {
			HDMITX_ERR("kthread_stop failed: %d\n", err);
		} else {
			g_hdmitx_task = NULL;
		}
	}

	g_hdmitx_state = FSM_INIT;
	misc_deregister(misc);
	reset_control_assert(sp_hdmitx->rstc);

#ifndef CONFIG_PM_RUNTIME_HDMITX
	clk_disable(sp_hdmitx->clk);
#else 
	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
#endif

	HDMITX_INFO("HDMITX uninstalled\n");
	
	return err;
}

static int hdmitx_suspend(struct platform_device *pdev, pm_message_t state)
{
	reset_control_assert(sp_hdmitx->rstc);
	
	return 0;
}

static int hdmitx_resume(struct platform_device *pdev)
{
	reset_control_deassert(sp_hdmitx->rstc);

	return 0;
}

#ifdef CONFIG_PM_RUNTIME_HDMITX
static int sp_hdmitx_runtime_suspend(struct device *dev)
{
	clk_disable(sp_hdmitx->clk);
	HDMITX_DBG("RPM SUSPEND\n");
	
	return 0;
}

static int sp_hdmitx_runtime_resume(struct device *dev)
{
	clk_enable(sp_hdmitx->clk);
	HDMITX_DBG("RPM RESUME\n");
	
	return 0;
}
#endif

MODULE_DESCRIPTION("HDMITX driver");
MODULE_LICENSE("GPL");
