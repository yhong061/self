#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>

#include <../drivers/base/base.h>

extern struct device my_bus_device;
extern struct bus_type my_bus_type;

static void my_device_release(struct device *dev)
{
	return;
}

struct soc_camera_link {
	int bus_id;
};

static struct soc_camera_link camera_link = {
	.bus_id = 0,
};

struct device my_device = {
	.init_name = "my_device",
	.bus = &my_bus_type,
	.parent = &my_bus_device,
	.release = my_device_release,
	.platform_data = &camera_link,
};

static ssize_t mydev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "This is my device!");
}

static DEVICE_ATTR(mydev, S_IRUGO, mydev_show, NULL);


static struct device *next_device(struct klist_iter *i)
{
	struct klist_node *n = klist_next(i);
	struct device *dev = NULL;
	struct device_private *dev_prv;

	if (n) {
		dev_prv = to_device_private_bus(n);
		dev = dev_prv->device;
	}    
	return dev; 
}


int my_bus_for_each_dev(struct bus_type *bus)
{
	struct klist_iter i;
	struct device *dev;
	int error = 0; 
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);

	if (!bus || !bus->p)
		return -EINVAL;

printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
	klist_iter_init_node(&bus->p->klist_devices, &i,NULL);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
	while ((dev = next_device(&i)) && !error)
		printk(KERN_ERR"device name = %s\n", dev->init_name);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
	klist_iter_exit(&i);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
	return error;
}

static int __init my_device_init(void)
{
	int ret;


printk(KERN_ERR"device name = %s\n", my_device.init_name);
	ret = device_register(&my_device);
	if(ret)
		printk("device_register fail\n");
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
my_bus_for_each_dev(&my_bus_type);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);

	device_create_file(&my_device, &dev_attr_mydev);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);
my_bus_for_each_dev(&my_bus_type);
printk(KERN_ERR" %s: %d\n", __FILE__, __LINE__);

	return ret;
}

static void __exit my_device_exit(void)
{
	device_unregister(&my_device);
}

module_init(my_device_init);
module_exit(my_device_exit);
MODULE_AUTHOR("yhong");
MODULE_LICENSE("GPL");




