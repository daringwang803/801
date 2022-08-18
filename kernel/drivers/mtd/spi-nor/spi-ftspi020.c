/*
 * Faraday ftspi020 SPI Nor Flash Controller
 *
 * (C) Copyright 2019-2022 Faraday Technology
 * Bing-Yao Luo <bjluo@faraday-tech.com>
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
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/bitfield.h>

#include <linux/mtd/mtd.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/spi-nor.h>

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/ftdmac020.h>
#include <linux/dma/ftdmac030.h>
#endif

/******************************************************************************
 * SPI020 Registers
 *****************************************************************************/
#define FTSPI020_REG_CMD0		0x00	/* Flash address */
#define FTSPI020_REG_CMD1		0x04
#define FTSPI020_REG_CMD2		0x08
#define FTSPI020_REG_CMD3		0x0c
#define FTSPI020_REG_CTRL		0x10	/* Control */
#define FTSPI020_REG_AC_TIME		0x14
#define FTSPI020_REG_STS		0x18	/* Status */
#define FTSPI020_REG_ICR		0x20	/* Interrupt Enable */
#define FTSPI020_REG_ISR		0x24	/* Interrupt Status */
#define FTSPI020_REG_READ_STS		0x28
#define FTSPI020_REG_REVISION		0x50
#define FTSPI020_REG_FEATURE		0x54
#define FTSPI020_REG_DATA_PORT		0x100

/*
 * Chip select bit [9:8] offset 0xc
 */
#define FTSPI020_CMD3_CHIP_SEL_SHIFT	8
/*
 * Control Register offset 0x10
 */
#define FTSPI020_CTRL_READY_LOC_MASK	(~(0x7 << 16))
#define FTSPI020_CTRL_READY_LOC(x)	(((x) & 0x7) << 16)

#define FTSPI020_CTRL_DAMR_PORT		(1 << 20)
#define FTSPI020_CTRL_ABORT		(1 << 8)
#ifdef CONFIG_SPI_FTSPI020_NOR_V1_1_0
#define FTSPI020_CTRL_XIP_PORT_IDLE     (1 << 7)
#endif

#define FTSPI020_CTRL_CLK_MODE_MASK	(~(1 << 4))
#define FTSPI020_CTRL_CLK_MODE_0	(0 << 4)
#define FTSPI020_CTRL_CLK_MODE_3	(1 << 4)

#define FTSPI020_CTRL_CLK_DIVIDER_MASK	(~(3 << 0))
#define FTSPI020_CTRL_CLK_DIVIDER_2	(0 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_4	(1 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_6	(2 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_8	(3 << 0)

/*
 * Status Register offset 0x18
 */
#define	FTSPI020_STS_RFR	(1 << 1) /* RX FIFO ready */
#define	FTSPI020_STS_TFR	(1 << 0) /* TX FIFO ready */

/*
 * Interrupt Control Register
 */
#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
#define	FTSPI020_ICR_DMA_RX_THOD	GENMASK(22, 20) /* DMA RX FIFO threshold */
#define	FTSPI020_ICR_DMA_TX_THOD	GENMASK(18, 16) /* DMA TX FIFO threshold */
#define	FTSPI020_ICR_RX_XFR_SIZE	GENMASK(13, 12) /* RX Transfer size */
#define	FTSPI020_ICR_TX_XFR_SIZE	GENMASK( 9,  8) /* TX Transfer size */
#else
#define	FTSPI020_ICR_DMA_RX_THOD	GENMASK(13, 12) /* DMA RX FIFO threshold */
#define	FTSPI020_ICR_DMA_TX_THOD	GENMASK( 9,  8) /* DMA TX FIFO threshold */
#endif
#define	FTSPI020_ICR_CMD_CMPL	(1 << 1) /* Command complete enable */
#define	FTSPI020_ICR_DMA	(1 << 0) /* DMA handshake enable */

/*
 * Interrupt Status Register
 */
#define	FTSPI020_ISR_CMD_CMPL	(1 << 0) /* Command complete status */

/*
 * Feature Register
 */
#define FTSPI020_FEATURE_CLK_MODE(reg)		(((reg) >> 25) & 0x1)
#define FTSPI020_FEATURE_DTR_MODE(reg)		(((reg) >> 24) & 0x1)
#define FTSPI020_FEATURE_CMDhw_DEPTH(reg)	(((reg) >> 16) & 0xff)
#define FTSPI020_FEATURE_RXFIFO_DEPTH(reg)	(((reg) >>  8) & 0xff)
#define FTSPI020_FEATURE_TXFIFO_DEPTH(reg)	(((reg) >>  0) & 0xff)

/*
 * CMD1 Register offset 4: Command queue Second Word
 */
#define FTSPI020_CMD1_CONT_READ_MODE_EN         (1 << 28)
#define FTSPI020_CMD1_CONT_READ_MODE_DIS        (0 << 28)

