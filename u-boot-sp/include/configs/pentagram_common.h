/*
 * SPDX-License-Identifier: GPL-2.0+
 */

/*
 * Note:
 *	Do NOT use "//" for comment, it will cause issue in u-boot.lds
 */

#ifndef __CONFIG_PENTAGRAM_H
#define __CONFIG_PENTAGRAM_H

/* define board-specific options/flags here, e.g. memory size, ... */
#if   defined(CONFIG_TARGET_PENTAGRAM_COMMON)
/* ... */
#elif defined(CONFIG_TARGET_PENTAGRAM_B_BOOT)
/* ... */
#else
#error "No board configuration is defined"
#endif

/* Disable some options which is enabled by default: */
#undef CONFIG_CMD_IMLS

//#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_SDRAM_BASE		0
#if defined(CONFIG_SYS_ENV_ZEBU)
#define CONFIG_SYS_SDRAM_SIZE           (64 << 20)
#elif defined(CONFIG_SYS_ENV_8388)
#define CONFIG_SYS_SDRAM_SIZE		(256 << 20)
#else /* normal SP7021 evb environment can have larger DRAM size */
#define CONFIG_SYS_SDRAM_SIZE		(512 << 20)
#endif
#define CONFIG_SYS_MALLOC_LEN		(6 << 20)

#ifndef CONFIG_SYS_TEXT_BASE		/* where U-Boot is loaded by xBoot */
/* It is defined in arch/arm/mach-pentagram/Kconfig */
#error "CONFIG_SYS_TEXT_BASE not defined"
#else
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN		(512 << 10)
#endif /* CONFIG_SYS_TEXT_BASE */

#undef CONFIG_SYS_PROMPT
#define CONFIG_SYS_PROMPT       		"BPI-IoT> "
#ifdef CONFIG_SPL_BUILD
#ifndef CONFIG_SYS_UBOOT_START		/* default entry point */
#define CONFIG_SYS_UBOOT_START		CONFIG_SYS_TEXT_BASE
#endif
#endif

#define CONFIG_SYS_INIT_SP_ADDR		(1 << 20)	/* set it in DRAM area (not SRAM) because DRAM is ready before U-Boot executed */
#define CONFIG_SYS_LOAD_ADDR		(4 << 20)	/* kernel loaded address */

#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			115200		/* the value doesn't matter, it's not change in U-Boot */
#endif
#define CONFIG_SYS_BAUDRATE_TABLE	{ 57600, 115200 }
/* #define CONFIG_SUNPLUS_SERIAL */

/* Main storage selection */
#if defined(CONFIG_SP_SPINAND)
#define SP_MAIN_STORAGE			"nand"
#elif defined(CONFIG_MMC_SP_EMMC)
#define SP_MAIN_STORAGE			"emmc"
#elif defined(BOOT_KERNEL_FROM_TFTP)
#define SP_MAIN_STORAGE			"tftp"
#else
#define SP_MAIN_STORAGE			"none"
#endif

/* u-boot env parameter */
#define CONFIG_SYS_MMC_ENV_DEV		0
#if defined(CONFIG_SYS_ENV_8388)
#define CONFIG_ENV_OFFSET		0x087E4400	/* LBA 0x00043f22 */
#define CONFIG_ENV_SIZE			0x2000
#else /* CONFIG_SYS_ENV_SP7021_EVB (and CONFIG_SYS_ENV_ZEBU) */
	#if defined(CONFIG_ENV_IS_IN_NAND)
#define CONFIG_ENV_OFFSET		(0x400000)
#define CONFIG_ENV_OFFSET_REDUND	(0x480000)
#define CONFIG_ENV_SIZE			(0x80000)
	#else
#define CONFIG_ENV_OFFSET		(0x1022 << 9)
#define CONFIG_ENV_SIZE			(0x0400 << 9)
	#endif
#endif

