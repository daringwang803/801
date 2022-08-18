/*
 * Faraday FTWDT10 WatchDog Timer
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

#define FTWDT010_OFFSET_COUNTER 0x00
#define FTWDT010_OFFSET_LOAD    0x04
#define FTWDT010_OFFSET_RESTART 0x08
#define FTWDT010_OFFSET_CR      0x0c
#define FTWDT010_OFFSET_STATUS  0x10
#define FTWDT010_OFFSET_CLEAR   0x14
#define FTWDT010_OFFSET_INTRLEN 0x18
#define FTWDT010_OFFSET_REVISON 0x1c

/*
 * Control register
 */
#define FTWDT010_CR_ENABLE      (1 << 0)
#define FTWDT010_CR_RST         (1 << 1)
#define FTWDT010_CR_INTR        (1 << 2)
#define FTWDT010_CR_EXT         (1 << 3)
#define FTWDT010_CR_PCLK        (0 << 4)
#define FTWDT010_CR_EXTCLK      (1 << 4)

#define FTWDT010_DEFAULT_TIMEOUT	60	/* in second */

#define UNLOCK_ACCESS	0x5ab9


struct ftwdt010 {
	struct watchdog_device	wdd;
	struct device 		*dev;
	struct resource 	*res;
	void __iomem		*base;
	struct clk		*clk;
	int			irq;
	spinlock_t		lock;
	unsigned int		counter;
};

static void ftwdt010_clear_interrupt(struct ftwdt010 *ftwdt010)
{
	iowrite32(0, ftwdt010->base + FTWDT010_OFFSET_CLEAR);
}

static void __ftwdt010_stop(struct ftwdt010 *ftwdt010)
{
	unsigned long cr;

	cr = ioread32(ftwdt010->base + FTWDT010_OFFSET_CR);
	cr &= ~FTWDT010_CR_ENABLE;
	iowrite32(cr, ftwdt010->base + FTWDT010_OFFSET_CR);

	return;
}

