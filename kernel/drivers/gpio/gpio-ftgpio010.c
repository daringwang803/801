#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include "gpio-ftgpio010.h"


#define FTGPIO010_NR_GPIOS	32

struct ftgpio010 {
	struct gpio_chip 	gpio_chip;
	void __iomem 		*base;
	struct resource		*res;
	int 			irq;
};

/******************************************************************************
 * internal functions
 *****************************************************************************/

static inline struct ftgpio010 *to_ftgpio010(struct gpio_chip *chip)
{
	return container_of(chip, struct ftgpio010, gpio_chip);
}

static inline void ftgpio010_data_set(struct ftgpio010 *ftgpio010, unsigned offset, int value)
{
	
	if (value)
		iowrite32(1 << offset, ftgpio010->base + FTGPIO010_OFFSET_DATASET);
	else
		iowrite32(1 << offset, ftgpio010->base + FTGPIO010_OFFSET_DATACLEAR);
}

static void ftgpio010_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);

	ftgpio010_data_set(ftgpio010, offset, value);
}

static int ftgpio010_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	unsigned long val;

	val = ioread32(ftgpio010->base + FTGPIO010_OFFSET_DATAIN);
	return (val >> offset) & 1;
}

static int ftgpio010_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	unsigned long val;

	val = ioread32(ftgpio010->base + FTGPIO010_OFFSET_PINDIR);
	val &= ~(1 << offset);
	iowrite32(val, ftgpio010->base + FTGPIO010_OFFSET_PINDIR);

	return 0;
}

static int
ftgpio010_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	unsigned long val;

	val = ioread32(ftgpio010->base + FTGPIO010_OFFSET_PINDIR);
	val |= 1 << offset;
	iowrite32(val, ftgpio010->base + FTGPIO010_OFFSET_PINDIR);
	
	ftgpio010_data_set(ftgpio010, offset, value);

	return 0;
}

static struct gpio_chip ftgpio010_gpio_chip = {
	.direction_input	= ftgpio010_gpio_direction_input,
	.get			= ftgpio010_gpio_get,
	.direction_output	= ftgpio010_gpio_direction_output,
	.set			= ftgpio010_gpio_set,
	.ngpio			= FTGPIO010_NR_GPIOS,
};

static void __init ftgpio010_init(struct ftgpio010 *ftgpio010)
{
	/* input pins for interrupt */
	iowrite32(0, ftgpio010->base + FTGPIO010_OFFSET_PINDIR);

	/* sanitize */
	iowrite32(~0, ftgpio010->base + FTGPIO010_OFFSET_INTR_CLEAR);

	/* unmask all */
	iowrite32(0, ftgpio010->base + FTGPIO010_OFFSET_INTR_MASK);

	/* single-falling edge trigger */
	iowrite32(0, ftgpio010->base + FTGPIO010_OFFSET_INTR_TRIG);
	iowrite32(0, ftgpio010->base + FTGPIO010_OFFSET_INTR_BOTH);
	iowrite32(~0, ftgpio010->base + FTGPIO010_OFFSET_INTR_RISENEG);

	/* enable max bounce prescale */
	iowrite32(~0, ftgpio010->base + FTGPIO010_OFFSET_BOUNCE_EN);
	//iowrite32(~0, ftgpio010->base + FTGPIO010_OFFSET_BOUNCE_PRESC);

	/* enable interrupt */
	iowrite32(~0, ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
}

static void ftgpio010_reset(struct ftgpio010 *ftgpio010)
{
	/* disable interrupt */
	iowrite32(0, ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
}

static void ftgpio010_irq_ack(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);

	writel(BIT(irqd_to_hwirq(d)), ftgpio010->base + FTGPIO010_OFFSET_INTR_CLEAR);
}

static void ftgpio010_irq_mask(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	u32 val;

	val = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
	val &= ~BIT(irqd_to_hwirq(d));
	writel(val, ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
}

static void ftgpio010_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	u32 val;

	val = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
	val |= BIT(irqd_to_hwirq(d));
	writel(val, ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
}

static int ftgpio010_irq_type_set(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	u32 mask = BIT(irqd_to_hwirq(d));
	u32 reg_both, reg_level, reg_type;

	reg_type = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_TRIG);
	reg_level = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_RISENEG);
	reg_both = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_BOTH);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_BOTH:
		irq_set_handler_locked(d, handle_edge_irq);
		reg_type &= ~mask;
		reg_both |= mask;
		break;
	case IRQ_TYPE_EDGE_RISING:
		irq_set_handler_locked(d, handle_edge_irq);
		reg_type &= ~mask;
		reg_both &= ~mask;
		reg_level &= ~mask;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		irq_set_handler_locked(d, handle_edge_irq);
		reg_type &= ~mask;
		reg_both &= ~mask;
		reg_level |= mask;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		irq_set_handler_locked(d, handle_level_irq);
		reg_type |= mask;
		reg_level &= ~mask;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		irq_set_handler_locked(d, handle_level_irq);
		reg_type |= mask;
		reg_level |= mask;
		break;
	default:
		irq_set_handler_locked(d, handle_bad_irq);
		return -EINVAL;
	}

	writel(reg_type, ftgpio010->base + FTGPIO010_OFFSET_INTR_TRIG);
	writel(reg_level, ftgpio010->base + FTGPIO010_OFFSET_INTR_RISENEG);
	writel(reg_both, ftgpio010->base + FTGPIO010_OFFSET_INTR_BOTH);

	ftgpio010_irq_ack(d);

	return 0;
}

