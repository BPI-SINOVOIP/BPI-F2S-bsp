
#include <mach/gpio_drv.h>
#include <dm.h>

#define REG_BASE           0x9c000000
#define RF_GRP(_grp, _reg) ((((_grp) * 32 + (_reg)) * 4) + REG_BASE)
#define RF_MASK_V(_mask, _val)       (((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask)         (((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask)         (((_mask) << 16) | 0)

struct moon2_regs {
	unsigned int sft_cfg[32];
};

struct moon3_regs {
	unsigned int sft_cfg[32];
};

struct g6_regs {
	unsigned int sft_cfg[32];
};

struct g7_regs {
	unsigned int sft_cfg[32];
};

struct g101_regs {
	unsigned int sft_cfg[32];
};

#define MOON2_REG ((volatile struct moon2_regs *)RF_GRP(2, 0))
#define MOON3_REG ((volatile struct moon3_regs *)RF_GRP(3, 0))

#define G6_REG ((volatile struct g6_regs *)RF_GRP(6, 0))
#define G7_REG ((volatile struct g7_regs *)RF_GRP(7, 0))
#define G101_REG ((volatile struct g101_regs *)RF_GRP(101, 0))

#define GPIO_FIRST(X)   (RF_GRP(101, (25+X)))
#define GPIO_MASTER(X)  (RF_GRP(6, (0+X)))
#define GPIO_OE(X)      (RF_GRP(6, (8+X)))
#define GPIO_OUT(X)     (RF_GRP(6, (16+X)))
#define GPIO_IN(X)      (RF_GRP(6, (24+X)))
#define GPIO_I_INV(X)   (RF_GRP(7, (0+X)))
#define GPIO_O_INV(X)   (RF_GRP(7, (8+X)))
#define GPIO_OD(X)      (RF_GRP(7, (16+X)))

int gpio_pin_mux_sel(PMXSEL_ID id, u32 sel)
{
	u32 grp ,idx, max_value, reg_val, mask, bit_num;
	
	grp = (id >> 24) & 0xff;
	if (grp > 0x03) {
		return -EINVAL;
	}	
	
	idx = (id >> 16) & 0xff;
	if (idx > 0x1f) {
		return -EINVAL;
	}
	
	max_value = (id >> 8) & 0xff;
	if (sel > max_value) {
		return -EINVAL;
	}
	
	bit_num = id & 0xff;
	
	if (max_value == 1) {
		mask = 0x01 << bit_num;
	}
	else if ((max_value == 2) || (max_value == 3)) {
		mask = 0x03 << bit_num;
	}
	else {
		mask = 0x7f << bit_num;
	}	

	reg_val = *((volatile unsigned int *)(RF_GRP(grp,idx)));
	reg_val |= mask << 0x10 ;
	reg_val &= (~mask);	
	reg_val = ((sel << bit_num) | (mask << 0x10));		
	*((volatile unsigned int *) (RF_GRP(grp,idx))) = reg_val;
	
	return 0;
}
EXPORT_SYMBOL(gpio_pin_mux_sel);

int gpio_pin_mux_get_val(PMXSEL_ID id, u32 *sel)
{
	u32 grp , idx, max_value, reg_val, mask, bit_num;
	
	grp = (id >> 24) & 0xff;
	
	idx = (id >> 16) & 0xff;
	if (idx > 0x11) {
		return -EINVAL;
	}

	max_value = (id >> 8) & 0xff;

	if (max_value > 0x40) {	
		return -EINVAL;
	}

	bit_num = id & 0xff;

	if (max_value == 1) {
		mask = 0x01 << bit_num;
	}
	else if ((max_value == 2) || (max_value == 3)) {
		mask = 0x03 << bit_num;
	}
	else {
		mask = 0x7f << bit_num;
	}

	reg_val = *((volatile unsigned int *)(RF_GRP(grp,idx)));
	reg_val &= mask;
	*sel = (reg_val >> bit_num);

	return 0;

}
EXPORT_SYMBOL(gpio_pin_mux_get);

u32 gpio_pin_mux_get(PMXSEL_ID id)
{
	u32 value = 0;

	gpio_pin_mux_get_val( id, &value);

	return value;
}
EXPORT_SYMBOL(gpio_pin_mux_get);



