#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/sched_clock.h>
#include <asm/mach/time.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "sp_stc.h"

#define SP_STC_FREQ	(13500000)		/* 13.5 MHz */

/* STC group can be B G99 STC, or A G19 STC. */
static void __iomem *sp_timer_base;

enum {
	SP_TIMER_STC_TIMER0,
	SP_TIMER_STC_TIMER1,
	SP_TIMER_STC_TIMER2,
	SP_TIMER_STC_TIMER3,
	SP_TIMER_NR_IRQS,
};

static int sp_timer_irqs[SP_TIMER_NR_IRQS];

u64 sp_read_sched_clock(void)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(sp_timer_base);
	u32 val0, val1;

	pstc_avReg->stcl_2 = 0;
	wmb();		/* Let the STC register accesses be in-order */

	do {
		val0 = pstc_avReg->stcl_0;
		val1 = pstc_avReg->stcl_1;
	} while (val1 != pstc_avReg->stcl_1); /* latched by others ? */

	return (u64)((val1 << 16) | val0);
}

static void sp_stc_hw_init(int reset_counter)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(sp_timer_base);
	u32 system_clk;

	system_clk = (270 * 1000 * 1000); /* 270MHz */

	/*
	 * STC
	 * STC: clock source for scheduler and jiffies (HZ)
	 * timer0/1: Unused.
	 * timer2: event for testing
	 * timer3: event for jiffies/hrtimer
	 */
#if (SP_STC_FREQ == 1000000)
	pstc_avReg->stc_divisor = (system_clk / SP_STC_FREQ) - 1;	/* CLKstc = 1 us */
#elif (SP_STC_FREQ == 13500000)
	pstc_avReg->stc_divisor = (1 << 15);				/* CLKstc = (1 / 13500000) s */
#elif (SP_STC_FREQ == 270000000)
	pstc_avReg->stc_divisor = (system_clk / SP_STC_FREQ) - 1;	/* CLKstc = CLKsys */
#else
#error Unexpected STC frequency
#endif
	if (reset_counter)
		pstc_avReg->stc_64 = 0;	/* reset STC and start counting*/
}



/***************************
 * Timer2
 ***************************/

/* Use timer2 to measure stc irq latency */
#ifdef CONFIG_SP_TIMER_IRQ_LATENCY_TEST

/*
 * Start/read the irq latency measuring by write/read /proc/cycstc_test.
 * Steps :
 * 1. To start the measuring, write hz value (timer irq frequency) to the proc entry.
 * 2. Read the proc entry to get statistics
 * 3. To stop the test, write 0 to the proc entry.
 *
 * Eg: Measure with timer irq in 100HZ. Read the result once per second.
 * echo 100 > /proc/cycstc_test ; while :; do cat /proc/cycstc_test ; sleep 1 ; done
 */

/* define to trace last act records */
//#define ACT_RECORDS	16

static struct cycstc_stat_st {
	u32 samples;
	u32 interval_ts;	/* interval to trigger stc interrupt */
	u64 sum_ts;		/* avg = sum / samples */
	u32 base_ts;		/* based last ts */
	u32 act_ts;		/* act delta ts */
	u32 min_ts, max_ts;	/* min/max delta ts */
	u32 first_ts;		/* first delta ts (setup timer -> first irq) */
#ifdef ACT_RECORDS
	u32 aidx;
	u32 act_record[ACT_RECORDS];
#endif
} cyc_stat;

static u32 sp_stc_ts_per_sec(void)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(sp_timer_base);
	return SP_STC_FREQ / ((pstc_avReg->stc_divisor & 0x3fff) + 1);
}

/* u32 usec max : 4294.9 sec */
static u32 cycstc_ts2us(u32 ts)
{
	u64 val = (u64)ts * USEC_PER_SEC;
	do_div(val, sp_stc_ts_per_sec());
	return (u32)val;
}

