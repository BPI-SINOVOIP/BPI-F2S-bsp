/*
 * SPDX-License-Identifier: GPL-2.0+
 */

/*
 * Note:
 *	Do NOT use "//" for comment, it will cause issue in u-boot.lds
 */

#ifndef __CONFIG_PENTAGRAM_H
#define __CONFIG_PENTAGRAM_H


#ifdef CONFIG_SYS_ZEBU_ENV
#define CONFIG_SYS_ZMEM_SKIP_RELOC	1
#ifdef CONFIG_SYS_ZMEM_SKIP_RELOC
#define CONFIG_SP_ZMEM_RELOC_ADDR	0xA3F00000
#endif
#endif


/* Disable some options which is enabled by default: */

#define CONFIG_SYS_SDRAM_BASE		0xA0000000

#define CONFIG_SYS_MALLOC_LEN		(8 << 20)


#ifdef CONFIG_SPL_BUILD
#ifndef CONFIG_SYS_UBOOT_START		/* default entry point */
#define CONFIG_SYS_UBOOT_START		CONFIG_SYS_TEXT_BASE
#endif
#endif

#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE+(1 << 20))	/* set it in DRAM area (not SRAM) because DRAM is ready before U-Boot executed */
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE+(6 << 20))	/* kernel loaded address */

#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			115200		/* the value doesn't matter, it's not change in U-Boot */
#endif
#define CONFIG_SYS_BAUDRATE_TABLE	{ 57600, 115200 }
/* #define CONFIG_SUNPLUS_SERIAL */

/* Main storage selection */
#if defined(CONFIG_SP_SPINAND)
#define SP_MAIN_STORAGE		"nand"
#elif defined(CONFIG_MMC_SP_EMMC)
#define SP_MAIN_STORAGE		"emmc"
#elif defined(BOOT_KERNEL_FROM_TFTP)
#define SP_MAIN_STORAGE		"tftp"
#else
#define SP_MAIN_STORAGE		"none"
#endif

/* u-boot env parameter */
#define CONFIG_SYS_MMC_ENV_DEV	0
#if defined(CONFIG_SYS_ENV_8388)
#define CONFIG_ENV_OFFSET	0x087E4400	/* LBA 0x00043f22 */
#define CONFIG_ENV_SIZE		0x2000
#else /* CONFIG_SYS_ENV_SP7021_EVB (and CONFIG_SYS_ENV_ZEBU) */
	#if defined(CONFIG_ENV_IS_IN_NAND)
#define CONFIG_ENV_OFFSET		(0x400000)
#define CONFIG_ENV_OFFSET_REDUND	(0x480000)
#define CONFIG_ENV_SIZE			(0x80000)
	#else
#define CONFIG_ENV_OFFSET	(0x1022 << 9)
#define CONFIG_ENV_SIZE		(0x0400 << 9)
	#endif
#endif

//#define CONFIG_CMDLINE_EDITING
//#define CONFIG_AUTO_COMPLETE
//#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_MAXARGS		32
#define CONFIG_SYS_CBSIZE		(2 << 10)
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_HZ			1000
#define CONFIG_STANDALONE_LOAD_ADDR	0xA1200000


#define CONFIG_DEBUG_UART_SUNPLUS
#define CONFIG_DEBUG_UART_BASE	0x9c000900

#undef DBG_SCR
#ifdef DBG_SCR
#define dbg_scr(s) s
#else
#define dbg_scr(s) ""


//#define CONFIG_MTD_PARTITIONS
//#define CONFIG_MTD_DEVICE	/* needed for mtdparts cmd */
#endif

/* TFTP server IP and board MAC address settings for TFTP ISP.
 * You should modify BOARD_MAC_ADDR to the address which you are assigned to */
#if !defined(BOOT_KERNEL_FROM_TFTP)
#define TFTP_SERVER_IP		172.18.12.62
#define BOARD_MAC_ADDR		00:22:60:00:88:20
#define USER_NAME		_username
#endif

