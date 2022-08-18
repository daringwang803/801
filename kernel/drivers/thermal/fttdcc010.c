/*
 * Driver for FTDCC010 Temperature-To-Digital Converter Controller
 *
 * Copyright (c) 2016 Faraday Technology Corporation
 *
 * B.C. Chen <bcchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/thermal.h>

#include "fttdcc010.h"

#define DRIVER_DESC     "FTDCC010 Temperature-To-Digital Converter Controller Driver"
#define DRIVER_VERSION  "28-Sep-2017"

#define NUM_TDC         1
#define SCANMODE        SCANMODE_CONT

static void fttdcc010_force_update_temp(struct fttdcc010_thermal_priv *priv, int id)
{
	struct fttdcc010_thermal_zone *zone;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);
	u32 val;
	
	val = readl(&priv->regs->ctrl);
	writel(val | TDC_EN, &priv->regs->ctrl);
	
	list_for_each_entry(zone, &priv->head, list) {
		if(zone->id != id)
			continue;
		
		if(!wait_for_completion_timeout(&zone->done, msecs_to_jiffies(1000)))
			dev_err(dev, "ftddcc010 conversion timeout\n");
	}
}

static int fttdcc010_get_temp(struct thermal_zone_device *thermal,
                              int *temp)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	u32 val;
	
	if(SCANMODE == SCANMODE_SGL)
		fttdcc010_force_update_temp(priv, thermal->id);
	
	val = readl(&priv->regs->data[thermal->id]);
	
	if(val >= 374)
		*temp = (val - 374)/4;
	else
		*temp = (374 - val)/4;
	
	return 0;
}

static int fttdcc010_get_mode(struct thermal_zone_device *thermal,
                              enum thermal_device_mode *mode)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	u32 val;

	val = readl(&priv->regs->ctrl);
	if(val & TDC_EN)
		*mode = THERMAL_DEVICE_ENABLED;
	else
		*mode = THERMAL_DEVICE_DISABLED;
	
	return 0;
}

static int fttdcc010_set_mode(struct thermal_zone_device *thermal,
                              enum thermal_device_mode mode)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);
	u32 val;
	
	switch (mode) {
		case THERMAL_DEVICE_DISABLED:
			dev_info(dev, "Disable FTTDCC010 conversion function\n");
			val = readl(&priv->regs->ctrl);
			val &= ~TDC_EN;
			writel(val, &priv->regs->ctrl);
			break;
		case THERMAL_DEVICE_ENABLED:
			dev_info(dev, "Enable FTTDCC010 conversion function\n");
			val = readl(&priv->regs->ctrl);
			val |= TDC_EN;
			writel(val, &priv->regs->ctrl);
			break;
	}
	
	return 0;
}

static int fttdcc010_get_trip_type(struct thermal_zone_device *thermal,
                                   int trip, enum thermal_trip_type *type)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct fttdcc010_thermal_zone *zone;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);
	
	list_for_each_entry(zone, &priv->head, list) {
		if(zone->id != thermal->id)
			continue;

		if(trip >= zone->ntrips) {
			dev_err(dev, "get trip type error\n");
			return -EINVAL;
		} else {
			*type = zone->trips[0].type;
		}
	}
	
	return 0;
}

static int fttdcc010_get_trip_temp(struct thermal_zone_device *thermal,
                                   int trip, int *temp)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct fttdcc010_thermal_zone *zone;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);

	list_for_each_entry(zone, &priv->head, list) {
		if(zone->id != thermal->id)
			continue;

		if(trip >= zone->ntrips) {
			dev_err(dev, "get trip temp error\n");
			return -EINVAL;
		} else {
			*temp = zone->trips[0].temperature;
		}
	}
	
	return 0;
}

static int fttdcc010_set_trip_temp(struct thermal_zone_device *thermal,
                                   int trip, int temp)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct fttdcc010_thermal_zone *zone;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);
	
	list_for_each_entry(zone, &priv->head, list) {
		if(zone->id != thermal->id)
			continue;
		
		if(trip >= zone->ntrips) {
			dev_err(dev, "set trip temp error\n");
			return -EINVAL;
		} else {
			zone->trips[0].temperature = temp;
		}
	}
	
	return 0;
}

static int fttdcc010_notify(struct thermal_zone_device *thermal,
                            int trip, enum thermal_trip_type type)
{
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct device *dev = fttdcc010_thermal_priv_to_dev(priv);
	u32 val;
	
	switch (type) {
		case THERMAL_TRIP_CRITICAL:
			/* FIXME */
			dev_warn(dev, "Thermal reached to critical temperature\n");
			/* Disable FTTDCC010 conversion function */
			val = readl(&priv->regs->ctrl);
			val &= ~TDC_EN;
			writel(val, &priv->regs->ctrl);
			break;
		default:
			break;
	}
	
	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp      = fttdcc010_get_temp,
	.get_mode      = fttdcc010_get_mode,
	.set_mode      = fttdcc010_set_mode,
	.get_trip_type = fttdcc010_get_trip_type,
	.get_trip_temp = fttdcc010_get_trip_temp,
	.set_trip_temp = fttdcc010_set_trip_temp,
	.notify        = fttdcc010_notify,
};

