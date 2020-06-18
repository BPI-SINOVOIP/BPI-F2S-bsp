#ifndef __PINCTRL_SUNPLUS_I143_H__
#define __PINCTRL_SUNPLUS_I143_H__

#include <common.h>


#define pctl_err(fmt, arg...)            printf(fmt, ##arg)
#if 0
#define pctl_info(fmt, arg...)           printf(fmt, ##arg)
#else
#define pctl_info(fmt, arg...)
#endif


#define GPIO_PINGRP(x)  moon1_regs[x]
#define GPIO_MASTER(x)  group6_regs[x]
#define GPIO_OE(x)      group6_regs[x+8]
#define GPIO_OUT(x)     group6_regs[x+16]
#define GPIO_IN(x)      group6_regs[x+24]
#define GPIO_I_INV(x)   group7_regs[x]
#define GPIO_O_INV(x)   group7_regs[x+8]
#define GPIO_OD(x)      group7_regs[x+16]
#define GPIO_FIRST(x)   first_regs[x]


#define MAX_PINS        108
#define D(x,y)          ((x)*8+(y))


#define EGRP(n,v,p) { \
	.name = n, \
	.gval = (v), \
	.pins = (p), \
	.pnum = ARRAY_SIZE(p) \
}

#define FNCE(n,o,bo,bl,g) { \
	.name = n, \
	.roff = o, \
	.boff = bo, \
	.blen = bl, \
	.grps = (g), \
	.gnum = ARRAY_SIZE(g) \
}

typedef struct {
	const char * const name;
	const u8 gval;                  // value for register
	const unsigned * const pins;    // list of pins
	const unsigned pnum;            // number of pins
} sp7021grp_t;

typedef struct {
	const char * const name;
	const u8 roff;                  // register offset
	const u8 boff;                  // bit offset
	const u8 blen;                  // number of bits
	const u8 gnum;                  // number of groups
	const sp7021grp_t * const grps; // list of groups
} func_t;

extern func_t list_funcs[];
extern const int list_funcsSZ;

extern volatile u32 *moon1_regs;
extern volatile u32 *moon2_regs;
extern volatile u32 *group6_regs;
extern volatile u32 *group7_regs;
extern volatile u32 *first_regs;

extern int gpio_input_invert_set(u32 bit, u32 val);
extern int gpio_input_invert_get(u32 bit, u32 *val);
extern u32 gpio_input_invert_val_get(u32 bit);
extern int gpio_output_invert_set(u32 bit, u32 val);
extern int gpio_output_invert_get(u32 bit, u32 *val);
extern u32 gpio_output_invert_val_get(u32 bit);
extern int gpio_open_drain_set(u32 bit, u32 val);
extern int gpio_open_drain_get(u32 bit, u32 *val);
extern u32 gpio_open_drain_val_get(u32 bit);
extern int gpio_first_set(u32 bit, u32 val);
extern int gpio_first_get(u32 bit, u32 *val);
extern u32 gpio_first_val_get(u32 bit);
extern int gpio_master_set(u32 bit, u32 val);
extern int gpio_master_get(u32 bit, u32 *val);
extern u32 gpio_master_val_get(u32 bit);
extern int gpio_oe_set(u32 bit, u32 val);
extern int gpio_oe_get(u32 bit, u32 *val);
extern u32 gpio_oe_val_get(u32 bit);
extern int gpio_out_set(u32 bit, u32 val);
extern int gpio_out_get(u32 bit, u32 *val);
extern u32 gpio_out_val_get(u32 bit);
extern int gpio_in(u32 pin, u32 *val);
extern u32 gpio_in_val(u32 bit);
extern u32 gpio_para_get(u32 pin);
extern int gpio_debug_set(u32 bit, u32 val);

#endif