#define FTSPI020_CMD1_OP_CODE_0_BYTE            (0 << 24)
#define FTSPI020_CMD1_OP_CODE_1_BYTE            (1 << 24)
#define FTSPI020_CMD1_OP_CODE_2_BYTE            (2 << 24)

#define FTSPI020_CMD1_DUMMY_CYCLE(x)    (((x) & 0xff) << 16)

#define FTSPI020_CMD1_ADDR_BYTES(x)	((x & 0x7) << 0)

/*
 * CMD3 Register offset 0xc: Command queue Fourth Word
 */
#define FTSPI020_CMD3_INSTR_CODE(x)     (((x) & 0xff) << 24)
#define FTSPI020_CMD3_CONT_READ_CODE(x) (((x) & 0xff) << 16)
#define FTSPI020_CMD3_CE(x)             (((x) & 0x3) << 8)
#define FTSPI020_CMD3_SERIAL_MODE       (0 << 5)
#define FTSPI020_CMD3_DUAL_MODE         (1 << 5)
#define FTSPI020_CMD3_QUAD_MODE         (2 << 5)
#define FTSPI020_CMD3_DUAL_IO_MODE      (3 << 5)
#define FTSPI020_CMD3_QUAD_IO_MODE      (4 << 5)
#define FTSPI020_CMD3_DTR_MODE_EN       (1 << 4)
#define FTSPI020_CMD3_DTR_MODE_DIS      (0 << 4)
#define FTSPI020_CMD3_STS_SW_READ       (1 << 3)
#define FTSPI020_CMD3_STS_HW_READ       (0 << 3)
#define FTSPI020_CMD3_RD_STS_EN         (1 << 2)
#define FTSPI020_CMD3_RD_STS_DIS        (0 << 2)
#define FTSPI020_CMD3_WRITE             (1 << 1)
#define FTSPI020_CMD3_READ              (0 << 1)
//A380 must NOT define SPI_FTSPI020_NOR_V1_1_0
#ifndef CONFIG_SPI_FTSPI020_NOR_V1_1_0
#define FTSPI020_CMD3_INTR_EN           (1 << 0)
#else
#define FTSPI020_CMD3_INTR_EN           (0 << 0)
#endif

#define MAX_CHIP_SELECT 	CONFIG_SPI_FTSPI020_NOR_MAX_CHIP_SELECT

struct spi020 {
	struct spi_nor nor[MAX_CHIP_SELECT];
	void __iomem *base;
	int irq;
	struct clk *clk, *clk_en;
	struct device *dev;
	struct completion c;
	u32 nor_list;
	int8_t txfifo_depth;
	int8_t rxfifo_depth;
#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
	u32 tx_transfer_size;
	u32 rx_transfer_size;
#endif

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
	int dma_sts;
#if defined(CONFIG_SPI_FTSPI020_NOR_USE_DMA_FTDMAC020)
	struct ftdmac020_dma_slave slave;
#elif defined(CONFIG_SPI_FTSPI020_NOR_USE_DMA_FTDMAC030)
	struct ftdmac030_dma_slave slave;
#endif
	struct dma_chan *dma_chan;
	wait_queue_head_t dma_wait_queue;

	unsigned char	*dma_buf;
	dma_addr_t	dmaaddr;
	dma_addr_t	phys_io_port;

	u32 dma_txfifo_thod;
	u32 dma_rxfifo_thod;
#endif
};

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA

#define DMA_COMPLETE	1
#define DMA_ONGOING	2
#define FTSPI020_DMA_BUF_SIZE   (4 * 1024 * 1024)

static void ftspi020_dma_callback(void *param)
{
	struct spi020 *hw = (struct spi020 *)param;

	BUG_ON(hw->dma_sts == DMA_COMPLETE);
	hw->dma_sts = DMA_COMPLETE;
	wake_up(&hw->dma_wait_queue);
}

static int ftspi020_dma_wait(struct spi020 *hw)
{
	int ret;

	ret = wait_event_timeout(hw->dma_wait_queue,
	      hw->dma_sts == DMA_COMPLETE, HZ);

	return ret;
}

#else

static void ftspi020_write_word(void __iomem *base, const void *data,
				int wsize)
{
	if (data) {
		switch (wsize) {
		case 1:
			writeb(*(const u8 *)data, base +
						  FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			writew(*(const u16 *)data, base +
						   FTSPI020_REG_DATA_PORT);
			break;

		default:
			writel(*(const u32 *)data, base +
						   FTSPI020_REG_DATA_PORT);
			break;
		}
	}

}

static void ftspi020_read_word(void __iomem *base, void *buf, int wsize)
{
	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = readb(base + FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			*(u16 *) buf = readw(base + FTSPI020_REG_DATA_PORT);
			break;

		default:
			*(u32 *) buf = readl(base + FTSPI020_REG_DATA_PORT);
			break;
		}
	}
}

static int ftspi020_txfifo_ready(void __iomem *base)
{
	return readl(base + FTSPI020_REG_STS) & FTSPI020_STS_TFR;
}

