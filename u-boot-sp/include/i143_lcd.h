/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * i143_lcd.h - I143 LCD Controller structures
 *
 * (C) Copyright 2020
 * 		hammer.hsieh@sunplus.com
 */

#ifndef _I143_LCD_H_
#define _I143_LCD_H_

typedef struct vidinfo {
	u_int logo_width;
	u_int logo_height;
	int logo_x_offset;
	int logo_y_offset;
	u_long logo_addr;
} vidinfo_t;

void i143_logo_info(vidinfo_t *info);

#endif //_I143_LCD_H_
