#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/fs.h>  /*file_operations*/
#include <linux/kernel.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/../../drivers/mfd/atc260x-core.h>



struct atc260x_dev * atc260x_dev=NULL;
struct timer_list timer;
int ir_led_gpio = -1;
int ircut_gpio = -1;
int ircut_remove_gpio = -1;

int ircut_flag = 0;

void timer_func(unsigned long data)
{
	unsigned int reg = 0x80;//0x4d;
	int value = 0;
	struct atc260x_dev * atc260x_dev_p =(struct atc260x*) data;
	printk(KERN_ERR"[%s]Timer Expired and para is %d!!\n",__FILE__, data);
	printk(KERN_ERR"[%s] atc260x_dev_p = %p, reg = 0x%02x\n",__FILE__, atc260x_dev_p, reg);
        value = atc260x_reg_read(atc260x_dev_p, reg);
        printk(KERN_ERR "sled reg = 0x%x, value = %d\n", reg, value);

	if(ircut_flag) {
		if(ircut_gpio > 0)
			gpio_direction_output(ircut_gpio, 1);
		if(ircut_remove_gpio > 0)
			gpio_direction_output(ircut_remove_gpio, 0);
		if(ir_led_gpio > 0)
			gpio_direction_output(ir_led_gpio, 0);
		ircut_flag = 0;
	}
	else {
		if(ircut_gpio > 0)
			gpio_direction_output(ircut_gpio, 0);
		if(ircut_remove_gpio > 0)
			gpio_direction_output(ircut_remove_gpio, 1);
		if(ir_led_gpio > 0)
			gpio_direction_output(ir_led_gpio, 1);
		ircut_flag = 1;
	}


	mod_timer(&timer,jiffies + msecs_to_jiffies(2000));
}


int timer_init(void)
{
	init_timer(&timer);  
	timer.data=(unsigned long)atc260x_dev;//10;   
	//timer.expires=jiffies+(10*HZ);  
	timer.function=timer_func; 
	add_timer(&timer);  
    
  return 0;
}


void timer_exit(void)
{
	del_timer(&timer);  
}

static int sled_probe(struct platform_device *pdev)
{
        struct device_node *np = pdev->dev.of_node;
        int flag = 0;
	int ret = 0;
	int reg = 0x4d;
	int value = 0;
        printk(KERN_ERR"sled probe");
        atc260x_dev = dev_get_drvdata(pdev->dev.parent);
        printk(KERN_ERR "atc260x_dev = %p\n",atc260x_dev);
        value = atc260x_reg_read(atc260x_dev, reg);
        printk(KERN_ERR "[probe]sled reg = 0x%x, value = %d\n", reg, value);
        
	ret = of_get_named_gpio_flags(np, "ir-led", 0, (enum of_gpio_flags *)&flag);
        if(ret > 0) { 

                if(gpio_request(ret, "ir-leds") < 0 )
                        printk(KERN_WARNING"request gpio:%d failed.This should not happen\n",ret);
                else {
                        printk("green led [gpio%d] request success!\n", ret);
                        ir_led_gpio = ret; 
                        gpio_export(ir_led_gpio, true);
                        gpio_direction_output(ret, 0);
                }
        }

	flag = 0;
        ret = of_get_named_gpio_flags(np, "ircut-gpio", 0, (enum of_gpio_flags *)&flag);
        if(ret > 0) { 

                if(gpio_request(ret, "ircut-gpio-name") < 0 )
                        printk(KERN_WARNING"request gpio:%d failed.This should not happen\n",ret);
                else {
                        printk("green led [gpio%d] request success!\n", ret);
                        ircut_gpio = ret; 
                        gpio_export(ircut_gpio, true);
                        gpio_direction_output(ret, 1);
                }
        }

	flag = 0;
        ret = of_get_named_gpio_flags(np, "ircut-remove-gpio", 0, (enum of_gpio_flags *)&flag);
        if(ret > 0) { 

                if(gpio_request(ret, "ircut-remove-gpio") < 0 )
                        printk(KERN_WARNING"request gpio:%d failed.This should not happen\n",ret);
                else {
                        printk("green led [gpio%d] request success!\n", ret);
                        ircut_remove_gpio = ret; 
                        gpio_export(ircut_remove_gpio, true);
                        gpio_direction_output(ret, 0);
                }
        }

        timer_init();
        return 0;

}
static int sled_remove(struct platform_device *pdev)
{
	timer_exit();
	if(ir_led_gpio > 0) {
		gpio_set_value(ir_led_gpio, 0);
		gpio_free(ir_led_gpio);
	}
	if(ircut_gpio > 0) {
		gpio_set_value(ircut_gpio, 0);
		gpio_free(ircut_gpio);
	}
	if(ircut_remove_gpio > 0) {
		gpio_set_value(ircut_remove_gpio, 0);
		gpio_free(ircut_remove_gpio);
	}
	return 0;
}
static const struct of_device_id sled_of_match[] = {
        {.compatible = "actions,actions-sensor-led",},
        {},
};

MODULE_DEVICE_TABLE(of, sled_of_match);

static struct platform_driver sled_driver = {
        .driver = {
                   .name = "sensor_led_name",
                   .owner = THIS_MODULE,
                   .of_match_table = sled_of_match,
                   },
        .probe = sled_probe,
        .remove = sled_remove,
};

int pmu_init(void)
{
	platform_driver_register(&sled_driver);
  
  return 0;
}

void pmu_exit(void)
{
	platform_driver_unregister(&sled_driver);
}

module_init(pmu_init);
module_exit(pmu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("Actions SOC YHONG device driver");
MODULE_ALIAS("platform: asoc-yhong");
