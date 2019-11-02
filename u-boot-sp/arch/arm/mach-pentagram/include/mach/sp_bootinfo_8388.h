#ifndef __SP_BOOTINFO_8388_H
#define __SP_BOOTINFO_8388_H

#include <asm/arch/sp_bootmode_bitmap_8388.h>

struct sp_bootinfo {
	u32     bootrom_ver;         // iboot version
	u32     hw_bootmode;         // hw boot mode (latched: auto, nand, usb_isp, sd_isp, etc)
	u32     gbootRom_boot_mode;  // sw boot mode (category: nand, sd, usb)
	u32     bootdev;             // boot device (exact device: sd0, sd1, ...)
	u32     bootdev_pinx;        // boot device pinmux
	u32     app_blk_start;       // the block after xboot block(s)
	u32     mp_flag;             // mp machine flag
	// SDCard
	int     gSD_HIGHSPEED_EN_SET_val[4];
	int     gSD_RD_CLK_DELAY_TIME_val[4];
	// other fields are not used by u-boot
};

enum Device_table {
	DEVICE_NAND =0,
	DEVICE_NAND_SBLK, // depricated
	DEVICE_USB0_ISP,
	DEVICE_USB1_ISP,
	DEVICE_SD0,
	DEVICE_SD1,
	DEVICE_UART_ISP,
	DEVICE_SPI_NOR,
	DEVICE_SPI_NAND,
	DEVICE_USB_MSDC,
	DEVICE_EMMC,
	DEVICE_MAX
};

#define SP_GET_BOOTINFO()      ((struct sp_bootinfo *)SP_BOOTINFO_BASE)

#define SP_IS_ISPBOOT()       (SP_GET_BOOTINFO()->gbootRom_boot_mode == USB_ISP || \
			       SP_GET_BOOTINFO()->gbootRom_boot_mode == SDCARD_ISP || \
			       SP_GET_BOOTINFO()->gbootRom_boot_mode == UART_ISP || \
			       SP_GET_BOOTINFO()->gbootRom_boot_mode == USB_MSDC_BOOT)

#endif /* __SP_BOOTINFO_8388_H */
