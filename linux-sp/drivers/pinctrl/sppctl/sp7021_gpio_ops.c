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

#include <linux/seq_file.h>
#include <asm/io.h>

#include "sp7021_gpio.h"

#define SP7021_GPIO_OFF_GFR  0x00
#define SP7021_GPIO_OFF_CTL  0x00
#define SP7021_GPIO_OFF_OE   0x20
#define SP7021_GPIO_OFF_OUT  0x40
#define SP7021_GPIO_OFF_IN   0x60
#define SP7021_GPIO_OFF_IINV 0x00
#define SP7021_GPIO_OFF_OINV 0x20
#define SP7021_GPIO_OFF_OD   0x40

// (/16)*4
#define R16_ROF(r) (((r)>>4)<<2)
#define R16_BOF(r) ((r)%16)
// (/32)*4
#define R32_ROF(r) (((r)>>5)<<2)
#define R32_BOF(r) ((r)%32)
#define R32_VAL(r,boff) (((r)>>(boff)) & BIT(0))

int sp7021gpio_f_gdi( struct gpio_chip *_c, unsigned _n);

// who is first: GPIO(1) | MUX(0)
int sp7021gpio_u_gfrst( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = readl( pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 //KINF( _c->parent, "u F r:%X = %d %p off:%d\n", r, R32_VAL(r,R32_BOF(_n)), pc->base2, SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 return( R32_VAL(r,R32_BOF(_n)));  }

// who is master: GPIO(1) | IOP(0)
int sp7021gpio_u_magpi( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = readl( pc->base0 + SP7021_GPIO_OFF_CTL + R16_ROF(_n));
 //KINF( _c->parent, "u M r:%X = %d %p off:%d\n", r, R32_VAL(r,R16_BOF(_n)), pc->base0, SP7021_GPIO_OFF_CTL + R16_ROF(_n));
 return( R32_VAL(r,R16_BOF(_n)));  }

// set master: GPIO(1)|IOP(0), first:GPIO(1)|MUX(0)
void sp7021gpio_u_magpi_set( struct gpio_chip *_c, unsigned int _n, muxF_MG_t _f, muxM_IG_t _m) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 // FIRST
 if ( _f != muxFKEEP) {
   r = readl( pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
   //KINF( _c->parent, "F r:%X %p off:%d\n", r, pc->base2, SP7021_GPIO_OFF_GFR + R32_ROF(_n));
   if ( _f != R32_VAL(r,R32_BOF(_n))) {
     if ( _f == muxF_G) r |= BIT(R32_BOF(_n));
     else r &= ~BIT(R32_BOF(_n));
     //KINF( _c->parent, "F w:%X\n", r);
     writel( r, pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
   }
 } 
 // MASTER
 if ( _m != muxMKEEP) {
   r = (BIT(R16_BOF(_n))<<16);
   if ( _m == muxM_G) r |= BIT(R16_BOF(_n));
   //KINF( _c->parent, "M w:%X %p off:%d\n", r, pc->base0, SP7021_GPIO_OFF_CTL + R16_ROF(_n));
   writel( r, pc->base0 + SP7021_GPIO_OFF_CTL + R16_ROF(_n));
 }
 return;  }

// is inv: INVERTED(1) | NORMAL(0)
int sp7021gpio_u_isinv( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 u16 inv_off = SP7021_GPIO_OFF_IINV;
 if ( sp7021gpio_f_gdi( _c, _n) == 0) inv_off = SP7021_GPIO_OFF_OINV;
 r = readl( pc->base1 + inv_off + R16_ROF(_n));
 return( R32_VAL(r,R16_BOF(_n)));  }

void sp7021gpio_u_siinv( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 u16 inv_off = SP7021_GPIO_OFF_IINV;
 r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n));
 writel( r, pc->base1 + inv_off + R16_ROF(_n));
 return;  }
void sp7021gpio_u_soinv( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 u16 inv_off = SP7021_GPIO_OFF_OINV;
 r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n));
 writel( r, pc->base1 + inv_off + R16_ROF(_n));
 return;  }

