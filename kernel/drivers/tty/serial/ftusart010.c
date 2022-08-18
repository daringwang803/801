/*
 * FTUSART010 Serial Driver
 *
 * Copyright (C) 2019 Faraday Technology Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#if defined(CONFIG_SERIAL_FTUSART010_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/dma-direction.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeirq.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/spinlock.h>
#include <linux/sysrq.h>
#include <linux/tty_flip.h>
#include <linux/tty.h>

#include "ftusart010.h"

static void ftusart010_stop_tx(struct uart_port *port);
static void ftusart010_transmit_chars(struct uart_port *port);

static inline struct ftusart010_port *to_ftusart010_port(struct uart_port *port)
{
	return container_of(port, struct ftusart010_port, port);
}

static void ftusart010_set_bits(struct uart_port *port, u32 reg, u32 bits)
{
	u32 val;

	val = readl_relaxed(port->membase + reg);
	val |= bits;
	writel_relaxed(val, port->membase + reg);
}

static void ftusart010_clr_bits(struct uart_port *port, u32 reg, u32 bits)
{
	u32 val;

	val = readl_relaxed(port->membase + reg);
	val &= ~bits;
	writel_relaxed(val, port->membase + reg);
}

static int ftusart010_pending_rx(struct uart_port *port, u32 *sts, int *last_res,
			    bool threaded)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	enum dma_status status;
	struct dma_tx_state state;

	*sts = readl_relaxed(port->membase + FTUSART010_STS);

	if (threaded && ftusart010_port->rx_ch) {
		status = dmaengine_tx_status(ftusart010_port->rx_ch,
		                             ftusart010_port->rx_ch->cookie,
		                             &state);
		if ((status == DMA_IN_PROGRESS) &&
		    (*last_res != state.residue))
			return 1;
		else
			return 0;
	} else if (*sts & USART_STS_RDNE) {
		return 1;
	}
	return 0;
}

static unsigned long
ftusart010_get_char(struct uart_port *port, u32 *sts, int *last_res)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	unsigned long c;

	if (ftusart010_port->rx_ch) {
		c = ftusart010_port->rx_buf[RX_BUF_L - (*last_res)--];
		if ((*last_res) == 0)
			*last_res = RX_BUF_L;
		return c;
	} else {
		return readl_relaxed(port->membase + FTUSART010_RDR);
	}
}

static void ftusart010_receive_chars(struct uart_port *port, bool threaded)
{
	struct tty_port *tport = &port->state->port;
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	unsigned long c;
	u32 sts;
	char flag;

	if (irqd_is_wakeup_set(irq_get_irq_data(port->irq)))
		pm_wakeup_event(tport->tty->dev, 0);

	while (ftusart010_pending_rx(port, &sts, &ftusart010_port->last_res, threaded)) {
		c = ftusart010_get_char(port, &sts, &ftusart010_port->last_res);
		flag = TTY_NORMAL;
		port->icount.rx++;

		if (sts & USART_STS_ERR_MASK) {
			if (sts & USART_STS_BDF) {
				ftusart010_clr_bits(port, FTUSART010_STS, USART_STS_BDF);
				port->icount.brk++;
				if (uart_handle_break(port))
					continue;
			} else if (sts & USART_STS_ORERR) {
				port->icount.overrun++;
			} else if (sts & USART_STS_PERR) {
				port->icount.parity++;
			} else if (sts & USART_STS_FERR) {
				port->icount.frame++;
			}

			sts &= port->read_status_mask;

			if (sts & USART_STS_BDF)
				flag = TTY_BREAK;
			else if (sts & USART_STS_PERR)
				flag = TTY_PARITY;
			else if (sts & USART_STS_FERR)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, c))
			continue;
		uart_insert_char(port, sts, USART_STS_ORERR, c, flag);
	}

	spin_unlock(&port->lock);
	tty_flip_buffer_push(tport);
	spin_lock(&port->lock);
}

static void ftusart010_tx_dma_complete(void *arg)
{
	struct uart_port *port = arg;
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	unsigned int sts;
	int ret;

	ret = readl_relaxed_poll_timeout_atomic(port->membase + FTUSART010_STS,
	                                        sts,
	                                        (sts & USART_STS_TRAC),
	                                        10, 100000);

	if (ret)
		dev_err(port->dev, "terminal count not set\n");

	ftusart010_clr_bits(port, FTUSART010_STS, USART_STS_TRAC);

	ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);
	ftusart010_port->tx_dma_busy = false;

	/* Check the pending data */
	ftusart010_transmit_chars(port);
}

