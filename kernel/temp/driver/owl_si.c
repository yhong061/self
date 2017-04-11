#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/mod_devicetable.h>

extern struct bus_type my_bus_type;

static int my_driver_probe(struct device *dev)
{
	printk("Driver found device!\n");
	return 0;
}

static int my_driver_remove(struct device *dev)
{
	printk("Driver unpluged!\n");
	return 0;
}

static void my_driver_shutdown(struct device *dev)
{
	return;
}

static int my_driver_suspend(struct device *dev)
{
	return 0;
}

static int my_driver_resume(struct device *dev)
{
	return 0;
}

static const struct of_device_id my_driver_match_table[] = {
	{.compatible = "my_device",},
	{},
};

static const struct dev_pm_ops my_driver_pm_ops = {
	.suspend = my_driver_suspend,
	.resume = my_driver_resume,
};

struct device_driver my_driver = {
	.name = "my_device",
	.owner = THIS_MODULE,
	.pm = &my_driver_pm_ops,
	.of_match_table = my_driver_match_table,
	.bus = &my_bus_type,
	.probe = my_driver_probe,
	.remove = my_driver_remove,
	.shutdown = my_driver_shutdown,
};

static ssize_t my_driver_show(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%s\n", "This is my driver!");
}

static DRIVER_ATTR(mydrv, S_IRUGO, my_driver_show, NULL);


static int __init my_driver_init(void)
{
	int ret;

printk(KERN_ERR"driver name = %s\n", my_driver.name);
	ret  = driver_register(&my_driver);
	if(ret)
		printk("driver_register fail\n");

	ret = driver_create_file(&my_driver, &driver_attr_mydrv);
	if(ret)
		printk("driver_create_file fail\n");

	return ret;
}

static void __exit my_driver_exit(void)
{
	driver_unregister(&my_driver);
}

module_init(my_driver_init);
module_exit(my_driver_exit);
MODULE_AUTHOR("yhong");
MODULE_LICENSE("GPL");

