#ifndef __SP_BOOTMODE_BITMAP_I143_H
#define __SP_BOOTMODE_BITMAP_I143_H

/*
 * This head is included by config header in include/configs/<soc>.h
 * DO NOT put struct/function definition in this file.
 */

/* copy from iboot "include/config.h" */
#define AUTO_SCAN               0x01
#define EMMC_BOOT               0x05
#define SPI_NOR_BOOT            0x07
#define SDCARD_ISP              0x11
#define UART_ISP                0x13
#define USB_ISP                 0x15

/* where to get boot info */
#define SP_BOOTINFO_BASE        0xfe806e08

#endif /* __SP_BOOTMODE_BITMAP_I143_H */