static void ftusart010_transmit_chars_pio(struct uart_port *port)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	struct circ_buf *xmit = &port->state->xmit;
	unsigned int sts;
	int ret;

	if (ftusart010_port->tx_dma_busy) {
		ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);
		ftusart010_port->tx_dma_busy = false;
	}

	ret = readl_relaxed_poll_timeout_atomic(port->membase + FTUSART010_STS,
	                                        sts,
	                                        (sts & USART_STS_TDE),
	                                        10, 100000);

	if (ret)
		dev_err(port->dev, "tx empty not set\n");

	ftusart010_set_bits(port, FTUSART010_CTRL1, USART_CTRL1_TDEIEN);

	writel_relaxed(xmit->buf[xmit->tail], port->membase + FTUSART010_TDR);
	xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	port->icount.tx++;
}

static void ftusart010_transmit_chars_dma(struct uart_port *port)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	struct circ_buf *xmit = &port->state->xmit;
	struct dma_async_tx_descriptor *desc = NULL;
	dma_cookie_t cookie;
	unsigned int count, i;

	if (ftusart010_port->tx_dma_busy)
		return;

	ftusart010_port->tx_dma_busy = true;

	count = uart_circ_chars_pending(xmit);

	if (count > TX_BUF_L)
		count = TX_BUF_L;

	if (xmit->tail < xmit->head) {
		memcpy(&ftusart010_port->tx_buf[0], &xmit->buf[xmit->tail], count);
	} else {
		size_t one = UART_XMIT_SIZE - xmit->tail;
		size_t two;

		if (one > count)
			one = count;
		two = count - one;

		memcpy(&ftusart010_port->tx_buf[0], &xmit->buf[xmit->tail], one);
		if (two)
			memcpy(&ftusart010_port->tx_buf[one], &xmit->buf[0], two);
	}

	desc = dmaengine_prep_slave_single(ftusart010_port->tx_ch,
	                                   ftusart010_port->tx_dma_buf,
	                                   count,
	                                   DMA_MEM_TO_DEV,
	                                   DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	if (!desc) {
		for (i = count; i > 0; i--)
			ftusart010_transmit_chars_pio(port);
		return;
	}

	desc->callback = ftusart010_tx_dma_complete;
	desc->callback_param = port;

	cookie = dmaengine_submit(desc);
	dma_async_issue_pending(ftusart010_port->tx_ch);

	ftusart010_clr_bits(port, FTUSART010_STS, USART_STS_TRAC);
	ftusart010_set_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);

	xmit->tail = (xmit->tail + count) & (UART_XMIT_SIZE - 1);
	port->icount.tx += count;
}

static void ftusart010_transmit_chars(struct uart_port *port)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	struct circ_buf *xmit = &port->state->xmit;

	if (port->x_char) {
		if (ftusart010_port->tx_dma_busy)
			ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);
		writel_relaxed(port->x_char, port->membase + FTUSART010_TDR);
		port->x_char = 0;
		port->icount.tx++;
		if (ftusart010_port->tx_dma_busy)
			ftusart010_set_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);
		return;
	}

	if (uart_tx_stopped(port)) {
		ftusart010_stop_tx(port);
		return;
	}

	if (uart_circ_empty(xmit)) {
		ftusart010_stop_tx(port);
		return;
	}

	if (ftusart010_port->tx_ch)
		ftusart010_transmit_chars_dma(port);
	else
		ftusart010_transmit_chars_pio(port);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		ftusart010_stop_tx(port);
}

static irqreturn_t ftusart010_interrupt(int irq, void *ptr)
{
	struct uart_port *port = ptr;
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	u32 sts;

	spin_lock(&port->lock);

	sts = readl_relaxed(port->membase + FTUSART010_STS);

	if ((sts & USART_STS_RDNE) && !(ftusart010_port->rx_ch))
		ftusart010_receive_chars(port, false);

	if ((sts & USART_STS_TDE) && !(ftusart010_port->tx_ch))
		ftusart010_transmit_chars(port);

	if (sts & USART_STS_IDLEF && ftusart010_port->rx_ch)
		readl_relaxed(port->membase + FTUSART010_RDR);

	spin_unlock(&port->lock);

	if (ftusart010_port->rx_ch)
		return IRQ_WAKE_THREAD;
	else
		return IRQ_HANDLED;
}

