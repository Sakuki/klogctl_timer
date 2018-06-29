#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/timer.h>

#define COEVDEMO_MAJOR 255 	//预设cdevdemo的主设备号

static int cdevdemo_major = COEVDEMO_MAJOR;
char temp[256]={0};
static struct timer_list tm;
struct timeval oldtv;
int i = 0;

struct cdevdemo_dev
{
	struct cdev cdev;
};

struct cdevdemo_dev *cdevdemo_devp;	//设备结构体指针

void times_up(unsigned long arg)
{
	char *str = (char*)arg;
	printk(KERN_NOTICE "%s %d\n",str,++i);
	
//	tm.expires = jiffies+msecs_to_jiffies(100);
	tm.expires = jiffies+25*(HZ/250);		//定时100毫秒
	add_timer(&tm);			//重新计时
}

//文件操作结构体
static const struct file_operations cdevdemo_fops =
{
	.owner = THIS_MODULE,
};

//初始化并注册cdev
static void cdevdemo_setup_cdev(struct cdevdemo_dev *dev, int index)
{
	int err, devno = MKDEV(cdevdemo_major, index);
	
	//初始化一个字符设备，设备所支持的操作在cdevdemo_fops中
	cdev_init(&dev->cdev, &cdevdemo_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &cdevdemo_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
	{
		printk(KERN_NOTICE "Error %d add cdevdemo %d", err, index);
	}
} 

int cdevdemo_init(void)
{
	int ret;
	dev_t devno = MKDEV(cdevdemo_major, 0);
	
	struct class *cdevdemo_class;
	//申请设备号，如果失败就采用动态申请方式
	if(cdevdemo_major)
	{
		ret = register_chrdev_region(devno, 1, "cdevdemo");
	}else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, "cdevdemo");
		cdevdemo_major = MAJOR(devno);
	}
	if(ret < 0)
	{
		return ret;
	}
	//动态申请设备结构体内存
	cdevdemo_devp = kmalloc( sizeof(struct cdevdemo_dev), GFP_KERNEL );
	if(!cdevdemo_devp)
	{
		ret = -ENOMEM;
		printk(KERN_NOTICE "Error add cdevdemo");
		goto fail_malloc;
	}
	
	memset(cdevdemo_devp, 0, sizeof(struct cdevdemo_dev));
	cdevdemo_setup_cdev(cdevdemo_devp, 0);
	/*下面两行是创建了一个总线类型，会在/sys/class下生成cdevdemo目录
	还有一个重要作用是执行device_create后会在/dev/下自动生成cdevdemo设备节点。
	而如果不调用此函数，如果想通过设备节点访问设备，则需要手动mknod来创建设备节点后再访问。
	*/
	cdevdemo_class = class_create(THIS_MODULE, "cdevdemo");
	device_create(cdevdemo_class, NULL, MKDEV(cdevdemo_major, 0), NULL, "cdevdemo");
	
	printk(KERN_NOTICE "=========cdevdemo_init success");
	init_timer(&tm);		//初始化内核定时器
	
	tm.function = times_up;		//指定定时时间到后的回调函数
	tm.data = (unsigned long)"This is a test dev";		//回调函数的参数
//	tm.expires = jiffies+msecs_to_jiffies(100);			//定时时间
	tm.expires = jiffies+25*(HZ/250);				//定时100毫秒
	add_timer(&tm);			//注册定时器
	
	return 0;
	
	fail_malloc:
		unregister_chrdev_region(devno, 1);
} 

void cdevdemo_exit(void)	//模块卸载
{
	printk(KERN_NOTICE "End cdevdemo");
	
	del_timer(&tm);			//注销定时器
	
	cdev_del(&cdevdemo_devp->cdev);		//注销cdev
	kfree(cdevdemo_devp);				//释放设备结构体内存
	unregister_chrdev_region(MKDEV(cdevdemo_major, 0), 1);		//释放设备号
}

MODULE_LICENSE("Dual BSD/GPL");
module_param(cdevdemo_major, int, S_IRUGO);
module_init(cdevdemo_init);
module_exit(cdevdemo_exit);