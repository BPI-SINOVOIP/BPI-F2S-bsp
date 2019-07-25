#ifndef __SP_OTP_H
#define __SP_OTP_H
#include <common.h>


#define OTPRX_BASE_ADR                 0x9C00AF80
#define HB_GPIO                        0x9C00AF00


struct hbgpio_sunplus {
	u32 hb_gpio_rgst_bus32[13];
};

int read_otp_data(int addr, char *value);


#endif /* __SP_OTP_H */

