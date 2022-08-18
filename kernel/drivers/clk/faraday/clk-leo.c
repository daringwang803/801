/**
 *  drivers/clk/faraday/clk-leo.c
 *
 *  Faraday LEO Clock Tree
 *
 *  Copyright (C) 2019 Faraday Technology
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
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>

#include <mach/hardware.h>
#include <mach/ftscu100.h>

extern void __iomem *ftscu100_base;

unsigned int leo_clk_get_pll_ns(const char *name)
{
	unsigned int cr;
	unsigned int ns = 0;

	if (!strcmp(name, "pll0")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL0CR);
		ns = FTSCU100_PLL0CR_NS(cr);
#else
		ns = 32;
#endif
	} else if (!strcmp(name, "pll1")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL1CR);
		ns = FTSCU100_PLL1CR_NS(cr);
#else
		ns = 40;
#endif
	} else if (!strcmp(name, "pll2")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL2CR);
		ns = FTSCU100_PLL2CR_NS(cr);
#else
		ns = 24;
#endif
	} else if (!strcmp(name, "pll3")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL3CR);
		ns = FTSCU100_PLL3CR_NS(cr);
#else
		ns = 213;
#endif
	} else if (!strcmp(name, "pll4")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL4CR);
		ns = FTSCU100_PLL4CR_NS(cr);
#else
		ns = 32;
#endif
	} else if (!strcmp(name, "pll5")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL5CR);
		ns = FTSCU100_PLL5CR_NS(cr);
#else
		ns = 198;
#endif
	} else if (!strcmp(name, "pll6")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL6CR);
		ns = FTSCU100_PLL6CR_NS(cr);
#else
		ns = 24;
#endif
	} else if (!strcmp(name, "pll7")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL7CR);
		ns = FTSCU100_PLL7CR_NS(cr);
#else
		ns = 32;
#endif
	}

	return ns;
}

unsigned int leo_clk_get_pll_ms(const char *name)
{
	unsigned int cr;
	unsigned int ms = 0;

	if (!strcmp(name, "pll0")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL0CR);
		ms = FTSCU100_PLL0CR_MS(cr);
#else
		ms = 1;
#endif
	} else if (!strcmp(name, "pll1")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL1CR);
		ms = FTSCU100_PLL1CR_MS(cr);
#else
		ms = 1;
#endif
	} else if (!strcmp(name, "pll2")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL2CR);
		ms = FTSCU100_PLL2CR_MS(cr);
#else
		ms = 1;
#endif
	} else if (!strcmp(name, "pll3")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL3CR);
		ms = FTSCU100_PLL3CR_MS(cr);
#else
		ms = 2;
#endif
	} else if (!strcmp(name, "pll4")) {
		ms = 1;
	} else if (!strcmp(name, "pll5")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL5CR);
		ms = FTSCU100_PLL5CR_MS(cr);
#else
		ms = 2;
#endif
	} else if (!strcmp(name, "pll6")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL6CR);
		ms = FTSCU100_PLL6CR_MS(cr);
#else
		ms = 1;
#endif
	} else if (!strcmp(name, "pll7")) {
#ifndef CONFIG_MACH_LEO_VP
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL7CR);
		ms = FTSCU100_PLL7CR_MS(cr);
#else
		ms = 1;
#endif
	}

	return ms;
}

#ifdef CONFIG_OF
unsigned int leo_clk_get_fastboot_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, (sel != 3) ? 0 : 1);

	if (!strcmp(*parent_name, "rosc0")) {
		div = 1;
	} else {
#ifndef CONFIG_MACH_LEO_VP
		switch (sel) {
			case 1:
				div = 2;
				break;
			case 2:
				div = 4;
				break;
			case 0:
			default:
				div = 1;
				break;
		}
#else
		div = 1;
#endif
	}

	return div;
}

#if 0
unsigned int leo_clk_get_cpu_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, (sel == 4));

#ifndef CONFIG_MACH_LEO_VP
	switch (sel) {
		case 3:
			div = 8;
			break;
		case 2:
			div = 4;
			break;
		case 1:
			div = 2;
			break;
		case 0:
		default:
			div = 1;
			break;
	}
#else
	div = 1;
#endif

	return div;
}
#endif

unsigned int leo_clk_get_ahb_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, (sel == 4));

	if (!strcmp(*parent_name, "fastboot_div2")) {
		div = 1;
	} else {
		div = 5;
	}

	return div;
}

unsigned int leo_clk_get_apb_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, sel);

	if (!strcmp(*parent_name, "fastboot")) {
		div = 2;
	} else {
		div = 6;
	}

	return div;
}

unsigned int leo_clk_get_spiclk_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, sel);

	if (!strcmp(*parent_name, "fastboot")) {
		div = 1;
	} else {
		div = 5;
	}

	div *= 2;

	return div;
}

unsigned int leo_clk_get_sspclk_i2s_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, sel);

	div = 1;

	return div;
}

unsigned int leo_clk_get_sdclk_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, sel);

	div = 2;

	return div;
}

unsigned int leo_clk_get_uart_uclk_div(struct device_node *np, unsigned int sel, const char **parent_name)
{
	unsigned int div;

	*parent_name = of_clk_get_parent_name(np, sel);

	div = 1;

	return div;
}

unsigned int leo_clk_get_div(struct device_node *np, const char **parent_name)
{
	const char *name = np->name;
	unsigned int cr;
	unsigned int sel, div = 1;

	if (!strcmp(name, "fastboot")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_STRAP);
		sel = FTSCU100_STRAP_BOOT_CK_SEL(cr);
		div = leo_clk_get_fastboot_div(np, sel, parent_name);
#if 0
	} else if (!strcmp(name, "cpu")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL0CR);
		sel = FTSCU100_PLL0CR_CLKIN_MUX(cr);
		div = leo_clk_get_cpu_div(np, sel, parent_name);
#endif
	} else if (!strcmp(name, "AHB") || !strcmp(name, "ahb") || !strcmp(name, "hclk")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_PLL0CR);
		sel = FTSCU100_PLL0CR_CLKIN_MUX(cr);
		div = leo_clk_get_ahb_div(np, sel, parent_name);
	} else if (!strcmp(name, "APB") || !strcmp(name, "apb") || !strcmp(name, "pclk")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_PLL6_OSC_SW2(cr);
		div = leo_clk_get_apb_div(np, sel, parent_name);
	} else if (!strcmp(name, "spiclk") || !strcmp(name, "sspclk")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_PERI1_OSC_SW(cr);
		div = leo_clk_get_spiclk_div(np, sel, parent_name);
	} else if (!strcmp(name,"sspclk_i2s")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_SSP_I2S_SEL(cr);
		div = leo_clk_get_sspclk_i2s_div(np, sel, parent_name);
	} else if (!strcmp(name,"sspclk_i2s_1")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_SSP1_I2S_SEL(cr);
		div = leo_clk_get_sspclk_i2s_div(np, sel, parent_name);
	} else if (!strcmp(name,"sdclk")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_EMMC_OSC_SW(cr);
		div = leo_clk_get_sdclk_div(np, sel, parent_name);
	} else if (!strcmp(name,"uart_uclk_30m")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_UART_30M_DIS(cr);
		div = leo_clk_get_uart_uclk_div(np, sel, parent_name);
	} else if (!strcmp(name,"uart_uclk")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_CLKMUX);
		sel = FTSCU100_CLKMUX_UART_FIR_SEL(cr);
		div = leo_clk_get_uart_uclk_div(np, sel, parent_name);
	} else if (!strcmp(name,"gmacclk")) {
		div = 10; 
	}

	return div;
}

static void __init of_leo_faraday_pll_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(np, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-div property\n",
			__func__, np->name);
		return;
	}

	of_property_read_string(np, "clock-output-names", &clk_name);

	mult = leo_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static void __init of_leo_faraday_mux_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	of_property_read_string(np, "clock-output-names", &clk_name);

	mult = 1;

	div = leo_clk_get_div(np, &parent_name);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static unsigned long faraday_clk_composite_divider_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct clk_divider *div = to_clk_divider(hw);
	unsigned long mode, rate;
	unsigned int val;

	val = readl(div->reg);

	mode = (val >> div->shift) & div->width;

	rate = parent_rate >> mode;

	return rate;
}

static const struct clk_ops faraday_clk_composite_divider_ops = {
	.recalc_rate = faraday_clk_composite_divider_recalc_rate,
};

static void __init of_leo_faraday_divider_setup(struct device_node *np)
{
	struct clk *clk;
	struct clk_mux *mux;
	struct clk_divider *div;
	const char *clk_name = np->name;
	const char *parent_name[2];
	u32 reg, shift, mask, width;
	u32 parent_count;

	of_property_read_string(np, "clock-output-names", &clk_name);

	/* if we have a mux, we will have >1 parents */
	parent_count = of_clk_parent_fill(np, parent_name, 2);

	mux = kzalloc(sizeof(struct clk_mux), GFP_KERNEL);
	if (!mux)
		return;

	if (of_property_read_u32(np, "mux-reg", &reg)) {
		pr_err("%s: clock <%s> must have a mux-reg property\n",
			__func__, np->name);
	}

	if (of_property_read_u32(np, "mux-shift", &shift)) {
		pr_err("%s: clock <%s> must have a mux-shift property\n",
			__func__, np->name);
	}

	if (of_property_read_u32(np, "mux-mask", &mask)) {
		pr_err("%s: clock <%s> must have a mux-mask property\n",
			__func__, np->name);
	}

	mux->reg = ftscu100_base + reg;
	mux->shift = shift;
	mux->mask = mask;

	div = kzalloc(sizeof(struct clk_divider), GFP_KERNEL);
	if (!div )
		return;

	if (of_property_read_u32(np, "div-reg", &reg)) {
		pr_err("%s: clock <%s> must have a div-reg property\n",
			__func__, np->name);
	}

	if (of_property_read_u32(np, "div-shift", &shift)) {
		pr_err("%s: clock <%s> must have a div-shift property\n",
			__func__, np->name);
	}

	if (of_property_read_u32(np, "div-width", &width)) {
		pr_err("%s: clock <%s> must have a div-width property\n",
			__func__, np->name);
	}

	div->reg = ftscu100_base + reg;
	div->shift = shift;
	div->width = width;

	clk = clk_register_composite(NULL, clk_name,
					  parent_name, parent_count,
					  &mux->hw, &clk_mux_ops,
					  &div->hw, &faraday_clk_composite_divider_ops,
					  NULL, NULL,
					  CLK_GET_RATE_NOCACHE);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}


