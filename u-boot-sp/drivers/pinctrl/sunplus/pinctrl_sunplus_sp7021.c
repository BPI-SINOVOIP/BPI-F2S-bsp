#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <mach/gpio_drv.h>
#include <asm/io.h>

#include "pinctrl_sunplus_sp7021.h"
#include <dt-bindings/pinctrl/sp7021.h>

DECLARE_GLOBAL_DATA_PTR;

//#define PINCTRL_DEBUG

volatile u32 *moon1_regs = NULL;
volatile u32 *moon2_regs = NULL;
volatile u32 *group6_regs = NULL;
volatile u32 *group7_regs = NULL;
volatile u32 *first_regs = NULL;
void* pin_registered_by_udev[MAX_PINS];

#ifdef PINCTRL_DEBUG
void pinmux_reg_dump(void)
{
	u32 pinmux_value[120];
	int i;

	printf("pinmux_reg_dump \n");
	for (i = MUXF_L2SW_CLK_OUT; i <= MUXF_GPIO_INT7; i++) {
		pinmux_value[i-MUXF_L2SW_CLK_OUT]  = gpio_pin_mux_get(i);
	}

	printf("l2sw = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[0], pinmux_value[1], pinmux_value[2], pinmux_value[3], pinmux_value[4],
		pinmux_value[5], pinmux_value[6], pinmux_value[7], pinmux_value[8], pinmux_value[9]);
	printf("l2sw = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[10], pinmux_value[11], pinmux_value[12], pinmux_value[13], pinmux_value[14],
		pinmux_value[15], pinmux_value[16], pinmux_value[17], pinmux_value[18], pinmux_value[19]);
	printf("l2sw = 0x%02x 0x%02x ---- ---- ---- ---- ---- ---- ---- ---- \n",
		pinmux_value[20], pinmux_value[21]);
	printf("sdio = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- \n",
		pinmux_value[22], pinmux_value[23], pinmux_value[24], pinmux_value[25], pinmux_value[26],
		pinmux_value[27]);
	printf("pwm  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[28], pinmux_value[29], pinmux_value[30], pinmux_value[31], pinmux_value[32],
		pinmux_value[33], pinmux_value[34], pinmux_value[35]);
	printf("icm  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[36], pinmux_value[37], pinmux_value[38], pinmux_value[39], pinmux_value[40],
		pinmux_value[41], pinmux_value[42], pinmux_value[43]);
	printf("spim = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[44], pinmux_value[45], pinmux_value[46], pinmux_value[47], pinmux_value[48],
		pinmux_value[49], pinmux_value[50], pinmux_value[51], pinmux_value[52], pinmux_value[53]);
	printf("spim = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[54], pinmux_value[55], pinmux_value[56], pinmux_value[57], pinmux_value[58],
		pinmux_value[59], pinmux_value[60], pinmux_value[61], pinmux_value[62], pinmux_value[63]);
	printf("spis = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[64], pinmux_value[65], pinmux_value[66], pinmux_value[67], pinmux_value[68],
		pinmux_value[69], pinmux_value[70], pinmux_value[71], pinmux_value[72], pinmux_value[73]);
	printf("spis = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[74], pinmux_value[75], pinmux_value[76], pinmux_value[77], pinmux_value[78],
		pinmux_value[79], pinmux_value[80], pinmux_value[81], pinmux_value[82], pinmux_value[83]);
	printf("i2c  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[84], pinmux_value[85], pinmux_value[86], pinmux_value[87], pinmux_value[88],
		pinmux_value[89], pinmux_value[90], pinmux_value[91]);
	printf("uart = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
		pinmux_value[92], pinmux_value[93], pinmux_value[94], pinmux_value[95], pinmux_value[96],
		pinmux_value[97], pinmux_value[98], pinmux_value[99], pinmux_value[100], pinmux_value[101]);
	printf("uart = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- \n",
		pinmux_value[102], pinmux_value[103], pinmux_value[104], pinmux_value[105], pinmux_value[106],
		pinmux_value[107]);
	printf("tim  = 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- ---- ---- ---- ---- \n",
		pinmux_value[108], pinmux_value[109], pinmux_value[110], pinmux_value[111]);
	printf("int  = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ---- ---- \n",
		pinmux_value[112], pinmux_value[113], pinmux_value[114], pinmux_value[115], pinmux_value[116],
		pinmux_value[117], pinmux_value[118], pinmux_value[119]);
}

