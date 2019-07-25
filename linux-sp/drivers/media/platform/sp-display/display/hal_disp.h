#ifndef __HAL_DISP_H__
#define __HAL_DISP_H__
/**
 * @file hal_disp.h
 * @brief
 * @author PoChou Chen
 */
#include "disp_dmix.h"
#include "disp_tgen.h"
#include "disp_dve.h"
#include "disp_osd.h"
#include "disp_vpp.h"

#define SP_DISP_V4L2_SUPPORT

#ifdef SP_DISP_V4L2_SUPPORT

#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <mach/io_map.h>
#include <media/videobuf-dma-contig.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>
#include <media/v4l2-common.h>

#endif
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define REG_START_Q628_B			(0x9c000000)

#define ALIGNED(x, n)				((x) & (~(n - 1)))
#define EXTENDED_ALIGNED(x, n)		(((x) + ((n) - 1)) & (~(n - 1)))

#define SWAP32(x)	((((UINT32)(x)) & 0x000000ff) << 24 \
					| (((UINT32)(x)) & 0x0000ff00) << 8 \
					| (((UINT32)(x)) & 0x00ff0000) >> 8 \
					| (((UINT32)(x)) & 0xff000000) >> 24)
#define SWAP16(x)	(((x) & 0x00ff) << 8 | ((x) >> 8))

#ifndef ENABLE
	#define ENABLE			1
#endif

#ifndef DISABLE
	#define DISABLE			0
#endif

#define diag_printf printk
extern int printk(const char *fmt, ...);

#define SUPPORT_DEBUG_MON

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct _display_size_t {
	UINT32 width;
	UINT32 height;
} display_size_t;

#ifdef SP_DISP_V4L2_SUPPORT
struct sp_fmt {
	char    *name;
	u32     fourcc;                                         /* v4l2 format id */
	int     width;
	int     height;
	int     walign;
	int     halign;
	int     depth;
	int     sol_sync;                                       /* sync of start of line */
};

struct sp_vout_layer_info {
	char                            name[32];               /* Sub device name */
	const struct sp_fmt             *formats;               /* pointer to video formats */
	int                             formats_size;           /* number of formats */
};

struct sp_disp_config {
	struct sp_vout_layer_info      *layer_devs;              /* information about each layer */
	int                             num_layerdevs;            /* Number of layer devices */
};

/* File handle structure */
struct sp_disp_fh {
	struct v4l2_fh fh;
	struct sp_disp_device *disp_dev;	
	u8 io_allowed;							/* Indicates whether this file handle is doing IO */
};
#endif

struct sp_disp_device {
	void *pHWRegBase;

	display_size_t		UIRes;
	UINT32				UIFmt;

	//OSD
	spinlock_t osd_lock;
	wait_queue_head_t osd_wait;
	UINT32 osd_field_end_protect;

	//void *aio;
	void *bio;

	//clk
	struct clk *tgen_clk;
	struct clk *dmix_clk;
	struct clk *osd0_clk;
	struct clk *gpost0_clk;
	struct clk *vpost_clk;
	struct clk *ddfch_clk;
	struct clk *dve_clk;
	struct clk *hdmi_clk;

	struct reset_control *rstc;
	
	display_size_t		panelRes;
#if 0
	DRV_Sys_OutMode_Info_t DispOutMode;
#endif

#ifdef SP_DISP_V4L2_SUPPORT
	struct device 			*pdev;			/* parent device */
	struct video_device 	video_dev;
	struct videobuf_buffer  *cur_frm;		/* Pointer pointing to current v4l2_buffer */
	struct videobuf_buffer  *next_frm;		/* Pointer pointing to next v4l2_buffer */
	struct videobuf_queue   buffer_queue;	/* Buffer queue used in video-buf */
	struct list_head	    dma_queue;		/* Queue of filled frames */

	struct v4l2_device 		v4l2_dev;   
	struct v4l2_format 		fmt;            /* Used to store pixel format */		
	struct v4l2_rect 		crop;
	struct v4l2_rect 		win;
	struct v4l2_control 	ctrl;
	enum v4l2_buf_type 		type;
	enum v4l2_memory 		memory;
	struct sp_vout_layer_info *current_layer;	/* ptr to currently selected sub device */
	struct sp_disp_config   *cfg;
	//const struct sp_fmt     *cur_format;

	spinlock_t irqlock;						/* Used in video-buf */	
	spinlock_t dma_queue_lock;				/* IRQ lock for DMA queue */	
	struct mutex lock;						/* lock used to access this structure */	

	int 					baddr;
	int 					fs_irq;
	int 					fe_irq;
	u32                     io_usrs;		/* number of users performing IO */
	u8 						started;		/* Indicates whether streaming started */
	u8 						capture_status;
//	u32 usrs;								/* number of open instances of the channel */	
//	u8 initialized;							/* flag to indicate whether decoder is initialized */
#endif
};

extern struct sp_disp_device gDispWorkMem;

#endif	//__HAL_DISP_H__

