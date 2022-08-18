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

#ifndef __FTIIC010_H
#define __FTIIC010_H


struct ftiic010 {
	struct resource *res;
	void __iomem *base;
	int irq;
	struct clk *clk;
	struct i2c_adapter adapter;
	int hs, burst;
};

#define MAX_RETRY       2000

#define SCL_SPEED       (100 * 1000)

#define GSR     2
#define TSR     1

#define FTIIC010_OFFSET_CR	0x00
#define FTIIC010_OFFSET_SR	0x04
#define FTIIC010_OFFSET_CDR	0x08
#define FTIIC010_OFFSET_DR	0x0c
#define FTIIC010_OFFSET_AR	0x10
#define FTIIC010_OFFSET_TGSR	0x14
#define FTIIC010_OFFSET_BSTMR	0x1c
/*
#define FTIIC010_OFFSET_SMCR	0x1c
#define FTIIC010_OFFSET_MAXTR	0x20
#define FTIIC010_OFFSET_MINTR	0x24
#define FTIIC010_OFFSET_METR	0x28
#define FTIIC010_OFFSET_SETR	0x2c
#define FTIIC010_OFFSET_FEAT	0x34
*/
#define FTIIC010_OFFSET_REV	0x30

/*
 * Control Register
 */
#define FTIIC010_CR_I2C_RST	(1 << 0)
#define FTIIC010_CR_I2C_EN	(1 << 1)
#define FTIIC010_CR_MST_EN	(1 << 2)
#define FTIIC010_CR_GC_EN	(1 << 3)
#define FTIIC010_CR_START	(1 << 4)
#define FTIIC010_CR_STOP	(1 << 5)
#define FTIIC010_CR_NACK	(1 << 6)
#define FTIIC010_CR_TB_EN	(1 << 7)
#define FTIIC010_CR_BSTTHODI_EN	(1 << 8)
#define FTIIC010_CR_TDI_EN	(1 << 9)
#define FTIIC010_CR_NACKI_EN	(1 << 10)
#define FTIIC010_CR_STOPI_EN	(1 << 11)
#define FTIIC010_CR_SAMI_EN	(1 << 12)
#define FTIIC010_CR_ALI_EN	(1 << 13)
#define FTIIC010_CR_STARTI_EN	(1 << 14)
#define FTIIC010_CR_SCL_LOW	(1 << 15)
#define FTIIC010_CR_SDA_LOW	(1 << 16)
#define FTIIC010_CR_TESTMODE	(1 << 17)
#define FTIIC010_CR_ARBOFF	(1 << 18)
#define FTIIC010_CR_HSMODE	(1 << 19)
#define FTIIC010_CR_HSI_EN	(1 << 20)
#define FTIIC010_CR_SBI_EN	(1 << 21)
#define FTIIC010_CR_FLOWCTRL_W	(1 << 24)
#define FTIIC010_CR_FLOWCTRL_R	(2 << 24)
/*
 * Status Register
 */
#define FTIIC010_SR_RW		(1 << 0)
#define FTIIC010_SR_I2CBUSY	(1 << 2)
#define FTIIC010_SR_BUSBUSY	(1 << 3)
#define FTIIC010_SR_BSTTHOD	(1 << 4)
#define FTIIC010_SR_TD		(1 << 5)
#define FTIIC010_SR_NACK	(1 << 6)
#define FTIIC010_SR_STOP	(1 << 7)
#define FTIIC010_SR_SAM		(1 << 8)
#define FTIIC010_SR_GC		(1 << 9)
#define FTIIC010_SR_AL		(1 << 10)
#define FTIIC010_SR_START	(1 << 11)
#define FTIIC010_SR_HSS		(1 << 22)
#define FTIIC010_SR_SBS		(1 << 23)

/*
 * Clock Division Register(CDR)
 */
#define FTIIC010_CDR_COUNT_MASK		0xfffff
#define FTIIC010_CDR_COUNTH_MASK	0xff00000
#define FTIIC010_CDR_DUTY_MASK		0xf0000000
#define FTIIC010_CDR_COUNT_OFF		0
#define FTIIC010_CDR_COUNTH_OFF		20
#define FTIIC010_CDR_DUTY_OFF		28

/*
 * Data Register
 */
#define FTIIC010_DR_MASK	0xff

/*
 * Address Register
 */
#define FTIIC010_ADDR_MASK	0x7f
#define FTIIC010_ADDR2_MASK	0x1c0
#define FTIIC010_ADDR10EN	(1 << 12)
#define FTIIC010_M2BIDXEN	(1 << 13)
#define FTIIC010_MEM_IDX_MASK	0xff0000
#define FTIIC010_MEM_IDX_OFF	16
#define FTIIC010_MEM_IDX2_MASK	0xff000000
#define FTIIC010_MEM_IDX2_OFF	24

/*
 * Set/Hold Time Glitch Suppression Setting Register
 */
#define FTIIC010_TGSR_TSR(x)	((x) & 0x3ff)	/* should not be zero */
#define FTIIC010_TGSR_GSR(x)	(((x) & 0x7) << 10)

/*
 * Bus Monitor Register
 */
#define FTIIC010_BMR_SDA_IN	(1 << 0)
#define FTIIC010_BMR_SCL_IN	(1 << 1)

/*
 * Burst Mode Register
 */
#define FTIIC010_BSTTHOD_OFF	0
#define FTIIC010_BSTTDC_OFF	8
#define FTIIC010_BUFHW_OFF	16


/*
 * SM Control Register
 */
#define FTIIC010_SMCR_SEXT_EN	(1 << 0)
#define FTIIC010_SMCR_MEXT_EN	(1 << 1)
#define FTIIC010_SMCR_TOUT_EN	(1 << 2)
#define FTIIC010_SMCR_ALERT_EN	(1 << 3)
#define FTIIC010_SMCR_SUS_EN	(1 << 4)
#define FTIIC010_SMCR_RSM_EN	(1 << 5)
#define FTIIC010_SMCR_SAL_EN	(1 << 8)
#define FTIIC010_SMCR_ALERT_LOW	(1 << 9)
#define FTIIC010_SMCR_SUS_LOW	(1 << 10)
#define FTIIC010_SMCR_SUSOUT_EN	(1 << 11)

/*
 * SM Maximum Timeout Register
 */
#define FTIIC010_MAXTR_MASK	0x3fffff

/*
 * SM Minimum Timeout Register
 */
#define FTIIC010_MINTR_MASK	0x3fffff

/*
 * SM Master Extend Time Register
 */
#define FTIIC010_METR_MASK	0xfffff

/*
 * SM Slave Extend Time Register
 */
#define FTIIC010_SETR_MASK	0x1fffff

/*
 * Feature Register
 */
#define FTIIC010_FEAT_SMBUS	(1 << 0)

#endif /* __FTIIC010_H */