static int ftspi020_rxfifo_ready(void __iomem *base)
{
	return readl(base + FTSPI020_REG_STS) & FTSPI020_STS_RFR;
}
#endif

static int ftspi020_txrx(struct spi_nor *nor, const u8 *tx_buf,
			 u8 *rx_buf, int len, int bpw)
{
	struct spi020 *hw = nor->priv;
	int wsize;
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
	int count;
	struct dma_chan *chan;
	struct scatterlist sg;
	struct dma_async_tx_descriptor *desc;

	chan = hw->dma_chan;
#endif
	if (bpw <= 8)
		wsize = 1;
	else if (bpw <= 16)
		wsize = 2;
	else
		wsize = 4;

	while (len > 0) {
		int access_byte;

		if (tx_buf) {
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
			access_byte = min_t(int, len,
					    FTSPI020_DMA_BUF_SIZE);

			hw->dma_sts = DMA_ONGOING;
			memcpy(hw->dma_buf, tx_buf, access_byte);
			sg_init_one(&sg, dma_to_virt(hw->dev,
				    hw->dmaaddr), access_byte);
			sg_dma_address(&sg) = hw->dmaaddr;
			hw->slave.common.direction = DMA_TO_DEVICE;
			hw->slave.common.dst_addr = hw->phys_io_port;
			hw->slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			hw->slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
			if (access_byte % hw->dma_txfifo_thod)
				hw->slave.common.src_maxburst = 1;
			else
				hw->slave.common.src_maxburst = hw->dma_txfifo_thod;

			desc = chan->device->device_prep_slave_sg(chan,
				&sg, 1, DMA_TO_DEVICE, DMA_PREP_INTERRUPT |
				DMA_CTRL_ACK, NULL);
			if (desc == NULL) {
				dev_err(nor->dev, "TX DMA prepare slave sg failed\n");
				return -EINVAL;
			}
			desc->callback = ftspi020_dma_callback;
			desc->callback_param = hw;
			desc->tx_submit(desc);
			chan->device->device_issue_pending(chan);

			if (ftspi020_dma_wait(hw) <= 0) {
				dev_err(nor->dev, "TX Wait DMA timeout\n");
				return -ETIMEDOUT;
			}

			tx_buf += access_byte;
			len -= access_byte;
#else
#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
			access_byte = min_t(int, len, hw->tx_transfer_size);
#else
			access_byte = min_t(int, len, hw->txfifo_depth);
#endif
			len -= access_byte;

			while (!ftspi020_txfifo_ready(hw->base))
				;

			while (access_byte) {
				ftspi020_write_word(hw->base, tx_buf, wsize);

				tx_buf += wsize;
				access_byte -= wsize;
			}
#endif
		} else if (rx_buf) {
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
			int dma_map, mod_len;

			dma_map = virt_addr_valid(rx_buf) &&
				  !((int)rx_buf & 3);
			access_byte = min_t(int, len, FTSPI020_DMA_BUF_SIZE);

			/* DMA read unit is 4 bytes(1 word), if read length
			 * not multiple of 4 bytes, add more bytes to make it
			 * becomes multiple of 4, but do not copy them to rx_buf.
			 */
			mod_len = access_byte & 0x3;
			if (mod_len)
				mod_len = 4 - mod_len;

			if (dma_map) {
				sg_init_one(&sg, (void *)rx_buf,
					    (access_byte + mod_len));

				count = dma_map_sg(hw->dev, &sg, 1, DMA_FROM_DEVICE);
				if (!count) {
					dev_err(nor->dev, "dma_map_sg failed\n ");
					return -EINVAL;
				}
			} else {
				sg_init_one(&sg, dma_to_virt(hw->dev,
					    hw->dmaaddr),
					    (access_byte + mod_len));
				sg_dma_address(&sg) = hw->dmaaddr;
			}

			hw->dma_sts = DMA_ONGOING;
			hw->slave.common.direction = DMA_FROM_DEVICE;
			hw->slave.common.src_addr = hw->phys_io_port;
			hw->slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			hw->slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			if ((access_byte + mod_len) % hw->dma_rxfifo_thod)
				hw->slave.common.src_maxburst = 1;
			else
				hw->slave.common.src_maxburst = hw->dma_rxfifo_thod / 4;

			desc = chan->device->device_prep_slave_sg(chan,
				&sg, 1, DMA_FROM_DEVICE, DMA_PREP_INTERRUPT |
				DMA_CTRL_ACK, NULL);
			if (desc == NULL) {
				dev_err(nor->dev, "RX DMA prepare slave sg failed\n");
				return -EINVAL;
			}
			desc->callback = ftspi020_dma_callback;
			desc->callback_param = hw;
			desc->tx_submit(desc);
			chan->device->device_issue_pending(chan);

			if (ftspi020_dma_wait(hw) <= 0) {
				if (dma_map) {
					dma_unmap_sg(hw->dev, &sg, 1, DMA_FROM_DEVICE);
					dev_err(nor->dev, "DMA_MAP: RX Wait DMA timeout\n");
				} else {
					dev_err(nor->dev, "RX Wait DMA timeout\n");
				}
				return -ETIMEDOUT;
			}

			if (dma_map) {
				dma_unmap_sg(hw->dev, &sg, 1, DMA_FROM_DEVICE);
			} else {
				memcpy(rx_buf, hw->dma_buf, access_byte);
			}

			rx_buf += access_byte;
			len -= access_byte;
#else
			while (!ftspi020_rxfifo_ready(hw->base))
				;

#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
			access_byte = min_t(int, len, hw->rx_transfer_size);
#else
			access_byte = min_t(int, len, hw->rxfifo_depth);
#endif
			len -= access_byte;

			while (access_byte) {
				ftspi020_read_word(hw->base, rx_buf,
						   wsize);

				rx_buf += wsize;
				access_byte -= wsize;
			}
#endif
		}
	}

	return len;
}

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_INT
static irqreturn_t ftspi020_isr(int irq, void *dev_id)
{
	struct spi020 *hw = dev_id;
	u32 reg;

	/* clear interrupt */
	reg = readl(hw->base + FTSPI020_REG_ISR);
	writel(reg, hw->base + FTSPI020_REG_ISR);

	if (reg & FTSPI020_ISR_CMD_CMPL)
		complete(&hw->c);

	return IRQ_HANDLED;
}
#else
/*
 * @timeout: timeout value in jiffies
 */
