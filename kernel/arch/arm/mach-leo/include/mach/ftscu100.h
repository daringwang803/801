/*
 *  arch/arm/mach-leo/include/mach/ftscu100.h
 *
 *  Faraday FTSCU100 System Control Unit
 *
 *  Copyright (C) 2014 Faraday Technology
 *  Copyright (C) 2020 Bo-Cun Chen <bcchen@faraday-tech.com>
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

#define FTSCU100_OFFSET_BOOTSR          0x000
#define FTSCU100_OFFSET_BOOTCR          0x004
#define FTSCU100_OFFSET_PWRCR           0x008
#define FTSCU100_OFFSET_CHIPID          0x010
#define FTSCU100_OFFSET_VER             0x014
#define FTSCU100_OFFSET_STRAP           0x018
#define FTSCU100_OFFSET_OSC             0x01c
#define FTSCU100_OFFSET_PWR_MODE        0x020
#define FTSCU100_OFFSET_INTSR           0x024
#define FTSCU100_OFFSET_INTEN           0x028
#define FTSCU100_OFFSET_SW_RST          0x02C
#define FTSCU100_OFFSET_PLL0CR          0x030
#define FTSCU100_OFFSET_PLL4CR          0x040
#define FTSCU100_OFFSET_DDR_DLL         0x044
#define FTSCU100_OFFSET_HCLK            0x050
#define FTSCU100_OFFSET_SLP_HCLK        0x058
#define FTSCU100_OFFSET_PCLK            0x060
#define FTSCU100_OFFSET_EXTPCLK         0x064
#define FTSCU100_OFFSET_SLP_PCLK        0x068
#define FTSCU100_OFFSET_SLP_EXTPCLK     0x06c
#define FTSCU100_OFFSET_ACLK            0x080
#define FTSCU100_OFFSET_SLP_ACLK        0x088
#define FTSCU100_OFFSET_WFI             0x0b4
#define FTSCU100_OFFSET_SLP_WAKE_STS    0x0c0
#define FTSCU100_OFFSET_SLP_WAKE_EN     0x0c4
#define FTSCU100_OFFSET_RTC_TIME1       0x200
#define FTSCU100_OFFSET_RTC_TIME2       0x204
#define FTSCU100_OFFSET_RTC_ALARM1      0x208
#define FTSCU100_OFFSET_RTC_ALARM2      0x20c
#define FTSCU100_OFFSET_RTC             0x210
#define FTSCU100_OFFSET_RTC_TRIM        0x214
#define FTSCU100_OFFSET_RTC_TIME1_BCD   0x220
#define FTSCU100_OFFSET_RTC_TIME2_BCD   0x224
#define FTSCU100_OFFSET_RTC_ALARM1_BCD  0x228
#define FTSCU100_OFFSET_RTC_ALARM2_BCD  0x22c

#define FTSCU100_OFFSET_PLL1CR          0x8040
#define FTSCU100_OFFSET_PLL2CR          0x8048
#define FTSCU100_OFFSET_PLL3CR          0x8050
#define FTSCU100_OFFSET_PLL5CR          0x8060
#define FTSCU100_OFFSET_CLKMUX          0x807c
#define FTSCU100_OFFSET_CLKDIV1         0x8080
#define FTSCU100_OFFSET_CLKDIV2         0x8084
#define FTSCU100_OFFSET_CLKDIV3         0x8088
#define FTSCU100_OFFSET_CLKDIV4         0x808c
#define FTSCU100_OFFSET_PLL6CR          0x80c8
#define FTSCU100_OFFSET_PLL7CR          0x8114

/* Strap Value Registers */
#define FTSCU100_STRAP_BOOT_CK_SEL(cr)  (((cr) >> 13) & 0x003)

/* PLL0 Control Registers */
#define FTSCU100_PLL0CR_NS(cr)          (((cr) >> 24) & 0x0ff)
#define FTSCU100_PLL0CR_MS(cr)          (((cr) >> 17) & 0x07f)
#define FTSCU100_PLL0CR_CLKIN_MUX(cr)   (((cr) >>  4) & 0x00f)

/* PLL1 Control Registers */
#define FTSCU100_PLL1CR_NS(cr)          (((cr) >> 13) & 0x07f)
#define FTSCU100_PLL1CR_MS(cr)          (((cr) >>  8) & 0x01f)

