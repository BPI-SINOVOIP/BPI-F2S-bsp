/*
 * SP7021 reset driver
 *
 * Copyright (C) 2015-2016 Sunplus Incorporated - http://www.sunplus.com/
 *	
 *	
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h> 
#include <linux/err.h> 
#include <linux/io.h> 
#include <linux/of.h> 
#include <linux/platform_device.h> 
#include <linux/reset-controller.h> 
#include <dt-bindings/reset/sp-q628.h>

#define BITASSERT(id, val)          ((1 << (16 + id)) | (val << id))


struct sp7021_reset_data { 
	struct reset_controller_dev	rcdev; 
	void __iomem			*membase; 
}; 


static inline struct sp7021_reset_data * 
to_sp7021_reset_data(struct reset_controller_dev *rcdev) 
{ 
	return container_of(rcdev, struct sp7021_reset_data, rcdev); 
} 


static int sp7021_reset_update(struct reset_controller_dev *rcdev, 
			      unsigned long id, bool assert) 
{ 
	struct sp7021_reset_data *data = to_sp7021_reset_data(rcdev); 
	int reg_width = sizeof(u32)/2; 
	int bank = id / (reg_width * BITS_PER_BYTE); 
	int offset = id % (reg_width * BITS_PER_BYTE); 
	void __iomem *addr; 

	addr = data->membase + (bank * 4); 

	if (assert) 
	   writel(BITASSERT(offset,1), addr);
	else    
	   writel(BITASSERT(offset,0), addr);

	return 0; 
} 


static int sp7021_reset_assert(struct reset_controller_dev *rcdev, 
			      unsigned long id) 
{ 
	return sp7021_reset_update(rcdev, id, true); 
} 


static int sp7021_reset_deassert(struct reset_controller_dev *rcdev, 
				unsigned long id) 
{ 
	return sp7021_reset_update(rcdev, id, false); 
} 


static int sp7021_reset_status(struct reset_controller_dev *rcdev, 
			      unsigned long id) 
{ 
	struct sp7021_reset_data *data = to_sp7021_reset_data(rcdev); 
	int reg_width = sizeof(u32)/2; 
	int bank = id / (reg_width * BITS_PER_BYTE); 
	int offset = id % (reg_width * BITS_PER_BYTE); 
	u32 reg; 

	reg = readl(data->membase + (bank * 4)); 

	return !!(reg & BIT(offset)); 
} 


static const struct reset_control_ops sp7021_reset_ops = { 
	.assert		= sp7021_reset_assert, 
	.deassert	= sp7021_reset_deassert, 
	.status		= sp7021_reset_status, 
}; 


static const struct of_device_id sp7021_reset_dt_ids[] = { 
	{ .compatible = "sunplus,sp7021-reset", },
	{ /* sentinel */ }, 
}; 


static int sp7021_reset_probe(struct platform_device *pdev) 
{ 
	struct device *dev = &pdev->dev; 
	struct sp7021_reset_data *data; 
	void __iomem *membase; 
	struct resource *res; 

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL); 
	if (!data) 
		return -ENOMEM; 

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0); 
	//printk(KERN_INFO "res->start : 0x%x \n", res->start);
	membase = devm_ioremap(dev, res->start, resource_size(res)); 
	//printk(KERN_INFO "membase : 0x%x \n", membase);
	if (IS_ERR(membase)) 
		return PTR_ERR(membase); 

	data->membase = membase; 
	data->rcdev.owner = THIS_MODULE; 
 	//data->rcdev.nr_resets = resource_size(res) * BITS_PER_BYTE;
	//printk(KERN_INFO "data->rcdev.nr_resets : 0x%x \n", data->rcdev.nr_resets);
	//printk(KERN_INFO "resource_size(res) : 0x%x \n", resource_size(res));
 	//RST_MAX get from dt-bindings/reset/sp-q628.h 
 	data->rcdev.nr_resets = RST_MAX; 
 	data->rcdev.ops = &sp7021_reset_ops; 
 	data->rcdev.of_node = dev->of_node; 
 
 	return devm_reset_controller_register(dev, &data->rcdev); 
} 
 

static struct platform_driver sp7021_reset_driver = { 
	.probe	= sp7021_reset_probe, 
	.driver = { 
		.name = "sp7021-reset",
		.of_match_table	= sp7021_reset_dt_ids, 
	}, 
}; 
 

//builtin_platform_driver(stm32_reset_driver); 
static int __init sp7021_reset_init(void)
{
	return platform_driver_register(&sp7021_reset_driver);
}
arch_initcall(sp7021_reset_init);


MODULE_AUTHOR("Edwin Chiu <edwin.chiu@sunplus.com>");
MODULE_DESCRIPTION("SUNPLUS SP7021 Reset Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sp7021-reset");
