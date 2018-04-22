#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/uaccess.h>

//定义描述LED硬件相关的数据结构
struct led_resource {
    int gpio;
    char *name; 
};

//初始化LED灯的资源信息
static struct led_resource led_info[] = {
    [0] = {
        .gpio = S5PV210_GPH2(0),
        .name = "LED1"
    	},
    [1] = {
        .gpio = S5PV210_GPH2(1),
        .name = "LED2"
   	 },
	
    [2] = {
        .gpio = S5PV210_GPH2(2),
	.name = "LED3"
    	},
    [3] = {
        .gpio = S5PV210_GPH2(3),
	.name = "LED4"
	}
};

static int major;
static struct cdev led_cdev; //分配一个字符设备对象
static int led_status; //记录灯的开关状态

static ssize_t led_read(struct file *file,
                        char __user *buf,
                        size_t count,
                        loff_t *ppos)
{
    copy_to_user(buf, &led_status, sizeof(led_status));
    printk("%s\n", __func__);
    return count; //返回读取的字节数
}

static ssize_t led_write(struct file *file,
                      const char __user *buf, //对应用户的第二个参数
                        size_t count, //对应用户的第三个参数
                        loff_t *ppos //文件指针
                        )
{
    int cmd; //内核缓冲区
    int i;

    //1.拷贝用户缓冲区的数据到内核空间
    copy_from_user(&cmd, buf, sizeof(cmd));

    printk("%s: cmd = %d\n", __func__, cmd);
    //2.判断解析cmd
    if (cmd == 1) {
        for (i = 0; i < ARRAY_SIZE(led_info); i++) 
            gpio_set_value(led_info[i].gpio, 0);
    } else if (cmd == 0) {
        for (i = 0; i < ARRAY_SIZE(led_info); i++) 
            gpio_set_value(led_info[i].gpio, 1);
    }

    //3.记录灯的开关状态
    led_status = cmd;

    return count; //返回写入的字节数
}

//分配底层驱动操作集合
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .read = led_read,   //获取灯的开关状态
    .write = led_write  //控制灯
};

static int led_init(void)
{
    dev_t dev;
    int i;

    //1.申请设备号
    alloc_chrdev_region(&dev, 0, 1, "leds");
    major = MAJOR(dev); //提取主设备号

    //2.初始化cdev
    cdev_init(&led_cdev, &led_fops);

    //3.向内核注册cdev
    cdev_add(&led_cdev, dev, 1);

    //4.向内核申请GPIO资源和配置GPIO为输出口，输出低电平
    for (i = 0; i < ARRAY_SIZE(led_info); i++) {
        gpio_request(led_info[i].gpio, led_info[i].name);
        gpio_direction_output(led_info[i].gpio, 0);
    }
    return 0;
}

static void led_exit(void)
{
    int i;

    //1.配置GPIO为输出口，输出低电平，释放GPIO资源
    for (i = 0; i < ARRAY_SIZE(led_info); i++) {
        gpio_set_value(led_info[i].gpio, 1);
        gpio_free(led_info[i].gpio);
    }

    //2.卸载cdev
    cdev_del(&led_cdev);

    //3.释放设备号
    unregister_chrdev_region(MKDEV(major, 0), 1);
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");

