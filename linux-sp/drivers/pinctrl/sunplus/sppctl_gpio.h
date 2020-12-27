/*
 * GPIO Driver for Sunplus/Tibbo SP7021 controller
 * Copyright (C) 2020 Sunplus Tech./Tibbo Tech.
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
 *
 */

#ifndef SPPCTL_GPIO_H
#define SPPCTL_GPIO_H

#define SPPCTL_GPIO_IRQS 8

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/gpio/driver.h>
#include <linux/stringify.h>

#include "sppctl.h"

#ifndef SPPCTL_H

#ifdef CONFIG_PINCTRL_SPPCTL
#define MNAME "sp7021_gpio"
#define M_NAM "SP7021 GPIO"
#else
#define MNAME "i143_gpio"
#define M_NAM "I143 GPIO"
#endif
#define M_LIC "GPL v2"
#define M_AUT "Dvorkin Dmitry <dvorkin@tibbo.com>"

#define M_ORG "Sunplus/Tibbo Tech."
#define M_CPR "(C) 2020"

#define KINF(pd,fmt,args...) { \
	if ((pd) != NULL) { dev_info((pd),""fmt,##args);  \
	} else { printk(KERN_INFO     MNAME": "fmt,##args); } }
#define KERR(pd,fmt,args...) { \
	if ((pd) != NULL) { dev_info((pd),""fmt,##args);  \
	} else { printk(KERN_ERR      MNAME": "fmt,##args); } }
#ifdef CONFIG_DEBUG_GPIO
#define KDBG(pd,fmt,args...) { \
	if ((pd) != NULL) { dev_info((pd),""fmt,##args);  \
	} else { printk(KERN_DEBUG    MNAME": "fmt,##args); } }
#else
#define KDBG(pd,fmt,args...)
#endif

#endif // SPPCTL_H

typedef struct sppctlgpio_chip_T {
	spinlock_t lock;
	struct gpio_chip chip;
	void __iomem *base0;   // MASTER , OE , OUT , IN
	void __iomem *base1;   // I_INV , O_INV , OD
	void __iomem *base2;   // GPIO_FIRST
	int irq[SPPCTL_GPIO_IRQS];
} sppctlgpio_chip_t;

extern const char * const sppctlgpio_list_s[];
extern const size_t GPIS_listSZ;

int sppctl_gpio_new(struct platform_device *_pd, void *_datap);
int sppctl_gpio_del(struct platform_device *_pd, void *_datap);

#ifdef CONFIG_PINCTRL_SPPCTL
#define D_PIS(x,y) "P" __stringify(x) "_0" __stringify(y)
#else
#define D_PIS(x) "GPIO" __stringify(x)
#endif

// FIRST: MUX=0, GPIO=1
enum muxF_MG {
	muxF_M = 0,
	muxF_G = 1,
	muxFKEEP = 2,
};
// MASTER: IOP=0,GPIO=1
enum muxM_IG {
	muxM_I = 0,
	muxM_G = 1,
	muxMKEEP = 2,
};

typedef enum muxF_MG muxF_MG_t;
typedef enum muxM_IG muxM_IG_t;

#endif // SPPCTL_GPIO_H
