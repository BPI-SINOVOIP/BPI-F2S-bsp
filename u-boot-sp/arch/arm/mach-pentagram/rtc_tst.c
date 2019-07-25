/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

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


static int do_rtc_tst(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	struct sp_rtc_reg *sp_rtc_ptr = (struct sp_rtc_reg *)(0x9C000000 + (116 << 7));

	printf("%s, %d\n", __func__, __LINE__);

	sp_rtc_ptr->rtc_timer_set = 0x12345678;
	sp_rtc_ptr->rtc_alarm_set = sp_rtc_ptr->rtc_timer_set + 2;

	sp_rtc_ptr->rtc_user_data = (0xFFFF << 16) | 0xabcd;
	sp_rtc_ptr->rtc_ctrl = (0x003F << 16) | 0x0017;

	for (i = 0;; i++) {
		if ((i & 0x0000ffff) == 0) {	/* reduce output message */
			printf("rtc_timer_out: 0x%x\n", sp_rtc_ptr->rtc_timer_out);
			printf("rtc_divider: 0x%x\n", sp_rtc_ptr->rtc_divider);
			printf("rtc_user_data: 0x%x\n", sp_rtc_ptr->rtc_user_data);
			printf("rtc_reset_record: 0x%x\n", sp_rtc_ptr->rtc_reset_record);
			printf("\n\n");
		}
	}

	printf("%s, %d\n", __func__, __LINE__);
	return 0;
}

U_BOOT_CMD(
	rtc_tst,	CONFIG_SYS_MAXARGS,	1,	do_rtc_tst,
	"Test for RTC module",
	"Test for RTC module\n"
	"rtc_tst command ...\n"
);

