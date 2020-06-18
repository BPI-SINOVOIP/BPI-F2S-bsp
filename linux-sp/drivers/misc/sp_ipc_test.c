#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/smp.h>

struct sp_ipc_test_dev {
	volatile u32 *reg;
};

static struct sp_ipc_test_dev sp_ipc_test;

static irqreturn_t sp_ipc_test_isr(int irq, void *dev_id)
{
	printk("!!!%d@CPU%d!!!\n", irq, smp_processor_id());
	return IRQ_HANDLED;
}

static void test_help(void)
{
	printk(
		"sp_ipc test:\n"
		"  echo <reg:0~63> [value] > /sys/module/sp_ipc_test/parameters/test\n"
		"  regs:\n"
		"     0~31: G258 A2B\n"
		"    32~63: G259 B2A\n"
	);
}

static int test_set(const char *val, const struct kernel_param *kp)
{
	int i;
	u32 value;

	switch (sscanf(val, "%d %d", &i, &value)) {
	case 0:
		test_help();
		break;
	case 1:
		printk("%08x\n", sp_ipc_test.reg[i]);
		break;
	case 2:
		sp_ipc_test.reg[i] = value;
		break;
	}

	return 0;
}

static const struct kernel_param_ops test_ops = {
	.set = test_set,
};
module_param_cb(test, &test_ops, NULL, 0600);

static const struct of_device_id sp_ipc_test_of_match[] = {
	{ .compatible = "sunplus,sp-ipc-test" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_ipc_test_of_match);

static int sp_ipc_test_probe(struct platform_device *pdev)
{
	struct sp_ipc_test_dev *dev = &sp_ipc_test;
	struct resource *res_mem, *res_irq;
	int i, ret = 0;

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem)
		return -ENODEV;

	dev->reg = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR((void *)dev->reg))
		return PTR_ERR((void *)dev->reg);

	for (i = 0; i < 18; i++) {
		res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res_irq) {
			return -ENODEV;
		}
		ret = devm_request_irq(&pdev->dev, res_irq->start, sp_ipc_test_isr, 0, "sp_ipc_test", dev);
		//printk("%s:%d %d\n", __FUNCTION__, __LINE__, ret);
	}

	dev->reg[0] = 1; // trigger A2B INT
	dev->reg[31] = 0x12345678; // trigger A2B DIRECT_INT
	dev->reg[32] = 1; // trigger B2A INT
	dev->reg[63] = 0x87654321; // trigger B2A DIRECT_INT

	return ret;
}

static struct platform_driver sp_ipc_test_driver = {
	.probe		= sp_ipc_test_probe,
	.driver		= {
		.name	= "sp_ipc_test",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_ipc_test_of_match),
	},
};

module_platform_driver(sp_ipc_test_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus ipc_test driver");