// is open-drain: YES(1) | NON(0)
int sp7021gpio_u_isodr( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = readl( pc->base1 + SP7021_GPIO_OFF_OD + R16_ROF(_n));
 return( R32_VAL(r,R16_BOF(_n)));  }

void sp7021gpio_u_seodr( struct gpio_chip *_c, unsigned int _n, unsigned _v) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = (BIT(R16_BOF(_n))<<16) | ( ( _v & BIT(0)) << R16_BOF(_n));
 writel( r, pc->base1 + SP7021_GPIO_OFF_OD + R16_ROF(_n));
 return;  }

#ifndef SPPCTL_H
// take pin (export/open for ex.): set GPIO_FIRST=1,GPIO_MASTER=1
// FIX: how to prevent gpio to take over the mux if mux is the default?
// FIX: idea: save state of MASTER/FIRST and return back after _fre?
int sp7021gpio_f_req( struct gpio_chip *_c, unsigned _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 //KINF( _c->parent, "f_req(%03d)\n", _n);
 // get GPIO_FIRST:32
 r = readl( pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 // set GPIO_FIRST(1):32
 r |= BIT(R32_BOF(_n));
 writel( r, pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 // set GPIO_MASTER(1):m16,v:16
 r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n));
 writel( r, pc->base0 + SP7021_GPIO_OFF_CTL + R16_ROF(_n));
 return( 0);  }

// gave pin back: set GPIO_MASTER=0,GPIO_FIRST=0
void sp7021gpio_f_fre( struct gpio_chip *_c, unsigned _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 // set GPIO_MASTER(1):m16,v:16 - doesn't matter now: gpio mode is default
 //r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n);
 //writel( r, pc->base0 + SP7021_GPIO_OFF_CTL + R16_ROF(_n));
 // get GPIO_FIRST:32
 r = readl( pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 // set GPIO_FIRST(0):32
 r &= ~BIT(R32_BOF(_n));
 writel( r, pc->base2 + SP7021_GPIO_OFF_GFR + R32_ROF(_n));
 return;  }
#endif // SPPCTL_H

// get dir: 0=out, 1=in, -E =err (-EINVAL for ex): OE inverted on ret
int sp7021gpio_f_gdi( struct gpio_chip *_c, unsigned _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = readl( pc->base0 + SP7021_GPIO_OFF_OE + R16_ROF(_n));
 return( R32_VAL(r,R16_BOF(_n)) ^ BIT(0));  }
 
// set to input: 0:ok: OE=0
int sp7021gpio_f_sin( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = (BIT(R16_BOF(_n))<<16);
 writel( r, pc->base0 + SP7021_GPIO_OFF_OE + R16_ROF(_n));
 return( 0);  }

// set to output: 0:ok: OE=1,O=_v
int sp7021gpio_f_sou( struct gpio_chip *_c, unsigned int _n, int _v) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n));
 writel( r, pc->base0 + SP7021_GPIO_OFF_OE + R16_ROF(_n));
 if ( _v < 0) return( 0);
 r = (BIT(R16_BOF(_n))<<16) | ( ( _v & BIT(0)) << R16_BOF(_n));
 writel( r, pc->base0 + SP7021_GPIO_OFF_OUT + R16_ROF(_n));
 return( 0);  }

// get value for signal: 0=low | 1=high | -err
int sp7021gpio_f_get( struct gpio_chip *_c, unsigned int _n) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = readl( pc->base0 + SP7021_GPIO_OFF_IN + R32_ROF(_n));
 return( R32_VAL(r,R32_BOF(_n)));  }

