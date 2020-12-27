// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * PWM device driver for SUNPLUS SoCs
 *
 * Copyright (C) 2020 SUNPLUS Inc.
 *
 * Author:	PoChou Chen <pochou.chen@sunplus.com>
 * 			Hammer Hsieh <hammer.hsieh@sunplus.com>
 *         
 */

/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
/* #define DEBUG */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#if defined(CONFIG_SOC_SP7021)
#include <linux/mfd/sunplus/pwm-sp7021.h>
#elif defined(CONFIG_SOC_I143)
#include <linux/mfd/sunplus/pwm-i143.h>
#endif
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#if defined(CONFIG_SOC_SP7021)
#define DRV_NAME "sp7021-pwm"
#define DESC_NAME "SP7021 PWM Driver"
#elif defined(CONFIG_SOC_I143)
#define DRV_NAME "i143-pwm"
#define DESC_NAME "I143 PWM Driver"
#endif

#define SUNPLUS_PWM_NUM		ePWM_MAX

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define to_sunplus_pwm(chip)	container_of(chip, struct sunplus_pwm, chip)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct sunplus_pwm {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
};

enum {
	ePWM0,
	ePWM1,
	ePWM2,
	ePWM3,
	ePWM4,
	ePWM5,
	ePWM6,
	ePWM7,
	ePWM_MAX
};

enum {
	ePWM_DD0,
	ePWM_DD1,
	ePWM_DD2,
	ePWM_DD3,
	ePWM_DD_MAX
};

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void _sunplus_pwm_unexport(struct pwm_chip *chip, struct pwm_device *pwm);
static int _sunplus_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm);
static void _sunplus_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm);
static int _sunplus_pwm_config(struct pwm_chip *chip,
							struct pwm_device *pwm,
							int duty_ns,
							int period_ns);
#if defined(CONFIG_SOC_I143)
static int _sunplus_pwm_polarity(struct pwm_chip *chip,
							struct pwm_device *pwm,
							enum pwm_polarity polarity);
#endif
static int _sunplus_pwm_probe(struct platform_device *pdev);
static int _sunplus_pwm_remove(struct platform_device *pdev);

#ifdef CONFIG_PM
static int __maybe_unused _sunplus_pwm_suspend(struct device *dev);
static int __maybe_unused _sunplus_pwm_resume(struct device *dev);
static int __maybe_unused _sunplus_pwm_runtime_suspend(struct device *dev);
static int __maybe_unused _sunplus_pwm_runtime_resume(struct device *dev);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const struct pwm_ops _sunplus_pwm_ops = {
	.free = _sunplus_pwm_unexport,
	.enable = _sunplus_pwm_enable,
	.disable = _sunplus_pwm_disable,
	.config = _sunplus_pwm_config,
#if defined(CONFIG_SOC_I143)
	.set_polarity = _sunplus_pwm_polarity,
#endif
	.owner = THIS_MODULE,
};

static const struct of_device_id _sunplus_pwm_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-pwm", },
	{ .compatible = "sunplus,i143-pwm", },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, _sunplus_pwm_dt_ids);

#ifdef CONFIG_PM
static const struct dev_pm_ops _sunplus_pwm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(_sunplus_pwm_suspend, _sunplus_pwm_resume)
	SET_RUNTIME_PM_OPS(_sunplus_pwm_runtime_suspend,
				_sunplus_pwm_runtime_resume, NULL)
};
#endif

static struct platform_driver _sunplus_pwm_driver = {
	.probe		= _sunplus_pwm_probe,
	.remove		= _sunplus_pwm_remove,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(_sunplus_pwm_dt_ids),
#ifdef CONFIG_PM
		.pm		= &_sunplus_pwm_pm_ops,
#endif
	},
};
module_platform_driver(_sunplus_pwm_driver);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void _sunplus_reg_init(void *base)
{
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)base;

