#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/uaccess.h>

//定义按键硬件的相关数据结构
struct button_resource{
	int gpio;
	char *name;
};

//初始化按键相关的硬件资源

static struct button_resource button_info[]= {
	[0] = {
		.gpio = S5PV210_GPH0(0),
		.name = "key_up"
	}
};

static int major;
static struct cdev button_cdev;
static struct class *cls;  //设备类指针

static ssize_t button_read(struct file *file,
                        char __user *buf,
                        size_t count,
                        loff_t *ppos)
{
    unsigned int pinstate;
    unsigned char key_val;

    //1.获取按键的状态
    pinstate = gpio_get_value(button_info[0].gpio);

    //2.判断按键的状态，上报对应的键值
    if (pinstate == 0) { //按下
        key_val = 0x01;
    } else {    //松开
        key_val = 0x00;
    }

    copy_to_user(buf, &key_val, 1);
    return count;
}
static struct file_operations button_fops = {
	.owner = THIS_MODULE,
	.read  = button_read
};

static int button_init()
{	
	dev_t dev;
	int ret;
	int i;
	//1.申请设备号
	ret = alloc_chrdev_region(&dev,0,1,"buttons");
	if(ret == 0)
		printk("succed to get a device number\n");
	else{
		printk("failed to get a device number\n");
	}
		
	major = MAJOR(dev);
	
	//2.初始化注册cdev
	cdev_init(&button_cdev,&button_fops);
	ret = cdev_add(&button_cdev,dev,1);
	if(ret == 0)
		printk("succed to add a char device\n");
	else{
		printk("failed to add a char device\n");
	}
	
	//3.自动创建设备节点
	cls = class_create(THIS_MODULE,"buttons");
	device_create(cls,NULL,dev,NULL,"mybuttons");  //dev/mybuttons
	
	//4.申请GPIO资源和配置GPIO为输入口
	for (i = 0;i < ARRAY_SIZE(button_info); i++){
		gpio_request(button_info[i].gpio,button_info[i].name);
		gpio_direction_input(button_info[i].gpio);
	}
	return 0;
}

static void button_exit()
{
	dev_t dev = MKDEV(major, 0);
	int i;
	//1.释放GPIO资源
	for (i = 0;i < ARRAY_SIZE(button_info); i++){
		gpio_free(button_info[i].gpio);
	}
	
	//2.删除设备节点
	device_destroy(cls, dev);
	class_destroy(cls);
	
	//3.卸载cdev
	cdev_del(&button_cdev);
	
	//4.释放设备号
	unregister_chrdev_region(dev,1);
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");