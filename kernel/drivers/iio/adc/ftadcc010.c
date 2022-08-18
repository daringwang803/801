/*
 * Faraday ftadcc010 ADC Controller
 *
 * (C) Copyright 2019-2020 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/events.h>

#include <asm/io.h>

struct ftadcc010 {
	struct device           *dev;
	void __iomem            *base;
	int                     irq;
};

#define	FTADCC010_REG_ADC_DATA(x)       (0x000 + (x) * 0x04)
#define FTADCC010_REG_ADC_THRHOLD(x)    (0x080 + (x) * 0x04)
#define FTADCC010_REG_TS_DATA           (0x0C0)
#define FTADCC010_REG_TS_THRHOLD        (0x0C4)
#define FTADCC010_REG_ADC_CTRL          (0x100)
#define FTADCC010_REG_ADC_MISC          (0x104)
#define FTADCC010_REG_ADC_INTEN         (0x108)
#define FTADCC010_REG_ADC_INTR          (0x10C)
#define FTADCC010_REG_ADC_TPARM         (0x110)
#define FTADCC010_REG_ADC_SMPR          (0x114)
#define FTADCC010_REG_ADC_DISCHTIME     (0x118)
#define FTADCC010_REG_ADC_PRESCAL       (0x11C)
#define FTADCC010_REG_ADC_SQR           (0x120)
#define FTADCC010_REG_DMA_DAT(x)        (0x200 + (x) * 0x10)
#define FTADCC010_REG_DMA_CTRL(x)       (0x204 + (x) * 0x10)
#define FTADCC010_REG_DMA_INTEN(x)      (0x208 + (x) * 0x10)
#define FTADCC010_REG_DMA_STS(x)        (0x20C + (x) * 0x10)

#define FTADCC010_VOLTAGE_CHANNEL(chan) {                   \
	.type = IIO_VOLTAGE,                                    \
	.indexed = 1,                                           \
	.channel = (chan),                                      \
	.address = (chan),                                      \
	.scan_index = (chan),                                   \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),           \
}

static const struct iio_chan_spec ftadcc010_channels[] = {
	FTADCC010_VOLTAGE_CHANNEL(0),
	FTADCC010_VOLTAGE_CHANNEL(1),
	FTADCC010_VOLTAGE_CHANNEL(2),
	FTADCC010_VOLTAGE_CHANNEL(3),
	FTADCC010_VOLTAGE_CHANNEL(4),
	FTADCC010_VOLTAGE_CHANNEL(5),
	FTADCC010_VOLTAGE_CHANNEL(6),
	FTADCC010_VOLTAGE_CHANNEL(7),
};

static int ftadcc010_read_raw(struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan,
                              int *val, int *val2, long mask)
{
	struct ftadcc010 *adc = iio_priv(indio_dev);
	int ret = 0;

	switch (mask) {
		case IIO_CHAN_INFO_RAW:
#ifdef CONFIG_FTADCC010_10BITS
			*val = readl(adc->base + FTADCC010_REG_ADC_DATA(chan->address)) * 33;
			*val2 = 10230;
#elif CONFIG_FTADCC010_12BITS
			*val = readl(adc->base + FTADCC010_REG_ADC_DATA(chan->address)) * 33;
			*val2 = 40950;
#endif
			ret = IIO_VAL_FRACTIONAL;
			break;
		default:
			dev_err(adc->dev, "mask %ld isn't support\n", mask);
			ret = -EINVAL;
			break;
	};

	return ret;
}

static const struct iio_info ftadcc010_info = {
	.read_raw = ftadcc010_read_raw,
	.driver_module = THIS_MODULE,
};

static void ftadcc010_hw_setup(struct ftadcc010 *adc)
{
	writel(0x00070211, adc->base + FTADCC010_REG_ADC_CTRL);
}

static irqreturn_t ftadcc010_isr(int irq, void *dev_id)
{
	struct ftadcc010 *adc = dev_id;
	u32 reg;

	/* clear interrupt */
	reg = readl(adc->base + FTADCC010_REG_ADC_INTR);
	writel(reg, adc->base + FTADCC010_REG_ADC_INTR);

	return IRQ_HANDLED;
}

static int ftadcc010_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct device *dev = &pdev->dev;
	struct ftadcc010 *adc;
	struct resource *res;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*adc));
	if (!indio_dev)
		return -ENOMEM;

	adc = iio_priv(indio_dev);

	/* find the resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adc->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(adc->base)) {
		ret = PTR_ERR(adc->base);
		goto map_failed;
	}

	/* find the irq */
	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(dev, "failed to get the irq.\n");
		goto map_failed;
	}
	adc->irq = ret;

	ret = devm_request_irq(dev, ret, ftadcc010_isr, 0, pdev->name, adc);
	if (ret) {
		dev_err(dev, "failed to request irq.\n");
		goto irq_failed;
	}

	adc->dev = &pdev->dev;

	ftadcc010_hw_setup(adc);

	indio_dev->dev.parent = dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->name = pdev->name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &ftadcc010_info;

	/* Setup the ADC channels available on the board */
	indio_dev->num_channels = ARRAY_SIZE(ftadcc010_channels);
	indio_dev->channels = ftadcc010_channels;

	platform_set_drvdata(pdev, adc);

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(dev, "Couldn't register the device.\n");
		goto err_device_register;
	}

	return 0;

irq_failed:
map_failed:
	devm_iio_device_free(dev, indio_dev);
err_device_register:
	return ret;
}

static int ftadcc010_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ftadcc010_suspend(struct device *dev)
{
	return 0;
}

static int ftadcc010_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ftadcc010_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ftadcc010_suspend, ftadcc010_resume)
};
#define DEV_PM_OPS	(&ftadcc010_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id ftadcc010_dt_match[] = {
	{.compatible = "faraday,ftadcc010" },
	{},
};
MODULE_DEVICE_TABLE(of, ftadcc010_dt_match);

static struct platform_driver ftadcc010_driver = {
	.probe		= ftadcc010_probe,
	.remove		= ftadcc010_remove,
	.driver		= {
		.name	= "ftadcc010",
		.owner	= THIS_MODULE,
		.of_match_table = ftadcc010_dt_match,
		.pm     = DEV_PM_OPS,
	},
};
module_platform_driver(ftadcc010_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTADCC010 driver for ADC");
