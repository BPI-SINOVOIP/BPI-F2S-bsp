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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <asm/io.h>

#include "sp7021_gpio_ops.h"
#include "sp7021_gpio.h"

#if 0 //Test code for GPIO_INT0
static irqreturn_t gpio_int_0(int irq, void *data)
{
	printk(KERN_INFO "register gpio int0 trigger \n");
	return IRQ_HANDLED;
}

#endif

int sp7021_gpio_resmap( struct platform_device *_pd, sp7021gpio_chip_t *_pc) {
 struct resource *rp;
 // res0
 if ( IS_ERR( rp = platform_get_resource( _pd, IORESOURCE_MEM, 1))) {
   KERR( &( _pd->dev), "%s get res#0 ERR\n", __FUNCTION__);
   return( PTR_ERR( rp));  }
 KDBG( &( _pd->dev), "mres #0:%p\n", rp);
 if ( !rp) return( -EFAULT);
 KDBG( &( _pd->dev), "mapping [%X-%X]\n", rp->start, rp->end);
 if ( IS_ERR( _pc->base0 = devm_ioremap_resource( &( _pd->dev), rp))) {
   KERR( &( _pd->dev), "%s map res#0 ERR\n", __FUNCTION__);
   return( PTR_ERR( _pc->base0));  }
 // res1
 if ( IS_ERR( rp = platform_get_resource( _pd, IORESOURCE_MEM, 2))) {
   KERR( &( _pd->dev), "%s get res#1 ERR\n", __FUNCTION__);
   return( PTR_ERR( rp));  }
 KDBG( &( _pd->dev), "mres #1:%p\n", rp);
 if ( !rp) return( -EFAULT);
 KDBG( &( _pd->dev), "mapping [%X-%X]\n", rp->start, rp->end);
 if ( IS_ERR( _pc->base1 = devm_ioremap_resource( &( _pd->dev), rp))) {
   KERR( &( _pd->dev), "%s map res#1 ERR\n", __FUNCTION__);
   return( PTR_ERR( _pc->base1));  }
 // res2
 if ( IS_ERR( rp = platform_get_resource( _pd, IORESOURCE_MEM, 3))) {
   KERR( &( _pd->dev), "%s get res#2 ERR\n", __FUNCTION__);
   return( PTR_ERR( rp));  }
 KDBG( &( _pd->dev), "mres #2:%p\n", rp);
 if ( !rp) return( -EFAULT);
 KDBG( &( _pd->dev), "mapping [%X-%X]\n", rp->start, rp->end);
 if ( IS_ERR( _pc->base2 = devm_ioremap_resource( &( _pd->dev), rp))) {
   KERR( &( _pd->dev), "%s map res#2 ERR\n", __FUNCTION__);
   return( PTR_ERR( _pc->base2));  }
 return( 0);  }

