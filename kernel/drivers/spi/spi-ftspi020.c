// SPDX-License-Identifier: GPL-2.0+

/*
 * Faraday FTSPI020 SPI memory driver.
 * Copyright (C) 2020 Faraday Technology Corporation.
 * Author: Bing-Yao, Luo <bjluo@faraday-tech.com>
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/sizes.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/mtd/spi-nor.h>

/******************************************************************************
 * FTSPI020 Registers
 *****************************************************************************/
#define FTSPI020_REG_CMD0               0x00	/* Flash address */
#define FTSPI020_REG_CMD1               0x04
#define FTSPI020_REG_CMD2               0x08
#define FTSPI020_REG_CMD3               0x0c
#define FTSPI020_REG_CTRL               0x10	/* Control */
#define FTSPI020_REG_AC_TIME            0x14
#define FTSPI020_REG_STS                0x18	/* TX/RX FIFO Status */
#define FTSPI020_REG_CR2                0x20
#define FTSPI020_REG_ISR                0x24	/* Interrupt Status */
#define FTSPI020_REG_XIPCMD             0x30
#define FTSPI020_REG_REVISION           0x50
#define FTSPI020_REG_FEATURE            0x54
#define FTSPI020_REG_DATA_PORT          0x100

/*
 * Control Register offset 0x10
 */
#ifndef CONFIG_SPI_FTSPI020_V1_7_0
#define FTSPI020_CTRL_DAMR_PORT         BIT(20)
#endif
#define FTSPI020_CTRL_READY_LOC         GENMASK(18, 16)
#ifndef CONFIG_SPI_FTSPI020_V1_7_0
#define FTSPI020_CTRL_ABORT             BIT(8)
#endif
#define FTSPI020_CTRL_XIP_PORT_IDLE     BIT(7)
#define FTSPI020_CTRL_CLK_MODE_3        BIT(4)
#define FTSPI020_CTRL_CLK_DIVIDER       GENMASK(1, 0)

enum {
	CLK_DIVIDER_2 = 0,
	CLK_DIVIDER_4 = 1,
	CLK_DIVIDER_6 = 2,
	CLK_DIVIDER_8 = 3
};

/*
 * Status Register offset 0x18
 */
#define	FTSPI020_STS_RFR                BIT(1) /* RX FIFO ready */
#define	FTSPI020_STS_TFR                BIT(0) /* TX FIFO ready */

/*
 * Control 2 Register
 */
#define	FTSPI020_CR2_DMA_RX_THOD        GENMASK(22, 20)
#define	FTSPI020_CR2_DMA_TX_THOD        GENMASK(18, 16)
#define	FTSPI020_CR2_RX_XFR_SIZE        GENMASK(13, 12)
#define	FTSPI020_CR2_TX_XFR_SIZE        GENMASK(9, 8)
#define	FTSPI020_CR2_CMD_CMPL           BIT(1) /* Command complete enable */
#define	FTSPI020_CR2_DMA_EN             BIT(0) /* DMA handshake enable */

enum {
	DMA_THOD_4BYTES = 0,
	DMA_THOD_16BYTES = 1,
	DMA_THOD_32BYTES = 2,
	DMA_THOD_64BYTES = 3,
	DMA_THOD_128BYTES = 4,
	DMA_THOD_256BYTES = 5
};

enum {
	XFR_SIZE_32BYTES = 0,
	XFR_SIZE_64BYTES = 1,
	XFR_SIZE_128BYTES = 2,
	XFR_SIZE_256BYTES = 3
};

/*
 * Interrupt Status Register
 */
#define	FTSPI020_ISR_CMD_CMPL           BIT(0) /* Command complete status */

/*
 * Feature Register
 */
#define FTSPI020_FEATURE_DAMR           BIT(29)
#define FTSPI020_FEATURE_DTR_MODE       BIT(24)
#define FTSPI020_FEATURE_RXFIFO_DEPTH   GENMASK(15, 8)
#define FTSPI020_FEATURE_TXFIFO_DEPTH   GENMASK(7, 0)

/*
 * CMD1 Register offset 4: Command queue Second Word
 */
#define FTSPI020_CMD1_CONT_READMODE_EN  BIT(28)
#define FTSPI020_CMD1_OPCODE_BYTE       GENMASK(25, 24)
#define FTSPI020_CMD1_DUMMY_CYCLE       GENMASK(23, 16)
#define FTSPI020_CMD1_ADDR_BYTES        GENMASK(2, 0)

