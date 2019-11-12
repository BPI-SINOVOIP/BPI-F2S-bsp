#include <mach/sp_uart.h>

		.macro	addruart, rp, rv, tmp
		ldr	\rp, =LL_UART_PADDR @ physical
		ldr	\rv, =LL_UART_VADDR @ virtual
		.endm

		.macro	senduart, rd, rx
		str	\rd, [\rx, #SP_UART_DATA]	@ TXDATA
		.endm

		/* waituart waits until there is space in the FIFO */
		.macro	waituart, rd, rx
1001:		ldr	\rd, [\rx, #SP_UART_LSR]
		tst	\rd, #SP_UART_LSR_TX	@ 1: Transmit FIFO is not full
		beq	1001b
		.endm

		/* busyuart waits until the FIFO is empty (all data is sent) 0: Transmit FIFO is not empty */
		.macro	busyuart, rd, rx
1002:		ldr	\rd, [\rx, #SP_UART_LSR]
		tst	\rd, #SP_UART_LSR_TXE	@ 1: Transmit-ended, transmit FIFO is empty
		beq	1002b	@ wait if FIFO is not empty
		.endm
