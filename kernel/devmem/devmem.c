#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/v4l2-chip-ident.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <linux/miscdevice.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/fs.h>  /*file_operations*/
#include <linux/io.h>  /*file_operations*/
#include <linux/slab.h>

#define DEVDRV_NAME_YHONG      "yhong"       //device_driver.h
#define DEVICE_YHONG           "/dev/yhong"  //major.h

#define MODULE_DBG_INFO
#define MODULE_DBG_ERR

#ifdef 	MODULE_DBG_INFO
#define DBG_INFO(fmt, args...)  printk(KERN_ERR"[" CAMERA_DEBUG_INFO "] (INFO) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#else
#define DBG_INFO(fmt, args...)  do {} while(0)
#endif

#ifdef 	MODULE_DBG_ERR
#define DBG_ERR(fmt, args...)   printk(KERN_ERR"[" CAMERA_DEBUG_INFO "] (ERR) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#else
#define DBG_ERR(fmt, args...) do {} while(0)
#endif

//#define USER_IO_H
#ifdef USER_IO_H
static inline u32 __raw_readl(const volatile void __iomem *addr)
{
	u32 val;
	asm volatile("ldr %w0, [%1]" : "=r" (val) : "r" (addr));
	return val;
}

static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
	asm volatile("str %w0, [%1]" : : "r" (val), "r" (addr));
}


#define readl_relaxed(c)        ({ u32 __v = le32_to_cpu((__force __le32)__raw_readl(c)); __v; })
#define writel_relaxed(v,c)     ((void)__raw_writel((__force u32)cpu_to_le32(v),(c)))
#endif

#if 0
static int camera_i2c_read(struct i2c_adapter *i2c_adap,unsigned int data_width, 
unsigned int reg, unsigned int *dest)
{
	unsigned char regs_array[4] = { 0, 0, 0, 0 };
	unsigned char data_array[4] = { 0, 0, 0, 0 };
	struct i2c_msg msg;
	int ret = 0;

	if (I2C_REGS_WIDTH == 1)
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len = I2C_REGS_WIDTH;
	msg.buf = regs_array;
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret < 0) {
		DBG_ERR("write register %s error %d", CAMERA_MODULE_NAME, ret);
		return ret;
	}

	msg.flags = I2C_M_RD;
	msg.len = data_width;
	msg.buf = data_array;
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret >= 0) {
		ret = 0;
		if (data_width == 1)
			*dest = data_array[0];
		if (data_width == 2)
			*dest = data_array[0] << 8 | data_array[1];
	} else
		DBG_ERR("read register %s error %d", CAMERA_MODULE_NAME, ret);

	return ret;
}
static int camera_i2c_write(struct i2c_adapter *i2c_adap, unsigned int data_width, 
unsigned int reg, unsigned int data)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
    unsigned char data_array[4] = {0, 0, 0, 0};
    unsigned char tran_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	struct i2c_msg msg;
	int ret,i;
		
	if (I2C_REGS_WIDTH == 1)		
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}
	if (data_width == 1)
		data_array[0] = data & 0xff;
	if (data_width == 2) {
		data_array[0] = (data >> 8) & 0xff;
		data_array[1] = data & 0xff;
	}
	for (i = 0; i < I2C_REGS_WIDTH; i++) {
        tran_array[i] = regs_array[i];
    }

    for (i = I2C_REGS_WIDTH; i < (I2C_REGS_WIDTH + data_width); i++) {
        tran_array[i] = data_array[i - I2C_REGS_WIDTH];
    }
	
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH + data_width;
	msg.buf   = tran_array;    
	ret = i2c_transfer(i2c_adap, &msg, 1);
	DBG_INFO("[xc6130 W2] addr=%x, len=%d, flags=%d, reg=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x, ret=%d\n", msg.addr, msg.len, msg.flags, tran_array[0], tran_array[1], tran_array[2], tran_array[3], tran_array[4], tran_array[5], tran_array[6], tran_array[7], ret);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
	    printk(KERN_ERR"write register %s error %d",CAMERA_MODULE_NAME, ret);
	}
	
	 
	return ret;	
}

