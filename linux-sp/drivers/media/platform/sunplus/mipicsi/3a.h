#ifndef __3A_H__
#define __3A_H__

#include "isp_api.h"
#include "reg_mipi.h"

#define FALSE   0
#define TURE    1

#define LO_BYTE_OF_WORD(x) (u8)(x&0x00ff)
#define HI_BYTE_OF_WORD(x) (u8)((x>>8)&0xff)

// Log macro definition
//#define ISP3A_DEBUG_ON
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

// Function prototype
void aeGetCurEv(struct mipi_isp_info *isp_info);
void sensorSetExposureTimeIsr(struct mipi_isp_info *isp_info);
void sensorSetGainIsr(struct mipi_isp_info *isp_info);
void aeGetCurEv(struct mipi_isp_info *isp_info);

void InstallVSinterrupt(void);
void intrIntr0SensorVsync(struct mipi_isp_info *isp_info);
void aaaLoadInit(struct mipi_isp_info *isp_info, const char *aaa_init_file);
void aeLoadAETbl(struct mipi_isp_info *isp_info, const char *ae_table_file);
void aeLoadGainTbl(struct mipi_isp_info *isp_info, const char *gain_table_file);

void aeInitExt(struct mipi_isp_info *isp_info);
void awbInit(struct mipi_isp_info *isp_info);

void vidctrlInit(struct mipi_isp_info *isp_info,
				u16 _sc2310_gain_addr, 
				u16 _sc2310_frame_len_addr, 
				u16 _sc2310_line_total_addr, 
				u16 _sc2310_exp_line_addr,
				u32 _sensor_pclk);
void aaaInitVar(struct mipi_isp_info *isp_info);
void aaaAeAwbAf(struct mipi_isp_info *isp_info);

#endif /* __3A_H__ */