static irqreturn_t ftusart010_threaded_interrupt(int irq, void *ptr)
{
	struct uart_port *port = ptr;
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);

	spin_lock(&port->lock);

	if (ftusart010_port->rx_ch)
		ftusart010_receive_chars(port, true);

	spin_unlock(&port->lock);

	return IRQ_HANDLED;
}

static unsigned int ftusart010_tx_empty(struct uart_port *port)
{
	return readl_relaxed(port->membase + FTUSART010_STS) & USART_STS_TDE;
}

static void ftusart010_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	if ((mctrl & TIOCM_RTS) && (port->status & UPSTAT_AUTORTS))
		ftusart010_set_bits(port, FTUSART010_CTRL3, USART_CTRL3_RTSEN);
	else
		ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_RTSEN);
}

static unsigned int ftusart010_get_mctrl(struct uart_port *port)
{
	/* This routine is used to get signals of: DCD, DSR, RI, and CTS */
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}

static void ftusart010_stop_tx(struct uart_port *port)
{
	ftusart010_clr_bits(port, FTUSART010_CTRL1, USART_CTRL1_TDEIEN);
}

static void ftusart010_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	if (uart_circ_empty(xmit))
		return;

	ftusart010_transmit_chars(port);
}

/* Throttle the remote when input buffer is about to overflow. */
static void ftusart010_throttle(struct uart_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	ftusart010_clr_bits(port, FTUSART010_CTRL1, USART_CTRL1_RDNEIEN);
	spin_unlock_irqrestore(&port->lock, flags);
}

/* Unthrottle the remote, the input buffer can now accept data. */
static void ftusart010_unthrottle(struct uart_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	ftusart010_set_bits(port, FTUSART010_CTRL1, USART_CTRL1_RDNEIEN);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void ftusart010_stop_rx(struct uart_port *port)
{
	ftusart010_clr_bits(port, FTUSART010_CTRL1, USART_CTRL1_RDNEIEN);
}

static void ftusart010_break_ctl(struct uart_port *port, int break_state)
{
}

static int ftusart010_startup(struct uart_port *port)
{
	const char *name = to_platform_device(port->dev)->name;
	u32 ctrl1;
	int ret;

	ret = request_threaded_irq(port->irq, ftusart010_interrupt,
	                           ftusart010_threaded_interrupt,
	                           IRQF_NO_SUSPEND, name, port);
	if (ret)
		return ret;

	ctrl1 = USART_CTRL1_RDNEIEN | USART_CTRL1_TEN | USART_CTRL1_REN;
	ftusart010_set_bits(port, FTUSART010_CTRL1, ctrl1);

	return 0;
}

static void ftusart010_shutdown(struct uart_port *port)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	u32 val;
	volatile bool dma_busy;

	/* Avoid usart shutdown early than DMA transfer completed */
	do {
		dma_busy = ftusart010_port->tx_dma_busy;
	} while (dma_busy);

	val = USART_CTRL1_TDEIEN | USART_CTRL1_RDNEIEN | USART_CTRL1_TEN | USART_CTRL1_REN
	    | USART_CTRL1_IDLEIEN | USART_CTRL1_UEN;
	ftusart010_clr_bits(port, FTUSART010_CTRL1, val);

	dev_pm_clear_wake_irq(port->dev);
	free_irq(port->irq, port);
}