static int ftwdt010_start(struct watchdog_device *wdd)
{
	struct ftwdt010 *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long cr;
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);

	__ftwdt010_stop(ftwdt010);

	cr = ioread32(ftwdt010->base + FTWDT010_OFFSET_CR);
	cr |= FTWDT010_CR_ENABLE | FTWDT010_CR_RST | FTWDT010_CR_INTR;

	iowrite32(ftwdt010->counter, ftwdt010->base + FTWDT010_OFFSET_LOAD);
	iowrite32(UNLOCK_ACCESS, ftwdt010->base + FTWDT010_OFFSET_RESTART);
	iowrite32(cr, ftwdt010->base + FTWDT010_OFFSET_CR);

	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_stop(struct watchdog_device *wdd)
{
	struct ftwdt010 *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	__ftwdt010_stop(ftwdt010);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_ping(struct watchdog_device *wdd)
{
	struct ftwdt010 *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	iowrite32(UNLOCK_ACCESS, ftwdt010->base + FTWDT010_OFFSET_RESTART);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_set_timeout(struct watchdog_device *wdd,
				unsigned int timeout)
{
	struct ftwdt010 *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long clkrate = clk_get_rate(ftwdt010->clk);
	unsigned int counter;
	unsigned long flags;

	/* WdCounter is 32bit register. If the counter is larger than
	   0xFFFFFFFF, set the counter as 0xFFFFFFFF*/
	if (clkrate > 0xFFFFFFFF / timeout)
		counter = 0xFFFFFFFF;
	else
		counter = clkrate * timeout;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	ftwdt010->counter = counter;
	iowrite32(counter, ftwdt010->base + FTWDT010_OFFSET_LOAD);
	iowrite32(UNLOCK_ACCESS, ftwdt010->base + FTWDT010_OFFSET_RESTART);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static const struct watchdog_info ftwdt010_info = {
	.options          = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity         = "FTWDT010 Watchdog",
};

static struct watchdog_ops ftwdt010_ops = {
	.owner = THIS_MODULE,
	.start = ftwdt010_start,
	.stop = ftwdt010_stop,
	.ping = ftwdt010_ping,
	.set_timeout = ftwdt010_set_timeout,
};

static irqreturn_t ftwdt010_interrupt(int irq, void *dev_id)
{
	struct ftwdt010 *ftwdt010 = dev_id;

	dev_info(ftwdt010->dev, "watchdog timer expired\n");

	ftwdt010_ping(&ftwdt010->wdd);
	ftwdt010_clear_interrupt(ftwdt010);
	return IRQ_HANDLED;
}

static int ftwdt010_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftwdt010 *ftwdt010;
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
		goto err_get_irq;
		return -ENXIO;
	}

	/* Allocate driver private data */
	ftwdt010 = kzalloc(sizeof(struct ftwdt010), GFP_KERNEL);
	if (!ftwdt010) {
		dev_err(dev, "Failed to allocate memory.\n");
		ret = -ENOMEM;
		goto err_alloc_priv;
	}

	/* Setup watchdog_device */
	ftwdt010->wdd.info = &ftwdt010_info;
	ftwdt010->wdd.ops = &ftwdt010_ops;

	ftwdt010->dev = dev;
	platform_set_drvdata(pdev, ftwdt010);
	watchdog_set_drvdata(&ftwdt010->wdd, ftwdt010);

	/* Setup clock */
	ftwdt010->clk = devm_clk_get(dev, "pclk");
	if (IS_ERR(ftwdt010->clk)) {
		dev_err(dev, "Failed to get clock.\n");
		ret = PTR_ERR(ftwdt010->clk);
		goto err_clk_get;
	}
	ret = clk_prepare_enable(ftwdt010->clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}

	/* Map io memory */
	ftwdt010->res = request_mem_region(res->start, resource_size(res),
					   dev_name(dev));
	if (!ftwdt010->res) {
		dev_err(dev, "Resources is unavailable.\n");
		ret = -EBUSY;
		goto err_req_mem_region;
	}

	ftwdt010->base = ioremap(res->start, resource_size(res));
	if (!ftwdt010->base) {
		dev_err(dev, "Failed to map registers.\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* Register irq */
	ret = request_irq(irq, ftwdt010_interrupt, 0, pdev->name, ftwdt010);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}
	ftwdt010->irq = irq;

	spin_lock_init(&ftwdt010->lock);
	/* Set the default timeout (60 sec). If the clock source (PCLK) is
	   larger than 71582788 Hz, the reset time will be less than 60 sec.
	   For A380, PCLK is 99 MHz, the maximum reset time is 43 sec.*/
	ftwdt010_set_timeout(&ftwdt010->wdd, FTWDT010_DEFAULT_TIMEOUT);	

	/* Register watchdog */
	ret = watchdog_register_device(&ftwdt010->wdd);
	if (ret) {
		dev_err(dev, "cannot register watchdog (%d)\n", ret);
		goto err_register_wdd;
	}

	dev_info(dev, "start watchdog timer\n");
	ftwdt010_start(&ftwdt010->wdd);

	dev_info(dev, "irq %d, mapped at %p\n", irq, ftwdt010->base);
	return 0;

err_register_wdd:
	free_irq(irq, ftwdt010);
err_req_irq:
	iounmap(ftwdt010->base);
err_ioremap:
	release_mem_region(res->start, resource_size(res));
err_req_mem_region:
	clk_disable_unprepare(ftwdt010->clk);
err_clk_get:
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt010);
err_alloc_priv:
err_get_irq:
	release_resource(res);
	return ret;
}

static int ftwdt010_remove(struct platform_device *pdev)
{
	struct ftwdt010 *ftwdt010 = platform_get_drvdata(pdev);
	struct resource *res = ftwdt010->res;

	watchdog_unregister_device(&ftwdt010->wdd);

	free_irq(ftwdt010->irq, ftwdt010);
	iounmap(ftwdt010->base);
	clk_disable_unprepare(ftwdt010->clk);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt010);
	release_resource(res);

	return 0;
}

static const struct of_device_id ftwdt010_wdt_of_match[] = {
	{ .compatible = "faraday,ftwdt010", },
	{ },
};
MODULE_DEVICE_TABLE(of, ftwdt010_wdt_of_match);

static struct platform_driver ftwdt010_driver = {
	.probe		= ftwdt010_probe,
	.remove		= ftwdt010_remove,
	.driver		= {
		.owner  = THIS_MODULE,
		.name	= "ftwdt010",
		.of_match_table = ftwdt010_wdt_of_match,
	},
};

module_platform_driver(ftwdt010_driver);

MODULE_AUTHOR("Yan-Pai Chen <ypchen@faraday-tech.com>");
MODULE_DESCRIPTION("FTWDT010 WatchDog Timer Driver");
MODULE_LICENSE("GPL");