void gpio_reg_dump(void)
{
	int i;
	u32 gpio_value[MAX_PINS];

	printf("gpio_reg_dump (FI MA IN II OU OI OE OD)\n");
	for (i = 0; i < MAX_PINS; i++) {
		gpio_value[i] = gpio_para_get(i);
		if ((i%8) == 7) {
			printf("GPIO_P%-2d 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",(i/8),
			gpio_value[i-7],gpio_value[i-6],gpio_value[i-5],gpio_value[i-4],
			gpio_value[i-3],gpio_value[i-2],gpio_value[i-1],gpio_value[i]);
		}
		if (i == (MAX_PINS-1)) {
			printf("GPIO_P%-2d 0x%02x 0x%02x 0x%02x\n",(i/8),
			gpio_value[i-2],gpio_value[i-1],gpio_value[i]);
		}
	}
}
#endif

static int register_pin(int pin, struct udevice *dev)
{
	// Check if pin number is within range.
	if ((pin >= 0) && (pin < MAX_PINS)) {
		if (pin_registered_by_udev[pin] == 0) {
			// If the pin has not been registered, register it by
			// save a pointer to the registering device.
			pin_registered_by_udev[pin] = dev;
			pctl_info("Register pin %d (from node: %s).\n", pin, dev->name);
			return 0;
		} else {
			pctl_err("ERROR: Pin %d of node %s has been registered (by node: %s)!\n",
				pin, dev->name, ((struct udevice*)pin_registered_by_udev[pin])->name);
			return (int)dev;
		}
	} else {
		pctl_err("ERROR: Invalid pin number %d from '%s'!\n", pin, dev->name);
		return -1;
	}
}

static int unregister_pin(int pin)
{
	// Check if pin number is within range.
	if ((pin >= 0) && (pin < MAX_PINS)) {
		if (pin_registered_by_udev[pin]) {
			pin_registered_by_udev[pin] = 0;
			return 0;
		 }
	}
	return -1;
}


static int sunplus_pinctrl_pins(struct udevice *dev)
{
	int offset = dev_of_offset(dev);
	u32 pin_mux[MAX_PINS];
	int len, i;

	// Get property: "sppctl,pins"
	len = fdtdec_get_int_array_count(gd->fdt_blob, offset,
					"sppctl,pins", pin_mux,
					ARRAY_SIZE(pin_mux));
	pctl_info("Number of entries of 'sppctl,pins' = %d\n", len);

	// Register all pins.
	for (i = 0; i < len; i++) {
		int pin  = (pin_mux[i] >> 24) & 0xff;

		if (register_pin(pin, dev) != 0) break;
	}

	// If any pin was not registered successfully, return -1.
	if ((len > 0) && (i != len)) return -1;

	// All pins were registered successfully, set up all pins.
	for (i = 0; i < len; i++) {
		int pins = pin_mux[i];
		int pin  = (pins >> 24) & 0xff;
		int type = (pins >> 16) & 0xff;
		int func = (pins >> 8) & 0xff;
		int flag = pins & 0xff;
		pctl_info("sppctl,pins = 0x%08x\n", pins);

		if (type == SP7021_PCTL_G_PMUX) {
			// It's a PinMux pin.
			gpio_pin_mux_set(func, pin);
			gpio_first_set(pin, 0);
			gpio_master_set(pin, 1);
			pctl_info("pinmux get = 0x%02x \n", gpio_pin_mux_get(func));
		} else if (type == SP7021_PCTL_G_IOPP) {
			// It's a IOP pin.
			gpio_first_set(pin, 1);
			gpio_master_set(pin, 0);
		} else if (type == SP7021_PCTL_G_GPIO) {
			// It's a GPIO pin.
			gpio_first_set(pin, 1);
			gpio_master_set(pin, 1);

			if (flag & (SP7021_PCTL_L_OUT|SP7021_PCTL_L_OU1)) {
				if (flag & SP7021_PCTL_L_OUT) {
					gpio_out_set(pin, 0);
				}
				if (flag & SP7021_PCTL_L_OU1) {
					gpio_out_set(pin, 1);
				}
				gpio_oe_set(pin, 1);
			} else if (flag & SP7021_PCTL_L_ODR) {
				gpio_open_drain_set(pin, 1);
			}

			if (flag & SP7021_PCTL_L_INV) {
				gpio_input_invert_set(pin, 1);
			} else {
				gpio_input_invert_set(pin, 0);
			}
			if (flag & SP7021_PCTL_L_ONV) {
				gpio_output_invert_set(pin, 1);
			} else {
				gpio_output_invert_set(pin, 0);
			}
		}
	}

	return 0;
}

