#ifndef __FTINTC030_H
#define __FTINTC030_H

#define FTINTC030_OFFSET_SPI_SOURCE	0x000
#define FTINTC030_OFFSET_SPI_EN		0x004
#define FTINTC030_OFFSET_SPI_CLEAR	0x008
#define FTINTC030_OFFSET_SPI_MODE	0x00c
#define FTINTC030_OFFSET_SPI_LEVEL	0x010
#define FTINTC030_OFFSET_SPI_PENDING	0x014

#define FTINTC030_OFFSET_SPI_PRIOR	0x240
#define FTINTC030_OFFSET_SPI_TARGET	0x420

#define FTINTC030_OFFSET_PRIMASK	0x684
#define FTINTC030_OFFSET_BINPOINT	0x688
#define FTINTC030_OFFSET_ACK		0x68c
#define FTINTC030_OFFSET_EOI		0x690
#define FTINTC030_OFFSET_INTCR		0x69c

#define FTINTC030_OFFSET_CPU_MATCH(cpu)	(0x6a0 + (cpu) * 4)

#define FTINTC030_OFFSET_FEAT1		0x700
#define FTINTC030_OFFSET_FEAT2		0x704
#define FTINTC030_OFFSET_REV		0x700

#define FTINTC030_FEAT1_CPUNUM(x)	(((x) >> 16) & 0xf)
#define FTINTC030_FEAT1_SGINUM(x)	(((x) >> 11) & 0x1f)
#define FTINTC030_FEAT1_SPINUM(x)	(((x) >> 2) & 0x1ff)

//For HW FTINTC030_SPI_TARGET_INVERSE define
#define FTINTC030_SPI_TARGET_INVERSE

#ifndef __ASSEMBLY__
#include <linux/irq.h>

void __init ftintc030_init(void __iomem *base, unsigned int irq_start,
			   unsigned int cpu_match_id);
void __init ftintc030_cascade_irq(unsigned int ftintc030_nr, unsigned int irq);
#ifdef CONFIG_OF
int __init ftintc030_of_init(struct device_node *np,
			     struct device_node *parent);
#endif

#endif	/* __ASSEMBLY__ */

#endif /* __FTINTC030_H */
