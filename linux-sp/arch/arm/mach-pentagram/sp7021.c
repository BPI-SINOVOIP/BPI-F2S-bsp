#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <asm/memory.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/io_map.h>
#include <mach/clk.h>
#include <mach/misc.h>
#include <dt-bindings/memory/sp-q628-mem.h> 
#include "common.h"

static void sp_power_off(void)
{
	unsigned int reg_value;
	void __iomem *regs = (void __iomem *)A_SYSTEM_BASE;
	void __iomem *regs_B = (void __iomem *)B_SYSTEM_BASE;
//	int i;
	pr_info("%s\n", __func__);

	writel(0x0000, regs_B + 0x434); /* iop_data5=0x0000 */
	writel(0x0060, regs_B + 0x438); /* iop_data6=0x0060 */
	writel(0x00dd, regs_B + 0x424); /* iop_data1=0x00dd */
		
	printk("PD RG_PLL_PDN and RG_PLLIO_PDN to save power\n");
	writel(0, regs + 0x54); /* bit0 RG_PLLIO_PDN */
	writel(0, regs + 0x2C); /* bit0 RG_PLL_PDN */

//	printk("PD Achip mo_gclk_en0/mo_clk_en0 to save power \n");
//	writel(0, regs + 0x28); 
//	writel(0, regs + 0x24); 
//	
//	printk("disable Q628 clk and enable reset to save power\n");
//	for (i = 0; i < 20; i++) {
//		writel(0xffff0000 , (void __iomem *)(B_SYSTEM_BASE + 4 * (1 + i)));
//	}
//	for (i = 0; i < 10; i++) {
//		writel(0xffffffff , (void __iomem *)(B_SYSTEM_BASE + 4 * (21 + i)));
//	}
}

static unsigned int b_pllsys_get_rate(void)
{
	unsigned int reg = readl((void __iomem *)B_SYSTEM_BASE + 0x268); /* G4.26 */
	unsigned int reg2 = readl((void __iomem *)B_SYSTEM_BASE + 0x26c); /* G4.27 */

	if ((reg >> 9) & 1) /* bypass? */
		return 27000000;
	return (((reg & 0xf) + 1) * 13500000) >> ((reg2 >> 4) & 0xf);
}

#ifdef CONFIG_SP_PARTIAL_CLKEN
/* power saving, provided by yuwen + CARD_CTL4 */
static void apply_partial_clken(void)
{
	int i;
	const int ps_clken[] = {
		0x67ef, 0x41ff, 0xff03, 0xfff0, 0x0004, /* G0.1~5  */
		0x0000, 0x8000, 0xffff, 0x0040, 0x0000, /* G0.6~10 */
	};

	printk("apply partial clken to save power\n");

	for (i = 0; i < sizeof(ps_clken) / 4; i++) {
		writel(0xffff0000 | ps_clken[i], (void __iomem *)(B_SYSTEM_BASE + 4 * (1 + i)));
	}
}
#endif

static struct platform_device sp7021_cpuidle = {
	.name              = "sp7021_cpuidle",
	.dev.platform_data = sp7021_enter_aftr,
	.id                = -1,
};

static void __init sp_init(void)
{
	unsigned int b_sysclk, io_ctrl;
#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
	unsigned int a_pllclk, coreclk, ioclk, sysclk, clk_cfg, a_pllioclk;
#endif

	early_printk("%s\n", __func__);

	sp_prn_uptime();

	pm_power_off = sp_power_off;

	io_ctrl = readl((void __iomem *)B_SYSTEM_BASE + 0x105030);

	b_sysclk = b_pllsys_get_rate();

	early_printk("B: b_sysclk=%uM abio_ctrl=(%ubit, %s)\n", b_sysclk / 1000000,
		(io_ctrl & 2) ? 16 : 8, (io_ctrl & 1) ? "DDR" : "SDR");

#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
	clk_cfg = readl((void __iomem *)A_SYSTEM_BASE + 0xc);
	a_pllclk = (((readl((void __iomem *)A_SYSTEM_BASE + 0x2c) >> 16) + 1) & 0xff) * (27 * 1000 * 1000);
	coreclk = a_pllclk / (1 + ((clk_cfg >> 10) & 1));
	sysclk = coreclk / (1 + ((clk_cfg >> 3) & 1));
	a_pllioclk = (((readl((void __iomem *)A_SYSTEM_BASE + 0x54) >> 16) & 0xff) + 1) * (27 * 1000 * 1000);
	ioclk = a_pllioclk / (20 + 5 * ((clk_cfg >> 4) & 7)) / ((clk_cfg >> 16) & 0xff) * 10;
	early_printk("A: core=%uM a_sysclk=%uM a_pllio=%uM abio_bus=%uM\n",
		coreclk / 1000000, sysclk / 1000000, a_pllioclk / 1000000, ioclk / 1000000);

#endif

#ifdef CONFIG_SP_PARTIAL_CLKEN
	apply_partial_clken();
#endif

		sp7021_cpuidle.dev.platform_data = &cpuidle_coupled_sp7021_data;
		platform_device_register(&sp7021_cpuidle);
}

