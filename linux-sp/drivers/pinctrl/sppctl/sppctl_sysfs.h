/*
 * SP7021 pinmux controller driver.
 * Copyright (C) SunPlus Tech/Tibbo Tech. 2019
 * Author: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SPPCTL_SYSFS_H
#define SPPCTL_SYSFS_H

#include "sppctl.h"

typedef struct sppctl_sdata_T {
 uint8_t i;
 uint8_t ridx;
 sppctl_pdata_t *pdata;
} sppctl_sdata_t;

void sppctl_sysfs_init(  struct platform_device *_pdev);
void sppctl_sysfs_clean( struct platform_device *_pdev);

#endif // SPPCTL_SYSFS_H
