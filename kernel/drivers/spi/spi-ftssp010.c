/*
 * Faraday ftssp010 Multi-function Controller - SPI Mode
 *
 * (C) Copyright 2019 Faraday Technology
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>

#include <asm/io.h>

#include <plat/ftgpio010.h>
#include "spi-ftssp010.h"

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/ftdmac020.h>
#include <linux/dma/ftdmac030.h>
#endif

/*
 * private data struct
 */
struct ftssp010_spi {
	spinlock_t          lock;

	void __iomem        *mmio;
	void __iomem        *gpio;	/* FS/CS GPIO Base */
	u32                 irq;
	u32                 rev;
	u8                  txfifo_depth;
	u8                  rxfifo_depth;

#ifdef CONFIG_SPI_FTSSP010_USE_INT
	struct completion   c;
#endif
	struct device       *dev;
	struct spi_master   *master;
	unsigned long       clk;
	unsigned long       max_speed;
	unsigned long       min_speed;

	/* current status */
	unsigned long       speed;
	unsigned long       bits_per_word;

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	int dma_sts;
#if defined(CONFIG_SPI_FTSSP010_USE_DMA_FTDMAC020)
	struct ftdmac020_dma_slave slave;
#elif defined(CONFIG_SPI_FTSSP010_USE_DMA_FTDMAC030)
	struct ftdmac030_dma_slave slave;
#endif
	struct dma_chan     *dma_chan;
	wait_queue_head_t   dma_wait_queue;

	unsigned char	    *dma_buf;
	dma_addr_t	        dmaaddr;
	dma_addr_t	        phys_io_port;
#endif
};

/* the spi->mode bits understood by this driver: */
#define MODEBITS        (SPI_CPOL|SPI_CPHA)

#ifdef CONFIG_SPI_FTSSP010_USE_DMA

#define DMA_COMPLETE    1
#define DMA_ONGOING     2
#define FTSSP010_DMA_BUF_SIZE   (4 * 1024)

static void ftssp010_dma_callback(void *param)
{
	struct ftssp010_spi *priv = (struct ftssp010_spi *)param;

	BUG_ON(priv->dma_sts == DMA_COMPLETE);
	priv->dma_sts = DMA_COMPLETE;
	wake_up_interruptible(&priv->dma_wait_queue);
}

static int ftssp010_dma_wait(struct ftssp010_spi *priv)
{
	int ret;

	ret = wait_event_interruptible_timeout(priv->dma_wait_queue,
	                                       priv->dma_sts == DMA_COMPLETE, HZ);

	return ret;
}

#else

static void ftssp010_write_word(void __iomem *base, const void *data, u32 wsize)
{
	if (data) {
		switch (wsize) {
		case 1:
			writeb(*(const u8 *)data, base);
			break;

		case 2:
			writew(*(const u16 *)data, base);
			break;

		default:
			writel(*(const u32 *)data, base);
			break;
		}
	}
}

static void ftssp010_read_word(void __iomem *base, void *buf, u32 wsize)
{
	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = readb(base);
			break;

		case 2:
			*(u16 *) buf = readw(base);
			break;

		default:
			*(u32 *) buf = readl(base);
			break;
		}
	}
}
#endif

/*
 * called only when no transfer is active on the bus
 */
static int ftssp010_spi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->master);
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long speed;
	unsigned long bits_per_word;
	unsigned long div;
	unsigned long clk;
	speed = spi->max_speed_hz;
	bits_per_word = spi->bits_per_word;

	if (t) {
		if (t->speed_hz)
			speed = t->speed_hz;
		if (t->bits_per_word)
			bits_per_word = t->bits_per_word;
	}

	if (speed > priv->max_speed)
		speed = priv->max_speed;

	if (speed < priv->min_speed)
		speed = priv->min_speed;

	clk = priv->clk;
	for (div = 0; div < 0xFFFF; ++div) {
		if (speed >= (clk / (2 * (div + 1))))
			break;
	}

	speed = clk / (2 * (div + 1));

	if (priv->speed != speed || priv->bits_per_word != bits_per_word) {

		priv->speed = speed;
		priv->bits_per_word = bits_per_word;

		/* Set SPI clock setup + data width */
		writel(CR1_CLKDIV(div) | CR1_SDL(bits_per_word), &regs->cr[1]);
	}

	return 0;
}