static struct irq_chip ftgpio010_irqchip = {
	.name         = "ftgpio010",
	.irq_ack      = ftgpio010_irq_ack,
	.irq_mask     = ftgpio010_irq_mask,
	.irq_unmask   = ftgpio010_irq_unmask,
	.irq_set_type = ftgpio010_irq_type_set,
};

static void ftgpio010_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *chip = irq_desc_get_handler_data(desc);
	struct ftgpio010 *ftgpio010 = to_ftgpio010(chip);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	int offset;
	unsigned long stat;

	chained_irq_enter(irqchip, desc);

	stat = readl(ftgpio010->base + FTGPIO010_OFFSET_INTR_STATE);
	if (stat)
		for_each_set_bit(offset, &stat, chip->ngpio){
			generic_handle_irq(irq_find_mapping(chip->irqdomain,
							    offset));
		}
	chained_irq_exit(irqchip, desc);
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftgpio010_probe(struct platform_device *pdev)
{
	struct ftgpio010 *ftgpio010;
	struct resource *res;
	int dev_id;
	int irq;
	int ret;

	/* get resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "No mmio resource defined.\n");
		return -ENXIO;
	}

	irq = platform_get_irq(pdev, 0);
	if (!irq) {
		dev_err(&pdev->dev, "Failed to get irq.\n");
		return -ENXIO;
	}

	/* allocate driver private data */
	ftgpio010 = devm_kzalloc(&pdev->dev, sizeof(*ftgpio010), GFP_KERNEL);
	if (!ftgpio010) {
		dev_err(&pdev->dev, "Failed to allocate memory.\n");
		return -ENOMEM;
	}

	/* map io memory */
	ftgpio010->res = request_mem_region(res->start, resource_size(res),
	                                    dev_name(&pdev->dev));
	if (!ftgpio010->res) {
		dev_err(&pdev->dev, "Resources is unavailable.\n");
		ret = -EBUSY;
		goto err_req_mem_region;
	}

	ftgpio010->base = ioremap(res->start, resource_size(res));
	if (!ftgpio010->base) {
		dev_err(&pdev->dev, "Failed to map registers.\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	ftgpio010->irq = irq;
	ftgpio010->gpio_chip = ftgpio010_gpio_chip;
	ftgpio010->gpio_chip.label = dev_name(&pdev->dev);
	ftgpio010->gpio_chip.parent = &pdev->dev;
	if (pdev->dev.of_node) {
		of_property_read_u32(pdev->dev.of_node, "dev_id", &dev_id);
		ftgpio010->gpio_chip.base = dev_id * FTGPIO010_NR_GPIOS;
	} else {
		ftgpio010->gpio_chip.base = pdev->id * FTGPIO010_NR_GPIOS;
	}
	ftgpio010->gpio_chip.of_node = pdev->dev.of_node;

	ftgpio010_init(ftgpio010);

	/* register gpio */
	ret = gpiochip_add(&ftgpio010->gpio_chip);
	if (ret) {
		dev_err(&pdev->dev, "gpiochip_add() failed, %d\n", ret);
		goto err_chip;
	}

	/* Disable, unmask and clear all interrupts */
	writel(0x0, ftgpio010->base + FTGPIO010_OFFSET_INTR_EN);
	writel(0x0, ftgpio010->base + FTGPIO010_OFFSET_INTR_MASK);
	writel(~0x0, ftgpio010->base + FTGPIO010_OFFSET_INTR_CLEAR);

	ret = gpiochip_irqchip_add(&ftgpio010->gpio_chip, &ftgpio010_irqchip,
				   0, handle_bad_irq,
				   IRQ_TYPE_NONE);
	if (ret) {
		dev_err(&pdev->dev, "gpiochip_irqchip_add() failed, %d\n", ret);
		goto err_irqchip;
	}
	gpiochip_set_chained_irqchip(&ftgpio010->gpio_chip, &ftgpio010_irqchip,
				     irq, ftgpio010_irq_handler);

	/* set private data */
	platform_set_drvdata(pdev, ftgpio010);

	dev_info(&pdev->dev, "Start ftgpio010.\n");
	return 0;

err_irqchip:
	gpiochip_remove(&ftgpio010->gpio_chip);
err_chip:
	ftgpio010_reset(ftgpio010);
	iounmap(ftgpio010->base);
err_ioremap:
	release_mem_region(res->start, resource_size(res));
err_req_mem_region:
	platform_set_drvdata(pdev, NULL);
	kfree(ftgpio010);
	release_resource(res);
	return ret;
}

static int __exit ftgpio010_remove(struct platform_device *pdev)
{
	struct ftgpio010 *ftgpio010 = platform_get_drvdata(pdev);
	struct resource *res = ftgpio010->res;

	ftgpio010_reset(ftgpio010);
	free_irq(ftgpio010->irq, ftgpio010);
	iounmap(ftgpio010->base);
	gpiochip_remove(&ftgpio010->gpio_chip);
	release_resource(res);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id ftgpio010_dt_ids[] = {
	{ .compatible = "faraday,ftgpio010", },
	{},
};

static struct platform_driver ftgpio010_driver = {
	.probe		= ftgpio010_probe,
	.remove		= __exit_p(ftgpio010_remove),
	.driver		= {
		.name	= "ftgpio010",
		.owner	= THIS_MODULE,
		.of_match_table = ftgpio010_dt_ids,
	},
};

module_platform_driver(ftgpio010_driver);

MODULE_AUTHOR("Yan-Pai Chen <ypchen@faraday-tech.com>");
MODULE_DESCRIPTION("FTGPIO010 GPIO Driver");
MODULE_LICENSE("GPL");
