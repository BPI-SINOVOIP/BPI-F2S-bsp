#ifndef __SP_MIPI_H__
#define __SP_MIPI_H__

#include <media/v4l2-dev.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>


#define MIPI_CSI_RX_NAME                "sp_mipi_csi-rx"
#define MIPICSI_REG_NAME                "mipicsi"
#define CSIIW_REG_NAME                  "csiiw"

#define mEXTENDED_ALIGNED(w,n)          (w%n)? ((w/n)*n+n): (w)
#define MIN_BUFFERS                     2

/* SOL Sync */
#define SYNC_RGB565                     0x22
#define SYNC_RGB888                     0x24
#define SYNC_RAW8                       0x2A
#define SYNC_RAW10                      0x2B
#define SYNC_YUY2                       0x1E

#if 0
#define DBG_INFO(fmt, args ...)         printk(KERN_INFO "[MIPI] " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif
#define MIP_INFO(fmt, args ...)         printk(KERN_INFO "[MIPI] " fmt, ## args)
#define MIP_ERR(fmt, args ...)          printk(KERN_ERR "[MIPI] ERR: " fmt, ## args)

#if 0
#define print_List()
#else
static void print_List(struct list_head *head){
	struct list_head *listptr;
	struct videobuf_buffer *entry;

	DBG_INFO("*********************************************************************************\n");
	DBG_INFO("(HEAD addr =  %p, next = %p, prev = %p)\n", head, head->next, head->prev);
	list_for_each(listptr, head) {
		entry = list_entry(listptr, struct videobuf_buffer, stream);
		DBG_INFO("list addr = %p | next = %p | prev = %p\n", &entry->stream, entry->stream.next,
			 entry->stream.prev);
	}
	DBG_INFO("*********************************************************************************\n");
}
#endif


struct sp_fmt {
	char    *name;
	u32     fourcc;                                         /* v4l2 format id */
	int     width;
	int     height;
	int     walign;
	int     halign;
	int     depth;
	int     mipi_lane;                                      /* number of data lanes of mipi */
	int     sol_sync;                                       /* sync of start of line */
};

struct sp_mipi_subdev_info {
	char                            name[32];               /* Sub device name */
	int                             grp_id;                 /* Sub device group id */
	struct i2c_board_info           board_info;             /* i2c subdevice board info */
	const struct sp_fmt             *formats;               /* pointer to video formats */
	int                             formats_size;           /* number of formats */
};

struct sp_mipi_config {
	int                             i2c_adapter_id;         /* i2c bus adapter no */
	struct sp_mipi_subdev_info      *sub_devs;              /* information about each subdev */
	int                             num_subdevs;            /* Number of sub devices connected to vpfe */
};

struct sp_mipi_device {
	struct mipicsi_reg              *mipicsi_regs;
	struct csiiw_reg                *csiiw_regs;
	struct clk                      *mipicsi_clk;
	struct clk                      *csiiw_clk;
	struct reset_control            *mipicsi_rstc;
	struct reset_control            *csiiw_rstc;
	u32                             i2c_id;                 /* i2c id (channel) */

	struct device                   *pdev;                  /* parent device */
	struct video_device             video_dev;
	struct sp_buffer                *cur_frm;               /* Pointer pointing to current v4l2_buffer */
	struct vb2_queue                buffer_queue;           /* Buffer queue used in video-buf2 */
	struct list_head                dma_queue;              /* Queue of filled frames */

	struct v4l2_device              v4l2_dev;
	struct v4l2_format              fmt;                    /* Used to store pixel format */
	struct v4l2_rect                crop;
	struct v4l2_rect                win;
	u32                             caps;
	unsigned                        sequence;

	struct i2c_adapter              *i2c_adap;
	struct sp_mipi_subdev_info      *current_subdev;        /* pointer to currently selected sub device */
	struct sp_mipi_config           *cfg;
	const struct sp_fmt             *cur_format;
	int                             cur_mode;

	spinlock_t                      dma_queue_lock;         /* IRQ lock for DMA queue */
	struct mutex                    lock;                   /* lock used to access this structure */

	int                             fs_irq;
	int                             fe_irq;
	bool                            streaming;              /* Indicates whether streaming started */
	bool                            skip_first_int;
	int 				gpio0;
	int 				gpio1;
};

/* buffer for one video frame */
struct sp_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_v4l2_buffer          vb;
	struct list_head                list;
};

/* sensor subdev private data */
struct sp_subdev_sensor_data {
	int     mode;
	u32     fourcc;
};

/* mipi-csi registers */
struct mipicsi_reg {
	volatile unsigned int mipicsi_ststus;                   /* 00 (mipicsi) */
	volatile unsigned int mipi_debug0;                      /* 01 (mipicsi) */
	volatile unsigned int mipi_wc_lpf;                      /* 02 (mipicsi) */
	volatile unsigned int mipi_analog_cfg0;                 /* 03 (mipicsi) */
	volatile unsigned int mipi_analog_cfg1;                 /* 04 (mipicsi) */
	volatile unsigned int mipicsi_fsm_rst;                  /* 05 (mipicsi) */
	volatile unsigned int mipi_analog_cfg2;                 /* 06 (mipicsi) */
	volatile unsigned int mipicsi_enable;                   /* 07 (mipicsi) */
	volatile unsigned int mipicsi_mix_cfg;                  /* 08 (mipicsi) */
	volatile unsigned int mipicsi_delay_ctl;                /* 09 (mipicsi) */
	volatile unsigned int mipicsi_packet_size;              /* 10 (mipicsi) */
	volatile unsigned int mipicsi_sot_syncword;             /* 11 (mipicsi) */
	volatile unsigned int mipicsi_sof_sol_syncword;         /* 12 (mipicsi) */
	volatile unsigned int mipicsi_eof_eol_syncword;         /* 13 (mipicsi) */
	volatile unsigned int mipicsi_reserved_a14;             /* 14 (mipicsi) */
	volatile unsigned int mipicsi_reserved_a15;             /* 15 (mipicsi) */
	volatile unsigned int mipicsi_ecc_error;                /* 16 (mipicsi) */
	volatile unsigned int mipicsi_crc_error;                /* 17 (mipicsi) */
	volatile unsigned int mipicsi_ecc_cfg;                  /* 18 (mipicsi) */
	volatile unsigned int mipi_analog_cfg3;                 /* 19 (mipicsi) */
	volatile unsigned int mipi_analog_cfg4;                 /* 20 (mipicsi) */
};

/* mipi-csiiw registers */
struct csiiw_reg {
	volatile unsigned int csiiw_latch_mode;                 /* 00 (csiiw) */
	volatile unsigned int csiiw_config0;                    /* 01 (csiiw) */
	volatile unsigned int csiiw_base_addr;                  /* 02 (csiiw) */
	volatile unsigned int csiiw_stride;                     /* 03 (csiiw) */
	volatile unsigned int csiiw_frame_size;                 /* 04 (csiiw) */
	volatile unsigned int csiiw_frame_buf;                  /* 05 (csiiw) */
	volatile unsigned int csiiw_config1;                    /* 06 (csiiw) */
	volatile unsigned int csiiw_frame_size_ro;              /* 07 (csiiw) */
};

#endif
