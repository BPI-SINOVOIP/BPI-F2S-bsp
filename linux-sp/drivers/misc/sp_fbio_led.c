#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/delay.h>

//#define TRACE(s) printk("### %s:%d %s\n", __FUNCTION__, __LINE__, s)
#define TRACE(s)

struct sp_fbio_ctl_reg {
	u32 ch0_addr; // 0x9c10_5000
	u32 ch0_ctrl; // 0x9c10_5004
	u32 ch1_addr; // 0x9c10_5008
	u32 ch1_ctrl; // 0x9c10_500c
	u32 ch2_addr; // 0x9c10_5010
	u32 ch2_ctrl; // 0x9c10_5014
	u32 ch3_addr; // 0x9c10_5018
	u32 ch3_ctrl; // 0x9c10_501c
	u32 ch4_addr; // 0x9c10_5020
	u32 ch4_ctrl; // 0x9c10_5024
	u32 ch5_addr; // 0x9c10_5028
	u32 ch5_ctrl; // 0x9c10_502c
	u32 io_ctrl ; // 0x9c10_5030
	u32 io_tctl ; // 0x9c10_5034
	u32 macro_c0; // 0x9c10_5038
	u32 macro_c1; // 0x9c10_503c
	u32 macro_c2; // 0x9c10_5040
	u32 io_tpctl; // 0x9c10_5044
	u32 io_rpctl; // 0x9c10_5048
	u32 boot_ra ; // 0x9c10_504c
	u32 io_tdat0; // 0x9c10_5050
	u32 reserved[11];
};

struct sp_fbio_led_reg {
	/*
	u32 disp_sel:1;		// led display on/off
	u32 cnt_start:1;	// timer start
	u32 int_en:1;		// timer interrupt enable
	u32 ovfl_cls:1;		// clear timer (counter overflow) flag enable
	*/
	u32 ex_con;			// config/control

	u32 ex_to;			// timer interval (25MHz???)
	u32 ex_buffer;		// led display data buffer

	/*
	u32 ovfl_sta:1;		// counter overflow flag
	*/
	u32 ex_state;		// state
};

struct sp_fbio_led_dev {
	volatile struct sp_fbio_ctl_reg *ctl;
	volatile struct sp_fbio_led_reg *reg;
	int irq;
	struct clk *clk_fio;
	struct clk *clk_fpga;
	struct reset_control *rstc;
	struct device *dev;
};

static struct sp_fbio_led_dev sp_fbio_led;

static irqreturn_t sp_fbio_led_isr(int irq, void *dev_id)
{
	sp_fbio_led.reg->ex_buffer++;

	return IRQ_HANDLED;
}

static int sp_fbio_led_setcfg(int ex_con, int ex_to, int ex_buffer)
{
	volatile struct sp_fbio_led_reg *reg = sp_fbio_led.reg;

	if (ex_con != -1)
		reg->ex_con = ex_con;
	if (ex_to != -1)
		reg->ex_to = ex_to;
	if (ex_buffer != -1)
		reg->ex_buffer = ex_buffer;

	return 0;
}

/* echo ex_con ex_to ex_buffer > /sys/module/sp_fbio_led/parameters/test */
static int test_set(const char *val, const struct kernel_param *kp)
{
	int ex_con, ex_to, ex_buffer;

	ex_con = ex_to = ex_buffer = -1;
	sscanf(val, "%d %d %d",	&ex_con, &ex_to, &ex_buffer);
	sp_fbio_led_setcfg(ex_con, ex_to, ex_buffer);

	return 0;
}

static const struct kernel_param_ops test_ops = {
	.set = test_set,
};
module_param_cb(test, &test_ops, NULL, S_IWUSR | S_IRUGO);

static const struct of_device_id sp_fbio_led_of_match[] = {
	{ .compatible = "sunplus,sp7021-fbio-led" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_fbio_led_of_match);

static int sp_fbio_led_probe(struct platform_device *pdev)
{
	struct sp_fbio_led_dev *dev = &sp_fbio_led;
	struct resource *res_mem, *res_irq;
	int ret = 0;

	TRACE("clocks");
	dev->clk_fio = devm_clk_get(&pdev->dev, "fio");
	if (IS_ERR(dev->clk_fio)) {
		dev_err(&pdev->dev, "not found clk_fio source\n");
		return PTR_ERR(dev->clk_fio);
	}
	ret = clk_prepare_enable(dev->clk_fio);
	if(ret)
		return ret;

	dev->clk_fpga = devm_clk_get(&pdev->dev, "fpga");
	if (IS_ERR(dev->clk_fpga)) {
		dev_err(&pdev->dev, "not found clk_fpga source\n");
		return PTR_ERR(dev->clk_fpga);
	}
	ret = clk_prepare_enable(dev->clk_fpga);
	if(ret)
		return ret;
	
	TRACE("resets");
	dev->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(dev->rstc)) {
		dev_err(&pdev->dev, "not found rstc_fpga source\n");
		return PTR_ERR(dev->rstc);
	}
	ret = reset_control_deassert(dev->rstc);
	if(ret)
		return ret;

	TRACE("regs");
	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem)
		return -ENODEV;
	dev->ctl = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR((void *)dev->ctl))
		return PTR_ERR((void *)dev->ctl);

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res_mem)
		return -ENODEV;
	dev->reg = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR((void *)dev->reg))
		return PTR_ERR((void *)dev->reg);

	TRACE("irq");
	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res_irq) {
		return -ENODEV;
	}

	dev->irq = res_irq->start;
	ret = devm_request_irq(&pdev->dev, dev->irq, sp_fbio_led_isr, IRQF_TRIGGER_RISING, "sp_fbio_led", dev);
	if (ret) {
		return -ENODEV;
	}
	
	TRACE("init fbio_ctl");
	dev->ctl->io_ctrl  = 0x3;		// enable 16bit DDR mode
	dev->ctl->io_tpctl = 0x10f0f;	// set tx clock delay
	dev->ctl->io_rpctl = 0x10f0f;	// set rx clock delay

	TRACE("done");
	platform_set_drvdata(pdev, dev);
	dev->dev = &pdev->dev;

	return ret;
}

static struct platform_driver sp_fbio_led_driver = {
	.probe		= sp_fbio_led_probe,
	.driver		= {
		.name	= "sp_fbio_led",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_fbio_led_of_match),
	},
};

module_platform_driver(sp_fbio_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus FBIO LED driver");