#ifdef CONFIG_BOOTARGS_WITH_MEM
#define DEFAULT_BOOTARGS	"console=ttyS0,115200 root=/dev/ram rw loglevel=8 user_debug=255 earlyprintk"
#endif


#define AUTO_SCAN               0x01
#define EMMC_BOOT               0x05
#define SPI_NOR_BOOT            0x07  	
#define SDCARD_ISP              0x11
#define UART_ISP                0x13
#define USB_ISP                 0x15
#define SPINAND_BOOT            0xfe  // not use ,for code compile
#define NAND_LARGE_BOOT         0xfd  // not use ,for code compile


/* where to get boot info */
#define SP_BOOTINFO_BASE    0xFE806E08

/*
 * In the beginning, bootcmd will check bootmode in SRAM and the flag
 * if_zebu to choose different boot flow :
 * romter_boot
 *      kernel is in SPI nor offset 6M, and dtb is in offset 128K.
 *      kernel will be loaded to 0x307FC0 and dtb will be loaded to 0x2FFFC0.
 *      Then bootm 0x307FC0 - 0x2FFFC0.
 * qk_romter_boot
 *      kernel is in SPI nor offset 6M, and dtb is in offset 128K(0x20000).
 *      kernel will be loaded to 0x307FC0 and dtb will be loaded to 0x2FFFC0.
 *      Then sp_go 0x307FC0 0x2FFFC0.
 * emmc_boot
 *      kernel is stored in emmc LBA CONFIG_SRCADDR_KERNEL and dtb is
 *      stored in emmc LBA CONFIG_SRCADDR_DTB.
 *      kernel will be loaded to 0x307FC0 and dtb will be loaded to 0x2FFFC0.
 *      Then bootm 0x307FC0 - 0x2FFFC0.
 * qk_emmc_boot
 *      kernel is stored in emmc LBA CONFIG_SRCADDR_KERNEL and dtb is
 *      stored in emmc LBA CONFIG_SRCADDR_DTB.
 *      kernel will be loaded to 0x307FC0 and dtb will be loaded to 0x2FFFC0.
 *      Then sp_go 0x307FC0 - 0x2FFFC0.
 * zmem_boot / qk_zmem_boot
 *      kernel is preloaded to 0x307FC0 and dtb is preloaded to 0x2FFFC0.
 *      Then sp_go 0x307FC0 0x2FFFC0.
 * zebu_emmc_boot
 *      kernel is stored in emmc LBA CONFIG_SRCADDR_KERNEL and dtb is
 *      stored in emmc LBA CONFIG_SRCADDR_DTB.
 *      kernel will be loaded to 0x307FC0 and dtb will be loaded to 0x2FFFC0.
 *      Then sp_go 0x307FC0 0x2FFFC0.
 *
 * About "sp_go"
 * Earlier, sp_go do not handle header so you should pass addr w/o header.
 * But now sp_go DO verify magic/hcrc/dcrc in the quick sunplus uIamge header.
 * So the address passed for sp_go must have header in it.
 */
#define CONFIG_BOOTCOMMAND \
"echo [scr] bootcmd started; " \
"md.l ${bootinfo_base} 1; " \
"if itest.l *${bootinfo_base} == " __stringify(SPI_NOR_BOOT) "; then " \
	"if itest ${if_zebu} == 1; then " \
		"if itest ${if_qkboot} == 1; then " \
			"echo [scr] qk zmem boot; " \
			"run qk_zmem_boot; " \
		"else " \
			"echo [scr] zmem boot; " \
			"run zmem_boot; " \
		"fi; " \
	"else " \
		"if itest ${if_qkboot} == 1; then " \
			"echo [scr] qk romter boot; " \
			"run qk_romter_boot; " \
		"elif itest.s ${sp_main_storage} == tftp; then " \
			"echo [scr] tftp_boot; " \
			"run tftp_boot; " \
		"else " \
			"echo [scr] romter boot; " \
			"run romter_boot; " \
		"fi; " \
	"fi; " \