// OUT only: can't call set on IN pin: protected by gpio_chip layer
void sp7021gpio_f_set( struct gpio_chip *_c, unsigned int _n, int _v) {
 u32 r;
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 r = (BIT(R16_BOF(_n))<<16) | ( _v & 0x0001) << R16_BOF(_n);
 writel( r, pc->base0 + SP7021_GPIO_OFF_OUT + R16_ROF(_n));
 return;  }

// FIX: test in-depth
int sp7021gpio_f_scf( struct gpio_chip *_c, unsigned _n, unsigned long _conf) {
 u32 r;
 int ret = 0;
 enum pin_config_param cp = pinconf_to_config_param( _conf);
 u16 ca = pinconf_to_config_argument( _conf);
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 KDBG( _c->parent, "f_scf(%03d,%lX) p:%d a:%d\n", _n, _conf, cp, ca);
 switch ( cp) {
   case PIN_CONFIG_DRIVE_OPEN_DRAIN:
          r = (BIT(R16_BOF(_n))<<16) | BIT(R16_BOF(_n));
          writel( r, pc->base1 + SP7021_GPIO_OFF_OD + R16_ROF(_n));
          break;
   case PIN_CONFIG_INPUT_ENABLE:
          KERR( _c->parent, "f_scf(%03d,%lX) input enable arg:%d\n", _n, _conf, ca);
          break;
   case PIN_CONFIG_OUTPUT:
          ret = sp7021gpio_f_sou( _c, _n, 0);
          break;
   case PIN_CONFIG_PERSIST_STATE:
          KDBG( _c->parent, "f_scf(%03d,%lX) not support pinconf:%d\n", _n, _conf, cp);
          ret = -ENOTSUPP;
          break;
   default:
       KDBG( _c->parent, "f_scf(%03d,%lX) unknown pinconf:%d\n", _n, _conf, cp);
       ret = -EINVAL;  break;
 }
 return( ret);  }

#ifdef CONFIG_DEBUG_FS
void sp7021gpio_f_dsh( struct seq_file *_s, struct gpio_chip *_c) {
 int i;
 const char *label;
 for ( i = 0; i < _c->ngpio; i++) {
   if ( !( label = gpiochip_is_requested( _c, i))) label = "";
   seq_printf( _s, " gpio-%03d (%-16.16s | %-16.16s)", i + _c->base, _c->names[ i], label);
   seq_printf( _s, " %c", sp7021gpio_f_gdi( _c, i) == 0 ? 'O' : 'I');
   seq_printf( _s, ":%d", sp7021gpio_f_get( _c, i));
   seq_printf( _s, " %s", ( sp7021gpio_u_gfrst( _c, i) ? "gpi" : "mux"));
   seq_printf( _s, " %s", ( sp7021gpio_u_magpi( _c, i) ? "gpi" : "iop"));
   seq_printf( _s, " %s", ( sp7021gpio_u_isinv( _c, i) ? "inv" : "   "));
   seq_printf( _s, " %s", ( sp7021gpio_u_isodr( _c, i) ? "oDr" : ""));
   seq_printf( _s, "\n");
 }
 return;  }
#else
#define sp7021gpio_f_dsh NULL
#endif

#ifdef CONFIG_OF_GPIO
int sp7021gpio_xlate( struct gpio_chip *_c,
        const struct of_phandle_args *_a, u32 *_flags) {
 //// sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 //KDBG( "%s(%X) %d\n", __FUNCTION__, _a->args[ 0], _a->args[ 1]);
 ////if ( _a->args[ 0] > GPIS_listSZ) return( -EINVAL);
 ////if ( _flags) *_flags = _a->args[ 1];
 return( of_gpio_simple_xlate( _c, _a, _flags));  }
#endif

int sp7021gpio_i_map( struct gpio_chip *_c, unsigned _off) {
 sp7021gpio_chip_t *pc = ( sp7021gpio_chip_t *)gpiochip_get_data( _c);
 if ( _off >= 8 && _off < 15) return( pc->irq[ _off - 8]);
 return( -ENXIO);  }
