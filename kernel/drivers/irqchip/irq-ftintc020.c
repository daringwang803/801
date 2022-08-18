/*
 *  Faraday FTINTC020 Interrupt Controller
 *
 * (C) Copyright 2019 Faraday Technology
 * B.C. Chen <bcchen@faraday-tech.com>
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <asm/io.h>
#include <asm/exception.h>

#include <linux/irqchip/chained_irq.h>
#include <linux/irqchip/irq-ftintc020.h>

static struct device_node *ftintc020_np = NULL;

struct ftintc020_chip_data {
	void __iomem *base;
	unsigned int irq_offset;
	struct irq_domain *domain;
};

static struct ftintc020_chip_data ftintc020_data;
static DEFINE_RAW_SPINLOCK(ftintc020_lock);

static inline void __iomem *ftintc020_base(struct irq_data *d)
{
	struct ftintc020_chip_data *chip_data = irq_data_get_irq_chip_data(d);
	if (!chip_data)
		BUG();
	return chip_data->base;
}

static inline unsigned int ftintc020_irq(struct irq_data *d)
{
	return d->hwirq;
}

/* somehow there is a gap in the SPI registers  */
static inline unsigned int ftintc020_register_offset(unsigned int irq)
{
	return irq < 32 ? 0x00 : 0x60;
}

static void ftintc020_irq_mask(struct irq_data *d)
{
	u32 irq = ftintc020_irq(d);
	u32 offset;
	u32 mask;

	raw_spin_lock(&ftintc020_lock);
	offset = ftintc020_register_offset(irq);
	mask = readl(ftintc020_base(d) + FTINTC020_OFFSET_IRQ_EN + offset);
	mask &= ~(1 << (irq % 32));
	writel(mask, ftintc020_base(d) + FTINTC020_OFFSET_IRQ_EN + offset);
#ifdef CONFIG_FARADAY_FTINTC020_VEC
	if (irq < 16) {
		mask = readl(ftintc020_base(d) + FTINTC020_OFFSET_VECT_CTRL(irq));
		mask &= ~(1 << 6);
		writel(mask, ftintc020_base(d) + FTINTC020_OFFSET_VECT_CTRL(irq));
	}
#endif
	raw_spin_unlock(&ftintc020_lock);
}

static void ftintc020_irq_unmask(struct irq_data *d)
{
	u32 irq = ftintc020_irq(d);
	u32 offset;
	u32 mask;

	raw_spin_lock(&ftintc020_lock);
	offset = ftintc020_register_offset(irq);
	mask = readl(ftintc020_base(d) + FTINTC020_OFFSET_IRQ_EN + offset);
	mask |= (1 << (irq % 32));
	writel(mask, ftintc020_base(d) + FTINTC020_OFFSET_IRQ_EN + offset);
#ifdef CONFIG_FARADAY_FTINTC020_VEC
	if (irq < 16) {
		mask = readl(ftintc020_base(d) + FTINTC020_OFFSET_VECT_CTRL(irq));
		mask |= (1 << 6);
		writel(mask, ftintc020_base(d) + FTINTC020_OFFSET_VECT_CTRL(irq));
	}
#endif

	raw_spin_unlock(&ftintc020_lock);
}

static void ftintc020_irq_eoi(struct irq_data *d)
{
	u32 irq = ftintc020_irq(d);
	u32 mask = 0;
	u32 offset;

	raw_spin_lock(&ftintc020_lock);
	offset = ftintc020_register_offset(irq);
	mask |= 1 << (irq % 32);
	writel(mask, ftintc020_base(d) + FTINTC020_OFFSET_IRQ_CLEAR + offset);
	raw_spin_unlock(&ftintc020_lock);
}