/*
 * CMD3 Register offset 0xc: Command queue Fourth Word
 */
#define FTSPI020_CMD3_INSTR_CODE        GENMASK(31, 24)
#define FTSPI020_CMD3_CONTREAD_CODE     GENMASK(23, 16)
#define FTSPI020_CMD3_CHIP_SELECT       GENMASK(9, 8)
#define FTSPI020_CMD3_PROTO             GENMASK(7, 5)
enum {
	CMD3_PROTO_SERIAL = 0,  /* 1-1-1 */
	CMD3_PROTO_DUAL,        /* 1-1-2 */
	CMD3_PROTO_QUAD,        /* 1-1-4 */
	CMD3_PROTO_DUALIO,      /* 1-2-2 */
	CMD3_PROTO_QUADIO,      /* 1-4-4 */
	CMD3_PROTO_OPCODE_2BIT, /* 2-2-2 */
	CMD3_PROTO_OPCODE_4BIT, /* 4-4-4 */
};

#define FTSPI020_CMD3_DTR_MODE_EN       BIT(4)
#define FTSPI020_CMD3_OP_MODE           GENMASK(3, 1)
enum {
	CMD3_OP_MODE_DIR_IN = 0,
	CMD3_OP_MODE_DIR_OUT,
	CMD3_OP_MODE_NODATA, //Send READ Status(0x5) and poll WIP bit
};

#ifdef CONFIG_SPI_FTSPI020_V1_7_0
#define FTSPI020_CMD3_SPI_NAND          BIT(0)
#endif

/*
 * CPU direct access memory setting
 */
#define FTSPI020_XIPCMD_CONTREAD_ENABLE BIT(28)
#define FTSPI020_XIPCMD_CONTREAD_CODE   GENMASK(27, 20)
#define FTSPI020_XIPCMD_INSTR_CODE      GENMASK(19, 12)
#define FTSPI020_XIPCMD_ADDR_BYTES      BIT(11)
#define FTSPI020_XIPCMD_PROTO           GENMASK(10, 8)
#define FTSPI020_XIPCMD_DUMMY_CYCLE     GENMASK(7, 0)

enum {
	XIPCMD_ADDR_3BYTES = 0,
	XIPCMD_ADDR_4BYTES,
};

#define FTSPI020_DAMR_MAX_SIZE          SZ_32M

struct ftspi020_priv {
	void __iomem *io_base;
	void __iomem *damr_base;
	resource_size_t	damr_size;
	struct clk *clk;
	struct device *dev;
	struct spi_controller *master;
	struct completion cmd_completion;
	struct mutex lock;
	u32 tx_transfer_size;
	u32 rx_transfer_size;
	u8 txfifo_depth;
	u8 rxfifo_depth;

	struct dma_chan *dma_ch;
	struct completion dma_completion;
	phys_addr_t phys_io_base;
};

#ifdef CONFIG_SPI_FTSPI020_USE_INT
static irqreturn_t ftspi020_isr(int irq, void *dev_id)
{
	struct ftspi020_priv *ctrl = dev_id;
	u32 reg;

	/* clear interrupt */
	reg = ioread32(ctrl->io_base + FTSPI020_REG_ISR);
	iowrite32(reg, ctrl->io_base + FTSPI020_REG_ISR);

	if (reg & FTSPI020_ISR_CMD_CMPL)
		complete(&ctrl->cmd_completion);

	return IRQ_HANDLED;
}
#else
/*
 * @timeout: timeout value in jiffies
 */