#if defined(CONFIG_SOC_SP7021)
	pPWMReg->grp244_0 = 0x0000;
	pPWMReg->grp244_1 = 0x0f00;
	pPWMReg->pwm_dd[0].idx_all = 0x0000;
	pPWMReg->pwm_dd[1].idx_all = 0x0000;
	pPWMReg->pwm_dd[2].idx_all = 0x0000;
	pPWMReg->pwm_dd[3].idx_all = 0x0000;
	pPWMReg->pwm_du[0].idx_all = 0x0000;
	pPWMReg->pwm_du[1].idx_all = 0x0000;
	pPWMReg->pwm_du[2].idx_all = 0x0000;
	pPWMReg->pwm_du[3].idx_all = 0x0000;
	pPWMReg->pwm_du[4].idx_all = 0x0000;
	pPWMReg->pwm_du[5].idx_all = 0x0000;
	pPWMReg->pwm_du[6].idx_all = 0x0000;
	pPWMReg->pwm_du[7].idx_all = 0x0000;
#elif defined(CONFIG_SOC_I143)
	pPWMReg->grp244_0 = 0x00f00000;
	pPWMReg->pwm_dd[0].idx_all = 0x00000000;
	pPWMReg->pwm_dd[1].idx_all = 0x00000000;
	pPWMReg->pwm_dd[2].idx_all = 0x00000000;
	pPWMReg->pwm_dd[3].idx_all = 0x00000000;
	pPWMReg->pwm_du[0].idx_all = 0x00000000;
	pPWMReg->pwm_du[1].idx_all = 0x00000000;
	pPWMReg->pwm_du[2].idx_all = 0x00000000;
	pPWMReg->pwm_du[3].idx_all = 0x00000000;
	pPWMReg->grp245_0 = 0x00000000;
#endif

}

static void _sunplus_savepwmclk(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sunplus_pwm *pdata = to_sunplus_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	int i;
#if defined(CONFIG_SOC_SP7021)
	u32 dd_sel = pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel;

	dev_dbg(chip->dev, "pwm clk:%d dd_sel:%d\n",
			pwm->hwpwm, dd_sel);

	if (!(pPWMReg->grp244_1 & (1 << dd_sel)))
		return;

	for (i = 0; i < ePWM_MAX; ++i) {
		if ((pPWMReg->grp244_0 & (1 << i))
			&& (pPWMReg->pwm_du[i].pwm_du_dd_sel == dd_sel))
			break;
	}

	if (i == ePWM_MAX) {
		pPWMReg->grp244_1 &= ~(1 << dd_sel);
		dev_dbg(chip->dev, "save pwm clk:%d dd_sel:%d\n",
			pwm->hwpwm, dd_sel);
	}
#elif defined(CONFIG_SOC_I143)
	u32 dd_sel = 0;

	if (pwm->hwpwm%2) {
		dd_sel = pPWMReg->pwm_du[((pwm->hwpwm)-1)/2].pwm_du_dd_sel_1;
	}
	else {
		dd_sel = pPWMReg->pwm_du[(pwm->hwpwm)/2].pwm_du_dd_sel_0;
	}

	dev_dbg(chip->dev, "pwm clk:%d dd_sel:%d\n",
			pwm->hwpwm, dd_sel);

	if(!(pPWMReg->pwm_dd[dd_sel].dd))
		return;

	for (i = 0; i < ePWM_MAX; ++i) {
		if(i%2) {
			if ((pPWMReg->grp244_0 & (1 << i))
				&& (pPWMReg->pwm_du[(i-1)/2].pwm_du_dd_sel_1 == dd_sel))
				break;
		}
		else {
			if ((pPWMReg->grp244_0 & (1 << i))
				&& (pPWMReg->pwm_du[i/2].pwm_du_dd_sel_0 == dd_sel))
				break;
		}
	}	

	if (i == ePWM_MAX) {
		pPWMReg->pwm_dd[dd_sel].dd = 0;
		dev_dbg(chip->dev, "save pwm clk:%d dd_sel:%d\n",
			pwm->hwpwm, dd_sel);
	}
#endif

}