"elif itest.l *${bootinfo_base} == " __stringify(EMMC_BOOT) "; then " \
	"if itest ${if_zebu} == 1; then " \
		"echo [scr] zebu emmc boot; " \
		"run zebu_emmc_boot; " \
	"else " \
		"if itest ${if_qkboot} == 1; then " \
			"echo [scr] qk emmc boot; " \
			"run qk_emmc_boot; " \
		"else " \
			"echo [scr] emmc boot; " \
			"run emmc_boot; " \
		"fi; " \
	"fi; " \
"elif itest.l *${bootinfo_base} == " __stringify(SPINAND_BOOT) "; then " \
	"echo [scr] nand boot; " \
	"run nand_boot; " \
"elif itest.l *${bootinfo_base} == " __stringify(USB_ISP) "; then " \
	"echo [scr] ISP from USB storage; " \
	"run isp_usb; " \
"elif itest.l *${bootinfo_base} == " __stringify(SDCARD_ISP) "; then " \
	"echo [scr] ISP from SD Card; " \
	"run isp_sdcard; " \
"else " \
	"echo Stop; " \
"fi"

#define DSTADDR_FREERTOS	0xA0000000 
#define TMPADDR_KERNEL		0xA3000000 
#define DSTADDR_KERNEL		0xA01FFFC0 
#define DSTADDR_DTB			0xA01F0000
#define TMPADDR_HEADER		0xA4000000

#if defined(CONFIG_SP_SPINAND) && defined(CONFIG_MMC_SP_EMMC)
#define SDCARD_DEVICE_ID	0
#else
#define SDCARD_DEVICE_ID	1
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
"bootinfo_base="		__stringify(SP_BOOTINFO_BASE) "\0" \
"addr_src_kernel="		__stringify(CONFIG_SRCADDR_KERNEL) "\0" \
"addr_src_dtb="			__stringify(CONFIG_SRCADDR_DTB) "\0" \
"addr_src_freertos=" 	__stringify(CONFIG_SRCADDR_FREERTOS) "\0" \
"addr_dst_kernel="		__stringify(DSTADDR_KERNEL) "\0" \
"addr_temp_kernel="		__stringify(TMPADDR_KERNEL) "\0" \
"addr_dst_dtb="			__stringify(DSTADDR_DTB) "\0" \
"addr_dst_freertos=" 	__stringify(DSTADDR_FREERTOS) "\0" \
"addr_tmp_header="		__stringify(TMPADDR_HEADER) "\0" \
"if_zebu="				__stringify(CONFIG_SYS_ZEBU_ENV) "\0" \
"if_qkboot="			__stringify(CONFIG_SYS_USE_QKBOOT_HEADER) "\0" \
"sp_main_storage="		SP_MAIN_STORAGE "\0" \
"serverip=" 			__stringify(TFTP_SERVER_IP) "\0" \
"macaddr="				__stringify(BOARD_MAC_ADDR) "\0" \
"sdcard_devid="			__stringify(SDCARD_DEVICE_ID) "\0" \
"fdt_high=0xffffffffffffffff\0" \
"be2le=setexpr byte *${tmpaddr} '&' 0x000000ff; " \
	"setexpr tmpval $tmpval + $byte; " \
	"setexpr tmpval $tmpval * 0x100; " \
	"setexpr byte *${tmpaddr} '&' 0x0000ff00; " \
	"setexpr byte ${byte} / 0x100; " \
	"setexpr tmpval $tmpval + $byte; " \
	"setexpr tmpval $tmpval * 0x100; " \
	"setexpr byte *${tmpaddr} '&' 0x00ff0000; " \
	"setexpr byte ${byte} / 0x10000; " \
	"setexpr tmpval $tmpval + $byte; " \
	"setexpr tmpval $tmpval * 0x100; " \
	"setexpr byte *${tmpaddr} '&' 0xff000000; " \
	"setexpr byte ${byte} / 0x1000000; " \
	"setexpr tmpval $tmpval + $byte;\0" \
