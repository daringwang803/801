/*
 *  arch/arm/mach-leo/platform_dt.c
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/platform_data/at24.h>
#ifdef CONFIG_CPU_HAS_GIC
#include <linux/irqchip/arm-gic.h>
#endif
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/serial_8250.h>
#include <linux/pinctrl/machine.h>
#include <linux/usb/phy.h>
#include <linux/input/matrix_keypad.h>
#include <linux/reboot.h>

#include <linux/platform_data/clk-faraday.h>

#include <asm/clkdev.h>
#include <asm/setup.h>
#include <asm/smp_twd.h>
#include <asm/memory.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#ifdef CONFIG_LOCAL_TIMERS
#include <asm/localtimer.h>
#endif
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <mach/hardware.h>
#include <mach/ftscu100.h>

#include <plat/core.h>
#include <plat/faraday.h>

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>

#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <linux/gpio_keys.h>
#include <linux/init.h>
#include <mach/irqs.h>

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/dmi.h>

#ifndef WDT_REG32
#define WDT_REG32(off)      *(volatile uint32_t __force*)(PLAT_FTWDT011_VA_BASE + (off))                                                
#endif

struct of_dev_auxdata plat_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("pinctrl-leo",           PLAT_SCU_BASE,          "pinctrl-leo",          NULL),
	OF_DEV_AUXDATA("faraday,ftwdt011",      PLAT_FTWDT011_BASE,     "ftwdt011.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftwdt011",      PLAT_FTWDT011_1_BASE,   "ftwdt011.1",           NULL),
	OF_DEV_AUXDATA("faraday,ftwdt011",      PLAT_FTWDT011_2_BASE,   "ftwdt011.2",           NULL),
	OF_DEV_AUXDATA("faraday,ftwdt011",      PLAT_FTWDT011_3_BASE,   "ftwdt011.3",           NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_BASE,    "serial0",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_1_BASE,  "serial1",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_2_BASE,  "serial2",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_3_BASE,  "serial3",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_4_BASE,  "serial4",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_5_BASE,  "serial5",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_6_BASE,  "serial6",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_7_BASE,  "serial7",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_8_BASE,  "serial8",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_9_BASE,  "serial9",              NULL),
	OF_DEV_AUXDATA("faraday,ftadcc010",     PLAT_FTADCC010_BASE,    "ftadcc010.0",          NULL),
	OF_DEV_AUXDATA("faraday,fttdcc010",     PLAT_FTTDCC010_BASE,    "fttdcc010.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_BASE,    "ftgpio010.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_1_BASE,  "ftgpio010.1",          NULL),
	OF_DEV_AUXDATA("faraday,ftcan010",      PLAT_FTCAN010_BASE,     "ftcan010.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftcan010",      PLAT_FTCAN010_BASE,     "ftcan010.1",           NULL),
	OF_DEV_AUXDATA("faraday,ftdmac020",     PLAT_FTDMAC020_BASE,    "ftdmac020.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac030",     PLAT_FTDMAC030_BASE,    "ftdmac030.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac030",     PLAT_FTDMAC030_1_BASE,  "ftdmac030.1",          NULL),
	OF_DEV_AUXDATA("faraday,ftsdc021-sdhci",PLAT_FTSDC021_BASE,     "ftsdc021.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftsdc021-sdhci",PLAT_FTSDC021_1_BASE,   "ftsdc021.1",           NULL),
	OF_DEV_AUXDATA("faraday,fotg210_hcd",   PLAT_FOTG210_BASE,      "fotg210_hcd.0",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_udc",   PLAT_FOTG210_BASE,      "fotg210_udc.0",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_hcd",   PLAT_FOTG210_1_BASE,    "fotg210_hcd.1",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_udc",   PLAT_FOTG210_1_BASE,    "fotg210_udc.1",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_hcd",   PLAT_FOTG210_2_BASE,    "fotg210_hcd.2",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_udc",   PLAT_FOTG210_2_BASE,    "fotg210_udc.2",        NULL),
	OF_DEV_AUXDATA("faraday,ftiic010",      PLAT_FTIIC010_BASE,     "fti2c010.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftiic010",      PLAT_FTIIC010_1_BASE,   "fti2c010.1",           NULL),
	OF_DEV_AUXDATA("faraday,ftiic010",      PLAT_FTIIC010_2_BASE,   "fti2c010.2",           NULL),
	OF_DEV_AUXDATA("faraday,ftiic010",      PLAT_FTIIC010_3_BASE,   "fti2c010.3",           NULL),
	OF_DEV_AUXDATA("faraday,ftiic010",      PLAT_FTIIC010_4_BASE,   "fti2c010.4",           NULL),
	OF_DEV_AUXDATA("faraday,ftkbc010",      PLAT_FTKBC010_BASE,     "ftkbc010.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftspi020-nor",  PLAT_FTSPI020_BASE,     "ftspi020-nor.0",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_BASE,     "ftssp010-spi.0",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_1_BASE,   "ftssp010-spi.1",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_2_BASE,   "ftssp010-spi.2",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_3_BASE,   "ftssp010-spi.3",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_4_BASE,   "ftssp010-spi.4",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_5_BASE,   "ftssp010-spi.5",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_6_BASE,   "ftssp010-spi.6",       NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-spi",  PLAT_FTSSP010_7_BASE,   "ftssp010-spi.7",       NULL),
	OF_DEV_AUXDATA("faraday,ftlcdc210",     PLAT_FTLCDC210_BASE,    "ftlcdc210.0",          NULL),
	{}
};
#if 0
static struct spi_board_info spi_devices[] __initdata = {
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 0,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 1,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 2,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 3,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 4,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 5,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 6,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	},
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 7,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	}
}; 
#endif
/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

static struct map_desc plat_io_desc[] __initdata = {
	{
		/* SCU */
		.virtual    = PLAT_SCU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SCU_BASE),
		.length     = SZ_1M,
		.type       = MT_DEVICE,
	},
	{	/* GIC DIST */
		.virtual    = PLAT_GIC_DIST_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GIC_DIST_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{	/* GIC CPU */
		.virtual    = PLAT_GIC_CPU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GIC_CPU_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		/* DDRC */
		.virtual    = PLAT_DDRC_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_DDRC_BASE),
		.length     = SZ_1M,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_3_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_3_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_4_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_4_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_5_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_5_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_6_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_6_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_7_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_7_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_8_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_8_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_9_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_9_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT011_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT011_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT011_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT011_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT011_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT011_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT011_3_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT011_3_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSPI020_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSPI020_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_3_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_3_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_4_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_4_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_5_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_5_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_6_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_6_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSSP010_7_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSSP010_7_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTLCDC210_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTLCDC210_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTCAN010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTCAN010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTCAN010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTCAN010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC020_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC020_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC030_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC030_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC030_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC030_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSDC021_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSDC021_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSDC021_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSDC021_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FOTG210_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FOTG210_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FOTG210_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FOTG210_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FOTG210_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FOTG210_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTGPIO010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTGPIO010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTGPIO010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTGPIO010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTIIC010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTIIC010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTIIC010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTIIC010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTIIC010_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTIIC010_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTIIC010_3_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTIIC010_3_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTIIC010_4_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTIIC010_4_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTKBC010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTKBC010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTGMAC030_VA_BASE, 
		.pfn        = __phys_to_pfn(PLAT_FTGMAC030_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTGMAC030_1_VA_BASE, 
		.pfn        = __phys_to_pfn(PLAT_FTGMAC030_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTGMAC030_2_VA_BASE, 
		.pfn        = __phys_to_pfn(PLAT_FTGMAC030_2_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_SMP_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SMP_BASE),
		.length     = SZ_2M,
		.type       = MT_DEVICE,
	},
        {
                .virtual    = PLAT_CM33_SRAM_VA_BASE,
                .pfn        = __phys_to_pfn(PLAT_CM33_SRAM_BASE),
                .length     = SZ_128K,
                .type       = MT_DEVICE,
        },	

};

static struct of_device_id faraday_clk_match[] __initconst = {
	{ .compatible = "faraday,leoevb-clk", .data = of_leo_clocks_init, },
	{}
};

static void __init platform_clock_init(void)
{
	struct device_node *np;
	const struct of_device_id *match;
	void (*clk_init)(struct device_node *);

	/* setup clock tree */
	np = of_find_matching_node(NULL, faraday_clk_match);
	if (!np)
		panic("unable to find a matching clock\n");

	match = of_match_node(faraday_clk_match, np);
	clk_init = match->data;
	clk_init(np);
}

static void __init platform_map_io(void)
{
	iotable_init((struct map_desc *)plat_io_desc, ARRAY_SIZE(plat_io_desc));

	ftscu100_init(__io(PLAT_SCU_VA_BASE));
}

static void __init platform_sys_timer_init(void)
{
	platform_clock_init();

	timer_probe();
}

#ifdef CONFIG_SND_SOC
static void __init platform_i2s_init(void)
{
	unsigned int temp = 0;

	// PLL#3 initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8050);
	temp |= 0x01;	// en
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8050);

	while((readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8020)>>2)&0x1)
		break;

	printk("%s: pll3 locked\n", __func__);

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x807C);
	temp |= 0x00000800;	// sspclk_i2s_1 from 12.288MHz
	temp |= 0x00000400;	// sspclk_i2s from 12.288MHz
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x807C);
}
#endif