static irqreturn_t cycstc_timer2_isr(int irq, void *dev_id)
{
	u32 now_ts, delta_ts;

	now_ts = (u32)sp_read_sched_clock();

	delta_ts = (now_ts > cyc_stat.base_ts) ?  (now_ts - cyc_stat.base_ts) :
		   ((0xffffffffU - cyc_stat.base_ts) + now_ts + 1);

	if (delta_ts < cyc_stat.interval_ts) {
		if(printk_ratelimit()) {
			printk("cycstc: delta_ts comes early -%u ts ! samples=%u\n",
			       cyc_stat.interval_ts - delta_ts, cyc_stat.samples);
		}
		delta_ts = cyc_stat.interval_ts;
	}

	now_ts = delta_ts - cyc_stat.interval_ts;

	/* ignore 1st stc irq for it is not punctual immediately after timer setup */
	if (cyc_stat.first_ts == 0) {
		cyc_stat.first_ts = now_ts;
		goto fixup_base_out;
	}

#ifdef ACT_RECORDS
	cyc_stat.act_record[cyc_stat.aidx % ACT_RECORDS] = now_ts;
	cyc_stat.aidx = (cyc_stat.aidx + 1) % ACT_RECORDS;
#endif
	cyc_stat.act_ts = now_ts;

	if (cyc_stat.act_ts > cyc_stat.max_ts)
		cyc_stat.max_ts = cyc_stat.act_ts;
	if (cyc_stat.act_ts < cyc_stat.min_ts) {
		cyc_stat.min_ts = cyc_stat.act_ts;
	}
	cyc_stat.sum_ts += cyc_stat.act_ts;
	cyc_stat.samples++;

	if (cyc_stat.samples == 0) /* for hz=4000, this happens in 12.4 days */
		cyc_stat.sum_ts = 0;

	/* let base_ts catch up with accumulated delays */
fixup_base_out:
	while (delta_ts >= cyc_stat.interval_ts) {
		delta_ts -= cyc_stat.interval_ts;
		cyc_stat.base_ts += cyc_stat.interval_ts;
	}

	return IRQ_HANDLED;
}

/* pre-configured stc setup */
enum {
	STCINT_TMR_1HZ,		/* 1s */
	STCINT_TMR_2HZ,		/* 500ms */
	STCINT_TMR_10HZ,	/* 100ms */
	STCINT_TMR_100HZ,	/* 10ms */
	STCINT_TMR_500HZ,	/* 5ms */
	STCINT_TMR_1000HZ,	/* 1ms */
	STCINT_TMR_2000HZ,	/* 500us */
	STCINT_TMR_NUM,
};

const static struct cycstc_setup_t {
	u16 hz;		/* cycstc timer interrupt frequency (hz) */
	u16 clksrc;
	u16 div;
	u16 reload;
} cycstc_setup[STCINT_TMR_NUM] = {
#if (SP_STC_FREQ == 13500000)
	[STCINT_TMR_1HZ]        = {    1, SP_STC_AV_TIMER23_CTL_SRC_EXT, (1350 - 1), (10000 - 1) }, /* 10KHz */
	[STCINT_TMR_2HZ]        = {    2, SP_STC_AV_TIMER23_CTL_SRC_EXT, ( 135 - 1), (50000 - 1) }, /* 100KHz */
	[STCINT_TMR_10HZ]       = {   10, SP_STC_AV_TIMER23_CTL_SRC_EXT, (  27 - 1), (50000 - 1) }, /* 500KHz */
	[STCINT_TMR_100HZ]      = {  100, SP_STC_AV_TIMER23_CTL_SRC_EXT, (   3 - 1), (45000 - 1) }, /* 4.5MHz */
	[STCINT_TMR_500HZ]      = {  500, SP_STC_AV_TIMER23_CTL_SRC_EXT, (   1 - 1), (27000 - 1) }, /* 13.5MHz */
	[STCINT_TMR_1000HZ]     = { 1000, SP_STC_AV_TIMER23_CTL_SRC_EXT, (   1 - 1), (13500 - 1) },
	[STCINT_TMR_2000HZ]     = { 2000, SP_STC_AV_TIMER23_CTL_SRC_EXT, (   1 - 1), ( 6750 - 1) },
#elif (SP_STC_FREQ == 270000000)
	[STCINT_TMR_1HZ]	= {    1, SP_STC_AV_TIMER23_CTL_SRC_STC, (6750 - 1), (40000 - 1) }, /* 40KHz */
	[STCINT_TMR_2HZ]	= {    2, SP_STC_AV_TIMER23_CTL_SRC_STC, (2700 - 1), (50000 - 1) }, /* 100KHz */
	[STCINT_TMR_10HZ]	= {   10, SP_STC_AV_TIMER23_CTL_SRC_STC, (2700 - 1), (10000 - 1) },
	[STCINT_TMR_100HZ]	= {  100, SP_STC_AV_TIMER23_CTL_SRC_STC, ( 270 - 1), (10000 - 1) }, /* 1MHz */
	[STCINT_TMR_500HZ]	= {  500, SP_STC_AV_TIMER23_CTL_SRC_STC, (   9 - 1), (60000 - 1) }, /* 30MHz */
	[STCINT_TMR_1000HZ]	= { 1000, SP_STC_AV_TIMER23_CTL_SRC_STC, (   9 - 1), (30000 - 1) },
	[STCINT_TMR_2000HZ]	= { 2000, SP_STC_AV_TIMER23_CTL_SRC_STC, (   3 - 1), (45000 - 1) }, /* 60MHz */
#else
#error Unexpected STC frequency
#endif
};

