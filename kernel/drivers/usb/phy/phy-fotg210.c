/*
 * FOTG210 USB otg driver.
 *
 * Copyright (C) 2016-2019 Faraday Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/* This driver helps to switch Faraday controller function between host
 * and peripheral. It works with FOTG210 HCD driver and FOTG210 UDC
 * driver together.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>

#include "phy-fotg210.h"

#define	DRIVER_DESC	"Faraday USB OTG transceiver driver"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

static const char driver_name[] = "fotg210_otg";

static char *state_string[] = {
	"undefined",
	"b_device", //"b_idle",	//lichun@modify
	"b_srp_init",
	"b_peripheral",
	"b_wait_acon",
	"b_host",
	"a_host", //"a_idle",	//lichun@modify
	"a_wait_vrise",
	"a_wait_bcon",
	"a_host",
	"a_suspend",
	"a_peripheral",
	"a_wait_vfall",
	"a_vbus_err"
};

static int fotg210_otg_set_host(struct usb_otg *otg,
			   struct usb_bus *host)
{
	otg->host = host;

	return 0;
}

static int fotg210_otg_set_peripheral(struct usb_otg *otg,
				 struct usb_gadget *gadget)
{
	otg->gadget = gadget;

	return 0;
}

static void fotg210_drv_A_vbus(struct fotg210_otg *fotg210)
{
	u32 value;

	//set Vbus
	value = ioread32(fotg210->regs + FOTG210_OTGCSR);
	value &= ~OTGCSR_A_BUS_DROP;
	value |= OTGCSR_A_BUS_REQ;
	iowrite32(value, fotg210->regs + FOTG210_OTGCSR);
}

static void fotg210_drop_A_vbus(struct fotg210_otg *fotg210)
{
	u32 value;

	//drop Vbus
	value = ioread32(fotg210->regs + FOTG210_OTGCSR);
	value &= ~OTGCSR_A_BUS_REQ;
	value |= OTGCSR_A_BUS_DROP;
	iowrite32(value, fotg210->regs + FOTG210_OTGCSR);
}

/* get current role (0: host; 1: peripheral) */
static u8 fotg210_get_role(struct fotg210_otg *fotg210)
{
	u32 temp = ioread32(fotg210->regs + FOTG210_OTGCSR);

	if (temp & OTGCSR_CROLE)
		return 1;	//Peripheral
	else
		return 0;
}

/* get current id (0: A-type; 1: B-type) */
static u8 fotg210_get_id(struct fotg210_otg *fotg210)
{
	u32 temp = ioread32(fotg210->regs + FOTG210_OTGCSR);

	if (temp & OTGCSR_ID)
		return 1;	//B-type
	else
		return 0;
}

static void fotg210_run_state_machine(struct fotg210_otg *fotg210,
					unsigned long delay)
{
	dev_dbg(&fotg210->pdev->dev, "transceiver is updated\n");
	if (!fotg210->qwork)
		return;

	queue_delayed_work(fotg210->qwork, &fotg210->work, delay);
}

static void fotg210_otg_update_state(struct fotg210_otg *fotg210)
{
	int old_state = fotg210->phy.otg->state;

	switch (old_state) {
	case OTG_STATE_UNDEFINED:
		fotg210->phy.otg->state = OTG_STATE_B_IDLE;
		/* FALL THROUGH */
	case OTG_STATE_B_IDLE:
		if (fotg210_get_id(fotg210) == ID_A_TYPE)
			fotg210->phy.otg->state = OTG_STATE_A_IDLE;
		break;

	case OTG_STATE_A_IDLE:
		if (fotg210_get_id(fotg210) == ID_B_TYPE)
			fotg210->phy.otg->state = OTG_STATE_B_IDLE;
		break;

	default:
		break;
	}
}

