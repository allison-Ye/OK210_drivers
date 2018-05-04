#include <linux/init.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

//定义按键硬件相关的数据结构
struct button_resource {
    int irq;    //中断号
    char *name; //中断名称
};

//初始化按键信息
static struct button_resource btn_info[] = {
    [0] = {
        .irq = IRQ_EINT(0),
        .name = "KEY_UP"
    },
    [1] = {
        .irq = IRQ_EINT(1),
        .name = "KEY_DOWN"
    },
	[2] = {
        .irq = IRQ_EINT(2),
        .name = "KEY_LEFT"
    },
	[3] = {
        .irq = IRQ_EINT(3),
        .name = "KEY_RIGHT"
    },
	[4] = {
        .irq = IRQ_EINT(4),
        .name = "KEY_ENTER"
    },
	[5] = {
        .irq = IRQ_EINT(5),
        .name = "KEY_ESC"
    },
	[6] = {
        .irq = IRQ_EINT(6),
        .name = "KEY_Reset"
    },
	[7] = {
        .irq = IRQ_EINT(7),
        .name = "KEY_power"
    }
};

static int mydata = 0x55;

//tasklet 处理函数
static void button_do_tasklet(unsigned long data)
{
	int *pdata = (int *)data;
	printk("%s: mydtat = %#x\n", __func__, *pdata);	
}
//1.分配和初始化tasklet

//paramater1:名称为button_tasklet
//paramater2:处理函数 button_do_tasklet
//paramater3:传递的参数mydata，注意这里用了unsigned long 做了一个类型的强转换
static DECLARE_TASKLET(button_tasklet, button_do_tasklet, (unsigned long)&mydata);

//中断处理函数就是顶半部
static irqreturn_t button_isr(int irq, void *dev_id)
{
    //1.登记底半部，CPU会在适当的时候执行相应的处理函数
	tasklet_schedule(&button_tasklet);
	//tasklet_hi_schedule(&button_tasklet); //高优先级的
	printk("%s is handled\n", __func__);	
    return IRQ_HANDLED; //处理完毕
}

static int btn_init(void)
{
    int i;

    printk("register irq!\n");

    for (i = 0; i < ARRAY_SIZE(btn_info); i++)
        request_irq(btn_info[i].irq, button_isr, 
                IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
                btn_info[i].name, &btn_info[i]);
    
    return 0;
}

static void btn_exit(void)
{
    int i;

    printk("unregister irq!\n");

    //注意注册中断传递的参数和释放中断传递的参数一定要一致!
    for(i = 0; i < ARRAY_SIZE(btn_info); i++)
        free_irq(btn_info[i].irq, &btn_info[i]);
}
module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
