#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>

struct att_dev {
	struct platform_device *pdev;
//	struct kobject *kobj;
};

static struct att_dev *dev = NULL;

static ssize_t test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) 
{
	int len;
	int base = 10;
	char areg[16] = {0};
	char aval[16] = {0};
	unsigned int reg = 0;
	unsigned int val = 0;
	char *ptr = strchr(buf, '=');
	printk(KERN_ERR "buf = [%s], ptr = %s, count = %d\n", buf, ptr, (int)count);
	if(ptr) {
		len = ptr - buf;
		base = 10;
		if(buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X') && len > 2) {
			len -= 2;
			buf += 2;
			base = 16;
		}
		memcpy(areg, buf, len);
		areg[len] = '\0';
    		reg = simple_strtoul(areg, NULL, base);
		
		ptr += 1;
		base = 10;
		if(ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
			ptr += 2;
			base = 16;
		}
		strcpy(aval, ptr);
    		val = simple_strtoul(aval, NULL, base);
		printk(KERN_ERR "areg = %s, aval = %s\n", areg, aval);
		printk(KERN_ERR "reg = %x, val = %x\n", reg, val);
	}
	else {
		base = 10;
		if(buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
			buf += 2;
			base = 16;
		}
		strcpy(areg, buf);
    		reg = simple_strtoul(areg, NULL, base);
		printk(KERN_ERR "areg = %s\n", areg);
		printk(KERN_ERR "reg = %x\n", reg);
	}
	return count;
}

static ssize_t test_show(struct device *dev, struct device_attribute *attr, char *buf) 
{
	printk(KERN_ERR "cat debug buf, buf = %s\n", buf);

	return 0;
}

static DEVICE_ATTR(test, 0777, test_show, test_store);

static int my_driver_probe(struct platform_device *ppdev)
{
	int ret;
//	dev->kobj = kobject_create_and_add("attkobj", NULL);
//	if(dev->kobj == NULL) {
//		ret = -ENOMEM;
//		goto kobj_err;
//	}

	ret = sysfs_create_file(&dev->pdev->dev.kobj, &dev_attr_test.attr);
	if(ret < 0)
		goto file_err;

	return 0;

file_err:
//	kobject_del(dev->kobj);
//kobj_err:
	return ret;
}


static struct platform_driver my_driver = {
	.probe = my_driver_probe,
	.driver = {
		.owner = THIS_MODULE,
		.name = "my_device",
	},
};


static int __init my_driver_init(void)
{
	int ret;

	dev = kzalloc(sizeof(struct att_dev), GFP_KERNEL);
	if(dev == NULL){
		printk("%s get dev memory error\n", __func__);
		return -ENOMEM;
	}

	dev->pdev = platform_device_register_simple("my_device", -1, NULL, 0);
	if(IS_ERR(dev->pdev)) {
		ret = PTR_ERR(dev->pdev);
		printk("%s pdev error\n", __func__);
		if(dev != NULL)
			kfree(dev);
		return -1;
	}

	ret  = platform_driver_register(&my_driver);
	if(ret < 0) {
		platform_device_unregister(dev->pdev);
		printk("driver_register fail\n");
		if(dev != NULL)
			kfree(dev);
		return ret;	
	}

	return 0;
}

static void __exit my_driver_exit(void)
{
	sysfs_remove_file(&dev->pdev->dev.kobj, &dev_attr_test.attr);
//	kobject_del(dev->kobj);
	platform_device_unregister(dev->pdev);
	platform_driver_unregister(&my_driver);
	if(dev != NULL)
		kfree(dev);
}

module_init(my_driver_init);
module_exit(my_driver_exit);
MODULE_AUTHOR("yhong");
MODULE_LICENSE("GPL");

