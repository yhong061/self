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
#define CAMERA_DEBUG_INFO           "XC6130"
#define CAMERA_COMMON_NAME           "sensor_common"


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



#define CAMERA_MODULE_NAME 		"XC6130"
#define CAMERA_MODULE_PID1		0x5843  //0x0003=0x58, 0x0002=0x43
#define CAMERA_MODULE_PID0		0x1101	//0x0001=0x11, 0x0000=0x01
#define MODULE_PIDH1				0x0003 /* Product ID Number H byte */
#define MODULE_PIDL1				0x0002 /* Product ID Number L byte */
#define MODULE_PIDH0				0x0001 /* Product ID Number H byte */
#define MODULE_PIDL0				0x0000 /* Product ID Number L byte */
#define VERSION(pid, ver) 		((pid<<8)|(ver&0xFF))

#define MODULE_I2C_REAL_ADDRESS		(0x36>>1)
#define MODULE_I2C_REG_ADDRESS		(0x36>>1)
#define I2C_REGS_WIDTH			2
#define I2C_DATA_WIDTH			1

#define OUTTO_SENSO_CLOCK 		24000000

#define CAMERA_SENSOR_NAME 		"OV2710"
#define CAMERA_SENSOR_PID		0x2710
#define SENSOR_I2C_REAL_ADDRESS		(0x6c>>1)
#define ENSOR_I2C_REG_ADDRESS		(0x6c>>1)
#define PIDH					0x300a /* Product ID Number H byte */
#define PIDL					0x300b /* Product ID Number L byte */

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



int sensor_type = -1; //0:1080p, 1:720p

static struct i2c_adapter       *g_adap;

#define ENDMARKER { 0xff, 0xff, 0xff }


static const struct regval_list bypass_on[] = {
{1,0xfffd,0x80},
{1,0xfffe,0x80},
{1,0x004d,0x01},
ENDMARKER,
};

static const struct regval_list bypass_off[] = {
{1,0xfffd,0x80},
{1,0xfffe,0x80},
{1,0x004d,0x00},
ENDMARKER,

};

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


static int camera_i2c_read_sensor(struct i2c_adapter *i2c_adap,unsigned int data_width, 
unsigned int reg, unsigned int *dest)
{
	char val = -1;
	int ret;
	unsigned char regs_array[2] = {0, 0};
	if (I2C_REGS_WIDTH == 1)		
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}

	struct i2c_msg msg[] = {
		{
			.addr	= SENSOR_I2C_REAL_ADDRESS,
			.flags	= 0,
			.len	= 2,
			.buf	= regs_array,
		},
		{
			.addr	= SENSOR_I2C_REAL_ADDRESS,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= &val,
		},
	};

	ret = i2c_transfer(i2c_adap, msg, 2);
	//DBG_INFO("[xc6130 ov2710 R2] addr=%x, len=%d, flags=%d, reg=%02x,%02x,%02x, val=%02x, ret=%d\n", msg[1].addr, msg[1].len, msg[1].flags, msg[0].buf[0], msg[0].buf[1], msg[1].buf[0], val, ret);
	if (ret < 0) {
	    DBG_ERR("read register %s error %d\n",CAMERA_MODULE_NAME, ret);
	}

	*dest = val;
	printk(KERN_ERR"[ov2710 R] reg = 0x%04x, val = 0x%02x\n", reg, *dest);

	return ret;	
}

static int camera_i2c_write_sensor(struct i2c_adapter *i2c_adap, unsigned int data_width, 
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
	
	msg.addr = SENSOR_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH + data_width;
	msg.buf   = tran_array;    
	ret = i2c_transfer(i2c_adap, &msg, 1);
	//DBG_INFO("[xc6130 ov2710 W1] addr=%x, len=%d, flags=%d, reg=%x,%x,%x,%x,%x,%x,%x,%x, ret=%d\n", msg.addr, msg.len, msg.flags, tran_array[0], tran_array[1], tran_array[2], tran_array[3], tran_array[4], tran_array[5], tran_array[6], tran_array[7], ret);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
	    printk(KERN_ERR"write register %s error %d",CAMERA_SENSOR_NAME, ret);
	}
	 
	return ret;	
}
static int camera_write_array_sensor(struct i2c_adapter *i2c_adap, const struct regval_list *vals)
{
	//DBG_INFO("xc6130 [%s:%d] vals->reg_num=%x, vals->value=%x\n", __func__, __LINE__, vals->reg_num, vals->value);
	while (vals->reg_num != 0xff) {
		int ret = camera_i2c_write_sensor(i2c_adap,
							vals->data_width,
							vals->reg_num,
							vals->value);
		if (ret < 0){
			printk(KERN_ERR"[camera] i2c write error!,i2c address is %x\n",SENSOR_I2C_REAL_ADDRESS);
			return ret;
		}
		vals++;
	}
	return 0;
}


