#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

/* Group 96, 97, 99: STC_AV0 - STC_AV2 */
typedef struct {
	volatile unsigned int stc_15_0;
	volatile unsigned int stc_31_16;
	volatile unsigned int stc_64;
	volatile unsigned int stc_divisor;
	volatile unsigned int rtc_15_0;
	volatile unsigned int rtc_23_16;
	volatile unsigned int rtc_divisor;
	volatile unsigned int stc_config;
	volatile unsigned int timer0_ctrl;
	volatile unsigned int timer0_cnt;
	volatile unsigned int timer1_ctrl;
	volatile unsigned int timer1_cnt;
	volatile unsigned int timerw_ctrl;	/* Only STCs @ 0x9C000600 and 0x9C003000 */
	volatile unsigned int timerw_cnt;	/* Only STCs @ 0x9C000600 and 0x9C003000 */
	volatile unsigned int stc_47_32;
	volatile unsigned int stc_63_48;
	volatile unsigned int timer2_ctrl;
	volatile unsigned int timer2_divisor;
	volatile unsigned int timer2_reload;
	volatile unsigned int timer2_cnt;
	volatile unsigned int timer3_ctrl;
	volatile unsigned int timer3_divisor;
	volatile unsigned int timer3_reload;
	volatile unsigned int timer3_cnt;
	volatile unsigned int stcl_0;
	volatile unsigned int stcl_1;
	volatile unsigned int stcl_2;
	volatile unsigned int atc_0;
	volatile unsigned int atc_1;
	volatile unsigned int atc_2;
	volatile unsigned int timer0_reload;
	volatile unsigned int timer1_reload;
} stc_avReg_t;

#define PENTAGRAM_BASE_ADDR	(0x9C000000)
#define PENTAGRAM_MOON0		(PENTAGRAM_BASE_ADDR + (0 << 7))
#define PENTAGRAM_MOON4		(PENTAGRAM_BASE_ADDR + (4 << 7))
#define PENTAGRAM_WDTMR_ADDR	(PENTAGRAM_BASE_ADDR + (12 << 7))	/* Either Group 12 or 96 */
#define PENTAGRAM_TIMER_ADDR	(PENTAGRAM_BASE_ADDR + (99 << 7))
#define PENTAGRAM_RTC_ADDR	(PENTAGRAM_BASE_ADDR + (116 << 7))
#define PENTAGRAM_OTP_ADDR	(PENTAGRAM_BASE_ADDR + (350<<7))

#define WATCHDOG_CMD_CNT_WR_UNLOCK	0xAB00
#define WATCHDOG_CMD_CNT_WR_LOCK	0xAB01
#define WATCHDOG_CMD_CNT_WR_MAX		0xDEAF
#define WATCHDOG_CMD_PAUSE		0x3877
#define WATCHDOG_CMD_RESUME		0x4A4B
#define WATCHDOG_CMD_INTR_CLR		0x7482

void s_init(void)
{
	/* Init watchdog timer, ... */
	/* TODO: Setup timer used by U-Boot, required to change on I-139 */
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(PENTAGRAM_TIMER_ADDR);

#if (CONFIG_SYS_HZ != 1000)
#error "CONFIG_SYS_HZ != 1000"
#else
	/*
	 * Clock @ 27 MHz, but stc_divisor has only 14 bits
	 * Min STC clock: 270000000 / (1 << 14) = 16479.4921875
	 */
	pstc_avReg->stc_divisor = ((270000000 + (1000 * 32) / 2) / (1000 * 32)) - 1 ; /* = 0x20F5, less then 14 bits */
#endif
}

unsigned long notrace timer_read_counter(void)
{
	unsigned long value;
	stc_avReg_t *pstc_avReg = (stc_avReg_t *)(PENTAGRAM_TIMER_ADDR);

	pstc_avReg->stcl_2 = 0; /* latch */
	value  = (unsigned long)(pstc_avReg->stcl_2);
	value  = value << 16;
	value |= (unsigned long)(pstc_avReg->stcl_1);
	value  = value << 16;
	value |= (unsigned long)(pstc_avReg->stcl_0);
	value = value >> 5; /* divided by 32 => (1000 * 32)/32 = 1000*/
	return value;
}

void reset_cpu(ulong ignored)
{
	volatile unsigned int *ptr;
#if 0
	stc_avReg_t *pstc_avReg;
#endif

	puts("System is going to reboot ...\n");

	/*
	 * Enable all methods (in Grp(4, 29)) to cause chip reset:
	 * Bit [4:1]
	 */
	ptr = (volatile unsigned int *)(PENTAGRAM_MOON4 + (29 << 2));
	*ptr = (0x001E << 16) | 0x001E;

#if 0
	/* Watchdogs used by RISC */
	pstc_avReg = (stc_avReg_t *)(PENTAGRAM_WDTMR_ADDR);
	pstc_avReg->stc_divisor = (0x0100 - 1);
	pstc_avReg->stc_64 = 0;		/* reset STC */
	pstc_avReg->timerw_ctrl = WATCHDOG_CMD_CNT_WR_UNLOCK;
	pstc_avReg->timerw_ctrl = WATCHDOG_CMD_PAUSE;
	pstc_avReg->timerw_cnt = 0x10 - 1;
	pstc_avReg->timerw_ctrl = WATCHDOG_CMD_RESUME;
	pstc_avReg->timerw_ctrl = WATCHDOG_CMD_CNT_WR_LOCK;
#else
	/* System reset */
	ptr = (volatile unsigned int *)(PENTAGRAM_MOON0 + (21 << 2));
	*ptr = (0x0001 << 16) | 0x0001;
#endif

	while (1) {
		/* wait for reset */
	}

	/*
	 * Note: When using Zebu's zmem, chip can't be reset correctly because part of the loaded memory is destroyed.
	 * 	 => Use eMMC for this test.
	 */
}
#ifdef CONFIG_BOOTARGS_WITH_MEM
// get dram size by otp
int dram_get_size(void)
{
	volatile unsigned int *ptr;
	ptr = (volatile unsigned int *)(PENTAGRAM_OTP_ADDR + (7 << 2));//G[350.7]
	int dramsize_Flag = ((*ptr)>>16)&0x03;
	int dramsize;
	switch(dramsize_Flag)
	{
		case 0x00:
			dramsize = 64<<20;
			break;
		case 0x01:
			dramsize = 128<<20;
			break;
		case 0x02:
			dramsize = 256<<20;
			break;
		case 0x03:
			dramsize = 512<<20;
			break;
		default:
			dramsize = 512<<20;
	}
	printf("dram size is %dM\n",dramsize>>20);
	return dramsize;
}
#endif
int dram_init(void)
{
#ifdef CONFIG_BOOTARGS_WITH_MEM
	int dramsize = dram_get_size();
	gd->ram_size = dramsize;
#else
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
#endif
	return 0;
}

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	printf("CONFIG_SYS_CACHELINE_SIZE: %d\n", CONFIG_SYS_CACHELINE_SIZE);
	return 0;
}
#endif

#ifdef CONFIG_ARCH_MISC_INIT
int arch_misc_init(void)
{
	volatile unsigned int *ptr;

	ptr = (volatile unsigned int *)(PENTAGRAM_RTC_ADDR + (22 << 2));
	printf("\nReason(s) of reset: REG(116, 22): 0x%04x\n", *ptr);
	*ptr = 0xFFFF0000;
	printf("\nAfter cleaning  REG(116, 22): 0x%04x\n\n", *ptr);

	printf("%s, %s: TBD.\n", __FILE__, __func__);
	return 0;
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif
