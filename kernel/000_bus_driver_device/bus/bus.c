#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>

static char *version = "version 1.0";

static int my_bus_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev_name(dev), driver->name, strlen(driver->name));
}

static void my_bus_release(struct device *dev)
{
	return;
}

static ssize_t show_bus_version(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", version);
}

static BUS_ATTR(version, S_IRUGO, show_bus_version, NULL);

struct bus_type my_bus_type = {
	.name = "my_bus_type",
	.match = my_bus_match,
};
EXPORT_SYMBOL(my_bus_type);

struct device my_bus_device = {
	.init_name = "my_bus_device",
	.release = my_bus_release,
};
EXPORT_SYMBOL(my_bus_device);

static int __init my_bus_init(void)
{
	int ret;
	//create bus:    /sys/bus/my_bus_type
	//create device: /sys/devices/my_bus_devices

	//the devices or drivers will be added under this bus
		//./my_bus_device/:      include the content of devices
		//./my_bus_type/drivers: include the content of drivers
	ret = bus_register(&my_bus_type); 
	if(ret)
		printk("bus_register fail\n");

	ret = bus_create_file(&my_bus_type, &bus_attr_version);
	if(ret)
		printk("bus_create_file fail\n");

	ret = device_register(&my_bus_device);
	if(ret)
		printk("device_register fail\n");

	return ret;
}

static void __exit my_bus_exit(void)
{
	bus_unregister(&my_bus_type);

	device_unregister(&my_bus_device);
}

module_init(my_bus_init);
module_exit(my_bus_exit);
MODULE_AUTHOR("yhong");
MODULE_LICENSE("GPL");

