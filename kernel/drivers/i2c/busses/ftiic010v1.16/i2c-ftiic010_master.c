/*
 * Faraday FTIIC010 I2C Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <asm/io.h>

#include "i2c-ftiic010.h"

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/io.h>
#include <linux/serial_core.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeirq.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <dt-bindings/gpio/gpio.h>


/******************************************************************************
 * internal functions
 *****************************************************************************/
static void ftiic010_set_clock_speed(struct ftiic010 *ftiic010, int hz);
static void ftiic010_set_tgsr(struct ftiic010 *ftiic010, int tsr, int gsr);
__u8 gInd;

static void ftiic010_reset(struct ftiic010 *ftiic010)
{
	int cr = FTIIC010_CR_I2C_RST;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);

	/* wait until reset bit cleared by hw */
	while (ioread32(ftiic010->base + FTIIC010_OFFSET_CR) & FTIIC010_CR_I2C_RST);

	/* HW initial */
	ftiic010_set_tgsr(ftiic010, TSR, GSR);
	ftiic010_set_clock_speed(ftiic010, SCL_SPEED);
}

static void ftiic010_set_clock_speed(struct ftiic010 *ftiic010, int hz)
{
	unsigned long pclk;
	int cdr;

	pclk = clk_get_rate(ftiic010->clk);

	cdr = (pclk/ hz - GSR) / 2 - 2;
	cdr &= FTIIC010_CDR_COUNT_MASK;
	cdr |= 0xA10 << FTIIC010_CDR_COUNTH_OFF;

	dev_dbg(&ftiic010->adapter.dev, "  [CDR] = %08x\n", cdr);
	iowrite32(cdr, ftiic010->base + FTIIC010_OFFSET_CDR);
}

static void ftiic010_set_tgsr(struct ftiic010 *ftiic010, int tsr, int gsr)
{
	int tgsr;

	tgsr = FTIIC010_TGSR_TSR(tsr);
	tgsr |= FTIIC010_TGSR_GSR(gsr);

	dev_dbg(&ftiic010->adapter.dev, "  [TGSR] = %08x\n", tgsr);
	iowrite32(tgsr, ftiic010->base + FTIIC010_OFFSET_TGSR);
}

static void ftiic010_hw_init(struct ftiic010 *ftiic010)
{
	ftiic010_reset(ftiic010);
	ftiic010_set_tgsr(ftiic010, TSR, GSR);
	ftiic010_set_clock_speed(ftiic010, SCL_SPEED);
}

static inline void ftiic010_set_cr(struct ftiic010 *ftiic010, int start,
		int stop, int nak, int hs)
{
	int cr;

	cr = FTIIC010_CR_I2C_EN
	   | FTIIC010_CR_MST_EN
	   | FTIIC010_CR_TB_EN
	   | FTIIC010_CR_ALI_EN;

	if (start)
		cr |= FTIIC010_CR_START;

	if (stop)
		cr |= FTIIC010_CR_STOP;

	if (nak)
		cr |= FTIIC010_CR_NACK;

	if (hs)
		cr |= FTIIC010_CR_HSMODE;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);
}

static int ftiic010_tx_byte(struct ftiic010 *ftiic010, __u8 data, int start,
		int stop, int hs)
{
	// struct i2c_adapter *adapter = &ftiic010->adapter;
	int i;

	iowrite32(data, ftiic010->base + FTIIC010_OFFSET_DR);
	ftiic010_set_cr(ftiic010, start, stop, 0, hs);

	for (i = 0; i < MAX_RETRY; i++) {
		unsigned int status;

		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR); // w1c
		if ((status & FTIIC010_SR_TD)&&!(status & FTIIC010_SR_NACK)) {
			return 0;
		}

		udelay(1);
	}

	// dev_err(&adapter->dev, "Failed to transmit: time out!!!\n");
	ftiic010_reset(ftiic010);
	return -EIO;
}

static int ftiic010_rx_byte(struct ftiic010 *ftiic010, int stop, int nak, int hs)
{
	// struct i2c_adapter *adapter = &ftiic010->adapter;
	int i;

	ftiic010_set_cr(ftiic010, 0, stop, nak, hs);

	for (i = 0; i < MAX_RETRY; i++) {
		unsigned int status;

		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR); // w1c
		if (status & FTIIC010_SR_TD) {
			return (ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK);
		}

		udelay(1);
	}

	// dev_err(&adapter->dev, "Failed to receive\n");
	ftiic010_reset(ftiic010);
	return -EIO;
}

