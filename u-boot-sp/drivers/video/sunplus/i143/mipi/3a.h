#ifndef __3A_H
#define __3A_H

#define FALSE   0
#define TURE    1

#define LO_BYTE_OF_WORD(x) (u8)(x&0x00ff)
#define HI_BYTE_OF_WORD(x) (u8)((x>>8)&0xff)

#define ISP3A_DEBUG_ON
#ifdef ISP3A_DEBUG_ON
#define ISP3A_TAG "[ISP-3A] "
#define ISP3A_LOGE(fmt, ...) printk(KERN_ERR ISP3A_TAG fmt,##__VA_ARGS__)
#define ISP3A_LOGW(fmt, ...) printk(KERN_WARNING ISP3A_TAG fmt,##__VA_ARGS__)
#define ISP3A_LOGI(fmt, ...) printk(KERN_INFO ISP3A_TAG fmt,##__VA_ARGS__)
#define ISP3A_LOGD(fmt, ...) printk(KERN_DEBUG ISP3A_TAG fmt,##__VA_ARGS__)
#else
#define ISP3A_LOGE(fmt, ...)  do{}while(0)
#define ISP3A_LOGW(fmt, ...)  do{}while(0)
#define ISP3A_LOGI(fmt, ...)  do{}while(0)
#define ISP3A_LOGD(fmt, ...)  do{}while(0)
#endif

// local function prototype
void aeGetCurEv(void);
void sensorSetExposureTimeIsr(void);
void sensorSetGainIsr(void);
void aeGetCurEv(void);

void InstallVSinterrupt(void);
void intrIntr0SensorVsync(void);
void aaaLoadInit(char *aaa_init_file);
void aeLoadAETbl(char *ae_table_file);
void aeLoadGainTbl(char *gain_table_file);

void aeInitExt(void);
void awbInit(void);

void vidctrlInit(u16 _sc2310_gain_addr, 
				u16 _sc2310_frame_len_addr, 
				u16 _sc2310_line_total_addr, 
				u16 _sc2310_exp_line_addr,
				u32 _sensor_pclk);
void aaaAeAwbAf(void);

#endif /* __3A_H */