#ifdef CONFIG_CAN
static void __init platform_can_init(void)
{
	unsigned int temp = 0;

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8074);
	temp |= 0x00000002;	// canbus1_en
	temp |= 0x00000001;	// canbus_en
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8074);
}
#endif

#ifdef CONFIG_FB
static void __init platform_lcdc_init(void)
{
	unsigned int temp = 0;

	// PLL#5 initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8060);
	temp |= 0x01;	// en
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8060);

	while((readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8020)>>3)&0x1)
		break;

	printk("%s: pll5 locked\n", __func__);
}
#endif

#ifdef CONFIG_MMC_SDHCI
static void __init platform_sdhci_init(void)
{
	unsigned int temp = 0;

	// PLL#7 initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8114);
	temp |= 0x01;	// en
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8114);

	while((readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8020)>>5)&0x1)
		break;

	printk("%s: pll7 locked\n", __func__);

	// switch eMMC clock source to PLL#7
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x807C);
	temp |= 0x2000;
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x807C);

	// eMMC DLL#0 initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8068);
	temp &= ~0x30;
	temp |= 0x30;	// frange
	temp |= 0x40;	// pdn
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8068);

	// eMMC DLL#1 initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x806C);
	temp &= ~0x30;
	temp |= 0x30;	// frange
	temp |= 0x40;	// pdn
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x806C);

