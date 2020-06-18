#ifndef __FB_SP7021_H__
#define __FB_SP7021_H__

#ifdef __cplusplus
extern "C" {
#endif

/* #define DEBUG_MSG */

#define FB_PALETTE_LEN		(1024)

#define DISP_ALIGN(x, n)		(((x) + ((n) - 1)) & ~((n) - 1))

struct framebuffer_t {
	struct fb_info			*fb;

	int						fbwidth;
	int						fbheight;

	enum DRV_OsdRegionFormat_e	ColorFmt;
	char					ColorFmtName[24];
	int						fbpagesize;
	int						fbpagenum;
	int						fbsize;
	void __iomem			*fbmem;
	void __iomem			*fbmem_palette;

	u32						OsdHandle;
};

unsigned int sp7021_fb_chan_by_field(unsigned char chan,
	struct fb_bitfield *bf);
int sp7021_fb_swapbuf(u32 buf_id, int buf_max);

extern struct fb_info *gFB_INFO;

#ifdef __cplusplus
};
#endif

#endif	/* __FB_SP7021_H__ */