static void fttdcc010_irq_ctrl(struct fttdcc010_thermal_priv *priv, u32 flags, int enable)
{
	u32 inten;
	
	inten = readl(&priv->regs->inten);
	if(enable)
		inten |= flags;
	else
		inten &= ~flags;
	
	writel(inten, &priv->regs->inten);
}

static void fttdcc010_thermal_update_work(struct work_struct *work)
{
	struct fttdcc010_thermal_priv *priv;
	struct fttdcc010_thermal_zone *zone;
	int i;
	
	priv = container_of(work, struct fttdcc010_thermal_priv, work);
	
	for(i = 0; i < NUM_TDC; i++) {
		list_for_each_entry(zone, &priv->head, list) {
			if(zone->id != i)
				continue;
			thermal_zone_device_update(zone->thermal, THERMAL_EVENT_TEMP_SAMPLE);
		}
	}
}

static void fttdcc010_irq_th(struct fttdcc010_thermal_priv *priv)
{
	struct fttdcc010_thermal_zone *zone;
	u32 ovr_intst, undr_intst;
	int i;
	
	for(i = 0; i < NUM_TDC; i++) {
		ovr_intst = priv->pending & OVR_INTST(i);
		undr_intst = priv->pending & UNDR_INTST(i);
		if(!(ovr_intst && undr_intst))
			continue;
		
		if(ovr_intst)
			priv->pending &= ~OVR_INTST(i);
		
		if(undr_intst)
			priv->pending &= ~UNDR_INTST(i);
		
		list_for_each_entry(zone, &priv->head, list) {
			if(zone->id != i)
				continue;
			schedule_work(&priv->work);
		}
	}
}

static void fttdcc010_irq_done(struct fttdcc010_thermal_priv *priv)
{
	struct fttdcc010_thermal_zone *zone;
	int i;
	
	for(i = 0; i < NUM_TDC; i++) {
		if(!(priv->pending & CHDONE_INTST(i)))
			continue;
		
		priv->pending &= ~CHDONE_INTST(i);
		
		list_for_each_entry(zone, &priv->head, list) {
			if(zone->id != i)
				continue;
			complete(&zone->done);
			schedule_work(&priv->work);
		}
	}
}

static irqreturn_t fttdcc010_irq(int irq, void *data)
{
	struct fttdcc010_thermal_priv *priv = data;
	u32 status, enable;
	u32 ret;
	
	enable = readl(&priv->regs->inten);
	status = readl(&priv->regs->intst);

	priv->pending = status & enable;
	writel(priv->pending, &priv->regs->intst);
	
	if(status & TH_INTST) {
		fttdcc010_irq_th(priv);
		ret = IRQ_HANDLED;
	} else if(status & DONE_INTST) {
		fttdcc010_irq_done(priv);
		ret = IRQ_HANDLED;
	} else {
		dev_err(priv->dev, "Null interrupt!!!\n");
		ret = IRQ_NONE;
	}
	
	return ret;
}

static void fttdcc010_init(struct fttdcc010_thermal_priv *priv)
{
	int i;
	
	writel(SCANMODE | TDC_EN, &priv->regs->ctrl);

	// Wait for scanning done.
	for(i = 0; i < NUM_TDC; i++) {
		while(!(readl(&priv->regs->intst) & CHDONE_INTST(i))) {}
	}
}

static void fttdcc010_populate_trips(struct fttdcc010_thermal_trip *trips)
{
	trips[0].type = THERMAL_TRIP_CRITICAL;
	trips[0].temperature = CRITICAL_THERMAL;
}

static int fttdcc010_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	struct device_node *np = pdev->dev.of_node;
#endif
	struct fttdcc010_thermal_priv *priv;
	struct fttdcc010_thermal_zone *zone;
	struct fttdcc010_thermal_trip *trip;
	struct resource *res, *irq;
	int ret, i;
	
#ifdef CONFIG_OF
	if (!np) {
		dev_err(&pdev->dev, "Failed to get device node\n");
		return -EINVAL;
	}
