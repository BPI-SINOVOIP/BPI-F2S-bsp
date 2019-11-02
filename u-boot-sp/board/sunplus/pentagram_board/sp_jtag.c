 /*
  * (C) Copyright 2015
  * Sunplus Technology. <http://www.sunplus.com/>
  *
  * SPDX-License-Identifier:     GPL-2.0+
  */
#include <common.h>
#include <command.h>
#include <linux/compiler.h>

#define RGST_OFFSET_A	0x9EC00000
#define RGST_OFFSET_B	0x9C000000

enum {
	CHIP_A = 0,
	CHIP_B,
};

#define RF_GRP_A(_grp, _reg)	((((_grp) * 32 + (_reg)) * 4) + RGST_OFFSET_A)
#define RF_GRP_B(_grp, _reg)	((((_grp) * 32 + (_reg)) * 4) + RGST_OFFSET_B)
#define RF_MASK_V(_mask, _val)	(((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask)	(((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask)	(((_mask) << 16) | 0)

struct moon1_regs {
	unsigned int sft_cfg[32];
};
#define MOON1_REG_B		((volatile struct moon1_regs *)RF_GRP_B(1, 0))

struct cfgdbg_regs {
	unsigned int cfg_dbg_ctrl;
};
#define CFGDBG_CA7_REG_B	((volatile struct cfgdbg_regs *)RF_GRP_B(83, 20))
#define CFGDBG_926_REG_B	((volatile struct cfgdbg_regs *)RF_GRP_B(83, 21))

/* Write 4 bytes to a specific register in a certain group */
void setup_pinmux(unsigned int chip)
{
	if (chip == CHIP_A) {
		printf("setup chip A SWD pinmux\n");
		/*
		 * Setup pinmux
		 * Need to turn these off :
		 *  sft_g1cfg1[1:0], sft_g1cfg1[3:2]
		 * Need to turn these on (as X1):
		 *  sft_g1cfg1[9:8]
		 */
		MOON1_REG_B->sft_cfg[1] = RF_MASK_V(0x030f, 0x0100);
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&MOON1_REG_B->sft_cfg[1],
			MOON1_REG_B->sft_cfg[1]);
		/* 83.20 Configure debug control of CPU (cpu debug ctrl) */
		CFGDBG_CA7_REG_B->cfg_dbg_ctrl = (0x01f);
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&CFGDBG_CA7_REG_B->cfg_dbg_ctrl,
			CFGDBG_CA7_REG_B->cfg_dbg_ctrl);
	} else if (chip == CHIP_B) {
		printf("setup chip B JTAG pinmux\n");
		/*
		 * Setup pinmux
		 * Need to turn these off :
		 *  sft_g1cfg1[1:0], sft_g1cfg1[3:2], sft_g1cfg1[9:8], sft_g1cfg1[14:13]
		 *  sft_g1cfg3[4], sft_g1cfg3[5], sft_g1cfg3[6], sft_g1cfg3[15:14]
		 * Need to turn these on (as X1):
		 *  sft_g1cfg4[1:0]
		 */
		MOON1_REG_B->sft_cfg[1] = RF_MASK_V_CLR(0x0f | 0x03 << 8 | 0x03 << 13);
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&MOON1_REG_B->sft_cfg[1],
			MOON1_REG_B->sft_cfg[1]);
		MOON1_REG_B->sft_cfg[3] = RF_MASK_V_CLR(0x07 << 4 | 0x03 << 14);
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&MOON1_REG_B->sft_cfg[3],
			MOON1_REG_B->sft_cfg[3]);
		MOON1_REG_B->sft_cfg[4] = RF_MASK_V_SET(0x1);
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&MOON1_REG_B->sft_cfg[4],
			MOON1_REG_B->sft_cfg[4]);
		/* 83.21 Configure debug control of A926 (a926 debug ctrl) */
		CFGDBG_926_REG_B->cfg_dbg_ctrl = (0x01 | (0x01 << 4));
		printf("reg 0x%08x set as 0x%08x\n", (unsigned int)&CFGDBG_926_REG_B->cfg_dbg_ctrl,
			CFGDBG_926_REG_B->cfg_dbg_ctrl);
	}

	return;
}

/* Setup JTAG environment
 *
 * Syntax:
 *	jtag <no parameter>
 */
static int do_jtag(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 2) {
		unsigned int chip = -1;

		if (strncmp(argv[1], "a", 1) == 0)
			chip = CHIP_A;
		else if (strncmp(argv[1], "b", 1) == 0)
			chip = CHIP_B;
		else {
			printf("Invalid chip. It should be be \'a\' or \'b\'.\n");
			return CMD_RET_USAGE;
		}
		setup_pinmux(chip);
	} else
		return CMD_RET_USAGE;

	return 0;
}

/**************************************************/
U_BOOT_CMD(
	jtag,	3,	1,	do_jtag,
	"Setup JTAG environment",
	"[a|b]"
);
