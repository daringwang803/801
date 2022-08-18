/*
 *  linux/arch/arm/mach-faraday/fttmr010.c
 *
 *  Faraday FTTMR010 Timer
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
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/sched_clock.h>
#include <asm/io.h>

#include <linux/clk.h>
#include <linux/clkdev.h>

#define FTTMR010_COUNTER	0x00
#define FTTMR010_LOAD		0x04
#define FTTMR010_MATCH1		0x08
#define FTTMR010_MATCH2		0x0c
#define FTTMR010_TIMER(x)	((x) * 0x10)
#define FTTMR010_CR		0x30
#define FTTMR010_INTR_STATE	0x34
#define FTTMR010_INTR_MASK	0x38

/*
 * Timer Control Register
 */
#define FTTMR010_TM3_UPDOWN	(1 << 11)
#define FTTMR010_TM2_UPDOWN	(1 << 10)
#define FTTMR010_TM1_UPDOWN	(1 << 9)
#define FTTMR010_TM3_OFENABLE	(1 << 8)
#define FTTMR010_TM3_CLOCK	(1 << 7)
#define FTTMR010_TM3_ENABLE	(1 << 6)
#define FTTMR010_TM2_OFENABLE	(1 << 5)
#define FTTMR010_TM2_CLOCK	(1 << 4)
#define FTTMR010_TM2_ENABLE	(1 << 3)
#define FTTMR010_TM1_OFENABLE	(1 << 2)
#define FTTMR010_TM1_CLOCK	(1 << 1)
#define FTTMR010_TM1_ENABLE	(1 << 0)

/*
 * Timer Interrupt State & Mask Registers
 */
#define FTTMR010_TM3_OVERFLOW	(1 << 8)
#define FTTMR010_TM3_MATCH2	(1 << 7)
#define FTTMR010_TM3_MATCH1	(1 << 6)
#define FTTMR010_TM2_OVERFLOW	(1 << 5)
#define FTTMR010_TM2_MATCH2	(1 << 4)
#define FTTMR010_TM2_MATCH1	(1 << 3)
#define FTTMR010_TM1_OVERFLOW	(1 << 2)
#define FTTMR010_TM1_MATCH2	(1 << 1)
#define FTTMR010_TM1_MATCH1	(1 << 0)

#define EVTTMR 0
#define SRCTMR 1

static void __iomem *fttmr010_base;
static unsigned int clk_reload;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static const unsigned int fttmr010_cr_mask[3] = {
	FTTMR010_TM1_ENABLE | FTTMR010_TM1_CLOCK |
	FTTMR010_TM1_OFENABLE | FTTMR010_TM1_UPDOWN,

	FTTMR010_TM2_ENABLE | FTTMR010_TM2_CLOCK |
	FTTMR010_TM2_OFENABLE | FTTMR010_TM2_UPDOWN,

	FTTMR010_TM3_ENABLE | FTTMR010_TM3_CLOCK |
	FTTMR010_TM3_OFENABLE | FTTMR010_TM3_UPDOWN,
};

/* we always use down counter */
static const unsigned int fttmr010_cr_enable_flag[3] = {
	FTTMR010_TM1_ENABLE | FTTMR010_TM1_OFENABLE,
	FTTMR010_TM2_ENABLE | FTTMR010_TM2_OFENABLE,
	FTTMR010_TM3_ENABLE | FTTMR010_TM3_OFENABLE,
};

static const unsigned int fttmr010_cr_enable_noirq_flag[3] = {
	FTTMR010_TM1_ENABLE,
	FTTMR010_TM2_ENABLE,
	FTTMR010_TM3_ENABLE,
};

static void fttmr010_enable(unsigned int id)
{
	unsigned int cr = readl(fttmr010_base + FTTMR010_CR);

	cr &= ~fttmr010_cr_mask[id];
	cr |= fttmr010_cr_enable_flag[id];
	writel(cr, fttmr010_base + FTTMR010_CR);
}