#endif
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get platform resource(IORESOURCE_MEM)\n");
		return -ENODEV;
	}
	
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "Failed to get platform resource(IORESOURCE_IRQ)\n");
		return -ENODEV;
	}
	
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "Failed to allocate priv\n");
		return -ENOMEM;
	}
	
	priv->dev = &pdev->dev;
	
	priv->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!priv->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}
	
	ret = devm_request_irq(&pdev->dev, irq->start, fttdcc010_irq, 0,
	                       "fttdcc010", priv);
	if (ret) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto err_request_irq;
	}
	
	fttdcc010_init(priv);

	INIT_LIST_HEAD(&priv->head);
	INIT_WORK(&priv->work, fttdcc010_thermal_update_work);
	
	for(i = 0; i < NUM_TDC; i++) {
		zone = devm_kzalloc(&pdev->dev, sizeof(*zone), GFP_KERNEL);
		if (!zone) {
			dev_err(&pdev->dev, "Failed to allocate zone\n");
			ret = -ENOMEM;
			goto err_alloc_zone;
		}
		
		INIT_LIST_HEAD(&zone->list);
		init_completion(&zone->done);
		
		zone->ntrips = 1;
		zone->trips = devm_kzalloc(&pdev->dev, zone->ntrips * sizeof(*trip), GFP_KERNEL);
		if (!zone->trips) {
			dev_err(&pdev->dev, "Failed to allocate zone trips\n");
			ret = -ENOMEM;
			kfree(zone);
			goto err_alloc_zone_trips;
		}
		fttdcc010_populate_trips(zone->trips);
		
		zone->thermal = thermal_zone_device_register("fttdcc010_thermal", zone->ntrips, 1,
		                                             priv, &ops, NULL, 0, SCANMODE == SCANMODE_SGL ? 500 : 0);
		if (IS_ERR(zone->thermal)) {
			dev_err(&pdev->dev, "Failed to register thermal zone device\n");
			ret = PTR_ERR(zone->thermal);
			kfree(zone->trips);
			kfree(zone);
			goto err_zone_register;
		}
		
		zone->id = zone->thermal->id;
		list_add_tail(&zone->list, &priv->head);

		writel(HTHR_EN | HTHR(MCELSIUS(CRITICAL_THERMAL)), &priv->regs->thrhold[i]);
		writel(OVR_INTEN(i) | CHDONE_INTEN(i), &priv->regs->inten);	
	}
	
	platform_set_drvdata(pdev, priv);
	
	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);
	
	return 0;
	
err_zone_register:
err_alloc_zone_trips:
err_alloc_zone:
	list_for_each_entry(zone, &priv->head, list) {
		thermal_zone_device_unregister(zone->thermal);
		kfree(zone->trips);
		kfree(zone);
	}
err_request_irq:
err_ioremap:
	kfree(priv);
	
	return ret;
}

static int fttdcc010_remove(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal = platform_get_drvdata(pdev);
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	struct fttdcc010_thermal_zone *zone;
	u32 val;
	
	/* Disable FTTDCC010 conversion function */
	val = readl(&priv->regs->ctrl);
	val &= ~TDC_EN;
	writel(val, &priv->regs->ctrl);
	
	list_for_each_entry(zone, &priv->head, list) {
		thermal_zone_device_unregister(zone->thermal);
		kfree(zone->trips);
		kfree(zone);
	}
	
	platform_set_drvdata(pdev, NULL);
	
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int fttdcc010_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct thermal_zone_device *thermal = platform_get_drvdata(pdev);
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	u32 val;
	
	/* Disable FTTDCC010 conversion function */
	val = readl(&priv->regs->ctrl);
	val &= ~TDC_EN;
	writel(val, &priv->regs->ctrl);
	
	return 0;
}

static int fttdcc010_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct thermal_zone_device *thermal = platform_get_drvdata(pdev);
	struct fttdcc010_thermal_priv *priv = thermal->devdata;
	u32 val;
	
	/* Enalbe FTTDCC010 conversion function */
	val = readl(&priv->regs->ctrl);
	val |= TDC_EN;
	writel(val, &priv->regs->ctrl);
	
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(fttdcc010_pm_ops,
                         fttdcc010_suspend, fttdcc010_resume);

#ifdef CONFIG_OF
static const struct of_device_id fttdcc010_dt_ids[] = {
	{ .compatible = "faraday,fttdcc010" },
	{ }
};

MODULE_DEVICE_TABLE(of, fttdcc010_dt_ids);
#endif

static struct platform_driver fttdcc010_driver = {
	.driver = {
		.name  = "fttdcc010",
		.owner = THIS_MODULE,
		.pm    = &fttdcc010_pm_ops,
		.of_match_table = of_match_ptr(fttdcc010_dt_ids),
	},
	.probe  = fttdcc010_probe,
	.remove = fttdcc010_remove,
};

module_platform_driver(fttdcc010_driver);

MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
