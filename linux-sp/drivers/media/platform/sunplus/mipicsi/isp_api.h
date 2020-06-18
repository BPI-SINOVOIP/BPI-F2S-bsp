#ifndef __SP_ISPAPB_H__
#define __SP_ISPAPB_H__

#include <linux/types.h>
#include <linux/kernel.h>

#define ISPAPB_DEBUG_ON
#ifdef ISPAPB_DEBUG_ON
#define ISPAPB_TAG "[ISP-APB] "
#define ISPAPB_LOGE(fmt, ...) printk(KERN_ERR ISPAPB_TAG fmt,##__VA_ARGS__)
#define ISPAPB_LOGW(fmt, ...) printk(KERN_WARNING ISPAPB_TAG fmt,##__VA_ARGS__)
#define ISPAPB_LOGI(fmt, ...) printk(KERN_INFO ISPAPB_TAG fmt,##__VA_ARGS__)
#define ISPAPB_LOGD(fmt, ...) printk(KERN_DEBUG ISPAPB_TAG fmt,##__VA_ARGS__)
#else
#define ISPAPB_LOGE(fmt, ...)  do{}while(0)
#define ISPAPB_LOGW(fmt, ...)  do{}while(0)
#define ISPAPB_LOGI(fmt, ...)  do{}while(0)
#define ISPAPB_LOGD(fmt, ...)  do{}while(0)
#endif

/* ISP Register */
#if 0 // Replace it with DRAM for debugging code 
#define	ISPAPB0_BASE_ADDR 0x01000000
#define	ISPAPB1_BASE_ADDR 0x01002000

#define ISPAPB0_REG8(addr) *((volatile unsigned char *)(addr+ISPAPB0_BASE_ADDR-0x2000))
#define ISPAPB1_REG8(addr) *((volatile unsigned char *)(addr+ISPAPB1_BASE_ADDR-0x2000))
#else
#define	ISPAPB0_BASE_ADDR 0x9C153000
#define	ISPAPB1_BASE_ADDR 0x9C155000

#define ISPAPB0_REG8(addr)  *((volatile u8 *) (addr+ISPAPB0_BASE_ADDR-0x2000))
#define ISPAPB0_REG16(addr) *((volatile u16 *)(addr+ISPAPB0_BASE_ADDR-0x2000))
#define ISPAPB0_REG32(addr) *((volatile u32 *)(addr+ISPAPB0_BASE_ADDR-0x2000))
#define ISPAPB1_REG8(addr)  *((volatile u8 *) (addr+ISPAPB1_BASE_ADDR-0x2000))
#define ISPAPB1_REG16(addr) *((volatile u16 *)(addr+ISPAPB1_BASE_ADDR-0x2000))
#define ISPAPB1_REG32(addr) *((volatile u32 *)(addr+ISPAPB1_BASE_ADDR-0x2000))
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
	SCALE_DOWN_FHD_VGA,
	SCALE_DOWN_FHD_QVGA,
};

/* Image information definition */
struct mipi_isp_info {
	struct mipi_isp_reg *mipi_isp_regs;

	u8                  isp_channel;
	u8                  isp_mode;
	u8                  test_pattern;
	u8                  probe;

	u16                 width;
	u16                 height;
	u8                  input_fmt;
	u8                  output_fmt;
	u8                  scale;
};


/* Function Prototype */
void isp_setting(struct mipi_isp_info *isp_info);

#endif /* __SP_ISPAPB_H */