//#define CONFIG_CMDLINE_EDITING
//#define CONFIG_AUTO_COMPLETE
//#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_MAXARGS		32
#define CONFIG_SYS_CBSIZE		(2 << 10)
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_ARCH_MISC_INIT
#define CONFIG_SYS_HZ			1000

#if defined(CONFIG_SYS_ENV_8388)
#include <asm/arch/sp_bootmode_bitmap_8388.h>
#else /* CONFIG_SYS_ENV_SP7021_EVB (and CONFIG_SYS_ENV_ZEBU) */
#include <asm/arch/sp_bootmode_bitmap_sc7xxx.h>
#endif

#undef DBG_SCR
#ifdef DBG_SCR
#define dbg_scr(s) s
#else
#define dbg_scr(s) ""
#endif

#ifdef CONFIG_SP_SPINAND
//#define CONFIG_MTD_PARTITIONS
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_SYS_NAND_BASE		0x9c002b80
//#define CONFIG_MTD_DEVICE		/* needed for mtdparts cmd */
#define MTDIDS_DEFAULT			"nand0=sp_spinand.0"
#define MTDPARTS_DEFAULT		"mtdparts=sp_spinand.0:-(whole_nand)"
#endif

/* TFTP server IP and board MAC address settings for TFTP ISP.
 * You should modify BOARD_MAC_ADDR to the address which you are assigned to */
#if !defined(BOOT_KERNEL_FROM_TFTP)
#define TFTP_SERVER_IP			172.18.12.62
#define BOARD_MAC_ADDR			00:22:60:00:88:20
#define USER_NAME			_username
#endif

#ifdef CONFIG_BOOTARGS_WITH_MEM
#define DEFAULT_BOOTARGS		"console=ttyS0,115200 root=/dev/ram rw loglevel=8 user_debug=255 earlyprintk"
#endif

#define RASPBIAN_CMD                    // Enable Raspbian command


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
#undef CONFIG_BOOTCOMMAND
/*
#define CONFIG_BOOTCOMMAND "run distro_bootcmd"
*/
#define CONFIG_BOOTCOMMAND "run boot_normal"
#undef CONFIG_BOOTCOMMAND2
#define CONFIG_BOOTCOMMAND2 \
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

#define DSTADDR_KERNEL		0x307FC0 /* if stext is on 0x308000 */
#define DSTADDR_DTB		0x2FFFC0
#define TMPADDR_HEADER		0x800000
#define DSTADDR_ROOTFS		0x13FFFC0

#if defined(CONFIG_SP_SPINAND) && defined(CONFIG_MMC_SP_EMMC)
#define SDCARD_DEVICE_ID	0
#else
#define SDCARD_DEVICE_ID	1
#endif

#ifdef RASPBIAN_CMD
#define RASPBIAN_INIT		"setenv filesize 0; " \
				"fatsize $isp_if $isp_dev /cmdline.txt; " \
				"if test $filesize != 0; then " \
				"	fatload $isp_if $isp_dev $addr_dst_dtb /cmdline.txt; " \
				"	raspb init $fileaddr $filesize; " \
				"	echo new bootargs; " \
				"	echo $bootargs; " \
				"fi; "
#else
#define RASPBIAN_INIT		""
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
   "bpiver=1\0" \
   "bpi=bananapi\0" \
   "board=bpi-f2s\0" \
   "chip=SP7201\0" \
   "service=linux\0" \
   "scriptaddr=0x1000000\0" \
   "device=mmc\0" \
   "partition=1:1\0" \
   "kernel=uImage\0" \
   "root=/dev/mmcblk0p2\0" \
   "debug=7\0" \
   "bootenv=uEnv.txt\0" \
   "checksd=fatinfo ${device} 1:1\0" \
   "loadbootenv=fatload ${device} ${partition} ${scriptaddr} ${bpi}/${board}/${service}/${bootenv} || fatload ${device} ${partition} ${scriptaddr} ${bootenv}\0" \
   "boot_normal=if run checksd; then echo Boot from ${device} ; setenv partition 1:1; else echo Boot from eMMC ; setenv partition 0:1 ; fi; if run loadbootenv; then echo Loaded environment from ${bootenv}; env import -t ${scriptaddr} ${filesize}; fi; run uenvcmd; run distro_bootcmd; fatload mmc 1:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootr\0" \
   "boot_user=echo Boot from USB ; usb start ; setenv device usb ; setenv partition 0:1 ; fi; if run loadbootenv; then echo Loaded environment from ${bootenv}; env import -t ${scriptaddr} ${filesize}; fi; run usercmd; fatload mmc 1:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootr\0" \