static int ftspi020_wait_complete(ftspi020_priv *ctrl, signed long timeout)
{
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

static void ftspi020_dma_callback(void *param)
{
	struct completion *c = (struct completion *)param;

	complete(c);
}

static int ftspi020_do_data_dma(struct ftspi020_priv *ctrl, const struct spi_mem_op *op)
{
	struct dma_async_tx_descriptor *desc;
	struct dma_slave_config dma_cfg;
	struct sg_table sgt;
	dma_cookie_t cookie;
	u32 val;
	int err;

	memset(&dma_cfg, 0, sizeof(dma_cfg));

	if (op->data.dir == SPI_MEM_DATA_IN) {
		dma_cfg.direction = DMA_DEV_TO_MEM;
		dma_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_cfg.src_addr = (dma_addr_t)ctrl->phys_io_base + FTSPI020_REG_DATA_PORT;
		dma_cfg.src_maxburst = 4;
		dma_cfg.dst_maxburst = 4;
	} else {
		dma_cfg.direction = DMA_MEM_TO_DEV;
		dma_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_cfg.dst_addr = (dma_addr_t)ctrl->phys_io_base + FTSPI020_REG_DATA_PORT;
		dma_cfg.src_maxburst = 4;
		dma_cfg.dst_maxburst = 4;
	}

	err = dmaengine_slave_config(ctrl->dma_ch, &dma_cfg);
	if (err) {
		dev_err(ctrl->dev, "DMA slave config failed(%d)\n", err);
		return err;
	}

	err = spi_controller_dma_map_mem_op_data(ctrl->master, op, &sgt);
	if (err)
		return err;

	desc = dmaengine_prep_slave_sg(ctrl->dma_ch, sgt.sgl, sgt.nents,
				       dma_cfg.direction, (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
	if (!desc) {
		dev_err(ctrl->dev, "DMA prepare slave sg failed\n");
		err = -ENOMEM;
		goto out_unmap;
	}

	val = ioread32(ctrl->io_base + FTSPI020_REG_CR2);

	reinit_completion(&ctrl->dma_completion);
	desc->callback = ftspi020_dma_callback;
	desc->callback_param = &ctrl->dma_completion;
	cookie = dmaengine_submit(desc);
	err = dma_submit_error(cookie);
	if (err)
		goto out;

	dma_async_issue_pending(ctrl->dma_ch);

	iowrite32(val | FTSPI020_CR2_DMA_EN, ctrl->io_base + FTSPI020_REG_CR2);

#ifdef CONFIG_SPI_FTSPI020_USE_INT
	if (!wait_for_completion_timeout(&ctrl->dma_completion, msecs_to_jiffies(5000)))
#else
	if (!ftspi020_wait_complete(ctrl, msecs_to_jiffies(5000)))
#endif
		err = -ETIMEDOUT;

	if (err)
		dmaengine_terminate_all(ctrl->dma_ch);

out:
	iowrite32(val & ~FTSPI020_CR2_DMA_EN, ctrl->io_base + FTSPI020_REG_CR2);
out_unmap:
	spi_controller_dma_unmap_mem_op_data(ctrl->master, op, &sgt);

	return err;
}

static void ftspi020_write_word(void __iomem *base, void *data, u32 wsize)
{
	if (data) {
		switch (wsize) {
			case 1:
			iowrite8(*(u8 *)data, base + FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			iowrite16(*(u16 *)data, base + FTSPI020_REG_DATA_PORT);
			break;

		default:
			iowrite32(*(u32 *)data, base + FTSPI020_REG_DATA_PORT);
			break;
		}
	}

}

static void ftspi020_read_word(void __iomem *base, void *buf, u32 wsize)
{
	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = ioread8(base + FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			*(u16 *) buf = ioread16(base + FTSPI020_REG_DATA_PORT);
			break;

		default:
			*(u32 *) buf = ioread32(base + FTSPI020_REG_DATA_PORT);
			break;
		}
	}
}

static int ftspi020_do_data_pio(struct ftspi020_priv *ctrl, const struct spi_mem_op *op)
{
	void (*access_fifo)(void __iomem *base, void *buf, u32 wsize);
	int ret = 0;
	u8 *buf;
	u32 len, val, wsize = 1;
	u32 transfer_size;
	u8 fifo_ready_mask;

	len = op->data.nbytes;

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		buf = (u8 *)op->data.buf.out;
		transfer_size = ctrl->tx_transfer_size;
		fifo_ready_mask = FTSPI020_STS_TFR;
		access_fifo = ftspi020_write_word;
	} else {
		buf = op->data.buf.in;
		transfer_size = ctrl->rx_transfer_size;
		fifo_ready_mask = FTSPI020_STS_RFR;
		access_fifo = ftspi020_read_word;
	}

	while (len > 0) {
		int access_byte;

		access_byte = min_t(u32, len, transfer_size);

		/* Wait for txfifo or rxfifo ready */
		ret = readl_relaxed_poll_timeout_atomic(ctrl->io_base + FTSPI020_REG_STS, val,
							(val & fifo_ready_mask), 1,
							3000000);
		if (ret) {
			dev_err(ctrl->dev, "wait fifo timeout(len:%d(%d) sts:%#x(%#x))\n",
				len, op->data.nbytes, val, fifo_ready_mask);
			return ret;
		}

		len -= access_byte;
		while (access_byte) {
			access_fifo(ctrl->io_base, buf, wsize);
			buf += wsize;
			access_byte -= wsize;
		}
	}

	return ret;
}

static int ftspi020_firecmd(struct ftspi020_priv *ctrl, u32 cmd3, const struct spi_mem_op *op)
{
	int err = 0;

	init_completion(&ctrl->cmd_completion);

	/* HW starts the command */
	iowrite32(cmd3, ctrl->io_base + FTSPI020_REG_CMD3);

	if (op->data.nbytes) {
		dev_dbg(ctrl->dev, "%s: nbytes %d, dir %s, dma_ch %px\n",
			__func__, op->data.nbytes,
			(op->data.dir == SPI_MEM_DATA_IN) ? "IN":"OUT", ctrl->dma_ch);
		if (ctrl->dma_ch)
			err = ftspi020_do_data_dma(ctrl, op);
		else
			err = ftspi020_do_data_pio(ctrl, op);
	}

	/* Wait for the interrupt. */
#ifdef CONFIG_SPI_FTSPI020_USE_INT
	if (!wait_for_completion_timeout(&ctrl->cmd_completion, msecs_to_jiffies(1000))) {
#else
	if (!ftspi020_wait_complete(ctrl, msecs_to_jiffies(1000)))
#endif
		dev_err(ctrl->dev, "wait cmd complete timeout\n");
		err = -ETIMEDOUT;
	}

	return err;
}

/**
 * CMD3_PROTO_SERIAL       (0 << 5) // 1-1-1
 * CMD3_PROTO_DUAL         (1 << 5) // 1-1-2
 * CMD3_PROTO_QUAD         (2 << 5) // 1-1-4
 * CMD3_PROTO_DUALIO       (3 << 5) // 1-2-2
 * CMD3_PROTO_QUADIO       (4 << 5) // 1-4-4
 * CMD3_PROTO_OPCODE_2BIT  (5 << 5) // 2-2-2
 * CMD3_PROTO_OPCODE_4BIT  (6 << 5) // 4-4-4
 */
static int ftspi020_decode_proto(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct ftspi020_priv *ctrl = spi_controller_get_devdata(mem->spi->master);
	int cmd_width, addr_width, data_width, ret;
	u32 proto;

	cmd_width = (op->cmd.buswidth) ? op->cmd.buswidth : 1;

	addr_width = (op->addr.nbytes) ? op->addr.buswidth : 1;

	data_width = (op->data.nbytes) ? op->data.buswidth : 1;

	/* Borrow SPI_NOR_ protocol definition */
	proto = SNOR_PROTO_STR(cmd_width, addr_width, data_width);

	switch (proto) {
	case SNOR_PROTO_1_1_1:
		ret = CMD3_PROTO_SERIAL;
		break;
	case SNOR_PROTO_1_1_2:
		ret = CMD3_PROTO_DUAL;
		break;
	case SNOR_PROTO_1_1_4:
		ret = CMD3_PROTO_QUAD;
		break;
	case SNOR_PROTO_1_2_2:
		ret = CMD3_PROTO_DUALIO;
		break;
	case SNOR_PROTO_1_4_4:
		ret = CMD3_PROTO_QUADIO;
		break;
	case SNOR_PROTO_2_2_2:
		ret = CMD3_PROTO_OPCODE_2BIT;
		break;
	case SNOR_PROTO_4_4_4:
		ret = CMD3_PROTO_OPCODE_4BIT;
		break;
	default:
		dev_err(ctrl->dev, "Decode protocol %d-%d-%d error",
			cmd_width, addr_width, data_width);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * CMD3_OP_MODE_DIR_IN = 0,
 * CMD3_OP_MODE_DIR_OUT,
 * CMD3_OP_MODE_NODATA, //Send READ Status(0x5) and poll WIP bit
 */
static u32 ftspi020_get_op_mode(const struct spi_mem_op *op)
{
	u32 ret = CMD3_OP_MODE_DIR_OUT;

	/*
	 * Kernel 4.19 doesn't have SPI_MEM_NO_DATA(version 5.x).
	 * No data means OUT direction.
	 */
	if (op->data.nbytes && (op->data.dir == SPI_MEM_DATA_IN)) {
		ret = CMD3_OP_MODE_DIR_IN;
	}

	/*
	 * For FTSPI020 no-data means HW auto poll WIP bit by sending
	 * READ Status(0x5). I think don't really need this because
	 * SPI memory framework will do this.
	 */
#if 0
	if (!op->data.nbytes) {
		ret |= CMD3_OP_MODE_NODATA;
	}
#endif
	return ret;
}

static int ftspi020_exec_op_by_cmd(struct ftspi020_priv *ctrl,
				   const struct spi_mem_op *op,
				   u32 chip_select, int proto)
{
	u32 cmd;
	int err = 0;

	/* No nbytes for cmd.opcode, fixed to one byte */
	cmd = FIELD_PREP(FTSPI020_CMD1_OPCODE_BYTE, 1);

	if (op->addr.nbytes) {
		cmd |= FIELD_PREP(FTSPI020_CMD1_ADDR_BYTES, op->addr.nbytes);

		iowrite32(op->addr.val, ctrl->io_base + FTSPI020_REG_CMD0);
	} else {
		iowrite32(0, ctrl->io_base + FTSPI020_REG_CMD0);
	}

	if (op->dummy.buswidth && op->dummy.nbytes)
		cmd |= FIELD_PREP(FTSPI020_CMD1_DUMMY_CYCLE,
				  op->dummy.nbytes * 8 / op->dummy.buswidth);

	iowrite32(cmd, ctrl->io_base + FTSPI020_REG_CMD1);

	if (op->data.nbytes)
		iowrite32(op->data.nbytes, ctrl->io_base + FTSPI020_REG_CMD2);
	else
		iowrite32(0, ctrl->io_base + FTSPI020_REG_CMD2);

	cmd = (FIELD_PREP(FTSPI020_CMD3_CHIP_SELECT, chip_select) |
	       FIELD_PREP(FTSPI020_CMD3_INSTR_CODE, op->cmd.opcode));
	cmd |= FIELD_PREP(FTSPI020_CMD3_PROTO, proto);
	cmd |= FIELD_PREP(FTSPI020_CMD3_OP_MODE, ftspi020_get_op_mode(op));
#ifdef CONFIG_SPI_FTSPI020_V1_7_0
	cmd |= FIELD_PREP(FTSPI020_CMD3_SPI_NAND, 1);
#endif
/*printk("cmd0: %08x, cmd1: %08x, cmd2: %08x, cmd3: %08x\n",
		ioread32(ctrl->io_base + FTSPI020_REG_CMD0),
		ioread32(ctrl->io_base + FTSPI020_REG_CMD1),
		ioread32(ctrl->io_base + FTSPI020_REG_CMD2), cmd);*/
	err = ftspi020_firecmd(ctrl, cmd, op);
	if (err < 0)
		dev_err(ctrl->dev, "cmd0: %08x, cmd1: %08x, cmd2: %08x, cmd3: %08x\n",
			ioread32(ctrl->io_base + FTSPI020_REG_CMD0),
			ioread32(ctrl->io_base + FTSPI020_REG_CMD1),
			ioread32(ctrl->io_base + FTSPI020_REG_CMD2), cmd);

	return err;
}

static int ftspi020_exec_op_by_damr(struct ftspi020_priv *ctrl,
				    const struct spi_mem_op *op, int proto)
{
	u32 cmd;

	if (op->addr.nbytes == 3) {
		cmd = FIELD_PREP(FTSPI020_XIPCMD_ADDR_BYTES, XIPCMD_ADDR_3BYTES);
	} else if (op->addr.nbytes == 4) {
		cmd = FIELD_PREP(FTSPI020_XIPCMD_ADDR_BYTES, XIPCMD_ADDR_4BYTES);
	} else {
		return -EINVAL;
	}

	cmd |= FIELD_PREP(FTSPI020_XIPCMD_INSTR_CODE, op->cmd.opcode);
	cmd |= FIELD_PREP(FTSPI020_XIPCMD_PROTO, proto);
	cmd |= FIELD_PREP(FTSPI020_XIPCMD_DUMMY_CYCLE,
			  op->dummy.nbytes * 8 / op->dummy.buswidth);

	iowrite32(cmd, ctrl->io_base + FTSPI020_REG_XIPCMD);

	memcpy_fromio(op->data.buf.in, ctrl->damr_base + op->addr.val, op->data.nbytes);

	return 0;
}

static int ftspi020_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct ftspi020_priv *ctrl = spi_controller_get_devdata(mem->spi->master);
	struct spi_device *spi = mem->spi;
	int err, proto;
	u32 ce;

	mutex_lock(&ctrl->lock);

	dev_dbg(ctrl->dev, "cmd:%#x mode:%d.%d.%d.%d addr:%#llx.%d len:%#x\n",
		op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->addr.nbytes, op->data.nbytes);

	proto = ftspi020_decode_proto(mem, op);
	if (proto == -EINVAL) {
		err = proto;
		goto error;
	}

	ce = spi->chip_select;

	/* Note:
	 * 1. CPU direct read memory only support chip select 0.
	 * 2. To check number of address bytes to prevent READ ID command from
	 *    using direct read memory.
	 * 3. Because FTSPI020 XIP mode doesn't support SPI NAND flash, make sure
	 *    CE 0 is not SPI NAND flash.
	 */
	if (!ce && (op->data.dir == SPI_MEM_DATA_IN) &&
	    op->addr.nbytes && (op->addr.val + op->data.nbytes < ctrl->damr_size)) {
		err = ftspi020_exec_op_by_damr(ctrl, op, proto);
	} else {
		err = ftspi020_exec_op_by_cmd(ctrl, op, ce, proto);
	}
error:
	mutex_unlock(&ctrl->lock);

	return err;
}

static int ftspi020_check_buswidth(struct ftspi020_priv *ctrl, u8 width)
{
	switch (width) {
	case 1:
	case 2:
	case 4:
		return 0;
	}

	return -ENOTSUPP;
}

static bool ftspi020_supports_op(struct spi_mem *mem,
				 const struct spi_mem_op *op)
{
	struct ftspi020_priv *ctrl = spi_controller_get_devdata(mem->spi->master);
	int ret;

	ret = ftspi020_check_buswidth(ctrl, op->cmd.buswidth);

	if (op->addr.nbytes)
		ret |= ftspi020_check_buswidth(ctrl, op->addr.buswidth);

	if (op->dummy.nbytes)
		ret |= ftspi020_check_buswidth(ctrl, op->dummy.buswidth);

	if (op->data.nbytes)
		ret |= ftspi020_check_buswidth(ctrl, op->data.buswidth);

	if (ret)
		return false;

	/*
	* The number of address bytes should be equal to or less than 4 bytes.
	*/
	if (op->addr.nbytes > 4)
		return false;

	/* Max 255 dummy clock cycles supported */
	if (op->dummy.nbytes &&
	    (op->dummy.nbytes * 8 / op->dummy.buswidth > 255))
		return false;

	return true;
}

static const char *ftspi020_get_name(struct spi_mem *mem)
{
	struct ftspi020_priv *ctrl = spi_controller_get_devdata(mem->spi->master);
	struct device *dev = &mem->spi->dev;
	const char *name;

	if (of_get_available_child_count(ctrl->dev->of_node) == 1)
		return dev_name(ctrl->dev);

	name = devm_kasprintf(dev, GFP_KERNEL,
			      "%s-%d", dev_name(ctrl->dev),
			      mem->spi->chip_select);

	if (!name) {
		dev_err(dev, "failed to get memory for custom flash name\n");
		return ERR_PTR(-ENOMEM);
	}

	return name;
}

static const struct spi_controller_mem_ops ftspi020_mem_ops = {
	.exec_op = ftspi020_exec_op,
	.supports_op = ftspi020_supports_op,
	.get_name = ftspi020_get_name,
};

static int ftspi020_setup(struct spi_device *spi)
{
	struct spi_controller *master = spi->master;
	struct ftspi020_priv *ctrl = spi_controller_get_devdata(master);
	u32 clk_rate, divider, val;

	if (master->busy)
		return -EBUSY;

	if (!spi->max_speed_hz)
		return 0;

	clk_rate = clk_get_rate(ctrl->clk);
	if (!clk_rate) {
		return 0;
        }

	divider = 2;
	while ((clk_rate / divider) > spi->max_speed_hz) {
		//FTSPI020 support max divide by 8
		if (divider >= 8)
			break;

		divider += 2;
	}

	val = ioread32(ctrl->io_base + FTSPI020_REG_CTRL);
#ifndef CONFIG_SPI_FTSPI020_V1_7_0
	val &= ~FTSPI020_CTRL_DAMR_PORT;
#endif
	val &= ~FTSPI020_CTRL_CLK_DIVIDER;

	switch (divider) {
	case 2:
		val |= FIELD_PREP(FTSPI020_CTRL_CLK_DIVIDER, CLK_DIVIDER_2);
		break;
	case 4:
		val |= FIELD_PREP(FTSPI020_CTRL_CLK_DIVIDER, CLK_DIVIDER_4);
		break;
	case 6:
		val |= FIELD_PREP(FTSPI020_CTRL_CLK_DIVIDER, CLK_DIVIDER_6);
		break;
	case 8:
		val |= FIELD_PREP(FTSPI020_CTRL_CLK_DIVIDER, CLK_DIVIDER_8);
		break;
	}
	iowrite32(val, ctrl->io_base + FTSPI020_REG_CTRL);

#ifndef CONFIG_SPI_FTSPI020_V1_7_0
	while ((readl(ctrl->io_base + FTSPI020_REG_CTRL) &
		FTSPI020_CTRL_ABORT)) {;}
#endif

	return 0;
}

static int ftspi020_default_setup(struct ftspi020_priv *ctrl)
{
#ifdef CONFIG_SPI_FTSPI020_USE_DMA
	struct device *dev = ctrl->dev;
	struct dma_slave_config dma_cfg;
#endif
	u32 val;

	val = ioread32(ctrl->io_base + FTSPI020_REG_CTRL);
	val &= ~(FTSPI020_CTRL_READY_LOC | FTSPI020_CTRL_CLK_DIVIDER);
	val |= (FIELD_PREP(FTSPI020_CTRL_READY_LOC, 0) |
		 FIELD_PREP(FTSPI020_CTRL_CLK_DIVIDER, CLK_DIVIDER_4));
	iowrite32(val, ctrl->io_base + FTSPI020_REG_CTRL);

	val = ioread32(ctrl->io_base + FTSPI020_REG_FEATURE);
	ctrl->rxfifo_depth = FIELD_GET(FTSPI020_FEATURE_RXFIFO_DEPTH, val);
	ctrl->txfifo_depth = FIELD_GET(FTSPI020_FEATURE_TXFIFO_DEPTH, val);

	val = ioread32(ctrl->io_base + FTSPI020_REG_CR2);
	val &= ~(FTSPI020_CR2_CMD_CMPL | FTSPI020_CR2_DMA_EN);
	val |= FIELD_PREP(FTSPI020_CR2_CMD_CMPL, 1);
	val |= FIELD_PREP(FTSPI020_CR2_RX_XFR_SIZE, XFR_SIZE_256BYTES);
	val |= FIELD_PREP(FTSPI020_CR2_TX_XFR_SIZE, XFR_SIZE_256BYTES);
	val |= FIELD_PREP(FTSPI020_CR2_DMA_RX_THOD, DMA_THOD_16BYTES);
	iowrite32(val, ctrl->io_base + FTSPI020_REG_CR2);

#ifdef CONFIG_SPI_FTSPI020_V1_7_0
	/*
	 * Use this for read/write RX/TX FIFO when in PIO mode. Can't use FIFO
	 * depth or user may encounter FIFO not ready problem.
	 */
	ctrl->rx_transfer_size = (32 << FIELD_GET(FTSPI020_CR2_RX_XFR_SIZE, val));
	ctrl->tx_transfer_size = (32 << FIELD_GET(FTSPI020_CR2_TX_XFR_SIZE, val));
#else
	ctrl->rx_transfer_size = ctrl->rxfifo_depth;
	ctrl->tx_transfer_size = ctrl->txfifo_depth;
#endif

#ifdef CONFIG_SPI_FTSPI020_USE_DMA
	memset(&dma_cfg, 0, sizeof(dma_cfg));
	dma_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_cfg.src_addr = (dma_addr_t)ctrl->phys_io_base + FTSPI020_REG_DATA_PORT;
	dma_cfg.dst_addr = (dma_addr_t)ctrl->phys_io_base + FTSPI020_REG_DATA_PORT;
	dma_cfg.src_maxburst = 4;
	dma_cfg.dst_maxburst = 4;

	ctrl->dma_ch = dma_request_slave_channel(dev, "tx-rx");
	if (ctrl->dma_ch) {
		init_completion(&ctrl->dma_completion);
	}
#endif
	dev_info(ctrl->dev, "CR=%08x, CR2=%08x, dma_ch=%px\n",
		ioread32(ctrl->io_base + FTSPI020_REG_CTRL),
		ioread32(ctrl->io_base + FTSPI020_REG_CR2),
		ctrl->dma_ch);
	return 0;
}

static void ftspi020_dma_free(struct ftspi020_priv *ctrl)
{
	if (ctrl->dma_ch)
		dma_release_channel(ctrl->dma_ch);
}

static int ftspi020_probe(struct platform_device *pdev)
{
	struct spi_controller *master;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct ftspi020_priv *ctrl;
	int ret;

	master = spi_alloc_master(&pdev->dev, sizeof(*ctrl));
	if (!master)
		return -ENOMEM;

	ctrl = spi_controller_get_devdata(master);
	ctrl->dev = dev;
	platform_set_drvdata(pdev, ctrl);
	ctrl->master = master;

	/* find the resources */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctrl-port");
	ctrl->io_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ctrl->io_base)) {
		ret = PTR_ERR(ctrl->io_base);
		goto err_put_ctrl;
	}

	ctrl->phys_io_base = res->start;

	if (ioread32(ctrl->io_base + FTSPI020_REG_FEATURE) & FTSPI020_FEATURE_DAMR) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "damr-port");
		ctrl->damr_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(ctrl->damr_base)) {
			ctrl->damr_base = 0;
			dev_info(dev, "alloc direct access memory resource failed\n");
		} else {
			ctrl->damr_size = resource_size(res);
			if (ctrl->damr_size > FTSPI020_DAMR_MAX_SIZE) {
				ctrl->damr_size = 0;
			}
			dev_info(dev, "damr base %px, size %x\n",
				 ctrl->damr_base, ctrl->damr_size);
		}
	}

	ctrl->clk = devm_clk_get(dev, "spiclk");
	if (IS_ERR(ctrl->clk)) {
		ret = PTR_ERR(ctrl->clk);
		goto err_put_ctrl;
	}

	ret = clk_prepare_enable(ctrl->clk);
	if (ret) {
		dev_err(dev, "can not enable the clock\n");
		goto err_put_ctrl;
	}
