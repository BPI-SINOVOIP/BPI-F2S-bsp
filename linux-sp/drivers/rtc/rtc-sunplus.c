/*
 * How to test RTC:
 *
 * hwclock - query and set the hardware clock (RTC)
 *
 * (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' && hwclock -r; sleep 1); done)
 * date 121209002014 # Set system to 2014/Dec/12 09:00
 * (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' && hwclock -r; sleep 1); done)
 * hwclock -s # Set the System Time from the Hardware Clock
 * (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' && hwclock -r; sleep 1); done)
 * date 121213002014 # Set system to 2014/Dec/12 13:00
 * (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' && hwclock -r; sleep 1); done)
 * hwclock -w # Set the Hardware Clock to the current System Time
 * (for i in `seq 10000`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' && hwclock -r; sleep 1); done)
 *
 *
 * How to setup alarm (e.g., 10 sec later):
 *     echo 0 > /sys/class/rtc/rtc0/wakealarm && \
 *     nnn=`date '+%s'` && echo $nnn && nnn=`expr $nnn + 10` && echo $nnn > /sys/class/rtc/rtc0/wakealarm
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <mach/io_map.h>
#include <linux/clk.h>
#include <linux/reset.h> 

#if 0
/* For code development on SPHE8388 */
#define VA_B_REG		0xF8000000
#endif


/* ---------------------------------------------------------------------------------------------- */
#define RTC_FUNC_DEBUG
//#define RTC_DBG_INFO
//#define RTC_DBG_ERR

#ifdef RTC_FUNC_DEBUG
	#define FUNC_DEBUG()    printk(KERN_INFO "[RTC] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef RTC_DBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "[RTC] Info: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef RTC_DBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR  "[RTC] Error: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif
/* ---------------------------------------------------------------------------------------------- */

struct sunplus_rtc {

	struct clk *rtcclk;
	struct reset_control *rstc;
		
};

struct sunplus_rtc sp_rtc;
//static struct sunplus_rtc *sp_rtc;

//static struct clk *rtc_clk;

//#define VIRT_ADDR_RTC		(VA_B_REG + (116 << 7))

#define RTC_REG_NAME             "rtc_reg"

struct sp_rtc_reg {
	volatile unsigned int rsv00;
	volatile unsigned int rsv01;
	volatile unsigned int rsv02;
	volatile unsigned int rsv03;
	volatile unsigned int rsv04;
	volatile unsigned int rsv05;
	volatile unsigned int rsv06;
	volatile unsigned int rsv07;
	volatile unsigned int rsv08;
	volatile unsigned int rsv09;
	volatile unsigned int rsv10;
	volatile unsigned int rsv11;
	volatile unsigned int rsv12;
	volatile unsigned int rsv13;
	volatile unsigned int rsv14;
	volatile unsigned int rsv15;
	volatile unsigned int rtc_ctrl;
	volatile unsigned int rtc_timer_out;
	volatile unsigned int rtc_divider;
	volatile unsigned int rtc_timer_set;
	volatile unsigned int rtc_alarm_set;
	volatile unsigned int rtc_user_data;
	volatile unsigned int rtc_reset_record;
	volatile unsigned int rtc_battery_ctrl;
	volatile unsigned int rtc_trim_ctrl;
	volatile unsigned int rsv25;
	volatile unsigned int rsv26;
	volatile unsigned int rsv27;
	volatile unsigned int rsv28;
	volatile unsigned int rsv29;
	volatile unsigned int rsv30;
	volatile unsigned int rsv31;
};
static volatile struct sp_rtc_reg *rtc_reg_ptr = NULL;

//static struct platform_device *sp_rtc_device0;

static void sp_get_seconds(unsigned long *secs)
{
	*secs = (unsigned long)(rtc_reg_ptr->rtc_timer_out);
}

static void sp_set_seconds(unsigned long secs)
{
	rtc_reg_ptr->rtc_timer_set = (u32)(secs);
}

static int sp_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long secs;

	sp_get_seconds(&secs);
	rtc_time_to_tm(secs, tm);
#if 0
	printk("%s:  RTC date/time to %d-%d-%d, %02d:%02d:%02d.\r\n",
	       __func__, tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);
#endif
	return rtc_valid_tm(tm);
}

int sp_rtc_get_time(struct rtc_time *tm)
{
	unsigned long secs;

	sp_get_seconds(&secs);
	rtc_time_to_tm(secs, tm);

	return 0;
}
EXPORT_SYMBOL(sp_rtc_get_time);

static int sp_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	FUNC_DEBUG();
	
	rtc_reg_ptr->rtc_ctrl |= 1 << 4;	/* Keep RTC from system reset */

	return 0;
}

static int sp_rtc_resume(struct platform_device *pdev)
{
	/*
	 * Because RTC is still powered during suspend,
	 * there is nothing to do here.
	 */
	FUNC_DEBUG();
	
	rtc_reg_ptr->rtc_ctrl |= 1 << 4;	/* Keep RTC from system reset */
	return 0;
}

static int sp_rtc_set_mmss(struct device *dev, unsigned long secs)
{
	dev_info(dev, "%s, secs = %lu\n", __func__, secs);
	sp_set_seconds(secs);
	return 0;
}