static int ftiic010_tx_msg(struct ftiic010 *ftiic010, struct i2c_msg *msg)
{
	__u8 data;
	int ret;
	int i;
	int hs = 0;

	gInd = msg->buf[0];
	if (ftiic010->hs)
	{
		// Master has to send a HS code(0x8) in the beginning
		hs = 0;
		data = 0x8; // HS code
		ret = ftiic010_tx_byte(ftiic010, data, 1, 0, hs);
		if (ret < 0)
			return ret;

		hs = 1;
	}

	data = (msg->addr & 0x7f) << 1;
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0, hs);
	if (ret < 0)
		return ret;

	for (i = 0; i < msg->len; i++) {
		int stop = 0;
		//if ((i + 1 == msg->len) && (msg->len != 2))
		if((i+1) == msg->len )
			stop = 1;

		ret = ftiic010_tx_byte(ftiic010, msg->buf[i], 0, stop, hs);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int ftiic010_rx_msg(struct ftiic010 *ftiic010, struct i2c_msg *msg)
{
	__u8 data;
	int ret;
	int i;
	int hs = 0;

	hs = ftiic010->hs;
	data = (msg->addr & 0x7f) << 1 | 1;
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0, hs);
	if (ret < 0)
		return ret;

	for (i = 0; i < msg->len; i++) {
		int nak = 0;
		int stop = 0;

		if (i + 1 == msg->len)
		{
			nak = 1;
			stop = 1;
		}

		ret = ftiic010_rx_byte(ftiic010, stop, nak, hs);

		if (ret < 0)
			return ret;

		msg->buf[i] = ret;
	}

	return 0;
}

static int ftiic010_burst_tx_msg(struct ftiic010 *ftiic010, struct i2c_msg *msg)
{
	
	// struct i2c_adapter *adapter = &ftiic010->adapter;
	__u32 addr, idx, bNum; 
	int i, cr;

	addr = msg->addr;
	idx = msg->buf[0];
	bNum = (msg->len)-1;
	if (bNum == 0)
	{
		gInd = msg->buf[0];
		return 0;
	}

	iowrite32(((idx<<FTIIC010_MEM_IDX_OFF)|(addr&FTIIC010_ADDR_MASK)), (ftiic010->base+FTIIC010_OFFSET_AR));
	iowrite32((bNum<<FTIIC010_BSTTDC_OFF), (ftiic010->base+FTIIC010_OFFSET_BSTMR));
	for (i=1; i<=bNum; i++) // buf[0]=idx buf[1]~buf[n]=data
		iowrite32(msg->buf[i], ftiic010->base + FTIIC010_OFFSET_DR);

	cr = FTIIC010_CR_FLOWCTRL_W;
	if (ftiic010->hs)
		cr |= FTIIC010_CR_HSMODE;

	iowrite32(cr, (ftiic010->base+FTIIC010_OFFSET_CR));

	for (i = 0; i < MAX_RETRY; i++) {
		unsigned int status;

		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR); // w1c
		if (status & FTIIC010_SR_TD) {
			return 0;
		}

		udelay(1);
	}

	// dev_err(&adapter->dev, "Failed to transmit with burst tx mode: time out!!!\n");
	ftiic010_reset(ftiic010);
	return -EIO;
}

static int ftiic010_burst_rx_msg(struct ftiic010 *ftiic010, struct i2c_msg *msg)
{
	__u32 addr, idx, bNum; 
	int i, j, cr;
	unsigned int status;

	addr = msg->addr;
	idx = gInd;
	bNum = msg->len;
	gInd += bNum;
	iowrite32(((idx<<FTIIC010_MEM_IDX_OFF)|(addr&FTIIC010_ADDR_MASK)), (ftiic010->base+FTIIC010_OFFSET_AR));
	iowrite32((bNum<<FTIIC010_BSTTDC_OFF), (ftiic010->base+FTIIC010_OFFSET_BSTMR));
	cr = FTIIC010_CR_FLOWCTRL_R;
	if (ftiic010->hs)
		cr |= FTIIC010_CR_HSMODE;

	iowrite32(cr, (ftiic010->base+FTIIC010_OFFSET_CR));

	for (i = 0; i < MAX_RETRY; i++) {
		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR); // w1c
		if (status & FTIIC010_SR_TD) {
			for (j = 0; j<bNum; j++)
				msg->buf[j] = ioread32(ftiic010->base + FTIIC010_OFFSET_DR);

			return 0;
		}

		udelay(1);
	}

	//dev_err(&adapter->dev, "Failed to transmit with burst rx mode: time out!!!\n");
	ftiic010_reset(ftiic010);
	return -EIO;
}