static int ftspi020_wait_complete(struct spi_nor *nor, signed long timeout)
{
	struct spi020 *hw = nor->priv;
	unsigned long expire;
	u32 isr;

	expire = timeout + jiffies;

	do {
		isr = readl(hw->base + FTSPI020_REG_ISR);
		writel(isr, hw->base + FTSPI020_REG_ISR);

		if (isr & FTSPI020_ISR_CMD_CMPL)
			goto out;

		cond_resched();
	} while (time_before(jiffies, expire));

	timeout = expire - jiffies;
out:
	return timeout < 0 ? 0 : timeout;
}
#endif

static void ftspi020_reset_ctrl(struct spi_nor *nor)
{
	struct spi020 *hw = nor->priv;
	int val;

	/* Set DAMR port selection to Command based */
	val = readl(hw->base + FTSPI020_REG_CTRL);
	val |= FTSPI020_CTRL_ABORT;
	writel(val, hw->base + FTSPI020_REG_CTRL);

	while (readl(hw->base + FTSPI020_REG_CTRL) & FTSPI020_CTRL_ABORT);
}

static int ftspi020_firecmd(struct spi_nor *nor, u32 cmd3,
			    const u8 *tx_buf, u8 *rx_buf, int len, int bpw)
{
	struct spi020 *hw = nor->priv;
	int ret;

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_INT
	init_completion(&hw->c);
#endif

#if 0
	dev_dbg(nor->dev, "cmd0: 0x%08x\n",
		readl(hw->base + FTSPI020_REG_CMD0));
	dev_dbg(nor->dev, "cmd1: 0x%08x\n",
		readl(hw->base + FTSPI020_REG_CMD1));
	dev_dbg(nor->dev, "cmd2: 0x%08x\n",
		readl(hw->base + FTSPI020_REG_CMD2));
	dev_dbg(nor->dev, "cmd3: 0x%08x\n",
		cmd3);
#endif

	writel((cmd3 | FTSPI020_CMD3_INTR_EN), hw->base + FTSPI020_REG_CMD3);

	if (tx_buf || rx_buf) {

		ret = ftspi020_txrx(nor, tx_buf, rx_buf, len, bpw);
		if (ret)
			goto out;
	}

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_INT
	/* Wait for the interrupt. */
	ret = wait_for_completion_timeout(&hw->c, msecs_to_jiffies(1000));
#else
	ret = ftspi020_wait_complete(nor, msecs_to_jiffies(1000));
#endif
	if (!ret) {
		dev_err(nor->dev,
			"timeout: cmd0 0x%08x cmd1 0x%08x " \
			"cmd2 0x%08x cmd3 0x%08x\n",
			readl(hw->base + FTSPI020_REG_CMD0),
			readl(hw->base + FTSPI020_REG_CMD1),
			readl(hw->base + FTSPI020_REG_CMD2),
			readl(hw->base + FTSPI020_REG_CMD3));
		ret = -ETIMEDOUT;
		goto out;
	}

	return 0;
out:
	ftspi020_reset_ctrl(nor);
	return ret;
}

