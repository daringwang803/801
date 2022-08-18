#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>

#include <linux/acpi.h>
#include <linux/bcd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/spi/spi.h>





static const struct of_device_id vango_spi_dt_ids[] = {
	{ .compatible = "spi-vango", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, vango_spi_dt_ids);

static int vango_spi_probe(struct spi_device *spi)
{
	int id, ret;

	printk("******** vango_spi_probe\n");
	if(spi->dev.of_node) {
		printk("********* vango 25spi->dev.of_node\n");
	}
	else {
		printk("********* vango no spi->dev.of_node\n");
	}
	ret = of_property_read_u32(spi->dev.of_node, "tmp-num", &id);
	if (0 == ret) {
		printk("************* vango tmp-num:%d\n", id);
	}
	else
	{
		printk("************* vango tmp-num error:%d\n", ret);
	}
	return 0;
}

static int vango_spi_remove(struct spi_device *spi)
{
	
	printk("******** vango_spi_remove\n");
	return 0;
}



static struct spi_driver vango_spi_driver = {
	.driver = {
		.name = "vango-spi",
		.owner = THIS_MODULE,
		.of_match_table = vango_spi_dt_ids,
	},
	.probe 		= vango_spi_probe,
	.remove 	= vango_spi_remove,
};

module_spi_driver(vango_spi_driver);


MODULE_DESCRIPTION("vango spi driver");
MODULE_AUTHOR("hech<hechchaohai@gmail.com>");
MODULE_LICENSE("GPL");


