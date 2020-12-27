#ifndef __PWM_I143_H__
#define __PWM_I143_H__

#define STATIC_ASSERT(b) extern int _static_assert[b ? 1 : -1]

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct _PWM_DD_REG_ {
	union {
		struct {
			u32 dd					:18;//b(0-17)
			u32 					:14;//b(18-31)
		};
		u32 idx_all;
	};
};
STATIC_ASSERT(sizeof(struct _PWM_DD_REG_) == 4);

struct _PWM_DU_REG_ {
	union {
		struct {
			u32 pwm_du_0			:8;	//b(0-7)
			u32 pwm_du_dd_sel_0		:2;	//b(8-9)
			u32						:6;	//b(10-15)
			u32 pwm_du_1			:8;	//b(16-23)
			u32 pwm_du_dd_sel_1		:2;	//b(24-25)
			u32						:6;	//b(26-31)
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
			u32 pwm_pro_en			:4; //(bit16-19)
			u32 pwm_sync_off		:4; //(bit20-23)
			u32 pwm_latch_mode		:1; //(bit24)
			u32 pwm_sync_start_mode	:1; //(bit25)
			u32 					:6; //(bit26-31)
		};
		u32 grp244_0;
	};
	u32 grp244_1_2[2];					//G244.1~2
	struct _PWM_DU_REG_ pwm_du[4];		//G244.3~6
	u32 grp244_7_14[8];					//G244.7 ~ 14 TBD
	u32 grp244_15[1];					//G244.15 TBD
	u32 pwm_version[1];					//G244.16 PWM VERSION
	struct _PWM_DD_REG_ pwm_dd[4];		//G244.17~20
	u32 grp244_21_31[11];				//G244.21~31

	//GROUP 245
	union {
		struct {
			u32 pwm_pro_end_ini_mask	:4;	//b(0-3)
			u32 pwm_pro_user_ini_mask	:4;	//b(4-7)
			u32	pwm_inv					:8;	//b(8-15)
			u32 pwm_pro_send			:4;	//b(16-19)
			u32 pwm_pro_reset			:4;	//b(20-23)
			u32							:8;	//b(24-31)
		};
		u32 grp245_0;
	};
	u32 grp245_1_12[12];				//G245.1~12 pwm_pro_config TBD
	u32 grp245_13[1];					//G245.13 pwm_mode2 TBD
	u32 grp245_14_29[16];				//G245.14~29 pwm_adj_config TBD
	u32 grp245_30_31[2];				//G245.30~31 reserved
};
STATIC_ASSERT(sizeof(struct _PWM_REG_) == (32 * 4 * 2));

#endif	/*__PWM_I143_H__ */

