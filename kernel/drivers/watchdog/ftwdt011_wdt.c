/*
 * Faraday FTWDT11 WatchDog Timer
 *
 * (C) Copyright 2012 Faraday Technology
 * Yan-Pai Chen <ypchen@faraday-tech.com>
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/watchdog.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>

#include <asm/io.h>

#define FTWDT011_OFFSET_KR          0x00
#define FTWDT011_OFFSET_CURR        0x00
#define FTWDT011_OFFSET_PR          0x04
#define FTWDT011_OFFSET_RLR         0x08
#define FTWDT011_OFFSET_SR          0x0c
#define FTWDT011_OFFSET_INTSR       0x10
#define FTWDT011_OFFSET_CR          0x14
#define FTWDT011_OFFSET_INTRLEN     0x18
#define FTWDT011_OFFSET_UDF         0x1c
#define FTWDT011_OFFSET_REVISON     0x10c

/*
 * Control register
 */
#define FTWDT011_CR_ENABLE          (1 << 0)
#define FTWDT011_CR_INTR            (1 << 1)
#define FTWDT011_CR_RST             (1 << 2)
#define FTWDT011_CR_EXT             (1 << 3)
#define FTWDT011_CR_PCLK            (0 << 4)
#define FTWDT011_CR_EXTCLK          (1 << 4)

#define FTWDT011_DEFAULT_TIMEOUT    60	/* in second */

#define UDFSHIFTBITS                16
#define OVFSHIFTBITS                24

#define WdIntrMask                  0xFF
#define WdUDFIntrMask               0xFF0000
#define WdOVFIntrMask               0xFF000000

 /* KEY Definition */
#define KICKDOG_KEY                 0xAAAA
#define REPROG_KEY                  0x5555
#define START_KEY                   0xCCCC

struct ftwdt011 {
	struct watchdog_device  wdd;
	struct device           *dev;
	struct resource         *res;
	void __iomem            *base;
	struct clk              *clk;
	int                     irq;
	spinlock_t              lock;
	unsigned int            counter;
};

static void ftwdt011_clear_interrupt(struct ftwdt011 *ftwdt011)
{
	unsigned long intsr;

	intsr = ioread32(ftwdt011->base + FTWDT011_OFFSET_INTSR);
	iowrite32(intsr, ftwdt011->base + FTWDT011_OFFSET_INTSR);
}

static void __ftwdt011_stop(struct ftwdt011 *ftwdt011)
{
	iowrite32(REPROG_KEY, ftwdt011->base + FTWDT011_OFFSET_KR);
	while(ioread32(ftwdt011->base + FTWDT011_OFFSET_CR) & FTWDT011_CR_ENABLE);

	return;
}

static int ftwdt011_start(struct watchdog_device *wdd)
{
	struct ftwdt011 *ftwdt011 = watchdog_get_drvdata(wdd);
	unsigned long cr;
	unsigned long flags;

	spin_lock_irqsave(&ftwdt011->lock, flags);

	__ftwdt011_stop(ftwdt011);

	cr = ioread32(ftwdt011->base + FTWDT011_OFFSET_CR);
	cr |= FTWDT011_CR_INTR;
	cr |= FTWDT011_CR_RST;
	iowrite32(cr, ftwdt011->base + FTWDT011_OFFSET_CR);
	iowrite32(0xFF, ftwdt011->base + FTWDT011_OFFSET_INTRLEN);

	iowrite32(ftwdt011->counter, ftwdt011->base + FTWDT011_OFFSET_RLR);
	iowrite32(START_KEY, ftwdt011->base + FTWDT011_OFFSET_KR);

	spin_unlock_irqrestore(&ftwdt011->lock, flags);

	return 0;
}

static int ftwdt011_stop(struct watchdog_device *wdd)
{
	struct ftwdt011 *ftwdt011 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt011->lock, flags);
	__ftwdt011_stop(ftwdt011);
	spin_unlock_irqrestore(&ftwdt011->lock, flags);

	return 0;
}

static int ftwdt011_ping(struct watchdog_device *wdd)
{
	struct ftwdt011 *ftwdt011 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt011->lock, flags);
	iowrite32(START_KEY, ftwdt011->base + FTWDT011_OFFSET_KR);
	spin_unlock_irqrestore(&ftwdt011->lock, flags);

	return 0;
}

static int ftwdt011_set_timeout(struct watchdog_device *wdd,
				unsigned int timeout)
{
	struct ftwdt011 *ftwdt011 = watchdog_get_drvdata(wdd);
	unsigned long clkrate = clk_get_rate(ftwdt011->clk);
	unsigned int counter;
	unsigned long flags;

	/* WdCounter is 32bit register. If the counter is larger than
	   0xFFFFFFFF, set the counter as 0xFFFFFFFF*/
	if (clkrate > (0xFFFFFFFF / timeout / 4))
		counter = 0xFFFFFFFF;
	else
		counter = clkrate * timeout / 4;

	spin_lock_irqsave(&ftwdt011->lock, flags);
	ftwdt011->counter = counter;
	__ftwdt011_stop(ftwdt011);
	iowrite32(counter, ftwdt011->base + FTWDT011_OFFSET_RLR);
	iowrite32(KICKDOG_KEY, ftwdt011->base + FTWDT011_OFFSET_KR);
	spin_unlock_irqrestore(&ftwdt011->lock, flags);

	return 0;
}