int gpio_input_invert_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) | 1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_I_INV(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_I_INV(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_input_invert_1);

int gpio_input_invert_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	reg_val = *((volatile unsigned int *)(GPIO_I_INV(idx)));
	value = (( reg_val | (1 << ((bit & 0x0f) + 0x10))) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_I_INV(idx))) = value;	

	return 0;
}
EXPORT_SYMBOL(gpio_input_invert_0);

int gpio_output_invert_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) | 1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_O_INV(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_O_INV(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_output_invert_1);

int gpio_input_invert_value_get(u32 bit, u32 *gpio_input_invert_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_I_INV(idx)));
	*gpio_input_invert_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_input_invert_value_get);

u32 gpio_input_invert_val_get(u32 bit)
{
	u32 value = 0;

	gpio_input_invert_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_input_invert_val_get);

int gpio_output_invert_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	reg_val = *((volatile unsigned int *)(GPIO_O_INV(idx)));
	value = (( reg_val | (1 << ((bit & 0x0f) + 0x10))) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_O_INV(idx))) = value;	

	return 0;
}
EXPORT_SYMBOL(gpio_output_invert_0);

int gpio_output_invert_value_get(u32 bit, u32 *gpio_output_invert_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_O_INV(idx)));
	*gpio_output_invert_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_output_invert_value_get);

u32 gpio_output_invert_val_get(u32 bit)
{
	u32 value = 0;

	gpio_output_invert_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_output_invert_val_get);

int gpio_open_drain_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) | 1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_OD(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_OD(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_open_drain_1);

int gpio_open_drain_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	reg_val = *((volatile unsigned int *)(GPIO_OD(idx)));
	value = (( reg_val | (1 << ((bit & 0x0f) + 0x10))) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_OD(idx))) = value;	

	return 0;
}
EXPORT_SYMBOL(gpio_open_drain_0);

int gpio_open_drain_value_get(u32 bit, u32 *gpio_open_drain_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_OD(idx)));
	*gpio_open_drain_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_open_drain_value_get);

u32 gpio_open_drain_val_get(u32 bit)
{
	u32 value = 0;

	gpio_open_drain_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_open_drain_val_get);

int gpio_first_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 5;
	if (idx > 4) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x1f);

	reg_val = *((volatile unsigned int *)(GPIO_FIRST(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_FIRST(idx))) = reg_val;	

	return 0;
}
EXPORT_SYMBOL(gpio_first_1);

int gpio_first_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 5;
	if (idx > 4) {
		return -EINVAL;
	}

	value = 1 << (bit & 0x1f);

	reg_val = *((volatile unsigned int *)(GPIO_FIRST(idx)));
	reg_val &= (~value);
	*((volatile unsigned int *) (GPIO_FIRST(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_first_0);

int gpio_first_value_get(u32 bit, u32 *gpio_first_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 5;
	if (idx > 5) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x1f);
	
	reg_val = *((volatile unsigned int *)(GPIO_FIRST(idx)));
	*gpio_first_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_first_value_get);

u32 gpio_first_val_get(u32 bit)
{
	u32 value = 0;

	gpio_first_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_first_val_get);

int gpio_master_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) | 1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_MASTER(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_MASTER(idx))) = reg_val;	

	return 0;
}
EXPORT_SYMBOL(gpio_master_1);

int gpio_master_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	reg_val = *((volatile unsigned int *)(GPIO_MASTER(idx)));
	value = ((reg_val | (1 << ((bit & 0x0f) + 0x10)) ) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_MASTER(idx))) = value;

	return 0;
}
EXPORT_SYMBOL(gpio_master_0);

int gpio_master_value_get(u32 bit, u32 *gpio_master_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_MASTER(idx)));
	*gpio_master_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_master_value_get);

u32 gpio_master_val_get(u32 bit)
{
	u32 value = 0;

	gpio_master_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_master_val_get);

int gpio_set_oe(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) |  1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_OE(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_OE(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_set_oe);

int gpio_clr_oe(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	reg_val = *((volatile unsigned int *)(GPIO_OE(idx)));
	value = ((reg_val | (1 << ((bit & 0x0f) + 0x10)) ) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_OE(idx))) = value;

	return 0;
}
EXPORT_SYMBOL(gpio_clr_oe);