static int _sunplus_setpwm(struct pwm_chip *chip,
		struct pwm_device *pwm,
		int duty_ns,
		int period_ns)
{
	struct sunplus_pwm *pdata = to_sunplus_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	u32 dd_sel_old = ePWM_DD_MAX, dd_sel_new = ePWM_DD_MAX;
	u32 duty = 0, dd_freq = 0x80;
	u32 tmp2;
	u64 tmp;
	u32 pwm_dd_en = 0;
	int i;

	tmp = clk_get_rate(pdata->clk) * (u64)period_ns;
	dd_freq = (u32)div64_u64(tmp, 256000000000ULL);

	dev_dbg(chip->dev, "set pwm:%d source clk rate:%llu duty_freq:0x%x(%d)\n",
			pwm->hwpwm, tmp, dd_freq, dd_freq);

	if (dd_freq == 0)
		return -EINVAL;

#if defined(CONFIG_SOC_SP7021)
	if (dd_freq >= 0xffff)
		dd_freq = 0xffff;

	pwm_dd_en = pPWMReg->grp244_1 & (0xff >> (8 - ePWM_DD_MAX));
#elif defined(CONFIG_SOC_I143)
	if (dd_freq >= 0x3ffff)
		dd_freq = 0x3ffff;

	for (i = 0; i < ePWM_DD_MAX; ++i) {
		if (pPWMReg->pwm_dd[i].dd)
			pwm_dd_en |= (1 << i);
	}
#endif

	if (pwm_dd_en & (1 << pwm->hwpwm)) {
#if defined(CONFIG_SOC_SP7021)
		dd_sel_old = pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel;
#elif defined(CONFIG_SOC_I143)
		if (pwm->hwpwm%2)
			dd_sel_old = pPWMReg->pwm_du[(pwm->hwpwm-1)/2].pwm_du_dd_sel_1;
		else
			dd_sel_old = pPWMReg->pwm_du[pwm->hwpwm/2].pwm_du_dd_sel_0;
#endif
	}
	else
		dd_sel_old = ePWM_DD_MAX;

	/* find the same freq and turnon clk source */
	for (i = 0; i < ePWM_DD_MAX; ++i) {
		if ((pwm_dd_en & (1 << i))
			&& (pPWMReg->pwm_dd[i].dd == dd_freq))
			break;
	}
	if (i != ePWM_DD_MAX)
		dd_sel_new = i;

	/* dd_sel only myself used */
	if (dd_sel_new == ePWM_DD_MAX) {
		for (i = 0; i < ePWM_MAX; ++i) {
			if (i == pwm->hwpwm)
				continue;
#if defined(CONFIG_SOC_SP7021)
			tmp2 = pPWMReg->pwm_du[i].pwm_du_dd_sel;
#elif defined(CONFIG_SOC_I143)
			if(i%2)
				tmp2 = pPWMReg->pwm_du[(i-1)/2].pwm_du_dd_sel_1;
			else
				tmp2 = pPWMReg->pwm_du[i/2].pwm_du_dd_sel_0;
#endif

			if ((pwm_dd_en & (1 << i))
					&& (tmp2 == dd_sel_old))
				break;
		}
		if (i == ePWM_MAX)
			dd_sel_new = dd_sel_old;
	}

	/* find unused clk source */
	if (dd_sel_new == ePWM_DD_MAX) {
		for (i = 0; i < ePWM_DD_MAX; ++i) {
			if (!(pwm_dd_en & (1 << i)))
				break;
		}
		dd_sel_new = i;
	}

	if (dd_sel_new == ePWM_DD_MAX) {
		dev_err(chip->dev, "pwm%d Can't found clk source[0x%x(%d)/256].\n",
				pwm->hwpwm, dd_freq, dd_freq);
		return -EBUSY;
	}

#if defined(CONFIG_SOC_SP7021)
	pPWMReg->grp244_1 |= (1 << dd_sel_new);
#endif
	pPWMReg->pwm_dd[dd_sel_new].dd = dd_freq;

	dev_dbg(chip->dev, "%s:%d found clk source:%d and set [0x%x(%d)/256]\n",
			__func__, __LINE__, dd_sel_new, dd_freq, dd_freq);

	if (duty_ns == period_ns) {
		pPWMReg->pwm_bypass |= (1 << pwm->hwpwm);
		dev_dbg(chip->dev, "%s:%d set pwm:%d bypass duty\n",
				__func__, __LINE__, pwm->hwpwm);
	} else {
		pPWMReg->pwm_bypass &= ~(1 << pwm->hwpwm);

		tmp = (u64)duty_ns * 256 + (period_ns >> 1);
		duty = (u32)div_u64(tmp, (u32)period_ns);

		dev_dbg(chip->dev, "%s:%d set pwm:%d duty:0x%x(%d)\n",
				__func__, __LINE__, pwm->hwpwm, duty, duty);
#if defined(CONFIG_SOC_SP7021)
		pPWMReg->pwm_du[pwm->hwpwm].pwm_du = duty;
		pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel = dd_sel_new;
#elif defined(CONFIG_SOC_I143)
		if (pwm->hwpwm%2) {
			pPWMReg->pwm_du[(pwm->hwpwm-1)/2].pwm_du_1 = duty;
			pPWMReg->pwm_du[(pwm->hwpwm-1)/2].pwm_du_dd_sel_1 = dd_sel_new;
		}
		else {
			pPWMReg->pwm_du[pwm->hwpwm/2].pwm_du_0 = duty;
			pPWMReg->pwm_du[pwm->hwpwm/2].pwm_du_dd_sel_0 = dd_sel_new;
		}
#endif
	}

	if ((dd_sel_old != dd_sel_new) && (dd_sel_old != ePWM_DD_MAX))
#if defined(CONFIG_SOC_SP7021)	
		pPWMReg->grp244_1 &= ~(1 << dd_sel_old);
#elif defined(CONFIG_SOC_I143)
		pPWMReg->pwm_dd[dd_sel_old].dd = 0;
#endif

	dev_dbg(chip->dev, "%s:%d pwm:%d, output freq:%lu Hz, duty:%u%%\n",
			__func__, __LINE__,
			pwm->hwpwm, clk_get_rate(pdata->clk) / (dd_freq * 256),
			(duty * 100) / 256);

	return 0;
}

