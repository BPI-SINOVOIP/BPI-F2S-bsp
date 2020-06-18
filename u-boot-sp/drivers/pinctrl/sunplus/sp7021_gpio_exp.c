#include <mach/gpio_drv.h>
#include <dm.h>
#include <dt-bindings/pinctrl/sp7021.h>

#include "pinctrl_sunplus_sp7021.h"


int gpio_pin_mux_set(u32 func, u32 pin)
{
	u32 idx, bit_pos;

	if ((func > MUXF_GPIO_INT7) || (func < MUXF_L2SW_CLK_OUT)) {
		pctl_err("[%s] Invalid function: %d\n", __func__, func);
		return -EINVAL;
	}
	if (pin == 0) {
		// zero_func
		pin = 7;
	} else if ((pin < 8) || (pin > 71)) {
		pctl_err("[%s] Invalid G_MX%d\n", __func__, pin);
		return -EINVAL;
	}

	func -= MUXF_L2SW_CLK_OUT;
	idx = func >> 1;
	bit_pos = (func & 0x01)? 8: 0;
	GPIO_PINMUX(idx) = (0x7f << (16+bit_pos)) | ((pin-7) << bit_pos);

	return 0;
}
EXPORT_SYMBOL(gpio_pin_mux_set);

int gpio_pin_mux_get_val(u32 func, u32 *pin)
{
	u32 idx, bit_pos;

	if ((func > MUXF_GPIO_INT7) || (func < MUXF_L2SW_CLK_OUT)) {
		pctl_err("[%s] Invalid function: %d\n", __func__, func);
		return -EINVAL;
	}

	func -= MUXF_L2SW_CLK_OUT;
	idx = func >> 1;
	bit_pos = (func & 0x01)? 8: 0;
	*pin = (GPIO_PINMUX(idx) >> bit_pos) & 0x7f;

	return 0;
}
EXPORT_SYMBOL(gpio_pin_mux_get);

u32 gpio_pin_mux_get(u32 func)
{
	u32 pin = 0;

	gpio_pin_mux_get_val(func, &pin);

	return pin;
}
EXPORT_SYMBOL(gpio_pin_mux_get);

int gpio_input_invert_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_I_INV(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_input_invert_set);

int gpio_input_invert_get(u32 pin, u32 *gpio_input_invert_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_I_INV(idx);
	*gpio_input_invert_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_input_invert_get);

u32 gpio_input_invert_val_get(u32 pin)
{
	u32 value = 0;

	gpio_input_invert_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_input_invert_val_get);

int gpio_output_invert_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_O_INV(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_output_invert_set);

int gpio_output_invert_get(u32 pin, u32 *gpio_output_invert_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_O_INV(idx);
	*gpio_output_invert_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_output_invert_value_get);

u32 gpio_output_invert_val_get(u32 pin)
{
	u32 value = 0;

	gpio_output_invert_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_output_invert_val_get);

int gpio_open_drain_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_OD(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_open_drain_set);

int gpio_open_drain_get(u32 pin, u32 *gpio_open_drain_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_OD(idx);
	*gpio_open_drain_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_open_drain_get);

u32 gpio_open_drain_val_get(u32 pin)
{
	u32 value = 0;

	gpio_open_drain_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_open_drain_val_get);

int gpio_first_set(u32 pin, u32 val)
{
	u32 idx, value, reg_val;

	idx = pin >> 5;
	if (idx > 3) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x1f);
	reg_val = GPIO_FIRST(idx);
	if (val != 0) {
		GPIO_FIRST(idx) = reg_val | value;
	} else {
		GPIO_FIRST(idx) = reg_val & (~value);
	}

	return 0;
}
EXPORT_SYMBOL(gpio_first_set);

int gpio_first_get(u32 pin, u32 *gpio_first_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 5;
	if (idx > 3) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x1f);
	reg_val = GPIO_FIRST(idx);
	*gpio_first_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_first_get);

u32 gpio_first_val_get(u32 pin)
{
	u32 value = 0;

	gpio_first_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_first_val_get);

int gpio_master_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_MASTER(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_master_set);

int gpio_master_get(u32 pin, u32 *gpio_master_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_MASTER(idx);
	*gpio_master_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_master_get);

u32 gpio_master_val_get(u32 pin)
{
	u32 value = 0;

	gpio_master_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_master_val_get);

int gpio_oe_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_OE(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_oe_set);

int gpio_oe_get(u32 pin, u32 *gpio_out_enable_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_OE(idx);
	*gpio_out_enable_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_oe_get);

u32 gpio_oe_val_get(u32 pin)
{
	u32 value = 0;

	gpio_oe_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_oe_val_get);

int gpio_out_set(u32 pin, u32 val)
{
	u32 idx, bit;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	bit = pin & 0x0f;
	if (val != 0) {
		val = 0xffff;
	}
	GPIO_OUT(idx) = (1 << (bit+16)) | val;

	return 0;
}
EXPORT_SYMBOL(gpio_out_set);

int gpio_out_get(u32 pin, u32 *gpio_out_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 4;
	if (idx > 7) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_OUT(idx);
	*gpio_out_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_out_get);

u32 gpio_out_val_get(u32 pin)
{
	u32 value = 0;

	gpio_out_get(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_out_val_get);

int gpio_in(u32 pin, u32 *gpio_in_value)
{
	u32 idx, value, reg_val;

	idx = pin >> 5;
	if (idx > 5) {
		return -EINVAL;
	}

	value = 1 << (pin & 0x0f);
	reg_val = GPIO_IN(idx);
	*gpio_in_value = (reg_val & value)? 1: 0;

	return 0;
}
EXPORT_SYMBOL(gpio_in);

u32 gpio_in_val(u32 pin)
{
	u32 value = 0;

	gpio_in(pin, &value);

	return value;
}
EXPORT_SYMBOL(gpio_in_val);

u32 gpio_para_get(u32 pin)
{
	u32 value = 0;
	u32 value_tmp = 0;

	//F M I II O OI OE OD
	gpio_first_get(pin, &value);                    //First value
	value_tmp |= (value << 7);
	gpio_master_get(pin, &value);                   //Master value
	value_tmp |= (value << 6);
	gpio_in(pin, &value);                           //Input value
	value_tmp |= (value << 5);
	gpio_input_invert_get(pin, &value);             //Input invert value
	value_tmp |= (value << 4);
	gpio_out_get(pin,&value);                       //Output value
	value_tmp |= (value << 3);
	gpio_output_invert_get(pin, &value);            //Output invert value
	value_tmp |= (value << 2);
	gpio_oe_get(pin, &value);                       //Output Enable value
	value_tmp |= (value << 1);
	gpio_open_drain_get(pin, &value);               //Open Drain value
	value_tmp |= (value << 0);

	return value_tmp;
}
EXPORT_SYMBOL(gpio_para_get);

int gpio_debug_set(u32 pin, u32 val)
{
	if (pin < 72) {
		gpio_open_drain_set(pin,0);
		if (gpio_output_invert_val_get(pin) == 0) {
			if (val)
				gpio_out_set(pin,1);
			else
				gpio_out_set(pin,0);
		} else {
			if (val)
				gpio_out_set(pin,0);
			else
				gpio_out_set(pin,1);
		}
		gpio_oe_set(pin,1);

		gpio_first_set(pin,1);
		gpio_master_set(pin,1);
	} else {
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(gpio_debug_set);
