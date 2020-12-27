#ifndef __I2C_API_H
#define __I2C_API_H

//#define ISPI2C_DEBUG_ON
#ifdef ISPI2C_DEBUG_ON
#define ISPI2C_TAG "[ISP-I2C] "
#define ISPI2C_LOGE(fmt, ...) printk(KERN_ERR ISPI2C_TAG fmt,##__VA_ARGS__)
#define ISPI2C_LOGW(fmt, ...) printk(KERN_WARNING ISPI2C_TAG fmt,##__VA_ARGS__)
#define ISPI2C_LOGI(fmt, ...) printk(KERN_INFO ISPI2C_TAG fmt,##__VA_ARGS__)
#define ISPI2C_LOGD(fmt, ...) printk(KERN_DEBUG ISPI2C_TAG fmt,##__VA_ARGS__)
#else
#define ISPI2C_LOGE(fmt, ...)  do{}while(0)
#define ISPI2C_LOGW(fmt, ...)  do{}while(0)
#define ISPI2C_LOGI(fmt, ...)  do{}while(0)
#define ISPI2C_LOGD(fmt, ...)  do{}while(0)
#endif

/* Function Prototype */
void Reset_I2C0(void);
void Init_I2C0(u8 DevSlaveAddr, u8 speed_div);
void setSensor8_I2C0(u8 addr8, u16 value16);
void setSensor16_I2C0(u16 addr16, u16 value16, u8 data_count);
void getSensor8_I2C0(u8 addr8, u16 *value16);
void getSensor16_I2C0(u16 addr16, u16 *value16, u8 data_count);

void Reset_I2C1(void);
void Init_I2C1(u8 DevSlaveAddr, u8 speed_div);
void setSensor8_I2C1(u8 addr8, u16 value16);
void setSensor16_I2C1(u16 addr16, u16 value16, u8 data_count);
void getSensor8_I2C1(u8 addr8, u16 *value16);
void getSensor16_I2C1(u16 addr16, u16 *value16, u8 data_count);

#endif /* __I2C_API_H */