static int camera_i2c_read(struct i2c_adapter *i2c_adap,unsigned int data_width, 
unsigned int reg, unsigned int *dest)
{
	char val = -1;
	int ret;
	unsigned char regs_array[2] = {0, 0};
	if (I2C_REGS_WIDTH == 1)		
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}

	struct i2c_msg msg[] = {
		{
			.addr	= MODULE_I2C_REAL_ADDRESS,
			.flags	= 0,
			.len	= 2,
			.buf	= regs_array,
		},
		{
			.addr	= MODULE_I2C_REAL_ADDRESS,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= &val,
		},
	};

	ret = i2c_transfer(i2c_adap, msg, 2);
	//DBG_INFO("[xc6130 R2] addr=%x, len=%d, flags=%d, reg=%02x,%02x,%02x, val=%02x ret=%d\n", msg[1].addr, msg[1].len, msg[1].flags, msg[0].buf[0], msg[0].buf[1], msg[1].buf[0], val, ret);
	if (ret < 0) {
	    printk(KERN_ERR"read register %s error %d\n",CAMERA_MODULE_NAME, ret);
	}

	*dest = val;
	printk(KERN_ERR"[xc6130 R] reg = 0x%04x, val = 0x%02x\n", reg, *dest);

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
	//DBG_INFO("[xc6130 W2] addr=%x, len=%d, flags=%d, reg=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x, ret=%d\n", msg.addr, msg.len, msg.flags, tran_array[0], tran_array[1], tran_array[2], tran_array[3], tran_array[4], tran_array[5], tran_array[6], tran_array[7], ret);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
	    printk(KERN_ERR"write register %s error %d",CAMERA_MODULE_NAME, ret);
	}
	
	 
	return ret;	
}


extern void module_bypass_on_off_sensor(struct i2c_adapter *i2c_adap, int on);

