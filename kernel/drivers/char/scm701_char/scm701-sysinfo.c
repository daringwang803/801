/*
 * Based on drivers/char/scm701-sysinfo.c
 *
 * Copyright (C) 2020 vango Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
//#include <linux/scm701-sid.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/ftscu100.h>

#include "scm701-sysinfo-user.h"

#define SYSINFO_OFFSET				0x11
#define SYSINFO_SIZE				60

#define ESIGNATURE_VERSION_OFFSET	0x00
#define COMPANY_NAME_OFFSET			0x04
#define PRODUCT_VERSION_OFFSET		0x10
#define CHIPID_OFFSET				0x14
#define PRODUCT_MODEL_OFFSET		0x20
#define MANUFACTURE_DATA_OFFSET 	0x30
#define CHECKSUM_OFFSET 			0x38

#define ESIGNATURE_VERSION_SIZE		4
#define COMPANY_NAME_SIZE			12
#define PRODUCT_VERSION_SIZE		4
#define CHIPID_SIZE					12
#define PRODUCT_MODEL_SIZE			16
#define MANUFACTURE_DATA_SIZE 		8
#define CHECKSUM_SIZE 				4

static int scm701_get_efuse_value(unsigned int *buf, int offset, int len)
{
	int i;

	writel(0x55, (void __iomem *)PLAT_EFUSE_VA_BASE + 0x1010);
	writel(0xAA, (void __iomem *)PLAT_EFUSE_VA_BASE + 0x1010);
	
	for(i = 0 ; i < len; i+=4) {
		*(buf + i/4) = readl((void __iomem *)PLAT_EFUSE_VA_BASE + \
			SYSINFO_OFFSET + offset + i);
	}

	return 0;
}

static int soc_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int soc_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long soc_info_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{
	int i, ret = 0;
	unsigned int sum;
	unsigned int databuf[SYSINFO_SIZE/4] = {0};

	pr_debug("IOCTRL cmd: %#x, param: %#lx\n", ioctl_num, ioctl_param);
	switch (ioctl_num) {
	case CHECK_SOC_SYSINFO:
		ret = scm701_get_efuse_value(databuf, 0, SYSINFO_SIZE);
		if(0 == ret) {
			sum = 0;
			for (i = 0; i < SYSINFO_SIZE/4; i++) {
				sum += databuf[i];
			}
			if(0xffffffff == sum) {
				ret = copy_to_user((void __user *)ioctl_param, 
					databuf, SYSINFO_SIZE);
			}
			else {
				ret = -1;
				printk("Error: scm701 sysinfo checksum failed!\n");
			}
		}
		break;
	default:
		pr_err("Unsupported cmd:%d\n", ioctl_num);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations soc_info_ops = {
	.owner   = THIS_MODULE,
	.open    = soc_info_open,
	.release = soc_info_release,
	.unlocked_ioctl = soc_info_ioctl,
};

struct miscdevice soc_info_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "scm701_soc_info",
	.fops  = &soc_info_ops,
};

//static ssize_t sys_info_show(struct class *class,
//			     struct class_attribute *attr, char *buf)
static ssize_t sys_info_show(struct device *cdev,
				 struct device_attribute *attr,
				 char *buf)
{
	int i, j, ret, offset_start, offset_end;
	unsigned int databuf[SYSINFO_SIZE/4] = {0};
	unsigned int sum;
	char tmpbuf[129] = {0};
	size_t size = 0;

	ret = scm701_get_efuse_value(databuf, 0, SYSINFO_SIZE);
	if(0 == ret) {
		sum = 0;
		for (i = 0; i < SYSINFO_SIZE/4; i++) {
			sum += databuf[i];
		}
		if(0xffffffff != sum) {
			printk("Error: scm701 sysinfo checksum failed!\n");
			goto show_out;
		}

		printk("sizeof(databuf):%d, SYSINFO_SIZE/4:%d\n", 
			sizeof(databuf), SYSINFO_SIZE/4);

	}

	/* e-signature version */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = ESIGNATURE_VERSION_OFFSET/4;
	offset_end   = ESIGNATURE_VERSION_OFFSET/4 + ESIGNATURE_VERSION_SIZE/4;
	//scm701_get_efuse_value(databuf, ESIGNATURE_VERSION_OFFSET, ESIGNATURE_VERSION_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + i*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_e-signature_version    : %s\n", tmpbuf);

	/* Company name  */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = COMPANY_NAME_OFFSET/4;
	offset_end   = COMPANY_NAME_OFFSET/4 + COMPANY_NAME_SIZE/4;
	//scm701_get_efuse_value(databuf, COMPANY_NAME_OFFSET, COMPANY_NAME_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + j*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_company                : %s\n", tmpbuf);

	/* Product version */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = PRODUCT_VERSION_OFFSET/4;
	offset_end   = PRODUCT_VERSION_OFFSET/4 + PRODUCT_VERSION_SIZE/4;
	//scm701_get_efuse_value(databuf, PRODUCT_VERSION_OFFSET, PRODUCT_VERSION_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + j*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_product_version        : %s\n", tmpbuf);

	/* chipid */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = CHIPID_OFFSET/4;
	offset_end   = CHIPID_OFFSET/4 + CHIPID_SIZE/4;
	//scm701_get_efuse_value(databuf, CHIPID_OFFSET, CHIPID_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + j*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_chipid                 : %s\n", tmpbuf);

	/* Product model */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = PRODUCT_MODEL_OFFSET/4;
	offset_end   = PRODUCT_MODEL_OFFSET/4 + PRODUCT_MODEL_SIZE/4;
	//scm701_get_efuse_value(databuf, PRODUCT_MODEL_OFFSET, PRODUCT_MODEL_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + j*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_product_model          : %s\n", tmpbuf);

	/* Manufacture data */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = MANUFACTURE_DATA_OFFSET/4;
	offset_end   = MANUFACTURE_DATA_OFFSET/4 + MANUFACTURE_DATA_SIZE/4;
	//scm701_get_efuse_value(databuf, MANUFACTURE_DATA_OFFSET, MANUFACTURE_DATA_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + j*8, "%08x", databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_manufacture_data       : %s\n", tmpbuf);

	/* checksum */
	memset(tmpbuf, 0, sizeof(tmpbuf));
	offset_start = CHECKSUM_OFFSET/4;
	offset_end   = CHECKSUM_OFFSET/4 + CHECKSUM_SIZE/4;
	//scm701_get_efuse_value(databuf, CHECKSUM_OFFSET, CHECKSUM_SIZE);
	for (j = 0,i = offset_start; i < offset_end; i++) {
		sprintf(tmpbuf + i*8, "%08x", databuf[i]);
		printk("*** databuf[%d]:0x%x\n", i, databuf[i]);
		j++;
	}
	size += sprintf(buf + size, "scm701_checksum               : %s\n", tmpbuf);

