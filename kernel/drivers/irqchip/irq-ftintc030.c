/*
 *  Faraday FTINTC030 Interrupt Controller
 *
 *  Copyright (C) 2014 Faraday Technology
 *  Copyright (C) 2014 Yan-Pai Chen <ypchen@faraday-tech.com>
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
#include <linux/irqchip/irq-ftintc030.h>

static struct device_node *ftintc030_np = NULL;

struct ftintc030_chip_data {
	void __iomem *base;
	unsigned int irq_offset;
	int nr_spi;
	struct irq_domain *domain;
};

static struct ftintc030_chip_data ftintc030_data;
static DEFINE_RAW_SPINLOCK(ftintc030_lock);

static inline void __iomem *ftintc030_base(struct irq_data *d)
{
	struct ftintc030_chip_data *chip_data = irq_data_get_irq_chip_data(d);
	if (!chip_data)
		BUG();
	return chip_data->base;
}

static inline unsigned int ftintc030_irq(struct irq_data *d)
{
	return d->hwirq;
}

/* somehow there is a gap in the SPI registers  */
static inline unsigned int ftintc030_register_offset(unsigned int irq)
{
	return irq < 64 ? (irq / 32) * 32 : (irq / 32) * 32 + 0x20;
}

static void ftintc030_irq_mask(struct irq_data *d)
{
	u32 irq = ftintc030_irq(d);
	u32 offset;
	u32 mask;

	raw_spin_lock(&ftintc030_lock);
	if (ftintc030_irq(d) < 32) {
		mask = readl(ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_EN);
		mask &= ~(1 << (irq % 32));
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_EN);
	} else {
		offset = ftintc030_register_offset(irq - 32);
		mask = readl(ftintc030_base(d) + FTINTC030_OFFSET_SPI_EN + offset);
		mask &= ~(1 << ((irq - 32) % 32));
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_SPI_EN + offset);
	}
	raw_spin_unlock(&ftintc030_lock);
}

static void ftintc030_irq_unmask(struct irq_data *d)
{
	u32 irq = ftintc030_irq(d);
	u32 offset;
	u32 mask;

	raw_spin_lock(&ftintc030_lock);
	if (ftintc030_irq(d) < 32) {
		mask = readl(ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_EN);
		mask |= (1 << (irq % 32));
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_EN);
	} else {
		offset = ftintc030_register_offset(irq - 32);
		mask = readl(ftintc030_base(d) + FTINTC030_OFFSET_SPI_EN + offset);
		mask |= 1 << ((irq - 32) % 32);
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_SPI_EN + offset);
	}
	raw_spin_unlock(&ftintc030_lock);
}

static void ftintc030_irq_eoi(struct irq_data *d)
{
	u32 irq = ftintc030_irq(d);
	u32 mask = 0;
	u32 offset;

	raw_spin_lock(&ftintc030_lock);
	if (ftintc030_irq(d) < 32) {
		mask |= 1 << (irq % 32);
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_PPI_CLEAR);
	} else {
		offset = ftintc030_register_offset(irq - 32);
		mask |= 1 << ((irq - 32) % 32);
		writel(mask, ftintc030_base(d) + FTINTC030_OFFSET_SPI_CLEAR + offset);
	}
	raw_spin_unlock(&ftintc030_lock);

	writel(irq, ftintc030_base(d) + FTINTC030_OFFSET_EOI);
}

static int ftintc030_irq_set_type(struct irq_data *d, unsigned int type)
{
	u32 irq = ftintc030_irq(d);
	u32 offset = ftintc030_register_offset(irq);
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

	raw_spin_lock(&ftintc030_lock);

	if (irq < 32) {
		val = readl(ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_MODE);
		val &= ~(1 << irq);
		val |= mode << irq;
		writel(val, ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_MODE);

		val = readl(ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_LEVEL);
		val &= ~(1 << irq);
		val |= level << irq;
		writel(val, ftintc030_base(d) + FTINTC030_OFFSET_PPI_SGI_LEVEL);
	} else {
		val = readl(ftintc030_base(d) + FTINTC030_OFFSET_SPI_MODE + offset);
		val &= ~(1 << (irq % 32));
		val |= mode << (irq % 32);
		writel(val, ftintc030_base(d) + FTINTC030_OFFSET_SPI_MODE + offset);

		val = readl(ftintc030_base(d) + FTINTC030_OFFSET_SPI_LEVEL + offset);
		val &= ~(1 << (irq % 32));
		val |= level << (irq % 32);
		writel(val, ftintc030_base(d) + FTINTC030_OFFSET_SPI_LEVEL + offset);
	}

	raw_spin_unlock(&ftintc030_lock);
	return 0;
}

static struct irq_chip ftintc030_chip = {
	.name		= "ftintc030",
	.irq_mask	= ftintc030_irq_mask,
	.irq_unmask	= ftintc030_irq_unmask,
	.irq_eoi	= ftintc030_irq_eoi,
	.irq_set_type	= ftintc030_irq_set_type,
};

static void __init ftintc030_cpu_init(void)
{
	void __iomem *base = ftintc030_data.base;

	writel(0x4, base + FTINTC030_OFFSET_INTCR);
	writel(0x7, base + FTINTC030_OFFSET_BINPOINT);
	writel(0xf, base + FTINTC030_OFFSET_PRIMASK);
}

static void __exception_irq_entry ftintc030_handle_irq(struct pt_regs *regs)
{
	unsigned int hwirq, irqnr;
	unsigned long status;

	raw_spin_lock(&ftintc030_lock);
	status = readl(ftintc030_data.base + FTINTC030_OFFSET_ACK);
	raw_spin_unlock(&ftintc030_lock);

	// spurious interrupt? 
	hwirq = status & 0x1ff;

	if (hwirq == 511)
		return;

	irqnr = irq_find_mapping(ftintc030_data.domain, hwirq);

	handle_IRQ(irqnr, regs);
}

