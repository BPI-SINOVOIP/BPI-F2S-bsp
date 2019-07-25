/*
 *
 * Coupled cpuidle support based on the work of:
 *	Colin Cross <ccross@android.com>
 *	Daniel Lezcano <daniel.lezcano@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/platform_data/cpuidle-sp7021.h>

#include <asm/suspend.h>
#include <asm/cpuidle.h>

static int tmp_state1;
extern int tmp_state1;

static struct cpuidle_sp7021_data *sp7021_cpuidle_pdata;

static int sp7021_enter_coupled_lowpower(struct cpuidle_device *dev,
					 struct cpuidle_driver *drv,
					 int index)
{
	int ret;
	unsigned int cpuid = smp_processor_id();

    if (tmp_state1 != index){
    	 tmp_state1 = index;
	     printk("index= %x \n",index);
	     printk("cpuid= %x \n",cpuid);
	     printk("dev->cpu= %x \n",dev->cpu);
	  }

	sp7021_cpuidle_pdata->pre_enter_aftr();


	/*
	 * Both cpus will reach this point at the same time
	 */
	if ((dev->cpu == 2)||(dev->cpu == 3)){
//	if ((dev->cpu == 0)||(dev->cpu == 1)){
	   ret = sp7021_cpuidle_pdata->cpu23_powerdown();
	}
//	ret = dev->cpu ? exynos_cpuidle_pdata->cpu1_powerdown()
//		       : exynos_cpuidle_pdata->cpu0_enter_aftr();
	if (ret)
		index = ret;


	sp7021_cpuidle_pdata->post_enter_aftr();

	return index;
}

//static int sp7021_enter_lowpower(struct cpuidle_device *dev,
//				struct cpuidle_driver *drv,
//				int index)
//{
//	int new_index = index;
//
//	/* AFTR can only be entered when cores other than CPU0 are offline */
//	if (num_online_cpus() > 1 || dev->cpu != 0)
//		new_index = drv->safe_state_index;
//
//	if (new_index == 0)
//		return arm_cpuidle_simple_enter(dev, drv, new_index);
//
//	sp7021_enter_aftr();
//
//	return new_index;
//}

//static struct cpuidle_driver sp7021_idle_driver = {
//	.name			= "sp7021_idle",
//	.owner			= THIS_MODULE,
//	.states = {
//		[0] = ARM_CPUIDLE_WFI_STATE,
//		[1] = {
//			.enter			= sp7021_enter_lowpower,
//			.exit_latency		= 300,
//			.target_residency	= 100000,
//			.name			= "C1",
//			.desc			= "ARM power down",
//		},
//	},
//	.state_count = 2,
//	.safe_state_index = 0,
//};

static struct cpuidle_driver sp7021_coupled_idle_driver = {
	.name			= "sp7021_coupled_idle",
	.owner			= THIS_MODULE,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= sp7021_enter_coupled_lowpower,
			.exit_latency		= 500,
			.target_residency	= 1000,
			.flags			= CPUIDLE_FLAG_COUPLED,
//			.flags			= CPUIDLE_FLAG_COUPLED |
//						  CPUIDLE_FLAG_TIMER_STOP,
			.name			= "C1",
			.desc			= "ARM power down",
		},
	},
	.state_count = 2,
	.safe_state_index = 0,
};

static int sp7021_cpuidle_probe(struct platform_device *pdev)
{
	int ret;

		sp7021_cpuidle_pdata = pdev->dev.platform_data;

		ret = cpuidle_register(&sp7021_coupled_idle_driver,
				       cpu_possible_mask);
				       
	if (ret) {
		dev_err(&pdev->dev, "failed to register cpuidle driver\n");
		return ret;
	}

	return 0;
}

static struct platform_driver sp7021_cpuidle_driver = {
	.probe	= sp7021_cpuidle_probe,
	.driver = {
		.name = "sp7021_cpuidle",
	},
};
builtin_platform_driver(sp7021_cpuidle_driver);
