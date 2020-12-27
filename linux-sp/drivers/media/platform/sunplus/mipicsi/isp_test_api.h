#ifndef __ISP_TEST_API_H__
#define __ISP_TEST_API_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include <media/v4l2-dev.h>
#include "isp_api.h"
#include "reg_mipi.h"

/* ISP Register Access */
#if 0 // These tegister access macro are used in uboot
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
void isp_test_setting(struct mipi_isp_info *isp_info);

#endif /* __ISP_TEST_API_H__ */
