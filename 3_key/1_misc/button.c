#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <mach/gpio.h>


#define KEYCON (*(volatile unsigned long *)0xE0200C00)  //GPH0 control register 
#define KEYDAT (*(volatile unsigned long *)0xE0200C04)	//GPH0 data register

static irqreturn_t button_interrupt(int irq, void *dev_id)
{
	//1.检测是否发生中断
	
	//2.清除已发生的中断
	
	//3.打印按键值
	printk("key down!\n");
	
	return 0;
}

void button_HW_init(void)
{
	unsigned int *gpio_config;
	unsigned int data;
	gpio_config = ioremap(KEYCON,4);
	//读取硬件寄存器的值，清零设置后再写回寄存器
	data = readl(gpio_config);
	data &= ~0x0000000f;
	data |=0x0000000f;
	writel(data,gpio_config);
}

int button_open(struct inode *node, struct file *filp)
{
	return 0;
}

static struct file_operations button_fops =
{
	.open = button_open,
    //.unlocked_ioctl = led_ioctl,
};


struct miscdevice button_miscdev = {
	.minor = 2,
	.name  = "button",
	.fops  = &button_fops,
};

static int button_init(void)
{
	misc_register(&button_miscdev);  //注册混杂设备驱动
	printk("Register misdevice succeed\n");
//	button_HW_init();
	printk("HW init finished\n");
	int err;
	err = request_irq(IRQ_EINT0,button_interrupt,IRQF_TRIGGER_FALLING,"button",0 );//注册中断
	if(err)
	{
		printk("request IRQ failed!\n");
	}
    return 0;	
}

static void button_exit(void)
{
	misc_deregister(&button_miscdev);
	free_irq(IRQ_EINT(0),0);
}




module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