int sp7021_gpio_new( struct platform_device *_pd, void *_datap) {
 struct device_node *np = _pd->dev.of_node, *npi;
 sp7021gpio_chip_t *pc = NULL;
 struct gpio_chip *gchip = NULL;
 int err = 0, i = 0, npins;
#ifdef SPPCTL_H
 sppctl_pdata_t *_pctrlp = ( sppctl_pdata_t *)_datap;
#endif
 if ( !np) {
   KERR( &( _pd->dev), "invalid devicetree node\n");
   return( -EINVAL);  }
 if ( !of_device_is_available( np)) {
   KERR( &( _pd->dev), "devicetree status is not available\n");
   return( -ENODEV);  }
// print_device_tree_node( np, 0);
 for_each_child_of_node( np, npi) {
   if ( of_find_property( npi, "gpio-controller", NULL)) {  i = 1;  break;  }
 }
 if ( of_find_property( np, "gpio-controller", NULL)) i = 1;
 if ( i == 0) {
   KERR( &( _pd->dev), "is not gpio-controller\n");
   return( -ENODEV);  }
 if ( !( pc = devm_kzalloc( &( _pd->dev), sizeof( *pc), GFP_KERNEL))) return( -ENOMEM);
 gchip = &( pc->chip);
#ifdef SPPCTL_H
 pc->base0 = _pctrlp->base0;
 pc->base1 = _pctrlp->base1;
 pc->base2 = _pctrlp->base2;
 _pctrlp->gpiod = pc;
#else
 if ( ( err = sp7021_gpio_resmap( _pd, pc)) != 0) {
   devm_kfree( &( _pd->dev), pc);
   return( err);  }
#endif
 gchip->label = MNAME;
 gchip->parent = &( _pd->dev);
 gchip->owner = THIS_MODULE;
#ifndef SPPCTL_H
 gchip->request =           sp7021gpio_f_req;
 gchip->free =              sp7021gpio_f_fre;
#else
 gchip->request =           gpiochip_generic_request; // place new calls there
 gchip->free =              gpiochip_generic_free;
#endif
 gchip->get_direction =     sp7021gpio_f_gdi;
 gchip->direction_input =   sp7021gpio_f_sin;
 gchip->direction_output =  sp7021gpio_f_sou;
 gchip->get =               sp7021gpio_f_get;
 gchip->set =               sp7021gpio_f_set;
 gchip->set_config =        sp7021gpio_f_scf;
 gchip->dbg_show =          sp7021gpio_f_dsh;
 gchip->base = 0; // it is main platform GPIO controller
 gchip->ngpio =             GPIS_listSZ;
 gchip->names =             sp7021gpio_list_s;
 gchip->can_sleep = 0;
#if defined(CONFIG_OF_GPIO)
 gchip->of_node = np;
 gchip->of_gpio_n_cells = 2;
 gchip->of_xlate =          sp7021gpio_xlate;
#endif
 gchip->to_irq =            sp7021gpio_i_map;
#ifdef SPPCTL_H
 _pctrlp->gpio_range.npins =    gchip->ngpio;
 _pctrlp->gpio_range.base =     gchip->base;
 _pctrlp->gpio_range.name =     gchip->label;
 _pctrlp->gpio_range.gc =       gchip;
#endif
 // FIXME: can't set pc globally
 if ( ( err = devm_gpiochip_add_data( &( _pd->dev), gchip, pc)) < 0) {
   KERR( &( _pd->dev), "gpiochip add failed\n");
   return( err);  }
 npins = platform_irq_count( _pd);
 for ( i = 0; i < npins && i < SP7021_GPIO_IRQS; i++) {
    pc->irq[ i] = irq_of_parse_and_map( np, i);
    KDBG( &( _pd->dev), "setting up irq#%d -> %d\n", i, pc->irq[ i]);
 }

#if 0 //Test code for GPIO_INT0
	err = devm_request_irq(&( _pd->dev), pc->irq[ 0], gpio_int_0, IRQF_TRIGGER_RISING, "sppctl_gpio_int0", _pd);
	KDBG( &( _pd->dev), "register gpio int0 irq \n");
	if (err) {
		KDBG( &( _pd->dev), "register gpio int0 irq err \n");
		return err;
	}
#endif

 spin_lock_init( &( pc->lock));
#ifndef SPPCTL_H
 printk( KERN_INFO M_NAM" by "M_ORG" "M_CPR);
#endif
 return( 0);  }

int sp7021_gpio_del( struct platform_device *_pd, void *_datap) {
 //sp7021gpio_chip_t *cp;
 // FIXME: can't use globally now
 //if ( ( cp = platform_get_drvdata( _pd)) == NULL) return( -ENODEV);
 //gpiochip_remove( &( cp->chip));
 // FIX: remove spinlock_t ?
 return( 0);  }

static const struct of_device_id sp7021_gpio_of_match[] = {
 { .compatible = "sunplus,sp7021-gpio", },
 { /* null */ }
};

#ifdef SPPCTL_H
#else
static int sp7021_gpio_probe( struct platform_device *_pd) {
 return( sp7021_gpio_new( _pd, NULL));  }
static int sp7021_gpio_remove( struct platform_device *_pd, void *_data) {
 return( sp7021_gpio_del( _pd, NULL));  }
MODULE_DEVICE_TABLE(of, sp7021_gpio_of_match);
MODULE_ALIAS( "platform:" MNAME);
static struct platform_driver sp7021_gpio_driver = {
 .driver = {
	.name	= MNAME,
	.owner	= THIS_MODULE,
	.of_match_table = sp7021_gpio_of_match,
 },
 .probe		= sp7021_gpio_probe,
 .remove	= sp7021_gpio_remove,
};
module_platform_driver(sp7021_gpio_driver);
#endif

#if 0
static int __init sp7021_gpio_drv_reg( void) {
 return platform_driver_register( &sp7021_gpio_driver);  }
postcore_initcall( sp7021_gpio_drv_reg);
static void __exit sp7021_gpio_exit( void) {
 platform_driver_unregister( &sp7021_gpio_driver);  }
module_exit(sp7021_gpio_exit);
#endif

MODULE_LICENSE(M_LIC);
MODULE_AUTHOR(M_AUT);
MODULE_DESCRIPTION(M_NAM);
