/*
 * Core driver for the Faraday pin controller
 *
 * (C) Copyright 2014 Faraday Technology
 * Yan-Pai Chen <ypchen@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include "../core.h"
#include "pinctrl-ftscu010.h"
#include <mach/hardware.h>
#include <mach/ftscu100.h>

struct ftscu010_pmx {
	struct device *dev;
	struct pinctrl_dev *pctl;
	struct ftscu010_pinctrl_soc_data *soc;
	void __iomem *base;
};

/*
 * Pin group operations
 */
static int ftscu010_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->ngroups;
}

static const char *ftscu010_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
						   unsigned selector)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= pmx->soc->ngroups)
		return NULL;

	return pmx->soc->groups[selector].name;
}

static int ftscu010_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
					   unsigned selector,
					   const unsigned **pins,
					   unsigned *npins)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= pmx->soc->ngroups)
		return -EINVAL;

	*pins = pmx->soc->groups[selector].pins;
	*npins = pmx->soc->groups[selector].npins;
	return 0;
}

#ifdef CONFIG_DEBUG_FS
static void ftscu010_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
				       struct seq_file *s,
				       unsigned offset)
{
	seq_printf(s, " %s", dev_name(pctldev->dev));
}
#endif

static int ftscu010_pinctrl_dt_node_to_group(
		struct ftscu010_pmx *pmx,
		struct device_node *np,
		u32 func)
{
	const __be32 *list;
	int size = 0;
	int i;
	struct ftscu010_pinctrl_soc_data *soc = pmx->soc;
	struct property *function;

	list = of_get_property(np, "scu010,pins", &size);

	if (!list)
		return -ENOMEM;

	size = size / FTSCU010_PIN_MAGIC_NUM_SIZE;
	if(size > FTSCU010_PIN_MAX) {
		dev_err(pmx->dev, "%pOF: over FTSCU010_PIN_MAX\n", np);
		return -ENOMEM;
	}

	soc->groups[func].pins = devm_kzalloc(pmx->dev, size *
				 sizeof(unsigned int), GFP_KERNEL);

	soc->groups[func].npins = size;

	//dev_info(pmx->dev, "Change soc->groups[%d].npins:%d\n",func, soc->groups[func].npins);

	for (i = 0; i < size; i++) {
		soc->groups[func].pins[i] = be32_to_cpu(*list++);
		//dev_info(pmx->dev, "soc->groups[%d].pins[%d]:%d\n",func, i, soc->groups[func].pins[i]);
	}
       function = of_find_property(np, "scu010,schmitt-trigger", NULL)
;
       if (function) {
               soc->groups[func].schmitt_trigger = 1;
       } else {
               soc->groups[func].schmitt_trigger = 0;
       }

	return 0;
}
static int ftscu010_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
					   struct device_node *np,
					   struct pinctrl_map **map,
					   unsigned *num_maps)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);
	struct pinctrl_map *new_map;
	struct property *function;
	int map_num = 1, err;
	u32 func;

	new_map = kmalloc(sizeof(struct pinctrl_map) * map_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = 1;

	function = of_find_property(np, "scu010,function", NULL);
	if (!function) {
		dev_err(pmx->dev, "%pOF: missing scu010,function property\n", np);
		return -EINVAL;
	}

	err = of_property_read_u32(np, "scu010,function", &func);
	if (err) {
		dev_err(pmx->dev, "%pOF: read property value error\n", np);
		return -EINVAL;
	}

	ftscu010_pinctrl_dt_node_to_group(pmx, np, func);

	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = pmx->soc->functions[func].name;
	new_map[0].data.mux.group = NULL;
#if 0
	dev_info(pmx->dev, "maps: function %s(#%d) group %s num %d\n",
		 (*map)->data.mux.function, func, (*map)->data.mux.group, map_num);
#endif
	return 0;
}

static void ftscu010_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
					 struct pinctrl_map *map,
					 unsigned num_maps)
{
	kfree(map);
}

static struct pinctrl_ops ftscu010_pinctrl_ops = {
	.get_groups_count = ftscu010_pinctrl_get_groups_count,
	.get_group_name = ftscu010_pinctrl_get_group_name,
	.get_group_pins = ftscu010_pinctrl_get_group_pins,
#ifdef CONFIG_DEBUG_FS
	.pin_dbg_show = ftscu010_pinctrl_pin_dbg_show,
#endif
	.dt_node_to_map = ftscu010_pinctrl_dt_node_to_map,
	.dt_free_map = ftscu010_pinctrl_dt_free_map,
};

#if defined(CONFIG_MACH_LEO)
#define FTSCU010_PIN_MASK	0x7			/* 3-bit per pin */
#else
#define FTSCU010_PIN_MASK	0x3			/* 2-bit per pin */
#endif
#define FTSCU010_PIN_SHIFT(pin)	((pin) % 16) << 1

/*
 * pinmux operations
 */
static int ftscu010_pinctrl_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->nfunctions;
}

static const char *ftscu010_pinctrl_get_func_name(struct pinctrl_dev *pctldev,
						  unsigned selector)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->functions[selector].name;
}

static int ftscu010_pinctrl_get_func_groups(struct pinctrl_dev *pctldev,
					     unsigned selector,
					     const char * const **groups,
					     unsigned * const ngroups)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	*groups = pmx->soc->functions[selector].groups;
	*ngroups = pmx->soc->functions[selector].ngroups;
	return 0;
}

