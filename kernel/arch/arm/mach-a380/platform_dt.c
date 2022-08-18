/*
 *  arch/arm/mach-a380/platform_dt.c
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
#include <mach/serdes.h>
#include <mach/pwrup_1_25gbps.h>

#include <plat/core.h>
#include <plat/faraday.h>
#include <plat/fttmr010.h>
#include <plat/ftintc030.h>

#ifndef WDT_REG32
#define WDT_REG32(off)      *(volatile uint32_t __force*)(PLAT_FTWDT010_VA_BASE + (off))                                                
#endif

/* CPU Match ID for FTINTC030 */
#define A380_EXTCPU_MATCH_ID	0x600
#define A380_FA626TE_MATCH_ID	0x300

static struct platform_nand_flash nand_fl = {
	.name = "nand",
};

struct of_dev_auxdata plat_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("pinctrl-a380",          PLAT_SCU_BASE,          "pinctrl-a380",         NULL),
	OF_DEV_AUXDATA("faraday,ftwdt010",      PLAT_FTWDT010_BASE,     "ftwdt010.0",           NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_BASE,    "serial0",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_1_BASE,  "serial1",              NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_BASE,    "ftgpio010.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac020",     PLAT_FTDMAC020_BASE,    "ftdmac020.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac030",     PLAT_FTDMAC030_BASE,    "ftdmac030.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac030",     PLAT_FTDMAC030_1_BASE,  "ftdmac030.1",          NULL),
	OF_DEV_AUXDATA("faraday,ftnandc024",    PLAT_FTNANDC024_BASE,   "ftnandc024_nand.0",    &nand_fl),
	OF_DEV_AUXDATA("faraday,ftsdc021-sdhci",PLAT_FTSDC021_BASE,     "ftsdc021.0",           NULL),	
	OF_DEV_AUXDATA("faraday,fotg210",       PLAT_FOTG210_BASE,      "fotg210_otg.0",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_hcd",   PLAT_FOTG210_BASE,      "fotg210_hcd.0",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_udc",   PLAT_FOTG210_BASE,      "fotg210_udc.0",        NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_BASE,     "fti2c010.0",           NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_1_BASE,   "fti2c010.1",           NULL),
	OF_DEV_AUXDATA("faraday,ftssp010-i2s",  PLAT_FTSSP010_BASE,     "ftssp010-i2s.0",       NULL),
	OF_DEV_AUXDATA("cdns,gem",              PLAT_GEM_BASE,          "macb.0",               NULL),
	{}
};

/*
 * I2C devices
 */
#ifdef CONFIG_I2C_BOARDINFO

static struct at24_platform_data at24c16 = {
	.byte_len  = SZ_16K ,
	.page_size = 16,
};

static struct i2c_board_info __initdata i2c_devices[] = {
	{
		I2C_BOARD_INFO("at24", 0x50),   /* eeprom */
		.platform_data = &at24c16,
	},
	{
		I2C_BOARD_INFO("wm8731", 0x1b), /* audio codec*/
	},
};
#endif  /* #ifdef CONFIG_I2C_BOARDINFO */

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

static struct map_desc plat_io_desc[] __initdata = {
	{
		/* SCU, GIC CPU and TWD */
		.virtual    = PLAT_SCU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SCU_BASE),
		.length     = SZ_8K,
		.type       = MT_DEVICE,
	},
#if defined(CONFIG_CPU_CA9)
	{	/* GIC DIST */
		.virtual    = PLAT_GIC_DIST_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GIC_DIST_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{	/* CPU PERIPHERAL */
		.virtual    = PLAT_CPU_PERIPH_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_CPU_PERIPH_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
#endif
	{
		.virtual    = PLAT_FTINTC030_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTINTC030_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
#ifdef CONFIG_CACHE_L2X0
	{
		.virtual    = PLAT_PL310_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_PL310_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
#endif
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
		.virtual    = PLAT_FTTMR010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTTMR010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual	= PLAT_FTEMC030_VA_BASE,
		.pfn		= __phys_to_pfn(PLAT_FTEMC030_BASE),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTPCIE_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTPCIE_BASE),
		.length     = SZ_16K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTPCIE_LOCAL_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTPCIE_LOCAL_BASE),
		.length     = SZ_16K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWRAP_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWRAP_BASE),
		.length     = SZ_16K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTPCIEAXISLV_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTPCIEAXISLV_BASE),
		.length     = SZ_16K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC020_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC020_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_SERDES_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SERDES_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_SERDES_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SERDES_1_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_SERDES_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SERDES_2_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTNANDC024_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTNANDC024_BASE),
		.length     = SZ_8K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTNANDC024_DATA_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTNANDC024_DATA_BASE),
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
		.virtual    = PLAT_FTLCDC210_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTLCDC210_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC030_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC030_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC030_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC030_1_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTDMAC030_2_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTDMAC030_2_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_GEM_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GEM_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_GEM_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GEM_1_BASE),
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
		.virtual    = PLAT_FTGPIO010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTGPIO010_BASE),
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
		.virtual    = PLAT_FTKBC010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTKBC010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
};