"bootinfo_base="		__stringify(SP_BOOTINFO_BASE) "\0" \
"addr_src_kernel="		__stringify(CONFIG_SRCADDR_KERNEL) "\0" \
"addr_src_dtb="			__stringify(CONFIG_SRCADDR_DTB) "\0" \
"addr_dst_kernel="		__stringify(DSTADDR_KERNEL) "\0" \
"addr_dst_dtb="			__stringify(DSTADDR_DTB) "\0" \
"addr_dst_rootfs="		__stringify(DSTADDR_ROOTFS) "\0" \
"addr_tmp_header="		__stringify(TMPADDR_HEADER) "\0" \
"if_zebu="			__stringify(CONFIG_SYS_ENV_ZEBU) "\0" \
"if_qkboot="			__stringify(CONFIG_SYS_USE_QKBOOT_HEADER) "\0" \
"sp_main_storage="		SP_MAIN_STORAGE "\0" \
"serverip=" 			__stringify(TFTP_SERVER_IP) "\0" \
"macaddr="			__stringify(BOARD_MAC_ADDR) "\0" \
"sdcard_devid="			__stringify(SDCARD_DEVICE_ID) "\0" \
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
"romter_boot=cp.b ${addr_src_dtb} ${addr_tmp_header} 0x28; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x4; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_dtb ${tmpval} + 0x28; " \
	"setexpr sz_dtb ${sz_dtb} + 4; setexpr sz_dtb ${sz_dtb} / 4; " \
	dbg_scr("echo dtb from ${addr_src_dtb} to ${addr_dst_dtb} sz ${sz_dtb}; ") \
	"cp.l ${addr_src_dtb} ${addr_dst_dtb} ${sz_dtb}; " \
	"cp.b ${addr_src_kernel} ${addr_tmp_header} 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 4; setexpr sz_kernel ${sz_kernel} / 4; " \
	dbg_scr("echo kernel from ${addr_src_kernel} to ${addr_dst_kernel} sz ${sz_kernel}; ") \
	"cp.l ${addr_src_kernel} ${addr_dst_kernel} ${sz_kernel}; " \
	dbg_scr("echo bootm ${addr_dst_kernel} - ${addr_dst_dtb}; ") \
	"bootm ${addr_dst_kernel} - ${addr_dst_dtb}\0" \
"qk_romter_boot=cp.b ${addr_src_dtb} ${addr_tmp_header} 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_dtb ${tmpval} + 0x40; " \
	"setexpr sz_dtb ${sz_dtb} + 4; setexpr sz_dtb ${sz_dtb} / 4; " \
	dbg_scr("echo dtb from ${addr_src_dtb} to ${addr_dst_dtb} sz ${sz_dtb}; ") \
	"cp.l ${addr_src_dtb} ${addr_dst_dtb} ${sz_dtb}; " \
	"cp.b ${addr_src_kernel} ${addr_tmp_header} 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel  ${sz_kernel} + 4; setexpr sz_kernel ${sz_kernel} / 4; " \
	dbg_scr("echo kernel from ${addr_src_kernel} to ${addr_dst_kernel} sz ${sz_kernel}; ") \
	"cp.l ${addr_src_kernel} ${addr_dst_kernel} ${sz_kernel}; " \
	dbg_scr("echo sp_go ${addr_dst_kernel} ${addr_dst_dtb}; ") \
	"sp_go ${addr_dst_kernel} ${addr_dst_dtb}\0" \