#if 1	//20200521@BC: Increase driving strength to 16mA for the EMMC_1.
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8658);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x865C);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8660);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8664);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8668);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x866C);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8670);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8674);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x8678);
	writel(0x48, (void __iomem *)PLAT_SCU_VA_BASE + 0x867C);
	writel(0x40, (void __iomem *)PLAT_SCU_VA_BASE + 0x8680);
#endif
}

void platform_sdhci_wait_dll_lock(void)
{
	while((readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8068)>>8)&0x1)
		break;

	while((readl((void __iomem *)PLAT_SCU_VA_BASE + 0x806C)>>8)&0x1)
		break;
}
#endif

#ifdef CONFIG_USB_SUPPORT
static void __init platform_usb_init(void)
{
	unsigned int temp = 0;

	// outclksel initialize.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8030);
	temp |= 0x01;	// Reference Clock from CORECLKIN for FOTG210
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8030);

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8034);
	temp |= 0x01;	// Reference Clock from CORECLKIN for FOTG210_1
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8034);

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x80E4);
	temp |= 0x01;	// Reference Clock from CORECLKIN for FOTG210_2
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x80E4);

	// otg_rst_n deassert.
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x80A4);
	temp |= 0x08;	// for FOTG210
	temp |= 0x10;	// for FOTG210_1
	temp |= 0x20;	// for FOTG210_2
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x80A4);
}
#endif

static void __init platform_clkmux_sel(void)
{
	unsigned int temp = 0;

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x807C);
	temp |= 0x80000000;	// adc_mclk from PLL#2
	temp |= 0x20000000;	// spi_clk, ssp_clk and canbus_clk from PLL#1
	temp |= 0x00010000;	// tdc_mclk from PLL#2
	temp |= 0x00000200;	// cm33_busclk from PLL#6
	temp |= 0x00000100;	// pclk_non_secure from PLL#6
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x807C);

	//GMAC0 :RGMII , bit 8 gmac_rxck_en in SCU extension 0x8070 should be set
	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8070);
	temp |= (0x01<<8);
	//GMAC1 :RMII
	//set gmac1_ck_src_en=1(bit11), gmac1_refk_out_en=0(bit12)
	temp |= 0x01<<11;
	temp &=~(0x01<<12);
	//GMAC2 : MII
	//enable bit 15 gmac2_ck_src_en
	temp |= (0x01<<15);
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8070);
}

static void __init platform_init_early(void)
{
#ifdef CONFIG_CAN
	platform_can_init();
#endif

#ifdef CONFIG_SND_SOC
	platform_i2s_init();
#endif

#ifdef CONFIG_FB
	platform_lcdc_init();
#endif

#ifdef CONFIG_MMC_SDHCI
	platform_sdhci_init();
#endif

#ifdef CONFIG_USB_SUPPORT
	platform_usb_init();
#endif

	platform_clkmux_sel();
}