#ifdef CONFIG_SPI_FTSPI020_USE_INT
	/* find the irq */
	ret = platform_get_irq(pdev, 0);
	if (ret < 0)
		goto err_disable_clk;

	ret = devm_request_irq(dev, ret, ftspi020_isr, 0, pdev->name, ctrl);
	if (ret) {
		dev_err(dev, "failed to request irq: %d\n", ret);
		goto err_disable_clk;
	}
#endif
	mutex_init(&ctrl->lock);
	ftspi020_default_setup(ctrl);

	dev_info(dev, "reg base %px, tx fifo %d rx fifo %d,"
		      "tx xfr sz %d, rx xfr sz %d\n",
		ctrl->io_base, ctrl->txfifo_depth, ctrl->rxfifo_depth,
		ctrl->tx_transfer_size, ctrl->rx_transfer_size);

	master->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD |
			  SPI_TX_DUAL | SPI_TX_QUAD;
	master->setup = ftspi020_setup;
	master->bus_num = -1;
	master->num_chipselect = 4;
	master->mem_ops = &ftspi020_mem_ops;
	master->dev.of_node = dev->of_node;

	ret = devm_spi_register_controller(dev, master);
	if (ret)
		goto err_destroy_mutex;

	return 0;

err_destroy_mutex:
	mutex_destroy(&ctrl->lock);

