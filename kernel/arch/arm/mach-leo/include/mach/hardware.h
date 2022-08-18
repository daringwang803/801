/*
 *  arch/arm/mach-leo/include/mach/hardware.h
 *
 *  Copyright (C) 2020 Faraday Technology
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

#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

#include <asm/sizes.h>

/* Memory Mapped Addresses */
#define PLAT_CPU_PERIPH_BASE            0x70000000
#define PLAT_CPU_PERIPH_VA_BASE         0xF9E00000

#define PLAT_GIC_CPU_BASE               (PLAT_CPU_PERIPH_BASE + 0x2000)
#define PLAT_GIC_CPU_VA_BASE            (PLAT_CPU_PERIPH_VA_BASE + 0x2000)
#define PLAT_GIC_DIST_BASE              (PLAT_CPU_PERIPH_BASE + 0x1000)
#define PLAT_GIC_DIST_VA_BASE           (PLAT_CPU_PERIPH_VA_BASE + 0x1000)

#define PLAT_SCU_BASE                   PLAT_FTSCU100_BASE
#define PLAT_SCU_VA_BASE                PLAT_FTSCU100_VA_BASE

#define PLAT_DDRC_BASE                  PLAT_FTDDRC3030_BASE
#define PLAT_DDRC_VA_BASE               PLAT_FTDDRC3030_VA_BASE

#define DEBUG_LL_FTUART010_PA_BASE      PLAT_FTUART010_BASE
#define DEBUG_LL_FTUART010_VA_BASE      PLAT_FTUART010_VA_BASE

/* ADC Devices */
#define PLAT_FTADCC010_BASE             0x54400000

#define PLAT_FTADCC010_VA_BASE          0xF5440000

/* CAN Devices */
#define PLAT_FTCAN010_BASE              0x56300000
#define PLAT_FTCAN010_1_BASE            0x54900000

#define PLAT_FTCAN010_VA_BASE           0xF5630000
#define PLAT_FTCAN010_1_VA_BASE         0xF5490000

/* DDRC Devices */
#define PLAT_FTDDRC3030_BASE            0x28900000

#define PLAT_FTDDRC3030_VA_BASE         0xF2890000

/* DMA Devices */
#define PLAT_FTDMAC020_BASE             0x50000000
#define PLAT_FTDMAC030_BASE             0x28500000
#define PLAT_FTDMAC030_1_BASE           0x54800000

#define PLAT_FTDMAC020_VA_BASE          0xF5000000
#define PLAT_FTDMAC030_VA_BASE          0xF2850000
#define PLAT_FTDMAC030_1_VA_BASE        0xF5480000

/* GPIO Devices */
#define PLAT_FTGPIO010_BASE             0x54000000
#define PLAT_FTGPIO010_1_BASE           0x56000000

#define PLAT_FTGPIO010_VA_BASE          0xF5400000
#define PLAT_FTGPIO010_1_VA_BASE        0xF5600000

/* IIC Devices */
#define PLAT_FTIIC010_BASE              0x55500000
#define PLAT_FTIIC010_1_BASE            0x55600000
#define PLAT_FTIIC010_2_BASE            0x55700000
#define PLAT_FTIIC010_3_BASE            0x56F00000
#define PLAT_FTIIC010_4_BASE            0x57000000

#define PLAT_FTIIC010_VA_BASE           0xF5550000
#define PLAT_FTIIC010_1_VA_BASE         0xF5560000
#define PLAT_FTIIC010_2_VA_BASE         0xF5570000
#define PLAT_FTIIC010_3_VA_BASE         0xF56F0000
#define PLAT_FTIIC010_4_VA_BASE         0xF5700000

/* KBC Devices */
#define PLAT_FTKBC010_BASE              0x56100000

#define PLAT_FTKBC010_VA_BASE           0xF5610000

/* LCDC Devices */
#define PLAT_FTLCDC210_BASE             0x56200000

#define PLAT_FTLCDC210_VA_BASE          0xF5620000

/* MAC Devices */
#define PLAT_FTGMAC030_BASE             0x54100000
#define PLAT_FTGMAC030_1_BASE           0x54200000
#define PLAT_FTGMAC030_2_BASE           0x54300000

#define PLAT_FTGMAC030_VA_BASE          0xF5410000
#define PLAT_FTGMAC030_1_VA_BASE        0xF5420000
#define PLAT_FTGMAC030_2_VA_BASE        0xF5430000

/* PWM Devices */
#define PLAT_FTPWMTMR010_BASE           0x55300000
#define PLAT_FTPWMTMR010_1_BASE         0x55400000
#define PLAT_FTPWMTMR010_2_BASE         0x56D00000
#define PLAT_FTPWMTMR010_3_BASE         0x56E00000

#define PLAT_FTPWMTMR010_VA_BASE        0xF5530000
#define PLAT_FTPWMTMR010_1_VA_BASE      0xF5540000
#define PLAT_FTPWMTMR010_2_VA_BASE      0xF56D0000
#define PLAT_FTPWMTMR010_3_VA_BASE      0xF56E0000

