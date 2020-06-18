#ifndef __SP_MIPI_H__
#define __SP_MIPI_H__

#include <media/v4l2-dev.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include "isp_api.h"
#include "reg_mipi.h"


#define MIPI_CSI_RX_NAME                "sp_mipi_csi-rx"
#define MIPI_ISP_REG_NAME               "mipi_isp"
#define CSIIW_REG_NAME                  "csi_iw"
#define MOON5_REG_NAME                  "moon5"

#define mEXTENDED_ALIGNED(w,n)          (w%n)? ((w/n)*n+n): (w)
#define MIN_BUFFERS                     2

/* Device attribute definitions in sysfs */
#define SYSFS_MIPI_CSIIW_ATTRIBUTE      1
#define SYSFS_MIPI_ISP_ATTRIBUTE        1


/* SOL Sync */
#define SYNC_RGB565                     0x22
#define SYNC_RGB888                     0x24
#define SYNC_RAW8                       0x2A
#define SYNC_RAW10                      0x2B
#define SYNC_YUY2                       0x1E


/* Message Definition */
#define MIPI_FUNC_DEBUG
#define MIPI_FUNC_INFO
#define MIPI_FUNC_ERR

#ifdef MIPI_FUNC_DEBUG
#define MIPI_DBG(fmt, args ...)         printk(KERN_INFO "[MIPI] DBG: " fmt, ## args)
#else
#define MIPI_DBG(fmt, args ...)
#endif
#ifdef MIPI_FUNC_INFO
#define MIPI_INFO(fmt, args ...)        printk(KERN_INFO "[MIPI] INFO: " fmt, ## args)
#else
#define MIPI_INFO(fmt, args ...)
#endif
#ifdef MIPI_FUNC_ERR
#define MIPI_ERR(fmt, args ...)         printk(KERN_ERR "[MIPI] ERR: " fmt, ## args)
#else
#define MIPI_ERR(fmt, args ...)
#endif

#if 0
#define print_List()
#else
static void print_List(struct list_head *head){
	struct list_head *listptr;
	struct videobuf_buffer *entry;

	MIPI_DBG("*********************************************************************************\n");
	MIPI_DBG("(HEAD addr =  %p, next = %p, prev = %p)\n", head, head->next, head->prev);
	list_for_each(listptr, head) {
		entry = list_entry(listptr, struct videobuf_buffer, stream);
		MIPI_DBG("list addr = %p | next = %p | prev = %p\n", &entry->stream, entry->stream.next,
			 entry->stream.prev);
	}
	MIPI_DBG("*********************************************************************************\n");
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
	struct mipi_isp_reg             *mipi_isp_regs;
	struct csiiw_reg                *csiiw_regs;
	struct moon5_reg                *moon5_regs;
	struct clk                      *clkc_csiiw;
	struct reset_control            *rstc_csiiw;
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
	struct mipi_isp_info            isp_info;               /* MIPI ISP info */
	const struct sp_fmt             *cur_format;
	int                             cur_mode;

	spinlock_t                      dma_queue_lock;         /* IRQ lock for DMA queue */
	struct mutex                    lock;                   /* lock used to access this structure */

	int                             fs_irq;
	int                             fe_irq;
	bool                            streaming;              /* Indicates whether streaming started */
	bool                            skip_first_int;
	struct gpio_desc                *cam_gpio0;
	struct gpio_desc                *cam_gpio1;
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

#endif
