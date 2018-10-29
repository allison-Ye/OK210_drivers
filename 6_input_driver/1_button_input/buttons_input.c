/*
 * Name:buttons_input.c
 * Copyright (C) 2014 Webee.JY  (2483053468@qq.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/device.h> 		
#include <mach/regs-gpio.h>	
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/interrupt.h>  
#include <linux/poll.h>   
#include <linux/fcntl.h>
#include <linux/input.h>


struct button_irq_desc {
    int irq;		/* 中断号 */
    int pin;		/* GPIO引脚 */
    int key_val;	/* 按键初始值 */
    char *name;		/* 名字 */
};

static struct button_irq_desc button_irqs [] = {
	{IRQ_EINT(16), S5PV210_GPH2(0), KEY_A, 		   "S1"}, /* S1 */
	{IRQ_EINT(17), S5PV210_GPH2(1), KEY_B, 		   "S2"}, /* S2 */
	{IRQ_EINT(18), S5PV210_GPH2(2), KEY_C, 		   "S3"}, /* S3 */
	{IRQ_EINT(19), S5PV210_GPH2(3), KEY_L, 		   "S4"}, /* S4 */
	
	{IRQ_EINT(24), S5PV210_GPH3(0), KEY_S, 		   "S5"}, /* S5 */
	{IRQ_EINT(25), S5PV210_GPH3(1), KEY_ENTER, 	   "S6"}, /* S6 */
	{IRQ_EINT(26), S5PV210_GPH3(2), KEY_LEFTSHIFT, "S7"}, /* S7 */
	{IRQ_EINT(27), S5PV210_GPH3(3), KEY_DELETE,    "S8"}, /* S8 */
};

static struct button_irq_desc *irq_pd;
static struct input_dev *buttons_dev;
static struct timer_list buttons_timer;

/* 用户中断处理函数 */
static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	irq_pd = (struct button_irq_desc *)dev_id;
	
	/* 修改定时器定时时间，定时10ms，即10秒后启动定时器
	 * HZ 表示100个jiffies，jiffies的单位为10ms，即HZ = 100*10ms = 1s
	 * 这里HZ/100即定时10ms
	 */
	mod_timer(&buttons_timer, jiffies + (HZ /100));
	return IRQ_HANDLED;
}


/* 定时器中断处理函数 */
static void buttons_timer_function(unsigned long data)
{
	struct button_irq_desc *button_irqs = irq_pd;
	unsigned int pinval;

	pinval = gpio_get_value(button_irqs->pin);

	if(pinval)
	{
		/* 松开, 最后一个参数: 0-松开, 1-按下
		 * 上报事件,最终会调用到evdev.c里的evdev_events函数
		 */
		input_event(buttons_dev,EV_KEY,button_irqs->key_val,0);
		input_sync(buttons_dev);
	}
	else
	{
		/* 按下, 最后一个参数: 0-松开, 1-按下
		 * 上报事件,最终会调用到evdev.c里的evdev_events函数
		 */
		input_event(buttons_dev,EV_KEY,button_irqs->key_val,1);
		input_sync(buttons_dev);
	}
}

/* 驱动入口函数 */
static int __init buttons_input_init(void)
{
	int i;
	int err;
	
	/* 1.分配一个input_dev结构体 */
	buttons_dev = input_allocate_device();

	/* 2.设置 */
	/* 2.1 设置按键能产生哪类事件 */
	set_bit(EV_KEY,buttons_dev->evbit);  /* 支持按键类事件 */
	set_bit(EV_REP,buttons_dev->evbit);	 /* 支持重复类事件 */

	/* 2.2 设置能产生这类操作的哪些事件 */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		set_bit(button_irqs[i].key_val,buttons_dev->keybit);
	}
	
	/* 3.注册 */
	err = input_register_device(buttons_dev);
	if(err)
	{
		printk("Unable to register input device, error: %d\n",err);
		goto fail;
	}

	/* 4.硬件相关的设置 */
	/* 4.1 定时器相关的操作 */
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	add_timer(&buttons_timer);

	/* 4.2 申请中断 */  
	/* 使用request_irq函数注册中断 */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		err = request_irq(button_irqs[i].irq, key_interrupt, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
                          button_irqs[i].name, (void *)&button_irqs[i]);
	}
	/* 注册中断失败处理 */
	if (err)
	{
		i--;
		for (; i >= 0; i--)
		{
			disable_irq(button_irqs[i].irq);
			free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
		}
		return -EBUSY;
	}
	return 0;
 fail:
 	input_free_device(buttons_dev);
 	return err;
	
}

/* 驱动出口函数 */
static void buttons_input_exit(void)
{
	int i;
	/* 注销中断 */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}
	/* 卸载定时器 */
	del_timer(&buttons_timer);
	/* 注销按键输入设备 */
	input_unregister_device(buttons_dev);
	/* 释放input_dev结构体 */
	input_free_device(buttons_dev);
}

module_init(buttons_input_init);  
module_exit(buttons_input_exit);  

MODULE_LICENSE("GPL");
MODULE_AUTHOR("webee");
MODULE_DESCRIPTION("Input subsystem hardware drivers for key");