/*
 * arch/arm/mach-a380/include/mach/irqs.h
 *
 *  Copyright (C) 2016 Faraday Technology
 *  B.C. Chen <bcchen@faraday-tech.com>
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

#ifndef __MACH_IRQS_H
#define __MACH_IRQS_H

#define IRQ_GLOBALTIMER             27
#define IRQ_LOCALTIMER              29
#define IRQ_LOCALWDOG               30
#define IRQ_LEGACY                  31

#define IRQ_INTC_START              0
#define IRQ_GIC_START               32

#define NR_IRQS_GIC                 IRQ_GIC_START + 128
#define NR_IRQS_INTC                IRQ_INTC_START + 128

#ifdef CONFIG_CPU_CA9
#ifdef CONFIG_FARADAY_CA9_TESTCHIP
#define IRQ_A380_START              NR_IRQS_GIC + 32	/* this is determined by irqs supported by GIC */
#define NR_IRQS                     (NR_IRQS_GIC + NR_IRQS_INTC)
#else
#define IRQ_A380_START              IRQ_GIC_START	/* this is determined by irqs supported by GIC */
#define NR_IRQS                     (160)
#endif
#else
#define IRQ_A380_START              IRQ_INTC_START
#define NR_IRQS                     (160)
#endif

/*
 * Interrupt numbers of Hierarchical Architecture
 */
#define PLAT_FTDDRII030_IRQ         (IRQ_A380_START + 8)
#define PLAT_SYSC_IRQ               (IRQ_A380_START + 9)

#define PLAT_FTUART010_IRQ          (IRQ_A380_START + 10)
#define PLAT_FTUART010_1_IRQ        (IRQ_A380_START + 11)
#define PLAT_FTUART010_2_IRQ        (IRQ_A380_START + 12)
#define PLAT_FTTMR010_T0_IRQ        (IRQ_A380_START + 13)
#define PLAT_FTTMR010_T1_IRQ        (IRQ_A380_START + 14)
#define PLAT_FTTMR010_T2_IRQ        (IRQ_A380_START + 15)
#define PLAT_FTGPIO010_IRQ          (IRQ_A380_START + 16)
#define PLAT_FTIIC010_IRQ           (IRQ_A380_START + 17)
#define PLAT_FTIIC010_1_IRQ         (IRQ_A380_START + 18)
#define PLAT_FTSSP010_IRQ           (IRQ_A380_START + 19)

#define PLAT_FTSSP010_1_IRQ         (IRQ_A380_START + 20)
#define PLAT_FTSSP010_2_IRQ         (IRQ_A380_START + 21)
#define PLAT_FTSPI020_IRQ           (IRQ_A380_START + 22)
#define PLAT_FTNANDC024_IRQ         (IRQ_A380_START + 23)
#define PLAT_FTWDT010_IRQ           (IRQ_A380_START + 24)
#define PLAT_FTAES020_IRQ           (IRQ_A380_START + 25)
#define PLAT_FTSDC021_IRQ           (IRQ_A380_START + 26)
#define PLAT_FTMCP280_IRQ           (IRQ_A380_START + 27)
#define PLAT_FTMCP300_IRQ           (IRQ_A380_START + 28)
#define PLAT_FOTG210_IRQ            (IRQ_A380_START + 29)

#define PLAT_FOTG210_1_IRQ          (IRQ_A380_START + 30)
#define PLAT_FTKBC010_IRQ           (IRQ_A380_START + 31)
#define PLAT_FTKBC010_RX_IRQ        (IRQ_A380_START + 32)
#define PLAT_FTKBC010_TX_IRQ        (IRQ_A380_START + 33)
#define PLAT_FTKBC010_PAD_IRQ       (IRQ_A380_START + 34)
#define PLAT_FTLCDC210_IRQ          (IRQ_A380_START + 35)
#define PLAT_FTLCDC210_VSTATUS_IRQ  (IRQ_A380_START + 36)
#define PLAT_FTLCDC210_BAUPD_IRQ    (IRQ_A380_START + 37)
#define PLAT_FTLCDC210_FUR_IRQ      (IRQ_A380_START + 38)
#define PLAT_FTLCDC210_MERR_IRQ     (IRQ_A380_START + 39)

#define PLAT_FTSCALER300_IRQ        (IRQ_A380_START + 40)
#define PLAT_FTCAP300_IRQ           (IRQ_A380_START + 41)
#define PLAT_FTMCP100_IRQ           (IRQ_A380_START + 42)
#define PLAT_FTDMAC030_TC0_IRQ      (IRQ_A380_START + 43)
#define PLAT_FTDMAC030_TC1_IRQ      (IRQ_A380_START + 44)
#define PLAT_FTDMAC030_TC2_IRQ      (IRQ_A380_START + 45)
#define PLAT_FTDMAC030_TC3_IRQ      (IRQ_A380_START + 46)
#define PLAT_FTDMAC030_TC4_IRQ      (IRQ_A380_START + 47)
#define PLAT_FTDMAC030_TC5_IRQ      (IRQ_A380_START + 48)
#define PLAT_FTDMAC030_TC6_IRQ      (IRQ_A380_START + 49)

