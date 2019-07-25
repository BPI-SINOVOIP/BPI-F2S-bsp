#ifndef __SP_BOOTINFO_SC7XXX_H
#define __SP_BOOTINFO_SC7XXX_H

#include <asm/arch/sp_bootmode_bitmap_sc7xxx.h>

/* copy from iboot "include/common.h" */
struct sp_bootinfo {
	u32     bootrom_ver;         // iboot version
	u32     hw_bootmode;         // hw boot mode (latched: auto, nand, usb_isp, sd_isp, etc)
	u32     gbootRom_boot_mode;  // sw boot mode (category: nand, sd, usb)
	u32     bootdev;             // boot device (exact device: sd0, sd1, ...)
	u32     bootdev_pinx;        // boot device pinmux
	u32     bootdev_port;        // usb0~1, sd0~1
	u32     app_blk_start;       // the block after xboot block(s)
	u32     mp_flag;             // mp machine flag
	u32     bootcpu;             // 0: B, 1: A
};

/* copy from iboot "include/common.h" */
enum Device_table {
	DEVICE_PARA_NAND = 0,
	DEVICE_USB_ISP,
	DEVICE_EMMC,
	DEVICE_SDCARD,
	DEVICE_UART_ISP,
	DEVICE_SPI_NOR,
	DEVICE_SPI_NAND,
	DEVICE_MAX
};

#define SP_GET_BOOTINFO()      ((struct sp_bootinfo *)(SP_BOOTINFO_BASE & 0xffffff00 ))

#define SP_IS_ISPBOOT()       (SP_GET_BOOTINFO()->gbootRom_boot_mode == USB_ISP || \
                               SP_GET_BOOTINFO()->gbootRom_boot_mode == SDCARD_ISP || \
                               SP_GET_BOOTINFO()->gbootRom_boot_mode == UART_ISP )

#endif /* __SP_BOOTINFO_SC7XXX_H */