"boot_Image_gz=setexpr addr_dst_kernel ${addr_dst_kernel} + 0x40; " \
	"setexpr addr_temp_kernel ${addr_temp_kernel} + 0x40; " \
	"unzip ${addr_temp_kernel} ${addr_dst_kernel};" \
	dbg_scr("echo booti ${addr_dst_kernel} - ${fdtcontroladdr}; ") \
	"booti ${addr_dst_kernel} - ${fdtcontroladdr}\0" \
"romter_boot=cp.b ${addr_src_kernel} ${addr_tmp_header} 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 4; setexpr sz_kernel ${sz_kernel} / 4; " \
	dbg_scr("echo kernel from ${addr_src_kernel} to ${addr_temp_kernel} sz ${sz_kernel}; ") \
	"cp.l ${addr_src_kernel} ${addr_temp_kernel} ${sz_kernel}; " \
	"cp.b ${addr_src_freertos} ${addr_tmp_header} 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_freertos ${tmpval} + 0x40; " \
	"setexpr sz_freertos ${sz_freertos} + 4; setexpr sz_freertos ${sz_freertos} / 4; " \
	dbg_scr("echo freertos from ${addr_src_freertos} to ${addr_dst_freertos} sz ${sz_freertos}; ") \
	"cp.l ${addr_src_freertos} ${addr_dst_freertos} ${sz_freertos}; " \
	"run boot_Image_gz; \0" \
"emmc_boot=mmc read ${addr_tmp_header} ${addr_src_freertos} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x4; run be2le; " \
	"setexpr sz_freertos ${tmpval} + 0x28; " \
	"setexpr sz_freertos ${sz_freertos} + 0x200; setexpr sz_freertos ${sz_freertos} / 0x200; " \
	"mmc read ${addr_dst_freertos} ${addr_src_freertos} ${sz_freertos}; " \
	"mmc read ${addr_tmp_header} ${addr_src_kernel} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 0x200; setexpr sz_kernel ${sz_kernel} / 0x200; " \
	"mmc read ${addr_temp_kernel} ${addr_src_kernel} ${sz_kernel}; " \
	"setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk0p7 rw user_debug=255 rootwait ;" \
	"run boot_Image_gz; \0" \
"qk_zmem_boot=sp_go ${addr_dst_kernel} ${fdtcontroladdr}\0" \
"zmem_boot=bootm ${addr_dst_kernel} - ${fdtcontroladdr}\0" \
"zebu_emmc_boot=mmc rescan; mmc part; " \
	"mmc read ${addr_tmp_header} ${addr_src_dtb} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_dtb ${tmpval} + 0x40; " \
	"setexpr sz_dtb ${sz_dtb} + 0x200; setexpr sz_dtb ${sz_dtb} / 0x200; " \
    "setexpr addr_dram ${fdtcontroladdr} - 0x40; " \
	"mmc read ${addr_dram} ${addr_src_dtb} ${sz_dtb}; " \
	"mmc read ${addr_tmp_header} ${addr_src_kernel} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 0x200; setexpr sz_kernel ${sz_kernel} / 0x200; " \
	"mmc read ${addr_temp_kernel} ${addr_src_kernel} ${sz_kernel}; " \
	"run boot_Image_gz; \0" \
"tftp_boot=setenv ethaddr ${macaddr} && printenv ethaddr; " \
	"printenv serverip; " \
	"dhcp ${addr_dst_dtb} ${serverip}:dtb" __stringify(USER_NAME) "; " \
	"dhcp ${addr_dst_kernel} ${serverip}:Image" __stringify(USER_NAME) "; " \
	"booti ${addr_dst_kernel} - ${addr_dst_dtb}; " \
	"\0" \
"isp_usb=setenv isp_if usb && setenv isp_dev 0; " \
	"$isp_if start; " \
	"run isp_common; " \
	"\0" \
"isp_sdcard=setenv isp_if mmc && setenv isp_dev $sdcard_devid; " \
	"setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk1p2 rw user_debug=255 rootwait;"\
	"mmc list; " \
	"run isp_common; " \
	"\0" \
