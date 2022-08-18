/*
 *  linux/arch/arm/mach-faraday/ftpwmtmr010.c
 *
 *  Faraday FTPWMTMR010 Timer
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/sched_clock.h>

#include <asm/io.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define FTPWMTMR010_INTSTAT	0x00
#define FTPWMTMR010_TIMER(x)	((x) * 0x10 + 0x10)	/* timer start from 0x10 */
#define FTPWMTMR010_CTRL	0x00
#define FTPWMTMR010_LOAD	0x04
#define FTPWMTMR010_CMP		0x08
#define FTPWMTMR010_CNT		0x0c

/*
 * Timer Control Register
 */
#define FTPWMTMR010_CTRL_SRC		(1 << 0)
#define FTPWMTMR010_CTRL_START		(1 << 1)
#define FTPWMTMR010_CTRL_UPDATE		(1 << 2)
#define FTPWMTMR010_CTRL_OUT_INV	(1 << 3)
#define FTPWMTMR010_CTRL_AUTO		(1 << 4)
#define FTPWMTMR010_CTRL_INT_EN		(1 << 5)
#define FTPWMTMR010_CTRL_INT_MODE	(1 << 6)
#define FTPWMTMR010_CTRL_DMA_EN		(1 << 7)
#define FTPWMTMR010_CTRL_PWM_EN		(1 << 8)
#define FTPWMTMR010_CTRL_DZ(x)		(((x) & 0xff) << 24)

#define EVTTMR 0
#define SRCTMR 1

static void __iomem *ftpwmtmr010_base;
static unsigned int clk_reload;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void ftpwmtmr010_clear_int(unsigned int id)
{
	writel(1 << id, ftpwmtmr010_base + FTPWMTMR010_INTSTAT);
}

static void ftpwmtmr010_enable_noirq(unsigned int id)
{
	unsigned int cr;

	cr = FTPWMTMR010_CTRL_START | FTPWMTMR010_CTRL_AUTO;

	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
}

static void ftpwmtmr010_disable(unsigned int id)
{
	writel(0, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
}

static void ftpwmtmr010_set_reload(unsigned int id, unsigned int value)
{
	unsigned int cr;

	writel(value, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) +
	       FTPWMTMR010_LOAD);

	cr = readl(ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
	cr |= FTPWMTMR010_CTRL_UPDATE;
	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
}

static void ftpwmtmr010_set_cmp(unsigned int id, unsigned int value)
{
	unsigned int cr;

	writel(value, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) +
	       FTPWMTMR010_CMP);

	cr = readl(ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
	cr |= FTPWMTMR010_CTRL_UPDATE;
	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(id) + FTPWMTMR010_CTRL);
}

/******************************************************************************
 * clockevent functions
 *****************************************************************************/
static int ftpwmtmr010_set_next_event(unsigned long clc,
				      struct clock_event_device *ce)
{
	ftpwmtmr010_set_reload(EVTTMR, clc);

	return 0;
}

static int ftpwmtmr010_set_periodic(struct clock_event_device *ce)
{
	unsigned int cr;

	ftpwmtmr010_set_reload(EVTTMR, clk_reload);

	cr = FTPWMTMR010_CTRL_START
		| FTPWMTMR010_CTRL_AUTO
		| FTPWMTMR010_CTRL_INT_EN;

	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(EVTTMR)
		+ FTPWMTMR010_CTRL);
	return 0;
}

static int ftpwmtmr010_set_oneshot(struct clock_event_device *ce)
{
	unsigned int cr;

	cr = FTPWMTMR010_CTRL_START
		| FTPWMTMR010_CTRL_INT_EN;

	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(EVTTMR)
		+ FTPWMTMR010_CTRL);
	return 0;
}

static int ftpwmtmr010_resume(struct clock_event_device *ce)
{
	unsigned int cr;

	cr = readl(ftpwmtmr010_base
		   + FTPWMTMR010_TIMER(EVTTMR)
		   + FTPWMTMR010_CTRL);

	cr |= FTPWMTMR010_CTRL_START;

	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(EVTTMR)
		+ FTPWMTMR010_CTRL);
	return 0;
}

static int ftpwmtmr010_shutdown(struct clock_event_device *ce)
{
	unsigned int cr;

	cr = readl(ftpwmtmr010_base
		   + FTPWMTMR010_TIMER(EVTTMR)
		   + FTPWMTMR010_CTRL);

	cr &= ~FTPWMTMR010_CTRL_START;

	writel(cr, ftpwmtmr010_base + FTPWMTMR010_TIMER(EVTTMR)
		+ FTPWMTMR010_CTRL);
	return 0;
}