static int camera_write_array(struct i2c_adapter *i2c_adap, const struct regval_list *vals)
{
	DBG_INFO("xc6130 [%s:%d] vals->reg_num=%x, vals->value=%x\n", __func__, __LINE__, vals->reg_num, vals->value);
	if(vals->reg_num == 0x2710) {
		if(vals->value == 0xFF)  {//1080P
			if(sensor_type == 0)
				return 0;
			else
				sensor_type= 0;
		}
		else if(vals->value == 0xFE) {//720P
			if(sensor_type == 1)
				return 0;
			else
				sensor_type= 1;
		}

		vals++;
//		DBG_INFO("xc6130 [%s:%d] set ov2710 reg\n", __func__, __LINE__);
		module_bypass_on_off_sensor(i2c_adap, true);
		camera_write_array_sensor(i2c_adap, vals);
		module_bypass_on_off_sensor(i2c_adap, false);
		return 0;
	}

//	DBG_INFO("xc6130 [%s:%d] set xc6130 reg\n", __func__, __LINE__);
	while (vals->reg_num != 0xff) {
		DBG_INFO("xc6130 [%s:%d] vals->reg_num=%x, vals->value=%x\n", __func__, __LINE__, vals->reg_num, vals->value);
		int ret = camera_i2c_write(i2c_adap,
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


void module_bypass_on_off_sensor(struct i2c_adapter *i2c_adap, int on)
{
	if(on) {
		printk(KERN_ERR"bypass on\n");
		camera_write_array(i2c_adap, bypass_on);
	}
	else {
		printk(KERN_ERR"bypass off\n");
		camera_write_array(i2c_adap, bypass_off);
	}
}



static int module_verify_pid_sensor(struct i2c_adapter *i2c_adap)
{
	unsigned int pidh = 0;
    unsigned int pidl = 0;
    int ret = 0;

	module_bypass_on_off_sensor(i2c_adap, true);

    /*
	 * check and show product ID and manufacturer ID
	 */  
	ret  = camera_i2c_read_sensor(i2c_adap, 1, PIDH, &pidh); 
	ret |= camera_i2c_read_sensor(i2c_adap, 1, PIDL, &pidl); 
	switch (VERSION(pidh, pidl)) 
    {
		case CAMERA_SENSOR_PID:
			printk(KERN_ERR"[%s] Product ID verified %x\n",CAMERA_SENSOR_NAME, VERSION(pidh, pidl));
			ret = 0; //OK
		break;
	
		default:
			printk(KERN_ERR"[%s] Product ID error %x\n",CAMERA_MODULE_NAME, VERSION(pidh, pidl));
		return -ENODEV;
	}
	
 	module_bypass_on_off_sensor(i2c_adap, false);
   
	return ret;
}

static void module_write_page(struct i2c_adapter *i2c_adap, unsigned int val0, unsigned int val1)
{
	int ret = 0;
	ret  = camera_i2c_write(i2c_adap, 1, 0xfffd, val0); 
	ret |= camera_i2c_write(i2c_adap, 1, 0xfffe, val1);
	if (ret < 0){
			printk(KERN_ERR"[camera] module_write_pager!,i2c address is %x\n",MODULE_I2C_REAL_ADDRESS);
	}
}

static int module_verify_pid(struct i2c_adapter *i2c_adap)
{
	unsigned int pidh0 = 0;
    unsigned int pidl0 = 0;
	unsigned int pidh1 = 0;
    unsigned int pidl1 = 0;
    int ret = 0;

	module_write_page(i2c_adap, 0x80, 0x80);

    /*
	 * check and show product ID and manufacturer ID
	 */  
	ret |= camera_i2c_read(i2c_adap, 1, MODULE_PIDL0, &pidl0); 
	ret |= camera_i2c_read(i2c_adap, 1, MODULE_PIDH0, &pidh0); 
	ret |= camera_i2c_read(i2c_adap, 1, MODULE_PIDL1, &pidl1); 	
	ret |= camera_i2c_read(i2c_adap, 1, MODULE_PIDH1, &pidh1); 
	switch (VERSION(pidh0, pidl0)) 
    {
		case CAMERA_MODULE_PID0:
			printk(KERN_ERR"[%s] Product ID0 verified %x\n",CAMERA_MODULE_NAME, VERSION(pidh0, pidl0));
		break;
	
		default:
			printk(KERN_ERR"[%s] Product ID0 error %x\n",CAMERA_MODULE_NAME, VERSION(pidh0, pidl0));
		return -ENODEV;
	}
	
	switch (VERSION(pidh1, pidl1)) 
    {
		case CAMERA_MODULE_PID1:
			printk(KERN_ERR"[%s] Product ID1 verified %x\n",CAMERA_MODULE_NAME, VERSION(pidh1, pidl1));
		break;
	
		default:
			printk(KERN_ERR"[%s] Product ID1 error %x\n",CAMERA_MODULE_NAME, VERSION(pidh1, pidl1));
		return -ENODEV;
	}

	ret = module_verify_pid_sensor(i2c_adap);
//	printk(KERN_ERR"xc6130 [%s:%d] ret=%d\n", __func__, __LINE__, ret);
	
	return ret;
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
    printk(KERN_ERR"[%s:%d] %s, cmd = %d, arg = %ld\n", __FILE__, __LINE__, __func__, cmd, arg);
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
	        //struct reg_buf argbuf;
	        //ret_count = copy_from_user(&argbuf, from, sizeof(argbuf));
    		//printk(KERN_ERR"[%s:%d] %s,argbuf.size = %d\n", __FILE__, __LINE__, __func__, argbuf.size);

    		camera_write_array(g_adap, argbuf.array);
		}
    	break;

		case REG_READ_XC6130: 
		{
	       // struct reg_buf argbuf;
	       // ret_count = copy_from_user(&argbuf, from, sizeof(argbuf));
    		//printk(KERN_ERR"[%s:%d] %s,argbuf.size = %d\n", __FILE__, __LINE__, __func__, argbuf.size);

			camera_i2c_read(g_adap, argbuf.array[0].data_width, argbuf.array[0].reg_num, (unsigned int *)&argbuf.array[0].value); 
    		printk(KERN_ERR"[%s:%d] %s,reg_num = 0x%x, value = 0x%x\n", __FILE__, __LINE__, __func__, (int)argbuf.array[0].reg_num, (int)argbuf.array[0].value);
        	ret_count = copy_to_user(from, &argbuf, sizeof(argbuf));
		}
    	break;

		case REG_WRITE_XC6130: 
		{
	       // struct reg_buf argbuf;
	       // ret_count = copy_from_user(&argbuf, from, sizeof(argbuf));
    		//printk(KERN_ERR"[%s:%d] %s,argbuf.size = %d\n", __FILE__, __LINE__, __func__, argbuf.size);

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




