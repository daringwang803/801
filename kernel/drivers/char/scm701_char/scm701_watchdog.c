/*
 * scm701_watchdog.c -- support  gpio watchdog
 *
 *  Author			hechaohai
 *  Email   		hech@vangotech.com
 *  Create time 	2020-08-27
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/miscdevice.h> 
#include <linux/delay.h> 
#include <asm/irq.h> 
//#include <mach/hardware.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/mm.h> 
#include <linux/fs.h> 
#include <linux/types.h> 
#include <linux/delay.h> 
#include <linux/moduleparam.h> 
#include <linux/slab.h> 
#include <linux/errno.h> 
#include <linux/ioctl.h> 
#include <linux/cdev.h> 
#include <linux/string.h> 
#include <linux/list.h> 
#include <linux/pci.h> 
#include <linux/gpio.h> 
#include <asm/uaccess.h> 
#include <asm/atomic.h> 
#include <asm/unistd.h> 

#include <linux/version.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
//#include <mach/cputype.h>
//#include <mach/hardware.h>
//#include <mach/mux.h>
#include <asm/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>



#define MOTOR_MAGIC 'L'
#define FEED_DOG 	_IOW(MOTOR_MAGIC, 0,int)



#define DEVICE_NAME				 "scm701_watchdog"
#define STATE_HIGH               1
#define STATE_LOW                0


#define WATCHDOG_ENABLE          STATE_LOW
#define WATCHDOG_DISABLE         STATE_HIGH

#define WATCHDOG_FEED_PIN_INIT   STATE_LOW

static struct watchdog_dev
{
	unsigned char state;			// state: current feeddog gpio state
	unsigned long feeddog_pin;      // feeddog_pin:  feeddog pin number
	unsigned long enable_pin;       // enable_pin:   feeddog enable pin
}*watchdog;

static inline void watchdog_feed(void)
{
	unsigned long feeddog_pin;

	/* toogle the feed dog pin */
	feeddog_pin = watchdog->feeddog_pin;
	watchdog->state = (watchdog->state == STATE_HIGH)?STATE_LOW:STATE_HIGH;
	gpio_set_value(feeddog_pin,watchdog->state);
}


static inline void watchdog_enable(void)
{

	unsigned long wathdog_enable_pin;

	wathdog_enable_pin = watchdog->enable_pin;
	/* we must feed dog once watchdog enable */
	watchdog_feed();
	gpio_set_value(wathdog_enable_pin,WATCHDOG_ENABLE);
}

static inline void watchdog_disable(void)
{

	unsigned long wathdog_enable_pin;

	wathdog_enable_pin = watchdog->enable_pin;
	gpio_set_value(wathdog_enable_pin, WATCHDOG_DISABLE);
}


static int watchdog_open(struct inode *inode,struct file *file)
{
	watchdog_enable();
	printk("watchdog: enable watchdog\n");

	return 0;
}

static int watchdog_close(struct inode *inode,struct file *file)
{
	watchdog_disable();
	printk("watchdog: disable watchdog\n");

	return 0;
}


static long watchdog_ioctl(struct file *file,  unsigned int cmd,  unsigned long arg) 
{
	
	if(cmd == FEED_DOG)
	{
		watchdog_feed();
	}

	return 0;
}


static struct file_operations watchdog_fops = { 
	 .owner = THIS_MODULE, 
	 .open = watchdog_open,
	 .release = watchdog_close,
	 .unlocked_ioctl = watchdog_ioctl, 
}; 

 static struct miscdevice miscwatchdog = 
 {
	 .minor = MISC_DYNAMIC_MINOR,  
 	 .name = DEVICE_NAME, 
 	 .fops = &watchdog_fops, 
 };
 
static int watchdog_probe(struct platform_device *pdev)
{
	int ret;

	watchdog = kmalloc(sizeof(struct watchdog_dev), GFP_KERNEL);
	if(!watchdog)
	{
		printk("watchdog: no memory to malloc\n");
		ret = -ENOMEM;
		goto exit;
	}

	watchdog->enable_pin = of_get_gpio(pdev->dev.of_node, 0);
	watchdog->feeddog_pin = of_get_gpio(pdev->dev.of_node, 1);
	watchdog->state = WATCHDOG_FEED_PIN_INIT;

	printk("watchdog enable_pin:%ld, feeddog_pin:%ld\n", watchdog->enable_pin, watchdog->feeddog_pin);

	ret = gpio_request(watchdog->enable_pin, "watchdog_enable");
	if (ret < 0) 
	{
	      printk("watchdog: ERROR can not open GPIO %ld\n", watchdog->enable_pin);
	      goto exit_kfree;
    }

	ret = gpio_request(watchdog->feeddog_pin, "watchdog_feeddog");
	if (ret < 0) 
	{
	      printk("watchdog: ERROR can not open GPIO %ld\n", watchdog->feeddog_pin);
	      goto exit_kfree;
    }
	
	gpio_direction_output(watchdog->enable_pin, 1); 
	gpio_direction_output(watchdog->feeddog_pin, 1); 
	gpio_set_value(watchdog->feeddog_pin, watchdog->state);

	/* disable wathdog */
	watchdog_disable();
	
	ret = misc_register(&miscwatchdog); 

	printk(" watchdog: misc register successed: \n");
	
	if(ret < 0)
	{
		printk("watchdog: misc register error\n");
		goto exit_kfree;
	}

	return ret;
	
exit_kfree:
	kfree(watchdog);
exit:
	return ret;
}

static int watchdog_remove(struct platform_device *pdev)
{
	unsigned long feeddog_pin = watchdog->feeddog_pin;
	unsigned long wathdog_enable_pin = watchdog->enable_pin;
	
	gpio_free(feeddog_pin);
	gpio_free(wathdog_enable_pin);
	misc_deregister(&miscwatchdog);
	kfree(watchdog);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id scm701_wdt_dt_ids[] = {
	{ .compatible = "scm701-watchdog"},
	{ /* sentinel */ }
}

MODULE_DEVICE_TABLE(of, scm701_wdt_dt_ids);
#endif

static struct platform_driver scm701_wdt_driver = {
	.probe		= watchdog_probe,
	.remove		= watchdog_remove,
	.driver		= {
		.name	= "scm701_wdt",
		.of_match_table = of_match_ptr(scm701_wdt_dt_ids),
	},
};

static int __init watchdog_init(void) 
{
	return platform_driver_register(&scm701_wdt_driver);
}

static void __exit watchdog_exit(void)
{
	platform_driver_unregister(&scm701_wdt_driver);
}

module_init(watchdog_init); 

module_exit(watchdog_exit);

MODULE_DESCRIPTION("Driver for Watchdog");
MODULE_AUTHOR("HECH");
MODULE_LICENSE("GPL");
MODULE_ALIAS("gpio:watchdog");
