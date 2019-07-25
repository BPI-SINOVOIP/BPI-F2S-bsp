#ifndef __REG_IOP_H__
#define __REG_IOP_H__

#include <mach/io_map.h>

typedef struct regs_iop_moon0_t_ {
	volatile unsigned int stamp;         /* 00 */
	volatile unsigned int clken[10];     /* 01~10 */
	volatile unsigned int gclken[10];    /* 11~20 */
	volatile unsigned int reset[10];     /* 21~30 */
	volatile unsigned int sfg_cfg_mode;  /* 31 */
} regs_iop_moon0_t;



typedef	struct regs_iop_t_{
	volatile unsigned int iop_control;                /* 00 */
	volatile unsigned int iop_reg1;                /* 01 */
	volatile unsigned int iop_bp;                /* 02 */
	volatile unsigned int iop_regsel;                  /* 03 */
	volatile unsigned int iop_regout;                  /* 04 */
	volatile unsigned int iop_reg5;                  /* 05 */
	volatile unsigned int iop_resume_pcl;                    /* 06 */
	volatile unsigned int iop_resume_pch;            /* 07 */
	volatile unsigned int iop_data0;               /* 08 */
	volatile unsigned int iop_data1;               /* 09 */
	volatile unsigned int iop_data2;              /* 10 */
	volatile unsigned int iop_data3;             /* 11 */
	volatile unsigned int iop_data4;             /* 12 */
	volatile unsigned int iop_data5;               /* 13 */
	volatile unsigned int iop_data6;                 /* 14 */
	volatile unsigned int iop_data7;                /* 15 */
	volatile unsigned int iop_data8;                 /* 16 */
	volatile unsigned int iop_data9;                 /* 17 */
	volatile unsigned int iop_data10;         /* 18 */
	volatile unsigned int iop_data11;        /* 19 */
	volatile unsigned int iop_base_adr_l;           /* 20 */
	volatile unsigned int iop_base_adr_h;          /* 21 */
	volatile unsigned int Memory_bridge_control;           /* 22 */
	volatile unsigned int iop_regmap_adr_l;           /* 23 */
	volatile unsigned int iop_regmap_adr_h;  /* 24 */
	volatile unsigned int iop_direct_adr;              /* 25*/
	volatile unsigned int reserved[6];              /* 26~31 */
}regs_iop_t;


typedef	struct regs_iop_qctl_t_{
	volatile unsigned int PD_GRP0;                	/* 00 */
	volatile unsigned int PD_GRP1;                	/* 01 */
	volatile unsigned int PD_GRP2;                	/* 02 */
	volatile unsigned int PD_GRP3;                  /* 03 */
	volatile unsigned int PD_GRP4;                  /* 04 */
	volatile unsigned int PD_GRP5;                  /* 05 */
	volatile unsigned int PD_GRP6;                  /* 06 */
	volatile unsigned int PD_GRP7;            	/* 07 */
	volatile unsigned int PD_GRP8;               	/* 08 */
	volatile unsigned int PD_GRP9;               	/* 09 */
	volatile unsigned int PD_GRP10;              	/* 10 */
	volatile unsigned int PD_GRP11;             	/* 11 */
	volatile unsigned int reserved[20];              /* 12~31 */
}regs_iop_qctl_t;

typedef	struct regs_iop_pmc_t_{
	volatile unsigned int PMC_TIMER;                				/* 00 */
	volatile unsigned int PMC_CTRL;                				/* 01 */
	volatile unsigned int XTAL27M_PASSWORD_I;                	/* 02 */
	volatile unsigned int XTAL27M_PASSWORD_II;                  	/* 03 */
	volatile unsigned int XTAL32K_PASSWORD_I;                  	/* 04 */
	volatile unsigned int XTAL32K_PASSWORD_II;                  	/* 05 */
	volatile unsigned int CLK27M_PASSWORD_I;                  	/* 06 */
	volatile unsigned int CLK27M_PASSWORD_II;            		/* 07 */
	volatile unsigned int PMC_TIMER2;               				/* 08 */
	volatile unsigned int reserved[23];              				/* 9~31 */
}regs_iop_pmc_t;

typedef	struct regs_iop_rtc_t_{
	volatile unsigned int reserved1[16];                				/* 00~15 */
	volatile unsigned int rtc_ctrl;                					/* 16 */
	volatile unsigned int rtc_timer_out;                				/* 17 */
	volatile unsigned int rtc_divider_cnt_out;                  		/* 18 */
	volatile unsigned int rtc_timer_set;                  			/* 19 */
	volatile unsigned int rtc_alarm_set;                  			/* 20 */
	volatile unsigned int rtc_user_data;                  			/* 21 */
	volatile unsigned int rtc_reset_record;            			/* 22 */
	volatile unsigned int rtc_batt_charge_ctrl;                  		/* 23 */
	volatile unsigned int rtc_trim_ctrl;            				/* 24 */
	volatile unsigned int rtc_otp_ctrl;                  				/* 25 */
	volatile unsigned int reserved2[5];              				/* 26~30 */
	volatile unsigned int rtc_ip_version;            				/* 31 */
}regs_iop_rtc_t;
#endif /* __REG_IOP_H__ */
