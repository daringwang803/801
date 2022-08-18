/*
 * Faraday eFuse Controller
 *
 * (C) Copyright 2019 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <asm/io.h>

static int ftc_efuse_reg_read(void *context,
			unsigned int offset, void *_val, size_t bytes)
{
	void __iomem *base = context;
	u32 *val = _val;
	int i = 0, words = bytes / 4;

	while (words--)
		*val++ = readl(base + offset + (i++ * 4));


	return 0;
}

static int ftc_efuse_reg_write(void *context,
			unsigned int offset, void *_val, size_t bytes)
{
	void __iomem *base = context;
	u32 *val = _val;
	int i = 0, words = bytes / 4;

	//unlock write protect
	writel(0x55, base + 0x1010);
	writel(0xAA, base + 0x1010);
	if(readl(base + 0x1010) == 0x00) {
		printk("Error: eFuse still locking...\n");
		return -EBUSY;
	}

	while (words--) {
		writel(0x40, base + 0x1014);
		writel(*val++, base + offset + (i++ * 4));
	}
	return 0;
}

static int ftc_efuse_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nvmem_config *econfig;
	struct nvmem_device *nvmem;
	struct resource *res;
	void __iomem *base;
	u32 prop;

	econfig = devm_kzalloc(dev, sizeof(*econfig), GFP_KERNEL);
	if (!econfig)
		return -ENOMEM;

	/* find the resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	econfig->name = "ftc-efuse",
	econfig->owner = THIS_MODULE,
	econfig->stride = 4,
	econfig->word_size = 4,
	econfig->reg_read = ftc_efuse_reg_read,
	econfig->reg_write = ftc_efuse_reg_write,
	econfig->dev = &pdev->dev;
	econfig->size = 8;		//64bits
	econfig->priv = base;

	// Overwrite size field of econfig if user defined 'efuse-size' in dts.
	if (of_property_read_u32(dev->of_node, "efuse-size", &prop) == 0)
		econfig->size = prop;

	nvmem = nvmem_register(econfig);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	platform_set_drvdata(pdev, nvmem);

	return 0;
}

static int ftc_efuse_remove(struct platform_device *pdev)
{
	struct nvmem_device *nvmem = platform_get_drvdata(pdev);

	return nvmem_unregister(nvmem);
}

static const struct of_device_id ftc_efuse_dt_match[] = {
	{.compatible = "faraday,ftc-efuse" },
	{},
};
MODULE_DEVICE_TABLE(of, ftssp010_dt_match);

static struct platform_driver ftc_efuse_driver = {
	.probe		= ftc_efuse_probe,
	.remove		= ftc_efuse_remove,
	.driver		= {
		.name = "ftc-efuse",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(ftc_efuse_dt_match),
	},
};

static int __init ftc_efuse_init(void)
{
	return platform_driver_register(&ftc_efuse_driver);
}
module_init(ftc_efuse_init);

static void __exit ftc_efuse_exit(void)
{
	platform_driver_unregister(&ftc_efuse_driver);
}
module_exit(ftc_efuse_exit);

MODULE_DESCRIPTION("Faraday eFuse Controller driver");
MODULE_AUTHOR("Bo-Cun Chen <bcchen@faraday-tech.com>");
MODULE_LICENSE("GPL v2+");
MODULE_ALIAS("platform:ftc_efuse");
