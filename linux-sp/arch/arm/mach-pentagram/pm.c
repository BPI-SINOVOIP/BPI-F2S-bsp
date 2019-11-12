/*
 *
 * SP7021 - Power Management support
 *
 * Based on arch/arm/mach-s3c2410/pm.c
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/cpu_pm.h>
#include <linux/io.h>
#include <linux/of.h>

#include <asm/firmware.h>
#include <asm/smp_scu.h>
#include <asm/suspend.h>
#include <asm/cacheflush.h>

#include <mach/io_map.h>

#include "common.h"

void sp7021_enter_aftr(void)
{
}

void sp7021_pm_central_suspend(void)
{
	void __iomem *regs = (void __iomem *)A_SYSTEM_BASE;

	/* power down
	 * core2 and core3 WFI (core0&1 can't power down in sp7021)
	 * G0.20(moon0.20 ca7_ctl_cfg).1=NSLEEP_CORES : 0 sleep core2&3 , 1 not sleep(default)
	 * G0.20(moon0.20 ca7_ctl_cfg).0=ISOLATE_CORES : 0 not isolate , 1 isolate(default)
	 * G0.2(moon0.2 ca7_sw_rst) default as 32"h667f
	 * G0.7(moon0.7 ca7_standbywfi).4=CA7_STANDBYWFIL2 RO 
	 * G0.7(moon0.7 ca7_standbywfi).3~0=CA7_STANDBYWFI RO : 0 processor not in WFI lower power state
	 *                                                      1 processor in WFI lower power state 
	 *                                                     
	 *                                                       
	 */
//	 while ((((readl((void __iomem *)A_SYSTEM_BASE + 0x1C))&(0xC)) != 0xC); //wait CA7_STANDBYWFI[3:2]=2"b11
	 while (((readl(regs + 0x1C))&(0xC)) != 0xC); //wait CA7_STANDBYWFI[3:2]=2"b11
	/* MOON0 REG20(G0.20 ca7_ctl_cfg) : isolate cores and sleep core2&3 */
	writel(0x3, regs + 0x50); /* isolate cores */
	writel(0x1, regs + 0x50); /* sleep core2&3 */
	/* MOON0 REG2(G0.2 ca7_sw_rst) : core2&3 set reset */
	writel(0x667f, regs + 0x8); /* b16=CA7CORE3POR_RST_B=0 
	                               b15=CA7CORE2POR_RST_B=0                
	                               b12=CA7CORE3_RST_B=0                  
	                               b11=CA7CORE2_RST_B=0 
	                               b8=CA7DBG3_RST_B=0                 
	                               b7=CA7DBG2_RST_B=0     */
}

int sp7021_pm_central_resume(void)
{
	void __iomem *regs = (void __iomem *)A_SYSTEM_BASE;

	/* power up*/
	/* MOON0 REG20(G0.20 ca7_ctl_cfg) : isolate cores and sleep core2&3 */
	writel(0x3, regs + 0x50); /* isolate cores */
	writel(0x2, regs + 0x50); /* not sleep core2&3 */
	/* MOON0 REG2(G0.2 ca7_sw_rst) : core2&3 set reset */
	writel(0x1e67f, regs + 0x8); /* b16=CA7CORE3POR_RST_B=1 
	                                b15=CA7CORE2POR_RST_B=1     */           
	writel(0x1ffff, regs + 0x8); /* b12=CA7CORE3_RST_B=1                  
	                                b11=CA7CORE2_RST_B=1 
	                                b8=CA7DBG3_RST_B=1                 
	                                b7=CA7DBG2_RST_B=1     */

	return 0;
}

#if 1//defined(CONFIG_SMP) && defined(CONFIG_ARM_EXYNOS_CPUIDLE)
static atomic_t cpu23_wakeup = ATOMIC_INIT(0);

/**
 * exynos_core_power_down : power down the specified cpu
 * @cpu : the cpu to power down
 *
 * Power down the specified cpu. The sequence must be finished by a
 * call to cpu_do_idle()
 *
 */
void sp7021_cpu_power_down(int cpu)
{
	void __iomem *regs = (void __iomem *)A_SYSTEM_BASE;

	/* power down
	 * core2 and core3 WFI (core0&1 can't power down in sp7021)
	 * G0.20(moon0.20 ca7_ctl_cfg).1=NSLEEP_CORES : 0 sleep core2&3 , 1 not sleep(default)
	 * G0.20(moon0.20 ca7_ctl_cfg).0=ISOLATE_CORES : 0 not isolate , 1 isolate(default)
	 * G0.2(moon0.2 ca7_sw_rst) default as 32"h667f
	 * G0.7(moon0.7 ca7_standbywfi).4=CA7_STANDBYWFIL2 RO 
	 * G0.7(moon0.7 ca7_standbywfi).3~0=CA7_STANDBYWFI RO : 0 processor not in WFI lower power state
	 *                                                      1 processor in WFI lower power state 
	 *                                                     
	 *                                                       
	 */
	 while (((readl(regs + 0x1C))&(0xC)) != 0xC); //wait CA7_STANDBYWFI[3:2]=2"b11
	/* MOON0 REG20(G0.20 ca7_ctl_cfg) : isolate cores and sleep core2&3 */
	writel(0x3, regs + 0x50); /* isolate cores */
	writel(0x1, regs + 0x50); /* sleep core2&3 */
	/* MOON0 REG2(G0.2 ca7_sw_rst) : core2&3 set reset */
	writel(0x667f, regs + 0x8); /* b16=CA7CORE3POR_RST_B=0 
	                               b15=CA7CORE2POR_RST_B=0                
	                               b12=CA7CORE3_RST_B=0                  
	                               b11=CA7CORE2_RST_B=0 
	                               b8=CA7DBG3_RST_B=0                 
	                               b7=CA7DBG2_RST_B=0     */
}

static int sp7021_wfi_finisher(unsigned long flags)
{
//	if (soc_is_exynos3250())
//		flush_cache_all();
	cpu_do_idle();

	return -1;
}
//static int sp7021_cpu0_enter_aftr(void)
//{
//	return 0;
//}

static int sp7021_cpu23_powerdown(void)
{
	int ret = -1;
//	unsigned int cpuid1 = smp_processor_id();

	/*
	 * Idle sequence for cpu1
	 */
//	printk("cpuid1= %x \n",cpuid1);
	if (cpu_pm_enter())
		goto cpu23_aborted;

	/*
	 * Turn off cpu 1
	 */
//	sp7021_cpu_power_down(23);

//	ret = cpu_suspend(0, sp7021_wfi_finisher);

	cpu_pm_exit();

cpu23_aborted:
	dsb();
	/*
	 * Notify cpu 0 that cpu 1 is awake
	 */
	atomic_set(&cpu23_wakeup, 1);

	return ret;
}

static void sp7021_pre_enter_aftr(void)
{
//	unsigned long boot_addr = __pa_symbol(sp7021_cpu_resume);
//	(void)exynos_set_boot_addr(1, boot_addr);
}

static void sp7021_post_enter_aftr(void)
{
	atomic_set(&cpu23_wakeup, 0);
}

struct cpuidle_sp7021_data cpuidle_coupled_sp7021_data = {
//	.cpu0_enter_aftr		= sp7021_cpu0_enter_aftr,
	.cpu23_powerdown		= sp7021_cpu23_powerdown,
	.pre_enter_aftr		= sp7021_pre_enter_aftr,
	.post_enter_aftr		= sp7021_post_enter_aftr,
};
#endif /* CONFIG_SMP && CONFIG_ARM_EXYNOS_CPUIDLE */
