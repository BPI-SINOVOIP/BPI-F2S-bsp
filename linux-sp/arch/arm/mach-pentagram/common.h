 /*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Common Header for EXYNOS machines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_PENTAGRAM_COMMON_H
#define __ARCH_ARM_MACH_PENTAGRAM_COMMON_H

#include <linux/platform_data/cpuidle-sp7021.h>

extern void sp7021_cpu_power_down(int cpu);
extern void sp7021_pm_central_suspend(void);
extern int sp7021_pm_central_resume(void);
extern void sp7021_enter_aftr(void);

extern struct cpuidle_sp7021_data cpuidle_coupled_sp7021_data;


#endif /* __ARCH_ARM_MACH_EXYNOS_COMMON_H */
