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

static irqreturn_t button_isr(int irq, void *dev_id)
{
    struct button_resource *pdata = 
                (struct button_resource *)dev_id;

    printk("%s: irq = %d, name = %s\n", 
            __func__, pdata->irq, pdata->name);

    return IRQ_HANDLED; //处理完毕
}

static int btn_init(void)
{
    int i;

    printk("register irq!\n");

    for (i = 0; i < ARRAY_SIZE(btn_info); i++)
        request_irq(btn_info[i].irq, button_isr, 
                IRQF_TRIGGER_RISING,
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