static void ftusart010_set_termios(struct uart_port *port, struct ktermios *termios,
			    struct ktermios *old)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	unsigned int baud;
	u32 usartdiv, mantissa, fraction;
	tcflag_t cflag = termios->c_cflag;
	u32 ctrl1, ctrl2, ctrl3;
	unsigned long flags;

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);

	spin_lock_irqsave(&port->lock, flags);

	/* Stop serial port and reset value */
	writel_relaxed(0, port->membase + FTUSART010_CTRL1);

	ctrl1 = USART_CTRL1_TEN | USART_CTRL1_REN | USART_CTRL1_RDNEIEN | USART_CTRL1_UEN;
	ctrl2 = 0;
	ctrl3 = 0;

	if (cflag & CSTOPB)
		ctrl2 |= USART_CTRL2_STOPB_2B;

	if (cflag & PARENB) {
		ctrl1 |= USART_CTRL1_PCEN;
		if ((cflag & CSIZE) == CS8) {
			ctrl1 |= USART_CTRL1_LEN;
		}
	}

	if (cflag & PARODD)
		ctrl1 |= USART_CTRL1_PSEL;

	port->status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS);
	if (cflag & CRTSCTS) {
		port->status |= UPSTAT_AUTOCTS | UPSTAT_AUTORTS;
		ctrl3 |= USART_CTRL3_CTSEN | USART_CTRL3_RTSEN;
	}

	usartdiv = DIV_ROUND_CLOSEST(port->uartclk, baud);

	mantissa = (usartdiv / 16) << USART_BAUDR_DIV_I_SHIFT;
	fraction = usartdiv % 16;
	writel_relaxed(mantissa | fraction, port->membase + FTUSART010_BAUDR);

	uart_update_timeout(port, cflag, baud);

	port->read_status_mask = USART_STS_ORERR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= USART_STS_PERR | USART_STS_FERR;
	if (termios->c_iflag & (IGNBRK | BRKINT | PARMRK))
		port->read_status_mask |= USART_STS_BDF;

	/* Characters to ignore */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask = USART_STS_PERR | USART_STS_FERR;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= USART_STS_BDF;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= USART_STS_ORERR;
	}

	if (ftusart010_port->rx_ch) {
		ctrl1 |= USART_CTRL1_IDLEIEN;
		ctrl3 |= USART_CTRL3_DMAREN;
	}

	writel_relaxed(ctrl3, port->membase + FTUSART010_CTRL3);
	writel_relaxed(ctrl2, port->membase + FTUSART010_CTRL2);
	writel_relaxed(ctrl1, port->membase + FTUSART010_CTRL1);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *ftusart010_type(struct uart_port *port)
{
	return (char *)DRIVER_NAME;
}

static void ftusart010_release_port(struct uart_port *port)
{
}

static int ftusart010_request_port(struct uart_port *port)
{
	return 0;
}

static void ftusart010_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_FARADAY;
}

static int
ftusart010_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	/* No user changeable parameters */
	return -EINVAL;
}

static void ftusart010_pm(struct uart_port *port, unsigned int state,
		unsigned int oldstate)
{
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);
	unsigned long flags = 0;

	switch (state) {
	case UART_PM_STATE_ON:
		clk_prepare_enable(ftusart010_port->clk);
		break;
	case UART_PM_STATE_OFF:
		spin_lock_irqsave(&port->lock, flags);
		ftusart010_clr_bits(port, FTUSART010_CTRL1, USART_CTRL1_UEN);
		spin_unlock_irqrestore(&port->lock, flags);
		clk_disable_unprepare(ftusart010_port->clk);
		break;
	}
}

static const struct uart_ops ftusart010_uart_ops = {
	.tx_empty       = ftusart010_tx_empty,
	.set_mctrl      = ftusart010_set_mctrl,
	.get_mctrl      = ftusart010_get_mctrl,
	.stop_tx        = ftusart010_stop_tx,
	.start_tx       = ftusart010_start_tx,
	.throttle       = ftusart010_throttle,
	.unthrottle     = ftusart010_unthrottle,
	.stop_rx        = ftusart010_stop_rx,
	.break_ctl      = ftusart010_break_ctl,
	.startup        = ftusart010_startup,
	.shutdown       = ftusart010_shutdown,
	.set_termios    = ftusart010_set_termios,
	.pm             = ftusart010_pm,
	.type           = ftusart010_type,
	.release_port   = ftusart010_release_port,
	.request_port   = ftusart010_request_port,
	.config_port    = ftusart010_config_port,
	.verify_port    = ftusart010_verify_port,
};

static int ftusart010_init_port(struct ftusart010_port *ftusart010_port,
			  struct platform_device *pdev)
{
	struct uart_port *port = &ftusart010_port->port;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	unsigned int clk;
	int ret;