err_disable_clk:
	clk_disable_unprepare(ctrl->clk);

err_put_ctrl:
	ftspi020_dma_free(ctrl);
	spi_controller_put(master);

	dev_err(dev, "FTSPI020 probe failed\n");
	return ret;
}

static int ftspi020_remove(struct platform_device *pdev)
{
	struct ftspi020_priv *ctrl = platform_get_drvdata(pdev);

	clk_disable_unprepare(ctrl->clk);

	mutex_destroy(&ctrl->lock);

	ftspi020_dma_free(ctrl);

	return 0;
}

static int ftspi020_suspend(struct device *dev)
{
	return 0;
}

static int ftspi020_resume(struct device *dev)
{
	return 0;
}

static const struct of_device_id ftspi020_dt_ids[] = {
	{ .compatible = "faraday,ftspi020"},
	{ }
};
MODULE_DEVICE_TABLE(of, ftspi020_dt_ids);

SIMPLE_DEV_PM_OPS(ftspi020_pm_ops, ftspi020_suspend, ftspi020_resume);

/* pm_ops to be called by SoC specific platform driver */
EXPORT_SYMBOL_GPL(ftspi020_pm_ops);

static struct platform_driver ftspi020_driver = {
	.driver = {
		.name	= "ftspi020",
		.of_match_table = ftspi020_dt_ids,
		.pm =   &ftspi020_pm_ops,
	},
	.probe          = ftspi020_probe,
	.remove		= ftspi020_remove,
};
module_platform_driver(ftspi020_driver);

MODULE_DESCRIPTION("FTSPI020 SPI memory Controller Driver");
MODULE_AUTHOR("Faraday Tech Corp.");
MODULE_LICENSE("GPL v2");
