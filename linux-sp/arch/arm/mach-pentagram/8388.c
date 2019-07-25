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
#include <mach/misc.h>

#define SYSTEM_BASE		VA_IO_ADDR(0 * 32 * 4)

#ifdef CONFIG_CACHE_L2X0

#include <asm/hardware/cache-l2x0.h>
#include <asm/cacheflush.h>

static int __init sp_l2_cache_init(void)
{
	u32 aux_ctrl = 0;
	u32 value;
	void __iomem *l2cache_base;

	//early_printk("%s\n", __func__);

	/* avoid data corruption during l2c init */
	flush_cache_all();

	/* Static mapping, never released */
	l2cache_base = (void __iomem *)ioremap(PA_L2CC_REG, 0x1000);
	if (WARN_ON(!l2cache_base))
		return -ENOMEM;

	value = *(volatile u32 *)(l2cache_base + 0x0104); /* reg1_aux_control */
	value |=  (1 << 30);    /* Early BRESP */
	value &= ~(1 << 25);    /* pseudo random */
	value |=  (1 <<  0);    /* Enable full line of zero */
	*(volatile u32 *)(l2cache_base + 0x0104) = value;
	aux_ctrl = value;

	asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r"(value)); /* aux control register */
	value |= (1 << 2);      /* enable L1 prefetch */
	value |= (1 << 1);      /* enable L2 prefetch hint */
	asm volatile("mcr p15, 0, %0, c1, c0, 1" : : "r"(value));

	value = *(volatile u32 *)(l2cache_base + 0x0F60); /* reg15_prefetch_ctrl */
	value |=  (1 << 30);    /* Double linefill */
	value |=  (1 << 29);    /* Instruction prefetch enable */
	value |=  (1 << 28);    /* Data prefetch enable */
	value |=  (1 << 27);    /* Double linefill on WRAP read disable */
	value |=  (1 << 23);    /* Incr double linefill enable */
	value |=  (1 << 24);    /* Prefetch drop enable */
	value &= ~(0x1F << 0);  /* Prefetch offset */
	value |=  (7 << 0);     /* Prefetch offset */
	*(volatile u32 *)(l2cache_base + 0x0F60) = value;

	value = *(volatile u32 *)(l2cache_base + 0x0F80); /* reg15_power_ctrl */
	value |= (1 << 1);      /* dynamic_clk_gating_en */
	value |= (1 << 0);      /* standby_mode_en */
	*(volatile u32 *)(l2cache_base + 0x0F80) = value;

	*(volatile u32 *)(l2cache_base + 0x0108) = 0;       /* reg1_tag_ram_control, recommanded by HW designer */
	*(volatile u32 *)(l2cache_base + 0x010C) = 0x0111;  /* reg1_data_ram_control, recommanded by HW designer */

	l2x0_init(l2cache_base, aux_ctrl, 0xFFFFFFFF);

	/*
	 * According to L2C-310 TRM r3p3 page 2-37~38,
	 * CA9 "Full line of zero" should be enabled after L2C enable.
	 */
	asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r"(value)); /* aux control register */
	value |= (1 << 3); /* enable write full line of zero */
	asm volatile("mcr p15, 0, %0, c1, c0, 1" : : "r"(value));

	return 0;
}
early_initcall(sp_l2_cache_init);
#endif

static void __init sp_power_off(void)
{
	early_printk("%s\n", __func__);
}

static void __init sp_init(void)
{
	early_printk("%s\n", __func__);
	pm_power_off = sp_power_off;

	sp_prn_uptime();
}

static struct map_desc sp_io_desc[] __initdata = {
	{	/* RGST Bus */
		.virtual = VA_REG,
		.pfn     = __phys_to_pfn(PA_REG),
		.length  = SIZE_REG,
		.type    = MT_DEVICE
	},
};

static void __init sp_map_io(void)
{
	early_printk("%s\n", __func__);
	iotable_init(sp_io_desc, ARRAY_SIZE( sp_io_desc));
	early_printk("%s: done\n", __func__);
}

void __init sp_reserve(void)
{
	early_printk("%s\n", __func__);
}

static void __init sp_fixup(void)
{
	early_printk("%s\n", __func__);
	memblock_add(PHYS_OFFSET, SZ_64M);
}

void sp_restart(enum reboot_mode mode, const char *cmd)
{
	void __iomem *regs = (void __iomem *)SYSTEM_BASE;

	/* MOON1: enable watchdog reset */
#ifdef CONFIG_MACH_PENTAGRAM_3502_ACHIP
	writel(BIT(10) | BIT(1), regs + 0xA0); /* G1.8 */
#else
	writel(BIT(10) | BIT(1), regs + 0xA8); /* G1.10 */
#endif

	/* STC: watchdog control */
	writel(0x3877, regs + 0x0630); /* stop */
	writel(0xAB00, regs + 0x0630); /* unlock */
	writel(0x0001, regs + 0x0634); /* counter */
	writel(0x4A4B, regs + 0x0630); /* resume */
}

static char const *achip_compat[] __initconst = {
	"sunplus,3502-achip",
	"sunplus,8388-achip",
	NULL
};

DT_MACHINE_START(8388_ACHIP_DT, "8388_ACHIP")
	.dt_compat	= achip_compat,
	.dt_fixup	= sp_fixup,
	.reserve	= sp_reserve,
	.map_io		= sp_map_io,
	.init_machine	= sp_init,
	.restart	= sp_restart,
MACHINE_END

#ifdef CONFIG_MACH_PENTAGRAM_8388_BCHIP
static char const *bchip_compat[] __initconst = {
	"sunplus,8388-bchip",
	NULL
};

DT_MACHINE_START(8388_BCHIP_DT, "8388_BCHIP")
	.dt_compat	= bchip_compat,
	.dt_fixup	= sp_fixup,
	.reserve	= sp_reserve,
	.map_io		= sp_map_io,
	.init_machine	= sp_init,
	.restart	= sp_restart,
MACHINE_END
#endif