static int ftspi020_decode_proto(struct spi_nor *nor, const int read, u8 *op_mode)
{
	if (read) {
		switch (nor->read_proto) {
		case SNOR_PROTO_1_1_1:
			*op_mode = FTSPI020_CMD3_SERIAL_MODE;
			break;
		case SNOR_PROTO_1_1_2:
			*op_mode = FTSPI020_CMD3_DUAL_MODE;
			break;
		case SNOR_PROTO_1_1_4:
			*op_mode = FTSPI020_CMD3_QUAD_MODE;
			break;
		case SNOR_PROTO_1_2_2:
			*op_mode = FTSPI020_CMD3_DUAL_IO_MODE;
			break;
		case SNOR_PROTO_1_4_4:
			 *op_mode = FTSPI020_CMD3_QUAD_IO_MODE;
			break;
		default:
			dev_err(nor->dev, "Read proto %d error", nor->read_proto);
			return -EINVAL;
		}
	} else {
		switch (nor->write_proto) {
		case SNOR_PROTO_1_1_1:
			*op_mode = FTSPI020_CMD3_SERIAL_MODE;
			break;
		case SNOR_PROTO_1_1_4:
			 *op_mode = FTSPI020_CMD3_QUAD_MODE;
			break;
		default:
			dev_err(nor->dev, "Write proto %d error", nor->write_proto);
			return -EINVAL;
		}
	}	

	return 0;
}

static int ftspi020_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
	struct spi020 *hw = nor->priv;
	u8  ce = nor - hw->nor;
	int ret;
	u32 cmd3;

	if (ce >= MAX_CHIP_SELECT) {
		dev_err(nor->dev, "ce %d out of max", ce);
		return -ENODEV;
	}

	if (len && !buf)
		return -EINVAL;

	writel(0, hw->base + FTSPI020_REG_CMD0);

	writel(FTSPI020_CMD1_OP_CODE_1_BYTE, hw->base + FTSPI020_REG_CMD1);

	cmd3 = (FTSPI020_CMD3_INSTR_CODE(opcode) |
		FTSPI020_CMD3_CE(ce) |
		FTSPI020_CMD3_SERIAL_MODE |
		FTSPI020_CMD3_READ);

#ifndef CONFIG_SPI_FTSPI020_NOR_V1_7_0
	if ((opcode == SPINOR_OP_RDSR) || (opcode == SPINOR_OP_RDCR)) {

		writel(0, hw->base + FTSPI020_REG_CMD2);

		cmd3 |= (FTSPI020_CMD3_RD_STS_EN | FTSPI020_CMD3_STS_SW_READ);
		ret = ftspi020_firecmd(nor, cmd3, NULL, NULL, 0, 8);

	} else
#endif
	{
		writel(len, hw->base + FTSPI020_REG_CMD2);

		ret = ftspi020_firecmd(nor, cmd3, NULL, buf, len, 8);
	}

	if (!ret) {
		//READ ID return all as zero means no chip
		//found, but spi-nor.c mis-interpret as
		//valid nor flash.
		if (opcode == SPINOR_OP_RDID) {
			if ((buf[0] == 0) && (buf[1] == 0))
				ret = -ENODEV;
		} else {
#ifndef CONFIG_SPI_FTSPI020_NOR_V1_7_0
			buf[0] = readl(hw->base + FTSPI020_REG_READ_STS);
#endif
		}
	}

	return ret;
}

static int ftspi020_write_reg(struct spi_nor *nor, u8 opcode, u8 *val, int len)
{
	struct spi020 *hw = nor->priv;
	u8  ce = nor - hw->nor;
	int ret;
	u32 cmd3;

	if (ce >= MAX_CHIP_SELECT) {
		dev_err(nor->dev, "ce %d out of max", ce);
		return -ENODEV;
	}

	if (len && !val)
		return -EINVAL;

	writel(0, hw->base + FTSPI020_REG_CMD0);

	writel(FTSPI020_CMD1_OP_CODE_1_BYTE, hw->base + FTSPI020_REG_CMD1);

	writel(len, hw->base + FTSPI020_REG_CMD2);

	cmd3 = (FTSPI020_CMD3_INSTR_CODE(opcode) |
		FTSPI020_CMD3_CE(ce) |
		FTSPI020_CMD3_SERIAL_MODE |
		FTSPI020_CMD3_WRITE);

	ret = ftspi020_firecmd(nor, cmd3, val, NULL, len, 8);

	return ret;
}

static ssize_t ftspi020_write(struct spi_nor *nor, loff_t to, size_t len,
			      const u_char *buf)
{
	struct spi020 *hw = nor->priv;
	u32 cmd3;
	int ret;
	u8  ce = nor - hw->nor;
	u8 op_mode;

	if (ce >= MAX_CHIP_SELECT) {
		dev_err(nor->dev, "ce %d out of max", ce);
		return -ENODEV;
	}

	writel((u32)to, hw->base + FTSPI020_REG_CMD0);

	writel(FTSPI020_CMD1_OP_CODE_1_BYTE |
	       FTSPI020_CMD1_ADDR_BYTES(nor->addr_width),
	       hw->base + FTSPI020_REG_CMD1);

	writel(len, hw->base + FTSPI020_REG_CMD2);

	ret = ftspi020_decode_proto(nor, 0, &op_mode);
	if (ret) {
		dev_err(nor->dev, "write: decode protocol");
		return ret;
	}

	cmd3 = (FTSPI020_CMD3_INSTR_CODE(nor->program_opcode) |
		FTSPI020_CMD3_CE(ce) | op_mode |
		FTSPI020_CMD3_WRITE);

	ret = ftspi020_firecmd(nor, cmd3, buf, NULL, len, 8);
	if (ret)
		return ret;

	return len;
}

