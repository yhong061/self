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
#include <linux/slab.h>

#define DEVDRV_NAME_YHONG      "yhong"       //device_driver.h
#define DEVICE_YHONG           "/dev/yhong"  //major.h

#define MODULE_DBG_INFO
#define MODULE_DBG_ERR
#define CAMERA_DEBUG_INFO           "IRS16X5C"
#define CAMERA_COMMON_NAME           "sensor_common"

static unsigned int *cam_reg = 0xa0a2;
module_param(cam_reg, int, 0644);


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



#define CAMERA_MODULE_NAME 		"IRS16X5C"

#define MODULE_I2C_REAL_ADDRESS		(0x7A>>1)
#define MODULE_I2C_REG_ADDRESS		(0x7A>>1)
#define I2C_REGS_WIDTH			2
#define I2C_DATA_WIDTH			2

#define OUTTO_SENSO_CLOCK 		24000000

struct regval_list {
	unsigned short data_width;
	unsigned short reg_num;
	unsigned short value;
};

#define REG_BUF_LEN (100)
struct reg_buf {
	int size;
	struct regval_list array[REG_BUF_LEN];
};




static struct i2c_adapter       *g_adap;

#define ENDMARKER { 0xff, 0xff, 0xff }

 enum {
	CHIPID_XC6130_OV2710,
	REG_READ_XC6130,
	REG_WRITE_XC6130,
	REG_READ_OV2710,
	REG_WRITE_OV2710,
	ARRAY_WRITE_XC6130,
	ARRAY_READ_XC6130,
	ARRAY_WRITE_OV2710,
	ARRAY_READ_OV2710,
	IO_END,
};

static const struct regval_list module_init_regs[] = {
		{2, 0x9000,	0x0000 }, // start trigger
		{2, 0x9000,	0x0000 }, // start trigger
		{2, 0x9000,	0x0000 }, // start trigger
		{2, 0xA001,	0x0001 }, //Readout settings register 3
		{2, 0xA007,	0x0007 },
		{2, 0x901C,	0x1423 }, //Readout settings register 3
		{2, 0xA033,	0x6060 }, //ADC Test Mode Support register 0
		{2, 0xA054,	0x0080 }, //PLL bandgap settings
		{2, 0xA057,	0x0BE2 }, //PLL configuration register 3
		{2, 0xA06A,	0x55A8 }, //DPHY PLL configuration register 2
		{2, 0x983F,	0x030A }, //PLL configuration register 3
		{2, 0x9842,	0x0BE2 }, //PLL configuration register 3
		{2, 0x9845,	0x03D5 }, //PLL configuration register 3
		{2, 0x9848,	0x1315 }, //PLL configuration register 3
		{2, 0x9835,	0x0192 }, //CSI Configuration
		{2, 0xA084,	0x1513 }, //Phaseshifter SE PAD Configuration
		{2, 0x9000,	0x0005 }, // start trigger
		{2, 0x9839,	0x0008 }, //Sequence settings
		{2, 0x9800,	0x539C }, //Sequence0 sequence exposure time                 //30fps
		{2, 0x9803,	0x539C }, //Sequence1 sequence exposure time
		{2, 0x9806,	0x539C }, //Sequence2 sequence exposure time
		{2, 0x9809,	0x539C }, //Sequence3 sequence exposure time
		{2, 0x980C,	0x4EB5 }, //Sequence4 sequence exposure time
		{2, 0x980F,	0x4EB5 }, //Sequence5 sequence exposure time
		{2, 0x9812,	0x4EB5 }, //Sequence6 sequence exposure time
		{2, 0x9815,	0x4EB5 }, //Sequence7 sequence exposure time
		{2, 0x9818,	0x45E2 }, //Sequence8 sequence exposure time
		{2, 0x9802,	0x0204 }, //Sequence0 Phaseshifter and PLL settings
		{2, 0x9805,	0x0248 }, //Sequence1 Phaseshifter and PLL settings
		{2, 0x9808,	0x028C }, //Sequence2 Phaseshifter and PLL settings
		{2, 0x980B,	0x02C0 }, //Sequence3 Phaseshifter and PLL settings
		{2, 0x980E,	0x0004 }, //Sequence4 Phaseshifter and PLL settings
		{2, 0x9811,	0x0048 }, //Sequence5 Phaseshifter and PLL settings
		{2, 0x9814,	0x008C }, //Sequence6 Phaseshifter and PLL settings
		{2, 0x9817,	0x00C0 }, //Sequence7 Phaseshifter and PLL settings
		{2, 0x981A,	0x2132 }, //Sequence8 Phaseshifter and PLL settings
		{2, 0x9001,	0x30D4 }, //LPFSM Framerate     
		ENDMARKER,
};

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

