#ifndef __SP_VIDEOIN_H
#define __SP_VIDEOIN_H

#define VIDEOIN_DEBUG_ON
#ifdef VIDEOIN_DEBUG_ON
#define VI_TAG "[VIDEO-IN] "
#define VIDEOIN_LOGE(fmt, ...) printk(KERN_ERR VI_TAG fmt,##__VA_ARGS__)
#define VIDEOIN_LOGW(fmt, ...) printk(KERN_WARNING VI_TAG fmt,##__VA_ARGS__)
#define VIDEOIN_LOGI(fmt, ...) printk(KERN_INFO VI_TAG fmt,##__VA_ARGS__)
#define VIDEOIN_LOGD(fmt, ...) printk(KERN_DEBUG VI_TAG fmt,##__VA_ARGS__)
#else
#define VIDEOIN_LOGE(fmt, ...)  do{}while(0)
#define VIDEOIN_LOGW(fmt, ...)  do{}while(0)
#define VIDEOIN_LOGI(fmt, ...)  do{}while(0)
#define VIDEOIN_LOGD(fmt, ...)  do{}while(0)
#endif

/* MOON 0 Register */
#define MOON0_BASE_ADDR  0x9C000000
struct moon0_regs {
	unsigned int stamp;            // 0.0
	unsigned int clken[10];        // 0.1
	unsigned int gclken[10];       // 0.11
	unsigned int reset[10];        // 0.21
	unsigned int sft_cfg_mode;     // 0.31
};
#define MOON0_REG ((volatile struct moon0_regs *) MOON0_BASE_ADDR)

/* MOON 4 Register */
#define MOON4_BASE_ADDR  0x9C000200
struct moon4_regs {
	unsigned int reserved1[15];		// 4.0-14
	unsigned int plltv_ctl[4];		// 4.15-18
	unsigned int reserved2[13];		// 4.19-31
};
#define MOON4_REG ((volatile struct moon4_regs *) MOON4_BASE_ADDR)

/* MOON 5 Register */
#define MOON5_BASE_ADDR   0x9c000280
struct moon5_regs {
	unsigned int cfg[7];           // 5.0
};
#define MOON5_REG ((volatile struct moon5_regs *) MOON5_BASE_ADDR)

/* CSI IW Register */
#define CSI_IW0_BASE_ADDR 0x9C005300
#define CSI_IW1_BASE_ADDR 0x9C005400

struct csi_iw_regs {
	u32 csiiw_latch_mode;       // 166.0
	u32 csiiw_config0;          // 166.1
	u32 csiiw_base_addr;        // 166.2
	u32 csiiw_stride;           // 166.3
	u32 csiiw_frame_size;       // 166.4
	u32 csiiw_frame_buf;        // 166.5
	u32 csiiw_config1;          // 166.6
	u32 csiiw_frame_size_ro;    // 166.7
	u32 csiiw_reserved;         // 166.8
	u32 csiiw_config2;          // 166.9
	u32 csiiw_version;          // 166.10
};
#define CSI_IW0_REG ((volatile struct csi_iw_regs *) CSI_IW0_BASE_ADDR)
#define CSI_IW1_REG ((volatile struct csi_iw_regs *) CSI_IW1_BASE_ADDR)

// DRAM 0x20000000 ~ 0x3FFFFFFF
// 1920x1080 : 1920 * 2 bytes * 1080 = 4147200 = 0x3F4800 (4MBs)
#define DRAM_START_ADDRESS    0x20000000
#define BUFFER_OFFSET         0x00800000 // Reserve 8M bytes for xboot and uboot
#define BUFFER_SIZE           0x00400000 // 4M bytes for 1920x1080 yuv422 image
#define BUFFER1_START_ADDRESS DRAM_START_ADDRESS + BUFFER_OFFSET
#define BUFFER2_START_ADDRESS BUFFER1_START_ADDRESS + BUFFER_SIZE

/* Image information definition */
struct sp_videoin_info {
	u8  isp;
	u8  pattern;
	u16 x_len;
	u16 y_len;
	u8  out_fmt;
	u8  scale;
	u8  probe;
};

enum SP_YUV_ORDER_FORMAT {
	UYVY_ORDER,
	YUYV_ORDER,
};
#endif /* __SP_VIDEOIN_H */