static irqreturn_t ftpwmtmr010_clockevent_interrupt(int irq, void *dev_id)
{
	unsigned int state;
	struct clock_event_device *ce = (struct clock_event_device *)dev_id;

	state = readl(ftpwmtmr010_base
			+ FTPWMTMR010_TIMER(EVTTMR)
			+ FTPWMTMR010_CTRL);

	/* If EVTTMR is level trigger mode, clear the PWMTMR IRQ source */
	if (!(state & FTPWMTMR010_CTRL_INT_MODE))
		ftpwmtmr010_clear_int(EVTTMR);

	if (ce == NULL || ce->event_handler == NULL) {
		pr_warn("ftpwmtmr010: event_handler is not found!\n");
		return IRQ_NONE;
	}

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static struct clock_event_device ftpwmtmr010_clkevt = {
	.name		= "ftpwmtmr010",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating		= 200,
	.set_next_event	= ftpwmtmr010_set_next_event,
	.set_state_shutdown = ftpwmtmr010_shutdown,
	.set_state_periodic = ftpwmtmr010_set_periodic,
	.set_state_oneshot = ftpwmtmr010_set_oneshot,
	.tick_resume	= ftpwmtmr010_resume,
};

static struct irqaction ftpwmtmr010_irq = {
	.name		= "ftpwmtmr010",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= ftpwmtmr010_clockevent_interrupt,
	.dev_id		= &ftpwmtmr010_clkevt,
};

static u64 notrace ftpwmtmr010_sched_clock_read(void)
{
	return ~readl_relaxed(ftpwmtmr010_base + FTPWMTMR010_TIMER(SRCTMR) +
			      FTPWMTMR010_CNT);
}

int __init ftpwmtmr010_init(void __iomem *base, int irq,
			     unsigned long clk_freq)
{
	int ret;
	ftpwmtmr010_base = base;
	ftpwmtmr010_irq.irq = irq;

	/* setup as free-running clocksource */
	ftpwmtmr010_disable(SRCTMR);
	ftpwmtmr010_set_cmp(SRCTMR, 0);
	ftpwmtmr010_set_reload(SRCTMR, 0xffffffff);
	ftpwmtmr010_enable_noirq(SRCTMR);

	sched_clock_register(ftpwmtmr010_sched_clock_read, BITS_PER_LONG,
			     clk_freq);

	ret = clocksource_mmio_init(ftpwmtmr010_base + FTPWMTMR010_TIMER(SRCTMR) +
				  FTPWMTMR010_CNT, "ftpwmtmr010_clksrc",
				  clk_freq, 300, 32,
				  clocksource_mmio_readl_down);
	if (ret) {
		pr_err("Failed to register clocksource\n");
		return ret;
	}

	/* initialize to a known state */
	ftpwmtmr010_disable(EVTTMR);
	ftpwmtmr010_set_cmp(EVTTMR, 0);

	/* setup reload value for periodic clockevents */
	clk_reload = clk_freq / HZ;

	ret = setup_irq(ftpwmtmr010_irq.irq, &ftpwmtmr010_irq);
	if (ret) {
		pr_err("Failed to register timer IRQ\n");
		return ret;
	}

#if defined(CONFIG_CPU_CA7) && defined(CONFIG_SMP)
	ftpwmtmr010_clkevt.cpumask = cpu_all_mask;
#else
	ftpwmtmr010_clkevt.cpumask = cpumask_of(0);
#endif

	ftpwmtmr010_clkevt.irq = ftpwmtmr010_irq.irq;
	clockevents_config_and_register(&ftpwmtmr010_clkevt, clk_freq, 0xf,
	                                0xffffffff);

	return 0;
}

int __init ftpwmtmr010_of_init(struct device_node *np)
{
	struct clk *clk;
	void __iomem *base;
	int irq;
	u32 freq;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return -ENXIO;

	irq = irq_of_parse_and_map(np, 0);
	if (irq <= 0)
		goto err;

	clk = of_clk_get(np, 0);
	clk_prepare_enable(clk);
	freq = clk_get_rate(clk);

	ftpwmtmr010_clkevt.name= of_get_property(np, "compatible", NULL);

	return ftpwmtmr010_init(base, irq, freq);

err:
	iounmap(base);
	return -EINVAL;
}

CLOCKSOURCE_OF_DECLARE(ftpwmtmr010, "faraday,ftpwmtmr010", ftpwmtmr010_of_init);
