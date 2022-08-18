/*
 * Faraday FTSDC021 controller driver.
 *
 * Copyright (c) 2012 Faraday Technology Corporation.
 *
 * Authors: Bing-Jiun, Luo <bjluo@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <plat/sd.h>

#include "sdhci-pltfm.h"

#define  DRIVER_NAME "ftsdc021"

#ifdef CONFIG_MACH_LEO
#define EMMC_EMBEDDED
#else
//#define EMMC_EMBEDDED
#endif
//#define ENABLE_8_BIT_BUS_WIDTH
//#define FORCE_1_BIT_DATA
//#define Transfer_Mode_PIO
//#define Transfer_Mode_SDMA
//#define NO_1_8_V 1

#define SDHCI_FTSDC021_VENDOR0		0x100
#define	SDHCI_PULSE_LATCH_OFF_MASK	0x3F00
#define	SDHCI_PULSE_LATCH_OFF_SHIFT	8
#define	SDHCI_PULSE_LATCH_EN		0x1

#define SDHCI_FTSDC021_VENDOR1		0x104
#define	SDHCI_FTSDC021_RST_N_FUNCTION	8

#define FTSDC021_BASE_CLOCK  100000000
unsigned int pulse_latch_offset = 1;

static unsigned int ftsdc021_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	return pltfm_host->clock;
}

static void ftsdc021_set_clock(struct sdhci_host *host, unsigned int clock)
{
	int vendor0;

	sdhci_set_clock(host, clock);

	vendor0 = sdhci_readl(host, SDHCI_FTSDC021_VENDOR0);
	if (host->mmc->actual_clock < 100000000)
		/* Enable pulse latch */
		vendor0 |= SDHCI_PULSE_LATCH_EN;
	else
		vendor0 &= ~(SDHCI_PULSE_LATCH_OFF_MASK | SDHCI_PULSE_LATCH_EN);

	vendor0 &= ~(0x3f << SDHCI_PULSE_LATCH_OFF_SHIFT);
	vendor0 |= (pulse_latch_offset << SDHCI_PULSE_LATCH_OFF_SHIFT);
	sdhci_writel(host, vendor0, SDHCI_FTSDC021_VENDOR0);
}

static unsigned int ftsdc021_get_timeout_clk(struct sdhci_host *host)
{
	return 33;
}

static void ftsdc021_hw_reset(struct sdhci_host *host)
{
	unsigned int value;

	value = sdhci_readl(host, SDHCI_FTSDC021_VENDOR1);

	if ((value & SDHCI_FTSDC021_RST_N_FUNCTION) == 0)
		sdhci_writel(host, value | SDHCI_FTSDC021_RST_N_FUNCTION, SDHCI_FTSDC021_VENDOR1);
}

void ftsdc021_wait_dll_lock(void)
{
	platform_sdhci_wait_dll_lock();
}
EXPORT_SYMBOL(ftsdc021_wait_dll_lock);

void ftsdc021_reset_dll(void)
{
#if 0
	int timeout = 1000;
	void __iomem *gpio_va;

	gpio_va = ioremap(0xa8500000, 0x10000);

	while((readl(gpio_va + 0x4) & 0x4) == 0x4) {
		if (timeout) {
			udelay(1);
			timeout--;
		}
		else {
			printk("Wait DLL timeout when reset DLL\n");
			iounmap(gpio_va);
			return;
		}
	}

	writel((readl(gpio_va + 0x8) | 0x10), gpio_va + 0x8);
	//reset DLL
	writel((readl(gpio_va) | 0x10), gpio_va);
	udelay(1);; //need some delay
	writel((readl(gpio_va) & 0xffffffef), gpio_va);
	//wait reset ok
	timeout = 1000;
	while((readl(gpio_va + 0x4) & 0x8) == 0x8) {
		if (timeout) {
			udelay(1);
			timeout--;
		}
		else {
			printk("Reset DLL timeout\n");
			break;
		}
	}

	iounmap(gpio_va);
#endif	
}
EXPORT_SYMBOL(ftsdc021_reset_dll);

