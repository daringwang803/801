#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/delay.h>

struct scm801_rtc {
	struct rtc_device	*rtc_dev;
	void __iomem		*base;
	struct resource 	*res;
};

#define SCM801_SCU_OSC_CTRL_REG		0x01C
#define SCM801_RTC_TIME1_REG     	0x200
#define SCM801_RTC_TIME2_REG		0x204
#define SCM801_RTC_ALM1_REG		0x208
#define SCM801_RTC_ALM2_REG		0x20C
#define SCM801_RTC_CTRL_REG		0x210
#define SCM801_RTC_TRIM			0x214
#define SCM801_RTC_BCD_TIME1_REG        0x220
#define SCM801_RTC_BCD_TIME2_REG        0x224
#define SCM801_RTC_BCD_ALM1_REG         0x228
#define SCM801_RTC_BCD_ALM2_REG         0x22C



static int scm801_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	//unsigned int  year,month,days, hour, min, sec;
	struct scm801_rtc *rtc = dev_get_drvdata(dev);

	/*
		sec = readl(rtc->base+SCM801_RTC_TIME1_REG) & 0x7f;
		min = (readl(rtc->base+SCM801_RTC_TIME1_REG)>>8) & 0x7f;
		hour = (readl(rtc->base+SCM801_RTC_TIME1_REG)>>16) & 0x3f;
		days =  (readl(rtc->base+SCM801_RTC_TIME2_REG)) & 0x3f;
		month = (readl(rtc->base+SCM801_RTC_TIME1_REG)>>8) & 0x1f;
		year = (readl(rtc->base+SCM801_RTC_TIME1_REG)>>16) & 0xff;
		printk(KERN_INFO"year: %d %x,month:%d %x,days:%d %x,hour:%d %x,min:%d %x,sec:%d %x\n",
			year,year,month,month,days,days,hour,hour,min,min,sec,sec);
	*/
	tm->tm_sec = readl(rtc->base+SCM801_RTC_TIME1_REG) & 0x7f;
	tm->tm_min = (readl(rtc->base+SCM801_RTC_TIME1_REG)>>8) & 0x7f;
	tm->tm_hour =  (readl(rtc->base+SCM801_RTC_TIME1_REG)>>16) & 0x3f;
        tm->tm_mday =  (readl(rtc->base+SCM801_RTC_TIME2_REG)) & 0x3f;
        tm->tm_mon = (readl(rtc->base+SCM801_RTC_TIME2_REG)>>8) & 0x1f;
        tm->tm_year = (readl(rtc->base+SCM801_RTC_TIME2_REG)>>16) & 0xff;
	return 0;
}

static int scm801_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int ctrl,time,date;
	struct scm801_rtc *rtc = dev_get_drvdata(dev);

	ctrl = readl(rtc->base+SCM801_RTC_CTRL_REG);
	ctrl &= ~(1<<0);
	writel(ctrl,rtc->base+SCM801_RTC_CTRL_REG);
	do{
		ctrl = readl(rtc->base+SCM801_RTC_CTRL_REG);
	}while((ctrl >> 8) & 0x01);

	//printk(KERN_INFO"tm->tm_wday:%d\n",tm->tm_wday);
		
	time =((tm->tm_wday) << 24) | ((tm->tm_hour) << 16) | ((tm->tm_min) << 8) | (tm->tm_sec);
	writel(time,rtc->base+SCM801_RTC_TIME1_REG);

	date =((tm->tm_year) << 16) | ((tm->tm_mon) << 8) | (tm->tm_mday);
	writel(date,rtc->base+SCM801_RTC_TIME2_REG);

	ctrl = readl(rtc->base+SCM801_RTC_CTRL_REG);
        ctrl |= (1<<0);
        writel(ctrl,rtc->base+SCM801_RTC_CTRL_REG);
	do{
                ctrl = readl(rtc->base+SCM801_RTC_CTRL_REG);
        }while(!((ctrl>>8) & 0x01));


	return 0;
}