static void cycstc_timer_stop(void)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(sp_timer_base);

	pstc_avReg->timer2_ctrl = 0;
}

static void cycstc_timer_start(int choice)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(sp_timer_base);

	cyc_stat = (const struct cycstc_stat_st){ 0 };

	cyc_stat.min_ts = 0xffffffffU;
	cyc_stat.interval_ts = sp_stc_ts_per_sec() / cycstc_setup[choice].hz; /* stc counter ticks */

	pstc_avReg->timer2_divisor = cycstc_setup[choice].div;
	pstc_avReg->timer2_reload = cycstc_setup[choice].reload;
	pstc_avReg->timer2_cnt = cycstc_setup[choice].reload + 1; /* +1 trick for 1st reload */

	/* measure based ts. Next stc irq happens at (base_ts + interval_ts) */
	cyc_stat.base_ts = (u32)sp_read_sched_clock();

	/* Start periodic timer */
	pstc_avReg->timer2_ctrl = cycstc_setup[choice].clksrc | SP_STC_AV_TIMER23_CTL_RPT | SP_STC_AV_TIMER23_CTL_GO;
}


/* Write hz to set cycstc frequency and start the test. Write 0 to stop test */
static ssize_t cycstc_test_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char buffer[128];
	int choice, len;
	u32 hz;

	if ((count > sizeof(buffer)) || (copy_from_user(buffer, buf, count))) {
		printk(KERN_ERR "%s: L#%d\n", __func__, __LINE__);
		return -EINVAL;
	}

	cycstc_timer_stop();

	sscanf(buffer, "%u", &hz);

	/* Stop test */
	if (hz == 0)
		return count;

	for (choice = 0; choice < STCINT_TMR_NUM; choice++) {
		if (cycstc_setup[choice].hz == hz)
			break;
	}

	if (choice >= STCINT_TMR_NUM) {
		len = sprintf(buffer, "No such hz choice! Available hz: ");
		for (choice = 0; choice < STCINT_TMR_NUM; choice++)
			len += sprintf(buffer + len, "%u ", cycstc_setup[choice].hz);
		printk("%s\n", buffer);
		return count;
	}

	cycstc_timer_start(choice);

	return count;
}

static int cycstc_test_show(struct seq_file *m, void *v)
{
	u64 avg = 0;
	u32 samples;
	u32 min, max, now;

	now = cyc_stat.act_ts;
	min = cyc_stat.min_ts;
	max = cyc_stat.max_ts;
	samples = cyc_stat.samples;
	avg = cyc_stat.sum_ts;

	if (samples) {
		do_div(avg, samples);
		seq_printf(m, "Cyc:%10u IRQ interval(us):%u  Min:%7u Act:%7u Avg:%7u Max:%7u\n",
			  samples, cycstc_ts2us(cyc_stat.interval_ts), cycstc_ts2us(min),
			  cycstc_ts2us(now), cycstc_ts2us(avg), cycstc_ts2us(max));
#ifdef ACT_RECORDS
			for (now = 0; now < ACT_RECORDS; now++) {
				if (cyc_stat.aidx == (now + 1) % ACT_RECORDS)
					seq_printf(m, "*");
				seq_printf(m, "%u ", cycstc_ts2us(cyc_stat.act_record[now]));
			}
			seq_printf(m, "\n");
#endif
	}

	return 0;
}

static int cycstc_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, cycstc_test_show, NULL);
}

static const struct file_operations cycstc_test_proc_fops = {
	.open           = cycstc_test_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.write          = cycstc_test_write,
	.release        = single_release,
};

static struct irqaction cycstc_timer2_irq = {
	.name           = "cycstc_timer2_test",
	.flags          = IRQF_TIMER | IRQF_TRIGGER_RISING,
	.handler        = cycstc_timer2_isr,
};