static struct of_device_id faraday_clk_match[] __initconst = {
	{ .compatible = "faraday,a380evb-clk", .data = of_a380_clocks_init, },
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

#ifdef CONFIG_SERDES_MULTI
static void platform_init_serdes(void)
{
	uint32_t val;
	void __iomem *PLAT_FTVSEMI_VA;
	void __iomem *PLAT_SCU_VA;

	PLAT_FTVSEMI_VA = ioremap(PLAT_SERDES_2_BASE, 0x100000);
	PLAT_SCU_VA = ioremap(PLAT_SCU_BASE, 0x10000);

	//controller reset
	//--- controller reset toggle test
	val = readl(PLAT_SCU_VA + 0x00001310);
	val &= ~((0x01<<20) | (0x01<<21));
	//--- assert controller reset
	writel(val,(PLAT_SCU_VA + 0x00001310));

	//POR set
	writel((readl(PLAT_SCU_VA + 0x00000674) & (~0x00800000)),(PLAT_SCU_VA + 0x00000674));

	val = readl(PLAT_SCU_VA + 0x00001310);
	val |= ((0x01<<20) | (0x01<<21));
	//--- assert controller reset
	writel(val,(PLAT_SCU_VA + 0x00001310));
	//--- release IRST_POR_B_A
	writel((readl(PLAT_SCU_VA + 0x00000674) | 0x00800000),(PLAT_SCU_VA + 0x00000674));

	//--- program Vsemi serdes
	serdes_multi_mode_init(PLAT_FTVSEMI_VA, pwrup_1_25gbps , ARRAY_SIZE(pwrup_1_25gbps));
	//--- release IRST_MULTI_HARD_SYNTH_B and IRST_MULTI_HARD_TXRX_Ln
	//--- wait serdes RX ready (RX_READY & RX_STATUS)

#ifndef CONFIG_MACH_A380_VP
	while ((readl(PLAT_SCU_VA + 0x00000600) & 0x00003333) != 0x00003333) {}
#else
	while ((readl(PLAT_SCU_VA + 0x00000600) & 0x00003333) != 0x00001313) {}
#endif
	//--- wait serdes TX status (TX_STATUS)
	while ((readl(PLAT_SCU_VA + 0x00000618) & 0x00000003) != 0x00000003) {}
	//--- wait serdes TX ready (TX_READY)
	while ((readl(PLAT_SCU_VA + 0x00000614) & 0x00003300) != 0x00003300) {}

	//--- controller reset toggle test
	val = readl(PLAT_SCU_VA + 0x00001310);
	val |= (0x01<<20) | (0x01<<21);
	//--- assert controller reset
	writel(val,(PLAT_SCU_VA + 0x00001310));
}
#endif

static void __init platform_sys_timer_init(void)
{
	platform_clock_init();

	timer_probe();
}

static void __init platform_init(void)
{
	iowrite32(0x01E2600B, (void __iomem *)PLAT_SCU_VA_BASE+0x1188); //OTG PHY setting
	iowrite32(0x00000003, (void __iomem *)PLAT_SCU_VA_BASE+0x1144); //SDC Driving setting(4mA)

#ifdef CONFIG_SERDES_MULTI
	platform_init_serdes();
#endif

#ifdef CONFIG_I2C_BOARDINFO
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
#endif

	of_platform_populate(NULL, of_default_bus_match_table, plat_auxdata_lookup, NULL);
}

static inline int is_external_cpu(void)
{
	unsigned int cr;

	cr = ftscu100_readl(FTSCU100_OFFSET_STRAP2);
	return CPU_STRAP_EXTCPU == FTSCU100_STRAP2_CPU_STRAP(cr);
}

void platform_reset(enum reboot_mode mode, const char *cmd)
{
	WDT_REG32(0x0C) = 0;
	WDT_REG32(0x04) = 1000;
	WDT_REG32(0x0C) = 0x03;	// system reset
	WDT_REG32(0x08) = 0x5AB9;
}

static int __init platform_late_init(void)
{
#ifdef CONFIG_CACHE_L2X0
#if 1
	__raw_writel(0x1|(0x1<<4)|(0x1<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x108); // tag RAM latency 1T l2_clk
	__raw_writel(0x2|(0x2<<4)|(0x2<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x10c); // data RAM latency 2T l2_clk
#else
	__raw_writel(0x2|(0x2<<4)|(0x2<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x108); // tag RAM latency 2T l2_clk
	__raw_writel(0x3|(0x3<<4)|(0x3<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x10c); // data RAM latency 3T l2_clk
#endif
	/* 512KB (64KB/way), 8-way associativity, evmon/parity/share enabled
	 * Bits:  .... ...0 0111 0110 0000 .... .... .... */
	l2x0_init((void __iomem *)PLAT_PL310_VA_BASE, 0x30660000, 0xce000fff);
#endif
	return 0;
}
late_initcall(platform_late_init);

static const char *faraday_dt_match[] __initconst = {
	"arm,faraday-soc",
	NULL,
};

DT_MACHINE_START(FARADAY, "A380")
	.atag_offset  = 0x100,
	.dt_compat    = faraday_dt_match,
	.smp          = smp_ops(faraday_smp_ops),
	.map_io       = platform_map_io,
	.init_time    = platform_sys_timer_init,
	.init_machine = platform_init,
	.restart      = platform_reset,
MACHINE_END
