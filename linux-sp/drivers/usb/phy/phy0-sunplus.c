#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/usb/phy.h>
#include <linux/usb/sp_usb.h>
#include <linux/nvmem-consumer.h>


static struct clk *uphy0_clk;
static struct resource *uphy0_res_mem;
void __iomem *uphy0_base_addr = NULL;
void __iomem *uphy0_res_moon0 = NULL;
void __iomem *uphy0_res_moon4 = NULL;
#ifdef CONFIG_SOC_I143
void __iomem *uphy0_res_moon5 = NULL;
#endif
int uphy0_irq_num = -1;
u8 sp_port0_enabled = 0;
EXPORT_SYMBOL_GPL(sp_port0_enabled);

#if 0
extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);
#endif

static void sp_get_port0_state(void)
{
#ifdef CONFIG_USB_PORT0
	sp_port0_enabled |= PORT0_ENABLED;
#endif
}

#ifdef CONFIG_SOC_SP7021
char *otp_read_disc0(struct device *_d, ssize_t *_l, char *_name)
{
	char *ret = NULL;
	struct nvmem_cell *c = nvmem_cell_get(_d, _name);

	if (IS_ERR_OR_NULL(c)) {
		dev_err(_d, "OTP %s read failure: %ld", _name, PTR_ERR(c));
		return (NULL);
	}

	ret = nvmem_cell_read(c, _l);
	nvmem_cell_put(c);
	dev_dbg(_d, "%d bytes read from OTP %s", *_l, _name);

	return (ret);
}
#endif