#ifdef CONFIG_PINCTRL_LEO_MODEX
static bool __init of_device_available_by_path(const char *path)
{
	struct device_node *np;

	np = of_find_node_opts_by_path(path, NULL);
	if (np)
		return of_device_is_available(np);
	else
		return false;
}

static bool __init of_device_property_by_path(const char *path, const char *propname)
{
	struct device_node *np;

	np = of_find_node_opts_by_path(path, NULL);
	if (np)
		return of_property_read_bool(np, propname);
	else
		return false;
}

static void __init platform_usb_u_iddig(void)
{
	bool avail;

	avail = of_device_available_by_path("/soc/usb_hcd@10200000");
	if (avail && of_device_property_by_path("/soc/usb_hcd@10200000", "mini-A-plug")) {
		writel(0x00F0C103, (void __iomem *)PLAT_SCU_VA_BASE + 0x8030);
	}

	avail = of_device_available_by_path("/soc/usb_hcd@10300000");
	if (avail && of_device_property_by_path("/soc/usb_hcd@10300000", "mini-A-plug")) {
		writel(0x00F0C103, (void __iomem *)PLAT_SCU_VA_BASE + 0x8034);
	}

	avail = of_device_available_by_path("/soc/usb_hcd@10400000");
	if (avail && of_device_property_by_path("/soc/usb_hcd@10400000", "mini-A-plug")) {
		writel(0x00F0C103, (void __iomem *)PLAT_SCU_VA_BASE + 0x80E4);
	}
}