#if defined(CONFIG_SOC_I143)
static int _sunplus_pwm_polarity(struct pwm_chip *chip,
		struct pwm_device *pwm,
		enum pwm_polarity polarity)
{
	struct sunplus_pwm *pdata = to_sunplus_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;

	if(polarity == PWM_POLARITY_NORMAL)
		pPWMReg->pwm_inv &= ~(1 << pwm->hwpwm);
	else
		pPWMReg->pwm_inv |= (1 << pwm->hwpwm);

	return 0;
}
#endif

static int _sunplus_pwm_config(struct pwm_chip *chip,
		struct pwm_device *pwm,
		int duty_ns,
		int period_ns)
{
	dev_dbg(chip->dev, "%s:%d set pwm:%d enable:%d duty_ns:%d period_ns:%d\n",
			__func__, __LINE__,
			pwm->hwpwm, pwm_is_enabled(pwm),
			duty_ns, period_ns);

	if (pwm_is_enabled(pwm)) {
		if (_sunplus_setpwm(chip, pwm, duty_ns, period_ns))
			return -EBUSY;
	}

	return 0;
}

static void _sunplus_pwm_unexport(struct pwm_chip *chip, struct pwm_device *pwm)
{
	dev_dbg(chip->dev, "%s:%d unexport pwm:%d\n",
		__func__, __LINE__, pwm->hwpwm);

	if (pwm_is_enabled(pwm)) {
		struct pwm_state state;

		pwm_get_state(pwm, &state);
		state.enabled = 0;
		pwm_apply_state(pwm, &state);
	}
}

