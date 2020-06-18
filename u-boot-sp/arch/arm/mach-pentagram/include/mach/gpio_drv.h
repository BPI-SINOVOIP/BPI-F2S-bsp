#ifndef _SP_GPIO_DRV_H_
#define _SP_GPIO_DRV_H_

#include <common.h>
#include <dm.h>


extern int gpio_pin_mux_set(u32 func, u32 pin);
extern u32 gpio_pin_mux_get(u32 func);
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


#endif /* _SP_GPIO_DRV_H_ */