static const struct watchdog_info ftwdt011_info = {
	.options          = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity         = "FTWDT011 Watchdog",
};

static struct watchdog_ops ftwdt011_ops = {
	.owner = THIS_MODULE,
	.start = ftwdt011_start,
	.stop = ftwdt011_stop,
	.ping = ftwdt011_ping,
	.set_timeout = ftwdt011_set_timeout,
};

static irqreturn_t ftwdt011_interrupt(int irq, void *dev_id)
{
	struct ftwdt011 *ftwdt011 = dev_id;

	dev_info(ftwdt011->dev, "watchdog timer expired\n");

	ftwdt011_ping(&ftwdt011->wdd);
	ftwdt011_clear_interrupt(ftwdt011);

	return IRQ_HANDLED;
}

static int ftwdt011_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftwdt011 *ftwdt011;
	struct resource *res;
	int irq;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "No mmio resource defined.\n");
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		dev_err(dev, "Failed to get irq.\n");
		ret = -ENXIO;
		goto err_get_irq;
	}

	/* Allocate driver private data */
	ftwdt011 = kzalloc(sizeof(struct ftwdt011), GFP_KERNEL);
	if (!ftwdt011) {
		dev_err(dev, "Failed to allocate memory.\n");
		ret = -ENOMEM;
		goto err_alloc_priv;
	}

	/* Setup watchdog_device */
	ftwdt011->wdd.info = &ftwdt011_info;
	ftwdt011->wdd.ops = &ftwdt011_ops;

	ftwdt011->dev = dev;
	platform_set_drvdata(pdev, ftwdt011);
	watchdog_set_drvdata(&ftwdt011->wdd, ftwdt011);

	/* Setup clock */
#ifdef CONFIG_OF
	ftwdt011->clk = devm_clk_get(dev, NULL);
#else
	ftwdt011->clk = devm_clk_get(dev, "pclk");
#endif
	if (IS_ERR(ftwdt011->clk)) {
		dev_err(dev, "Failed to get clock.\n");
		ret = PTR_ERR(ftwdt011->clk);
		goto err_clk_get;
	}
	ret = clk_prepare_enable(ftwdt011->clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}

	/* Map io memory */
	ftwdt011->res = request_mem_region(res->start, resource_size(res),
					   dev_name(dev));
	if (!ftwdt011->res) {
		dev_err(dev, "Resources is unavailable.\n");
		ret = -EBUSY;
		goto err_req_mem_region;
	}

	ftwdt011->base = ioremap(res->start, resource_size(res));
	if (!ftwdt011->base) {
		dev_err(dev, "Failed to map registers.\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* Register irq */
	ret = request_irq(irq, ftwdt011_interrupt, 0, pdev->name, ftwdt011);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}
	ftwdt011->irq = irq;

	spin_lock_init(&ftwdt011->lock);
	/* Set the default timeout (60 sec). If the clock source (PCLK) is
	   larger than 71582788 Hz, the reset time will be less than 60 sec.
	   For A380, PCLK is 99 MHz, the maximum reset time is 43 sec.*/
	ftwdt011_set_timeout(&ftwdt011->wdd, FTWDT011_DEFAULT_TIMEOUT);	

	/* Register watchdog */
	ret = watchdog_register_device(&ftwdt011->wdd);
	if (ret) {
		dev_err(dev, "cannot register watchdog (%d)\n", ret);
		goto err_register_wdd;
	}

	dev_info(dev, "start watchdog timer\n");
	ftwdt011_start(&ftwdt011->wdd);

	dev_info(dev, "irq %d, mapped at %p\n", irq, ftwdt011->base);
	return 0;

err_register_wdd:
	free_irq(irq, ftwdt011);
err_req_irq:
	iounmap(ftwdt011->base);
err_ioremap:
	release_mem_region(res->start, resource_size(res));
err_req_mem_region:
	clk_disable_unprepare(ftwdt011->clk);
err_clk_get:
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt011);
err_alloc_priv:
err_get_irq:
	release_resource(res);
	return ret;
}

static int ftwdt011_remove(struct platform_device *pdev)
{
	struct ftwdt011 *ftwdt011 = platform_get_drvdata(pdev);
	struct resource *res = ftwdt011->res;

	watchdog_unregister_device(&ftwdt011->wdd);

	free_irq(ftwdt011->irq, ftwdt011);
	iounmap(ftwdt011->base);
	clk_disable_unprepare(ftwdt011->clk);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt011);
	release_resource(res);

	return 0;
}

static const struct of_device_id ftwdt011_wdt_of_match[] = {
	{ .compatible = "faraday,ftwdt011", },
	{ },
};
MODULE_DEVICE_TABLE(of, ftwdt011_wdt_of_match);

static struct platform_driver ftwdt011_driver = {
	.probe		= ftwdt011_probe,
	.remove		= ftwdt011_remove,
	.driver		= {
		.owner  = THIS_MODULE,
		.name	= "ftwdt011",
		.of_match_table = ftwdt011_wdt_of_match,
	},
};

module_platform_driver(ftwdt011_driver);

MODULE_AUTHOR("Yan-Pai Chen <ypchen@faraday-tech.com>");
MODULE_DESCRIPTION("FTWDT011 WatchDog Timer Driver");
MODULE_LICENSE("GPL");