#ifdef CONFIG_SPI_FTSSP010_USE_INT
static irqreturn_t ftssp010_isr(int irq, void *dev_id)
{
	struct ftssp010_spi *priv = dev_id;
	struct ftssp010_regs *regs = priv->mmio;
	u32 isr;

	isr = readl(&regs->isr);
	if (!isr) {
		dev_err(priv->dev, "Null interrupt!!!\n");
		return IRQ_NONE;
	}

	if (isr & ISR_TXCI)
		complete(&priv->c);

	if (isr & ISR_TFURI)
		dev_err(priv->dev, "TX FIFO Underrun!!!\n");

	if (isr & ISR_RFORI)
		dev_err(priv->dev, "RX FIFO Overrun!!!\n");

	return IRQ_HANDLED;
}
#endif

static inline int ftssp010_wait_tx_idle(struct ftssp010_spi *priv, bool slave, signed long timeout)
{
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long expire;
	u32 sr;

	expire = timeout + jiffies;

	do {
		sr = readl(&regs->sr);

		if (!SR_TFVE(sr) && (slave ? 1 : !(sr & SR_BUSY)))
			goto out;
#ifdef CONFIG_MACH_LEO_VP
		usleep_range(1,5);
#else
		cond_resched();
#endif
	} while (time_before(jiffies, expire));

	timeout = expire - jiffies;
out:
	return timeout < 0 ? 0 : timeout;
}

static inline int ftssp010_wait_idle(struct ftssp010_spi *priv, signed long timeout)
{
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long expire;
	u32 sr;

	expire = timeout + jiffies;

	do {
		sr = readl(&regs->sr);

		if (!(sr & SR_BUSY))
			goto out;

		cond_resched();
	} while (time_before(jiffies, expire));

	timeout = expire - jiffies;
out:
	return timeout < 0 ? 0 : timeout;
}

static inline int ftssp010_wait_txfifo_ready(struct ftssp010_spi *priv)
{
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long timeout;
	int ret = -1;

	for (timeout = jiffies + HZ; jiffies < timeout; ) {
		if (!(readl(&regs->sr) & SR_TFNF)) {
#ifdef CONFIG_MACH_LEO_VP
			usleep_range(1,5);
#endif
			continue;
		}
		ret = 0;
		break;
	}

	if (ret)
		dev_err(priv->dev, "Wait TXFIFO timeout\n");

	return ret;
}

static inline int ftssp010_wait_rxfifo_ready(struct ftssp010_spi *priv)
{
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long timeout;
	int ret = -1;

	for (timeout = jiffies + HZ; jiffies < timeout; ) {
		if (!SR_RFVE(readl(&regs->sr)))
			continue;
		ret = 0;
		break;
	}

	if (ret)
		dev_err(priv->dev, "Wait RXFIFO timeout\n");

	return ret;
}

static int ftssp010_spi_hard_xfer8_v2(struct spi_device *spi,
	const u8 *tx_buf, u8 *rx_buf, u32 count)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->master);
	struct ftssp010_regs *regs = priv->mmio;
	u32 i, mask,size,wsize, wcount;
	u32 ret;
#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	struct dma_chan *chan;
	struct scatterlist sg;
	struct dma_async_tx_descriptor *desc;
	u32 tmp;

	spin_lock(&priv->lock);

	chan = priv->dma_chan;
#endif

#ifdef CONFIG_SPI_FTSSP010_USE_INT
	init_completion(&priv->c);