static struct sdhci_ops ftsdc021_ops = {
	.set_clock = ftsdc021_set_clock,
	.get_max_clock = ftsdc021_get_max_clk,
	.get_timeout_clock = ftsdc021_get_timeout_clk,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.hw_reset = ftsdc021_hw_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
};

static struct sdhci_pltfm_data ftsdc021_pdata = {
	.ops = &ftsdc021_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
                  SDHCI_QUIRK_32BIT_ADMA_SIZE |
		  SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};

static const struct of_device_id sdhci_ftsdc021_of_match_table[] = {
	{ .compatible = "faraday,ftsdc021-sdhci", .data = &ftsdc021_pdata},
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_ftsdc021_of_match_table);

static int ftsdc021_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	const struct sdhci_pltfm_data *soc_data;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host = NULL;
	int ret = 0;
	struct clk *clk;
	struct device_node *np = pdev->dev.of_node;
	const __be32 *pulse;

	pr_info("ftsdc021: probe\n");

	match = of_match_device(sdhci_ftsdc021_of_match_table, &pdev->dev);

	if (match)
		soc_data = match->data;
	else
		soc_data = &ftsdc021_pdata;

	host = sdhci_pltfm_init(pdev, soc_data, 0);
	if (IS_ERR(host))
		return PTR_ERR(host);
	pltfm_host = sdhci_priv(host);

	/* Get sd clock (sdclk1x for FTSDC021) */
	clk = clk_get(&pdev->dev, "sdclk");
	if (!IS_ERR(clk)) {
		pltfm_host->clk = clk;
		clk_prepare_enable(clk);
		pltfm_host->clock = clk_get_rate(pltfm_host->clk);
	} else {
		pltfm_host->clock = FTSDC021_BASE_CLOCK;
	}

	sdhci_get_of_property(pdev);

	if (!match) {
#ifdef EMMC_EMBEDDED
		host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
#endif
	/* Set transfer mode, default use ADMA */
#if defined(Transfer_Mode_PIO)
		host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
		host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
#elif defined(Transfer_Mode_SDMA)
		host->quirks |= SDHCI_QUIRK_FORCE_DMA;
		host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
#endif
#ifdef FORCE_1_BIT_DATA
		host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;
#endif
#ifdef ENABLE_8_BIT_BUS_WIDTH
		host->mmc->caps |= MMC_CAP_8_BIT_DATA;
#endif
	/* If not support 1.8v signal, set this*/
#ifdef NO_1_8_V
		host->quirks2 |= SDHCI_QUIRK2_NO_1_8_V;
#endif
	}
	else {
		/* Get default pulse latch offset */
		pulse = of_get_property(np, "pulse-latch", NULL);
		if (pulse)
			pulse_latch_offset = be32_to_cpup(pulse);
		/* Enable 8 bit data bus width. */
		if (of_get_property(np, "enable-8-bits", NULL))
			host->mmc->caps |= MMC_CAP_8_BIT_DATA;
	}
	host->mmc->caps |= MMC_CAP_HW_RESET;
#ifdef EMMC_EMBEDDED
	host->mmc->caps |= MMC_CAP_NONREMOVABLE;
#endif
	ret = sdhci_add_host(host);
	if (ret)
		sdhci_pltfm_free(pdev);

	return ret;
}

static int ftsdc021_remove(struct platform_device *pdev)
{
	return sdhci_pltfm_unregister(pdev);
}

static struct platform_driver ftsdc021_driver = {
	.driver		= {
		.name	= "ftsdc021",
		.owner	= THIS_MODULE,
		.of_match_table = sdhci_ftsdc021_of_match_table,
		.pm     = &sdhci_pltfm_pmops,
	},
	.probe		= ftsdc021_probe,
	.remove		= ftsdc021_remove,
};

module_platform_driver(ftsdc021_driver);

MODULE_DESCRIPTION("SDHCI driver for FTSDC021");
MODULE_AUTHOR("Bing-Yao, Luo <bjluo@faraday-tech.com>");
MODULE_LICENSE("GPL v2");