/*
 * Read an address range from the nor chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static ssize_t ftspi020_read(struct spi_nor *nor, loff_t from, size_t len,
			     u_char *buf)
{
	struct spi020 *hw = nor->priv;
	u32 cmd3;
	int ret;
	u8  ce = nor - hw->nor;
	u8  op_mode; 

	if (ce >= MAX_CHIP_SELECT) {
		dev_err(nor->dev, "ce %d out of max", ce);
		return -ENODEV;
	}

	if (len && !buf)
		return -EINVAL;

	writel((u32)from, hw->base + FTSPI020_REG_CMD0);

	writel(FTSPI020_CMD1_OP_CODE_1_BYTE |
	       FTSPI020_CMD1_DUMMY_CYCLE(nor->read_dummy) |
	       FTSPI020_CMD1_ADDR_BYTES(nor->addr_width),
	       hw->base + FTSPI020_REG_CMD1);

	writel(len, hw->base + FTSPI020_REG_CMD2);

	ret = ftspi020_decode_proto(nor, 1, &op_mode);
	if (ret) {
		dev_err(nor->dev, "read: decode protocol");
		return ret;
	}

	cmd3 = (FTSPI020_CMD3_INSTR_CODE(nor->read_opcode) |
		FTSPI020_CMD3_CE(ce) | op_mode |
		FTSPI020_CMD3_READ);

	ret = ftspi020_firecmd(nor, cmd3, NULL, buf,  len, 8);
	if (ret)
		return ret;

	return len;
}

static int ftspi020_erase(struct spi_nor *nor, loff_t offset)
{
	struct spi020 *hw = nor->priv;
	u8  ce = nor - hw->nor;
	u32 cmd3;
	int ret;

	dev_dbg(nor->dev, "%dKiB at 0x%08x\n",
		nor->mtd.erasesize / 1024, (u32)offset);

	if (ce >= MAX_CHIP_SELECT) {
		dev_err(nor->dev, "ce %d out of max", ce);
		return -ENODEV;
	}

	writel((u32)offset, hw->base + FTSPI020_REG_CMD0);

	writel(FTSPI020_CMD1_OP_CODE_1_BYTE |
	       FTSPI020_CMD1_ADDR_BYTES(nor->addr_width),
	       hw->base + FTSPI020_REG_CMD1);

	writel(0, hw->base + FTSPI020_REG_CMD2);

	cmd3 = (FTSPI020_CMD3_INSTR_CODE(nor->erase_opcode) |
		FTSPI020_CMD3_CE(ce) |
		FTSPI020_CMD3_WRITE);

	ret = ftspi020_firecmd(nor, cmd3, NULL, NULL, 0, 0);

	return ret;
}

static int ftspi020_prepare(struct spi_nor *nor, enum spi_nor_ops ops)
{
	struct spi020 *hw = nor->priv;

	if (hw) {;}

	return 0;
}

static void ftspi020_unprepare(struct spi_nor *nor, enum spi_nor_ops ops)
{
	struct spi020 *hw = nor->priv;

	if (hw) {;}
}

static int ftspi020_hw_setup(struct spi020 *hw, resource_size_t phys_base)
{
	void __iomem *base = hw->base;
	u32 val, div, clk;
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
	struct device *dev = hw->dev;
	struct dma_chan *dma_chan;
	dma_cap_mask_t mask;
#endif

#ifdef CONFIG_SPI_FTSPI020_NOR_V1_1_0
	while (!(readl(base + FTSPI020_REG_CTRL) &
		FTSPI020_CTRL_XIP_PORT_IDLE)) {;}
#endif

	/* Calculate SPI clock divider */
	clk = clk_get_rate(hw->clk);
	for (div = 2; div < 8; div += 2) {
		if (clk / div <= CONFIG_SPI_FTSPI020_NOR_CLOCK)
			break;
	}

	/* Set SPI clock divider and DAMR port selection to Command based */
	val = readl(base + FTSPI020_REG_CTRL);
	val &= ~FTSPI020_CTRL_DAMR_PORT;
	val &= FTSPI020_CTRL_CLK_DIVIDER_MASK;
	val |= ((div >> 1) - 1);
	writel(val, base + FTSPI020_REG_CTRL);

	while ((readl(base + FTSPI020_REG_CTRL) &
		FTSPI020_CTRL_ABORT)) {;}

	/* busy bit location */
	val = readl(base + FTSPI020_REG_CTRL);
	val &= FTSPI020_CTRL_READY_LOC_MASK;
	val |= FTSPI020_CTRL_READY_LOC(0);
	writel(val, base + FTSPI020_REG_CTRL);

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_INT
	//The version of FTSPI020 at A380 EVB is 1.0.0
	//Command complete interrupt enable is at CMD3 offset=0x3 bit 0. 