"emmc_boot=setenv sdmmc_on 0; run boot_normal; mmc read ${addr_tmp_header} ${addr_src_dtb} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x4; run be2le; " \
	"setexpr sz_dtb ${tmpval} + 0x28; " \
	"setexpr sz_dtb ${sz_dtb} + 0x200; setexpr sz_dtb ${sz_dtb} / 0x200; " \
	"mmc read ${addr_dst_dtb} ${addr_src_dtb} ${sz_dtb}; " \
	"mmc read ${addr_tmp_header} ${addr_src_kernel} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 0x200; setexpr sz_kernel ${sz_kernel} / 0x200; " \
	"mmc read ${addr_dst_kernel} ${addr_src_kernel} ${sz_kernel}; " \
	"setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk0p7 rw user_debug=255 rootwait ;" \
	"bootm ${addr_dst_kernel} - ${addr_dst_dtb}\0" \
"qk_emmc_boot=mmc read ${addr_tmp_header} ${addr_src_dtb} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_dtb ${tmpval} + 0x40; " \
	"setexpr sz_dtb ${sz_dtb} + 0x200; setexpr sz_dtb ${sz_dtb} / 0x200; " \
	"mmc read ${addr_dst_dtb} ${addr_src_dtb} ${sz_dtb}; " \
	"mmc read ${addr_tmp_header} ${addr_src_kernel} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 0x200; setexpr sz_kernel ${sz_kernel} / 0x200; " \
	"mmc read ${addr_dst_kernel} ${addr_src_kernel} ${sz_kernel}; " \
	"sp_go ${addr_dst_kernel} ${addr_dst_dtb}\0" \
