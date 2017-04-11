#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define MEMDEV_MAJOR (0)   /*预设的mem的主设备号*/
#define MEMDEV_NR_DEVS (2)    /*设备数*/
#define MEMDEV_SIZE 4096

/*mem设备描述结构体*/
struct mem_dev                                     
{                                                        
  char *data;                      
  unsigned long size;       
};

/* 定义幻数 */
#define MEMDEV_IOC_MAGIC  'k'

/* 定义命令 */
#define MEMDEV_IOCPRINT   _IO(MEMDEV_IOC_MAGIC, 1)
#define MEMDEV_IOCGETDATA _IOR(MEMDEV_IOC_MAGIC, 2, int)
#define MEMDEV_IOCSETDATA _IOW(MEMDEV_IOC_MAGIC, 3, int)

#define MEMDEV_IOC_MAXNR 3


static int mem_major = MEMDEV_MAJOR;

module_param(mem_major, int, S_IRUGO);

struct mem_dev *mem_devp; /*设备结构体指针*/

struct cdev cdev; 

/*文件打开函数*/
int mem_open(struct inode *inode, struct file *filp)
{
printk(KERN_ERR "[%s:%d] %s\n", __FILE__, __LINE__, __func__ );
    return 0; 
}

/*文件释放函数*/
int mem_release(struct inode *inode, struct file *filp)
{
printk(KERN_ERR "[%s:%d] %s\n", __FILE__, __LINE__, __func__ );
  return 0;
}

/*IO操作*/
long memdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

printk(KERN_ERR "[%s:%d] %s\n", __FILE__, __LINE__, __func__ );

    /* 根据命令，执行相应的操作 */
    switch(cmd) {

      default:  
printk(KERN_ERR "[%s:%d] %s, cmd=%d\n", __FILE__, __LINE__, __func__ , cmd);
        return -EINVAL;
    }
    return ret;

}

/*文件操作结构体*/
static const struct file_operations mem_fops =
{
  .owner = THIS_MODULE,
  .open = mem_open,
  .release = mem_release,
  .unlocked_ioctl = memdev_ioctl,
};

/*设备驱动模块加载函数*/
static int memdev_init(void)
{
  
printk(KERN_ERR "[%s:%d] %s\n", __FILE__, __LINE__, __func__ );
  int result;
  int i;

  dev_t devno = MKDEV(mem_major, 0);

  /* 静态申请设备号*/
  if (mem_major)
    result = register_chrdev_region(devno, 2, "memdev");
  else  /* 动态分配设备号 */
  {
    result = alloc_chrdev_region(&devno, 0, 2, "memdev");
    mem_major = MAJOR(devno);
  }  
  
  if (result < 0)
    return result;

  /*初始化cdev结构*/
  cdev_init(&cdev, &mem_fops);
  cdev.owner = THIS_MODULE;
  cdev.ops = &mem_fops;
  
  /* 注册字符设备 */
  cdev_add(&cdev, MKDEV(mem_major, 0), MEMDEV_NR_DEVS);
 #if 0  
  /* 为设备描述结构分配内存*/
  mem_devp = kmalloc(MEMDEV_NR_DEVS * sizeof(struct mem_dev), GFP_KERNEL);
  if (!mem_devp)    /*申请失败*/
  {
    result =  - ENOMEM;
    goto fail_malloc;
  }
  memset(mem_devp, 0, sizeof(struct mem_dev));
  
  /*为设备分配内存*/
  for (i=0; i < MEMDEV_NR_DEVS; i++) 
  {
        mem_devp[i].size = MEMDEV_SIZE;
        mem_devp[i].data = kmalloc(MEMDEV_SIZE, GFP_KERNEL);
        memset(mem_devp[i].data, 0, MEMDEV_SIZE);
  }  
#endif
  return 0;

}

/*模块卸载函数*/
static void memdev_exit(void)
{
printk(KERN_ERR "[%s:%d] %s\n", __FILE__, __LINE__, __func__ );
  cdev_del(&cdev);   /*注销设备*/
 // kfree(mem_devp);     /*释放设备结构体内存*/
  unregister_chrdev_region(MKDEV(mem_major, 0), 2); /*释放设备号*/
}

MODULE_AUTHOR("David Xie");
MODULE_LICENSE("GPL");

module_init(memdev_init);
module_exit(memdev_exit);
