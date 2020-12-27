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
#include <linux/reset.h>

static struct clk *uphy2_clk;
static struct clk *u3phy_clk;
static struct reset_control *uphy2_rst;
static struct reset_control *u3phy_rst;

//void __iomem *uphy2_res_moon0 = NULL;
static struct resource *uphy2_res_mem;
void __iomem *uphy2_base_addr = NULL;//temporary phy3 settings
void __iomem *uphy3_base_addr = NULL;//temporary phy3 settings

int uphy2_irq_num = -1;

u8 sp_port2_enabled = 0;
EXPORT_SYMBOL_GPL(sp_port2_enabled);

#if 0
extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);
#endif

static void sp_get_port2_state(void)
{
#ifdef CONFIG_USB_PORT2
	sp_port2_enabled |= PORT2_ENABLED;
#endif
}

static void uphy2_init(struct platform_device *pdev)
{
	u32 val;
			
	//phy2 settings
	/* 1. enable UPHY 2 & U3PHY HW CLOCK */
	//writel(RF_MASK_V_SET(1 << 15), uphy2_res_moon0 + CLK_REG_OFFSET);
	//writel(RF_MASK_V_SET(1 << 9), uphy2_res_moon0 + CLK_REG_OFFSET);
        
	/* 2. reset UPHY2 & U3HY*/
	//writel(RF_MASK_V_SET(1 << 15), uphy2_res_moon0 + USB_RESET_OFFSET);
	reset_control_assert(uphy2_rst);
	//writel(RF_MASK_V_CLR(1 << 15), uphy2_res_moon0 + USB_RESET_OFFSET);
	reset_control_deassert(uphy2_rst);
	mdelay(1); 
	//writel(RF_MASK_V_SET(1 << 9), uphy2_res_moon0 + USB_RESET_OFFSET);
	reset_control_assert(u3phy_rst);
	//writel(RF_MASK_V_CLR(1 << 9), uphy2_res_moon0 + USB_RESET_OFFSET);
	reset_control_deassert(u3phy_rst);
	mdelay(1);

	/* 3. Default value modification */
	writel(0x18888002, uphy2_base_addr + CTRL_OFFSET);

	/* 4. PLL power off/on twice */
	writel(0x88, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x80, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x88, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x80, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);
	mdelay(1);
	writel(0x00, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);

	/* 5. USB30C reset */
	//writel(RF_MASK_V_SET(1 << 12), uphy2_res_moon0 + USB_RESET_OFFSET);
	//writel(RF_MASK_V_CLR(1 << 12), uphy2_res_moon0 + USB_RESET_OFFSET);
	//mdelay(1);

	/* 6. HW workaround */
	val = readl(uphy2_base_addr + UPHY_INTR_OFFSET);
	val |= 0x0f;
	writel(val, uphy2_base_addr + UPHY_INTR_OFFSET);

	/* 7. USB DISC (disconnect voltage) */
	#if 0
	otp_v = otp_read_disc0(&pdev->dev, &otp_l, disc_name);
	set = *otp_v & OTP_DISC_LEVEL_BIT;
	if (set == 0)
		set = 0xD;
	#else
	writel(0x8b, uphy2_base_addr + DISC_LEVEL_OFFSET);
	#endif

	val = readl(uphy2_base_addr + ECO_PATH_OFFSET);
	val &= ~(ECO_PATH_SET);
	writel(val, uphy2_base_addr + ECO_PATH_OFFSET);

	val = readl(uphy2_base_addr + POWER_SAVING_OFFSET);
	val &= ~(POWER_SAVING_SET);
	writel(val, uphy2_base_addr + POWER_SAVING_OFFSET);

	val = readl(uphy2_base_addr + APHY_PROBE_OFFSET);
	val = (val & ~APHY_PROBE_CTRL_MASK) | APHY_PROBE_CTRL;
	writel(val, uphy2_base_addr + APHY_PROBE_OFFSET);

	/* 8. RX SQUELCH LEVEL */
	writel(0x04, uphy2_base_addr + SQ_CT_CTRL_OFFSET);

	/* 9. switch to host */
	//writel(RF_MASK_V_SET(3 << 12), uphy1_res_moon5 + USBC_CTL_OFFSET);

	//#ifdef CONFIG_USB_SUNPLUS_OTG
	//writel(RF_MASK_V_SET(1 << 13), uphy1_res_moon0 + PIN_MUX_CTRL);
	//writel(RF_MASK_V_CLR(1 << 12), uphy1_res_moon5 + USBC_CTL_OFFSET);
	//mdelay(1);
	//#endif
}