static void scm801_rtc_setup(struct device *dev)
{
	unsigned int tmp;
	struct scm801_rtc *rtc = dev_get_drvdata(dev);
	tmp = readl(rtc->base+SCM801_SCU_OSC_CTRL_REG);
	if(readl(rtc->base+SCM801_SCU_OSC_CTRL_REG) && 0x01) {
		if(readl(rtc->base+SCM801_SCU_OSC_CTRL_REG) && 0x08) {
			/*do{
				tmp = readl(rtc->base+SCM801_RTC_CTRL_REG);
			}while(!((tmp>>11) & 0x01));*/
			tmp = readl(rtc->base+SCM801_RTC_CTRL_REG);
			tmp |= (1<<0);
			writel(tmp,rtc->base+SCM801_RTC_CTRL_REG);
			do{
		                tmp = readl(rtc->base+SCM801_RTC_CTRL_REG);
        		}while(!((tmp>>8) & 0x01));

		}
	}
}


static const struct rtc_class_ops scm801_rtc_ops = {
	.read_time     = scm801_rtc_read_time,
	.set_time      = scm801_rtc_set_time,
};


static int scm801_rtc_probe(struct platform_device * platform_dev)
{
	int ret;
	struct scm801_rtc *rtc;
	
	rtc = kzalloc(sizeof(*rtc),GFP_KERNEL);
	if(!rtc) {
		dev_err(&platform_dev->dev,"alloc memory rtc failed.\n");
		return -ENOMEM;
	}
	
	rtc->res = platform_get_resource(platform_dev, IORESOURCE_MEM, 0);
	if (!rtc->res) {
		dev_err(&platform_dev->dev,"get resource failed.\n");
		ret = -EINVAL;
		//return ret;
		goto res_err;
	}
	
/*
	if(!request_mem_region(rtc->res->start, resource_size(rtc->res), platform_dev->name)) {
		dev_err(&platform_dev->dev,"unable to request region\n");
		ret = -EBUSY;
		goto res_err;
	}
*/
	rtc->base = ioremap(rtc->res->start, resource_size(rtc->res));
	if(!rtc->base) {
		dev_err(&platform_dev->dev,"ioremap failed.\n");
		ret = -ENOMEM;
		goto map_err;
	};
	
        platform_set_drvdata(platform_dev,rtc);
	scm801_rtc_setup(&platform_dev->dev);
	rtc->rtc_dev= rtc_device_register(platform_dev->name, &platform_dev->dev, &scm801_rtc_ops, THIS_MODULE);
	if(!rtc->rtc_dev) {
		 dev_err(&platform_dev->dev,"rtc dev register failed.\n");
		 ret = -ENOMEM; 
		 goto dev_err;
	}
	dev_info(&platform_dev->dev,"scm801 rtc driver install ok!\n");
	return 0;
dev_err:
map_err:
	iounmap(rtc->base);
res_err:
	kfree(rtc);
	return ret;
}


static int scm801_rtc_remove(struct platform_device *platform_dev)
{	
	struct scm801_rtc *rtc = platform_get_drvdata(platform_dev);
	rtc_device_unregister(rtc->rtc_dev);
	iounmap(rtc->base);
	kfree(rtc);
	return 0;
}


static const struct of_device_id scm801_rtc_of_match[] = {
	{ .compatible = "smartchip,scm801-rtc" },
	{ }
};
MODULE_DEVICE_TABLE(of, scm801_rtc_of_match);

static struct platform_driver  scm801_rtc_driver = {
	.driver = {
		.name = "scm801-rtc",
		.of_match_table = scm801_rtc_of_match,
	},
	.probe	= scm801_rtc_probe,
	.remove	= scm801_rtc_remove,
};

static int __init scm801_rtc_init(void)
{
	return platform_driver_register(&scm801_rtc_driver);
}
module_init(scm801_rtc_init);

static void __exit  scm801_rtc_exit(void)
{
	platform_driver_unregister(&scm801_rtc_driver);	
}
module_exit(scm801_rtc_exit);

MODULE_AUTHOR("zg");
MODULE_DESCRIPTION("smart chip scm801 rtc driver\n");
MODULE_LICENSE("GPL");