int gpio_oe_value_get(u32 bit, u32 *gpio_out_enable_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_OE(idx)));
	*gpio_out_enable_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_oe_value_get);

u32 gpio_oe_val_get(u32 bit)
{
	u32 value = 0;

	gpio_oe_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_oe_val_get);

int gpio_out_1(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	value = (1 << (bit & 0x0f) | 1 << ((bit & 0x0f) + 0x10));

	reg_val = *((volatile unsigned int *)(GPIO_OUT(idx)));
	reg_val |= value;
	*((volatile unsigned int *) (GPIO_OUT(idx))) = reg_val;

	return 0;
}
EXPORT_SYMBOL(gpio_out_1);

int gpio_out_0(u32 bit)
{
	u32 idx, value, reg_val;

	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}

	reg_val = *((volatile unsigned int *)(GPIO_OUT(idx)));
	value = (( reg_val | (1 << ((bit & 0x0f) + 0x10)) ) & ~( 1 << (bit & 0x0f)));
	*((volatile unsigned int *) (GPIO_OUT(idx))) = value;

	return 0;
}
EXPORT_SYMBOL(gpio_out_0);

int gpio_out_value_get(u32 bit, u32 *gpio_out_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 4;
	if (idx > 8) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x0f);
	
	reg_val = *((volatile unsigned int *)(GPIO_OUT(idx)));
	*gpio_out_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_out_value_get);

u32 gpio_out_val_get(u32 bit)
{
	u32 value = 0;

	gpio_out_value_get(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_out_val_get);

int gpio_in(u32 bit, u32 *gpio_in_value)
{
	u32 idx, value, reg_val;
	
	idx = bit >> 5;
	if (idx > 5) {
		return -EINVAL;
	}
	
	value = 1 << (bit & 0x1f);
	
	reg_val = *((volatile unsigned int *)(GPIO_IN(idx)));
	*gpio_in_value =  (reg_val & value) ? 1 : 0;
		
	return 0;
}
EXPORT_SYMBOL(gpio_in);

u32 gpio_in_val(u32 bit)
{
	u32 value = 0;

	gpio_in(bit, &value);

	return value;
}
EXPORT_SYMBOL(gpio_in_val);

u32 gpio_para_get(u32 bit)
{
	u32 value = 0;
	u32 value_tmp = 0;

	//F M I II O OI OE OD
	gpio_first_value_get(bit, &value); //First value
	value_tmp |= (value << 7);
	gpio_master_value_get(bit, &value); //Master value
	value_tmp |= (value << 6);
	gpio_in(bit, &value); //Input value
	value_tmp |= (value << 5);
	gpio_input_invert_value_get(bit, &value); //Input invert value
	value_tmp |= (value << 4);
	gpio_out_value_get(bit,&value); //Output value
	value_tmp |= (value << 3);
	gpio_output_invert_value_get(bit, &value); //Output invert value
	value_tmp |= (value << 2);
	gpio_oe_value_get(bit, &value); //Output Enable value
	value_tmp |= (value << 1);
	gpio_open_drain_value_get(bit, &value); //Open Drain value
	value_tmp |= (value << 0);

	value = value_tmp;
	return value;
}
EXPORT_SYMBOL(gpio_para_get);


int gpio_debug_1(u32 bit)
{
	u32 value;

	if(bit < 72)
	{
		value = gpio_output_invert_val_get(bit);

		gpio_open_drain_0(bit);
		if(value == 0)
			gpio_out_1(bit);
		else if(value == 1)
			gpio_out_0(bit);
		else
			return -EINVAL;

		gpio_set_oe(bit);

		gpio_first_1(bit);
		gpio_master_1(bit);
	}
	else
	{
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(gpio_debug_1);

int gpio_debug_0(u32 bit)
{
	u32 value;

	if(bit < 72)
	{
		value = gpio_output_invert_val_get(bit);

		gpio_open_drain_0(bit);
		if(value == 0)
			gpio_out_0(bit);
		else if(value == 1)
			gpio_out_1(bit);
		else
			return -EINVAL;

		gpio_set_oe(bit);

		gpio_first_1(bit);
		gpio_master_1(bit);
	}
	else
	{
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(gpio_debug_0);