/* SDC Devices */
#define PLAT_FTSDC021_BASE              0x12100000
#define PLAT_FTSDC021_1_BASE            0x12200000

#define PLAT_FTSDC021_VA_BASE           0xF1210000
#define PLAT_FTSDC021_1_VA_BASE         0xF1220000

/* SPI Devices */
#define PLAT_FTSPI020_BASE              0x12000000

#define PLAT_FTSPI020_VA_BASE           0xF1200000

/* SSP Devices */
#define PLAT_FTSSP010_BASE              0x54B00000
#define PLAT_FTSSP010_1_BASE            0x54C00000
#define PLAT_FTSSP010_2_BASE            0x54D00000
#define PLAT_FTSSP010_3_BASE            0x56500000
#define PLAT_FTSSP010_4_BASE            0x56600000
#define PLAT_FTSSP010_5_BASE            0x56700000
#define PLAT_FTSSP010_6_BASE            0x56800000
#define PLAT_FTSSP010_7_BASE            0x22100000

#define PLAT_FTSSP010_VA_BASE           0xF54B0000
#define PLAT_FTSSP010_1_VA_BASE         0xF54C0000
#define PLAT_FTSSP010_2_VA_BASE         0xF54D0000
#define PLAT_FTSSP010_3_VA_BASE         0xF5650000
#define PLAT_FTSSP010_4_VA_BASE         0xF5660000
#define PLAT_FTSSP010_5_VA_BASE         0xF5670000
#define PLAT_FTSSP010_6_VA_BASE         0xF5680000
#define PLAT_FTSSP010_7_VA_BASE         0xF2210000

/* SYSC Devices */
#define PLAT_FTSCU100_BASE              0x28000000

#define PLAT_FTSCU100_VA_BASE           0xFD000000

/* TDC Devices */
#define PLAT_FTTDCC010_BASE             0x54500000

#define PLAT_FTTDCC010_VA_BASE          0xF5450000

/* UART Devices */
#define PLAT_FTUART010_BASE             0x54E00000
#define PLAT_FTUART010_1_BASE           0x54F00000
#define PLAT_FTUART010_2_BASE           0x55000000
#define PLAT_FTUART010_3_BASE           0x55100000
#define PLAT_FTUART010_4_BASE           0x55200000
#define PLAT_FTUART010_5_BASE           0x56900000
#define PLAT_FTUART010_6_BASE           0x56A00000
#define PLAT_FTUART010_7_BASE           0x56B00000
#define PLAT_FTUART010_8_BASE           0x56C00000
#define PLAT_FTUART010_9_BASE           0x57100000

#define PLAT_FTUART010_VA_BASE          0xF54E0000
#define PLAT_FTUART010_1_VA_BASE        0xF54F0000
#define PLAT_FTUART010_2_VA_BASE        0xF5500000
#define PLAT_FTUART010_3_VA_BASE        0xF5510000
#define PLAT_FTUART010_4_VA_BASE        0xF5520000
#define PLAT_FTUART010_5_VA_BASE        0xF5690000
#define PLAT_FTUART010_6_VA_BASE        0xF56A0000
#define PLAT_FTUART010_7_VA_BASE        0xF56B0000
#define PLAT_FTUART010_8_VA_BASE        0xF56C0000
#define PLAT_FTUART010_9_VA_BASE        0xF5710000

/* USB Devices */
#define PLAT_FOTG210_BASE               0x10200000
#define PLAT_FOTG210_1_BASE             0x10300000
#define PLAT_FOTG210_2_BASE             0x10400000

#define PLAT_FOTG210_VA_BASE            0xF1020000
#define PLAT_FOTG210_1_VA_BASE          0xF1030000
#define PLAT_FOTG210_2_VA_BASE          0xF1040000

/* WDT Devices */
#define PLAT_FTWDT011_BASE              0x28100000
#define PLAT_FTWDT011_1_BASE            0x28200000
#define PLAT_FTWDT011_2_BASE            0x54600000
#define PLAT_FTWDT011_3_BASE            0x54700000

#define PLAT_FTWDT011_VA_BASE           0xF2810000
#define PLAT_FTWDT011_1_VA_BASE         0xF2820000
#define PLAT_FTWDT011_2_VA_BASE         0xF5460000
#define PLAT_FTWDT011_3_VA_BASE         0xF5470000

/* SMP */
#define PLAT_SMP_BASE                   0x22200000
#define PLAT_SMP_VA_BASE                0xF2220000

/* EFUSE */
#define PLAT_EFUSE_BASE					0x22000000
#define PLAT_EFUSE_VA_BASE				0xF0F30000

/* CM33 SRAM */
#define PLAT_CM33_SRAM_BASE		0x50100000
#define PLAT_CM33_SRAM_VA_BASE		0xF5010000

#endif /* __MACH_HARDWARE_H */
