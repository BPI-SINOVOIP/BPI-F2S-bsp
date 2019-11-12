#ifndef __SP_UART_H__
#define __SP_UART_H__

#include <mach/io_map.h>

#define LL_UART_PADDR		PA_IOB_ADDR(18 * 32 * 4)
#define LL_UART_VADDR		VA_IOB_ADDR(18 * 32 * 4)
#define LOGI_ADDR_UART0_REG	VA_IOB_ADDR(18 * 32 * 4)

/* uart register map */
#define SP_UART_DATA		0x00
#define SP_UART_LSR		0x04
#define SP_UART_MSR		0x08
#define SP_UART_LCR		0x0C
#define SP_UART_MCR		0x10
#define SP_UART_DIV_L		0x14
#define SP_UART_DIV_H		0x18
#define SP_UART_ISC		0x1C

/* lsr */
#define SP_UART_LSR_TXE		(1 << 6)	/* 1: trasmit fifo is empty */

/* interrupt */
#define SP_UART_LSR_BC		(1 << 5)	/* break condition */
#define SP_UART_LSR_FE		(1 << 4)	/* frame error */
#define SP_UART_LSR_OE		(1 << 3)	/* overrun error */
#define SP_UART_LSR_PE		(1 << 2)	/* parity error */
#define SP_UART_LSR_RX		(1 << 1)	/* 1: receive fifo not empty */
#define SP_UART_LSR_TX		(1 << 0)	/* 1: transmit fifo is not full */

#define SP_UART_LSR_BRK_ERROR_BITS	\
	(SP_UART_LSR_PE | SP_UART_LSR_OE | SP_UART_LSR_FE | SP_UART_LSR_BC)

/* lcr */
#define SP_UART_LCR_WL5		(0 << 0)
#define SP_UART_LCR_WL6		(1 << 0)
#define SP_UART_LCR_WL7		(2 << 0)
#define SP_UART_LCR_WL8		(3 << 0)
#define SP_UART_LCR_ST		(1 << 2)
#define SP_UART_LCR_PE		(1 << 3)
#define SP_UART_LCR_PR		(1 << 4)
#define SP_UART_LCR_BC		(1 << 5)

/* isc */
#define SP_UART_ISC_MSM		(1 << 7)	/* Modem status ctrl */
#define SP_UART_ISC_LSM		(1 << 6)	/* Line status interrupt */
#define SP_UART_ISC_RXM		(1 << 5)	/* RX interrupt, when got some input data */
#define SP_UART_ISC_TXM		(1 << 4)	/* TX interrupt, when trans start */
#define HAS_UART_ISC_FLAGMASK	0x0F
#define SP_UART_ISC_MS		(1 << 3)
#define SP_UART_ISC_LS		(1 << 2)
#define SP_UART_ISC_RX		(1 << 1)
#define SP_UART_ISC_TX		(1 << 0)

/* modem control register */
#define SP_UART_MCR_AT		(1 << 7)
#define SP_UART_MCR_AC		(1 << 6)
#define SP_UART_MCR_AR		(1 << 5)
#define SP_UART_MCR_LB		(1 << 4)
#define SP_UART_MCR_RI		(1 << 3)
#define SP_UART_MCR_DCD		(1 << 2)
#define SP_UART_MCR_RTS		(1 << 1)
#define SP_UART_MCR_DTS		(1 << 0)

/* DMA-RX, dma_enable_sel */
#define DMA_INT			(1 << 31)
#define DMA_MSI_ID_SHIFT	12
#define DMA_MSI_ID_MASK		(0x7F << DMA_MSI_ID_SHIFT)
#define DMA_SEL_UARTX_SHIFT	8
#define DMA_SEL_UARTX_MASK	(0x07 << DMA_SEL_UARTX_SHIFT)
#define DMA_SW_RST_B		(1 << 7)
#define DMA_INIT		(1 << 6)
#define DMA_GO			(1 << 5)
#define DMA_AUTO_ENABLE		(1 << 4)
#define DMA_TIMEOUT_INT_EN	(1 << 3)
#define DMA_P_SAFE_DISABLE	(1 << 2)
#define DMA_PBUS_DATA_SWAP	(1 << 1)
#define DMA_ENABLE		(1 << 0)

#if !defined(__ASSEMBLY__)
#define UART_SZ			0x80
struct regs_uart {
	volatile u32 uart_data;
	volatile u32 uart_lsr;
	volatile u32 uart_msr;
	volatile u32 uart_lcr;
	volatile u32 uart_mcr;
	volatile u32 uart_div_l;
	volatile u32 uart_div_h;
	volatile u32 uart_isc;
	volatile u32 uart_tx_residue;
	volatile u32 uart_rx_residue;
	volatile u32 uart_rx_threshold;
	volatile u32 uart_clk_src;
};
typedef struct regs_uart regs_uart_t;

struct regs_uarxdma {
	volatile u32 rxdma_enable_sel;
	volatile u32 rxdma_start_addr;
	volatile u32 rxdma_timeout_set;
	volatile u32 rxdma_reserved;
	volatile u32 rxdma_wr_adr;
	volatile u32 rxdma_rd_adr;
	volatile u32 rxdma_length_thr;
	volatile u32 rxdma_end_addr;
	volatile u32 rxdma_databytes;
	volatile u32 rxdma_debug_info;
};

struct regs_uatxdma {
	volatile u32 txdma_enable;
	volatile u32 txdma_sel;
	volatile u32 txdma_start_addr;
	volatile u32 txdma_end_addr;
	volatile u32 txdma_wr_adr;
	volatile u32 txdma_rd_adr;
	volatile u32 txdma_status;
	volatile u32 txdma_tmr_unit;
	volatile u32 txdma_tmr_cnt;
	volatile u32 txdma_rst_done;
};

struct regs_uatxgdma {
	volatile u32 gdma_hw_ver;
	volatile u32 gdma_config;
	volatile u32 gdma_length;
	volatile u32 gdma_addr;
	volatile u32 gdma_port_mux;
	volatile u32 gdma_int_flag;
	volatile u32 gdma_int_en;
	volatile u32 gdma_software_reset_status;
	volatile u32 txdma_tmr_cnt;
};

#endif /* __ASSEMBLY__ */

#endif /* __SP_UART_H__ */