static void __init platform_pinctrl_modex(void)
{
	unsigned int temp = 0;
	bool avail;

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8110);

	avail = of_device_available_by_path("/soc/spi@22100000");
	if (avail && of_device_property_by_path("/soc/spi@22100000", "spi-slave"))
		temp |= 0x00000080;     // ssp7_spi slave mode
	else
		temp &= ~0x00000080;    // ssp7_spi master mode

	avail = of_device_available_by_path("/soc/spi@56800000");
	if (avail && of_device_property_by_path("/soc/spi@56800000", "spi-slave"))
		temp |= 0x00000040;     // ssp6_spi slave mode
	else
		temp &= ~0x00000040;    // ssp6_spi master mode

	avail = of_device_available_by_path("/soc/spi@56700000");
	if (avail && of_device_property_by_path("/soc/spi@56700000", "spi-slave"))
		temp |= 0x00000020;     // ssp5_spi slave mode
	else
		temp &= ~0x00000020;    // ssp5_spi master mode

	avail = of_device_available_by_path("/soc/spi@56600000");
	if (avail && of_device_property_by_path("/soc/spi@22100000", "spi-slave"))
		temp |= 0x00000010;     // ssp4_spi slave mode
	else
		temp &= ~0x00000010;    // ssp4_spi master mode

	avail = of_device_available_by_path("/soc/spi@56500000");
	if (avail && of_device_property_by_path("/soc/spi@56500000", "spi-slave"))
		temp |= 0x00000008;     // ssp3_spi slave mode
	else
		temp &= ~0x00000008;    // ssp3_spi master mode

	avail = of_device_available_by_path("/soc/spi@54d00000");
	if (avail && of_device_property_by_path("/soc/spi@54d00000", "spi-slave"))
		temp |= 0x00000004;     // ssp2_spi slave mode
	else
		temp &= ~0x00000004;    // ssp2_spi master mode

	avail = of_device_available_by_path("/soc/spi@54c00000");
	if (avail && of_device_property_by_path("/soc/spi@54c00000", "spi-slave"))
		temp |= 0x00000002;     // ssp1_spi slave mode
	else
		temp &= ~0x00000002;    // ssp1_spi master mode

	avail = of_device_available_by_path("/soc/spi@54b00000");
	if (avail && of_device_property_by_path("/soc/spi@54b00000", "spi-slave"))
		temp |= 0x00000001;     // ssp0_spi slave mode
	else
		temp &= ~0x00000001;    // ssp0_spi master mode

	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8110);

	temp = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8204);

	avail = of_device_available_by_path("/soc/uart_sir@56c00000");
	if (avail && of_device_property_by_path("/soc/uart_sir@56c00000", "pin-mux-modex")) {
		temp |= 0x00040000;     // X_UART8_TX     pin-mux by UART8_TX pin
		                        // X_UART8_RX     pin-mux by UART8_RX pin
	} else {
		temp &= ~0x00040000;    // X_UART8_TX     pin-mux by pinctrl-driver
		                        // X_UART8_RX     pin-mux by pinctrl-driver
	}

	avail = of_device_available_by_path("/soc/spi@56800000");
	if (avail && of_device_property_by_path("/soc/spi@56800000", "pin-mux-modex")) {
		temp |= 0x00020000;     // X_RGMII_TXD[3] pin-mux by SPI6_CS1 pin
		temp |= 0x00010000;     // X_RGMII_TXD[2] pin-mux by SPI6_CS0 pin
		                        // X_RGMII_TXD[1] pin-mux by SPI6_CLK pin
		                        // X_RGMII_TXD[0] pin-mux by SPI6_MOSI pin
		                        // X_RGMII_RX_CK  pin-mux by SPI6_MISO pin
	} else {
		temp &= ~0x00020000;    // X_RGMII_TXD[3] pin-mux by pinctrl-driver
		temp &= ~0x00010000;    // X_RGMII_TXD[2] pin-mux by pinctrl-driver
		                        // X_RGMII_TXD[1] pin-mux by pinctrl-driver
		                        // X_RGMII_TXD[0] pin-mux by pinctrl-driver
		                        // X_RGMII_RX_CK  pin-mux by pinctrl-driver
	}

	avail = of_device_available_by_path("/soc/uart@57100000");
	if (avail && of_device_property_by_path("/soc/uart@57100000", "pin-mux-modex")) {
		temp |= 0x00008000;     // X_RGMII_RXD[1] pin-mux by UART9_TX pin
		                        // X_RGMII_RXD[0] pin-mux by UART9_RX pin
	} else {
		temp &= ~0x00008000;	// X_RGMII_RXD[1] pin-mux by pinctrl-driver
		                        // X_RGMII_RXD[0] pin-mux by pinctrl-driver
	}

	avail = of_device_available_by_path("/soc/uart@56a00000");
	if (avail && of_device_property_by_path("/soc/uart@56a00000", "pin-mux-modex")) {
		temp |= 0x00004000;     // X_RGMII_RX_CTL pin-mux by UART6_TX pin
		                        // X_RGMII_RX_MDC pin-mux by UART6_RX pin
	} else {
		temp &= ~0x00004000;	// X_UART6_TX     pin-mux by pinctrl-driver
		                        // X_UART6_RX     pin-mux by pinctrl-driver
	}

	avail = of_device_available_by_path("/soc/gpio@54000000");
	if (avail && of_device_property_by_path("/soc/gpio@54000000", "pin-mux-modex")) {
//		temp |= 0x00002000;     // X_UART7_RX     pin-mux by GPIO0_30
//		temp |= 0x00001000;     // X_UART7_TX     pin-mux by GPIO0_29
//		temp |= 0x00000800;     // X_RMII_RXD[1]  pin-mux by GPIO0_26
		temp |= 0x00000400;     // X_I2S0_FS      pin-mux by GPIO0_25
		temp |= 0x00000200;     // X_MII_TX_EN    pin-mux by GPIO0_24
		temp |= 0x00000100;     // X_I2S0_SCLK    pin-mux by GPIO0_23
		temp |= 0x00000080;     // X_I2S0_RXD     pin-mux by GPIO0_22
		temp |= 0x00000040;     // X_LC_DATA[5]   pin-mux by GPIO0_21
		temp |= 0x00000020;     // X_UART2_NCTS   pin-mux by GPIO0_20
		temp |= 0x00000010;     // X_UART2_NRTS   pin-mux by GPIO0_19
		temp |= 0x00000008;     // X_I2S0_TXD     pin-mux by GPIO0_18
		temp |= 0x00000004;     // X_LC_DATA[6]   pin-mux by GPIO0_17
		temp |= 0x00000002;     // X_UART5_RX     pin-mux by GPIO0_3
		temp |= 0x00000001;     // X_UART5_TX     pin-mux by GPIO0_2
	} else {
		temp &= ~0x00002000;    // X_UART7_RX     pin-mux by pinctrl-driver
		temp &= ~0x00001000;    // X_UART7_TX     pin-mux by pinctrl-driver
		temp &= ~0x00000800;    // X_RMII_RXD[1]  pin-mux by pinctrl-driver
		temp &= ~0x00000400;    // X_I2S0_FS      pin-mux by pinctrl-driver
		temp &= ~0x00000200;    // X_MII_TX_EN    pin-mux by pinctrl-driver
		temp &= ~0x00000100;    // X_I2S0_SCLK    pin-mux by pinctrl-driver
		temp &= ~0x00000080;    // X_I2S0_RXD     pin-mux by pinctrl-driver
		temp &= ~0x00000040;    // X_LC_DATA[5]   pin-mux by pinctrl-driver
		temp &= ~0x00000020;    // X_UART2_NCTS   pin-mux by pinctrl-driver
		temp &= ~0x00000010;    // X_UART2_NRTS   pin-mux by pinctrl-driver
		temp &= ~0x00000008;    // X_I2S0_TXD     pin-mux by pinctrl-driver
		temp &= ~0x00000004;    // X_LC_DATA[6]   pin-mux by pinctrl-driver
		temp &= ~0x00000002;    // X_UART5_RX     pin-mux by pinctrl-driver
		temp &= ~0x00000001;    // X_UART5_TX     pin-mux by pinctrl-driver
	}
	temp |=((1<<7) | (1<<10)); //gpio22 gpio25
	writel(temp, (void __iomem *)PLAT_SCU_VA_BASE + 0x8204);
}
#endif