static int __init cycstc_test_init(void)
{
	struct proc_dir_entry *ent;
	int err = 0;

	printk("cycstc: init\n");

	if (!sp_timer_base) {
		printk(KERN_ERR "%s: no timer base\n", __func__);
		return -EINVAL;
	}

	/* stc is not initialized if not being a clocksource */
	sp_stc_hw_init(0);

	cycstc_timer_stop();

	cycstc_timer2_irq.irq = sp_timer_irqs[SP_TIMER_STC_TIMER2];

	err = setup_irq(cycstc_timer2_irq.irq, &cycstc_timer2_irq);
	if (err) {
		printk(KERN_ERR "%s: failed to setup irq, err=%d\n", __func__, err);
		BUG();
	}

	ent = proc_create("cycstc_test", 0666, NULL, &cycstc_test_proc_fops);
	if (!ent) {
		printk(KERN_ERR "Error: %s:%d", __func__, __LINE__);
		return -ENOENT;
	}

	return err;
}
late_initcall(cycstc_test_init);
#endif /* CONFIG_SP_TIMER_IRQ_LATENCY_TEST */

/***************************
 * Timer3
 ***************************/

static void sp_stc_timer3_start(struct clock_event_device *evt, unsigned long cycles, int periodic)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)sp_timer_base;
	u32 div;

	/* printk(KERN_INFO "%s: mode: %d, cycles = %u\n", __func__, (int)(evt->mode), (u32)(cycles)); */

	pstc_avReg->timer3_ctrl = 0;

	if (!periodic) {
		/*
		 * timer3_cnt only has 16 bits.
		 * When (cycles > (1 << 16)), timer3_divisor is required.
		 * => the precision is lower.
		 */
		div = (cycles >> 16) + 1;
		pstc_avReg->timer3_divisor = div - 1;
		/* a little larger than setting, make sure the ISR will see tasks' timeout as expected */
		pstc_avReg->timer3_reload = ((u32)(cycles) + div) / div - 1;

		pstc_avReg->timer3_cnt = pstc_avReg->timer3_reload;
		pstc_avReg->timer3_ctrl = SP_STC_AV_TIMER23_CTL_SRC_STC | SP_STC_AV_TIMER23_CTL_GO;
	} else { //if (evt->mode == CLOCK_EVT_MODE_PERIODIC) {
#if (SP_STC_FREQ == 1000000)
		pstc_avReg->timer3_divisor = 0;	/* CLKthis = CLKstc*/
		pstc_avReg->timer3_reload = SP_STC_FREQ/HZ - 1;
#elif (SP_STC_FREQ == 13500000)
		pstc_avReg->timer3_divisor = 3;	/* CLKthis = CLKstc / 4 */
		pstc_avReg->timer3_reload = (SP_STC_FREQ / HZ / 4) - 1;
#elif (SP_STC_FREQ == 270000000)
		pstc_avReg->timer3_divisor = 1000 - 1;	/* CLKthis = CLKstc / 1000 */
		pstc_avReg->timer3_reload = ((SP_STC_FREQ / 1000) / HZ) - 1;
#else
#error Unexpected STC frequency
#endif
		pstc_avReg->timer3_cnt = pstc_avReg->timer3_reload;
		pstc_avReg->timer3_ctrl = SP_STC_AV_TIMER23_CTL_SRC_STC | SP_STC_AV_TIMER23_CTL_RPT | SP_STC_AV_TIMER23_CTL_GO;
	}
}

static int sp_stc_timer3_event_set_next_event(unsigned long cycles, struct clock_event_device *evt)
{
	sp_stc_timer3_start(evt, cycles, 0);
	return 0;
}

static int sp_stc_timer3_set_oneshot(struct clock_event_device *evt)
{
	sp_stc_timer3_start(evt, ~0, 0);	/* max. cycle, should be updated by .set_next_event() */
	return 0;
}

static int sp_stc_timer3_set_period(struct clock_event_device *evt)
{
	sp_stc_timer3_start(evt, ~0, 1);	/* max. cycle, should be updated by .set_next_event() */
	return 0;
}

static int sp_stc_timer3_shutdown(struct clock_event_device *clkevt)
{
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)sp_timer_base;
	pstc_avReg->timer3_ctrl = 0;
	return 0;
}

static struct clock_event_device sp_clockevent_dev_timer3 = {
	.name		= "sp_stc_timer3_event",
	.features	= CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC,
	.rating		= 350,
	.set_next_event	= sp_stc_timer3_event_set_next_event,
	.set_state_periodic = sp_stc_timer3_set_period,
	.set_state_oneshot = sp_stc_timer3_set_oneshot,
	.set_state_shutdown = sp_stc_timer3_shutdown,
};