static void fotg210_otg_work(struct work_struct *work)
{
	struct fotg210_otg *fotg210;
	struct usb_phy *phy;
	struct usb_otg *otg;
	int old_state;
	u32 reg;

	fotg210 = container_of(to_delayed_work(work), struct fotg210_otg, work);

run:
	/* work queue is single thread, or we need spin_lock to protect */
	phy = &fotg210->phy;
	otg = fotg210->phy.otg;
	old_state = otg->state;

	fotg210_otg_update_state(fotg210);

	if (old_state != fotg210->phy.otg->state) {
		dev_info(&fotg210->pdev->dev, "change from state %s to %s\n",
			 state_string[old_state],
			 state_string[fotg210->phy.otg->state]);

		switch (fotg210->phy.otg->state) {
		case OTG_STATE_B_IDLE:
			otg->default_a = 0;

			//STOP host
			reg = ioread32(fotg210->regs + FOTG210_USBCMD);
			reg &= ~USBCMD_RUN;
			iowrite32(reg, fotg210->regs + FOTG210_USBCMD);
			while ((ioread32(fotg210->regs + FOTG210_USBSTS) & USBSTS_HALT) == 0) ;

			//drop Vbus
			fotg210_drop_A_vbus(fotg210);

			//set device global interrupt
			iowrite32(GMIR_MHC_INT, fotg210->regs + FOTG210_GMIR);

			reg = ioread32(fotg210->regs + FOTG210_DEVMCR);
			reg |= DEVMCR_GLINT_EN;
			iowrite32(reg, fotg210->regs + FOTG210_DEVMCR);

			/* clear device un-plug */
			reg = ioread32(fotg210->regs + FOTG210_PHYTMSR);
			reg &= ~PHYTMSR_UNPLUG;
			iowrite32(reg, fotg210->regs + FOTG210_PHYTMSR);

			break;

		case OTG_STATE_A_IDLE:
			otg->default_a = 1;

			/* set device un-plug */
			reg = ioread32(fotg210->regs + FOTG210_PHYTMSR);
			reg |= PHYTMSR_UNPLUG;
			iowrite32(reg, fotg210->regs + FOTG210_PHYTMSR);

			/* disable device global interrupt */
			reg = ioread32(fotg210->regs + FOTG210_DEVMCR);
			reg &= ~DEVMCR_GLINT_EN;
			iowrite32(reg, fotg210->regs + FOTG210_DEVMCR);

			//set host global interrupt
			iowrite32(GMIR_MDEV_INT, fotg210->regs + FOTG210_GMIR);

			//set Vbus
			fotg210_drv_A_vbus(fotg210);

			//RUN host
			reg = ioread32(fotg210->regs + FOTG210_USBCMD);
			reg |= USBCMD_RUN;
			iowrite32(reg, fotg210->regs + FOTG210_USBCMD);
			while ((ioread32(fotg210->regs + FOTG210_USBSTS) & USBSTS_HALT) == 1) ;

			break;

		default:
			break;
		}
		goto run;
	}
}


static irqreturn_t fotg210_otg_irq(int irq, void *dev)
{
	struct fotg210_otg *fotg210 = dev;
	u32 otgsc;

	/* Write 1 to clear */
	otgsc = ioread32(fotg210->regs + FOTG210_OTGISR);
	if (otgsc)
		iowrite32(otgsc, fotg210->regs + FOTG210_OTGISR);

	if ((otgsc & fotg210->irq_en) == 0)
		return IRQ_NONE;

	fotg210_run_state_machine(fotg210, 10);

	return IRQ_HANDLED;
}

static void fotg210_init(struct fotg210_otg *fotg210)
{
	u32 val;

	//clear otg interrupt status (write '1' clear)
	val = ioread32(fotg210->regs + FOTG210_OTGISR);
	iowrite32(val, fotg210->regs + FOTG210_OTGISR);

	//disable otg interrupt
	iowrite32(0, fotg210->regs + FOTG210_OTGIER);

	//set otg interrupt enable
	fotg210->irq_en = INT_IDCHG;
	iowrite32(fotg210->irq_en, fotg210->regs + FOTG210_OTGIER);

	fotg210_otg_update_state(fotg210);

	/* set global interrupt */
	if (fotg210_get_role(fotg210) == CROLE_Host) {
		iowrite32(GMIR_MDEV_INT, fotg210->regs + FOTG210_GMIR);

		fotg210_drv_A_vbus(fotg210);
	}
	else
		iowrite32(GMIR_MHC_INT, fotg210->regs + FOTG210_GMIR);
}