static void fttmr010_enable_noirq(unsigned int id)
{
	unsigned int cr = readl(fttmr010_base + FTTMR010_CR);

	cr &= ~fttmr010_cr_mask[id];
	cr |= fttmr010_cr_enable_noirq_flag[id];
	writel(cr, fttmr010_base + FTTMR010_CR);
}

static void fttmr010_disable(unsigned int id)
{
	unsigned int cr = readl(fttmr010_base + FTTMR010_CR);

	cr &= ~fttmr010_cr_mask[id];
	writel(cr, fttmr010_base + FTTMR010_CR);
}

static const unsigned int fttmr010_irq_mask[3] = {
	FTTMR010_TM1_MATCH1 | FTTMR010_TM1_MATCH2 | FTTMR010_TM1_OVERFLOW,
	FTTMR010_TM2_MATCH1 | FTTMR010_TM2_MATCH2 | FTTMR010_TM2_OVERFLOW,
	FTTMR010_TM3_MATCH1 | FTTMR010_TM3_MATCH2 | FTTMR010_TM3_OVERFLOW,
};

static void fttmr010_unmask_irq(unsigned int id)
{
	unsigned int mask;

	mask = readl(fttmr010_base + FTTMR010_INTR_MASK);
	mask &= ~fttmr010_irq_mask[id];
	writel(mask, fttmr010_base + FTTMR010_INTR_MASK);
}

static inline void fttmr010_write_timer(unsigned int id, unsigned int reg,
                                        unsigned int value)
{
	void __iomem *addr = fttmr010_base + FTTMR010_TIMER(id) + reg;
	writel (value, addr);
}

static inline unsigned int fttmr010_read_timer(unsigned int id,
                                               unsigned int reg)
{
	void __iomem *addr = fttmr010_base + FTTMR010_TIMER(id) + reg;
	return readl(addr);
}

static void fttmr010_set_counter(unsigned int id, unsigned int value)
{
	fttmr010_write_timer(id, FTTMR010_COUNTER, value);
}

static void fttmr010_set_reload(unsigned int id, unsigned int value)
{
	fttmr010_write_timer(id, FTTMR010_LOAD, value);
}

static void fttmr010_set_match1(unsigned int id, unsigned int value)
{
	fttmr010_write_timer(id, FTTMR010_MATCH1, value);
}

static void fttmr010_set_match2(unsigned int id, unsigned int value)
{
	fttmr010_write_timer(id, FTTMR010_MATCH2, value);
}

/******************************************************************************
 * clockevent functions
 *****************************************************************************/
static int fttmr010_set_next_event(unsigned long clc,
					struct clock_event_device *ce)
{
	fttmr010_set_counter(EVTTMR, clc);
	return 0;
}

static int fttmr010_set_periodic(struct clock_event_device *ce)
{
	fttmr010_set_reload(EVTTMR, clk_reload);
	fttmr010_set_counter(EVTTMR, clk_reload);
	fttmr010_enable(EVTTMR);
	return 0;
}

static int fttmr010_set_oneshot(struct clock_event_device *ce)
{
	fttmr010_set_reload(EVTTMR, 0);
	fttmr010_enable(EVTTMR);
	return 0;
}

static int fttmr010_resume(struct clock_event_device *ce)
{
	fttmr010_enable(EVTTMR);
	return 0;
}

static int fttmr010_shutdown(struct clock_event_device *ce)
{
	fttmr010_disable(EVTTMR);
	return 0;
}

