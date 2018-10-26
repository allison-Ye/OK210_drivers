#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "led.h"
//#define LED_ON  0x100001
//#define LED_OFF 0x100002

struct led_resource {
    char *name;
    int productid;
};
static void *gpio_base; //寄存器的虚拟起始地址
static int pin; //操作的管脚编号
static unsigned long *gpiocon, *gpiodata;



static int major;
static struct cdev led_cdev;
static struct class *cls;


static int led_ioctl(struct inode *inode,
                        struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    switch(cmd) {
        case LED_ON:
                    *gpiodata |= (1 << pin);
                break;
        case LED_OFF:
                    *gpiodata &= ~(1 << pin);
                break;
        default:
                return -1;
    }
    printk("GPIOCON = %#x, GPIODATA = %#x\n", 
                            *gpiocon, *gpiodata);
    return 0;
}


static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .ioctl = led_ioctl
};


//led_probe被执行说明硬件和软件匹配成功
//pdev指向匹配成功的led_dev硬件信息
static int led_probe (struct platform_device *pdev)
{
    //1.通过pdev获取硬件信息
    struct resource *reg_res; //寄存器
    struct resource *pin_res; //GPIO编号
    int size;
    dev_t dev;

    //1.1获取硬件私有信息(第一种获取资源的办法)
    struct led_resource *pled = 
                        pdev->dev.platform_data;
    printk("Name = %s, Productid = %#x\n",
                    pled->name, pled->productid);
    
    //1.2获取resource资源类型（第二种获取资源的办法）
    reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0); 
    pin_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    size = reg_res->end - reg_res->start + 1;

    //2.处理硬件信息 
	
    //2.1地址映射
    gpio_base = ioremap(reg_res->start, size);
    gpiocon = (unsigned long *)gpio_base;
    gpiodata = (unsigned long *)(gpio_base + 0x04);
    
    pin = pin_res->start;
    printk("pin = %d\n", pin);
   
    //2.2配置GPIO为输出口，并且输出0
    *gpiocon &= ~(0xf << (pin*4));
    *gpiocon |= (1 << (pin*4));
    *gpiodata &= ~(1 << pin);

    //3.注册字符设备驱动
    //3.1申请设备号
    alloc_chrdev_region(&dev, 0, 1, "leds");
    major = MAJOR(dev);

    //3.2初始化注册cdev
    cdev_init(&led_cdev, &led_fops);
    cdev_add(&led_cdev, dev, 1);

    //3.3自动创建设备节点
    cls = class_create(THIS_MODULE, "leds");
    device_create(cls, NULL, dev, NULL, "myled");
    return 0;//成功返回0，失败返回负值
}

//卸载platform_device或者卸载platform_driver调用
//pdev指向匹配成功的led_dev硬件信息
static int led_remove(struct platform_device *pdev)
{
    dev_t dev = MKDEV(major, 0);
    //1.删除设备节点
    device_destroy(cls, dev);
    class_destroy(cls);

    //2.卸载cdev
    cdev_del(&led_cdev);
    
    //3.释放设备号
    unregister_chrdev_region(dev, 1);

    //4.解除地址映射
    iounmap(gpio_base);
    return 0;//成功返回0，失败返回负值
}

static struct platform_driver led_drv = {
    .driver = {
        .name = "myled" //必须有
    },
    .probe = led_probe, //匹配成功调用
    .remove = led_remove //卸载硬件或者软件调用
};

static int led_drv_init(void)
{
    //注册platform_driver
    platform_driver_register(&led_drv);
    return 0;
}

static void led_drv_exit(void)
{
    //卸载platform_driver
    platform_driver_unregister(&led_drv);
}
module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");