static int sunplus_pinctrl_zero(struct udevice *dev)
{
	int offset = dev_of_offset(dev);
	u32 pin_mux[MAX_PINS];
	int len, i;

	// Get property: "sppctl,zero_func"
	len = fdtdec_get_int_array_count(gd->fdt_blob, offset,
					"sppctl,zero_func", pin_mux,
					ARRAY_SIZE(pin_mux));
	pctl_info("Number of entries of 'sppctl,zero_func' = %d\n", len);

	// All pins were registered successfully, set up all pins.
	for (i = 0; i < len; i++) {
		int func = pin_mux[i];
		pctl_info("sppctl,zero_func = 0x%08x\n", func);

		// Set it to no use.
		gpio_pin_mux_set(func, 0);
		pctl_info("pinmux get = 0x%02x \n", gpio_pin_mux_get(func));
	}

	return 0;
}

static int sunplus_pinctrl_function(struct udevice *dev)
{
	int offset = dev_of_offset(dev);
	const char *pin_func;
	const char *pin_group;
	int len, i;

	// Get property: 'sppctl,function'
	pin_func = fdt_getprop(gd->fdt_blob, offset, "sppctl,function", &len);
	pctl_info("sppctl,function = %s (%d)\n", pin_func, len);
	if (len > 1) {
		// Find 'pin_func' string in list.
		for (i = 0; i < list_funcsSZ; i++) {
			if (strcmp (pin_func, list_funcs[i].name) == 0) break;
		}
		if (i == list_funcsSZ) {
			pctl_err("Error: Invalid 'sppctl,function' in node %s! "
				"Cannot find \"%s\"!\n", dev->name, pin_func);
			return -1;
		}

		// 'pin_func' is found! Next, find its group.
		// Get property: 'sppctl,groups'
		pin_group = fdt_getprop(gd->fdt_blob, offset, "sppctl,groups", &len);
		pctl_info("sppctl,groups = %s (%d)\n", pin_group, len);
		if (len > 1) {
			func_t *func = &list_funcs[i];

			// Find 'pin_group' string in list.
			for (i = 0; i < func->gnum; i++) {
				if (strcmp (pin_group, func->grps[i].name) == 0) break;
			}
			if (i == func->gnum) {
				pctl_err("Error: Invalid 'sppctl,groups' in node %s! "
					"Cannot find \"%s\"!\n", dev->name, pin_group);
				return -1;
			}

			// 'pin_group' is found!
			const sp7021grp_t *grp = &func->grps[i];

			// Register all pins of the group.
			for (i = 0; i < grp->pnum; i++) {
				if (register_pin(grp->pins[i], dev) != 0) break;
			}

			// Check if all pins of the group register successfully
			if (i == grp->pnum) {
				int mask;

				// All pins of the group was registered successfully.
				// Enable the pin-group.
				mask = (1 << func->blen) - 1;
				GPIO_PINGRP(func->roff) = (mask<<(func->boff+16)) | (grp->gval<<func->boff);
			} else {
				return -1;
			}
		} else if (len <= 1) {
			pctl_err("Error: Invalid 'sppctl,groups' in node %s!\n", dev->name);
			return -1;
		}
	} else if (len == 1) {
		pctl_err("Error: Invalid 'sppctl,function' in node %s!\n", dev->name);
		return -1;
	}

	return 0;
}

