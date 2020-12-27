#ifndef __ISP_API_H__
#define __ISP_API_H__

#include "reg_mipi.h"
#include "3a_types.h"

/* Message Definition */
//#define ISPAPI_FUNC_DEBUG
//#define ISPAPI_FUNC_INFO
#define ISPAPI_FUNC_ERR

#define ISPAPI_TAG "[ISP-API] "
#ifdef ISPAPI_FUNC_DEBUG
#define ISPAPI_LOGD(fmt, ...) printk(KERN_DEBUG ISPAPI_TAG fmt,##__VA_ARGS__)
#else
#define ISPAPI_LOGD(fmt, ...)
#endif
#ifdef ISPAPI_FUNC_INFO
#define ISPAPI_LOGI(fmt, ...) printk(KERN_INFO ISPAPI_TAG fmt,##__VA_ARGS__)
#else
#define ISPAPI_LOGI(fmt, ...)
#endif
#ifdef ISPAPI_FUNC_ERR
#define ISPAPI_LOGE(fmt, ...) printk(KERN_ERR ISPAPI_TAG fmt,##__VA_ARGS__)
#else
#define ISPAPI_LOGE(fmt, ...)
#endif

/* Enumeration Definition for ISP Image */
enum ISP_PATH {
	ISP0_PATH,
	ISP1_PATH,
	ISP_ALL_PATH,
};

enum ISP_MODE {
	ISP_NORMAL_MODE,
	ISP_TEST_MODE,
};

enum ISP_TEST_PATTERN {
	STILL_WHITE,                    // 0
	STILL_YELLOW,                   // 1
	STILL_CYAN,                     // 2
	STILL_GREEN,                    // 3
	STILL_MAGENTA,                  // 4
	STILL_RED,                      // 5
	STILL_BLUE,                     // 6
	STILL_BLACK,                    // 7
	GRAY_LEVEL,                     // 8
	HCNT,                           // 9
	VCNT,                           // A
	COLOR_CHECKER,                  // B
	STILL_VERTICAL_COLOR_BAR,       // C
	STILL_HORIZONTAL_COLOR_BAR,     // D
	R_G_B_W_FLASHING,               // E
	MOVING_HORIZONTAL_COLOR_BAR,    // F
};

enum ISP_FORMAT {
	YUV422_FORMAT,              // 0x00
	YUV422_FORMAT_UYVY_ORDER,   // 0x01
	YUV422_FORMAT_YUYV_ORDER,   // 0x02
	RAW8_FORMAT = 0x10,         // 0x10
	RAW10_FORMAT = 0x20,        // 0x20
	RAW10_FORMAT_PACK_MODE,     // 0x21
};

enum ISP_SCALE_SIZE {
	SCALE_DOWN_OFF,
	SCALE_DOWN_FHD_HD,
	SCALE_DOWN_FHD_WVGA,
	SCALE_DOWN_FHD_VGA,
	SCALE_DOWN_FHD_QQVGA,
};

/* Image information definition */
struct mipi_isp_info {
	struct mipi_isp_reg *mipi_isp_regs;
	struct video_device *video_device;

	u8  isp_channel;
	u8  isp_mode;
	u8  test_pattern;
	u8  probe;

	u16 width;
	u16 height;
	u8  input_fmt;
	u8  output_fmt;
	u8  scale;

	// 3A function
	aaa_var_t aaa_var;
};

/* Function Prototype */
void ispVsyncIntCtrl(struct mipi_isp_info *isp_info, u8 on);
void ispVsyncInt(struct mipi_isp_info *isp_info);
void videoStartMode(struct mipi_isp_info *isp_info);
void videoStopMode(struct mipi_isp_info *isp_info);
void isp_setting(struct mipi_isp_info *isp_info);

#endif /* __ISP_API_H__ */
