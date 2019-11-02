// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <regmap.h>
#include <syscon.h>

#include "pinctrl-rockchip.h"

static struct rockchip_mux_route_data rk3228_mux_route_data[] = {
	{
		/* pwm0-0 */
		.bank_num = 0,
		.pin = 26,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16),
	}, {
		/* pwm0-1 */
		.bank_num = 3,
		.pin = 21,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16) | BIT(0),
	}, {
		/* pwm1-0 */
		.bank_num = 0,
		.pin = 27,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 1),
	}, {
		/* pwm1-1 */
		.bank_num = 0,
		.pin = 30,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 1) | BIT(1),
	}, {
		/* pwm2-0 */
		.bank_num = 0,
		.pin = 28,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 2),
	}, {
		/* pwm2-1 */
		.bank_num = 1,
		.pin = 12,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 2) | BIT(2),
	}, {
		/* pwm3-0 */
		.bank_num = 3,
		.pin = 26,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 3),
	}, {
		/* pwm3-1 */
		.bank_num = 1,
		.pin = 11,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 3) | BIT(3),
	}, {
		/* sdio-0_d0 */
		.bank_num = 1,
		.pin = 1,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 4),
	}, {
		/* sdio-1_d0 */
		.bank_num = 3,
		.pin = 2,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 4) | BIT(4),
	}, {
		/* spi-0_rx */
		.bank_num = 0,
		.pin = 13,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 5),
	}, {
		/* spi-1_rx */
		.bank_num = 2,
		.pin = 0,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 5) | BIT(5),
	}, {
		/* emmc-0_cmd */
		.bank_num = 1,
		.pin = 22,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 7),
	}, {
		/* emmc-1_cmd */
		.bank_num = 2,
		.pin = 4,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 7) | BIT(7),
	}, {
		/* uart2-0_rx */
		.bank_num = 1,
		.pin = 19,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 8),
	}, {
		/* uart2-1_rx */
		.bank_num = 1,
		.pin = 10,
		.func = 2,
		.route_offset = 0x50,
		.route_val = BIT(16 + 8) | BIT(8),
	}, {
		/* uart1-0_rx */
		.bank_num = 1,
		.pin = 10,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 11),
	}, {
		/* uart1-1_rx */
		.bank_num = 3,
		.pin = 13,
		.func = 1,
		.route_offset = 0x50,
		.route_val = BIT(16 + 11) | BIT(11),
	},
};

#define RK3228_PULL_OFFSET		0x100

static void rk3228_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, struct regmap **regmap,
					 int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	*reg = RK3228_PULL_OFFSET;
	*reg += bank->bank_num * ROCKCHIP_PULL_BANK_STRIDE;
	*reg += ((pin_num / ROCKCHIP_PULL_PINS_PER_REG) * 4);

	*bit = (pin_num % ROCKCHIP_PULL_PINS_PER_REG);
	*bit *= ROCKCHIP_PULL_BITS_PER_PIN;
}

#define RK3228_DRV_GRF_OFFSET		0x200

static void rk3228_calc_drv_reg_and_bit(struct rockchip_pin_bank *bank,
					int pin_num, struct regmap **regmap,
					int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	*reg = RK3228_DRV_GRF_OFFSET;
	*reg += bank->bank_num * ROCKCHIP_DRV_BANK_STRIDE;
	*reg += ((pin_num / ROCKCHIP_DRV_PINS_PER_REG) * 4);

	*bit = (pin_num % ROCKCHIP_DRV_PINS_PER_REG);
	*bit *= ROCKCHIP_DRV_BITS_PER_PIN;
}

static struct rockchip_pin_bank rk3228_pin_banks[] = {
	PIN_BANK(0, 32, "gpio0"),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
};

static struct rockchip_pin_ctrl rk3228_pin_ctrl = {
		.pin_banks		= rk3228_pin_banks,
		.nr_banks		= ARRAY_SIZE(rk3228_pin_banks),
		.label			= "RK3228-GPIO",
		.type			= RK3288,
		.grf_mux_offset		= 0x0,
		.iomux_routes		= rk3228_mux_route_data,
		.niomux_routes		= ARRAY_SIZE(rk3228_mux_route_data),
		.pull_calc_reg		= rk3228_calc_pull_reg_and_bit,
		.drv_calc_reg		= rk3228_calc_drv_reg_and_bit,
};

static const struct udevice_id rk3228_pinctrl_ids[] = {
	{
		.compatible = "rockchip,rk3228-pinctrl",
		.data = (ulong)&rk3228_pin_ctrl
	},
	{ }
};

U_BOOT_DRIVER(pinctrl_rk3228) = {
	.name		= "rockchip_rk3228_pinctrl",
	.id		= UCLASS_PINCTRL,
	.of_match	= rk3228_pinctrl_ids,
	.priv_auto_alloc_size = sizeof(struct rockchip_pinctrl_priv),
	.ops		= &rockchip_pinctrl_ops,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.bind		= dm_scan_fdt_dev,
#endif
	.probe		= rockchip_pinctrl_probe,
};