static irqreturn_t fttmr010_clockevent_interrupt(int irq, void *dev_id)
{
	unsigned int state;
	struct clock_event_device *ce = (struct clock_event_device *)dev_id;

	state = readl_relaxed(fttmr010_base + FTTMR010_INTR_STATE);
	if (!(state & fttmr010_irq_mask[EVTTMR]))
		return IRQ_NONE;

//	state &= ~fttmr010_irq_mask[EVTTMR];	//20150902 BC: INTR_STATE need to use W1C after IP version 1.8
	writel_relaxed(state, fttmr010_base + FTTMR010_INTR_STATE);

	if (ce == NULL || ce->event_handler == NULL) {
		pr_warn("fttmr010: event_handler is not found!\n");
		return IRQ_NONE;
	}

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static struct clock_event_device fttmr010_clkevt = {
	.name           = "fttrm010",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 200,
	.set_next_event = fttmr010_set_next_event,
	.set_state_shutdown = fttmr010_shutdown,
	.set_state_periodic = fttmr010_set_periodic,
	.set_state_oneshot = fttmr010_set_oneshot,
	.tick_resume    = fttmr010_resume,
};

static struct irqaction fttmr010_irq = {
	.name           = "fttmr010",
	.flags          = IRQF_TIMER | IRQF_IRQPOLL,
	.handler        = fttmr010_clockevent_interrupt,
	.dev_id         = &fttmr010_clkevt,
};

static u64 notrace fttmr010_sched_clock_read(void)
{
	return ~readl_relaxed(fttmr010_base + FTTMR010_TIMER(SRCTMR) +
	                      FTTMR010_COUNTER);
}

int __init fttmr010_init(void __iomem *base, int irq, unsigned long clk_freq)
{
	int ret;

	fttmr010_base = base;
	fttmr010_irq.irq = irq;

	/* setup as free-running clocksource */
	fttmr010_disable(SRCTMR);
	fttmr010_set_match1(SRCTMR, 0);
	fttmr010_set_match2(SRCTMR, 0);
	fttmr010_set_reload(SRCTMR, 0xffffffff);
	fttmr010_set_counter(SRCTMR, 0xffffffff);
	fttmr010_enable_noirq(SRCTMR);

	sched_clock_register(fttmr010_sched_clock_read, BITS_PER_LONG,
	                     clk_freq);

	ret = clocksource_mmio_init(fttmr010_base + FTTMR010_TIMER(SRCTMR) +
	                            FTTMR010_COUNTER, "fttmr010_clksrc", clk_freq,
	                            300, 32, clocksource_mmio_readl_down);
	if (ret) {
		pr_err("Failed to register clocksource\n");
		return ret;
	}

	/* initialize to a known state */
	fttmr010_disable(EVTTMR);
	fttmr010_set_match1(EVTTMR, 0);
	fttmr010_set_match2(EVTTMR, 0);
	fttmr010_unmask_irq(EVTTMR);

	/* setup reload value for periodic clockevents */
	clk_reload = clk_freq / HZ;

	/* Make irqs happen for the system timer */

	ret = setup_irq(fttmr010_irq.irq, &fttmr010_irq);
	if (ret) {
		pr_err("Failed to register timer IRQ\n");
		return ret;
	}

	/* setup struct clock_event_device */
#if defined(CONFIG_CPU_CA7) && defined(CONFIG_SMP)
	fttmr010_clkevt.cpumask	= cpu_all_mask;
#else
	fttmr010_clkevt.cpumask	= cpumask_of(0);
#endif
	fttmr010_clkevt.irq = fttmr010_irq.irq;
	clockevents_config_and_register(&fttmr010_clkevt, clk_freq, 0xf,
	                                0xffffffff);

	return 0;
}

int __init fttmr010_of_init(struct device_node *np)
{
	struct clk *clk;
	u32 freq;
	void __iomem *base;
	int irq;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return -ENXIO;

	irq = irq_of_parse_and_map(np, 0);
	if (irq <= 0)
		goto err;

	clk = of_clk_get(np, 0);
	clk_prepare_enable(clk);
	freq = clk_get_rate(clk);

	fttmr010_clkevt.name = of_get_property(np, "compatible", NULL);

	return fttmr010_init(base, irq, freq);

err:
	iounmap(base);
	return -EINVAL;
}

CLOCKSOURCE_OF_DECLARE(fttmr010, "faraday,fttmr010", fttmr010_of_init);
