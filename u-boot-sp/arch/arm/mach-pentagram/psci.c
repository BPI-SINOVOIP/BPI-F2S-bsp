/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * psci code for board/sunplus/pentagram_board
 *
 * Copyright (c) 2020 Sunplus
 */
#include <config.h>
#include <common.h>
#include <asm/armv7.h>
#include <asm/gic.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/secure.h>
#include <linux/bitops.h>

int __secure psci_cpu_on(u32 __always_unused unused, u32 mpidr, u32 pc,
			 u32 context_id)
{
	u32 cpu = (mpidr & 0x3) ;

	/* store target PC and context id */
	psci_save(cpu, pc, context_id);

	/* wakeup core */
	*(volatile u32 *)(CONFIG_SMP_PEN_ADDR - cpu * 4) = (u32)&psci_cpu_entry;
	__asm__ __volatile__ ("dsb ishst; sev");

	return ARM_PSCI_RET_SUCCESS;
}

void __secure psci_cpu_off(void)
{
	u32 cpu = psci_get_cpu_id();
	volatile u32 *ca7_sw_rst = (void *)0x9ec00008;

	psci_cpu_off_common();

	/* reset core, wfe is managed by BootRom(iboot) */
	clrbits_le32(ca7_sw_rst, BIT(cpu + 9));
	dsb();
	setbits_le32(ca7_sw_rst, BIT(cpu + 9));
	dsb();

	/* Wait to be turned off */
	while (1)
		wfi();
}
