/*
 * Code can be shared between Linux and U-Boot,
 * just change IS_LINUX accordingly
 */
/* Linux: IS_LINUX == 1, U-Boot: IS_LINUX == 0 */
#ifndef IS_LINUX
#define IS_LINUX 1
#endif

#if IS_LINUX
#include <asm/memory.h>	/* phys_to_virt */
#include <mach/io_map.h>
#include <mach/sp_mon.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/debug.h>
#else	/* U-Boot */
#include <asm/arch/hardware.h>
#include <common.h>
#include "sp_mon.h"
#endif

#if IS_LINUX
extern void dbg_putc(char);
extern int dbg_printf(const char *, ...);
#define RGST_OFFSET		PA_IOB_ADDR(0)
#else	/* U-Boot */
#define dbg_printf		printf
#define RGST_OFFSET		SPHE_DEVICE_BASE
#endif

#if 0
#define DBG_PRN			dbg_printf
#else
#define	DBG_PRN(...)
#endif

#define DUMP_CHARS_PER_LINE	16
/* Dump / fill format */
#define MEM_FORMAT_BYTE		(sizeof(unsigned char))
#define MEM_FORMAT_WORD		(sizeof(unsigned short))
#define MEM_FORMAT_DWORD	(sizeof(unsigned int))

/*
 * getMemAddr() is for compatibility with U-Boot code
 * Since Linux memory addresses are remapped,
 * convert the physical to logical address
 */
static unsigned int getMemAddr(unsigned int physAddr)
{
#if IS_LINUX
	return (unsigned int)phys_to_virt(physAddr);
#else	/* U-Boot */
	return physAddr;
#endif
}

/*
 * Reading memory data safely
 * (dealing with 8, 16, 32 bit cases separately)
 */
static unsigned int safe8bitRead(unsigned int addr, unsigned char *data)
{
	*data = *((volatile unsigned char *)addr);
	return 1;
}

static unsigned int safe16bitRead(unsigned int addr, unsigned short *data)
{
	if (addr & 0x01) {
		return 0;
	}

	*data = *((volatile unsigned short *)addr);
	return 1;
}

static unsigned int safe32bitRead(unsigned int addr, unsigned int *data)
{
	if (addr & 0x03) {
		return 0;
	}

	*data = *((volatile unsigned int *)addr);
	return 1;
}

/* wreg: Write 4 bytes to a specific register in designated group */
void write_regs(unsigned int regGroupNum, unsigned int regIndex, unsigned int value)
{
	unsigned int targetAddr = 0;

	/* Calculate offset for register group */
#if IS_LINUX
	unsigned int va_addr, pa_addr;

	/* unit: bytes */
	pa_addr = RGST_OFFSET + regGroupNum * 32 * 4 + regIndex * 4;
	DBG_PRN("\npa:0x%08X\tva:0x%08X\n", pa_addr, va_addr);

	va_addr = (unsigned int)ioremap(pa_addr, 4);

	if (!va_addr)
		return;
	DBG_PRN("\npa:0x%08X\tva:0x%08X\n", pa_addr, va_addr);
	targetAddr = va_addr;
#else	/* U-Boot */
	/* unit: bytes */
	targetAddr = RGST_OFFSET + (regGroupNum * 32 * 4) + regIndex * 4;
#endif

	dbg_printf("Write G%u.%2u = 0x%08X (%u)\n", regGroupNum, regIndex, value, value);
	*((volatile unsigned int *)targetAddr) = value;

#if IS_LINUX
	iounmap((void *)va_addr);
#endif
	return;
}

