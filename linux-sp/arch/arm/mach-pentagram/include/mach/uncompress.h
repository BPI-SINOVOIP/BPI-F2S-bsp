#ifndef __UNCOMPRESS_H_
#define __UNCOMPRESS_H_

#include <mach/sp_uart.h>

static regs_uart_t *uart_base = ((regs_uart_t *)(LL_UART_PADDR));

static void putc(int c)
{
	while (!(uart_base->uart_lsr & SP_UART_LSR_TX))
		barrier();
	uart_base->uart_data = c;
	return;
}

static inline void flush(void)
{
}

#define arch_decomp_setup()	/* Do nothing */

#endif /* __UNCOMPRESS_H_ */