static int sunplus_pinctrl_set_state(struct udevice *dev, struct udevice *config)
{
	int ret;

	pctl_info("\nConfig node: %s\n",config->name);
	ret = sunplus_pinctrl_pins(config);
	if (ret != 0) return ret;

	sunplus_pinctrl_zero(config);

	ret = sunplus_pinctrl_function(config);
	if (ret != 0) return ret;

#ifdef PINCTRL_DEBUG
	pinmux_reg_dump();
	gpio_reg_dump();
#endif

	return 0;
}

static int sunplus_pinctrl_probe(struct udevice *dev)
{
	int i;

	// Get the 1st address of property 'reg' from dtb.
	moon2_regs = (void*)devfdt_get_addr_index(dev, 0);
	pctl_info("moon2_regs = %08x\n", (int)moon2_regs);
	if ((int)moon2_regs == -1) {
		pctl_err("Failed to get base address of MOON2!\n");
		return -EINVAL;
	}

	// Get the 2nd address of property 'reg' from dtb.
	group6_regs = (void*)devfdt_get_addr_index(dev, 1);
	pctl_info("group6_regs = %08x\n", (int)group6_regs);
	if ((int)group6_regs == -1) {
		pctl_err("Failed to get base address of GROUP6!\n");
		return -EINVAL;
	}

	// Get the 3rd address of property 'reg' from dtb.
	group7_regs = (void*)devfdt_get_addr_index(dev, 2);
	pctl_info("group7_regs = %08x\n", (int)group7_regs);
	if ((int)group7_regs == -1) {
		pctl_err("Failed to get base address of GROUP7!\n");
		return -EINVAL;
	}

	// Get the 4th address of property 'reg' from dtb.
	first_regs = (void*)devfdt_get_addr_index(dev, 3);
	pctl_info("first_regs = %08x\n", (int)first_regs);
	if ((int)first_regs == -1) {
		pctl_err("Failed to get base address of FIRST!\n");
		return -EINVAL;
	}

	// Get the 5th address of property 'reg' from dtb.
	moon1_regs = (void*)devfdt_get_addr_index(dev, 4);
	pctl_info("moon1_regs = %08x\n", (int)moon1_regs);
	if ((int)moon1_regs == -1) {
		pctl_err("Failed to get base address of MOON1!\n");
		return -EINVAL;
	}

	// Unregister all pins.
	for (i = 0; i < MAX_PINS; i++) {
		unregister_pin(i);
	}

	return 0;
}

static struct pinctrl_ops sunplus_pinctrl_ops = {
	.set_state      = sunplus_pinctrl_set_state,
};

static const struct udevice_id sunplus_pinctrl_ids[] = {
	{ .compatible = "sunplus,sp7021-pctl"},
	{ /* zero */ }
};

U_BOOT_DRIVER(pinctrl_sunplus) = {
	.name           = "sunplus_pinctrl",
	.id             = UCLASS_PINCTRL,
	.probe          = sunplus_pinctrl_probe,
	.of_match       = sunplus_pinctrl_ids,
	.ops            = &sunplus_pinctrl_ops,
	.bind           = dm_scan_fdt_dev,
};
