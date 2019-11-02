/*
 * (C) Copyright 2014
 * Sunplus Technology
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>

struct stc_regs {
        unsigned int stc_15_0;       // 12.0
        unsigned int stc_31_16;      // 12.1
        unsigned int stc_64;         // 12.2
        unsigned int stc_divisor;    // 12.3
        unsigned int rtc_15_0;       // 12.4
        unsigned int rtc_23_16;      // 12.5
        unsigned int rtc_divisor;    // 12.6
        unsigned int stc_config;     // 12.7
        unsigned int timer0_ctrl;    // 12.8
        unsigned int timer0_cnt;     // 12.9
        unsigned int timer1_ctrl;    // 12.10
        unsigned int timer1_cnt;     // 12.11
        unsigned int timerw_ctrl;    // 12.12
        unsigned int timerw_cnt;     // 12.13
        unsigned int stc_47_32;      // 12.14
        unsigned int stc_63_48;      // 12.15
        unsigned int timer2_ctl;     // 12.16
        unsigned int timer2_pres_val;// 12.17
        unsigned int timer2_reload;  // 12.18
        unsigned int timer2_cnt;     // 12.19
        unsigned int timer3_ctl;     // 12.20
        unsigned int timer3_pres_val;// 12.21
        unsigned int timer3_reload;  // 12.22
        unsigned int timer3_cnt;     // 12.23
        unsigned int stcl_0;         // 12.24
        unsigned int stcl_1;         // 12.25
        unsigned int stcl_2;         // 12.26
        unsigned int atc_0;          // 12.27
        unsigned int atc_1;          // 12.28
        unsigned int atc_2;          // 12.29
};

#define SPHE_DEVICE_BASE       0x9C000000
#define RF_GRP(_grp, _reg)     ((((_grp)*32 + (_reg))*4) + SPHE_DEVICE_BASE)

#define STC_REG     ((volatile struct stc_regs *)RF_GRP(12, 0))
#define STC_AV0_REG ((volatile struct stc_regs *)RF_GRP(96, 0))
#define STC_AV2_REG ((volatile struct stc_regs *)RF_GRP(99, 0))

DECLARE_GLOBAL_DATA_PTR;

static volatile struct stc_regs *g_regs = STC_AV2_REG;

#undef USE_EXT_CLK
/* 
 * TRIGGER_CLOCK is timer's runtime frequency. We expect it to be 1MHz.
 * TRIGGER_CLOCK = SOURCE_CLOCK / ([13:0] of 12.3 + 1).
 */
#define SP_STC_TRIGGER_CLOCK	1000000
#ifdef USE_EXT_CLK
#define SP_STC_SOURCE_CLOCK	13500000	/* Use div_ext_clk, it is 13.5 MHz. */
#else
#define SP_STC_SOURCE_CLOCK	202000000	/* Use sysclk, it is 202 MHz. */
#endif

ulong get_timer_masked(void)
{
	ulong freq = gd->arch.timer_rate_hz;
	ulong secs = 0xffffffff / freq;
	ulong tick;

	writel(0x1234, &g_regs->stcl_2); /* 99.26 stcl 2, write anything to latch */
	tick = (readl(&g_regs->stcl_1) << 16) | readl(&g_regs->stcl_0);

	if (tick >= secs * freq) {
		tick = 0;
		gd->arch.tbl += secs * CONFIG_SYS_HZ;
		writel(0, &g_regs->stc_15_0);
		writel(0, &g_regs->stc_31_16);
		writel(0, &g_regs->stc_64);
	}

	return gd->arch.tbl + (tick / (freq / CONFIG_SYS_HZ));

	return 0;
}

ulong get_timer(ulong base)
{
        return get_timer_masked() - base;
}

void reset_timer_masked(void)
{
	writel(0, &g_regs->stc_64);
}

void reset_timer(void)
{
        reset_timer_masked();
}

int timer_init(void)
{
	/* 
	 * refer to 12.3 STC pre-scaling Register (stc divisor)
	 * trigger clock = sysClk or divExtClk / ([13:0] of 12.3 + 1)
	 * so 12.3[13:0] = (sysClk or divExtClk / trigger clcok) - 1
	 * 12.3[15] = 1 for div_ext_clk
	 */
#ifdef USE_EXT_CLK
	g_regs->stc_divisor = (1 << 15) | ((SP_STC_SOURCE_CLOCK / SP_STC_TRIGGER_CLOCK) - 1);
#else  /* Use sysclk */
	g_regs->stc_divisor = (0 << 15) | ((SP_STC_SOURCE_CLOCK / SP_STC_TRIGGER_CLOCK) - 1);
#endif
	gd->arch.timer_rate_hz = SP_STC_TRIGGER_CLOCK;

	gd->arch.tbl = 0;
	reset_timer_masked();

        return 0;
}

void udelay_masked(unsigned long usec)
{
        ulong freq = gd->arch.timer_rate_hz;
        ulong secs = 0xffffffff / freq;
        ulong wait, tick, timeout;

	/* 
	 * how many cycle should be count
	 * When freq is 10K-1M, use the second rule or it will be always 0.
	 */
        wait = (freq >= 1000000) ? usec * (freq / 1000000) :
               (usec * (freq / 10000)) / 100;

        if (!wait) {
                wait = 1;
        }

        writel(0x1234, &g_regs->stcl_2); /* 99.26 stcl 2, write anything to latch */
        tick = (readl(&g_regs->stcl_1) << 16) | readl(&g_regs->stcl_0);

        /* restart timer if counter is going to overflow */
        if (secs * freq - tick < wait) {
                gd->arch.tbl += tick / (freq / CONFIG_SYS_HZ);
                writel(0, &g_regs->stc_15_0);
                writel(0, &g_regs->stc_31_16);
                writel(0, &g_regs->stc_64);
                tick = 0;
        }

        /* now we wait ... */
        timeout = tick + wait;
        do {
                writel(0x1234, &g_regs->stcl_2);
                tick = (readl(&g_regs->stcl_1) << 16) | readl(&g_regs->stcl_0);
        } while (timeout > tick);
}

void __udelay(unsigned long usec)
{
	udelay_masked(usec);
}

unsigned long long get_ticks(void)
{
	unsigned long long ret;

	writel(0x1234, &g_regs->stcl_2); /* 99.26 stcl 2, write anything to latch */
	ret = (readl(&g_regs->stcl_1) << 16) | readl(&g_regs->stcl_0);

	if (readl(&g_regs->stcl_2) & 1)
		ret |= 0x100000000ull;

	return ret;
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return gd->arch.timer_rate_hz;
}