static int ftintc030_irq_domain_map(struct irq_domain *d, unsigned int irq,
				   irq_hw_number_t hwirq)
{
	struct ftintc030_chip_data *ftintc030 = d->host_data;

	irq_set_chip_data(irq, ftintc030);
	irq_set_chip_and_handler(irq, &ftintc030_chip, handle_fasteoi_irq);
	irq_clear_status_flags(irq, IRQ_NOREQUEST | IRQ_NOPROBE);
	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_HIGH);

	return 0;
}

static struct irq_domain_ops ftintc030_irq_domain_ops = {
	.map = ftintc030_irq_domain_map,
	.xlate = irq_domain_xlate_twocell,
};

static void ftintc030_handle_cascade_irq(struct irq_desc *desc)
{
	struct ftintc030_chip_data *chip_data = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned int cascade_irq, hwirq;
	unsigned long status;

	chained_irq_enter(chip, desc);

	raw_spin_lock(&ftintc030_lock);
	status = readl(chip_data->base + FTINTC030_OFFSET_ACK);
	raw_spin_unlock(&ftintc030_lock);

	hwirq = status & 0x1ff;

	/* spurious interrupt? */
	if (hwirq == 511)
		goto out;

	cascade_irq = irq_find_mapping(chip_data->domain, hwirq);
	generic_handle_irq(cascade_irq);

out:
	chained_irq_exit(chip, desc);
}

void __init ftintc030_cascade_irq(unsigned int ftintc030_nr, unsigned int irq)
{
	if (irq_set_handler_data(irq, &ftintc030_data) != 0)
		BUG();

	irq_set_chained_handler(irq, ftintc030_handle_cascade_irq);
}

static void __init ftintc030_dist_init(void)
{
	void __iomem *base = ftintc030_data.base;
	int nr_spi = ftintc030_data.nr_spi;
	unsigned int irq;

	/* setup PPIs and SGIs, set all to be level triggered, active high. */
	writel(0, base + FTINTC030_OFFSET_PPI_SGI_EN );
	writel(0, base + FTINTC030_OFFSET_PPI_SGI_MODE );
	writel(0, base + FTINTC030_OFFSET_PPI_SGI_LEVEL );
	writel(~0, base + FTINTC030_OFFSET_PPI_CLEAR );

	/* setup SPIs, set all to be level triggered, active high. */
	for (irq = 0; irq < nr_spi; irq += 32) {
		unsigned int offset = ftintc030_register_offset(irq);

		writel(0, base + FTINTC030_OFFSET_SPI_EN + offset);
		writel(0, base + FTINTC030_OFFSET_SPI_MODE + offset);
		writel(0, base + FTINTC030_OFFSET_SPI_LEVEL + offset);
		writel(~0, base + FTINTC030_OFFSET_SPI_CLEAR + offset);
	}

	/* setup SPI target */
#ifdef FTINTC030_SPI_TARGET_INVERSE
	for (irq = 0; irq < nr_spi; irq += 32)
		writel(~0, base + FTINTC030_OFFSET_SPI_TARGET + (irq / 32) * 4);
#else		
	for (irq = 0; irq < nr_spi; irq += 4)
		writel(~0, base + FTINTC030_OFFSET_SPI_TARGET + (irq / 4) * 4);
#endif	
	
	/* setup SPI priority */
	for (irq = 0; irq < nr_spi; irq += 4)
		writel(0, base + FTINTC030_OFFSET_SPI_PRIOR + (irq / 4) * 4);
}

void __init ftintc030_init(void __iomem *base, unsigned int irq_start,
			   unsigned int cpu_match_id)
{
	int irqs;
	int irq_base;
	
	ftintc030_data.base = base;
	ftintc030_data.irq_offset = irq_start;

	/* we handle SGIs, PPIs and SPIs now */
	irqs = FTINTC030_FEAT1_SPINUM(readl(base + FTINTC030_OFFSET_FEAT1));
	irqs += 32;
	ftintc030_data.nr_spi = irqs;

	/* CPU0 bank registers for now */
	writel(cpu_match_id, base + FTINTC030_OFFSET_CPU_MATCH(0));

	ftintc030_cpu_init();
	ftintc030_dist_init();

	irq_base = irq_alloc_descs(irq_start, 0, irqs, numa_node_id());

	ftintc030_data.domain = irq_domain_add_legacy(ftintc030_np, irqs, irq_base, 0,
	                                              &ftintc030_irq_domain_ops,
	                                              &ftintc030_data);
	
	set_handle_irq(ftintc030_handle_irq);
}

#ifdef CONFIG_OF
static int ftintc030_cnt __initdata = 0;

int __init ftintc030_of_init(struct device_node *np, struct device_node *parent)
{
	void __iomem *base;
	unsigned int parent_irq, match_id;
	const __be32 *tmp;

	ftintc030_np = np;

	base = of_iomap(ftintc030_np, 0);
	if (WARN_ON(!base))
		return -ENOMEM;

	tmp = of_get_property(ftintc030_np, "cpu-match-id", NULL);
	if (tmp == NULL)
		match_id = 0x300;
	else
		match_id = be32_to_cpu(*tmp);

	ftintc030_init(base, -1, match_id);

	parent_irq = irq_of_parse_and_map(np, 0);
	if (parent_irq) {
		ftintc030_cascade_irq(ftintc030_cnt, parent_irq);
	}

	ftintc030_cnt++;

	return 0;
}
IRQCHIP_DECLARE(ftintc030_intc, "faraday,ftintc030", ftintc030_of_init);
#endif
