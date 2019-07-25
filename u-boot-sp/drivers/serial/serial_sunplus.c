/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <errno.h>
#include <linux/compiler.h>
#include <serial.h>
/* #include <fdtdec.h> */

DECLARE_GLOBAL_DATA_PTR;

struct uart_sunplus {
	u32 uart_data;
	u32 uart_lsr;
	u32 uart_msr;
	u32 uart_lcr;
	u32 uart_mcr;
	u32 uart_div_l;
	u32 uart_div_h;
	u32 uart_isc;
	u32 uart_tx_residue;
	u32 uart_rx_residue;
	u32 uart_rx_threshold;
};

struct sunplus_uart_priv {
        struct uart_sunplus *regs;
};

#define SP_UART_LSR_TXE		(1 << 6)  // 1: trasmit fifo is empty
#define SP_UART_LSR_RX		(1 << 1)  // 1: receive fifo not empty
#define SP_UART_LSR_TX		(1 << 0)  // 1: transmit fifo is not full

//static volatile struct uart_sunplus *regs = (volatile struct uart_sunplus *)(0x9C000900);

static int _uart_sunplus_serial_putc(struct uart_sunplus *regs, const char c)
{
	if (!(readl(&regs->uart_lsr) & SP_UART_LSR_TX))
		return -EAGAIN;

	writel(c, &regs->uart_data);
	return 0;
}

int sunplus_serial_setbrg(struct udevice *dev, int baudrate)
{
	/* Baudrate has been setup in iBoot/xBoot. */
	return 0;
}

static int sunplus_serial_probe(struct udevice *dev)
{
	/* UART0 has been initialized in iBoot/xBoot. */
	debug("%s\n", __func__);
	return 0;
}

#ifdef DEBUG
static int sunplus_serial_bind(struct udevice *dev)
{
	debug("%s\n", __func__);
	return 0;
}
#endif

static int sunplus_serial_getc(struct udevice *dev)
{
	struct sunplus_uart_priv *priv = dev_get_priv(dev);
	struct uart_sunplus *regs = priv->regs;

	if (!(readl(&regs->uart_lsr) & SP_UART_LSR_RX))
		return -EAGAIN;

	return readl(&regs->uart_data);
}

static int sunplus_serial_putc(struct udevice *dev, const char ch)
{
	struct sunplus_uart_priv *priv = dev_get_priv(dev);

	return _uart_sunplus_serial_putc(priv->regs, ch);
}

static int sunplus_serial_pending(struct udevice *dev, bool input)
{
	struct sunplus_uart_priv *priv = dev_get_priv(dev);
	struct uart_sunplus *regs = priv->regs;

	if (input)
		return !!(readl(&regs->uart_lsr) & SP_UART_LSR_RX);
	else
		return !!(readl(&regs->uart_lsr) & SP_UART_LSR_TXE);
}

static int sunplus_serial_ofdata_to_platdata(struct udevice *dev)
{
        struct sunplus_uart_priv *priv = dev_get_priv(dev);

        priv->regs = (struct uart_sunplus *)devfdt_get_addr(dev);

        return 0;
}

static const struct dm_serial_ops sunplus_serial_ops = {
	.putc = sunplus_serial_putc,
	.pending = sunplus_serial_pending,
	.getc = sunplus_serial_getc,
	.setbrg = sunplus_serial_setbrg,
};

static const struct udevice_id sunplus_serial_ids[] = {
	{ .compatible = "sunplus,sunplus-uart"},
	{}
};

U_BOOT_DRIVER(serial_sunplus) = {
	.name	= "serial_sunplus",
	.id	= UCLASS_SERIAL,
	.of_match = sunplus_serial_ids,
	.ofdata_to_platdata = sunplus_serial_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct sunplus_uart_priv),
#if defined(DEBUG)
	.bind = sunplus_serial_bind,
#endif
	.probe = sunplus_serial_probe,
	.ops	= &sunplus_serial_ops,
};

#ifdef CONFIG_DEBUG_UART_SUNPLUS
/*
 * CONFIG_DEBUG_UART_BASE, CONFIG_DEBUG_UART_CLOCK, CONFIG_DEBUG_UART_SHIFT defined in configs/... are not used.
 * They are defined there for passing compilation.
 */

static inline void _debug_uart_putc(int ch)
{
	struct uart_sunplus *regs = (struct uart_sunplus *)CONFIG_DEBUG_UART_BASE;

	while (_uart_sunplus_serial_putc(regs, ch) == -EAGAIN) {
		// WATCHDOG_RESET();
	}
}

void _debug_uart_init(void)
{
	/* Baudrate has been setup in IBoot/XBoot, don't change it here. */

#if defined(DEBUG)
	_debug_uart_putc('_');
	_debug_uart_putc('d');
	_debug_uart_putc('e');
	_debug_uart_putc('b');
	_debug_uart_putc('u');
	_debug_uart_putc('g');
	_debug_uart_putc('_');
	_debug_uart_putc('u');
	_debug_uart_putc('a');
	_debug_uart_putc('r');
	_debug_uart_putc('t');
	_debug_uart_putc('_');
	_debug_uart_putc('i');
	_debug_uart_putc('n');
	_debug_uart_putc('i');
	_debug_uart_putc('t');
	_debug_uart_putc(0x0D);
	_debug_uart_putc(0x0A);
#endif
}

static inline void _printch(int ch)
{
	if (ch == '\n')
		_debug_uart_putc('\r');
	_debug_uart_putc(ch);
}

void printch(int ch)
{
	_printch(ch);
}

void printascii(const char *str)
{
	while (*str)
		_printch(*str++);
}

#endif /* CONFIG_DEBUG_UART_SUNPLUS */