#endif

	if (priv->bits_per_word <= 8)
		wsize = 1;
	else if (priv->bits_per_word <= 16)
		wsize = 2;
	else
		wsize = 4;

	// The transfer count should be multiple of wsize
	if (count % wsize)
		count += (wsize - (count % wsize));

	// Setup mask for control register #2
	mask = 0;
	if (rx_buf)
		mask |= CR2_RXEN;

	while (count > 0) {
		wcount = 0;

		// Handle tx event
		if (tx_buf) {
#ifdef CONFIG_SPI_FTSSP010_USE_DMA
			size = min_t(int, count, FTSSP010_DMA_BUF_SIZE);

			priv->dma_sts = DMA_ONGOING;
			memcpy(priv->dma_buf, tx_buf, size);
			sg_init_one(&sg, dma_to_virt(priv->dev, priv->dmaaddr), size);
			sg_dma_address(&sg) = priv->dmaaddr;
			priv->slave.common.direction = DMA_TO_DEVICE;
			priv->slave.common.src_addr = priv->dmaaddr;
			priv->slave.common.dst_addr = priv->phys_io_port;
			priv->slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			priv->slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			priv->slave.common.src_maxburst = priv->txfifo_depth;

			desc = chan->device->device_prep_slave_sg(chan, &sg, 1, DMA_TO_DEVICE,
			                                          DMA_PREP_INTERRUPT | DMA_CTRL_ACK, NULL);
			if (desc == NULL) {
				dev_err(priv->dev, "TX DMA prepare slave sg failed\n");
				spin_unlock(&priv->lock);
				return -EINVAL;
			}
			desc->callback = ftssp010_dma_callback;
			desc->callback_param = priv;
			desc->tx_submit(desc);
			chan->device->device_issue_pending(chan);

			if (ftssp010_dma_wait(priv) == 0) {
				dev_err(priv->dev, "TX Wait DMA timeout\n");
				spin_unlock(&priv->lock);
				return -ETIMEDOUT;
			}

			tx_buf += (size * 1);
#else
			size = min_t(u32, priv->txfifo_depth - SR_TFVE(readl(&regs->sr)), count / wsize);
			for (i = 0; i < size; ++i) {
				ftssp010_write_word(&regs->dr, tx_buf, wsize);
				tx_buf += wsize;
				wcount ++;
			}
#endif
		}

		// Set control register #2 now
		if ((readl(&regs->cr[2]) & mask) != mask)
			setbits_le32(&regs->cr[2], mask);

		dev_dbg(priv->dev, "cr0: 0x%08x cr1: 0x%08x cr2: 0x%08x\n",
		        readl(&regs->cr[0]), readl(&regs->cr[1]), readl(&regs->cr[2]));

		// Handle rx event
		if (rx_buf) {
			size = min_t(u32, priv->rxfifo_depth - SR_RFVE(readl(&regs->sr)), count / wsize);
			// Write dummy data to control rx data quantity in master mode
			if (!spi_controller_is_slave(priv->master)) {
				for (i = 0; i < size - wcount; ++i) {
					ftssp010_wait_txfifo_ready(priv);
					writel(0x0, &regs->dr);	//Dummy write
				}
			}

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
			int dma_map, mod_len;

			dma_map = virt_addr_valid(rx_buf) && !((int)rx_buf & 3);
			size = min_t(int, count, FTSSP010_DMA_BUF_SIZE);

			/* DMA read unit is 4 bytes(1 word), if read length
			 * not multiple of 4 bytes, add more bytes to make it
			 * becomes multiple of 4, but do not copy them to rx_buf.
			 */
			mod_len = size & 0x3;
			if (mod_len)
				mod_len = 4 - mod_len;

			if (dma_map) {
				sg_init_one(&sg, (void *)rx_buf, (size + mod_len));

				tmp = dma_map_sg(priv->dev, &sg, 1, DMA_FROM_DEVICE);
				if (!tmp) {
					dev_err(priv->dev, "dma_map_sg failed\n ");
					return -EINVAL;
				}
			} else {
				sg_init_one(&sg, dma_to_virt(priv->dev, priv->dmaaddr),
				            (size + mod_len));
				sg_dma_address(&sg) = priv->dmaaddr;
			}

			priv->dma_sts = DMA_ONGOING;
			priv->slave.common.direction = DMA_FROM_DEVICE;
			priv->slave.common.src_addr = priv->phys_io_port;
			priv->slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			priv->slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			priv->slave.common.src_maxburst = priv->rxfifo_depth / 4;

			desc = chan->device->device_prep_slave_sg(chan, &sg, 1, DMA_FROM_DEVICE,
			                                          DMA_PREP_INTERRUPT | DMA_CTRL_ACK, NULL);
			if (desc == NULL) {
				dev_err(priv->dev, "RX DMA prepare slave sg failed\n");
				return -EINVAL;
			}
			desc->callback = ftssp010_dma_callback;
			desc->callback_param = priv;
			desc->tx_submit(desc);
			chan->device->device_issue_pending(chan);

			if (ftssp010_dma_wait(priv) <= 0) {
				if (dma_map) {
					dma_unmap_sg(priv->dev, &sg, 1, DMA_FROM_DEVICE);
					dev_err(priv->dev, "DMA_MAP: RX Wait DMA timeout\n");
				} else {
					dev_err(priv->dev, "RX Wait DMA timeout\n");
				}
				return -ETIMEDOUT;
			}

			if (dma_map) {
				dma_unmap_sg(priv->dev, &sg, 1, DMA_FROM_DEVICE);
			} else {
				memcpy(rx_buf, priv->dma_buf, size);
			}

			rx_buf += (size * 4);
#else
			size = min_t(u32, priv->rxfifo_depth, count / wsize);
			for (i = 0; i < size; ++i) {
				while (!SR_RFVE(readl(&regs->sr))) {
#ifdef CONFIG_MACH_LEO_VP
					usleep_range(1, 5);
#else
					cond_resched();
#endif
				}
				ftssp010_read_word(&regs->dr, rx_buf, wsize);
//				if (spi_controller_is_slave(priv->master))
//					printk("slave rx: 0x%x\n", *rx_buf);
				rx_buf += wsize;
			}
#endif
		}

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
		count -= size;
#else
		count -= (size * wsize);
#endif
	}

	// Wait TFVE empty before disabling SSP core
	if (tx_buf && SR_TFVE(readl(&regs->sr))) {
		if (priv->master->slave) {
			ret = ftssp010_wait_tx_idle(priv, spi->master->slave, msecs_to_jiffies(1000));
		} else {
#ifdef CONFIG_SPI_FTSSP010_USE_INT
			ret = wait_for_completion_timeout(&priv->c, msecs_to_jiffies(1000));
#else
			ret = ftssp010_wait_tx_idle(priv, spi->master->slave, msecs_to_jiffies(1000));
#endif
		}
		if (!ret) {
			dev_err(priv->dev, "Wait TX idle timeout(0x%x)\n", readl(&regs->sr));
			ret = -ETIMEDOUT;
			goto out;
		}
	}

	clrbits_le32(&regs->cr[2], CR2_RXEN);

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	spin_unlock(&priv->lock);
#endif

	return 0;
