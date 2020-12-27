// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Sunplus
 *		      Hammer Hsieh <hammer.hsieh@sunplus.com>
 */

#include <common.h>
#include <dm.h>
#include <asm/arch/i143_common.h>
#include <lcd.h>
#include <i143_lcd.h>
#include <version.h>
#include <video.h>
#include <video_console.h>
#include <asm/io.h>

#ifdef CONFIG_VIDEO_BMP_LOGO_MANUAL
#include <i143_logo_data.h>
#else
#ifdef CONFIG_DM_VIDEO_I143_LOGO
#include <dm_video_logo.h>
#include <dm_video_logo_data.h>
#endif
#endif

#include <mapmem.h>
#include <bmp_layout.h>
#include <command.h>
#include <asm/byteorder.h>
#include <malloc.h>
#include <mapmem.h>
#include <splash.h>

DECLARE_GLOBAL_DATA_PTR;

extern int i143_video_show_board_info(void);

int i143_video_show_board_info(void)
{
#ifdef CONFIG_DM_VIDEO
#if defined(CONFIG_CMD_BMP)
#ifdef CONFIG_DM_VIDEO_I143_LOGO	
	struct vidconsole_priv *priv;
#endif
#endif
#endif
	ulong dram_size;
	int i;
	u32 len = 0;
	char buf[255];
	char *corp = "2020 Sunplus Technology Inc.\n";
	struct udevice *dev, *con;
#ifdef CONFIG_DM_VIDEO
#if defined(CONFIG_CMD_BMP)
#ifdef CONFIG_DM_VIDEO_I143_LOGO
	const char *s;
#endif
#endif
#endif
	vidinfo_t logo_info;
	int ret;

	logo_info.logo_width = 0;
	logo_info.logo_height = 0;
	logo_info.logo_x_offset = 0;
	logo_info.logo_y_offset = 0;

	len += sprintf(&buf[len], "%s\n", U_BOOT_VERSION);
	memcpy(&buf[len], corp, strlen(corp));
	len += strlen(corp);

	dram_size = 0;
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		dram_size += gd->bd->bi_dram[i].size;

	
	len += sprintf(&buf[len], "I143 EVB BOARD V1.0 (V2.0) \n");

	len += sprintf(&buf[len], "%ld MB SDRAM \n",
		       dram_size >> 20);

	ret = uclass_get_device(UCLASS_VIDEO, 0, &dev);
	if (ret)
		return ret;

	i143_logo_info(&logo_info);

#ifdef CONFIG_DM_VIDEO
#if defined(CONFIG_CMD_BMP)
#ifdef CONFIG_DM_VIDEO_I143_LOGO
	gd->bmp_logo_addr = (long long)i143_uboot_logo;
	//debug("bmp addr = 0x%08x \n", (long long)i143_uboot_logo);
#ifdef CONFIG_DM_VIDEO_I143_LOGO_ALIGN
	bmp_display((long long)i143_uboot_logo,
		    BMP_ALIGN_CENTER,
			BMP_ALIGN_CENTER);
#else
	bmp_display((long long)i143_uboot_logo,
		    logo_info.logo_x_offset,
			logo_info.logo_y_offset);
#endif
#endif /* CONFIG_DM_VIDEO_I143_LOGO */
#endif /* CONFIG_CMD_BMP */
#endif /* CONFIG_DM_VIDEO */

	ret = uclass_get_device(UCLASS_VIDEO_CONSOLE, 0, &con);
	if (ret)
		return ret;

	
#ifdef CONFIG_DM_VIDEO
#if defined(CONFIG_CMD_BMP)
#ifdef CONFIG_DM_VIDEO_I143_LOGO
	priv = dev_get_uclass_priv(con);
	vidconsole_position_cursor(con, 0, ( logo_info.logo_height +
				   priv->y_charsize - 1) / priv->y_charsize);
	for (s = buf, i = 0; i < len; s++, i++)
		vidconsole_put_char(con, *s);
#endif /* CONFIG_DM_VIDEO_I143_LOGO */
#endif /* CONFIG_CMD_BMP */
#endif /* CONFIG_DM_VIDEO */

	return 0;
}

void i143_logo_info(vidinfo_t *info)
{
#ifdef CONFIG_DM_VIDEO_I143_LOGO
	info->logo_width = I143_LOGO_BMP_WIDTH;
	info->logo_height = I143_LOGO_BMP_HEIGHT;
	info->logo_x_offset = I143_LOGO_BMP_X_OFFSET;
	info->logo_y_offset = I143_LOGO_BMP_Y_OFFSET;
#endif
}
