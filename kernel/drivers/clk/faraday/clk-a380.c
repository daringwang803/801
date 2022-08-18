/**
 *  drivers/clk/faraday/clk-a380.c
 *
 *  Faraday A380 Evaluation Board Clock Tree
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2015 Bing-Yao Luo <bjluo@faraday-tech.com>
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

#include <mach/ftscu100.h>

struct a380_clk {
	char *name;
	char *parent;
	unsigned long rate;
	unsigned int mult;
	unsigned int div;
};

static struct a380_clk a380_fixed_rate_clk[] = {
	{
		.name = "osc0",
		.rate = 25000000,
	}, {
		.name = "osc1",
		.rate = 33000000,
	}, {
		.name = "osc2",
		.rate = 27000000,
	}, {
		.name = "osc3",
		.rate = 24000000,
	}, {
		.name = "osc4",
		.rate = 33000000,
	}, {
		.name = "osc5",
		.rate = 25000000,
	},
};

static struct a380_clk a380_pll_clk[] = {
	{
		.name = "pll0",
		.parent = "osc0",
		.div = 2,
	}, {
		.name = "pll1",
		.parent = "pll0",
		.div = 1,
	}, {
		.name = "pll2",
		.parent = "osc1",
		.div = 2,
	}, {
		.name = "pll3",
		.parent = "osc2",
		.div = 2,
	}, {
		.name = "pll4",
		.parent = "osc3",
		.div = 1,
	}, {
		.name = "pll5",
		.parent = "osc4",
		.div = 1,
	}, {
		.name = "pll6",
		.parent = "osc5",
		.div = 1,
	},
};

static struct a380_clk a380_fixed_factor_clk[] = {
	{
		.name = "aclk0",
		.parent = "pll4",
		.mult = 1,
		.div = 2,
	}, {
		.name = "aclk1",
		.parent = "pll3",
		.mult = 1,
		.div = 1,
	}, {
		.name = "aclk2",
		.parent = "pll4",
		.mult = 1,
		.div = 2,
	}, {
		.name = "aclk3",
		.parent = "pll2",
		.mult = 1,
		.div = 1,
	}, {
		.name = "AHB",
		.parent = "pll5",
		.mult = 1,
		.div = 4,
	}, {
		.name = "ahb",
		.parent = "pll5",
		.mult = 1,
		.div = 4,
	}, {
		.name = "hclk",
		.parent = "pll5",
		.mult = 1,
		.div = 4,
	}, {
		.name = "APB",
		.parent = "pll5",
		.mult = 1,
		.div = 8,
	}, {
		.name = "apb",
		.parent = "pll5",
		.mult = 1,
		.div = 8,
	}, {
		.name = "pclk",
		.parent = "pll5",
		.mult = 1,
		.div = 8,
	}, {
		.name = "mcpu",
		.parent = "pll1",
		.mult = 1,
		.div = 1,
	}, {
		.name = "scpu",
		.parent = "pll4",
		.mult = 1,
		.div = 1,
	}, {
		.name = "ddrmclk",
		.parent = "pll2",
		.mult = 1,
		.div = 1,
	}, {
		.name = "sdclk",
		.parent = "pll5",
		.mult = 1,
		.div = 8,
	}, {
		.name = "spiclk",
		.parent = "pll4",
		.mult = 1,
		.div = 2,
	},
};

#define FTSCU100_PLL3_NS_FIXED	20
#define FTSCU100_PLL5_NS_FIXED	24
#define FTSCU100_PLL6_NS_FIXED	20

unsigned int a380_clk_get_pll_ns(const char *name)
{
	unsigned int cr;
	unsigned int ns = 0;

	if (!strcmp(name, "pll0")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_STRAP2);
		ns = FTSCU100_STRAP2_PLL0_NS(cr);
	} else if (!strcmp(name, "pll1")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_STRAP2);
		ns = FTSCU100_STRAP2_PLL1_NS(cr);
	} else if (!strcmp(name, "pll2")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_STRAP2);
		ns = FTSCU100_STRAP2_PLL2_NS(cr);
	} else if (!strcmp(name, "pll3")) {
		ns = FTSCU100_PLL3_NS_FIXED;
	} else if (!strcmp(name, "pll4")) {
		cr = ftscu100_readl(FTSCU100_OFFSET_STRAP2);
		ns = FTSCU100_STRAP2_PLL4_NS(cr);
	} else if (!strcmp(name, "pll5")) {
		ns = FTSCU100_PLL5_NS_FIXED;
	} else if (!strcmp(name, "pll6")) {
		ns = FTSCU100_PLL6_NS_FIXED;
	}

	return ns;
}

int __init a380_clocks_init(void)
{
	struct clk *clk;
	const char *name, *parent;
	unsigned int i, mult, div;
	unsigned long cpuclk, aclk, hclk, pclk, mclk;

	for (i = 0; i < ARRAY_SIZE(a380_fixed_rate_clk); i++) {
		name = a380_fixed_rate_clk[i].name;
		clk = clk_register_fixed_rate(NULL, name, NULL,
				0, a380_fixed_rate_clk[i].rate);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

	for (i = 0; i < ARRAY_SIZE(a380_pll_clk); i++) {
		name = a380_pll_clk[i].name;
		parent = a380_pll_clk[i].parent;

		mult = a380_clk_get_pll_ns(name);
		div = a380_pll_clk[i].div;
		clk = clk_register_fixed_factor(NULL, name,
				parent, 0, mult, div);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

	for (i = 0; i < ARRAY_SIZE(a380_fixed_factor_clk); i++) {
		name = a380_fixed_factor_clk[i].name;
		parent = a380_fixed_factor_clk[i].parent;

		mult = a380_fixed_factor_clk[i].mult;
		div = a380_fixed_factor_clk[i].div;
		clk = clk_register_fixed_factor(NULL, name,
				parent, 0, mult, div);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

#ifdef CONFIG_CPU_CA9
	clk = clk_get(NULL, "mcpu");
	cpuclk = clk_get_rate(clk);
#else
	clk = clk_get(NULL, "scpu");
	cpuclk = clk_get_rate(clk);
#endif
	clk = clk_get(NULL, "aclk0");
	aclk = clk_get_rate(clk);

	clk = clk_get(NULL, "hclk");
	hclk = clk_get_rate(clk);

	clk = clk_get(NULL, "pclk");
	pclk = clk_get_rate(clk);

	clk = clk_get(NULL, "ddrmclk");
	mclk = clk_get_rate(clk);

	printk(KERN_INFO "CPU: %ld, DDR MCLK: %ld\nACLK0: %ld, HCLK: %ld, PCLK: %ld\n",
	       cpuclk, mclk, aclk, hclk, pclk);

	return 0;
}

#ifdef CONFIG_OF
static void __init of_a380_faraday_pll_setup(struct device_node *np)
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

	mult = a380_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id a380_clk_match[] = {
	{ .compatible = "a380evb,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,osc1", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,osc2", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,osc3", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,osc4", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,osc5", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,twdclk", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,pll0", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll1", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll2", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll3", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll4", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll5", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,pll6", .data = of_a380_faraday_pll_setup, },
	{ .compatible = "a380evb,aclk0", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,aclk1", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,aclk2", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,aclk3", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,ahb", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,hclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,apb", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,pclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,mcpu", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,scpu", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,ddrmclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,sdclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,spiclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,ssp0clk", .data = of_fixed_clk_setup, },
	{ .compatible = "a380evb,ssp1clk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a380evb,ssp2clk", .data = of_fixed_factor_clk_setup, },
};

void __init of_a380_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long cpuclk, aclk, hclk, pclk, mclk;

	cpuclk = 0; aclk = 0; hclk = 0; pclk = 0; mclk = 0;

	of_clk_init(a380_clk_match);

	for (node = of_find_matching_node(NULL, a380_clk_match);
	     node; node = of_find_matching_node(node, a380_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

#ifdef CONFIG_CPU_CA9
		if (!strcmp(__clk_get_name(clk), "mcpu"))
			cpuclk = clk_get_rate(clk);
#else
		if (!strcmp(__clk_get_name(clk), "scpu"))
			cpuclk = clk_get_rate(clk);
#endif
		else if (!strcmp(__clk_get_name(clk), "aclk0"))
			aclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "hclk"))
			hclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "CPU: %ld, DDR MCLK: %ld\nACLK0: %ld, HCLK: %ld, PCLK: %ld\n",
	       cpuclk, mclk, aclk, hclk, pclk);
}
#endif