static int sunplus_usb_phy2_probe(struct platform_device *pdev)
{
	struct resource *res_mem;

#if 0
	s32 ret;
	u32 port_id = 1;

	usb_vbus_gpio[port_id] = of_get_named_gpio(pdev->dev.of_node, "vbus-gpio", 0);
	if ( !gpio_is_valid( usb_vbus_gpio[port_id]))
		printk(KERN_NOTICE "Wrong pin %d configured for vbus", usb_vbus_gpio[port_id]);
	ret = gpio_request( usb_vbus_gpio[port_id], "usb1-vbus");
	if ( ret < 0)
		printk(KERN_NOTICE "Can't get vbus pin %d", usb_vbus_gpio[port_id]);
	printk(KERN_NOTICE "%s,usb1_vbus_gpio_pin:%d\n",__FUNCTION__,usb_vbus_gpio[port_id]);
#endif
	/*enable uphy2 system clock*/
	uphy2_clk = devm_clk_get(&pdev->dev, "clkc_uphy2");
	if (IS_ERR(uphy2_clk)) {
		pr_err("not found clk source\n");
		return PTR_ERR(uphy2_clk);
	}
	clk_prepare(uphy2_clk);
	clk_enable(uphy2_clk);
        /*enable uphy3 system clock*/
        u3phy_clk = devm_clk_get(&pdev->dev, "clkc_u3phy");
	if (IS_ERR(u3phy_clk)) {
		pr_err("not found clk source\n");
		return PTR_ERR(u3phy_clk);
	}
	clk_prepare(u3phy_clk);
	clk_enable(u3phy_clk);
	
	uphy2_rst = devm_reset_control_get(&pdev->dev, "rstc_uphy2");
	if (IS_ERR(uphy2_rst)) {
		pr_err("not found uphy2 reset source\n");
		return PTR_ERR(uphy2_rst);
	}
	
	u3phy_rst = devm_reset_control_get(&pdev->dev, "rstc_u3phy");
	if (IS_ERR(u3phy_rst)) {
		pr_err("not found u3phy reset source\n");
		return PTR_ERR(u3phy_rst);
	}
	
	uphy2_irq_num = platform_get_irq(pdev, 0);
	if (uphy2_irq_num < 0) {
		printk(KERN_NOTICE "no irq provieded,ret:%d\n", uphy2_irq_num);
		return uphy2_irq_num;
	}
	printk(KERN_NOTICE "uphy2_irq:%d\n", uphy2_irq_num);
        
        uphy2_res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!uphy2_res_mem) {
		printk(KERN_NOTICE "no memory recourse provieded");
		return -ENXIO;
	}

	if (!request_mem_region(uphy2_res_mem->start, resource_size(uphy2_res_mem), "uphy0")) {
		printk(KERN_NOTICE "hw already in use");
		return -EBUSY;
	}

	uphy2_base_addr = ioremap_nocache(uphy2_res_mem->start, resource_size(uphy2_res_mem));
	if (!uphy2_base_addr){
		release_mem_region(uphy2_res_mem->start, resource_size(uphy2_res_mem));
		return -EFAULT;
	}
	//printk("phy2 base 0x%x, remap 0x%x\n", uphy2_res_mem->start, uphy2_base_addr);
	
	//res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	//uphy2_res_moon0 = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	//if (IS_ERR(uphy2_res_moon0)) {
	//	return PTR_ERR(uphy2_res_moon0);
	//}

    	//phy3 settings
    	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	uphy3_base_addr = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	if (IS_ERR(uphy3_base_addr)) {
		return PTR_ERR(uphy3_base_addr);
	}
	//printk("phy3 base 0x%x, remap 0x%x\n", res_mem->start, uphy3_base_addr);
			
	uphy2_init(pdev);

        writel(0x19, uphy2_base_addr + CDP_REG_OFFSET);
	writel(0x92, uphy2_base_addr + DCP_REG_OFFSET);
	writel(0x17, uphy2_base_addr + UPHY_INTER_SIGNAL_REG_OFFSET);

	// u3phy settings
        writel(0x43, uphy3_base_addr);
        writel(0x21, uphy3_base_addr + 0x2c);
        writel(0x5 , uphy3_base_addr + 0x34);
        writel(0x1f, uphy3_base_addr + 0x44);
        writel(0x0 , uphy3_base_addr + 0x48);        
        writel(0x33, uphy3_base_addr + 0x17c);
        writel(0x11, uphy3_base_addr + 0x1c0);
        writel(0x0 , uphy3_base_addr + 0x1c4);
        writel(0x1 , uphy3_base_addr + 0x1c8);
        writel(0x00, uphy3_base_addr + 0x1cc);
        writel(0x09, uphy3_base_addr + 0x200);
        //printk("*****EQ table\n");
        writel(0x00, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x1f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x01, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x1f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x02, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x4f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x03, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x4f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x04, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x7f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x05, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x7f, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x06, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xaf, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x07, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xaf, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x08, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xdf, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x09, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xdf, uphy3_base_addr + 0x138);
        writel(0x0e, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0a, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x0f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0b, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x0f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0c, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x3f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0d, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x3f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0e, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x6f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x0f, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x6f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x10, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x9f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x11, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0x9f, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x12, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xcf, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x13, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xcf, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x14, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xef, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x15, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xef, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x16, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xef, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x17, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xef, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x18, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xee, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x19, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xee, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1a, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xed, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1b, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xed, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1c, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xec, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1d, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xec, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1e, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xec, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x1f, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xec, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x20, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xeb, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x21, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xeb, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x22, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xea, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x23, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xea, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x24, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe9, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x25, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe9, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x26, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe9, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x27, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe9, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x28, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe8, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x29, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe8, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2a, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe7, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2b, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe7, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2c, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe6, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2d, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe6, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2e, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe6, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x2f, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe6, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x30, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe5, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x31, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe5, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x32, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe4, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x33, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe4, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x34, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe3, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x35, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe3, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x36, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe3, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x37, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe3, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x38, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe2, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x39, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x70, uphy3_base_addr + 0x134);
        writel(0xe2, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3a, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe1, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3b, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xb0, uphy3_base_addr + 0x134);
        writel(0xe1, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3c, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe0, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3d, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0xf0, uphy3_base_addr + 0x134);
        writel(0xe0, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3e, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe0, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        
        writel(0x00, uphy3_base_addr + 0x140);
        writel(0x3f, uphy3_base_addr + 0x12c);
        writel(0xbe, uphy3_base_addr + 0x130);
        writel(0x30, uphy3_base_addr + 0x134);
        writel(0xe0, uphy3_base_addr + 0x138);
        writel(0x0f, uphy3_base_addr + 0x13c);
        writel(0x01, uphy3_base_addr + 0x140);
        writel(0x00, uphy3_base_addr + 0x140);

	return 0;
}

