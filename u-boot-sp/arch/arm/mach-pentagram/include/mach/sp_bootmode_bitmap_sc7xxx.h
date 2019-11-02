#ifndef __SP_BOOTMODE_BITMAP_SC7XXX_H
#define __SP_BOOTMODE_BITMAP_SC7XXX_H

/*
 * This head is included by config header in include/configs/<soc>.h
 * DO NOT put struct/function definition in this file.
 */

/* copy from iboot "include/config.h" */
#define AUTO_SCAN               0x01
#define AUTO_SCAN_ACHIP         0x15
#define SPI_NOR_BOOT            0x11
#define SPINAND_BOOT            0x09
#define EMMC_BOOT               0x1F
#define SDCARD_ISP              0x07
#define UART_ISP                0x0F
#define USB_ISP                 0x17
#define NAND_LARGE_BOOT         0xff /* Q628: no PARA_NAND */

/* where to get boot info */
#define SP_SRAM_BASE        0x9e800000
#define SP_BOOTINFO_BASE    0x9e809408

#endif /* __SP_BOOTMODE_BITMAP_SC7XXX_H */