#define PLAT_FTDMAC030_TC7_IRQ      (IRQ_A380_START + 50)
#define PLAT_FTDMAC030_IRQ          (IRQ_A380_START + 51)
#define PLAT_FTDMAC030_ERR_IRQ      (IRQ_A380_START + 52)
#define PLAT_FTDMAC030_TC_IRQ       (IRQ_A380_START + 53)
#define PLAT_FTDMAC020_TC_IRQ       (IRQ_A380_START + 54)
#define PLAT_FTDMAC020_ERR_IRQ      (IRQ_A380_START + 55)
#define PLAT_PMMONITOR_IRQ          (IRQ_A380_START + 56)
#define PLAT_CA9_IRQ                (IRQ_A380_START + 57)
#define PLAT_CA9_1_IRQ              (IRQ_A380_START + 58)

#define PLAT_FTTMR010_IRQ           (IRQ_A380_START + 60)
#define PLAT_FTPCIEPPIO_GDAG2_IRQ   (IRQ_A380_START + 62)
#define PLAT_FTPCIEAPIO_GDAG2_IRQ   (IRQ_A380_START + 63)
#define PLAT_FTPCIEMISC_GDAG2_IRQ   (IRQ_A380_START + 64)
#define PLAT_FTPCIEWRAP_CDNG3_IRQ   (IRQ_A380_START + 67)

#define PLAT_GEM_IRQ                (IRQ_A380_START + 98)
#define PLAT_ADC_IRQ                (IRQ_A380_START + 99)

#define PLAT_FTAXIC030_3_IRQ        (IRQ_A380_START + 100)
#define PLAT_GEM_1_IRQ              (IRQ_A380_START + 102)
#define PLAT_FTDMAC030_2_ERR_IRQ    (IRQ_A380_START + 103)
#define PLAT_FTDMAC030_2_TC_IRQ     (IRQ_A380_START + 104)
#define PLAT_FTDMAC030_2_IRQ        (IRQ_A380_START + 105)
#define PLAT_FTDMAC030_1_TC0_IRQ    (IRQ_A380_START + 106)
#define PLAT_FTDMAC030_1_TC1_IRQ    (IRQ_A380_START + 107)
#define PLAT_FTDMAC030_1_TC2_IRQ    (IRQ_A380_START + 108)
#define PLAT_FTDMAC030_1_TC3_IRQ    (IRQ_A380_START + 109)

#define PLAT_FTDMAC030_1_TC4_IRQ    (IRQ_A380_START + 110)
#define PLAT_FTDMAC030_1_TC5_IRQ    (IRQ_A380_START + 111)
#define PLAT_FTDMAC030_1_TC6_IRQ    (IRQ_A380_START + 112)
#define PLAT_FTDMAC030_1_TC7_IRQ    (IRQ_A380_START + 113)
#define PLAT_FTDMAC030_1_IRQ        (IRQ_A380_START + 114)
#define PLAT_FTDMAC030_1_ERR_IRQ    (IRQ_A380_START + 115)
#define PLAT_FTDMAC030_1_TC_IRQ     (IRQ_A380_START + 116)
#define PLAT_FTX2P030_IRQ           (IRQ_A380_START + 117)
#define PLAT_FTX2P030_1_IRQ         (IRQ_A380_START + 118)
#define PLAT_FTX2P030_2_IRQ         (IRQ_A380_START + 119)

#define PLAT_FTAXIC030_IRQ          (IRQ_A380_START + 120)
#define PLAT_FTAXIC030_1_IRQ        (IRQ_A380_START + 121)
#define PLAT_FTAXIC030_2_IRQ        (IRQ_A380_START + 122)
#define PLAT_FTH2X030_IRQ           (IRQ_A380_START + 123)
#define PLAT_FTH2X030_1_IRQ         (IRQ_A380_START + 124)
#define PLAT_FTH2X030_2_IRQ         (IRQ_A380_START + 125)
#define PLAT_FTAHBB020_IRQ          (IRQ_A380_START + 127)

/*
 * Hardware handshake number for FTAPBB020
 */
#define PLAT_FTAPBB020_DMAHS_UART0TX 1
#define PLAT_FTAPBB020_DMAHS_UART0RX 1
#define PLAT_FTAPBB020_DMAHS_UART1TX 2
#define PLAT_FTAPBB020_DMAHS_UART1RX 2
#define PLAT_FTAPBB020_DMAHS_UART2TX 3
#define PLAT_FTAPBB020_DMAHS_UART2RX 3
#define PLAT_FTAPBB020_DMAHS_UART3TX 4
#define PLAT_FTAPBB020_DMAHS_UART3RX 5
#define PLAT_FTAPBB020_DMAHS_IRDA    6
#define PLAT_FTAPBB020_DMAHS_SSP0TX  7
#define PLAT_FTAPBB020_DMAHS_SSP0RX  8
#define PLAT_FTAPBB020_DMAHS_SSP1TX  9
#define PLAT_FTAPBB020_DMAHS_SSP1RX  10
#define PLAT_FTAPBB020_DMAHS_TSC     11
#define PLAT_FTAPBB020_DMAHS_TMR1    12
#define PLAT_FTAPBB020_DMAHS_TMR2    13
#define PLAT_FTAPBB020_DMAHS_TMR5    14

#endif    /* __MACH_IRQS_H */
