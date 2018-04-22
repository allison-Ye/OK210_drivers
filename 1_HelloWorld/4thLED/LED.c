#include <linux/init.h>
#include <linux/module.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>

//定义GPIO管脚的硬件数据结构
struct led_resource {
    int gpio; //描述GPIO的软件编号
    char *name;//标签，名称
};

//初始化LED灯对应的GPIO管脚信息
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

static int led_init(void)
{
    int i;

    //1.申请GPIO资源
    //2.配置GPIO为输出口，输出1
    for (i = 0; i < ARRAY_SIZE(led_info); i++) {
        gpio_request(led_info[i].gpio, led_info[i].name);
        gpio_direction_output(led_info[i].gpio, 0);
    }
    return 0;
}

static void led_exit(void)
{
    int i;

    //1.设置GPIO为输出口，输出0
    //2.释放GPIO资源
    for (i = 0; i < ARRAY_SIZE(led_info); i++) {
        gpio_direction_output(led_info[i].gpio, 1);
        gpio_free(led_info[i].gpio);
    }
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");

