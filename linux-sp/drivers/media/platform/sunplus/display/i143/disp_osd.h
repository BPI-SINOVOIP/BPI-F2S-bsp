#ifndef __DISP_OSD_H__
#define __DISP_OSD_H__

#include <linux/fb.h>

typedef enum {
	DRV_OSD0 = 0,
	DRV_OSD1,
	DRV_OSD_MAX
} DRV_OsdWindow_e;

enum DRV_OsdRegionFormat_e {
	/* 8 bit/pixel with CLUT */
	DRV_OSD_REGION_FORMAT_8BPP			= 0x2,
	/* 16 bit/pixel YUY2 */
	DRV_OSD_REGION_FORMAT_YUY2			= 0x4,
	/* 16 bit/pixel RGB 5:6:5 */
	DRV_OSD_REGION_FORMAT_RGB_565		= 0x8,
	/* 16 bit/pixel ARGB 1:5:5:5 */
	DRV_OSD_REGION_FORMAT_ARGB_1555		= 0x9,
	/* 16 bit/pixel RGBA 4:4:4:4 */
	DRV_OSD_REGION_FORMAT_RGBA_4444		= 0xA,
	/* 16 bit/pixel ARGB 4:4:4:4 */
	DRV_OSD_REGION_FORMAT_ARGB_4444		= 0xB,
	/* 32 bit/pixel RGBA 8:8:8:8 */
	DRV_OSD_REGION_FORMAT_RGBA_8888		= 0xD,
	/* 32 bit/pixel ARGB 8:8:8:8 */
	DRV_OSD_REGION_FORMAT_ARGB_8888		= 0xE,
};

enum DRV_OsdTransparencyMode_e {
	DRV_OSD_TRANSPARENCY_DISABLED = 0,	/*!< transparency is disabled */
	DRV_OSD_TRANSPARENCY_ALL			/*!< the whole region is transparent */
};

typedef u32 DRV_OsdRegionHandle_t;

enum DRV_OsdBlendMethod_e {
	DRV_OSD_BLEND_REPLACE,		/*!< OSD blend method is region alpha replace */
	DRV_OSD_BLEND_MULTIPLIER,	/*!< OSD blend method is region alpha multiplier */
	MAX_BLEND_METHOD,
};

typedef struct _DRV_Region_Info_t
{
	u32 bufW;
	u32 bufH;
	u32 startX;
	u32 startY;
	u32 actW;
	u32 actH;
} DRV_Region_Info_t;

struct colormode_t {
	char name[24];
	unsigned int bits_per_pixel;
	unsigned int nonstd;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

struct UI_FB_Info_t {
	unsigned int UI_width;
	unsigned int UI_height;
	unsigned int UI_bufNum;
	unsigned int UI_bufAlign;
	enum DRV_OsdRegionFormat_e UI_ColorFmt;
	struct colormode_t UI_Colormode;

	unsigned int UI_bufAddr;
	unsigned int UI_bufAddr_pal;		/* palette */
	unsigned int UI_bufsize;
	unsigned int UI_handle;
};

struct HW_OSD_Header_s {
	u8 config0;	//config0 includes:
	// [bit 7] cu	: color table update
	// [bit 6] ft	: force transparency
	// [bit 5:4]	: reserved
	// [bit 3:0] md : bitmap format (color mode)

	u8 reserved0; // reserved bits.

	u8 config1;	//config1 includes:
	// [bit 7:5]	: reserved
	// [bit 4] b_s	: byte swap enable
	// [bit 3] KY	: reserved
	// [bit 2] bl2	: region blend alpha enable (multiplier)
	// [bit 1]		: reserved
	// [bit 0] bl	: region blend alpha enable (replace)

	u8 blend_level;	//region blend level value

	u16 v_size;		//vertical display region size (line number)
	u16 h_size;		//horizontal display region size (pixel number)

	u16 disp_start_row;		//region vertical start row (0~(y-1))
	u16 disp_start_column;	//region horizontal start column (0~(x-1))

	u8 keying_R;
	u8 keying_G;
	u8 keying_B;
	u8 keying_A;

	u16 data_start_row;
	u16 data_start_column;

	u8 reserved2;
	u8 csc_mode_sel; //color space converter mode sel
	u16 data_width;	//source bitmap crop width

	u32 link_next;
	u32 link_data;

	u32 reserved3[24];	// need 128 bytes for HDR
};
//STATIC_ASSERT(sizeof(struct HW_OSD_Header_s) == 128);

void DRV_OSD_Init(void *pInHWReg1, void *pInHWReg2);
void DRV_OSD1_Init(void *pInHWReg1, void *pInHWReg2);

void DRV_OSD0_off(void);
void DRV_OSD0_on(void);
void DRV_OSD1_off(void);
void DRV_OSD1_on(void);

void DRV_OSD_SetColorbar(DRV_OsdWindow_e osd_layer, int enable);
int osd0_setting(int base_addr, int w, int h, int osd0_fmt);
int osd1_setting(int base_addr, int w, int h, int osd1_fmt);
void DRV_OSD_INIT_OSD_Header(void);
void DRV_OSD0_dump(void);
void DRV_OSD1_dump(void);

#endif	//__DISP_OSD_H__

