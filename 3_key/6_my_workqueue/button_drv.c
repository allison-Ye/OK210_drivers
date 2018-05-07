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
//分配工作（或者延时工作）
static struct work_struct btn_work;
static struct delayed_work btn_dwork;

//1.分配工作队列的指针
static struct workqueue_struct *btn_wq;

//工作处理函数
static void btn_work_func(struct work_struct *work)
{
    printk("%s: %#x\n", __func__, work);
}

//延时工作处理函数
static void btn_dwork_func(struct work_struct *work)
{
    printk("%s: %#x\n", __func__, work);
}

//中断处理函数就是顶半部
static irqreturn_t button_isr(int irq, void *dev_id)
{
    //将自己的工作和自己的工作队列进行关联，然后再登记
    queue_work(btn_wq, &btn_work);

    //将自己延时的工作和自己的工作队列进行关联，然后再登记
    queue_delayed_work(btn_wq, &btn_dwork, 3*HZ);
	
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
	//初始化工作或延时的工作
    INIT_WORK(&btn_work, btn_work_func); //指定工作处理函数
    INIT_DELAYED_WORK(&btn_dwork, btn_dwork_func); //指定延时工作处理函数
    printk("%s: %#x, %#x\n", 
            __func__, &btn_work, &btn_dwork);
			
	//创建自己的工作队列和自己的内核线程
    btn_wq = create_workqueue("mybuttons");
    
    return 0;
}

static void btn_exit(void)
{
    int i;

    printk("unregister irq!\n");

    //注意注册中断传递的参数和释放中断传递的参数一定要一致!
    for(i = 0; i < ARRAY_SIZE(btn_info); i++)
        free_irq(btn_info[i].irq, &btn_info[i]);
	
	//销毁自己的工作队列和内核线程
    destroy_workqueue(btn_wq);
}
module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