	port->iotype    = UPIO_MEM;
	port->flags     = UPF_BOOT_AUTOCONF;
	port->ops       = &ftusart010_uart_ops;
	port->dev       = &pdev->dev;
	port->irq       = platform_get_irq(pdev, 0);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	port->membase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(port->membase))
		return PTR_ERR(port->membase);
	port->mapbase = res->start;

	spin_lock_init(&port->lock);

	if (of_property_read_u32(np, "clock-frequency", &clk)) {

		/* Get clk rate through clk driver if present */
		ftusart010_port->clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(ftusart010_port->clk)) {
			dev_warn(&pdev->dev, "clk or clock-frequency not defined\n");
			return PTR_ERR(ftusart010_port->clk);
		}

		ret = clk_prepare_enable(ftusart010_port->clk);
		if (ret < 0)
			return ret;

		clk = clk_get_rate(ftusart010_port->clk);
	}

	ftusart010_port->port.uartclk = clk;
	if (!ftusart010_port->port.uartclk) {
		clk_disable_unprepare(ftusart010_port->clk);
		ret = -EINVAL;
	}

	return ret;
}

static struct ftusart010_port *ftusart010_of_get_ftusart010_port(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int id;

	if (!np)
		return NULL;

	id = of_alias_get_id(np, "serial");
	if (id < 0) {
		dev_err(&pdev->dev, "failed to get alias id, errno %d\n", id);
		return NULL;
	}

	if (WARN_ON(id >= FTUSART010_MAX_PORTS))
		return NULL;

	ftusart010_ports[id].port.line = id;
	ftusart010_ports[id].last_res = RX_BUF_L;
	return &ftusart010_ports[id];
}

#ifdef CONFIG_OF
static const struct of_device_id ftusart010_match[] = {
	{ .compatible = "faraday,ftusart010" },
	{},
};

MODULE_DEVICE_TABLE(of, ftusart010_match);
#endif

static int ftusart010_of_dma_rx_probe(struct ftusart010_port *ftusart010_port,
				 struct platform_device *pdev)
{
	struct uart_port *port = &ftusart010_port->port;
	struct device *dev = &pdev->dev;
	struct dma_slave_config config;
	struct dma_async_tx_descriptor *desc = NULL;
	dma_cookie_t cookie;
	int ret;

	/* Request DMA RX channel */
	ftusart010_port->rx_ch = dma_request_slave_channel(dev, "rx");
	if (!ftusart010_port->rx_ch) {
		dev_info(dev, "rx dma alloc failed\n");
		return -ENODEV;
	}
	ftusart010_port->rx_buf = dma_alloc_coherent(&pdev->dev, RX_BUF_L,
	                                             &ftusart010_port->rx_dma_buf,
	                                             GFP_KERNEL);

	if (!ftusart010_port->rx_buf) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	/* Configure DMA channel */
	memset(&config, 0, sizeof(config));
	config.direction = DMA_DEV_TO_MEM;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	config.src_addr = port->mapbase + FTUSART010_RDR;
#if defined(CONFIG_FTDMAC020) || defined(CONFIG_FTDMAC030) || defined(CONFIG_FTAPBB020)
	config.dst_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
	config.src_maxburst     = 1;
	config.dst_maxburst     = 1;
#endif

	ret = dmaengine_slave_config(ftusart010_port->rx_ch, &config);
	if (ret < 0) {
		dev_err(dev, "rx dma channel config failed\n");
		ret = -ENODEV;
		goto config_err;
	}

	/* Prepare a DMA cyclic transaction */
	desc = dmaengine_prep_dma_cyclic(ftusart010_port->rx_ch,
	                                 ftusart010_port->rx_dma_buf,
	                                 RX_BUF_L, RX_BUF_P, DMA_DEV_TO_MEM,
	                                 DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	if (!desc) {
		dev_err(dev, "rx dma prep cyclic failed\n");
		ret = -ENODEV;
		goto config_err;
	}

	/* No callback as dma buffer is drained on usart interrupt */
	desc->callback = NULL;
	desc->callback_param = NULL;

	cookie = dmaengine_submit(desc);
	dma_async_issue_pending(ftusart010_port->rx_ch);

	return 0;

config_err:
	dma_free_coherent(&pdev->dev,
			  RX_BUF_L, ftusart010_port->rx_buf,
			  ftusart010_port->rx_dma_buf);

alloc_err:
	dma_release_channel(ftusart010_port->rx_ch);
	ftusart010_port->rx_ch = NULL;

	return ret;
}

static int ftusart010_of_dma_tx_probe(struct ftusart010_port *ftusart010_port,
				 struct platform_device *pdev)
{
	struct uart_port *port = &ftusart010_port->port;
	struct device *dev = &pdev->dev;
	struct dma_slave_config config;
	int ret;