out:
	clrbits_le32(&regs->cr[2], CR2_RXEN);
	return ret;
}

static int ftssp010_spi_hard_xfer8_v1(struct spi_device *spi,
	const u8 *tx_buf, u8 *rx_buf, u32 count)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->master);
	struct ftssp010_regs *regs = priv->mmio;
	u32 i, tmp, size, mask;

	while (count > 0) {
		mask = CR2_TXDOE;
		if ((readl(&regs->cr[2]) & mask) != mask)
			setbits_le32(&regs->cr[2], mask);

		size = min_t(u32, priv->txfifo_depth, count);
		for (i = 0; i < size; ++i) {
			ftssp010_wait_txfifo_ready(priv);
			writel(tx_buf ? (*tx_buf++) : 0, &regs->dr);
		}

		size = min_t(u32, priv->rxfifo_depth, count);
		for (i = 0; i < size; ++i) {
			ftssp010_wait_rxfifo_ready(priv);
			tmp = readl(&regs->dr);
			if (rx_buf)
				*rx_buf++ = (uint8_t)tmp;
		}

		count -= size;
	}

	return 0;
}

static unsigned int ftssp010_spi_hard_xfer(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->master);
	unsigned int count = xfer->len;
	const void *tx = xfer->tx_buf;
	void *rx = xfer->rx_buf;
	if (priv->rev >= 0x00011900) {
		if (ftssp010_spi_hard_xfer8_v2(spi, tx, rx, count) < 0)
			goto out;
	} else {
		if (ftssp010_spi_hard_xfer8_v1(spi, tx, rx, count) < 0)
			goto out;
	}

	count = 0;
