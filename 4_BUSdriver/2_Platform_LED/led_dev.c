#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>


//定义自己私有的硬件相关的数据结构
struct led_resource {
    char *name; //厂家名称
    int productid; //设备ID号
};

//初始化LED灯的硬件资源信息
static struct led_resource led_info = {
    .name = "OK210",
    .productid = 0x12345678
};

//利用resource来制定LED额外的硬件资源信息
static struct resource led_res[] = {
    //寄存器硬件资源信息
    [0] = {
        .start = 0xe0200C40,
        .end = 0xe0200C40 + 8 - 1,
        .flags = IORESOURCE_MEM
    },
    //GPIO管脚编号信息
    [1] = {
        .start = 1,
        .end = 1,
        .flags = IORESOURCE_IRQ
    }
};

//给出release的实现，否则有警告
static void led_release(struct device *dev)
{
	printk("led_release is done!\n");
}

//分配初始化platform_device
static struct platform_device led_dev = {
    .name = "myled", //必须有
    .id = -1, //
    .dev = {
        .platform_data = &led_info, //指向私有的硬件信息
        .release = led_release
    },
    .resource = led_res, //存放resource描述的硬件信息
    .num_resources = ARRAY_SIZE(led_res) //资源个数
};


static int led_dev_init(void)
{
    //注册platform_device
    platform_device_register(&led_dev);
    return 0;
}

static void led_dev_exit(void)
{
    //卸载platform_device
    platform_device_unregister(&led_dev);
}


module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");