	ftusart010_port->tx_dma_busy = false;

	/* Request DMA TX channel */
	ftusart010_port->tx_ch = dma_request_slave_channel(dev, "tx");
	if (!ftusart010_port->tx_ch) {
		dev_info(dev, "tx dma alloc failed\n");
		return -ENODEV;
	}
	ftusart010_port->tx_buf = dma_alloc_coherent(&pdev->dev, TX_BUF_L,
	                                             &ftusart010_port->tx_dma_buf,
	                                             GFP_KERNEL);

	if (!ftusart010_port->tx_buf) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	/* Configure DMA channel */
	memset(&config, 0, sizeof(config));

	config.direction = DMA_MEM_TO_DEV;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	config.dst_addr = port->mapbase + FTUSART010_TDR;
#if defined(CONFIG_FTDMAC020) || defined(CONFIG_FTDMAC030) || defined(CONFIG_FTAPBB020)
	config.src_addr_width   = DMA_SLAVE_BUSWIDTH_1_BYTE;
	config.src_maxburst     = 1;
	config.dst_maxburst     = 1;
#endif


	ret = dmaengine_slave_config(ftusart010_port->tx_ch, &config);
	if (ret < 0) {
		dev_err(dev, "tx dma channel config failed\n");
		ret = -ENODEV;
		goto config_err;
	}

	return 0;

config_err:
	dma_free_coherent(&pdev->dev,
	                  TX_BUF_L, ftusart010_port->tx_buf,
	                  ftusart010_port->tx_dma_buf);

alloc_err:
	dma_release_channel(ftusart010_port->tx_ch);
	ftusart010_port->tx_ch = NULL;

	return ret;
}

static int ftusart010_serial_probe(struct platform_device *pdev)
{
	struct ftusart010_port *ftusart010_port;
	int ret;

	ftusart010_port = ftusart010_of_get_ftusart010_port(pdev);
	if (!ftusart010_port)
		return -ENODEV;

	ret = ftusart010_init_port(ftusart010_port, pdev);
	if (ret)
		return ret;

	ret = uart_add_one_port(&ftusart010_driver, &ftusart010_port->port);
	if (ret)
		goto err_nowup;

	ret = ftusart010_of_dma_rx_probe(ftusart010_port, pdev);
	if (ret)
		dev_info(&pdev->dev, "interrupt mode used for rx (no dma)\n");

	ret = ftusart010_of_dma_tx_probe(ftusart010_port, pdev);
	if (ret)
		dev_info(&pdev->dev, "interrupt mode used for tx (no dma)\n");

	platform_set_drvdata(pdev, &ftusart010_port->port);

	return 0;

err_nowup:
	clk_disable_unprepare(ftusart010_port->clk);

	return ret;
}

static int ftusart010_serial_remove(struct platform_device *pdev)
{
	struct uart_port *port = platform_get_drvdata(pdev);
	struct ftusart010_port *ftusart010_port = to_ftusart010_port(port);

	ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMAREN);

	if (ftusart010_port->rx_ch)
		dma_release_channel(ftusart010_port->rx_ch);

	if (ftusart010_port->rx_dma_buf)
		dma_free_coherent(&pdev->dev,
		                  RX_BUF_L, ftusart010_port->rx_buf,
		                  ftusart010_port->rx_dma_buf);

	ftusart010_clr_bits(port, FTUSART010_CTRL3, USART_CTRL3_DMATEN);

	if (ftusart010_port->tx_ch)
		dma_release_channel(ftusart010_port->tx_ch);

	if (ftusart010_port->tx_dma_buf)
		dma_free_coherent(&pdev->dev,
		                  TX_BUF_L, ftusart010_port->tx_buf,
		                  ftusart010_port->tx_dma_buf);

