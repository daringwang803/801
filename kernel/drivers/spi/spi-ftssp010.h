/*
 * Faraday Multi-function Controller - SPI Mode
 *
 * (C) Copyright 2010 Faraday Technology
 * Dante Su <dantesu@faraday-tech.com>
 *
 * This file is released under the terms of GPL v2 and any later version.
 * See the file COPYING in the root directory of the source tree for details.
 */

#ifndef _FTSSP010_H
#define _FTSSP010_H

/* FTSSP010 HW Registers */
struct ftssp010_regs {
	uint32_t cr[3];/* control register */
	uint32_t sr;   /* status register */
	uint32_t icr;  /* interrupt control register */
	uint32_t isr;  /* interrupt status register */
	uint32_t dr;   /* data register */
	uint32_t rsvd[17];
	uint32_t revr; /* revision register */
	uint32_t fear; /* feature register */
};

/* Control Register 0  */
#define CR0_LCDDCX          (1 << 21) /* LCD D/CX value */
#define CR0_LCDDCXS         (1 << 20) /* LCD D/CX select */
#define CR0_SPICONTX        (1 << 19) /* SPI continuous transfer control */
#define CR0_FLASHTX         (1 << 18) /* Flash transmit control */
#define CR0_FSFDBK          (1 << 17) /* fs_in is the internal feedback from fs_out_r */
#define CR0_SCLKFDBK        (1 << 16) /* sclk_in is the internal feedback from sclk_out_r*/
#define CR0_SPIFSPO         (1 << 15) /* FS polarity for the SPI mode */
#define CR0_FFMT_MASK       (7 << 12)
#define CR0_FFMT_SSP        (0 << 12)
#define CR0_FFMT_SPI        (1 << 12)
#define CR0_FFMT_MICROWIRE  (2 << 12)
#define CR0_FFMT_I2S        (3 << 12)
#define CR0_FFMT_AC97       (4 << 12)
#define CR0_FLASH           (1 << 11)
#define CR0_FSDIST(x)       (((x) & 0x03) << 8)
#define CR0_LBM             (1 << 7)  /* Loopback mode */
#define CR0_LSB             (1 << 6)  /* LSB first */
#define CR0_FSPO            (1 << 5)  /* Frame sync atcive low */
#define CR0_FSJUSTIFY       (1 << 4)
#define CR0_OPM_SLAVE       (0 << 2)
#define CR0_OPM_MASTER      (3 << 2)
#define CR0_OPM_I2S_MSST    (3 << 2)  /* Master stereo mode */
#define CR0_OPM_I2S_MSMO    (2 << 2)  /* Master mono mode */
#define CR0_OPM_I2S_SLST    (1 << 2)  /* Slave stereo mode */
#define CR0_OPM_I2S_SLMO    (0 << 2)  /* Slave mono mode */
#define CR0_SCLKPO          (1 << 1)  /* SCLK Remain HIGH */
#define CR0_SCLKPH          (1 << 0)  /* Half CLK cycle */

/* Control Register 1 */

#define CR1_PDL(x)          (((x) & 0xff) << 24) /* padding length */
#define CR1_SDL(x)          ((((x) - 1) & 0x7f) << 16) /* data length */
#define CR1_CLKDIV(x)       ((x) & 0xffff) /*  clk divider */

/* Control Register 2 */
#define CR2_FSOS(x)         (((x) & 0x03) << 10)	/* FS/CS Select */
#define CR2_FS              (1 << 9)	/* FS/CS Signal Level */
#define CR2_TXEN            (1 << 8)	/* Tx Enable */
#define CR2_RXEN            (1 << 7)	/* Rx Enable */
#define CR2_SSPRST          (1 << 6)	/* SSP reset */
#define CR2_TXFCLR          (1 << 3)	/* TX FIFO Clear */
#define CR2_RXFCLR          (1 << 2)	/* RX FIFO Clear */
#define CR2_TXDOE           (1 << 1)	/* TX Data Output Enable */
#define CR2_SSPEN           (1 << 0)	/* SSP Enable */

/*
 * Status Register
 */
#define SR_RFF       (1 << 0) /* receive FIFO full */
#define SR_TFNF      (1 << 1) /* transmit FIFO not full */
#define SR_BUSY      (1 << 2) /* bus is busy */
#define SR_RFVE(reg) (((reg) >> 4) & 0x3f)  /* receive  FIFO valid entries */
#define SR_TFVE(reg) (((reg) >> 12) & 0x3f) /* transmit FIFO valid entries */

/*
 * Interrupt Control Register
 */
#define ICR_TXCIEN       (1 << 18) /* TX Data Complete Interrupt Enable */
#define ICR_RFTHOD_UNIT  (1 << 17) /* RX FIFO Threshold Unit */
#define ICR_TFTHOD(reg)  (((reg) & 0x1f) << 12) /* TX FIFO Threshold */
#define ICR_RFTHOD(reg)  (((reg) & 0x1f) << 7)  /* RX FIFO Threshold */
#define ICR_TFDMAEN      (1 << 5)  /* TX DMA Request Enable */
#define ICR_RFDMAEN      (1 << 4)  /* RX DMA Request Enable */
#define ICR_TFTHIEN      (1 << 3)  /* TX FIFO Threshold Interrupt Enable */
#define ICR_RFTHIEN      (1 << 2)  /* RX FIFO Threshold Interrupt Enable */
#define ICR_TFURIEN      (1 << 1)  /* TX FIFO Underrun Interrupt Enable */
#define ICR_RFORIEN      (1 << 1)  /* RX FIFO Overrun Interrupt Enable */

/*
 * Interrupt Status Register
 */
#define ISR_TXCI         (1 << 5)  /* TX Data Complete Interrupt */
#define ISR_TFTHI        (1 << 3)  /* TX FIFO Threshold Interrupt */
#define ISR_RFTHI        (1 << 2)  /* RX FIFO Threshold Interrupt */
#define ISR_TFURI        (1 << 1)  /* TX FIFO Underrun Interrupt */
#define ISR_RFORI        (1 << 0)  /* RX FIFO Overrun Interrupt */

/*
 * Feature Register
 */
#define FEAR_WIDTH(reg)  ((((reg) >>  0) & 0xff) + 1)
#define FEAR_RXFIFO(reg) ((((reg) >>  8) & 0xff) + 1)
#define FEAR_TXFIFO(reg) ((((reg) >> 16) & 0xff) + 1)
#define FEAR_AC97        (1 << 24)
#define FEAR_I2S         (1 << 25)
#define FEAR_SPI_MWR     (1 << 26)
#define FEAR_SSP         (1 << 27)
#define FEAR_SPDIF       (1 << 28)

#endif /* EOF */
