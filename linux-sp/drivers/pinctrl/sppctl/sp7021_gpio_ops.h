/*
 * GPIO Driver for SunPlus/Tibbo SP7021 controller
 * Copyright (C) 2019 SunPlus Tech./Tibbo Tech.
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

#ifndef SP7021_GPIO_OPS_H
#define SP7021_GPIO_OPS_H

#include "sp7021_gpio.h"

int sp7021gpio_f_gdi( struct gpio_chip *_c, unsigned _n);

// who is first: GPIO(1) | MUX(0)
int sp7021gpio_u_gfrst( struct gpio_chip *_c, unsigned int _n);

// who is master: GPIO(1) | IOP(0)
int sp7021gpio_u_magpi( struct gpio_chip *_c, unsigned int _n);

// set MASTER and FIRST
void sp7021gpio_u_magpi_set( struct gpio_chip *_c, unsigned int _n, muxF_MG_t _f, muxM_IG_t _m);

// is inv: INVERTED(1) | NORMAL(0)
int sp7021gpio_u_isinv( struct gpio_chip *_c, unsigned int _n);
// set (I|O)inv
int sp7021gpio_u_siinv( struct gpio_chip *_c, unsigned int _n);
int sp7021gpio_u_soinv( struct gpio_chip *_c, unsigned int _n);

// is open-drain: YES(1) | NON(0)
int sp7021gpio_u_isodr( struct gpio_chip *_c, unsigned int _n);
void sp7021gpio_u_seodr( struct gpio_chip *_c, unsigned int _n, unsigned _v);

#ifndef SPPCTL_H
// take pin (export/open for ex.): set GPIO_FIRST=1,GPIO_MASTER=1
// FIX: how to prevent gpio to take over the mux if mux is the default?
// FIX: idea: save state of MASTER/FIRST and return back after _fre?
int sp7021gpio_f_req( struct gpio_chip *_c, unsigned _n);

// gave pin back: set GPIO_MASTER=0,GPIO_FIRST=0
void sp7021gpio_f_fre( struct gpio_chip *_c, unsigned _n);
#endif // SPPCTL_H

// get dir: 0=out, 1=in, -E =err (-EINVAL for ex): OE inverted on ret
int sp7021gpio_f_gdi( struct gpio_chip *_c, unsigned _n);
 
// set to input: 0:ok: OE=0
int sp7021gpio_f_sin( struct gpio_chip *_c, unsigned int _n);

// set to output: 0:ok: OE=1,O=_v
int sp7021gpio_f_sou( struct gpio_chip *_c, unsigned int _n, int _v);

// get value for signal: 0=low | 1=high | -err
int sp7021gpio_f_get( struct gpio_chip *_c, unsigned int _n);

// OUT only: can't call set on IN pin: protected by gpio_chip layer
void sp7021gpio_f_set( struct gpio_chip *_c, unsigned int _n, int _v);

 // FIX: test in-depth
int sp7021gpio_f_scf( struct gpio_chip *_c, unsigned _n, unsigned long _conf);

#ifdef CONFIG_DEBUG_FS
void sp7021gpio_f_dsh( struct seq_file *_s, struct gpio_chip *_c);
#else
#define sp7021gpio_f_dsh NULL
#endif

#ifdef CONFIG_OF_GPIO
int sp7021gpio_xlate( struct gpio_chip *_c,
        const struct of_phandle_args *_a, u32 *_flags);
#endif

int sp7021gpio_i_map( struct gpio_chip *_c, unsigned _off);

#endif // SP7021_GPIO_OPS_H