static int _sunplus_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sunplus_pwm *pdata = to_sunplus_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	u32 period = pwm_get_period(pwm);
	u32 duty_cycle = pwm_get_duty_cycle(pwm);

	dev_dbg(chip->dev, "%s:%d duty_ns:%d period_ns:%d\n",
			__func__, __LINE__,
			duty_cycle, period);

	pm_runtime_get_sync(chip->dev);

	if (!_sunplus_setpwm(chip, pwm, duty_cycle, period))
		pPWMReg->pwm_en |= (1 << pwm->hwpwm);
	else {
		pm_runtime_put(chip->dev);
		return -EBUSY;
	}

	return 0;
}

static void _sunplus_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sunplus_pwm *pdata = to_sunplus_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;

	dev_dbg(chip->dev, "%s:%d pwm:%d\n", __func__, __LINE__, pwm->hwpwm);

	pPWMReg->pwm_en &= ~(1 << pwm->hwpwm);
	_sunplus_savepwmclk(chip, pwm);

	pm_runtime_put(chip->dev);
}

static int _sunplus_pwm_probe(struct platform_device *pdev)
{
	struct sunplus_pwm *pdata;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (!np) {
		dev_err(&pdev->dev, "invalid devicetree node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		dev_err(&pdev->dev, "devicetree status is not available\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		dev_err(&pdev->dev, "get resource memory from devicetree node.\n");
		return PTR_ERR(res);
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	pdata->chip.dev = &pdev->dev;
	pdata->chip.ops = &_sunplus_pwm_ops;
	pdata->chip.npwm = SUNPLUS_PWM_NUM;
	/* pwm cell = 2 (of_pwm_simple_xlate) */
	pdata->chip.of_xlate = NULL;

	pdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->base)) {
		dev_err(&pdev->dev, "mapping resource memory.\n");
		return PTR_ERR(pdata->base);
	}

	pdata->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pdata->clk)) {
		dev_err(&pdev->dev, "not found clk source.\n");
		return PTR_ERR(pdata->clk);
	}
	ret = clk_prepare_enable(pdata->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clk source.\n");
		return ret;
	}

	/* init module reg */
	_sunplus_reg_init(pdata->base);

	ret = pwmchip_add(&pdata->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add PWM chip\n");
		clk_disable_unprepare(pdata->clk);
		return ret;
	}

	platform_set_drvdata(pdev, pdata);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	pm_runtime_put(&pdev->dev);

	return 0;
}

static int _sunplus_pwm_remove(struct platform_device *pdev)
{
	struct sunplus_pwm *pdata;
	int ret;

	pm_runtime_disable(&pdev->dev);

	pdata = platform_get_drvdata(pdev);
	if (pdata == NULL)
		return -ENODEV;

	ret = pwmchip_remove(&pdata->chip);

#ifndef CONFIG_PM
	clk_disable_unprepare(pdata->clk);
#endif
	return ret;
}

#ifdef CONFIG_PM
static int __maybe_unused _sunplus_pwm_suspend(struct device *dev)
{
	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	return pm_runtime_force_suspend(dev);
}

static int __maybe_unused _sunplus_pwm_resume(struct device *dev)
{
	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	return pm_runtime_force_resume(dev);
}

static int __maybe_unused _sunplus_pwm_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	clk_disable(devm_clk_get(dev, NULL));
	return 0;
}

static int __maybe_unused _sunplus_pwm_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	return clk_enable(devm_clk_get(dev, NULL));
}
#endif

MODULE_DESCRIPTION(DESC_NAME);
MODULE_AUTHOR("SUNPLUS,Inc");
MODULE_LICENSE("GPL v2");

