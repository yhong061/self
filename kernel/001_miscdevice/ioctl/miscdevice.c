#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/fs.h>  /*file_operations*/


#define DEVDRV_NAME_YHONG      "yhong"       //device_driver.h
#define DEVICE_YHONG           "/dev/yhong"  //major.h


long yhong_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_ERR"[%s:%d] %s cmd = %d\n", __FILE__, __LINE__, __func__, cmd);

    return 0;
}

int yhong_open(struct inode *inode, struct file *filp)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    return 0;
}

int yhong_release(struct inode *inode, struct file *filp)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    return 0;
}

int yhong_suspend (struct platform_device *dev, pm_message_t state)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    return 0;
}

int yhong_resume (struct platform_device *dev)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
static void yhong_dummy_release(struct device *dev)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
}



static struct file_operations yhong_fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl = yhong_ioctl,
    .compat_ioctl = yhong_ioctl,
    .open = yhong_open,
    .release = yhong_release,
};

static struct platform_device yhong_platform_device =
{
    .name   = DEVDRV_NAME_YHONG,
    .id = -1,
    .dev = {
        .release = yhong_dummy_release,
    },
};

static struct platform_driver yhong_platform_driver =
{
    .driver =
    {
        .name = DEVDRV_NAME_YHONG,
        .owner = THIS_MODULE,
    },
    .suspend = yhong_suspend,
    .resume = yhong_resume,
};

static struct miscdevice yhong_miscdevice =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVDRV_NAME_YHONG,
    .fops = &yhong_fops,
};


static int yhong_init(void)
{
    int ret;
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    ret = misc_register(&yhong_miscdevice);
    if (ret)
    {
        printk(KERN_ERR"register yhong misc device failed!...\n");
        goto err0;
    }
    printk(KERN_ERR"yhong_drv_init ...\n");

    ret = platform_device_register(&yhong_platform_device);
    if (ret)
    {
        printk(KERN_ERR"register yhong_platform_device error!...\n");
        goto err1;
    }

    ret = platform_driver_register(&yhong_platform_driver);
    if (ret)
    {
        printk(KERN_ERR"register yhong platform driver4pm error!...\n");
        goto err2;
    }
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);

    return 0;

err2:
    platform_device_unregister(&yhong_platform_device);
err1:
    misc_deregister(&yhong_miscdevice);
err0:
    return ret;
}

static void yhong_exit(void)
{
    printk(KERN_ERR"yhong_drv_exit ..!.\n");
    misc_deregister(&yhong_miscdevice);
    platform_device_unregister(&yhong_platform_device);
    platform_driver_unregister(&yhong_platform_driver);
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
}

module_init(yhong_init);
module_exit(yhong_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("Actions SOC YHONG device driver");
MODULE_ALIAS("platform: asoc-yhong");