/* lreg : Print specific group of registers */
void prn_regs(unsigned int regGroupNum)
{
	unsigned int i = 0;
	unsigned int regBaseAddr = 0;

	/* Calculate register group offset base address */
#if IS_LINUX
	unsigned int va_base, pa_base;

	pa_base = RGST_OFFSET + regGroupNum * 32 * 4;
	va_base = (unsigned int)ioremap(pa_base, 4 * 32);

	if (!va_base)
		return;

	DBG_PRN("\npa:0x%08X\tva:0x%08X\n", pa_base, va_base);
	regBaseAddr = va_base;
#else
	/* unit: bytes */
	regBaseAddr = RGST_OFFSET + regGroupNum * 32 * 4;
#endif

	DBG_PRN("Register group %u base=0x%08X\n", regGroupNum, regBaseAddr);
	for (i = 0 ; i < 32 ; i++)
		dbg_printf("Read G%u.%2u = 0x%08X (%u)\n",
			   regGroupNum, i,
			   *((volatile unsigned int *)(regBaseAddr) + i),
			   *((volatile unsigned int *)(regBaseAddr) + i));

#if IS_LINUX
	iounmap((void *)va_base);
#endif
	return;
}

/* Dumps requested physical memory data */
int dumpPhysMem(unsigned int physAddr, unsigned int dumpSize)
{
	unsigned int dumpFmt = MEM_FORMAT_DWORD;
	unsigned int baseAddr;
	unsigned char readMem[DUMP_CHARS_PER_LINE];
	/* Used to store read data from memory */
	unsigned char data_8;
	unsigned short data_16;
	unsigned int data_32;
	unsigned int offset = 0;

	baseAddr = getMemAddr(physAddr);

	DBG_PRN("phyAddr, baseAddr, dumpSize = 0x%08X, 0x%08X, %u", physAddr, baseAddr, dumpSize);
	while (offset < dumpSize) {
		/* Print the line offset label */
		if ((offset % DUMP_CHARS_PER_LINE) == 0)
			dbg_printf("\n%08X:  ", physAddr + offset);

		switch (dumpFmt) {
		case MEM_FORMAT_BYTE:
			if (safe8bitRead(baseAddr + offset, &data_8)) {
				dbg_printf("%02X ", data_8);
				readMem[(offset % DUMP_CHARS_PER_LINE) + 0] = data_8;
			} else
				return 0;
			break;

		case MEM_FORMAT_WORD:
			if (safe16bitRead(baseAddr + offset, &data_16)) {
				dbg_printf("%04X ", data_16);
				readMem[(offset % DUMP_CHARS_PER_LINE) + 0] = data_16;
			} else
				return 0;
			break;

		case MEM_FORMAT_DWORD:
			if (safe32bitRead(baseAddr + offset, &data_32)) {
				dbg_printf("%08X ", data_32);
				readMem[(offset % DUMP_CHARS_PER_LINE) + 0] = data_32;
			} else
				return 0;
			break;
		}
		offset = offset + dumpFmt;
	}
	dbg_printf("\n");
	return 1;
}

/* Write 4 bytes to a specific memory address */
int writeToMem(unsigned int physAddr, unsigned int value)
{
	unsigned int baseAddr;

	baseAddr = getMemAddr(physAddr);
	*(volatile unsigned int *)(baseAddr & 0xFFFFFFFC) = value;
	return 1;
}

#if IS_LINUX
void showTaskInfo(pid_t pid)
{
	struct task_struct *task, *child;
	static const char stat_nam[] = TASK_STATE_TO_CHAR_STR;
	unsigned state;

	/* If no pid, list all tasks */
	if ((int)pid == 0) {
		dbg_printf("pid\tppid\tstate\tcommand\n");
		do_each_thread(task, child) {
			state = child->state ? __ffs(child->state) + 1 : 0;
			dbg_printf("[%d]\t[%d]\t%c\t%-15s\n",
				   task_pid_nr(child),
				   task_pid_nr(task),
				   (state < sizeof(stat_nam) - 1) ? stat_nam[state] : '?',
				   child->comm);
		}
		while_each_thread(task, child);
		return;
	}

	/* Find the task and show its info */
	task = find_task_by_vpid(pid);
	if (task == NULL) {
		dbg_printf("no such task\n");
	} else {
		struct thread_info *info = task_thread_info(task);

		dbg_printf("pid(%d): task_struct@<0x%08X> thread_info@<0x%08X>\n",
			   (int)pid, (int)task, (int)info);
		show_stack(task, NULL);
	}
}
#endif
