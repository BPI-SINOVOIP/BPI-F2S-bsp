#ifndef __PWM_SP7021_H__
#define __PWM_SP7021_H__

#define STATIC_ASSERT(b) extern int _static_assert[b ? 1 : -1]

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct _PWM_DD_REG_ {
	union {
		struct {
			u32 dd					:16;//b(0-15)
			u32 :16;
		};
		u32 idx_all;
	};
};
STATIC_ASSERT(sizeof(struct _PWM_DD_REG_) == 4);

struct _PWM_DU_REG_ {
	union {
		struct {
			u32 pwm_du				:8;	//b(0-7)
			u32 pwm_du_dd_sel		:2;	//b(8-9)
			u32						:6;	//b(10-15)
			u32 :16;
		};
		u32 idx_all;
	};
};
STATIC_ASSERT(sizeof(struct _PWM_DU_REG_) == 4);

struct _PWM_REG_ {
	//GROUP 244
	union {
		struct {
			u32 pwm_en				:8;	//b(0-7)
			u32 pwm_bypass			:8;	//b(8-15)
			u32 :16;
		};
		u32 grp244_0;
	};
	union {
		struct {
			u32 pwm_cnt0_en			:1;	//b(0)
			u32 pwm_cnt1_en			:1;	//b(1)
			u32 pwm_cnt2_en			:1;	//b(2)
			u32 pwm_cnt3_en			:1;	//b(3)
			u32 pwm_clk54_en		:1;	//b(4)
			u32						:3;	//b(5-7)
			u32 pwm_dd0_sync_off	:1;	//b(8)
			u32 pwm_dd1_sync_off	:1;	//b(9)
			u32 pwm_dd2_sync_off	:1;	//b(10)
			u32 pwm_dd3_sync_off	:1;	//b(11)
			u32						:4;	//b(12-15)
			u32 :16;
		};
		u32 grp244_1;
	};
	struct _PWM_DD_REG_ pwm_dd[4];		//G244.2~5
	struct _PWM_DU_REG_ pwm_du[8];		//G244.6~13
	u32 grp244_14_31[18];
};
STATIC_ASSERT(sizeof(struct _PWM_REG_) == (32 * 4));

#endif	/*__PWM_SP7021_H__ */