out:
	return xfer->len - count;
}

static int ftssp010_spi_setup(struct spi_device *spi)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->controller);
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long val;
	if (spi->bits_per_word == 0) {
		dev_err(&spi->dev, "setup: invalid transfer bits_per_word\n");
		return -EINVAL;
	}

	if (spi->mode & (~MODEBITS)) {
		dev_err(&spi->dev, "setup: unsupported mode bits %x\n", spi->mode);
		return -EINVAL;
	}

	// Setup FS polarity
	val = readl(&regs->cr[2]);
	if (spi->mode & SPI_CS_HIGH) {
		val &= ~CR2_FS;    // active high
	} else {
		val |= CR2_FS;     // active low
	}
	writel(val, &regs->cr[2]);

	return 0;
}

static int ftssp010_spi_prepare_transfer_hardware(struct spi_controller *ctlr)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(ctlr);
	struct ftssp010_regs *regs = priv->mmio;
	unsigned long mode = 0;
	if (ctlr->cur_msg->spi->mode & SPI_CPHA)
		mode |= CR0_SCLKPH;
	if (ctlr->cur_msg->spi->mode & SPI_CPOL)
		mode |= CR0_SCLKPO;

	if (spi_controller_is_slave(ctlr)) {
		writel(CR0_OPM_SLAVE | CR0_FFMT_SPI | mode, &regs->cr[0]);
	} else {
		if (priv->rev >= 0x00012800) {
#ifdef CONFIG_SPI_FTSSP010_FLASH
			writel(CR0_OPM_MASTER | CR0_FFMT_SPI | CR0_FLASH | CR0_FLASHTX | mode, &regs->cr[0]);
#else
			writel(CR0_OPM_MASTER | CR0_FFMT_SPI | CR0_SPICONTX | mode, &regs->cr[0]);
#endif
		} else if (priv->rev >= 0x00011900) {
#ifdef CONFIG_SPI_FTSSP010_FLASH
			writel(CR0_OPM_MASTER | CR0_FFMT_SPI | CR0_FLASH | mode, &regs->cr[0]);
#else
			writel(CR0_OPM_MASTER | CR0_FFMT_SPI | mode, &regs->cr[0]);
#endif
		} else {
			writel(CR0_OPM_MASTER | CR0_FFMT_SPI | mode, &regs->cr[0]);
		}
	}

	writel(CR2_SSPRST | CR2_TXFCLR | CR2_RXFCLR | CR2_TXEN | CR2_TXDOE | CR2_SSPEN, &regs->cr[2]);

	return 0;
}

static void ftssp010_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct ftssp010_spi *priv = spi_master_get_devdata(spi->master);
	struct ftssp010_regs *regs = priv->mmio;
	struct ftgpio010_regs *gpio = priv->gpio;
	uint32_t mask = BIT_MASK(spi->chip_select);
	uint32_t val;
	if (spi_controller_is_slave(spi->master))
		return;

	if (priv->rev >= 0x00011900) {
		val = readl(&regs->cr[2]);
		val &= ~(CR2_FS | CR2_FSOS(3));
		if (enable) {
			val |= CR2_FS | CR2_FSOS(spi->chip_select);
		} else {
			val |= CR2_FSOS(spi->chip_select);
		}
		writel(val, &regs->cr[2]);
	} else if (priv->gpio) {
		if (enable) {
			/* GPIO pull high */
			setbits_le32(&gpio->set, mask);
		} else {
			/* GPIO pull low */
			setbits_le32(&gpio->clr, mask);
		}
	}
}

static int ftssp010_spi_xfer_one(struct spi_master *master,
	struct spi_device *spi, struct spi_transfer *t)
{
	int status = 0;
	int len;

	if (t->speed_hz || t->bits_per_word) {
		status = ftssp010_spi_setup_transfer(spi, t);
		if (status < 0)
			return -EINVAL;
	}

	len = ftssp010_spi_hard_xfer(spi, t);

	spi_finalize_current_transfer(master);

	return len;
}

