#ifndef _SP_MON_H_
#define _SP_MON_H_

#ifdef CONFIG_MAGIC_SYSRQ
/*
 * Code can be shared between Linux and U-Boot
 */
#define BUF_SIZE	256

#if 0
#define DBG_PRN		dbg_printf
#else
#define DBG_PRN(...)
#endif

/* Write 4 bytes to a specific register in designated group */
void write_regs(unsigned int, unsigned int, unsigned int);
/* Print specific group of registers */
void prn_regs(unsigned int);
/* Dump requested physical memory data */
int dumpPhysMem(unsigned int, unsigned int);
/* Write 4 bytes to a specific memory address */
int writeToMem(unsigned int, unsigned int);
/* Show task info */
void showTaskInfo(pid_t);
/* Query if we're in MON shell */
int is_in_mon_shell(void);
/* Print to MON debug port */
int dbg_printf(const char *, ...);

#endif	/* CONFIG_MAGIC_SYSRQ */
#endif	/* _SP_MON_H_ */