"nand_boot=nand read ${addr_tmp_header} dtb 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x4; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_dtb ${tmpval} + 0x40; " \
	dbg_scr("echo from dtb partition to ${addr_dst_dtb} sz ${sz_dtb}; ") \
	"nand read ${addr_dst_dtb} dtb ${sz_dtb}; " \
	"nand read ${addr_tmp_header} kernel 0x40; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	dbg_scr("md ${addr_tmp_header} 0x10; printenv tmpval; ") \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	dbg_scr("echo from kernel partition to ${addr_dst_kernel} sz ${sz_kernel}; ") \
	"nand read ${addr_dst_kernel} kernel ${sz_kernel}; " \
	"setenv bootargs console=ttyS0,115200 earlyprintk root=ubi0:rootfs rw ubi.mtd=8,2048 rootflags=sync rootfstype=ubifs mtdparts=sp_spinand:128k(nand_header),128k(xboot1),768k(uboot1),3m(uboot2),512k(env),512k(env_redund),256k(dtb),15m(kernel),-(rootfs) user_debug=255 rootwait ;" \
	"bootm ${addr_dst_kernel} - ${addr_dst_dtb}\0" \
"qk_zmem_boot=sp_go ${addr_dst_kernel} ${addr_dst_dtb}\0" \
"zmem_boot=bootm ${addr_dst_kernel} - ${addr_dst_dtb}\0" \
"zebu_emmc_boot=mmc rescan; mmc part; " \
	"mmc read ${addr_tmp_header} ${addr_src_dtb} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_dtb ${tmpval} + 0x40; " \
	"setexpr sz_dtb ${sz_dtb} + 0x200; setexpr sz_dtb ${sz_dtb} / 0x200; " \
	"mmc read ${addr_dst_dtb} ${addr_src_dtb} ${sz_dtb}; " \
	"mmc read ${addr_tmp_header} ${addr_src_kernel} 0x1; " \
	"setenv tmpval 0; setexpr tmpaddr ${addr_tmp_header} + 0x0c; run be2le; " \
	"setexpr sz_kernel ${tmpval} + 0x40; " \
	"setexpr sz_kernel ${sz_kernel} + 72; " \
	"setexpr sz_kernel ${sz_kernel} + 0x200; setexpr sz_kernel ${sz_kernel} / 0x200; " \
	"mmc read ${addr_dst_kernel} ${addr_src_kernel} ${sz_kernel}; " \
	"sp_go ${addr_dst_kernel} ${addr_dst_dtb}\0" \
"tftp_boot=setenv ethaddr ${macaddr} && printenv ethaddr; " \
	"printenv serverip; " \
	"dhcp ${addr_dst_dtb} ${serverip}:dtb" __stringify(USER_NAME) "; " \
	"dhcp ${addr_dst_kernel} ${serverip}:uImage" __stringify(USER_NAME) "; " \
	"bootm ${addr_dst_kernel} - ${addr_dst_dtb}; " \
	"\0" \
"isp_usb=setenv isp_if usb && setenv isp_dev 0; " \
	"$isp_if start; " \
	"run isp_common; " \
	"\0" \
"isp_sdcard=setenv isp_if mmc && setenv isp_dev $sdcard_devid; " \
	"setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk1p2 rw user_debug=255 rootwait;"\
	"mmc list; " \
	"setenv sdmmc_on 1; " \
	"run boot_normal; run isp_common; " \
	"\0" \
"isp_common=setenv isp_ram_addr 0x1000000; " \
	RASPBIAN_INIT \
	"fatload $isp_if $isp_dev $isp_ram_addr /ISPBOOOT.BIN 0x800 0x100000; " \
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
	"fatload $isp_if $isp_dev $isp_ram_addr /$isp_update_file_name 0x800; " \
	"setenv isp_main_storage ${sp_main_storage} && printenv isp_main_storage; " \
	"setenv isp_image_header_offset 0; " \
	"setexpr script_addr $isp_ram_addr + 0x20 && setenv script_addr 0x${script_addr} && source $script_addr; " \
	"\0" \
"update_tftp=setenv isp_ram_addr 0x1000000; " \
	"setenv ethaddr ${macaddr} && printenv ethaddr; " \
	"printenv serverip; " \
	"dhcp $isp_ram_addr $serverip:TFTP0000.BIN; " \
	"setenv isp_main_storage ${sp_main_storage} && printenv isp_main_storage; " \
	"setexpr script_addr $isp_ram_addr + 0x00 && setenv script_addr 0x${script_addr} && source $script_addr; " \
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
#define SPEED_UP_SPI_NOR_CLK    /* Set CLK based on flash id */
#endif


#define SCRIPT_BOOT \
	"fileload=${mmctype}load mmc ${devnum}:${mmcpart} " \
		"${loadaddr} ${mmcfile}\0" \
	"fileload2=${mmctype}load mmc ${devnum}:2 " \
		"${loadaddr} ${mmcfile}\0" \
	"kernload=setenv loadaddr ${kernel_addr_r};" \
		"setenv mmcfile ${mmckernfile};" \
		"run fileload\0" \
	"initrdload=setenv loadaddr ${rdaddr};" \
		"setenv mmcfile ${mmcinitrdfile};" \
		"run fileload\0" \
	"fdtload=setenv loadaddr ${fdtaddr};" \
		"setenv mmcfile ${mmcfdtfile};" \
		"run fileload\0" \
	"fdtload2=setenv loadaddr ${fdtaddr};" \
		"setenv mmcfile ${mmcfdtfile};" \
		"run fileload2\0" \
	"scriptload=setenv loadaddr ${scriptaddr};" \
		"setenv mmcfile ${mmcscriptfile};" \
		"run fileload\0" \
	"scriptboot=echo Running ${mmcscriptfile} from: mmc ${devnum}:${mmcpart} using ${mmcscriptfile};" \
		"source ${scriptaddr};" \


