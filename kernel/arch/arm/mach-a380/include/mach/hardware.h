/*
 *  arch/arm/mach-a380/include/mach/hardware.h
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

#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

#include <asm/sizes.h>

/* Memory Mapped Addresses */

/* Cortex-A9 Private Memory Region */
#ifdef CONFIG_CPU_CA9
#ifdef CONFIG_FARADAY_CA9_TESTCHIP
#define PLAT_CPU_PERIPH_BASE            0xE0000000
#else
#define PLAT_CPU_PERIPH_BASE            0xF4000000
#endif /* CONFIG_FARADAY_CA9_TESTCHIP */
#define PLAT_CPU_PERIPH_VA_BASE         0xF4000000

#define PLAT_GIC_CPU_BASE               (PLAT_CPU_PERIPH_BASE + 0x100)
#define PLAT_GIC_CPU_VA_BASE            (PLAT_CPU_PERIPH_VA_BASE + 0x100)
#define PLAT_GTIMER_BASE                (PLAT_CPU_PERIPH_BASE + 0x200)
#define PLAT_GTIMER_VA_BASE             (PLAT_CPU_PERIPH_VA_BASE + 0x200)
#define PLAT_TWD_BASE                   (PLAT_CPU_PERIPH_BASE + 0x600)
#define PLAT_TWD_VA_BASE                (PLAT_CPU_PERIPH_VA_BASE + 0x600)
#define PLAT_GIC_DIST_BASE              (PLAT_CPU_PERIPH_BASE + 0x1000)
#define PLAT_GIC_DIST_VA_BASE           (PLAT_CPU_PERIPH_VA_BASE + 0x1000)
#endif /* CONFIG_CPU_CA9 */

#ifdef CONFIG_CACHE_L2X0
#ifdef CONFIG_FARADAY_CA9_TESTCHIP
#define PLAT_PL310_BASE                 0xF0200000
#else
#define PLAT_PL310_BASE                 0xF5000000
#endif /* CONFIG_FARADAY_CA9_TESTCHIP */
#define PLAT_PL310_VA_BASE              0xF9F00000
#endif /* CONFIG_CACHE_L2X0 */

#define PLAT_SCU_BASE                   PLAT_FTSCU100_BASE
#define PLAT_SCU_VA_BASE                PLAT_FTSCU100_VA_BASE

#define DEBUG_LL_FTUART010_PA_BASE      PLAT_FTUART010_BASE
#define DEBUG_LL_FTUART010_VA_BASE      PLAT_FTUART010_VA_BASE

/* AXI Devices */
#define PLAT_FTEMC030_BASE              0x88000000
#define PLAT_FTLCDC210_BASE             0x92900000
#define PLAT_FTDMAC030_BASE             0x92D00000
#define PLAT_FTDMAC030_1_BASE           0x92E00000
#define PLAT_FTDDRII030_BASE            0x92F00000
#define PLAT_GEM_BASE                   0x93000000
#define PLAT_GEM_1_BASE                 0x93100000
#define PLAT_FTDMAC030_2_BASE           0x96000000
#define PLAT_FTINTC030_BASE             0x96800000
#define PLAT_FTSCALER300_BASE           0x98000000
#define PLAT_FTVCAP300_BASE             0x98100000
#define PLAT_FTPCIEAXISLV_BASE          0xE0000000

#define PLAT_FTEMC030_VA_BASE           0xF8800000
#define PLAT_FTLCDC210_VA_BASE          0xF9290000
#define PLAT_FTDMAC030_VA_BASE          0xF92D0000
#define PLAT_FTDMAC030_1_VA_BASE        0xF92E0000
#define PLAT_FTDDRII030_VA_BASE         0xF92F0000
#define PLAT_GEM_VA_BASE                0xF9300000
#define PLAT_GEM_1_VA_BASE              0xF9310000
#define PLAT_FTDMAC030_2_VA_BASE        0xF9600000
#define PLAT_FTINTC030_VA_BASE          0xF9680000
#define PLAT_FTSCALER300_VA_BASE        0xF9800000
#define PLAT_FTVCAP300_VA_BASE          0xF9810000
#define PLAT_FTPCIEAXISLV_VA_BASE       0xFE000000

/* AHB Devices */
#define PLAT_AXI_Slave_CSR_BASE         0xA0000000
#define PLAT_AXI_Slave_APIO_BASE        0xA8000000
#define PLAT_FTDMAC020_BASE             0xC0300000
#define PLAT_FTSDC021_BASE              0xC0400000
#define PLAT_FTNANDC024_DATA_BASE       0xC0500000
#define PLAT_FTNANDC024_BASE            0xC0600000
#define PLAT_FTAES020_BASE              0xC0700000
#define PLAT_FOTG210_BASE               0xC0800000
#define PLAT_FOTG210_1_BASE             0xC0900000
#define PLAT_FTSPI020_BASE              0xC0A00000
#define PLAT_FTMCP100_BASE              0xC0B00000
#define PLAT_FTMCP280_BASE              0xC0D00000
#define PLAT_FTMCP300_BASE              0xC0E00000

