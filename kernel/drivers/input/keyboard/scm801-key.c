#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h> 
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

struct pin_desc {
	const char *name;	
	unsigned int pin;
	unsigned int code;
};

struct scm801_keypad_data{
	struct device *dev;
	unsigned int npin;
	struct pin_desc *pins;
};

/*struct scm801_gpio_keypad{
    struct input_dev * input_dev;
    struct time_list * timer;
};*/

static struct pin_desc *irq_pd; 
struct input_dev * input_dev;
struct timer_list timer;

/*struct pin_desc pin_descs[2] = {
	{"scm801_gpio0",22,KEY_L},
	{"scm801_gpio0",25,KEY_S}
};*/


static irqreturn_t scm801_keypad_isr(int irq, void *dev_id)
{

	irq_pd = (struct pin_desc *)dev_id;  
	mod_timer(&timer, jiffies+HZ/100);  
	
	return IRQ_RETVAL(IRQ_HANDLED);  
}


static void timer_function(unsigned long data)  
{  
	struct pin_desc *pin = irq_pd;
	unsigned int pinval;  

	if (!pin)  
		return;  
        
	pinval = gpio_get_value(pin->pin);  
	if (pinval) {
		input_event(input_dev, EV_KEY, pin->code,1);  
		input_sync(input_dev);  
	}
	else {  
		input_event(input_dev, EV_KEY, pin->code,0);  
		input_sync(input_dev);  
	}  
}  


static int scm801_keypad_probe(struct platform_device *pdev)
{
	int ret, i=0;
	struct device *dev= &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child;
	struct scm801_keypad_data *keypad_data;

	

	keypad_data = kzalloc(sizeof(*keypad_data),GFP_KERNEL);
	if(!keypad_data) {
		dev_err(dev," scm801 keypad memery failed\n");
		return -ENOMEM;
	}

	for_each_child_of_node(np,child) {
		if(child)
			keypad_data->npin++;
	}
	keypad_data->pins =(struct pin_desc *) kzalloc((keypad_data->npin*sizeof(struct pin_desc)),GFP_KERNEL);
	for_each_child_of_node(np,child) {
		if(child) {
			of_property_read_string(child,"label",&keypad_data->pins[i].name);
			keypad_data->pins[i].pin = of_get_gpio(child,0);
			of_property_read_u32(child,"linux,code",&keypad_data->pins[i].code);
			i++;
		}
	}
	platform_set_drvdata(pdev,keypad_data);
	input_dev = input_allocate_device();
	if(!input_dev) {
		dev_err(dev," allocate failed\n");
		return -ENOMEM;
	}
	input_dev->name = dev_name(dev);
	input_dev->dev.parent = dev;

	set_bit(EV_KEY,input_dev->evbit);
//	set_bit(EV_REP,input_dev->evbit);
	set_bit(KEY_L,input_dev->keybit);
	set_bit(KEY_S,input_dev->keybit);

	ret = input_register_device(input_dev);
	if(ret) {
		dev_err(dev,"unable to register\n ");
		goto reg_failed;
	}
	

	init_timer(&timer);
	timer.function = timer_function;
	add_timer(&timer);

	for(i=0; i < keypad_data->npin; i++) {
		gpio_request(keypad_data->pins[i].pin,keypad_data->pins[i].name);
		gpio_direction_input(keypad_data->pins[i].pin);
		request_irq(gpio_to_irq(keypad_data->pins[i].pin), scm801_keypad_isr, 
				IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_SHARED,keypad_data->pins[i].name,&keypad_data->pins[i]);
#if 0           
				request_irq(gpio_to_irq(keypad_data->pin_descs[i].pin), scm801_keypad_isr, 
							IRQF_TRIGGER_HIGH|IRQF_SHARED,keypad_data->pin_descs[i].name,&keypad_data->pin_descs[i]);
#endif
		
	}

	return  0;
reg_failed:
	input_free_device(input_dev);
	return ret;
}


static int scm801_keypad_remove(struct platform_device *pdev)
{
	int i;
	struct scm801_keypad_data *keypad_data = platform_get_drvdata(pdev);

	for(i = 0; i < keypad_data->npin; i++) {
		gpio_free(keypad_data->pins[i].pin);
		free_irq(gpio_to_irq(keypad_data->pins[i].pin), &keypad_data->pins[i]);  
	}  
	del_timer(&timer);  

	input_unregister_device(input_dev);
	input_free_device(input_dev);
	return 0;
}


static const struct of_device_id scm801_keypad_of_match[] = {
	{ .compatible = "smartchip,scm801-keypad", },
	{ }
};

MODULE_DEVICE_TABLE(of, scm801_keypad_of_match);


static struct platform_driver scm801_keypad_driver = {
	.driver	= {
		.name		= "smartchip,sm801-keypad",
		.of_match_table	= scm801_keypad_of_match,
	},
	.probe	= scm801_keypad_probe,
	.remove	= scm801_keypad_remove,
};


static int __init scm801_keypad_init(void)
{
	return platform_driver_register(&scm801_keypad_driver);
};


static void __exit scm801_kaypad_exit(void)
{
	platform_driver_unregister(&scm801_keypad_driver);
}

module_init(scm801_keypad_init);
module_exit(scm801_kaypad_exit);

MODULE_AUTHOR("zg");
MODULE_DESCRIPTION("scm801 gpio-key driver\n");
MODULE_LICENSE("GPL");