static int ftintc020_irq_set_type(struct irq_data *d, unsigned int type)
{
	u32 irq = ftintc020_irq(d);
	u32 offset = ftintc020_register_offset(irq);
	int mode = 0;
	int level = 0;
	unsigned int val;

	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:	/* mode = 0, level = 1 */
		level = 1;
		/* fall through */

	case IRQ_TYPE_LEVEL_HIGH:	/* mode = 0, level = 0 */
		break;

	case IRQ_TYPE_EDGE_FALLING:	/* mode = 1, level = 1 */
		level = 1;
		/* fall through */

	case IRQ_TYPE_EDGE_RISING:	/* mode = 1, level = 0 */
		mode = 1;
		break;

	default:
		return -EINVAL;
	}

	val = readl(ftintc020_base(d) + FTINTC020_OFFSET_IRQ_MODE + offset);
	val &= ~(1 << (irq % 32));
	val |= mode << (irq % 32);
	writel(val, ftintc020_base(d) + FTINTC020_OFFSET_IRQ_MODE + offset);

	val = readl(ftintc020_base(d) + FTINTC020_OFFSET_IRQ_LEVEL + offset);
	val &= ~(1 << (irq % 32));
	val |= level << (irq % 32);
	writel(val, ftintc020_base(d) + FTINTC020_OFFSET_IRQ_LEVEL + offset);

	return 0;
}

static struct irq_chip ftintc020_chip = {
	.name		= "ftintc020",
	.irq_mask	= ftintc020_irq_mask,
	.irq_unmask	= ftintc020_irq_unmask,
	.irq_eoi	= ftintc020_irq_eoi,
	.irq_set_type	= ftintc020_irq_set_type,
};

static void __exception_irq_entry ftintc020_handle_irq(struct pt_regs *regs)
{
	unsigned int hwirq, irqnr;
	unsigned long status;

	raw_spin_lock(&ftintc020_lock);
#ifdef CONFIG_FARADAY_FTINTC020_VEC
	status = readl(ftintc020_data.base + FTINTC020_OFFSET_SEL_ADDR);
	if (status == 64) {	//default vector handler
		hwirq = 32;
		status = readl(ftintc020_data.base + FTINTC020_OFFSET_EIRQ_STATUS);     /* IRQ 32 ~ 63 */
		if (!status) {
			hwirq = 0;
			status = readl(ftintc020_data.base + FTINTC020_OFFSET_IRQ_STATUS);  /* IRQ  0 ~ 31 */
		}
		hwirq += ffs(status) - 1;
#if defined(CONFIG_FARADAY_FTINTC020_PRIORSCHEME_FIXPRIOR)
		writel(0x00010000, ftintc020_data.base + FTINTC020_OFFSET_SEL_ADDR);
#endif
	} else {
		hwirq = status;
#if defined(CONFIG_FARADAY_FTINTC020_PRIORSCHEME_FIXPRIOR)
		writel(1 << hwirq, ftintc020_data.base + FTINTC020_OFFSET_SEL_ADDR);
#endif
	}
#else
	hwirq = 32;
	status = readl(ftintc020_data.base + FTINTC020_OFFSET_EIRQ_STATUS);     /* IRQ 32 ~ 63 */
	if (!status) {
		hwirq = 0;
		status = readl(ftintc020_data.base + FTINTC020_OFFSET_IRQ_STATUS);  /* IRQ  0 ~ 31 */
	}
	hwirq += ffs(status) - 1;
#endif
	raw_spin_unlock(&ftintc020_lock);

	// spurious interrupt?
	if (hwirq == -1)
		return;

	irqnr = irq_find_mapping(ftintc020_data.domain, hwirq);

	handle_IRQ(irqnr, regs);
}

static int ftintc020_irq_domain_map(struct irq_domain *d, unsigned int irq,
	                                irq_hw_number_t hwirq)
{
	struct ftintc020_chip_data *ftintc020 = d->host_data;

	irq_set_chip_data(irq, ftintc020);
	irq_set_chip_and_handler(irq, &ftintc020_chip, handle_fasteoi_irq);
	irq_clear_status_flags(irq, IRQ_NOREQUEST | IRQ_NOPROBE);

	return 0;
}

static struct irq_domain_ops ftintc020_irq_domain_ops = {
	.map = ftintc020_irq_domain_map,
	.xlate = irq_domain_xlate_twocell,
};