/* Extra environment variables */
#define SUNPLUS_ENV_SETTINGS	\
   "bpiver=1\0" \
   "bpi=bananapi\0" \
   "board=bpi-f2s\0" \
   "chip=SP7201\0" \
   "service=linux\0" \
   "scriptaddr=0x1000000\0" \
   "device=mmc\0" \
   "partition=1:1\0" \
   "kernel=uImage\0" \
   "root=/dev/mmcblk0p2\0" \
   "debug=7\0" \
   "bootenv=uEnv.txt\0" \
	"loadaddr=0x82000000\0" \
	"mmckernfile=boot/zImage\0" \
	"mmcinitrdfile= boot/uInitrd\0" \
	"mmcfdtfile=boot/bpi-f2s.dtb\0" \
	"mmcscriptfile=boot/boot.scr\0" \
	"mmctype=ext4\0" \
	"devnum=1\0" \
	"mmcpart=1\0" \
	"mmc_bootdev=1\0" \
   "checksd=fatinfo ${device} 1:1\0" \
   "loadbootenv=fatload ${device} ${partition} ${scriptaddr} ${bpi}/${board}/${service}/${bootenv} || fatload ${device} ${partition} ${scriptaddr} ${bootenv}\0" \
   "boot_normal=if run checksd; then echo Boot from ${device}/SD; setenv root /dev/mmcblk1p2 ; setenv sdmmc_on 1 ; setenv partition 1:1; else echo Boot from eMMC ; setenv root /dev/mmcblk0p2 ; setenv sdmmc_on 0; setenv partition 0:1 ; fi; if run loadbootenv; then echo Loaded environment from ${bootenv}; env import -t ${scriptaddr} ${filesize}; fi; run uenvcmd; fatload mmc 1:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootr\0" \
   "boot_user=echo Boot from USB ; usb start ; setenv device usb ; setenv partition 0:1 ; fi; if run loadbootenv; then echo Loaded environment from ${bootenv}; env import -t ${scriptaddr} ${filesize}; fi; run usercmd; fatload mmc 1:1 ${loadaddr} ${bpi}/${board}/${service}/${kernel}; bootr\0" \
	SCRIPT_BOOT


/* Ethernet */
#define CONFIG_IPADDR			192.168.1.1
#define CONFIG_SERVERIP			192.168.1.2

#define CONFIG_SYS_MMC_ENV_DEV		0
//#define CONFIG_ENV_OFFSET		0x100000


#ifndef CONFIG_SPL_BUILD

/*
 * 160M RAM (256M minimum minus 64MB heap + 32MB for u-boot, stack, fb, etc.
 * 32M uncompressed kernel, 16M compressed kernel, 1M fdt,
 * 1M script, 1M pxe and the ramdisk at the end.
	"bootm_size=0x10000000\0"
 */
#define BOOTM_SIZE     __stringify(0xa000000)

#define KERNEL_ADDR_R  __stringify(DSTADDR_KERNEL)
#define FDT_ADDR_R     __stringify(DSTADDR_DTB)
#define SCRIPT_ADDR_R  __stringify(0x1000000)
#define PXEFILE_ADDR_R __stringify(DSTADDR_ROOTFS)
#define RAMDISK_ADDR_R __stringify(DSTADDR_ROOTFS)

#define MEM_LAYOUT_ENV_SETTINGS \
	"bootm_size=" BOOTM_SIZE "\0" \
	"kernel_addr_r=" KERNEL_ADDR_R "\0" \
	"fdt_addr_r=" FDT_ADDR_R "\0" \
	"fdtaddr=" FDT_ADDR_R "\0" \
	"scriptaddr=" SCRIPT_ADDR_R "\0" \
	"pxefile_addr_r=" PXEFILE_ADDR_R "\0" \
	"ramdisk_addr_r=" RAMDISK_ADDR_R "\0"

