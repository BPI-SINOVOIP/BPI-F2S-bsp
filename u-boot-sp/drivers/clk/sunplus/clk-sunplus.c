// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus common clock driver
 *
 * Copyright (C) 2020 Sunplus Inc.
 * Author: qinjian <qinjian@sunmedia.com.cn>
 */

#include <asm/arch/clk-sunplus.h>
#ifdef SP_CLK_TEST
#include <dt-bindings/clock/sp-q628.h>
#endif

ulong extclk_rate;
struct udevice *clkc_dev;

/************************************************* CLKC *************************************************/

static int sunplus_clk_enable(struct clk *clk)
{
	sp_clk_endisable(clk, 1);
	return 0;
}

static int sunplus_clk_disable(struct clk *clk)
{
	sp_clk_endisable(clk, 0);
	return 0;
}

static ulong sunplus_clk_get_rate(struct clk *clk)
{
	if (clk->id < PLL_MAX)
		return sp_pll_get_rate(clk->id);
	else
		return sp_gate_get_rate(clk->id);
}

static ulong sunplus_clk_set_rate(struct clk *clk, ulong rate)
{
	if (clk->id < PLL_MAX)
		return sp_pll_set_rate(clk->id, rate);
	else
		return -ENOENT;
}

int sunplus_clk_get_by_index(int index, struct clk *clk)
{
	clk->dev = clkc_dev;
	clk->id = index;
	return 0;
}

int sunplus_clk_request(struct udevice *dev, struct clk *clk)
{
	u32 n = (clk->id + 1) * 2;
	u32 *clkd = malloc(n * sizeof(u32));

	if (!clkd)
		return -ENOMEM;

	if (fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "clocks", clkd, n)) {
		pr_err("Faild to find clocks node. Check device tree\n");
		free(clkd);
		return -ENOENT;
	}
	n = clkd[n - 1];
	free(clkd);

	return sunplus_clk_get_by_index(n, clk);
}

static struct clk_ops sunplus_clk_ops = {
	.disable	= sunplus_clk_disable,
	.enable		= sunplus_clk_enable,
	.get_rate	= sunplus_clk_get_rate,
	.set_rate	= sunplus_clk_set_rate,
};

static int sunplus_clk_probe(struct udevice *dev)
{
	int ret;
	struct sunplus_clk *priv = dev_get_priv(dev);
	struct clk extclk = { .id = 0, };
	struct udevice *extclk_dev;

	ret = uclass_get_device_by_name(UCLASS_CLK, "clk@osc0", &extclk_dev);
	if (ret < 0) {
		pr_err("Failed to find extclk(clk@osc0) node. Check device tree\n");
		return ret;
	}
	clk_request(extclk_dev, &extclk);
	extclk_rate = clk_get_rate(&extclk);
	clk_free(&extclk);

	priv->base = (void *)devfdt_get_addr(dev);
	clkc_dev = dev;

	return 0;
}

static const struct udevice_id sunplus_clk_ids[] = {
	{ .compatible = "sunplus,sp-clkc" },
	{ }
};

U_BOOT_DRIVER(sunplus_clk) = {
	.name					= "sunplus_clk",
	.id						= UCLASS_CLK,
	.of_match				= sunplus_clk_ids,
	.priv_auto_alloc_size	= sizeof(struct sunplus_clk),
	.ops					= &sunplus_clk_ops,
	.probe					= sunplus_clk_probe,
};

int set_cpu_clk_info(void)
{
	struct udevice *dev;
	struct uclass *uc;
	struct clk clk;
	int ret;

	ret = uclass_get(UCLASS_CLK, &uc);
	if (ret) {
		pr_err("Failed to find clocks node. Check device tree\n");
		return ret;
	}

	uclass_foreach_dev(dev, uc) {
		if (!device_probe(dev)) {
			if (dev != clkc_dev) {
				clk.id = 0;
				if (!clk_request(dev, &clk)) {
					sp_clk_dump(&clk);
					clk_free(&clk);
				}
			} else
				sp_plls_dump();
		}
	}

#ifdef CONFIG_RESET_SP7021
	ret = uclass_get_device(UCLASS_RESET, 0, &dev);
	if (ret) {
		pr_err("Failed to find reset node. Check device tree\n");
		return ret;
	}
#endif

#ifdef SP_CLK_TEST
	printf("===== SP_CLK_TEST: disable/enable uartdmarx0 clock #1\n");
	ret = uclass_get_device_by_name(UCLASS_SERIAL, "serial@sp_uartdmarx0", &dev);
	if (!ret) {
		clk.id = 1;
		ret = sunplus_clk_request(dev, &clk);
		if (!ret) {
			sp_clk_dump(&clk);
			clk_disable(&clk); // disable
			sp_clk_dump(&clk);
			clk_enable(&clk); // enable
			sp_clk_dump(&clk);
			clk_free(&clk);
		}
	}

	printf("===== SP_CLK_TEST: set PLL_TV_A rate\n");
	ret = sunplus_clk_get_by_index(PLL_TV_A, &clk);
	if (!ret) {
		ulong rate = clk_get_rate(&clk);
		sp_clk_dump(&clk);
		clk_set_rate(&clk, 90000000UL); // change rate
		sp_clk_dump(&clk);
		clk_set_rate(&clk, rate); // restore
		sp_clk_dump(&clk);
		clk_free(&clk);
	}
#endif

	return ret;
}
