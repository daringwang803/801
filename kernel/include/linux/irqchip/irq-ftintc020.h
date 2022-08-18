#ifndef __FTINTC020_H
#define __FTINTC020_H

#define FTINTC020_OFFSET_IRQ_SOURCE     0x000
#define FTINTC020_OFFSET_IRQ_EN         0x004
#define FTINTC020_OFFSET_IRQ_CLEAR      0x008
#define FTINTC020_OFFSET_IRQ_MODE       0x00c
#define FTINTC020_OFFSET_IRQ_LEVEL      0x010
#define FTINTC020_OFFSET_IRQ_STATUS     0x014

#define FTINTC020_OFFSET_FIQ_SOURCE     0x020
#define FTINTC020_OFFSET_FIQ_EN         0x024
#define FTINTC020_OFFSET_FIQ_CLEAR      0x028
#define FTINTC020_OFFSET_FIQ_MODE       0x02c
#define FTINTC020_OFFSET_FIQ_LEVEL      0x030
#define FTINTC020_OFFSET_FIQ_STATUS     0x034

#define FTINTC020_OFFSET_EIRQ_SOURCE    0x060
#define FTINTC020_OFFSET_EIRQ_EN        0x064
#define FTINTC020_OFFSET_EIRQ_CLEAR     0x068
#define FTINTC020_OFFSET_EIRQ_MODE      0x06c
#define FTINTC020_OFFSET_EIRQ_LEVEL     0x070
#define FTINTC020_OFFSET_EIRQ_STATUS    0x074
#define FTINTC020_OFFSET_EIRQ_DB_EN     0x078

#define FTINTC020_OFFSET_EFIQ_SOURCE    0x080
#define FTINTC020_OFFSET_EFIQ_EN        0x084
#define FTINTC020_OFFSET_EFIQ_CLEAR     0x088
#define FTINTC020_OFFSET_EFIQ_MODE      0x08c
#define FTINTC020_OFFSET_EFIQ_LEVEL     0x090
#define FTINTC020_OFFSET_EFIQ_STATUS    0x094
#define FTINTC020_OFFSET_EFIQ_DB_EN     0x098

#define FTINTC020_OFFSET_REV            0x050
#define FTINTC020_OFFSET_FEAT           0x054
#define FTINTC020_OFFSET_IRQ_DB_EN      0x058
#define FTINTC020_OFFSET_FIQ_DE_EN      0x05C

#define FTINTC020_OFFSET_VECT_ADDR(x)   (0x100 + (x) * 4)

#define FTINTC020_OFFSET_VECT_CTRL(x)   (0x180 + (x) * 4)

#define FTINTC020_OFFSET_DEF_ADDR       0x200
#define FTINTC020_OFFSET_SEL_ADDR       0x204
#define FTINTC020_OFFSET_IRQ_PRIOR      0x208
#define FTINTC020_OFFSET_SGI_EN         0x20C
#define FTINTC020_OFFSET_SGI_CLEAR      0x210

#define FTINTC020_FEAT_IRQNUM(x)	(((x) >> 8) & 0xff)
#define FTINTC020_FEAT_FIQNUM(x)	(((x) >> 0) & 0xff)


#ifndef __ASSEMBLY__
#include <linux/irq.h>

void __init ftintc020_init(void __iomem *base, unsigned int irq_start);
void __init ftintc020_cascade_irq(unsigned int ftintc020_nr, unsigned int irq);
#ifdef CONFIG_OF
int __init ftintc020_of_init(struct device_node *np,
			     struct device_node *parent);
#endif

#endif	/* __ASSEMBLY__ */

#endif /* __FTINTC020_H */
