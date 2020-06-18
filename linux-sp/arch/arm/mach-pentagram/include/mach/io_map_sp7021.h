#ifndef __IO_MAP_SP7021_H__
#define __IO_MAP_SP7021_H__

/*****************
 * PA (B)
 *****************/
#define	PA_B_REG		0x9C000000
#define	SIZE_B_REG		SZ_32M
#define PA_B_SRAM0		0x9E800000
#define SIZE_B_SRAM0		(40 * SZ_1K)

#define PA_IOB_ADDR(x)		((x) + PA_B_REG)

/*****************
 * PA (A)
 *****************/
#define	PA_A_REG		0x9EC00000
#define	SIZE_A_REG		SZ_4M
#define PA_A_WORKMEM_SRAM0_BASE	0x9E000000
#define SIZE_A_WORKMEM_SRAM0	(SZ_512K)

#define PA_IOA_ADDR(x)		((x) + PA_A_REG)


/*****************
 * VA chain
 *****************/
#define VA_B_REG		0xF8000000
#define VA_A_REG		(VA_B_REG + SIZE_B_REG)
#define VA_B_SRAM0		(VA_A_REG + SIZE_A_REG)
#define VA_A_WORKMEM_SRAM0	(VA_B_SRAM0 + SIZE_B_SRAM0)

#define VA_IOB_ADDR(x)		((x) + VA_B_REG)
#define VA_IOA_ADDR(x)		((x) + VA_A_REG)

/*****************
 * Global VA
 *****************/
#define B_SYSTEM_BASE           VA_IOB_ADDR(0 * 32 * 4)
#define A_SYSTEM_BASE           VA_IOA_ADDR(0 * 32 * 4)
#define A_SYS_COUNTER_BASE      (A_SYSTEM_BASE + 0x10a000) /* 9ed0_a000 */

#endif
