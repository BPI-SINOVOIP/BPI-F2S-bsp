/*
 * sysrq handler for entering Sunplus MON mode
 *
 * "Sunplus MON" masks all TX output to console, while accepting certain
 * commands which are useful for register/memory debugging
 */

#include <linux/kernel.h>
#include <linux/sysrq.h>
#include <linux/init.h>
#include <linux/string.h>
#include <mach/io_map.h>	/* IO0_ADDRESS */
#include <mach/sp_mon.h>	/* Actual function implementations */
#include <linux/serial_core.h>	/* struct uart_port */

#define IO0_ADDRESS(_pa)	VA_IOB_ADDR(_pa)

#define SPACE			0x20
#define BCKSPACE		0x7F
#define	IS_DIGIT(c)		((c) >= '0' && (c) <= '9')
#define	IS_LETTER_LOW(c)	((c) >= 'a' && (c) <= 'z')
#define	IS_LETTER_UP(c)		((c) >= 'A' && (c) <= 'Z')

/* Functions to help print to uart directly! */
#define OUTPUT_CONSOLE		0
#define UART_BASE		uart_base_addrs[OUTPUT_CONSOLE]
#define UART0_LSR		(UART_BASE + 0x04)
#define UART0_DATA		(UART_BASE + 0x00)

unsigned int uart0_mask_tx = 0;

static unsigned long uart_base_addrs[] = {
	IO0_ADDRESS(0x0900), IO0_ADDRESS(0x0980), IO0_ADDRESS(0x0800),
	IO0_ADDRESS(0x0880), IO0_ADDRESS(0x8780), IO0_ADDRESS(0x8800),
};

static void dbg_putchar(char c)
{
	/* Wait for transmit FIFO to not be full (available for more input) */
	while (!(*((volatile unsigned short *)(UART0_LSR)) & 0x01))
		/* wait */;
	/* Place data to UART data port register */
	*((volatile unsigned short *)(UART0_DATA)) = c;
}

void dbg_putc(char c)
{
	if (c == '\n')
		dbg_putchar('\r');
	dbg_putchar(c);
}

int dbg_printf(const char *fmt, ...)
{
	int i, len;
	va_list args;
	char buf[BUF_SIZE];

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	for (i = 0; i < len; i++)
		dbg_putc(buf[i]);

	return len;
}

int is_in_mon_shell(void)
{
	return uart0_mask_tx;
}

/*
 * When the user presses 'Ctrl + Break + A' to control UART TX output,
 * this is the callback we use.
 */
static void handle_mask_tx(int key)
{
	if (uart0_mask_tx == 1) {
		dbg_printf("Leaving Sunplus MON\n");
		uart0_mask_tx = 0;
	} else {
		uart0_mask_tx = 1;
		dbg_printf("Welcome to Sunplus MON\n");
	}
}

static struct sysrq_key_op sysrq_mask_tx_op = {
	.handler	= handle_mask_tx,
	.help_msg	= "maskTX",
	.action_msg	= "Console tx mask",
	.enable_mask	= SYSRQ_ENABLE_LOG
};

static int pm_sysrq_init(void)
{
	register_sysrq_key('a', &sysrq_mask_tx_op);
	return 1;
}

static void skipBlanks(char *cmd, unsigned int strnCount, int *currentIdx)
{
	/* Blanks were replaced with '\0' for kstrtoul() to work directly */
	while (cmd[*currentIdx] == '\0' && *currentIdx < strnCount) {
		*currentIdx = *currentIdx + 1;
	}
	return;
}

static void getNextParam(char *cmd, unsigned int strnCount, int *currentIdx, unsigned long *param)
{
	while (cmd[*currentIdx] != '\0' && *currentIdx < strnCount) {
		*currentIdx = *currentIdx + 1;
	}

	skipBlanks(cmd, strnCount, currentIdx);
	if (kstrtoul(&cmd[*currentIdx], 0, param)) {
		dbg_printf("Err: getNextParam()\n");
	}

	return;
}