#define DFU_ALT_INFO_RAM \
	"dfu_alt_info_ram=" \
	"kernel ram " KERNEL_ADDR_R " 0x1000000;" \
	"fdt ram " FDT_ADDR_R " 0x100000;" \
	"ramdisk ram " RAMDISK_ADDR_R " 0x4000000\0"

#ifdef CONFIG_MMC
#if CONFIG_MMC_SUNXI_SLOT_EXTRA != -1
#define BOOTENV_DEV_MMC_AUTO(devtypeu, devtypel, instance)		\
	BOOTENV_DEV_MMC(MMC, mmc, 0)					\
	BOOTENV_DEV_MMC(MMC, mmc, 1)					\
	"bootcmd_mmc_bpi=setenv devnum ${mmc_bootdev}; run mmc_boot;\0"	\
	"bootcmd_mmc0=setenv devnum 0; run fdtload2; run fdtload; run mmc_boot;\0"			\
	"bootcmd_mmc1=setenv devnum 1; run fdtload2; run fdtload; run mmc_boot;\0"			\
	"bootcmd_mmc_auto="						\
		"if test ${mmc_bootdev} -eq 1; then "			\
			"run bootcmd_mmc1; "				\
			"run bootcmd_mmc0; "				\
		"elif test ${mmc_bootdev} -eq 0; then "			\
			"run bootcmd_mmc0; "				\
			"run bootcmd_mmc1; "				\
		"fi\0"

#define BOOTENV_DEV_NAME_MMC_AUTO(devtypeu, devtypel, instance) \
	"mmc_auto "

#define BOOT_TARGET_DEVICES_MMC(func) func(MMC_AUTO, mmc_auto, na)
#else
#define BOOT_TARGET_DEVICES_MMC(func) func(MMC, mmc, 0)
#endif
#else
#define BOOT_TARGET_DEVICES_MMC(func)
#endif

#ifdef CONFIG_AHCI
#define BOOT_TARGET_DEVICES_SCSI(func) func(SCSI, scsi, 0)
#else
#define BOOT_TARGET_DEVICES_SCSI(func)
#endif

#ifdef CONFIG_USB_STORAGE
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifdef CONFIG_CMD_PXE
#define BOOT_TARGET_DEVICES_PXE(func) func(PXE, pxe, na)
#else
#define BOOT_TARGET_DEVICES_PXE(func)
#endif

#ifdef CONFIG_CMD_DHCP
#define BOOT_TARGET_DEVICES_DHCP(func) func(DHCP, dhcp, na)
#else
#define BOOT_TARGET_DEVICES_DHCP(func)
#endif

/* FEL boot support, auto-execute boot.scr if a script address was provided */
#define BOOTENV_DEV_FEL(devtypeu, devtypel, instance) \
	"bootcmd_fel=" \
		"if test -n ${fel_booted} && test -n ${fel_scriptaddr}; then " \
			"echo '(FEL boot)'; " \
			"source ${fel_scriptaddr}; " \
		"fi\0"
#define BOOTENV_DEV_NAME_FEL(devtypeu, devtypel, instance) \
	"fel "

