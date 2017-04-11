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



#define PMU_AUXADC0_REG (0x4d)
#define PMU_AUXADC0_VAL_LIMIT (0x2a)
#define SLED_THREAD_SLEEP_TIME_MS (2*1000)

struct atc260x_dev * atc260x_dev = NULL;
int ir_led_gpio = -1;
int ircut_gpio = -1;
int ircut_remove_gpio = -1;
int ircut_flag = 1; // 0: ircut remove; 1: ircut

static struct delayed_work sled_blink;
static void sled_blink_control(struct work_struct *work)
{
	unsigned int reg = PMU_AUXADC0_REG;
	int value = 0;
atc260x_dev = NULL; 
	if(atc260x_dev == NULL) {
   	 	printk(KERN_ERR "sled_blink_control: atc260x_dev is NULL\n");
		return;
	}
	
    value = atc260x_reg_read(atc260x_dev, reg);
    printk(KERN_ERR "sled reg = 0x%x, value = %d\n", reg, value);

	if(value > PMU_AUXADC0_VAL_LIMIT && ircut_flag != 1) { //ircut
    printk(KERN_ERR " 1 sled reg = 0x%x, value = %d\n", reg, value);
		if(ircut_gpio > 0)
			gpio_direction_output(ircut_gpio, 1);
		if(ircut_remove_gpio > 0)
			gpio_direction_output(ircut_remove_gpio, 0);
		if(ir_led_gpio > 0)
			gpio_direction_output(ir_led_gpio, 0);
		ircut_flag = 1;
	}
	else if(value <= PMU_AUXADC0_VAL_LIMIT && ircut_flag != 0) { //ircut remove
    printk(KERN_ERR " 2 sled reg = 0x%x, value = %d\n", reg, value);
		if(ircut_gpio > 0)
			gpio_direction_output(ircut_gpio, 0);
		if(ircut_remove_gpio > 0)
			gpio_direction_output(ircut_remove_gpio, 1);
		if(ir_led_gpio > 0)
			gpio_direction_output(ir_led_gpio, 1);
		ircut_flag = 0;
	}
	
	schedule_delayed_work(&sled_blink, msecs_to_jiffies(SLED_THREAD_SLEEP_TIME_MS));
}

void sled_thread_create(void)
{
	INIT_DELAYED_WORK(&sled_blink,  sled_blink_control);
	schedule_delayed_work(&sled_blink, msecs_to_jiffies(2000));
}

void sled_thread_destroy(void)
{
 	cancel_delayed_work(&sled_blink);
}


static int sled_probe(struct platform_device *pdev)
{
        struct device_node *np = pdev->dev.of_node;
        int flag = 0;
	int ret = 0;
        printk(KERN_ERR"sled probe");
        atc260x_dev = dev_get_drvdata(pdev->dev.parent);
        printk(KERN_ERR "atc260x_dev = %p\n",atc260x_dev);
        
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
	sled_thread_create();
        return 0;

}
static int sled_remove(struct platform_device *pdev)
{
	sled_thread_destroy();
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