static int ftssp010_hw_setup(struct ftssp010_spi *priv, resource_size_t phys_base)
{
#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	struct device *dev = priv->dev; 
	struct dma_chan *dma_chan;
	dma_cap_mask_t mask;
#endif

#ifdef CONFIG_SPI_FTSSP010_USE_INT
	if (!priv->master->slave) {
		val = readl(&regs->icr);
		val |= ICR_TXCIEN | ICR_TFURIEN | ICR_RFORIEN;
		writel(val, &regs->icr);
	}
#endif

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	priv->dma_buf = dma_alloc_coherent(dev, FTSSP010_DMA_BUF_SIZE,
	                                   &priv->dmaaddr, GFP_KERNEL);
	if (!priv->dma_buf) {
		dev_err(dev, "Failed to allocate dma buffer\n");
		return -ENOMEM;
	}

	priv->slave.id = 0;

	if (of_property_read_u32(dev->of_node, "dma-req-sel",
		                     &priv->slave.handshake) < 0) {
		dev_err(dev, "Failed to get DMA select number\n");
		dma_free_coherent(dev, FTSSP010_DMA_BUF_SIZE, priv->dma_buf,
		                  priv->dmaaddr);
		return -EINVAL;
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	priv->slave.channels = 0xff;
#if defined(CONFIG_SPI_FTSSP010_USE_DMA_FTDMAC020)
	dma_chan = dma_request_channel(mask, ftdmac020_chan_filter, &priv->slave);
#elif defined(CONFIG_SPI_FTSSP010_USE_DMA_FTDMAC030)
	dma_chan = dma_request_channel(mask, ftdmac030_chan_filter, &priv->slave);
#endif
	if (dma_chan) {
		dev_info(dev, "Use DMA(%s) channel %d...\n", dev_name(dma_chan->device->dev),
		         dma_chan->chan_id);
	} else {
		dev_err(dev, "Failed to allocate DMA channel\n");
		dma_free_coherent(dev, FTSSP010_DMA_BUF_SIZE, priv->dma_buf,
		                  priv->dmaaddr);
		return -EBUSY;
	}

	priv->dma_chan = dma_chan;
	init_waitqueue_head(&priv->dma_wait_queue);

	priv->phys_io_port = virt_to_phys(&regs->dr);

	dev_info(dev, "Data Port 0x%08x Dma_buf 0x%08x, addr 0x%08x slave %d\n",
	         priv->phys_io_port, (int)priv->dma_buf, (int)priv->dmaaddr,
	         priv->slave.handshake);

	val = readl(&regs->icr);
	val |= ICR_TFDMAEN | ICR_RFDMAEN;
	writel(val, &regs->icr);
#endif

	return 0;
}

/*-------------------------------------------------------------------------*/

static int ftssp010_spi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct spi_master *master;
	struct ftssp010_spi *priv;
	struct ftssp010_regs *regs;
	struct resource *res, *res1;
	struct clk *clk;
	unsigned long val;
	int rc = -ENOMEM;
	u32 dev_id;

	if (of_property_read_bool(np, "spi-slave"))
		master = spi_alloc_slave(&pdev->dev, sizeof *priv);
	else
		master = spi_alloc_master(&pdev->dev, sizeof *priv);

	if (!master)
		goto out_free;

	if (pdev->dev.of_node)
		of_property_read_u32(pdev->dev.of_node, "dev_id", &dev_id);
	else
		dev_id = pdev->id;

	master->dev.of_node = pdev->dev.of_node;
	master->bus_num = dev_id;
	master->num_chipselect = 256;	/* dummy, it depends on the system GPIO */
	master->mode_bits = MODEBITS;
	master->setup = ftssp010_spi_setup;
	master->prepare_transfer_hardware = ftssp010_spi_prepare_transfer_hardware;
	master->set_cs = ftssp010_spi_set_cs;
	master->transfer_one = ftssp010_spi_xfer_one;
	platform_set_drvdata(pdev, master);

	priv = spi_master_get_devdata(master);
	if (!priv)
		goto out_free;

	memset(priv, 0, sizeof(*priv));

	clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Failed to get clock(0x%x).\n", (unsigned int)clk);
		goto out_free;
	}

	priv->clk = clk_get_rate(clk);
	priv->dev = &pdev->dev;
	priv->master = master;
	priv->max_speed = priv->clk / 2;
	priv->min_speed = priv->clk / 0x20000;

	spin_lock_init(&priv->lock);

	/* ftssp010 io-remap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		goto out_free;
	} else {
		priv->mmio = devm_ioremap_resource(&pdev->dev, res);
		if (!priv->mmio)
			goto out_free;
	}

	/* ftssp010-ftgpio010 io-remap */
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res1) {
		priv->gpio = devm_ioremap_resource(&pdev->dev, res1);
	}

	/* ftssp010 irq-request */
	rc = platform_get_irq(pdev, 0);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to get the irq\n");
		goto out_unmap_regs;
	}
	priv->irq = rc;

