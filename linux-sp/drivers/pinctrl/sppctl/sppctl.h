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

#ifndef SPPCTL_H
#define SPPCTL_H

#define MNAME "sppctl"
#define M_LIC "GPL v2"
#define M_AUT "Dvorkin Dmitry dvorkin@tibbo.com"
#define M_NAM "SP7021 PinCtl"
#define M_ORG "SunPlus/Tibbo Tech."
#define M_CPR "(C) 2019-2019"

//#define FW_DEFNAME "sppctl.bin"
#define FW_DEFNAME NULL

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/sysfs.h>
#include <linux/printk.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

#include <mach/io_map.h>
#include <dt-bindings/pinctrl/sp7021.h>

#define SPPCTL_MAX_NAM 64
#define SPPCTL_MAX_BUF PAGE_SIZE

#define KINF(pd,fmt,args...) { \
    if ( (pd) != NULL) {  dev_info((pd),""fmt,##args);  \
    } else {  printk( KERN_INFO     MNAME": "fmt,##args);  }  }
#define KERR(pd,fmt,args...) { \
    if ( (pd) != NULL) {  dev_info((pd),""fmt,##args);  \
    } else {  printk( KERN_ERR      MNAME": "fmt,##args);  }  }
#ifdef CONFIG_PINCTRL_SPPCTL_DEBUG
#define KDBG(pd,fmt,args...) { \
    if ( (pd) != NULL) {  dev_info((pd),""fmt,##args);  \
    } else {  printk( KERN_DEBUG    MNAME": "fmt,##args);  }  }
#else
#define KDBG(pd,fmt,args...) 
#endif

#include "sp7021_gpio.h"

//#define MOON_REG_BASE 0x9C000000
//#define MOON_REG_N(n) 0x80*(n)+MOON_REG_BASE
#define MOON_REG_N(n) PA_IOB_ADDR(0x80*(n))

typedef struct sp7021gpio_chip_T sp7021gpio_chip_t;

typedef struct sppctl_pdata_T {
 char name[ SPPCTL_MAX_NAM];
 uint8_t debug;
 char fwname[ SPPCTL_MAX_NAM];
 void *sysfs_sdp;
 void __iomem *baseF;   // functions
 void __iomem *base0;   // MASTER , OE , OUT , IN
 void __iomem *base1;   // I_INV , O_INV , OD
 void __iomem *base2;   // GPIO_FIRST
 void __iomem *baseI;   // IOP
 // pinctrl-related
 struct pinctrl_desc pdesc;
 struct pinctrl_dev *pcdp;
 struct pinctrl_gpio_range gpio_range;
 sp7021gpio_chip_t *gpiod;
} sppctl_pdata_t;

typedef struct sppctl_reg_T {
 uint16_t v;		// value part
 uint16_t m;		// mask part
} sppctl_reg_t;

#include "sppctl_sysfs.h"
#include "sppctl_pinctrl.h"

void sppctl_gmx_set( sppctl_pdata_t *_p, uint8_t _roff, uint8_t _boff, uint8_t _bsiz, uint8_t _rval);
uint8_t sppctl_gmx_get( sppctl_pdata_t *_p, uint8_t _roff, uint8_t _boff, uint8_t _bsiz);
void sppctl_pin_set( sppctl_pdata_t *_p, uint8_t _pin, uint8_t _fun);
uint8_t sppctl_fun_get( sppctl_pdata_t *_p, uint8_t _pin);
void sppctl_loadfw( struct device *_dev, const char *_fwname);

typedef enum {
 fOFF_0,    // nowhere
 fOFF_M,    // in mux registers
 fOFF_G,    // mux group registers
 fOFF_I,    // in iop registers
} fOFF_t;

typedef struct {
 const char *name;
 uint8_t gval;                          // value for register
 const unsigned *pins;   // list of pins
 const unsigned pnum;                   // number of pins
} sp7021grp_t;

#define EGRP(n,v,p) { \
    .name = n, \
    .gval = (v), \
    .pins = (p), \
    .pnum = ARRAY_SIZE(p), \
}

typedef struct {
 const char *name;
 fOFF_t freg;               // function register type
 uint8_t roff;              // +register offset
 uint8_t boff;              // bit offset
 uint8_t blen;              // number of bits
 const sp7021grp_t *grps;   // list of groups
 const unsigned gnum;       // number of groups
 char *grps_sa[100];     // array of pointers to func's grps names
// char **grps_sa;     // array of pointers to func's grps names
} func_t;

#define FNCE(n,r,o,bo,bl,g) { \
    .name = n, \
    .freg = r, \
    .roff = o, \
    .boff = bo, \
    .blen = bl, \
    .grps = (g), \
    .gnum = ARRAY_SIZE(g), \
}

#define FNCN(n,r,o,bo,bl) { \
    .name = n, \
    .freg = r, \
    .roff = o, \
    .boff = bo, \
    .blen = bl, \
    .grps = NULL, \
    .gnum = 0, \
}
extern func_t list_funcs[];
extern const size_t list_funcsSZ;

extern const char * const sp7021pmux_list_s[];
extern const size_t PMUX_listSZ;

typedef struct grp2fp_map_T {
 uint16_t f_idx;        // function index
 uint16_t g_idx;        // pins/group index inside function
} grp2fp_map_t;

// for debug
void print_device_tree_node(struct device_node *node, int depth);

#endif // SPPCTL_H