#define PLAT_AXI_SLAVE_CSR_VA_BASE      0xFA000000
#define PLAT_AXI_SLAVE_APIO_VA_BASE     0xFA800000
#define PLAT_FTDMAC020_VA_BASE          0xFC030000
#define PLAT_FTSDC021_VA_BASE           0xFC040000
#define PLAT_FTNANDC024_DATA_VA_BASE    0xFC050000
#define PLAT_FTNANDC024_VA_BASE         0xFC060000
#define PLAT_FTAES020_VA_BASE           0xFC070000
#define PLAT_FOTG210_VA_BASE            0xFC080000
#define PLAT_FOTG210_1_VA_BASE          0xFC090000
#define PLAT_FTSPI020_VA_BASE           0xFC0A0000
#define PLAT_FTMCP100_VA_BASE           0xFC0B0000
#define PLAT_FTMCP280_VA_BASE           0xFC0D0000
#define PLAT_FTMCP300_VA_BASE           0xFC0E0000

/* APB Devices */
#define PLAT_FTGPIO010_BASE             0x90100000
#define PLAT_FTIIC010_BASE              0x90200000
#define PLAT_FTIIC010_1_BASE            0x90300000
#define PLAT_FTUART010_BASE             0x90400000
#define PLAT_FTUART010_1_BASE           0x90500000
#define PLAT_FTUART010_2_BASE           0x90600000
#define PLAT_FTSSP010_BASE              0x90700000
#define PLAT_FTSSP010_1_BASE            0x90800000
#define PLAT_FTSSP010_2_BASE            0x90900000
#define PLAT_FTSCU100_BASE              0x90A00000
#define PLAT_FTTMR010_BASE              0x90B00000
#define PLAT_FTWDT010_BASE              0x90C00000
#define PLAT_FTKBC010_BASE              0x90D00000
#define PLAT_TEMPERATURE_SENSOR_BASE    0x91400000
#define PLAT_SERDES_BASE                0x92600000
#define PLAT_SERDES_1_BASE              0x92700000
#define PLAT_SERDES_2_BASE              0x92800000
#define PLAT_FTLCDC210_BASE             0x92900000
#define PLAT_FTWRAP_BASE                0x93600000
#define PLAT_FTPCIE_BASE                0x93800000
#define PLAT_FTPCIE_LOCAL_BASE          0x93900000

#define PLAT_FTGPIO010_VA_BASE          0xF9010000
#define PLAT_FTIIC010_VA_BASE           0xF9020000
#define PLAT_FTIIC010_1_VA_BASE         0xF9030000
#define PLAT_FTUART010_VA_BASE          0xF9040000
#define PLAT_FTUART010_1_VA_BASE        0xF9050000
#define PLAT_FTUART010_2_VA_BASE        0xF9060000
#define PLAT_FTSSP010_VA_BASE           0xF9070000
#define PLAT_FTSSP010_1_VA_BASE         0xF9080000
#define PLAT_FTSSP010_2_VA_BASE         0xF9090000
#define PLAT_FTSCU100_VA_BASE           0xF90A0000
#define PLAT_FTTMR010_VA_BASE           0xF90B0000
#define PLAT_FTWDT010_VA_BASE           0xF90C0000
#define PLAT_FTKBC010_VA_BASE           0xF90D0000
#define PLAT_TEMPERATURE_SENSOR_VA_BASE 0xf9140000
#define PLAT_SERDES_VA_BASE             0xF9260000
#define PLAT_SERDES_1_VA_BASE           0xF9270000
#define PLAT_SERDES_2_VA_BASE           0xF9280000
#define PLAT_FTLCDC210_VA_BASE          0xF9290000
#define PLAT_FTWRAP_VA_BASE             0xF9360000
#define PLAT_FTPCIE_VA_BASE             0xF9380000
#define PLAT_FTPCIE_LOCAL_VA_BASE       0xF9390000

/* LCDC Framebuffer */
#define PLAT_FRAMEBUFFER_BASE           0x40000000 + SZ_256M

/* SMP */
#define PLAT_SMP_BASE                   PLAT_FTEMC030_BASE+0x3800
#define PLAT_SMP_VA_BASE                PLAT_FTEMC030_VA_BASE+0x3800
#define PLAT_SMP_MAGIC                  0x12345678
#endif /* __MACH_HARDWARE_H */

#if defined(CONFIG_PCI) && defined(CONFIG_FTPCIE_CDNG3)
#define PCIBIOS_MIN_IO                  0x1000
#define PCIBIOS_MIN_MEM                 0x01000000
#define pcibios_assign_all_busses()     1
#endif
