// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus SP7021 Reset Controller driver
 *
 * Copyright (C) 2020 Sunplus Inc.
 * Author: qinjian <qinjian@sunmedia.com.cn>
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <reset-uclass.h>
#ifdef CONFIG_TARGET_PENTAGRAM_I143_P
#include <dt-bindings/reset/sp-i143.h>
#else
#include <dt-bindings/reset/sp-q628.h>
#endif

#define BITASSERT(id, val)	(BIT(16 + (id)) | ((val) << (id)))

struct sp7021_reset_priv {
	void *membase;
};

static int sp7021_reset_update(struct reset_ctl *reset_ctl, int assert)
{
	struct sp7021_reset_priv *priv = dev_get_priv(reset_ctl->dev);
	int reg_width = sizeof(u32)/2;
	int bank = reset_ctl->id / (reg_width * BITS_PER_BYTE);
	int offset = reset_ctl->id % (reg_width * BITS_PER_BYTE);
	void *addr;

	addr = priv->membase + (bank * 4);
	writel(BITASSERT(offset, assert), addr);

	return 0;
}

static int sp7021_reset_status(struct reset_ctl *reset_ctl)
{ 
	struct sp7021_reset_priv *priv = dev_get_priv(reset_ctl->dev);
	int reg_width = sizeof(u32)/2;
	int bank = reset_ctl->id / (reg_width * BITS_PER_BYTE);
	int offset = reset_ctl->id % (reg_width * BITS_PER_BYTE);
	u32 reg;

	reg = readl(priv->membase + (bank * 4));

	return !!(reg & BIT(offset));
}

static int sp7021_reset_assert(struct reset_ctl *reset_ctl)
{
	return sp7021_reset_update(reset_ctl, 1);
}

static int sp7021_reset_deassert(struct reset_ctl *reset_ctl)
{
	return sp7021_reset_update(reset_ctl, 0);
}

struct reset_ops sp7021_reset_ops = {
	.rst_assert   = sp7021_reset_assert,
	.rst_deassert = sp7021_reset_deassert,
	.rst_status   = sp7021_reset_status,
};

static const struct udevice_id sp7021_reset_ids[] = {
	{ .compatible = "sunplus,sp-reset" },
	{ .compatible = "sunplus,sp7021-reset" },
	{ /* sentinel */ }
};

static int sp7021_reset_probe(struct udevice *dev)
{
	struct sp7021_reset_priv *priv = dev_get_priv(dev);

	priv->membase = (void *)devfdt_get_addr(dev);

	//printf("!!! %s: %p\n", __FUNCTION__, priv->membase);
	return 0;
}

U_BOOT_DRIVER(sp7021_reset) = {
	.name = "sp7021_reset",
	.id = UCLASS_RESET,
	.of_match = sp7021_reset_ids,
	.probe = sp7021_reset_probe,
	.ops = &sp7021_reset_ops,
	.priv_auto_alloc_size = sizeof(struct sp7021_reset_priv),
};