#ifdef CONFIG_SPI_FTSPI020_NOR_V1_1_0
	val = readl(base + FTSPI020_REG_ICR);
	val |= FTSPI020_ICR_CMD_CMPL;
	writel(val, base + FTSPI020_REG_ICR);
#endif
#endif
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
	hw->dma_buf = dma_alloc_coherent(dev, FTSPI020_DMA_BUF_SIZE,
					 &hw->dmaaddr, GFP_KERNEL);
	if (!hw->dma_buf) {
		dev_err(hw->dev, "Failed to allocate dma buffer\n");
		return -ENOMEM;
	}

	hw->slave.id = 0;

	if (dev->platform_data)
		hw->slave.handshake = *((int *)dev->platform_data);
	else {
		if (of_property_read_u32(dev->of_node, "dma-req-sel",
				&hw->slave.handshake) < 0) {
			dev_err(hw->dev, "Failed to get DMA select number\n");
			dma_free_coherent(dev, FTSPI020_DMA_BUF_SIZE, hw->dma_buf,
					  hw->dmaaddr);
			return -EINVAL;
		}
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
#if defined(CONFIG_SPI_FTSPI020_NOR_USE_DMA_FTDMAC020)
	dma_chan = dma_request_channel(mask, ftdmac020_chan_filter, &hw->slave);
#elif defined(CONFIG_SPI_FTSPI020_NOR_USE_DMA_FTDMAC030)
		dma_chan = dma_request_channel(mask, ftdmac030_chan_filter, &hw->slave);
#endif
	if (!dma_chan) {
		dev_err(hw->dev, "Failed to allocate DMA channel at addr 0x%08x\n", hw->dmaaddr);
		dma_free_coherent(dev, FTSPI020_DMA_BUF_SIZE, hw->dma_buf,
				  hw->dmaaddr);
		return -EBUSY;
	}
	hw->dma_chan = dma_chan;
	init_waitqueue_head(&hw->dma_wait_queue);

#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
	hw->dma_rxfifo_thod = FIELD_GET(FTSPI020_ICR_DMA_RX_THOD, readl(hw->base + FTSPI020_REG_ICR)) == 0 ?
		4 : (1 << FIELD_GET(FTSPI020_ICR_DMA_RX_THOD, readl(hw->base + FTSPI020_REG_ICR))) << 3;
	hw->dma_txfifo_thod = FIELD_GET(FTSPI020_ICR_DMA_TX_THOD, readl(hw->base + FTSPI020_REG_ICR)) == 0 ?
		4 : (1 << FIELD_GET(FTSPI020_ICR_DMA_TX_THOD, readl(hw->base + FTSPI020_REG_ICR))) << 3;
#else
	hw->dma_rxfifo_thod = (FIELD_GET(FTSPI020_ICR_DMA_RX_THOD, readl(hw->base + FTSPI020_REG_ICR)) + 1) * 
		FTSPI020_FEATURE_RXFIFO_DEPTH(readl(hw->base + FTSPI020_REG_FEATURE));
	hw->dma_txfifo_thod = (FIELD_GET(FTSPI020_ICR_DMA_TX_THOD, readl(hw->base + FTSPI020_REG_ICR)) + 1) * 
		FTSPI020_FEATURE_TXFIFO_DEPTH(readl(hw->base + FTSPI020_REG_FEATURE));
#endif

	hw->phys_io_port = (dma_addr_t)phys_base + FTSPI020_REG_DATA_PORT;

	dev_info(dev, "Use DMA(%s) channel %d...\n", dev_name(dma_chan->device->dev),
		 dma_chan->chan_id);
	dev_info(dev, "Data Port 0x%08x Dma_buf 0x%08x, addr 0x%08x slave %d\n",
		 hw->phys_io_port, (int)hw->dma_buf, (int)hw->dmaaddr,
		 hw->slave.handshake);
	dev_info(dev, "dma tx threshold %d dma rx threshold %d\n",
		 hw->dma_txfifo_thod, hw->dma_rxfifo_thod);

	/* Enable DMA handshake */
	val = readl(base + FTSPI020_REG_ICR);
	val |= FTSPI020_ICR_DMA;
	writel(val, base + FTSPI020_REG_ICR);
#endif

	dev_info(hw->dev, "ctrl reg 0x%08x, icr reg 0x%08x\n",
		 readl(base + FTSPI020_REG_CTRL), readl(base + FTSPI020_REG_ICR));

	return 0;
}

/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int ftspi020_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct spi020 *hw;
	struct resource *res;
	struct spi_nor *nor;
	struct mtd_info *mtd;
	int ret, i;
	//A380 not have Quad capabilities, because W/P and HOLD line not connected
	//to SPI flash.
	const struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ
			| SNOR_HWCAPS_READ_FAST
			| SNOR_HWCAPS_PP
#if CONFIG_SPI_FTSPI020_NOR_FAST_READ_DUAL
			| SNOR_HWCAPS_READ_1_1_2
			| SNOR_HWCAPS_READ_1_2_2