static irqreturn_t sp_stc_timer3_isr(int irq, void *dev_id)
{
	struct clock_event_device *evt = &sp_clockevent_dev_timer3;

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction sp_stc_timer3_irq = {
	.name		= "sp_stc_timer3",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL | IRQF_TRIGGER_RISING,
	.handler	= sp_stc_timer3_isr,
	.dev_id		= &sp_clockevent_dev_timer3,
};

static void sp_clockevent_init(void)
{
	int ret;
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)sp_timer_base;
	int bc_late = 0;

	pstc_avReg->timer3_ctrl = 0;

	sp_stc_timer3_irq.irq = sp_timer_irqs[SP_TIMER_STC_TIMER3];
	BUG_ON(!sp_stc_timer3_irq.irq);

	sp_clockevent_dev_timer3.irq = sp_stc_timer3_irq.irq;
	if (irq_can_set_affinity(sp_clockevent_dev_timer3.irq)) {
		sp_clockevent_dev_timer3.cpumask = cpu_possible_mask;
	} else {
		sp_clockevent_dev_timer3.cpumask = cpumask_of(0);
		bc_late = 1;
	}

#if (SP_STC_FREQ == 1000000)
	clockevents_config_and_register(&sp_clockevent_dev_timer3, SP_STC_FREQ, 1, ((1 << 16) - 1));
#elif (SP_STC_FREQ == 13500000)
	/* 28 bits > 16 bits counter => timer3_divisor is required. */
	clockevents_config_and_register(&sp_clockevent_dev_timer3, SP_STC_FREQ, 1, ((1 << 28) - 1));
#elif (SP_STC_FREQ == 270000000)
	/* 28 bits > 16 bits counter => timer3_divisor is required. */
	clockevents_config_and_register(&sp_clockevent_dev_timer3, SP_STC_FREQ, 1, ((1 << 28) - 1));
#else
#error Unexpected STC frequency
#endif

	if (bc_late)
		sp_clockevent_dev_timer3.cpumask = cpu_possible_mask;

	ret = setup_irq(sp_stc_timer3_irq.irq, &sp_stc_timer3_irq);
	if (ret) {
		printk(KERN_ERR "%s, %d: reg = %d\n", __FILE__, __LINE__, ret);
		BUG();
	}
}

static u64 sp_clocksource_hz_read(struct clocksource *cs)
{
	return sp_read_sched_clock();
}

static void sp_clocksource_resume(struct clocksource *cs)
{
	sp_stc_hw_init(1);
}

struct clocksource sp_clocksource_hz = {
	.name	= "sp_clocksource_hz",
#if (SP_STC_FREQ == 1000000)
	.rating	= 250,
#elif (SP_STC_FREQ == 13500000)
	.rating	= 350,
#elif (SP_STC_FREQ == 270000000)
	.rating	= 350,
#else
#error Unexpected STC frequency
#endif
	.read	= sp_clocksource_hz_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.resume	= sp_clocksource_resume,
};

static void __init sp_clocksource_init(void)
{
	u64 (*func_ptr)(void);

	func_ptr = sp_read_sched_clock;
	sp_stc_hw_init(1);
	sched_clock_register(func_ptr, 32, SP_STC_FREQ);

	if (clocksource_register_hz(&sp_clocksource_hz, SP_STC_FREQ))
		panic("%s: can't register clocksource\n", sp_clocksource_hz.name);
}

void __init sp_timer_init(void)
{
	sp_clocksource_init();
	sp_clockevent_init();
}

static int __init sp_timer_init_of(struct device_node *np)
{
	u32 nr_irqs, i;

	nr_irqs = of_irq_count(np);

	for (i = SP_TIMER_STC_TIMER0; i < nr_irqs; i++) {
		sp_timer_irqs[i] = irq_of_parse_and_map(np, i);
		WARN(!sp_timer_irqs[i], "%s: no IRQ for timer%d", __func__, i);
	}

	sp_timer_base = of_iomap(np, 0);
	if (!sp_timer_base)
		panic("%s: Could not ioremap system timer base\n", __func__);

	sp_clocksource_init();
	sp_clockevent_init();

	return 0;
}

TIMER_OF_DECLARE(sp_timer, "sunplus,sp-stc", sp_timer_init_of);