static int fotg210_otg_probe(struct platform_device *pdev)
{
	struct fotg210_otg *fotg210;
	struct usb_otg *otg;
	struct resource *res;
	int retval = 0;

	fotg210 = devm_kzalloc(&pdev->dev, sizeof(*fotg210), GFP_KERNEL);
	if (!fotg210) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		return -ENOMEM;
	}

	otg = devm_kzalloc(&pdev->dev, sizeof(*otg), GFP_KERNEL);
	if (!otg)
		return -ENOMEM;

	platform_set_drvdata(pdev, fotg210);

	fotg210->qwork = create_singlethread_workqueue("fotg210_otg_queue");
	if (!fotg210->qwork) {
		dev_dbg(&pdev->dev, "cannot create workqueue for FOTG210\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&fotg210->work, fotg210_otg_work);

	/* OTG common part */
	fotg210->pdev = pdev;
	fotg210->phy.dev = &pdev->dev;
	fotg210->phy.otg = otg;
	fotg210->phy.label = driver_name;

	otg->state = OTG_STATE_UNDEFINED;
	otg->usb_phy = &fotg210->phy;
	otg->set_host = fotg210_otg_set_host;
	otg->set_peripheral = fotg210_otg_set_peripheral;

	res = platform_get_resource(fotg210->pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no I/O memory resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	fotg210->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (fotg210->regs == NULL) {
		dev_err(&pdev->dev, "failed to map I/O memory\n");
		retval = -EFAULT;
		goto err_destroy_workqueue;
	}

	//initial HW setting
	fotg210_init(fotg210);

	res = platform_get_resource(fotg210->pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	fotg210->irq = res->start;
	if (devm_request_irq(&pdev->dev, fotg210->irq, fotg210_otg_irq, IRQF_SHARED,
			driver_name, fotg210)) {
		dev_err(&pdev->dev, "Request irq %d for FOTG210 failed\n",
			fotg210->irq);
		fotg210->irq = 0;
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	retval = usb_add_phy(&fotg210->phy, USB_PHY_TYPE_USB2);
	if (retval < 0) {
		dev_err(&pdev->dev, "can't register transceiver, %d\n",
			retval);
		goto err_destroy_workqueue;
	}

	spin_lock_init(&fotg210->wq_lock);
	if (spin_trylock(&fotg210->wq_lock)) {
		fotg210_run_state_machine(fotg210, 2 * HZ);
		spin_unlock(&fotg210->wq_lock);
	}

	dev_info(&pdev->dev, "successful probe FOTG210\n");

	return 0;

	usb_remove_phy(&fotg210->phy);
err_destroy_workqueue:
	flush_workqueue(fotg210->qwork);
	destroy_workqueue(fotg210->qwork);

	return retval;
}

static int fotg210_otg_remove(struct platform_device *pdev)
{
	struct fotg210_otg *fotg210 = platform_get_drvdata(pdev);

	if (fotg210->qwork) {
		flush_workqueue(fotg210->qwork);
		destroy_workqueue(fotg210->qwork);
	}

	usb_remove_phy(&fotg210->phy);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id fotg210_otg_dt_ids[] = {
	{ .compatible = "faraday,fotg210" },
	{ }
};

MODULE_DEVICE_TABLE(of, fotg210_otg_dt_ids);
#endif

static struct platform_driver fotg210_otg_driver = {
	.probe	= fotg210_otg_probe,
	.remove	= fotg210_otg_remove,
	.driver	= {
		.name = driver_name,
		.of_match_table = of_match_ptr(fotg210_otg_dt_ids),
	},
};
module_platform_driver(fotg210_otg_driver);