static int sp_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned long alarm_time;

	alarm_time = rtc_tm_to_time64(&alrm->time);
	printk("%s, alarm_time: %u\n", __func__, (u32)(alarm_time));

	if (alarm_time > 0xFFFFFFFF)
		return -EINVAL;

	rtc_reg_ptr->rtc_alarm_set = (u32)(alarm_time);
	rtc_reg_ptr->rtc_ctrl = (0x003F << 16) | 0x0017;

	return 0;
}

static int sp_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned int alarm_time;

	alarm_time = rtc_reg_ptr->rtc_alarm_set;
	printk("%s, alarm_time: %u\n", __func__, alarm_time);
	rtc_time64_to_tm((unsigned long)(alarm_time), &alrm->time);

	return 0;
}


static const struct rtc_class_ops sp_rtc_ops = {
	.read_time = sp_rtc_read_time,
	.set_mmss = sp_rtc_set_mmss,
	.set_alarm = sp_rtc_set_alarm,
	.read_alarm = sp_rtc_read_alarm,
};

static int sp_rtc_probe(struct platform_device *plat_dev)
{
	int ret;
	struct rtc_device *rtc;
	struct resource *res;
	void __iomem *reg_base;

      FUNC_DEBUG();

	//memset(sp_rtc, 0, sizeof(sp_rtc));
	memset(&sp_rtc, 0, sizeof(sp_rtc));

	/* find and map our resources */
	res = platform_get_resource_byname(plat_dev, IORESOURCE_MEM, RTC_REG_NAME);
	DBG_INFO("res 0x%x\n",res->start);
	if (res) {
		reg_base = devm_ioremap_resource(&plat_dev->dev, res);
		if (IS_ERR(reg_base)) {
			dev_err(&plat_dev->dev,"%s devm_ioremap_resource fail\n",RTC_REG_NAME);
		}
	}
        DBG_INFO("reg_base 0x%x\n",(unsigned int)reg_base);


      /* clk*/
	DBG_INFO("Enable RTC clock\n");
	sp_rtc.rtcclk = devm_clk_get(&plat_dev->dev,NULL);
	DBG_INFO("sp_rtc->clk = %x\n",sp_rtc.rtcclk);
	if(IS_ERR(sp_rtc.rtcclk)) {
		dev_err(&plat_dev->dev, "devm_clk_get fail\n");
	}
	ret = clk_prepare_enable(sp_rtc.rtcclk);


	/* reset*/
	DBG_INFO("Enable RTC reset function\n");	
	sp_rtc.rstc = devm_reset_control_get(&plat_dev->dev, NULL);
	DBG_INFO( "sp_rtc->rstc : 0x%x \n",sp_rtc.rstc);
	if (IS_ERR(sp_rtc.rstc)) {
		ret = PTR_ERR(sp_rtc.rstc);
		dev_err(&plat_dev->dev, "SPI failed to retrieve reset controller: %d\n", ret);
		goto free_clk;
	}
	ret = reset_control_deassert(sp_rtc.rstc);
	if (ret)
		goto free_clk;

	DBG_INFO("sp_rtc->rstc002\n");
	rtc_reg_ptr = (volatile struct sp_rtc_reg *)(reg_base);
	rtc_reg_ptr->rtc_ctrl |= 1 << 4;	/* Keep RTC from system reset */

	rtc = rtc_device_register("sp7021-rtc", &plat_dev->dev, &sp_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		goto free_reset_assert;
	}

	platform_set_drvdata(plat_dev, rtc);

	return 0;


free_reset_assert:
	reset_control_assert(sp_rtc.rstc);
free_clk:
	clk_disable_unprepare(sp_rtc.rtcclk);

		return ret;	
	
}

static int sp_rtc_remove(struct platform_device *plat_dev)
{
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	reset_control_assert(sp_rtc.rstc);
	
	rtc_device_unregister(rtc);
	return 0;
}

static const struct of_device_id sp_rtc_of_match[] = {
	{ .compatible = "sunplus,sp7021-rtc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_rtc_of_match);

static struct platform_driver sp_rtc_driver = {
	.probe		= sp_rtc_probe,
	.remove 	= sp_rtc_remove,
	.suspend	= sp_rtc_suspend,
	.resume 	= sp_rtc_resume,
	.driver 	= {
		.name = "sp7021-rtc",
		.owner = THIS_MODULE,
		.of_match_table = sp_rtc_of_match,
	},
};



static int __init sp_rtc_init(void)
{
	int err;

      FUNC_DEBUG();

	if ((err = platform_driver_register(&sp_rtc_driver)))
		return err;

//	if ((sp_rtc_device0 = platform_device_alloc("sp7021-rtc", 0)) == NULL) {
//		err = -ENOMEM;

//		goto exit_driver_unregister;
//	}

//	if ((err = platform_device_add(sp_rtc_device0)))
//		goto exit_free_sp_rtc_device0;

//	if (device_init_wakeup(&(sp_rtc_device0->dev), true)) {
//		printk(KERN_WARNING "dev_init_wakeup() fails.\n");
//	}

	return 0;

//exit_free_sp_rtc_device0:
//	platform_device_put(sp_rtc_device0);

//exit_driver_unregister:
//	platform_driver_unregister(&sp_rtc_driver);
//	return err;
}

static void __exit sp_rtc_exit(void)
{
	//platform_device_unregister(sp_rtc_device0);
	platform_driver_unregister(&sp_rtc_driver);
}

MODULE_AUTHOR("Sunplus");
MODULE_DESCRIPTION("Sunplus RTC driver");
MODULE_LICENSE("GPL");

module_init(sp_rtc_init);
module_exit(sp_rtc_exit);