static int sunplus_usb_phy2_remove(struct platform_device *pdev)
{
	u32 val;
#if 0
	u32 port_id = 1;
#endif
	val = readl(uphy2_base_addr + CDP_REG_OFFSET);
	val &= ~(1u << CDP_OFFSET);
	writel(val, uphy2_base_addr + CDP_REG_OFFSET);

	iounmap(uphy2_base_addr);
	release_mem_region(uphy2_res_mem->start, resource_size(uphy2_res_mem));

	/* pll power off*/

	val = readl(uphy2_base_addr + CDP_REG_OFFSET);
	val &= ~(1u << CDP_OFFSET);
	writel(val, uphy2_base_addr + CDP_REG_OFFSET);

	iounmap(uphy2_base_addr);
	//release_mem_region(uphy2_res_mem->start, resource_size(uphy2_res_mem));
	writel(0x88, uphy2_base_addr + PLL_PWR_CTRL_OFFSET);


	/*disable uphy2/3 system clock*/
	clk_disable(uphy2_clk);
	clk_disable(u3phy_clk);
#if 0
	gpio_free(usb_vbus_gpio[port_id]);
#endif
	return 0;
}

static const struct of_device_id phy2_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,i143-usb-phy2" },
	{ }
};

MODULE_DEVICE_TABLE(of, phy2_sunplus_dt_ids);

void phy2_otg_ctrl(void)
{
}
EXPORT_SYMBOL(phy2_otg_ctrl);

static struct platform_driver sunplus_usb_phy2_driver = {
	.probe		= sunplus_usb_phy2_probe,
	.remove		= sunplus_usb_phy2_remove,
	.driver		= {
		.name	= "sunplus-usb-phy2",
		.of_match_table = phy2_sunplus_dt_ids,
	},
};


static int __init usb_phy2_sunplus_init(void)
{
	sp_get_port2_state();

	if (sp_port2_enabled & PORT2_ENABLED) {
		printk(KERN_NOTICE "register sunplus_usb_phy2_driver\n");
		return platform_driver_register(&sunplus_usb_phy2_driver);	
	} else {
		printk(KERN_NOTICE "uphy2 not enabled\n");
		return 0;
	}
}
fs_initcall(usb_phy2_sunplus_init);

static void __exit usb_phy2_sunplus_exit(void)
{
	sp_get_port2_state();

	if (sp_port2_enabled & PORT2_ENABLED) {
		printk(KERN_NOTICE "unregister sunplus_usb_phy2_driver\n");
		platform_driver_unregister(&sunplus_usb_phy2_driver);
	} else {
		printk(KERN_NOTICE "uphy2 not enabled\n");
		return;
	}
}
module_exit(usb_phy2_sunplus_exit);

MODULE_ALIAS("sunplus_usb_phy2");
MODULE_LICENSE("GPL");
