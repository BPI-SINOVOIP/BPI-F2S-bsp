#ifndef __BOARD_SP_ENV_PREBOOT_H
#define __BOARD_SP_ENV_PREBOOT_H

/* NOTICE: these definition should be matched with u-boot init script */
/* name list of env */
#define SP_ENVNAME_BOOTACT      "sp_boot_action"
#define SP_ENVNAME_BOOTMED      "sp_boot_media"
#define SP_ENVNAME_BOOT_RAMADDR "sp_boot_scr_saddr"
#define SP_ENVNAME_BOOT_MEDADDR "sp_boot_scr_daddr"

#define SP_ENVNAME_BOOTDEV      "sp_boot_dev"
#define SP_ENVNAME_BOOTPART     "sp_boot_part"

/* value of env "sp_boot_action" */
#define SP_BOOTACT_FROM_NAND    "boot_from_nand"
#define SP_BOOTACT_FROM_EMMC    "boot_from_emmc"
#define SP_BOOTACT_FROM_NOR     "boot_from_nor"
#define SP_BOOTACT_ISP_USB      "isp_from_usb"
#define SP_BOOTACT_ISP_SDCARD   "isp_from_sdcard"
#define SP_BOOTACT_OTHER        "other"

/* value of env "sp_boot_media" */
#define SP_BOOTMED_NAND         "nand"
#define SP_BOOTMED_EMMC         "emmc"
#define SP_BOOTMED_NOR          "nor"
#define SP_BOOTMED_USB          "usb"
#define SP_BOOTMED_SDCARD       "sdcard"
#define SP_BOOTMED_OTHER        "other"
#endif /* __BOARD_SP_ENV_PREBOOT_H */
