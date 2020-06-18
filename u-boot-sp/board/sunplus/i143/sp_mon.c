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
#define RGST_OFFSET_B   0x9C000000

#define ENTER		0x0D
#define SPACE		0x20
#define BCKSPACE	0x7F
#define	IS_DIGIT(c)		((c) >= '0' && (c) <= '9')
#define	IS_LETTER_LOW(c)	((c) >= 'a' && (c) <= 'z')
#define	IS_LETTER_UP(c)		((c) >= 'A' && (c) <= 'Z')
#define IS_HEX(c)		(((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'a' && (c) <= 'f'))
#define TO_INT(x)		((unsigned int)(x - '0'))
#define ASCII_PRINTABLE(c)	((c >= ' ') && (c <= '~'))
#define LTR_LOW_TO_INT(c)	((unsigned int)(c - 'a' + 10))
#define LTR_UP_TO_INT(c)	((unsigned int)(c - 'A' + 10))
#define DUMP_CHARS_PER_LINE 16
/* Dump / Fill format */
#define MEM_FORMAT_BYTE		(sizeof(unsigned char))
#define MEM_FORMAT_WORD		(sizeof(unsigned short))
#define MEM_FORMAT_DWORD	(sizeof(unsigned int))

#define dbg_printf	printf

/* Print specific group of registers */
void prn_regs(unsigned int regGroupNum, unsigned int rgst_offset)
{
	unsigned int i = 0;
	unsigned int regBaseAddr = 0;

	regBaseAddr = rgst_offset + regGroupNum * 32 * 4; /* unit: bytes */
	for(i = 0; i < 32; i++)
		dbg_printf("Read G%u.%2u = 0x%08x (%u)\n", regGroupNum, i,
			   *((volatile unsigned int *)(regBaseAddr)+i),
			   *((volatile unsigned int *)(regBaseAddr)+i));
	return;
}

/* Write 4 bytes to a specific register in a certain group */
void write_regs(unsigned int regGroupNum, unsigned int regIndex,
		unsigned int value, unsigned int rgst_offset)
{
	unsigned int targetAddr = 0;

	targetAddr = rgst_offset + (regGroupNum * 32 * 4) + regIndex * 4; /* unit: bytes */
	*((volatile unsigned int *) targetAddr) = value;

	dbg_printf("Write G%u.%2u = 0x%08x (%u)\n", regGroupNum, regIndex, value, value);
	return;
}

/* Load Registers (by group)
 *
 * Syntax:
 *	lreg {group}
 */
static int do_reg_lreg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int	regGroupNum = 0;
	unsigned int rgst_offset = 0;

	if (argc == 4) {

	} else if (argc == 3) {
		if (strncmp(argv[1], "a", 1) == 0)
			rgst_offset = RGST_OFFSET_A;
		else if (strncmp(argv[1], "b", 1) == 0)
			rgst_offset = RGST_OFFSET_B;
		else {
			printf("Invalid chip. It should be \'a\' or \'b\'.\n");
			return CMD_RET_USAGE;
		}

		regGroupNum = simple_strtoul(argv[2], NULL, 0);
		prn_regs(regGroupNum, rgst_offset);
	} else
		return CMD_RET_USAGE;

	return 0;
}

/* Write to Register
 *
 * Syntax:
 *	wreg {group} {register index in group} {value}
 */
static int do_reg_wreg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 5) {
		unsigned int regGroupNum = 0, offset = 0, value = 0;
		unsigned int rgst_offset = -1;

		if (strncmp(argv[1], "a", 1) == 0)
			rgst_offset = RGST_OFFSET_A;
		else if (strncmp(argv[1], "b", 1) == 0)
			rgst_offset = RGST_OFFSET_B;
		else {
			printf("Invalid chip. It should be be \'a\' or \'b\'.\n");
			return CMD_RET_USAGE;
		}

		regGroupNum = simple_strtoul(argv[2], NULL, 0);
		offset = simple_strtoul(argv[3], NULL, 0);
		value = simple_strtoul(argv[4], NULL, 0);

		write_regs(regGroupNum, offset, value, rgst_offset);
	} else
		return CMD_RET_USAGE;
	return 0;
}

/**************************************************/
U_BOOT_CMD(
	lreg,	3,	1,	do_reg_lreg,
	"Sunplus MON : load register (by group)",
	"[a|b] [register group #]"
);

U_BOOT_CMD(
	wreg,	5,	1,	do_reg_wreg,
	"Sunplus MON : write 4 bytes to register",
	"[a|b] [register group #] [register # in group] [value (4 bytes)]"
);
