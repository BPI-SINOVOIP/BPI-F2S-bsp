#ifndef __IO_MAP_8388_H__
#define __IO_MAP_8388_H__

/* PA */
#define	PA_REG			0x9C000000
#define	SIZE_REG		SZ_32M
#define PA_IO_ADDR(x)		((x) + PA_REG)
#define PA_IOB_ADDR(x)		PA_IO_ADDR(x)
#define PA_L2CC_REG		0x9f000000

/* VA */
#define VA_REG			0xF8000000
#define VA_IO_ADDR(x)		((x) + VA_REG)
#define VA_IOB_ADDR(x)		VA_IO_ADDR(x)

#endif