static const __initconst struct of_device_id leo_clk_match[] = {
	{ .compatible = "leo,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "leo,rosc0", .data = of_fixed_clk_setup, },
	{ .compatible = "leo,rosc0_div4", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,rosc1", .data = of_fixed_clk_setup, },
	{ .compatible = "leo,audio", .data = of_fixed_clk_setup, },
	{ .compatible = "leo,pll0", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll1", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll1_div4", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll2", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll2_div5", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,pll2_div50", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,pll3", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll4", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll5", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll6", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,pll7", .data = of_leo_faraday_pll_setup, },
	{ .compatible = "leo,fastboot", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,fastboot_div2", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,ahb", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,hclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,apb", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,pclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,cpu", .data = of_leo_faraday_divider_setup, },
	{ .compatible = "leo,ddrmclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,spiclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,sspclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,sspclk_i2s", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,sspclk_i2s_1", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,sdclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,lcclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,gmacclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,uart_uclk_src", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "leo,uart_uclk_30m", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,uart_uclk", .data = of_leo_faraday_mux_setup, },
	{ .compatible = "leo,irda", .data = of_fixed_factor_clk_setup, },
};

void __init of_leo_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long pll0, pll1, pll2, pll3, pll4, pll5, pll6, pll7;
	unsigned long cpuclk, hclk, pclk, mclk;
	unsigned long spiclk, sspclk, sspclk_i2s, sdclk;
	unsigned long lcclk, irdaclk, gmacclk;

	pll0 = pll1 = pll2 = pll3 = pll4 = pll5 = pll6 = pll7 = 0;
	cpuclk = hclk = pclk = mclk = 0;
	spiclk = sspclk = sspclk_i2s = sdclk = 0;
	lcclk = irdaclk = gmacclk = 0;

	of_clk_init(leo_clk_match);

	for (node = of_find_matching_node(NULL, leo_clk_match);
	     node; node = of_find_matching_node(node, leo_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "pll0"))
			pll0 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll1"))
			pll1 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll2"))
			pll2 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll3"))
			pll3 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll4"))
			pll4 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll5"))
			pll5 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll6"))
			pll6 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll7"))
			pll7 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "hclk"))
			hclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "spiclk"))
			spiclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sspclk"))
			sspclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sspclk_i2s"))
			sspclk_i2s = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sdclk"))
			sdclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "lcclk"))
			lcclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "irda"))
			irdaclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "gmacclk"))
			gmacclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "PLL0: %4ld MHz, PLL1 MCLK: %4ld MHz, PLL2: %4ld MHz, PLL3: %4ld MHz\n",
	       pll0/1000/1000, pll1/1000/1000, pll2/1000/1000, pll3/1000/1000);
	printk(KERN_INFO "PLL4: %4ld MHz, PLL5 MCLK: %4ld MHz, PLL6: %4ld MHz, PLL7: %4ld MHz\n",
	       pll4/1000/1000, pll5/1000/1000, pll6/1000/1000, pll7/1000/1000);
	printk(KERN_INFO "CPU: %ld MHz, DDR MCLK: %ld MHz, HCLK: %ld MHz, PCLK: %ld MHz\n",
	       cpuclk/1000/1000, mclk/1000/1000, hclk/1000/1000, pclk/1000/1000);
	printk(KERN_INFO "SPI CLK: %ld MHz, SSP CLK: %ldMHz, SSP_I2S CLK: %ldMHz, SD CLK: %ldMHz\n",
	       spiclk/1000/1000, sspclk/1000/1000, sspclk_i2s/1000/1000, sdclk/1000/1000);
	printk(KERN_INFO "LC CLK: %ldMHz, IRDA: %ldMHz , GMAC_REF: %ldMhz\n",
	       lcclk/1000/1000, irdaclk/1000/1000,gmacclk/1000/1000);
}
#endif
