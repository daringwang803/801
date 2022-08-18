/*
 *  arch/arm/mach-faraday/include/mach/ftscu100.h
 *
 *  Faraday FTSCU100 System Control Unit
 *
 *  Copyright (C) 2014 Faraday Technology
 *  Copyright (C) 2014 Yan-Pai Chen <ypchen@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __FTSCU100_H
#define __FTSCU100_H

#define FTSCU100_OFFSET_BOOTSR		0x000
#define FTSCU100_OFFSET_BOOTCR		0x004
#define FTSCU100_OFFSET_PWRCR		0x008
#define FTSCU100_OFFSET_PWRUP_SEQ	0x00c
#define FTSCU100_OFFSET_CHIPID		0x010
#define FTSCU100_OFFSET_VER		0x014
#define FTSCU100_OFFSET_STRAP0		0x018
#define FTSCU100_OFFSET_OSCC		0x01c
#define FTSCU100_OFFSET_PWR_MODE	0x020
#define FTSCU100_OFFSET_INTSR		0x024
#define FTSCU100_OFFSET_INTEN		0x028
#define FTSCU100_OFFSET_PLLCR1		0x030
#define FTSCU100_OFFSET_TCLCR1		0x034
#define FTSCU100_OFFSET_TCLCR2		0x038
#define FTSCU100_OFFSET_TCLCR3		0x03c
#define FTSCU100_OFFSET_PLLCR2		0x040
#define FTSCU100_OFFSET_DLL		0x044
#define FTSCU100_OFFSET_STRAP1		0x048
#define FTSCU100_OFFSET_HCLK		0x050
#define FTSCU100_OFFSET_EXTHCLK		0x054
#define FTSCU100_OFFSET_PCLK		0x060
#define FTSCU100_OFFSET_EXTPCLK		0x064
#define FTSCU100_OFFSET_SLP_PCLK	0x068
#define FTSCU100_OFFSET_SLP_EXTPCLK	0x06c
#define FTSCU100_OFFSET_ACLK		0x080
#define FTSCU100_OFFSET_EXTACLK		0x084
#define FTSCU100_OFFSET_SLP_ACLK	0x088
#define FTSCU100_OFFSET_SLP_EXTACLK	0x08c
#define FTSCU100_OFFSET_AXI_MASK	0x090
#define FTSCU100_OFFSET_AXI_ACCEPT	0x094
#define FTSCU100_OFFSET_AXI_DENIAL	0x098
#define FTSCU100_OFFSET_AXI_SYSACK	0x09c
#define FTSCU100_OFFSET_AXI_TIMEOUT	0x0a0
#define FTSCU100_OFFSET_STRAP2		0x0b0

/* SPR: 0x100 ~ 0x1fc */
#define FTSCU100_OFFSET_SPR(x)		(0x100 + (x) * 4)

#define FTSCU100_OFFSET_RTC_TIME1	0x200
#define FTSCU100_OFFSET_RTC_TIME2	0x204
#define FTSCU100_OFFSET_RTC_ALARM1	0x208
#define FTSCU100_OFFSET_RTC_ALARM2	0x20c
#define FTSCU100_OFFSET_RTC		0x210
#define FTSCU100_OFFSET_RTC_TRIM	0x214

/* SERDES200 Registers for GMAC */
#define FTSCU100_OFFSET_RX1		0x600
#define FTSCU100_OFFSET_RX2		0x604
#define FTSCU100_OFFSET_RX3		0x608
#define FTSCU100_OFFSET_TX1		0x614
#define FTSCU100_OFFSET_TX2		0x618
#define FTSCU100_OFFSET_REQ1		0x61c
#define FTSCU100_OFFSET_REQ2		0x620
#define FTSCU100_OFFSET_EN		0x62c
#define FTSCU100_OFFSET_PSTAT_RXEQ	0x630
#define FTSCU100_OFFSET_MULTI_RXEQ1	0x634
#define FTSCU100_OFFSET_MULTI_RXEQ2	0x638
#define FTSCU100_OFFSET_MULTI_RXEQ3	0x63c
#define FTSCU100_OFFSET_MULTI_RXEQ4	0x640
#define FTSCU100_OFFSET_MULTI_RX	0x654
#define FTSCU100_OFFSET_MULTI_RXRATE	0x658
#define FTSCU100_OFFSET_MULTI_TX1	0x65c
#define FTSCU100_OFFSET_MULTI_TXDEEMPH1	0x660
#define FTSCU100_OFFSET_MULTI_TXDEEMPH2	0x664
#define FTSCU100_OFFSET_MULTI_TX2	0x670
#define FTSCU100_OFFSET_GMAC_PCS_PMA_SB	0x674

#define FTSCU100_OFFSET_SCR		0x1280

/* SW Reset Registers */
#define FTSCU100_OFFSET_IP_SW_RST1	0x1300
#define FTSCU100_OFFSET_IP_SW_RST2	0x1304
#define FTSCU100_OFFSET_IP_SW_RST3	0x1308
#define FTSCU100_OFFSET_IP_SW_RST4	0x130c
#define FTSCU100_OFFSET_IP_SW_RST5	0x1310
#define FTSCU100_OFFSET_IP_SW_RST6	0x1314
#define FTSCU100_OFFSET_IP_SW_RST7	0x1318
#define FTSCU100_OFFSET_IP_SW_RST8	0x131c
#define FTSCU100_OFFSET_IP_SW_RST9	0x1320
#define FTSCU100_OFFSET_IP_SW_RST10	0x1324
#define FTSCU100_OFFSET_IP_SW_RST11	0x1328
#define FTSCU100_OFFSET_IP_SW_RST12	0x1330

/* Strap Value Register 2 */
#define FTSCU100_STRAP2_PLL0_NS(cr)	(((cr) >> 3) & 0x3f)
#define FTSCU100_STRAP2_PLL1_NS(cr)	(((cr) >> 9) & 0x7)
#define FTSCU100_STRAP2_PLL2_NS(cr)	(((cr) >> 12) & 0x3f)
#define FTSCU100_STRAP2_PLL4_NS(cr)	(((cr) >> 18) & 0x3f)
#define FTSCU100_STRAP2_CPU_STRAP(cr)	(((cr) >> 25) & 0x3)

/* CPU Strap */
enum {
	CPU_STRAP_CA9,
	CPU_STRAP_CA9_FA626TE,
	CPU_STRAP_FA626TE,
	CPU_STRAP_EXTCPU,
};

/* System Configuration Register */
#define FTSCU100_SCR_EXTIRQN_P2SB	(0 << 0)
#define FTSCU100_SCR_EXTIRQN_INTC	(2 << 0)
#define FTSCU100_SCR_EXTIRQN_MASK	(0x3 << 0)

void ftscu100_init(void __iomem *base);
unsigned int ftscu100_readl(unsigned int offset);
void ftscu100_writel(unsigned int val, unsigned int offset);

#endif /* __FTSCU100_H */