#elif CONFIG_SPI_FTSPI020_NOR_FAST_READ_QUAD
			| SNOR_HWCAPS_READ_1_1_4
			| SNOR_HWCAPS_READ_1_4_4
			| SNOR_HWCAPS_PP_1_1_4
#endif
	};


	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return -ENOMEM;

	/* find the resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(hw->base)) {
		ret = PTR_ERR(hw->base);
		goto map_failed;
	}

	/* find the irq */
	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(dev, "failed to get the irq\n");
		goto map_failed;
	}
	hw->irq = ret;

#ifdef CONFIG_SPI_FTSPI020_NOR_USE_INT
	ret = devm_request_irq(dev, ret,
			ftspi020_isr, 0, pdev->name, hw);
	if (ret) {
		dev_err(dev, "failed to rehwuest irhw.\n");
		goto irq_failed;
	}
#endif
	hw->clk = clk_get(&pdev->dev, "spiclk");
	hw->dev = dev;
	ret = ftspi020_hw_setup(hw, res->start);
	if (ret)
		goto irq_failed;

	platform_set_drvdata(pdev, hw);

	hw->rxfifo_depth = FTSPI020_FEATURE_RXFIFO_DEPTH(
				readl(hw->base + FTSPI020_REG_FEATURE));
	hw->txfifo_depth = FTSPI020_FEATURE_TXFIFO_DEPTH(
				readl(hw->base + FTSPI020_REG_FEATURE));
	dev_info(dev, "reg base 0x%x, irq %d, tx fifo %d rx fifo %d\n",
		(u32)hw->base, hw->irq, hw->txfifo_depth, hw->rxfifo_depth);

#ifdef CONFIG_SPI_FTSPI020_NOR_V1_7_0
	hw->rx_transfer_size = (1 << FIELD_GET(FTSPI020_ICR_RX_XFR_SIZE, readl(hw->base + FTSPI020_REG_ICR))) * 32;
	hw->tx_transfer_size = (1 << FIELD_GET(FTSPI020_ICR_TX_XFR_SIZE, readl(hw->base + FTSPI020_REG_ICR))) * 32;
	dev_info(dev, "tx transfer %d, rx transfer %d\n",
		hw->tx_transfer_size, hw->rx_transfer_size);
#endif

	// iterate the subnodes.
	hw->nor_list = 0;
	for (i = 0; i < MAX_CHIP_SELECT; i++) {

		nor = &hw->nor[i];

		nor->dev = dev;
		spi_nor_set_flash_node(nor, np);
		nor->priv = hw;

		/* fill the hooks */
		nor->read = ftspi020_read;
		nor->write = ftspi020_write;
		nor->erase = ftspi020_erase;
		nor->write_reg = ftspi020_write_reg;
		nor->read_reg = ftspi020_read_reg;
		nor->prepare = ftspi020_prepare;
		nor->unprepare = ftspi020_unprepare;

		mtd = &nor->mtd;
		mtd->priv = nor;
		mtd->name = devm_kasprintf(dev, GFP_KERNEL, "%s.%d",
                                           dev_name(dev), i);
		if (!mtd->name) {
			ret = -ENOMEM;
			goto map_failed;
		}
 
		dev_info(dev, "Scanning Chip %d\n", i);
		ret = spi_nor_scan(nor, NULL, &hwcaps);
		if (ret) {
			if (ret == -ENODEV)
				continue;
			else
				goto irq_failed;
		}

		ret = mtd_device_register(mtd, NULL, 0);
		if (ret)
			continue;

		hw->nor_list |= (1 << i);
	}

	if (!hw->nor_list)
		goto map_failed;

	return 0;

irq_failed:
map_failed:
	devm_kfree(dev, hw);
	dev_err(dev, "Faraday FTSPI020 probe failed\n");

	return ret;
}

static int ftspi020_remove(struct platform_device *pdev)
{
	struct spi020 *hw = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < MAX_CHIP_SELECT; i++) {
		if (hw->nor_list & (1 << i))
			mtd_device_unregister(&hw->nor[i].mtd);
	}
#ifdef CONFIG_SPI_FTSPI020_NOR_USE_DMA
	dma_free_coherent(hw->dev, FTSPI020_DMA_BUF_SIZE,
			  hw->dma_buf, hw->dmaaddr);
	dma_release_channel(hw->dma_chan);
#endif
	devm_kfree(&pdev->dev, hw);

	return 0;
}

static const struct of_device_id ftspi020_dt_match[] = {
	{.compatible = "faraday,ftspi020-nor" },
	{},
};
MODULE_DEVICE_TABLE(of, ftspi020_dt_match);

static struct platform_driver ftspi020_driver = {
	.probe = ftspi020_probe,
	.remove = ftspi020_remove,
	.driver = {
		.name = "ftspi020-nor",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(ftspi020_dt_match),
		.pm = NULL,
	},
};
module_platform_driver(ftspi020_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BingYao Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTSPI020 driver for SPI NOR flash");

