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
#include <media/sp-disp/disp_osd.h>
#include <media/sp-disp/disp_vpp.h>

#define SP_DISP_V4L2_SUPPORT
//#define SP_DISP_VPP_FIXED_ADDR
#define	SP_DISP_OSD_PARM
#define V4L2_TEST_DQBUF

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
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-common.h>

#endif
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//#define SP_DISP_DEBUG

#if 0
#define sp_disp_err(fmt, args...)		printk(KERN_ERR "[DISP][Err][%s:%d]"fmt, __func__, __LINE__, ##args)
#define sp_disp_info(fmt, args...)		printk(KERN_INFO "[DISP][%s:%d]"fmt, __func__, __LINE__, ##args)
#ifdef SP_DISP_DEBUG
#define sp_disp_dbg(fmt, args...)		printk(KERN_INFO "[DISP][%s:%d]"fmt, __func__, __LINE__, ##args)
#else
#define sp_disp_dbg(fmt, args...)
#endif
#else
#define sp_disp_err(fmt, args...)		printk(KERN_ERR "[DISP][Err]"fmt, ##args)
#define sp_disp_info(fmt, args...)		printk(KERN_INFO "[DISP]"fmt, ##args)
#ifdef SP_DISP_DEBUG
#define sp_disp_dbg(fmt, args...)		printk(KERN_INFO "[DISP]"fmt, ##args)
#else
#define sp_disp_dbg(fmt, args...)
#endif
#endif

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

#ifdef SP_DISP_V4L2_SUPPORT
#define MIN_BUFFERS				2
#define	SP_DISP_MAX_DEVICES			3
#endif
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct _display_size_t {
	UINT32 width;
	UINT32 height;
} display_size_t;

typedef struct _ttl_spec_t {
	UINT32 dts_exist;
	UINT32 clk;
	UINT32 divm,divn;
	UINT32 hfp;
	UINT32 hsync;
	UINT32 hbp;
	UINT32 hactive;
	UINT32 vfp;
	UINT32 vsync;
	UINT32 vbp;
	UINT32 vactive;
	UINT32 ttl_rgb_swap;
	UINT32 ttl_clock_pol;
	UINT32 ttl_vpp_adj;
	UINT32 ttl_osd_adj;
	UINT32 ttl_parm_adj;
} ttl_spec_t;

#ifdef SP_DISP_V4L2_SUPPORT
enum sp_disp_device_id {
	SP_DISP_DEVICE_0,
	SP_DISP_DEVICE_1,
	SP_DISP_DEVICE_2
};
struct sp_disp_layer {
	/*for layer specific parameters */
	struct sp_disp_device	*disp_dev;		/* Pointer to the sp_disp_device */
	struct sp_disp_buffer   *cur_frm;		/* Pointer pointing to current v4l2_buffer */
	struct sp_disp_buffer   *next_frm;		/* Pointer pointing to next v4l2_buffer */
	struct vb2_queue   		buffer_queue;	/* Buffer queue used in video-buf2 */
	struct list_head	    dma_queue;		/* Queue of filled frames */
	spinlock_t				irqlock;		/* Used in video-buf */	
	struct video_device 	video_dev;

	struct v4l2_format 		fmt;            /* Used to store pixel format */
	unsigned int 			usrs;			/* number of open instances of the layer */
	struct mutex			opslock;		/* facilitation of ioctl ops lock by v4l2*/
	enum sp_disp_device_id	device_id;		/* Identifies device object */
	bool					skip_first_int; /* skip first int */
	bool					streaming; 		/* layer start_streaming */
	unsigned                sequence;

};
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

/* buffer for one video frame */
struct sp_disp_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_v4l2_buffer          vb;
	struct list_head                list;
};
#endif

struct sp_disp_device {
	void *pHWRegBase;
	void *bio;

	display_size_t		UIRes;
	UINT32				UIFmt;

	ttl_spec_t	TTLPar;

	//OSD
	spinlock_t osd_lock;
	wait_queue_head_t osd_wait;
	UINT32 osd_field_end_protect;

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

#ifdef SP_DISP_V4L2_SUPPORT

	#ifdef	SP_DISP_OSD_PARM
	u8					*Osd0Header;
	u8					*Osd1Header;
	u32 				Osd0Header_phy;
	u32 				Osd1Header_phy;
	#endif

	/* for device */
	struct v4l2_device 		v4l2_dev;		/* V4l2 device */
	struct device 			*pdev;			/* parent device */
	struct mutex			lock;			/* lock used to access this structure */
	spinlock_t				dma_queue_lock;	/* IRQ lock for DMA queue */

	struct sp_disp_layer	*dev[SP_DISP_MAX_DEVICES];
#endif
};

extern struct sp_disp_device *gDispWorkMem;

#endif	//__HAL_DISP_H__

