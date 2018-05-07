#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>

//分配定时器
static struct timer_list mytimer;
static int mydata = 0x5555;

//超时处理函数
static void mytimer_func(unsigned long data)
{
    int *pdata = (int *)data;
    
    printk("hello,kernel, data = %#x\n", *pdata);

    //重新添加定时器,如果不添加，定时器处理函数只执行一次
    mod_timer(&mytimer, jiffies + msecs_to_jiffies(2000));
}

static int mytimer_init(void)
{
    //初始化定时器
    init_timer(&mytimer);
    //指定定时器超时时候的时间
    mytimer.expires = jiffies + msecs_to_jiffies(2000); //2s
    mytimer.function = mytimer_func; //超时处理函数
    mytimer.data = (unsigned long)&mydata; //给超时处理函数传递参数

    //启动定时器
    add_timer(&mytimer);
    
    printk("add timer ok!\n");
    return 0;
}

static void mytimer_exit(void)
{
    del_timer(&mytimer); 
    printk("del timer ok!\n");
}
module_init(mytimer_init);
module_exit(mytimer_exit);
MODULE_LICENSE("GPL");
