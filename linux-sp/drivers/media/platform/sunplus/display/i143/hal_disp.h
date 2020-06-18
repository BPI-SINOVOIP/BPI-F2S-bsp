#ifndef __HAL_DISP_H__
#define __HAL_DISP_H__
/**
 * @file hal_disp.h
 * @brief
 * @author Hammer Hsieh
 */

#include <linux/dma-mapping.h>

//#define SP_DISP_VPP_FIXED_ADDR
#define	SP_DISP_OSD_PARM
#define V4L2_TEST_DQBUF

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
#define sp_disp_err(fmt, args...)		printk(KERN_INFO "[DISP][Err]"fmt, ##args)
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

struct sp_disp_device {
	void *pHWRegBase;

	display_size_t		UIRes;
	UINT32				UIFmt;

	//OSD
	//spinlock_t osd_lock;
	//wait_queue_head_t osd_wait;
	//UINT32 osd_field_end_protect;

	//clk
	struct clk *tgen_clk;
	struct clk *dmix_clk;
	struct clk *osd0_clk;
	struct clk *gpost0_clk;
	struct clk *osd1_clk;
	struct clk *gpost1_clk;
	struct clk *vpost_clk;
	struct clk *ddfch_clk;
	struct clk *vppdma_clk;
	struct clk *vscl_clk;
	struct clk *dve_clk;
	struct clk *hdmi_clk;

	struct reset_control *rstc;
	
	display_size_t		panelRes;
	
	//#ifdef SP_DISP_OSD_PARM
	void *Osd0Header;
	u64 Osd0Header_phy;
	void *Osd1Header;
	u64 Osd1Header_phy;
	//#endif
	
	struct device *pdev; /*parent device */
	struct mutex	lock;
	spinlock_t		dma_queue_lock;

};

extern struct sp_disp_device *gDispWorkMem;

#endif	//__HAL_DISP_H__