static int camera_write_array(struct i2c_adapter *i2c_adap, const struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != 0xff) {
		DBG_INFO(" [%s:%d] vals->reg_num=%x, vals->value=%x\n", __func__, __LINE__, vals->reg_num, vals->value);
		ret = camera_i2c_write(i2c_adap,
							vals->data_width,
							vals->reg_num,
							vals->value);
		if (ret < 0){
			printk(KERN_ERR"[camera] i2c write error!,i2c address is %x\n",MODULE_I2C_REAL_ADDRESS);
			return ret;
		}
		vals++;
	}
	return 0;
}
#endif

int yhong_mem_read(unsigned int mem, unsigned int *data)
{
	int ret = -1;
	unsigned int val;
	//void *mem_addr = ioremap(mem, 0x4);
	printk(KERN_ERR "[%s:%d] read mem=0x%08x\n\n\n", __func__, __LINE__, mem);
	void *mem_addr = ioremap(mem, 0x4);
	if(mem_addr) {
		val = readl_relaxed(mem_addr);
		iounmap(mem_addr);
		//printk(KERN_ERR "[%s:%d] read mem=0x%08x, data=0x%08x\n", __func__, __LINE__, mem, val);
		*data = val;
		ret = 0;
	}
	else
		printk(KERN_ERR "[%s:%d] read ioremap fail, mem = 0x%08x\n", __func__, __LINE__, mem);
	return ret;
}

int yhong_mem_write(unsigned int mem, unsigned int data)
{
	int ret = -1;
	unsigned int val=data;
	void *mem_addr = ioremap(mem, 0x4);
	if(mem_addr) {
		writel_relaxed(val, mem_addr);
		iounmap(mem_addr);
		//printk(KERN_ERR "[%s:%d] read mem=0x%08x, data=0x%08x\n", __func__, __LINE__, mem, val);
		ret = 0;
	}
	else
		printk(KERN_ERR "[%s:%d] write ioremap fail\n", __func__, __LINE__);
	return ret;
}

enum YHONG_IOCTL_CMD {
	MEM_READ,
	MEM_WRITE,
	IO_END,
};

struct yhong_mem {
	unsigned int mem;
	unsigned int data;
};

long yhong_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *from = (void __user *)arg;
	int ret_count =-1;
	int ret = 0;
	
	switch(cmd) {
    	case MEM_READ: 
	{
		struct yhong_mem mem;
		ret_count = copy_from_user(&mem, from, sizeof(mem));
		printk(KERN_ERR"[%s:%d] read: mem=0x%08x, data=0x%08x, ret = %d\n", __func__, __LINE__, mem.mem, mem.data, ret);
    		ret = yhong_mem_read(mem.mem, &mem.data);
		if(ret)
			mem.data = 0x0;
		printk(KERN_ERR"[%s:%d] read: mem=0x%08x, data=0x%08x, ret = %d\n", __func__, __LINE__, mem.mem, mem.data, ret);
        	ret_count = copy_to_user(from, &mem, sizeof(mem));
	}break;

    	case MEM_WRITE: 
	{
		struct yhong_mem mem;
		ret_count = copy_from_user(&mem, from, sizeof(mem));
    		ret = yhong_mem_write(mem.mem, mem.data);
		//printk(KERN_ERR"[%s:%d] write: mem=0x%08x, data=0x%08x, ret = %d\n", __func__, __LINE__, mem.mem, mem.data, ret);
	}break;

    	default:
    		printk(KERN_ERR"[%s:%d] cmd = %d\n", __func__, __LINE__, cmd);
    		
    }

    return ret;
}

int yhong_open(struct inode *inode, struct file *filp)
{
//    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
    return 0;
}

int yhong_release(struct inode *inode, struct file *filp)
{
//    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
    return 0;
}

int yhong_suspend(struct platform_device *dev, pm_message_t state)
{
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
    return 0;
}

int yhong_resume(struct platform_device *dev)
{
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
    return 0;
}
static void yhong_dummy_release(struct device *dev)
{
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
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
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
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
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);

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
    printk(KERN_ERR"[%s:%d]\n", __func__, __LINE__);
}


static int __init my_device_init(void)
{
	yhong_init();
	return 0;
}

static void __exit my_device_exit(void)
{
	yhong_exit();
	return;	
}

module_init(my_device_init);
module_exit(my_device_exit);
MODULE_AUTHOR("yhong");
MODULE_LICENSE("GPL");