#ifdef CONFIG_SPI_FTSSP010_USE_INT
	rc = devm_request_irq(&pdev->dev, rc, ftssp010_isr,
	                      0, pdev->name, priv);
	if (rc) {
		dev_err(&pdev->dev, "failed to request irq.\n");
		goto out_unmap_regs;
	}
#endif

	/* check revision id */
	regs = priv->mmio;
	priv->rev = readl(&regs->revr);
	dev_info(&pdev->dev, "%s mode (rev. 0x%08x)\n",
	         priv->master->slave ? "slave" : "master", priv->rev);

	/* check tx/rx fifo depth */
	priv->txfifo_depth = FEAR_TXFIFO(readl(&regs->fear));
	priv->rxfifo_depth = FEAR_RXFIFO(readl(&regs->fear));
	dev_info(&pdev->dev, "tx fifo %d rx fifo %d\n",
	         priv->txfifo_depth, priv->rxfifo_depth);

	ftssp010_hw_setup(priv, res->start);

	rc = spi_register_master(master);
	if (rc) {
		dev_err(&pdev->dev, "failed at spi_register_master(...)\n");
		goto out_free_irq;
	}

	// Setup FS polarity
	val = readl(&regs->cr[0]);
	val |= CR0_FFMT_SPI;          // select set SPI mode
	writel(val, &regs->cr[0]);

	return 0;

out_free_irq:
	free_irq(priv->irq, priv);
out_unmap_regs:
	iounmap(priv->mmio);
	iounmap(priv->gpio);
out_free:
	spi_master_put(master);
	return rc;
}

static int ftssp010_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct ftssp010_spi *priv = spi_master_get_devdata(master);

	/* Release resources */
	if (priv->gpio) {
		iounmap(priv->gpio);
		priv->gpio = NULL;
	}
	iounmap(priv->mmio);
	priv->mmio = NULL;

	spi_unregister_master(master);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ftssp010_spi_suspend(struct device *dev)
{
	return 0;
}

static int ftssp010_spi_resume(struct device *dev)
{
	return 0;
}
static const struct dev_pm_ops ftssp010_spi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ftssp010_spi_suspend, ftssp010_spi_resume)
};
#define DEV_PM_OPS	(&ftssp010_spi_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id ftssp010_dt_match[] = {
	{.compatible = "faraday,ftssp010-spi" },
	{},
};
MODULE_DEVICE_TABLE(of, ftssp010_dt_match);

static struct platform_driver ftssp010_spi_driver = {
	.probe		= ftssp010_spi_probe,
	.remove		= ftssp010_spi_remove,
	.driver		= {
		.name = "ftssp010-spi",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(ftssp010_dt_match),
		.pm     = DEV_PM_OPS,
	},
};

static int __init ftssp010_spi_init(void)
{
	return platform_driver_register(&ftssp010_spi_driver);
}
module_init(ftssp010_spi_init);

static void __exit ftssp010_spi_exit(void)
{
	platform_driver_unregister(&ftssp010_spi_driver);
}
module_exit(ftssp010_spi_exit);

MODULE_DESCRIPTION("Faraday FTSSP010 SPI Controller driver");
MODULE_AUTHOR("Bo-Cun Chen <bcchen@faraday-tech.com>");
MODULE_LICENSE("GPL v2+");
MODULE_ALIAS("platform:ftssp010_spi");