static struct map_desc sp_io_desc[] __initdata = {
	{	/* B RGST Bus */
		.virtual = VA_B_REG,
		.pfn     = __phys_to_pfn(PA_B_REG),
		.length  = SIZE_B_REG,
		.type    = MT_DEVICE
	},
	{
		/* B SRAM0 */
		.virtual = VA_B_SRAM0,
		.pfn     = __phys_to_pfn(PA_B_SRAM0),
		.length  = SIZE_B_SRAM0,
		.type    = MT_DEVICE
	},
#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
	{	/* A RGST Bus */
		.virtual = VA_A_REG,
		.pfn     = __phys_to_pfn(PA_A_REG),
		.length  = SIZE_A_REG,
		.type    = MT_DEVICE
	},
#endif
};

static void __init sp_map_io(void)
{
	early_printk("%s\n", __func__);

	iotable_init(sp_io_desc, ARRAY_SIZE( sp_io_desc));

	printk("B_REG %08x -> [%08x-%08x]\n", PA_B_REG, VA_B_REG, VA_B_REG + SIZE_B_REG);
#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
        printk("A_REG %08x -> [%08x-%08x]\n", PA_A_REG, VA_A_REG, VA_A_REG + SIZE_A_REG);
#endif
}

static void __init sp_init_early(void)
{
#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
	/* enable counter before timer_init */
	writel(3, (void __iomem *)A_SYS_COUNTER_BASE); /* CNTCR: EN=1 HDBG=1 */
	mb();
#endif
}

void __init sp_reserve(void)
{
}

static void __init sp_fixup(void)
{
	early_printk("%s\n", __func__);
}

void sp_restart(enum reboot_mode mode, const char *cmd)
{
	void __iomem *regs = (void __iomem *)B_SYSTEM_BASE;

	/* MOON : enable watchdog reset */
	writel(0x00120012, regs + 0x0274); /* G4.29 misc_ctl */

	/* STC: watchdog control */
	writel(0x3877, regs + 0x0630); /* stop */
	writel(0xAB00, regs + 0x0630); /* unlock */
	writel(0x0001, regs + 0x0634); /* counter */
	writel(0x4A4B, regs + 0x0630); /* resume */
}

#ifdef CONFIG_MACH_PENTAGRAM_SP7021_BCHIP
static char const *bchip_compat[] __initconst = {
	"sunplus,sp7021-bchip",
	NULL
};

DT_MACHINE_START(SP7021_BCHIP_DT, "SP7021_BCHIP")
	.dt_compat	= bchip_compat,
	.dt_fixup	= sp_fixup,
	.reserve	= sp_reserve,
	.map_io		= sp_map_io,
	.init_early	= sp_init_early,
	.init_machine	= sp_init,
	.restart	= sp_restart,
MACHINE_END
#endif

#ifdef CONFIG_MACH_PENTAGRAM_SP7021_ACHIP
static char const *achip_compat[] __initconst = {
	"sunplus,sp7021-achip",
	NULL
};

DT_MACHINE_START(SP7021_ACHIP_DT, "SP7021_ACHIP")
	.dt_compat	= achip_compat,
	.dt_fixup	= sp_fixup,
	.reserve	= sp_reserve,
	.map_io		= sp_map_io,
	.init_early	= sp_init_early,
	.init_machine	= sp_init,
	.restart	= sp_restart,
MACHINE_END
#endif