/* PLL2 Control Registers */
#define FTSCU100_PLL2CR_NS(cr)          (((cr) >> 13) & 0x07f)
#define FTSCU100_PLL2CR_MS(cr)          (((cr) >>  8) & 0x01f)

/* PLL3 Control Registers */
#define FTSCU100_PLL3CR_PS(cr)          (((cr) >> 24) & 0x01f)
#define FTSCU100_PLL3CR_NS(cr)          (((cr) >> 12) & 0x1ff)
#define FTSCU100_PLL3CR_MS(cr)          (((cr) >>  8) & 0x007)

/* PLL4 Control Registers */
#define FTSCU100_PLL4CR_NS(cr)          (((cr) >> 24) & 0x0ff)

/* PLL5 Control Registers */
#define FTSCU100_PLL5CR_PS(cr)          (((cr) >> 24) & 0x01f)
#define FTSCU100_PLL5CR_NS(cr)          (((cr) >> 12) & 0x1ff)
#define FTSCU100_PLL5CR_MS(cr)          (((cr) >>  8) & 0x007)

/* PLL6 Control Registers */
#define FTSCU100_PLL6CR_NS(cr)          (((cr) >> 13) & 0x07f)
#define FTSCU100_PLL6CR_MS(cr)          (((cr) >>  8) & 0x01f)

/* PLL7 Control Registers */
#define FTSCU100_PLL7CR_NS(cr)          (((cr) >> 13) & 0x07f)
#define FTSCU100_PLL7CR_MS(cr)          (((cr) >>  8) & 0x01f)

/* Clock Mux Selection Registers */
#define FTSCU100_CLKMUX_PERI3_OSC_SW(cr) (((cr) >> 31) & 0x001)
#define FTSCU100_CLKMUX_PERI2_OSC_SW(cr) (((cr) >> 30) & 0x001)
#define FTSCU100_CLKMUX_PERI1_OSC_SW(cr) (((cr) >> 29) & 0x001)
#define FTSCU100_CLKMUX_UART8_FIR_SEL(cr)(((cr) >> 28) & 0x001)
#define FTSCU100_CLKMUX_UART7_FIR_SEL(cr)(((cr) >> 27) & 0x001)
#define FTSCU100_CLKMUX_UART6_FIR_SEL(cr)(((cr) >> 26) & 0x001)
#define FTSCU100_CLKMUX_UART5_FIR_SEL(cr)(((cr) >> 25) & 0x001)
#define FTSCU100_CLKMUX_UART4_FIR_SEL(cr)(((cr) >> 24) & 0x001)
#define FTSCU100_CLKMUX_UART3_FIR_SEL(cr)(((cr) >> 23) & 0x001)
#define FTSCU100_CLKMUX_UART2_FIR_SEL(cr)(((cr) >> 22) & 0x001)
#define FTSCU100_CLKMUX_UART1_FIR_SEL(cr)(((cr) >> 21) & 0x001)
#define FTSCU100_CLKMUX_UART_FIR_SEL(cr) (((cr) >> 20) & 0x001)
#define FTSCU100_CLKMUX_EMMC_OSC_SW(cr)  (((cr) >> 13) & 0x001)
#define FTSCU100_CLKMUX_PLL3_REF_SEL(cr) (((cr) >> 12) & 0x001)
#define FTSCU100_CLKMUX_SSP1_I2S_SEL(cr) (((cr) >> 11) & 0x001)
#define FTSCU100_CLKMUX_SSP_I2S_SEL(cr)  (((cr) >> 10) & 0x001)
#define FTSCU100_CLKMUX_PLL6_OSC_SW1(cr) (((cr) >>  9) & 0x001)
#define FTSCU100_CLKMUX_PLL6_OSC_SW2(cr) (((cr) >>  8) & 0x001)
#define FTSCU100_CLKMUX_TIMER_CLK_SW(cr) (((cr) >>  7) & 0x001)
#define FTSCU100_CLKMUX_UART_30M_DIS(cr) (((cr) >>  6) & 0x001)

/* Clock Divider 4 Registers */
#define FTSCU100_CLKDIV4_FCNT_N(cr)     (((cr) >> 26) & 0x00f)
#define FTSCU100_CLKDIV4_FCNT_M(cr)     (((cr) >> 22) & 0x00f)

void ftscu100_init(void __iomem *base);
unsigned int ftscu100_readl(unsigned int offset);
void ftscu100_writel(unsigned int val, unsigned int offset);

#endif /* __FTSCU100_H */