#define BOOT_TARGET_DEVICES(func) \
	func(FEL, fel, na) \
	BOOT_TARGET_DEVICES_MMC(func) \
	BOOT_TARGET_DEVICES_SCSI(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	BOOT_TARGET_DEVICES_PXE(func) \
	BOOT_TARGET_DEVICES_DHCP(func)

#ifdef CONFIG_OLD_SUNXI_KERNEL_COMPAT
/* BPI */
#define BOOTCMD_SUNXI_COMPAT \
	"bootcmd_sunxi_compat=" \
		"setenv root /dev/mmcblk0p2 rootwait; " \
		"if fatload mmc 0 0x44000000 ${bpi}/${board}/${service}/uEnv.txt; then " \
			"echo Loaded environment from uEnv.txt; " \
			"env import -t 0x44000000 ${filesize}; " \
		"fi; " \
		"setenv root /dev/mmcblk0p3 rootwait; " \
		"if ext2load mmc 0 0x44000000 uEnv.txt; then " \
			"echo Loaded environment from uEnv.txt; " \
			"env import -t 0x44000000 ${filesize}; " \
		"fi; " \
		"setenv bootargs console=${console} root=${root} ${extraargs}; " \
		"ext2load mmc 0 0x43000000 script.bin && " \
		"ext2load mmc 0 0x48000000 uImage && " \
		"bootm 0x48000000\0"
#else
#define BOOTCMD_SUNXI_COMPAT
#endif

#include <config_distro_bootcmd.h>

#ifdef CONFIG_USB_KEYBOARD
#define CONSOLE_STDIN_SETTINGS \
	"preboot=usb start\0" \
	"stdin=serial,usbkbd\0"
#else
#define CONSOLE_STDIN_SETTINGS \
	"stdin=serial\0"
#endif

#ifdef CONFIG_VIDEO
#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial,vga\0" \
	"stderr=serial,vga\0"
#elif CONFIG_DM_VIDEO
#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial,vidconsole\0" \
	"stderr=serial,vidconsole\0"
#else
#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial\0" \
	"stderr=serial\0"
#endif

#ifdef CONFIG_MTDIDS_DEFAULT
#define SUNXI_MTDIDS_DEFAULT \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0"
#else
#define SUNXI_MTDIDS_DEFAULT
#endif

#ifdef CONFIG_MTDPARTS_DEFAULT
#define SUNXI_MTDPARTS_DEFAULT \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"
#else
#define SUNXI_MTDPARTS_DEFAULT
#endif

#define PARTS_DEFAULT \
	"name=loader1,start=8k,size=32k,uuid=${uuid_gpt_loader1};" \
	"name=loader2,size=984k,uuid=${uuid_gpt_loader2};" \
	"name=esp,size=128M,bootable,uuid=${uuid_gpt_esp};" \
	"name=system,size=-,uuid=${uuid_gpt_system};"

#define UUID_GPT_ESP "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"

#ifdef CONFIG_ARM64
#define UUID_GPT_SYSTEM "b921b045-1df0-41c3-af44-4c6f280d3fae"
#else
#define UUID_GPT_SYSTEM "69dad710-2ce4-4e3c-b16c-21a1d49abed3"
#endif

#define CONSOLE_ENV_SETTINGS \
	CONSOLE_STDIN_SETTINGS \
	CONSOLE_STDOUT_SETTINGS

#ifdef CONFIG_ARM64
#define FDTFILE "allwinner/" CONFIG_DEFAULT_DEVICE_TREE ".dtb"
#else
#define FDTFILE CONFIG_DEFAULT_DEVICE_TREE ".dtb"
#endif

#undef CONFIG_EXTRA_ENV_SETTINGS

#define CONFIG_EXTRA_ENV_SETTINGS \
	SUNPLUS_ENV_SETTINGS \
	CONSOLE_ENV_SETTINGS \
	MEM_LAYOUT_ENV_SETTINGS \
	DFU_ALT_INFO_RAM \
	"fdtfile=" FDTFILE "\0" \
	"console=ttyS0,115200\0" \
	SUNXI_MTDIDS_DEFAULT \
	SUNXI_MTDPARTS_DEFAULT \
	"uuid_gpt_esp=" UUID_GPT_ESP "\0" \
	"uuid_gpt_system=" UUID_GPT_SYSTEM "\0" \
	"partitions=" PARTS_DEFAULT "\0" \
	BOOTCMD_SUNXI_COMPAT \
	BOOTENV

#else /* ifndef CONFIG_SPL_BUILD */
#define CONFIG_EXTRA_ENV_SETTINGS
#endif


#endif /* __CONFIG_PENTAGRAM_H */