static int ftiic010_do_msg(struct ftiic010 *ftiic010, struct i2c_msg *msg)
{
	if (msg->flags & I2C_M_RD) // RX
	{
		if (ftiic010->burst)
			return ftiic010_burst_rx_msg(ftiic010, msg);
		else
			return ftiic010_rx_msg(ftiic010, msg);
	}
	else
	{
		if (ftiic010->burst)
			return ftiic010_burst_tx_msg(ftiic010, msg);
		else
			return ftiic010_tx_msg(ftiic010, msg);
	}
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftiic010_interrupt(int irq, void *dev_id)
{
	struct ftiic010 *ftiic010 = dev_id;
	struct i2c_adapter *adapter = &ftiic010->adapter;
	int sr;

	sr = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
	iowrite32(sr, ftiic010->base + FTIIC010_OFFSET_SR); // w1c
	if (sr & FTIIC010_SR_AL) {
		dev_err(&adapter->dev, "arbitration lost!\n");
		ftiic010_reset(ftiic010);
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct i2c_algorithm functions
 *****************************************************************************/
static int ftiic010_master_xfer(struct i2c_adapter *adapter,
		struct i2c_msg *msgs, int num)
{
	struct ftiic010 *ftiic010 = i2c_get_adapdata(adapter);
	int i;

	for (i = 0; i < num; i++) {
		int ret;

		ret = ftiic010_do_msg(ftiic010, &msgs[i]);
		if (ret)
			return ret;
	}

	return num;
}

static u32 ftiic010_functionality(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm ftiic010_algorithm = {
	.master_xfer	= ftiic010_master_xfer,
	.functionality	= ftiic010_functionality,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftiic010_probe(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010;
	struct resource *res;
	struct clk *clk;
	int irq;
	int ret;
	u32 dev_id;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	if ((irq = platform_get_irq(pdev, 0)) < 0)
		return irq;

	ftiic010 = kzalloc(sizeof(*ftiic010), GFP_KERNEL);
	if (!ftiic010) {
		dev_err(&pdev->dev, "Could not allocate private data\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	ftiic010->res = request_mem_region(res->start,
			res->end - res->start, dev_name(&pdev->dev));
	if (ftiic010->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		ret = -ENOMEM;
		goto err_req_mem;
	}

	ftiic010->base = ioremap(res->start, res->end - res->start);
	if (ftiic010->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	ret = request_irq(irq, ftiic010_interrupt, IRQF_SHARED, pdev->name, ftiic010);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}

	ftiic010->irq = irq;

	clk = clk_get(&pdev->dev, "pclk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Failed to get clock(0x%x).\n", (unsigned int)clk);
		goto err_clk;
	}

	clk_prepare_enable(clk);
	ftiic010->clk = clk;

	/*
	 * initialize i2c adapter
	 */
	ftiic010->adapter.owner 	= THIS_MODULE;
	ftiic010->adapter.algo		= &ftiic010_algorithm;
	ftiic010->adapter.timeout	= 1;
	ftiic010->adapter.dev.parent	= &pdev->dev;
	ftiic010->adapter.dev.of_node	= pdev->dev.of_node;
	ftiic010->burst = 0;
	ftiic010->hs = 0;

	if (pdev->dev.of_node) {
		ret = of_property_read_u32(pdev->dev.of_node, "dev_id", &dev_id);
		ftiic010->adapter.nr = dev_id;
		of_property_read_u32(pdev->dev.of_node, "hs", &ftiic010->hs);
		of_property_read_u32(pdev->dev.of_node, "burst", &ftiic010->burst);
		printk("hs = %d  burst = %d\n", ftiic010->hs, ftiic010->burst);
	}
	else
		ftiic010->adapter.nr = pdev->id;

	strcpy(ftiic010->adapter.name, "ftiic010 adapter");

	i2c_set_adapdata(&ftiic010->adapter, ftiic010);

	ftiic010_hw_init(ftiic010);

	ret = i2c_add_numbered_adapter(&ftiic010->adapter);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add i2c adapter\n");
		goto err_add_adapter;
	}

	platform_set_drvdata(pdev, ftiic010);

	dev_info(&pdev->dev, "irq %d, mapped at %px, clk %lu\n", irq,
		 ftiic010->base, clk_get_rate(ftiic010->clk));

	return 0;

err_add_adapter:
	clk_disable_unprepare(clk);
	clk_put(clk);
err_clk:
	free_irq(ftiic010->irq, ftiic010);
err_req_irq:
	iounmap(ftiic010->base);
err_ioremap:
	release_resource(ftiic010->res);
err_req_mem:
	kfree(ftiic010);
err_alloc:
	return ret;
};

static int __exit ftiic010_remove(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010 = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&ftiic010->adapter);
	clk_disable(ftiic010->clk);
	clk_put(ftiic010->clk);
	free_irq(ftiic010->irq, ftiic010);
	iounmap(ftiic010->base);
	release_resource(ftiic010->res);
	kfree(ftiic010);
	return 0;
};

static const struct of_device_id ftiic010_dt_ids[] = {
	{ .compatible = "faraday,ftiic010", },
	{},
};
MODULE_DEVICE_TABLE(of, ftiic010_dt_ids);

static struct platform_driver ftiic010_driver = {
	.probe		= ftiic010_probe,
	.remove		= __exit_p(ftiic010_remove),
	.driver		= {
		.name	= "ftiic010",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ftiic010_dt_ids),
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftiic010_init(void)
{
	return platform_driver_register(&ftiic010_driver);
}

static void __exit ftiic010_exit(void)
{
	platform_driver_unregister(&ftiic010_driver);
}

module_init(ftiic010_init);
module_exit(ftiic010_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTIIC010 I2C bus adapter");
MODULE_LICENSE("GPL");