static unsigned int ftscu010_pinmux_setting(const struct ftscu010_pin *pin,
					    unsigned selector)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pin->functions); ++i) {
		if (pin->functions[i] == selector)
			break;
	}
#ifndef CONFIG_PINCTRL_LEO_MODEX
	WARN_ON(i == ARRAY_SIZE(pin->functions));
#endif
	return i;
}

static void ftscu010_pinctrl_setup(struct pinctrl_dev *pctldev,
				   unsigned selector,
				   unsigned gselector,
				   bool enable)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);
	const struct ftscu010_pinctrl_soc_data *soc = pmx->soc;
	const struct ftscu010_pin_group *group;
	int i, npins;
	unsigned int schmitt_trigger = 0;
#if 0
	dev_info(pmx->dev, "setup: %s function %s(#%d) group %s(#%d)\n",
		enable ? "enable" : "disable",
		soc->functions[selector].name, selector,
		soc->groups[gselector].name, gselector);
#endif
	group = &pmx->soc->groups[gselector];
	npins = group->npins;
	if(group->schmitt_trigger) {
		schmitt_trigger = (1 << 9);
	}


	/* for each pin in the group */
	for (i = 0; i < npins; ++i) {
		unsigned int id = group->pins[i];
		const struct ftscu010_pin *pin = &pmx->soc->map[id];
#ifdef CONFIG_PINCTRL_FTSCU010_GROUP
		int shift = 0;
#else
		int shift = FTSCU010_PIN_SHIFT(id);
#endif
		unsigned int val;

		val = readl(pmx->base + pin->offset);
		val &= ~(FTSCU010_PIN_MASK << shift);
		val |= schmitt_trigger;
		if (enable)
			val |= ftscu010_pinmux_setting(pin, selector) << shift;
		writel(val, pmx->base + pin->offset);
#if 0
		printk("val:%x, addr:%x, shift:%d, selector:%d\n",
			val, (volatile u32 __force *)(pmx->base + pin->offset),
			shift, selector);
#endif
	}

}

static int ftscu010_pinctrl_set_mux(struct pinctrl_dev *pctldev,
				    unsigned selector, unsigned group)
{
	ftscu010_pinctrl_setup(pctldev, selector, group, true);
	return 0;
}

static struct pinmux_ops ftscu010_pinmux_ops = {
	.get_functions_count = ftscu010_pinctrl_get_funcs_count,
	.get_function_name = ftscu010_pinctrl_get_func_name,
	.get_function_groups = ftscu010_pinctrl_get_func_groups,
	.set_mux = ftscu010_pinctrl_set_mux,
};

static struct pinctrl_desc ftscu010_pinctrl_desc = {
	.pctlops = &ftscu010_pinctrl_ops,
	.pmxops = &ftscu010_pinmux_ops,
	.owner = THIS_MODULE,
};

static void ftscu010_pinctrl_dt_node_to_modex(
		struct ftscu010_pmx *pmx,
		struct device_node *np
)
{
	const __be32 *list;
	int size = 0;
	int i;
	unsigned int value = 0;

	list = of_get_property(np, "scu010,modex", &size);

	if (!list)
		return;

	size = size / FTSCU010_PIN_MAGIC_NUM_SIZE;
	if(size > FTSCU010_MODEX_MAX) {
		dev_err(pmx->dev, "%pOF: over FTSCU010_MODEX_MAX\n", np);
		return;
	}

	//value = readl((void __iomem *)PLAT_SCU_VA_BASE + 0x8204);
	for (i = 0; i < size; i++) {
		value |= be32_to_cpu(*list++);
	}
#if 0
	dev_info(pmx->dev, "scu010,modex value:%x\n", value);
#endif
	writel(value, (void __iomem *)PLAT_SCU_VA_BASE + 0x8204);

}

int ftscu010_pinctrl_probe(struct platform_device *pdev,
			   const struct ftscu010_pinctrl_soc_data *data)
{
	struct ftscu010_pmx *pmx;
	struct resource *res;
	struct device_node *dev_np = pdev->dev.of_node;
	
	pmx = devm_kzalloc(&pdev->dev, sizeof(*pmx), GFP_KERNEL);
	if (!pmx) {
		dev_err(&pdev->dev, "Could not alloc ftscu010_pmx\n");
		return -ENOMEM;
	}

	pmx->dev = &pdev->dev;
	pmx->soc = data;

	ftscu010_pinctrl_desc.name = dev_name(&pdev->dev);
	ftscu010_pinctrl_desc.pins = pmx->soc->pins;
	ftscu010_pinctrl_desc.npins = pmx->soc->npins;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0); 
	if (!res)
		return -ENOENT;
	
	pmx->base = devm_ioremap_resource(&pdev->dev, res);
	if (!pmx->base)
		return -ENOMEM;

	pmx->pctl = pinctrl_register(&ftscu010_pinctrl_desc, &pdev->dev, pmx);
	if (!pmx->pctl) {
		dev_err(&pdev->dev, "Could not register FTSCU010 pinmux driver\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, pmx);
	ftscu010_pinctrl_dt_node_to_modex(pmx, dev_np);
	dev_info(&pdev->dev, "Initialized FTSCU010 pinmux driver\n");

	return 0;
}
EXPORT_SYMBOL_GPL(ftscu010_pinctrl_probe);

int ftscu010_pinctrl_remove(struct platform_device *pdev)
{
	struct ftscu010_pmx *pmx = platform_get_drvdata(pdev);

	pinctrl_unregister(pmx->pctl);
	return 0;
}
EXPORT_SYMBOL_GPL(ftscu010_pinctrl_remove);