static void ftintc020_handle_cascade_irq(struct irq_desc *desc)
{
	struct ftintc020_chip_data *chip_data = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned int cascade_irq, hwirq;
	unsigned long status;

	chained_irq_enter(chip, desc);

	raw_spin_lock(&ftintc020_lock);
	hwirq = 32;
	status = readl(chip_data->base + FTINTC020_OFFSET_EIRQ_STATUS);     /* IRQ 32 ~ 63 */
	if (!status) {
		hwirq = 0;
		status = readl(chip_data->base + FTINTC020_OFFSET_IRQ_STATUS);  /* IRQ  0 ~ 31 */
	}
	hwirq += ffs(status) - 1;
	raw_spin_unlock(&ftintc020_lock);

	// spurious interrupt?
	if (hwirq == -1)
		goto out;

	cascade_irq = irq_find_mapping(chip_data->domain, hwirq);
	generic_handle_irq(cascade_irq);

out:
	chained_irq_exit(chip, desc);
}

void __init ftintc020_cascade_irq(unsigned int ftintc020_nr, unsigned int irq)
{
	if (irq_set_handler_data(irq, &ftintc020_data) != 0)
		BUG();

	irq_set_chained_handler(irq, ftintc020_handle_cascade_irq);
}

/*
 * Initialize the Faraday interrupt controller.
 */
void __init ftintc020_init(void __iomem *base, unsigned int irq_start)
{
	int nr_irq;
	int irq_base;
	unsigned int irq;
#ifdef CONFIG_FARADAY_FTINTC020_VEC
	unsigned int virq;
#endif
	unsigned int val;

	ftintc020_data.base = base;
	ftintc020_data.irq_offset = irq_start;

	nr_irq = FTINTC020_FEAT_IRQNUM(readl(base + FTINTC020_OFFSET_FEAT));

	for (irq = 0; irq < nr_irq; irq += 32) {
		unsigned int offset = ftintc020_register_offset(irq);

		writel(0, base + FTINTC020_OFFSET_IRQ_EN + offset);
		writel(0, base + FTINTC020_OFFSET_IRQ_MODE + offset);
		writel(0, base + FTINTC020_OFFSET_IRQ_LEVEL + offset);
		writel(~0, base + FTINTC020_OFFSET_IRQ_CLEAR + offset);
	}

#ifdef CONFIG_FARADAY_FTINTC020_VEC
	for (virq = 0; virq < 16; virq ++) {
		writel(virq, base + FTINTC020_OFFSET_VECT_ADDR(virq));
		writel(virq, base + FTINTC020_OFFSET_VECT_CTRL(virq));
	}

	writel(64, base + FTINTC020_OFFSET_DEF_ADDR);

	val = readl(base + FTINTC020_OFFSET_IRQ_PRIOR);
	val |= (1 << 19);
	writel(val, base + FTINTC020_OFFSET_IRQ_PRIOR);
#endif

	val = readl(base + FTINTC020_OFFSET_IRQ_PRIOR) & ~(0x3 << 17);
#if defined(CONFIG_FARADAY_FTINTC020_PRIORSCHEME_FIXPRIOR)
	val |= (0 << 17);
#elif defined(CONFIG_FARADAY_FTINTC020_PRIORSCHEME_FIXORDER)
	val |= (1 << 17);
#elif defined(CONFIG_FARADAY_FTINTC020_PRIORSCHEME_RR)
	val |= (2 << 17);
#endif
	writel(val, base + FTINTC020_OFFSET_IRQ_PRIOR);

	irq_base = irq_alloc_descs(irq_start, 0, nr_irq, numa_node_id());
	ftintc020_data.domain = irq_domain_add_legacy(ftintc020_np, nr_irq, irq_base, 0,
	                                              &ftintc020_irq_domain_ops,
	                                              &ftintc020_data);

	set_handle_irq(ftintc020_handle_irq);
}

#ifdef CONFIG_OF
static int ftintc020_cnt __initdata = 0;

int __init ftintc020_of_init(struct device_node *np, struct device_node *parent)
{
	void __iomem *base;
	unsigned int parent_irq;

	ftintc020_np = np;

	base = of_iomap(ftintc020_np, 0);
	if (WARN_ON(!base))
		return -ENOMEM;

	ftintc020_init(base, -1);

	parent_irq = irq_of_parse_and_map(np, 0);
	if (parent_irq) {
		ftintc020_cascade_irq(ftintc020_cnt, parent_irq);
	}

	ftintc020_cnt++;

	return 0;
}
IRQCHIP_DECLARE(ftintc020_intc, "faraday,ftintc020", ftintc020_of_init);
#endif
