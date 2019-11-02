#ifndef __SP_BOOTMODE_BITMAP_8388_H
#define __SP_BOOTMODE_BITMAP_8388_H

/* 
 * This head is included by config header in include/configs/<soc>.h 
 * DO NOT put struct/function definition in this file. 
 */

/* copy from iboot "include/config.h" */
#define PLL_BYPASS		0x00
#define AUTO_SCAN		0x01
#define SDCARD_ISP		0x05 /*new*/
#define SPI_NAND_BOOT		0x09 /*new*/
#define UART_ISP		0x0D /*new*/
#define SPI_NOR_X2_BOOT		0x11 /*new, debug*/
#define SPI_NAND_X2_BOOT	0x15 /*new, debug*/
#define USB_ISP			0x19
#define TEST_BYPASS		0x1C
#define NAND_LARGE_BOOT		0x1D
/* sw boot mode only */
#define EMMC_BOOT		0xA3
#define CARD0_ISP		0xA5
#define CARD1_ISP		0xA6
#define CARD2_ISP		0xA7
#define CARD3_ISP		0xA8
#define SPI_NOR_BOOT		0x1F
#define USB_MSDC_BOOT		0x2F
#define NAND_BOOT		0x26
#define SPINAND_BOOT		0x46

/* where to get boot info */
#define SP_SRAM_BASE		0x9e800000
#define SP_BOOTINFO_BASE	0x9E80FC08 /* Careful! used by __stringify */

#endif /* __SP_BOOTMODE_BITMAP_8388_H */