/* GPIO-keys input device */
static struct gpio_keys_button leo_gpio_keys[] = {
	{
		.type	= EV_KEY,
		.code	= KEY_1,
		.gpio	= 1 * 32 + 29,
		.desc	= "KEY S1",
		.active_low = 1,
		.wakeup	= 0,
	}, {
		.type	= EV_KEY,
		.code	= KEY_2,
		.gpio	= 1 * 32 + 28,
		.desc	= "KEY S2",
		.active_low = 1,
		.wakeup	= 0,
	}, {
		.type	= EV_KEY,
		.code	= KEY_3,
		.gpio	= 1 * 32 + 2,
		.desc	= "KEY S3",
		.active_low = 1,
		.wakeup	= 0,
	}, {
		.type	= EV_KEY,
		.code	= KEY_4,
		.gpio	= 0 * 32 + 15,
		.desc	= "KEY S3",
		.active_low = 1,
		.wakeup	= 0,
	},
};

static struct gpio_keys_platform_data leo_gpio_keys_platform_data = {
	.buttons	= leo_gpio_keys,
	.nbuttons	= ARRAY_SIZE(leo_gpio_keys),
	.poll_interval = 20,
};

static struct platform_device leo_keys_device  = {
	.name		= "gpio-keys-polled",
	.id		= 0,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &leo_gpio_keys_platform_data,
	}
};


static struct platform_device *leo_devices[] __initdata = {
	&leo_keys_device,
};

static void __init platform_init(void)
{
	platform_usb_u_iddig();
#ifdef CONFIG_PINCTRL_LEO_MODEX
	platform_pinctrl_modex();
#endif

#ifdef CONFIG_SPI_FTSSP010
	//spi_register_board_info(spi_devices, ARRAY_SIZE(spi_devices));
#endif 
	of_platform_populate(NULL, of_default_bus_match_table, plat_auxdata_lookup, NULL);
}

void platform_reset(enum reboot_mode mode, const char *cmd)
{
	WDT_REG32(0x00) = 0x5555;       //write REPROG_KEY
	while (WDT_REG32(0x14) & 0x01); //wait timer disable
	WDT_REG32(0x14) |= 0x0004;      //reset enable
	WDT_REG32(0x08) = 0x0001;       //write counter reload value
	WDT_REG32(0x00) = 0xCCCC;       //write START_KEY
	printk("restart...");
	mdelay(50);
	/* Software reset command  */
	*(volatile uint32_t __force*)(PLAT_FTSCU100_VA_BASE + (0x20)) = 0x400;
}

static int __init platform_late_init(void)
{
	//printk("#########platform_late_init\n");
	//platform_add_devices(leo_devices, ARRAY_SIZE(leo_devices));
	return 0;
}
late_initcall(platform_late_init);

static const char *faraday_dt_match[] __initconst = {
	"arm,faraday-soc",
	NULL,
};

DT_MACHINE_START(FARADAY, "LEO")
	.atag_offset  = 0x100,
	.dt_compat    = faraday_dt_match,
	.smp          = smp_ops(faraday_smp_ops),
	.map_io       = platform_map_io,
	.init_time    = platform_sys_timer_init,
	.init_early   = platform_init_early,
	.init_machine = platform_init,
	.restart      = platform_reset,
MACHINE_END
