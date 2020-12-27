#ifndef __ISP_API_H
#define __ISP_API_H

#include "sp_videoin.h"


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

/* Function Prototype */
void isp_setting(struct sp_videoin_info vi_info);


/* Enumeration Definition for ISP Image */
enum ISP_PATH {
	ISP0_PATH,
	ISP1_PATH,
	ISP_ALL_PATH,
};

enum ISP_TEST_PATTERN {
	STILL_COLORBAR_YUV422_64X64,
	STILL_COLORBAR_YUV422_1920X1080,
	STILL_COLORBAR_RAW10_1920X1080,
	MOVING_COLORBAR_RAW10_1920X1080,
	SENSOR_INPUT = 0xFF,
};

enum ISP_OUTPUT_FORMAT {
	YUV422_FORMAT_UYVY_ORDER,
	YUV422_FORMAT_YUYV_ORDER,
	RAW8_FORMAT,
	RAW10_FORMAT,
	RAW10_FORMAT_PACK_MODE,
};

enum ISP_SCALE_SIZE {
	SCALE_DOWN_OFF,
	SCALE_DOWN_FHD_HD,
	SCALE_DOWN_FHD_WVGA,
	SCALE_DOWN_FHD_VGA,
	SCALE_DOWN_FHD_QQVGA,
};
#endif /* __ISP_API_H */