static void uphy0_init(struct platform_device *pdev)
{
	u32 val;
#ifdef CONFIG_SOC_SP7021
	u32 set;
	void __iomem *usb_otp_reg;
	char *disc_name = "disc_vol0";
	ssize_t otp_l = 0;
	char *otp_v;

	usb_otp_reg = ioremap_nocache(USB_OTP_REG, 1);

	/* 1. Default value modification */
	writel(RF_MASK_V(0xffff, 0x4002), uphy0_res_moon4 + UPHY0_CTL0_OFFSET);
	writel(RF_MASK_V(0xffff, 0x8747), uphy0_res_moon4 + UPHY0_CTL1_OFFSET);

	/* 2. PLL power off/on twice */
	writel(RF_MASK_V(0xffff, 0x88), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
	mdelay(1);
	writel(RF_MASK_V(0xffff, 0x80), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
	mdelay(1);
	writel(RF_MASK_V(0xffff, 0x88), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
	mdelay(1);
	writel(RF_MASK_V(0xffff, 0x80), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
	mdelay(1);
	writel(RF_MASK_V(0xffff, 0x00), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);

	/* 3. reset UPHY0 */
	writel(RF_MASK_V_SET(1 << 13), uphy0_res_moon0 + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << 13), uphy0_res_moon0 + USB_RESET_OFFSET);
	mdelay(1);

	/* 4.b board uphy 0 internal register modification for tid certification */
	otp_v = otp_read_disc0(&pdev->dev, &otp_l, disc_name);
	set = *otp_v & OTP_DISC_LEVEL_BIT;
	if (set == 0)
		set = 0xD;

	val = readl(uphy0_base_addr + DISC_LEVEL_OFFSET);
	val = (val & ~OTP_DISC_LEVEL_BIT) | set;
	writel(val, uphy0_base_addr + DISC_LEVEL_OFFSET);

	val = readl(uphy0_base_addr + ECO_PATH_OFFSET);
	val &= ~(ECO_PATH_SET);
	writel(val, uphy0_base_addr + ECO_PATH_OFFSET);

	val = readl(uphy0_base_addr + POWER_SAVING_OFFSET);
	val &= ~(POWER_SAVING_SET);
	writel(val, uphy0_base_addr + POWER_SAVING_OFFSET);

	val = readl(uphy0_base_addr + APHY_PROBE_OFFSET);
	val = (val & ~APHY_PROBE_CTRL_MASK) | APHY_PROBE_CTRL;
	writel(val, uphy0_base_addr + APHY_PROBE_OFFSET);

	/* 5. USBC 0 reset */
	writel(RF_MASK_V_SET(1 << 10), uphy0_res_moon0 + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << 10), uphy0_res_moon0 + USB_RESET_OFFSET);

	/* port 0 uphy clk fix */
	writel(RF_MASK_V_SET(1 << 6), uphy0_res_moon4 + UPHY0_CTL2_OFFSET);

	/* 6. switch to host */
	writel(RF_MASK_V_SET(3 << 4), uphy0_res_moon4 + USBC_CTL_OFFSET);

	#ifdef CONFIG_USB_SUNPLUS_OTG
	writel(RF_MASK_V_SET(1 << 2), uphy0_res_moon0 + PIN_MUX_CTRL);
	writel(RF_MASK_V_CLR(1 << 4), uphy0_res_moon4 + USBC_CTL_OFFSET);
	mdelay(1);
	#endif

	/* 7. AC & ACB */
	writel(RF_MASK_V_SET(1 << 11), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
	writel(RF_MASK_V_SET(1 << 14), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);

	iounmap(usb_otp_reg);
#elif defined(CONFIG_SOC_I143)
	/* 1. enable UPHY 0 & USBC 0 HW CLOCK */
	writel(RF_MASK_V_SET(1 << 13), uphy0_res_moon0 + CLK_REG_OFFSET);
	writel(RF_MASK_V_SET(1 << 10), uphy0_res_moon0 + CLK_REG_OFFSET);

	/* 2. reset UPHY0 */
	writel(RF_MASK_V_SET(1 << 13), uphy0_res_moon0 + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << 13), uphy0_res_moon0 + USB_RESET_OFFSET);
	mdelay(1);

	/* 3. Default value modification */
	writel(0x18888002, uphy0_base_addr + CTRL_OFFSET);

	/* 4. PLL power off/on twice */
	writel(0x88, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x80, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x88, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x80, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x00, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);

	/* 5. USBC 0 reset */
	writel(RF_MASK_V_SET(1 << 10), uphy0_res_moon0 + USB_RESET_OFFSET);
	writel(RF_MASK_V_CLR(1 << 10), uphy0_res_moon0 + USB_RESET_OFFSET);
	mdelay(1);

	/* 6. HW workaround */
	val = readl(uphy0_base_addr + UPHY_INTR_OFFSET);
	val |= 0x0f;
	writel(val, uphy0_base_addr + UPHY_INTR_OFFSET);

	/* 7. USB DISC (disconnect voltage) */
	#if 0
	otp_v = otp_read_disc0(&pdev->dev, &otp_l, disc_name);
	set = *otp_v & OTP_DISC_LEVEL_BIT;
	if (set == 0)
		set = 0xD;

	val = readl(uphy0_base_addr + DISC_LEVEL_OFFSET);
	val = (val & ~OTP_DISC_LEVEL_BIT) | set;
	writel(val, uphy0_base_addr + DISC_LEVEL_OFFSET);
	#else
	writel(0x8b, uphy0_base_addr + DISC_LEVEL_OFFSET);
	#endif

	val = readl(uphy0_base_addr + ECO_PATH_OFFSET);
	val &= ~(ECO_PATH_SET);
	writel(val, uphy0_base_addr + ECO_PATH_OFFSET);

	val = readl(uphy0_base_addr + POWER_SAVING_OFFSET);
	val &= ~(POWER_SAVING_SET);
	writel(val, uphy0_base_addr + POWER_SAVING_OFFSET);

	val = readl(uphy0_base_addr + APHY_PROBE_OFFSET);
	val = (val & ~APHY_PROBE_CTRL_MASK) | APHY_PROBE_CTRL;
	writel(val, uphy0_base_addr + APHY_PROBE_OFFSET);

	/* 8. RX SQUELCH LEVEL */
	writel(0x04, uphy0_base_addr + SQ_CT_CTRL_OFFSET);

	/* 9. switch to host */
	writel(RF_MASK_V_SET(3 << 4), uphy0_res_moon5 + USBC_CTL_OFFSET);

	#ifdef CONFIG_USB_SUNPLUS_OTG
	writel(RF_MASK_V_SET(1 << 12), uphy0_res_moon0 + PIN_MUX_CTRL);
	writel(RF_MASK_V_CLR(1 << 4), uphy0_res_moon5 + USBC_CTL_OFFSET);
	mdelay(1);
	#endif
#endif
}

static int sunplus_usb_phy0_probe(struct platform_device *pdev)
{
	struct resource *res_mem;

#if 0
	s32 ret;
	u32 port_id = 0;

	usb_vbus_gpio[port_id] = of_get_named_gpio(pdev->dev.of_node, "vbus-gpio", 0);
	if ( !gpio_is_valid( usb_vbus_gpio[port_id]))
		printk(KERN_NOTICE "Wrong pin %d configured for vbus", usb_vbus_gpio[port_id]);
	ret = gpio_request( usb_vbus_gpio[port_id], "usb0-vbus");
	if ( ret < 0)
		printk(KERN_NOTICE "Can't get vbus pin %d", usb_vbus_gpio[port_id]);
	printk(KERN_NOTICE "%s,usb0_vbus_gpio_pin:%d\n",__FUNCTION__,usb_vbus_gpio[port_id]);
#endif

	/*enable uphy0 system clock*/
	uphy0_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(uphy0_clk)) {
		pr_err("not found clk source\n");
		return PTR_ERR(uphy0_clk);
	}
	clk_prepare(uphy0_clk);
	clk_enable(uphy0_clk);

	uphy0_irq_num = platform_get_irq(pdev, 0);
	if (uphy0_irq_num < 0) {
		printk(KERN_NOTICE "no irq provieded,ret:%d\n",uphy0_irq_num);
		return uphy0_irq_num;
	}
	printk(KERN_NOTICE "uphy0_irq:%d\n",uphy0_irq_num);

	uphy0_res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!uphy0_res_mem) {
		printk(KERN_NOTICE "no memory recourse provieded");
		return -ENXIO;
	}

	if (!request_mem_region(uphy0_res_mem->start, resource_size(uphy0_res_mem), "uphy0")) {
		printk(KERN_NOTICE "hw already in use");
		return -EBUSY;
	}

	uphy0_base_addr = ioremap_nocache(uphy0_res_mem->start, resource_size(uphy0_res_mem));
	if (!uphy0_base_addr){
		release_mem_region(uphy0_res_mem->start, resource_size(uphy0_res_mem));
		return -EFAULT;
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	uphy0_res_moon0 = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	if (IS_ERR(uphy0_res_moon0)) {
		return PTR_ERR(uphy0_res_moon0);
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	uphy0_res_moon4 = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	if (IS_ERR(uphy0_res_moon4)) {
		return PTR_ERR(uphy0_res_moon4);
	}

#ifdef CONFIG_SOC_I143
	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	uphy0_res_moon5 = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	if (IS_ERR(uphy0_res_moon5)) {
		return PTR_ERR(uphy0_res_moon5);
	}
#endif

	uphy0_init(pdev);

	writel(0x19, uphy0_base_addr + CDP_REG_OFFSET);
	writel(0x92, uphy0_base_addr + DCP_REG_OFFSET);
	writel(0x17, uphy0_base_addr + UPHY_INTER_SIGNAL_REG_OFFSET);

	return 0;
}

static int sunplus_usb_phy0_remove(struct platform_device *pdev)
{
	u32 val;
#if 0
	u32 port_id = 0;
#endif

	val = readl(uphy0_base_addr + CDP_REG_OFFSET);
	val &= ~(1u << CDP_OFFSET);
	writel(val, uphy0_base_addr + CDP_REG_OFFSET);

	iounmap(uphy0_base_addr);
	release_mem_region(uphy0_res_mem->start, resource_size(uphy0_res_mem));

	/* pll power off */
#ifdef CONFIG_SOC_SP7021
	writel(RF_MASK_V(0xffff, 0x88), uphy0_res_moon4 + UPHY0_CTL3_OFFSET);
#elif defined(CONFIG_SOC_I143)
	writel(0x88, uphy0_base_addr + PLL_PWR_CTRL_OFFSET);
#endif

	/*disable uphy0 system clock*/
	clk_disable(uphy0_clk);
#if 0
	gpio_free(usb_vbus_gpio[port_id]);
#endif

	return 0;
}

static const struct of_device_id phy0_sunplus_dt_ids[] = {
#ifdef CONFIG_SOC_SP7021
	{ .compatible = "sunplus,sp7021-usb-phy0" },
#elif defined(CONFIG_SOC_I143)
	{ .compatible = "sunplus,i143-usb-phy0" },
#endif
	{ }
};
MODULE_DEVICE_TABLE(of, phy0_sunplus_dt_ids);

void phy0_otg_ctrl(void)
{
#ifdef CONFIG_SOC_SP7021
	writel(RF_MASK_V_SET(1 << 8), uphy0_res_moon4 + UPHY0_CTL0_OFFSET);
#elif defined(CONFIG_SOC_I143)
	/* TBD */
#endif
}
EXPORT_SYMBOL(phy0_otg_ctrl);


static struct platform_driver sunplus_usb_phy0_driver = {
	.probe		= sunplus_usb_phy0_probe,
	.remove		= sunplus_usb_phy0_remove,
	.driver		= {
		.name	= "sunplus-usb-phy0",
		.of_match_table = phy0_sunplus_dt_ids,
	},
};


static int __init usb_phy0_sunplus_init(void)
{
	sp_get_port0_state();

	if (sp_port0_enabled & PORT0_ENABLED) {
		printk(KERN_NOTICE "register sunplus_usb_phy0_driver\n");
		return platform_driver_register(&sunplus_usb_phy0_driver);	
	} else {
		printk(KERN_NOTICE "uphy0 not enabled\n");
		return 0;
	}
}
fs_initcall(usb_phy0_sunplus_init);

static void __exit usb_phy0_sunplus_exit(void)
{
	if (sp_port0_enabled & PORT0_ENABLED) {
		printk(KERN_NOTICE "unregister sunplus_usb_phy0_driver\n");
		platform_driver_unregister(&sunplus_usb_phy0_driver);
	} else {
		printk(KERN_NOTICE "uphy0 not enabled\n");
		return;
	}
}
module_exit(usb_phy0_sunplus_exit);


MODULE_ALIAS("sunplus_usb_phy0");
MODULE_LICENSE("GPL");
