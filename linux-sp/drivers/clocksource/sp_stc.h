#ifndef __SP_STC_H__
#define __SP_STC_H__

#include <mach/io_map.h>

#if 0 // moved to DT
#define LOGI_ADDR_INT_CTRL_G0_REG		(VA_IOB_ADDR(0) +  9 * 32 * 4)
#define LOGI_ADDR_INT_CTRL_G1_REG		(VA_IOB_ADDR(0) + 10 * 32 * 4)
#define LOGI_ADDR_STC_REG			(VA_IOB_ADDR(0) + 12 * 32 * 4)
#define LOGI_ADDR_STC_AV1_REG			(VA_IOB_ADDR(0) + 97 * 32 * 4)
#define LOGI_ADDR_STC_AV2_REG			(VA_IOB_ADDR(0) + 99 * 32 * 4)
#endif

typedef struct stcReg_s {
	/* Group 12: STC */
	volatile unsigned int stc_15_0;
	volatile unsigned int stc_31_16;
	volatile unsigned int stc_32;
	volatile unsigned int stc_divisor;
	volatile unsigned int rtc_15_0;
	volatile unsigned int rtc_23_16;
	volatile unsigned int rtc_divisor;
	volatile unsigned int stc_config;
	volatile unsigned int timer0_ctrl;
	volatile unsigned int timer0_cnt;
	volatile unsigned int timer1_ctrl;
	volatile unsigned int timer1_cnt;
	volatile unsigned int timerw_ctrl;
	volatile unsigned int timerw_cnt;
	volatile unsigned int g12_reserved_0[2];
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
	volatile unsigned int g12_reserved_1[2];
} stcReg_t;

typedef struct stc_avReg_s {
	/* Group 96, 97, 99: STC_AV0 - STC_AV2 */
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
	volatile unsigned int rsv_12;
	volatile unsigned int rsv_13;
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
	volatile unsigned int rsv_30;
	volatile unsigned int rsv_31;
} stc_avReg_t;

#define SP_STC_TIMER0CTL_SRC_STC		(1 << 14)
#define SP_STC_TIMER0CTL_SRC_RTC		(2 << 14)
#define SP_STC_TIMER0CTL_RTP			(1 << 13)
#define SP_STC_TIMER0CTL_GO			(1 << 11)
#define SP_STC_TIMER0CTL_TM0_RELOAD_MASK	(0x07FF << 0)

#define SP_STC_AV_TIMER01_CTL_SRC_SYS		(0 << 14)
#define SP_STC_AV_TIMER01_CTL_SRC_STC		(1 << 14)
#define SP_STC_AV_TIMER01_CTL_SRC_RTC		(2 << 14)
#define SP_STC_AV_TIMER01_CTL_RTP		(1 << 13)
#define SP_STC_AV_TIMER01_CTL_GO		(1 << 11)
#define SP_STC_AV_TIMER01_CTL_RELOAD_MASK	(0x07FF << 0)

#define SP_STC_AV_TIMER23_CTL_SRC_SYS		(0 << 2)
#define SP_STC_AV_TIMER23_CTL_SRC_STC		(1 << 2)
#define SP_STC_AV_TIMER23_CTL_SRC_EXT		(3 << 2)
#define SP_STC_AV_TIMER23_CTL_RPT		(1 << 1)
#define SP_STC_AV_TIMER23_CTL_GO		(1 << 0)

#endif /* __SP_STC_H__ */