static int module_verify_pid(struct i2c_adapter *i2c_adap)
{
	unsigned int val = 0;
	DBG_INFO("===========reg = 0x%x \n", cam_reg);
	camera_i2c_read(i2c_adap, 1, cam_reg,  (unsigned int *)&val);
	//camera_write_array(i2c_adap, module_init_regs);
	return 0;
}

static int get_parent_node_id(struct device_node *node,
    const char *property, const char *stem)
{
    struct device_node *pnode;
    unsigned int value = -ENODEV;

    pnode = of_parse_phandle(node, property, 0);
    if (NULL == pnode) {
        printk(KERN_ERR "err: fail to get node[%s]", property);
        return value;
    }
    value = of_alias_get_id(pnode, stem);

    return value;
}


static int init_common(void)
{
    struct device_node *fdt_node;
    int i2c_adap_id = 0;

    // get i2c adapter
    fdt_node = of_find_compatible_node(NULL, NULL, CAMERA_COMMON_NAME);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["CAMERA_COMMON_NAME"] in dts");
        return -1;
    }
    i2c_adap_id = get_parent_node_id(fdt_node, "i2c_adapter", "i2c");
    g_adap = i2c_get_adapter(i2c_adap_id);
	
    return 0;
}

static int detect_process(void)
{
    int ret = -1;
    ret = module_verify_pid(g_adap);

    return ret;
}


long yhong_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    //printk(KERN_ERR"[%s:%d] %s, cmd = %d, arg = %ld\n", __FILE__, __LINE__, __func__, cmd, arg);
    void __user *from = (void __user *)arg;
	int ret_count =-1;

	struct reg_buf argbuf;
	ret_count = copy_from_user(&argbuf, from, sizeof(argbuf));
	printk(KERN_ERR"[%s:%d] %s,argbuf.size = %d\n", __FILE__, __LINE__, __func__, argbuf.size);

    switch(cmd) {
    	case CHIPID_XC6130_OV2710:
    		module_verify_pid(g_adap);
    	break;

    	case ARRAY_WRITE_XC6130: 
		{
    		camera_write_array(g_adap, argbuf.array);
		}
    	break;

		case REG_READ_XC6130: 
		{
		camera_i2c_read(g_adap, argbuf.array[0].data_width, argbuf.array[0].reg_num, (unsigned int *)&argbuf.array[0].value); 
    		printk(KERN_ERR"[%s:%d] %s,reg_num = 0x%x, value = 0x%x\n", __FILE__, __LINE__, __func__, (int)argbuf.array[0].reg_num, (int)argbuf.array[0].value);
        	ret_count = copy_to_user(from, &argbuf, sizeof(argbuf));
		}
    	break;

		case REG_WRITE_XC6130: 
		{
		camera_i2c_write(g_adap, argbuf.array[0].data_width, argbuf.array[0].reg_num, (unsigned int)argbuf.array[0].value); 
    		printk(KERN_ERR"[%s:%d] %s,reg_num = 0x%x, value = 0x%x\n", __FILE__, __LINE__, __func__, (int)argbuf.array[0].reg_num, (int)argbuf.array[0].value);
		}
    	break;

    	default:
    		printk(KERN_ERR"[%s:%d] %s cmd = %d\n", __FILE__, __LINE__, __func__, cmd);
    		
    }

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

int yhong_suspend(struct platform_device *dev, pm_message_t state)
{
    printk(KERN_ERR"[%s:%d] %s\n", __FILE__, __LINE__, __func__);
    return 0;
}

int yhong_resume(struct platform_device *dev)
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


static int __init my_device_init(void)
{
	init_common();
	detect_process();
	
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