"isp_common=setenv isp_ram_addr 0x1000000; " \
	"fatls $isp_if $isp_dev / ; " \
	"fatload $isp_if $isp_dev $isp_ram_addr /ISPBOOOT.BIN 0x800 0x100000 && md.b $isp_ram_addr 0x200; " \
	"setenv isp_main_storage ${sp_main_storage} && printenv isp_main_storage; " \
	"setexpr script_addr $isp_ram_addr + 0x20 && setenv script_addr 0x${script_addr} && source $script_addr; " \
	"\0" \
"update_usb=setenv isp_if usb && setenv isp_dev 0; " \
	"$isp_if start; " \
	"run update_common; " \
	"\0" \
"update_sdcard=setenv isp_if mmc && setenv isp_dev 1; " \
	"mmc list; " \
	"run update_common; " \
	"\0" \
"update_common=setenv isp_ram_addr 0x1000000; " \
	"setenv isp_update_file_name ISP_UPDT.BIN; " \
	"fatload $isp_if $isp_dev $isp_ram_addr /$isp_update_file_name 0x800 && md.b $isp_ram_addr 0x200; " \
	"setenv isp_main_storage ${sp_main_storage} && printenv isp_main_storage; " \
	"setenv isp_image_header_offset 0; " \
	"setexpr script_addr $isp_ram_addr + 0x20 && setenv script_addr 0x${script_addr} && source $script_addr; " \
	"\0" \
"update_tftp=setenv isp_ram_addr 0x1000000; " \
	"setenv ethaddr ${macaddr} && printenv ethaddr; " \
	"printenv serverip; " \
	"dhcp $isp_ram_addr $serverip:TFTP0000.BIN; " \
	"setenv isp_main_storage ${sp_main_storage} && printenv isp_main_storage; " \
	"setexpr script_addr $isp_ram_addr + 0x00 && setenv script_addr 0x${script_addr}; " \
	"md.b $isp_ram_addr 0x200 && source $script_addr; " \
	"\0"

#if 0
/* romter test booting command */
#define CONFIG_BOOTCOMMAND      "echo bootcmd started ; sp_preboot dump ; sp_preboot ; printenv ; \
echo [cmd] cp.l 0x98600000 0x307FC0 0x280000 ; \
cp.l 0x98600000 0x307FC0 0x280000 ; \
echo [cmd] cp.l 0x98020000 0x2FFFC0 0x400 ; \
cp.l 0x98020000 0x2FFFC0 0x400 ; \
sp_go 0x308000 0x300000"

/* zebu emmc booting test command */
#define CONFIG_BOOTCOMMAND      "echo [scr] emmc bootcmd started ; \
mmc rescan ; mmc part ; \
mmc read 0x2fffc0 0x1422 0x1 ; md 0x2fffc0 0x60 ; \
mmc read 0x307fc0 0x1822 0x1 ; md 0x307fc0 0x60 ; \
mmc read 0x2fffc0 0x1422 0xa ; mmc read 0x307fc0 0x1822 0x30f0 ; sp_go 0x308000 0x300000"

/* zebu zmem booting test command */
#define CONFIG_BOOTCOMMAND      "echo [scr] zmem bootcmd started ; sp_go 0x308000 0x300040"
#endif

/* MMC related configs */
#define CONFIG_SUPPORT_EMMC_BOOT
/* #define CONFIG_MMC_TRACE */

#define CONFIG_ENV_OVERWRITE    /* Allow to overwrite ethaddr and serial */

#if !defined(CONFIG_SP_SPINAND) && !defined(CONFIG_MMC_SP_EMMC)
//#define SPEED_UP_SPI_NOR_CLK    /* Set CLK based on flash id */
#endif

#ifdef CONFIG_USB_OHCI_HCD
/* USB Config */
#define CONFIG_USB_OHCI_NEW			1
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	2
#endif

#endif /* __CONFIG_PENTAGRAM_H */