static void prn_usage(void)
{
	dbg_printf("======================================================================\n");
	dbg_printf("Supported functions:\n");

	dbg_printf("lreg: list registers (by group)\n");
	dbg_printf("\tlreg [register group #]\n");
	dbg_printf("\tEx: list registers of group 18\n\t\tlreg 18\n");

	dbg_printf("wreg: write 4 bytes to register\n");
	dbg_printf("\twreg [register group #] [register # in group] [value (4 bytes)]\n");
	dbg_printf("\tEx: write value 200 to group 18 register 0\n\t\twreg 18 0 200\n");

	dbg_printf("lw: list 64 bytes of memory data\n");
	dbg_printf("\tlw [physical address]\n");
	dbg_printf("\tEx: list 64 bytes from physical memory address 0x00800100\n\t\tlw 0x00800100\n");

	dbg_printf("sw: store 4 bytes to memory\n");
	dbg_printf("\tsw [physical address] [value (4 bytes)]\n");
	dbg_printf("\tEx: store value 0xFFFFFFFF to physical memory address 0x00800100\n\t\tsw 0x00800100 0xFFFFFFFF\n");

	dbg_printf("task: show task backtrace info by pid\n");
	dbg_printf("\ttask [pid]\n");

	dbg_printf("exit: leave \"Sunplus MON\" mode\n");

	dbg_printf("Notes:\n");
	dbg_printf("1. Entered parameters may be in unsigned decimal (12345) or hex (0xFFFFFFFF) format\n");
	dbg_printf("2. Register group number X = ((group X base address) - (group 0 base address)) / (32 * 4)\n");
	dbg_printf("======================================================================\n");
}

/* Parse which command user entered and execute it */
static void executeCmd(char *cmd, unsigned int strnCount)
{
	int idx = 0, i = 0;

	DBG_PRN("command line :%s", cmd);
	/* Replace blanks with '\0' for kstrtoul() to work directly */
	for (i = 0; i < strnCount; i++) {
		if (cmd[i] == ' ')
			cmd[i] = '\0';
	}

	if (strncmp(cmd, "exit", 4) == 0) {
		uart0_mask_tx = 0;
		dbg_printf("Leaving Sunplus MON..\n");
	} else if (strncmp(cmd, "lreg", 4) == 0) {
		unsigned long regGroupNum = 0;

		getNextParam(cmd, strnCount, &idx, &regGroupNum);

		prn_regs((unsigned int)regGroupNum);
	} else if (strncmp(cmd, "wreg", 4) == 0) {
		unsigned long regGroupNum = 0, regIndex = 0, value = 0;

		getNextParam(cmd, strnCount, &idx, &regGroupNum);
		getNextParam(cmd, strnCount, &idx, &regIndex);
		getNextParam(cmd, strnCount, &idx, &value);

		write_regs(regGroupNum, regIndex, value);
	} else if (strncmp(cmd, "lw", 2) == 0) {
		unsigned long startAddr = 0;

		getNextParam(cmd, strnCount, &idx, &startAddr);

		dumpPhysMem(startAddr, 64);
	} else if (strncmp(cmd, "sw", 2) == 0) {
		unsigned long startAddr = 0, value = 0;

		getNextParam(cmd, strnCount, &idx, &startAddr);
		getNextParam(cmd, strnCount, &idx, &value);

		writeToMem(startAddr, value);
	} else if (strncmp(cmd, "task", 4) == 0) {
		unsigned long pid = 0;

		if (strnCount > 4)
			getNextParam(cmd, strnCount, &idx, &pid);

		showTaskInfo((pid_t)pid);
	} else {
		dbg_printf("Unrecognized command!\n");
		prn_usage();
	}
}

int sysrqCheckState(char c, struct uart_port *port)
{
	static char cmd[BUF_SIZE] = "";
	/* This indicates the number of characters currently stored in array */
	static unsigned int strnCount = 0;
	int i = 0;

	if (uart0_mask_tx == 0 || port->cons == NULL)
		return 0;

	dbg_putc(c);
	/* Pressing "Enter" or buffer is full triggers execution of command */
	/* We currently only deal with letters, enter key and digits */
	if (c == '\n' || c == '\r') {
		dbg_putc('\n');
		executeCmd(&cmd[0], strnCount);
		/* Clear command buffer after executing command */
		strnCount = 0;
		for (i = 0; i < BUF_SIZE; i++)
			cmd[i] = '\0';
	} else if (IS_LETTER_LOW(c) || IS_LETTER_UP(c) || IS_DIGIT(c) || (c == SPACE)) {
		cmd[strnCount] = c;
		strnCount++;
	} else if (c == BCKSPACE) {
		if (strnCount <= 0)
			return -1;
		strnCount--;
		cmd[strnCount] = '\0';
	}
	DBG_PRN("\nindex=%d\tCurrent array: %s\n", strnCount, cmd);

	if (strnCount >= BUF_SIZE) {
		executeCmd(&cmd[0], strnCount);
		dbg_printf("Buffer full.. Trying to execute command\n");
		strnCount = 0;

		for (i = 0; i < BUF_SIZE; i++)
			cmd[i] = '\0';
	}
	return 1;
}

subsys_initcall(pm_sysrq_init);
