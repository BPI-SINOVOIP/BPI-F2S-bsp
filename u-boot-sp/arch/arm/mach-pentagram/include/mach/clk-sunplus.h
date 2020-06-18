// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus common clock driver
 *
 * Copyright (C) 2020 Sunplus Inc.
 * Author: qinjian <qinjian@sunmedia.com.cn>
 */

#ifndef __CLK_SUNPLUS_H__
#define __CLK_SUNPLUS_H__
#include <common.h>
#include <asm/io.h>
#include <clk-uclass.h>
#include <div64.h>
#include <dm.h>
#include <dm/device-internal.h>

//#define SP_CLK_TEST

//#define TRACE	printf("!!! %s:%d\n", __FUNCTION__, __LINE__)
#define TRACE

#ifndef clk_readl
#define clk_readl	readl
#define clk_writel	writel
#endif

#define PLL_MAX		(16)
#define EXTCLK		(-1)
#define P_EXTCLK	(1 << 16)

struct sunplus_clk {
	void *base;
};

ulong sp_gate_get_rate(int id);
ulong sp_pll_get_rate(int id);
ulong sp_pll_set_rate(int id, ulong rate);
int sp_clk_is_enabled(struct clk *clk);
void sp_clk_endisable(struct clk *clk, int enable);
void sp_clk_dump(struct clk *clk);
void sp_plls_dump(void);

int sunplus_clk_get_by_index(int index, struct clk *clk);
int sunplus_clk_request(struct udevice *dev, struct clk *clk);

#endif /*__CLK_SUNPLUS_H__*/