	clk_disable_unprepare(ftusart010_port->clk);

	return uart_remove_one_port(&ftusart010_driver, port);
}

#ifdef CONFIG_SERIAL_FTUSART010_CONSOLE
static void ftusart010_console_putchar(struct uart_port *port, int ch)
{
	while (!(readl_relaxed(port->membase + FTUSART010_STS) & USART_STS_TDE))
		cpu_relax();

	writel_relaxed(ch, port->membase + FTUSART010_TDR);
}

static void ftusart010_console_write(struct console *co, const char *s, unsigned cnt)
{
	struct uart_port *port = &ftusart010_ports[co->index].port;
	unsigned long flags;
	u32 old_ctrl1, new_ctrl1;
	int locked = 1;

	local_irq_save(flags);
	if (port->sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock(&port->lock);
	else
		spin_lock(&port->lock);

	/* Save and disable interrupts, enable the transmitter */
	old_ctrl1 = readl_relaxed(port->membase + FTUSART010_CTRL1);
	new_ctrl1 = old_ctrl1 & ~USART_CTRL1_IEN_MASK;
	new_ctrl1 |=  USART_CTRL1_TEN | USART_CTRL1_UEN;
	writel_relaxed(new_ctrl1, port->membase + FTUSART010_CTRL1);

	uart_console_write(port, s, cnt, ftusart010_console_putchar);

	/* Restore interrupt state */
	writel_relaxed(old_ctrl1, port->membase + FTUSART010_CTRL1);

	if (locked)
		spin_unlock(&port->lock);
	local_irq_restore(flags);
}

static int ftusart010_console_setup(struct console *co, char *options)
{
	struct ftusart010_port *ftusart010_port;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index >= FTUSART010_MAX_PORTS)
		return -ENODEV;

	ftusart010_port = &ftusart010_ports[co->index];

	/*
	 * This driver does not support early console initialization
	 * (use ARM early printk support instead), so we only expect
	 * this to be called during the uart port registration when the
	 * driver gets probed and the port should be mapped at that point.
	 */
	if (ftusart010_port->port.mapbase == 0 || ftusart010_port->port.membase == NULL)
		return -ENXIO;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(&ftusart010_port->port, co, baud, parity, bits, flow);
}

static struct console ftusart010_console = {
	.name       = FTUSART010_SERIAL_NAME,
	.device     = uart_console_device,
	.write      = ftusart010_console_write,
	.setup      = ftusart010_console_setup,
	.flags      = CON_PRINTBUFFER,
	.index      = -1,
	.data       = &ftusart010_driver,
};

#define FTUSART010_SERIAL_CONSOLE (&ftusart010_console)

#else
#define FTUSART010_SERIAL_CONSOLE NULL
#endif /* CONFIG_SERIAL_FTUSART010_CONSOLE */

static struct uart_driver ftusart010_driver = {
	.driver_name    = DRIVER_NAME,
	.dev_name       = FTUSART010_SERIAL_NAME,
	.major          = 0,
	.minor          = 0,
	.nr             = FTUSART010_MAX_PORTS,
	.cons           = FTUSART010_SERIAL_CONSOLE,
};

static struct platform_driver ftusart010_serial_driver = {
	.probe      = ftusart010_serial_probe,
	.remove     = ftusart010_serial_remove,
	.driver = {
		.name   = DRIVER_NAME,
		.of_match_table = of_match_ptr(ftusart010_match),
	},
};

static int __init usart_init(void)
{
	static char banner[] __initdata = "FTUSART010 driver initialized";
	int ret;

	pr_info("%s\n", banner);

	ret = uart_register_driver(&ftusart010_driver);
	if (ret)
		return ret;

	ret = platform_driver_register(&ftusart010_serial_driver);
	if (ret)
		uart_unregister_driver(&ftusart010_driver);

	return ret;
}

static void __exit usart_exit(void)
{
	platform_driver_unregister(&ftusart010_serial_driver);
	uart_unregister_driver(&ftusart010_driver);
}

module_init(usart_init);
module_exit(usart_exit);

MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_DESCRIPTION("FARADAY FTUSART010 serial driver");
MODULE_LICENSE("GPL v2");