show_out:
	return size;
}


#if 0
static struct class_attribute info_class_attrs[] = {
	__ATTR(sys_info, 0644, sys_info_show, NULL),
	__ATTR_NULL,
};
#else
static DEVICE_ATTR_RO(sys_info);

static struct attribute *info_class_attrs[] = {
	&dev_attr_sys_info.attr,
	NULL,
};
ATTRIBUTE_GROUPS(info_class);
#endif

static struct class info_class = {
	.name           = "scm701_info",
	.owner          = THIS_MODULE,
	//.class_attrs    = info_class_attrs,
	.class_groups     = info_class_groups,
};

static int __init scm701_sys_info_init(void)
{
	s32 ret = 0;

	//writel(0x55, (void __iomem *)PLAT_EFUSE_VA_BASE + 0x1010);
	//writel(0xAA, (void __iomem *)PLAT_EFUSE_VA_BASE + 0x1010);
	//writel(0x55, PLAT_EFUSE_VA_BASE + 0x1010);

	ret = class_register(&info_class);
	if (ret != 0)
		return ret;

	ret = misc_register(&soc_info_device);
	if (ret != 0) {
		pr_err("%s: misc_register() failed!(%d)\n", __func__, ret);
		class_unregister(&info_class);
		return ret;
	}
	return ret;
}

static void __exit scm701_sys_info_exit(void)
{
	misc_deregister(&soc_info_device);
	class_unregister(&info_class);
}

module_init(scm701_sys_info_init);
module_exit(scm701_sys_info_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("hechaohai<hech@vangotech.com>");
MODULE_DESCRIPTION("scm701 sys info.");
