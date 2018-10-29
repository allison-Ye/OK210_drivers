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
    int irq;		/* �жϺ� */
    int pin;		/* GPIO���� */
    int key_val;	/* ������ʼֵ */
    char *name;		/* ���� */
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

/* �û��жϴ����� */
static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	irq_pd = (struct button_irq_desc *)dev_id;
	
	/* �޸Ķ�ʱ����ʱʱ�䣬��ʱ10ms����10���������ʱ��
	 * HZ ��ʾ100��jiffies��jiffies�ĵ�λΪ10ms����HZ = 100*10ms = 1s
	 * ����HZ/100����ʱ10ms
	 */
	mod_timer(&buttons_timer, jiffies + (HZ /100));
	return IRQ_HANDLED;
}


/* ��ʱ���жϴ����� */
static void buttons_timer_function(unsigned long data)
{
	struct button_irq_desc *button_irqs = irq_pd;
	unsigned int pinval;

	pinval = gpio_get_value(button_irqs->pin);

	if(pinval)
	{
		/* �ɿ�, ���һ������: 0-�ɿ�, 1-����
		 * �ϱ��¼�,���ջ���õ�evdev.c���evdev_events����
		 */
		input_event(buttons_dev,EV_KEY,button_irqs->key_val,0);
		input_sync(buttons_dev);
	}
	else
	{
		/* ����, ���һ������: 0-�ɿ�, 1-����
		 * �ϱ��¼�,���ջ���õ�evdev.c���evdev_events����
		 */
		input_event(buttons_dev,EV_KEY,button_irqs->key_val,1);
		input_sync(buttons_dev);
	}
}

/* ������ں��� */
static int __init buttons_input_init(void)
{
	int i;
	int err;
	
	/* 1.����һ��input_dev�ṹ�� */
	buttons_dev = input_allocate_device();

	/* 2.���� */
	/* 2.1 ���ð����ܲ��������¼� */
	set_bit(EV_KEY,buttons_dev->evbit);  /* ֧�ְ������¼� */
	set_bit(EV_REP,buttons_dev->evbit);	 /* ֧���ظ����¼� */

	/* 2.2 �����ܲ��������������Щ�¼� */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		set_bit(button_irqs[i].key_val,buttons_dev->keybit);
	}
	
	/* 3.ע�� */
	err = input_register_device(buttons_dev);
	if(err)
	{
		printk("Unable to register input device, error: %d\n",err);
		goto fail;
	}

	/* 4.Ӳ����ص����� */
	/* 4.1 ��ʱ����صĲ��� */
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	add_timer(&buttons_timer);

	/* 4.2 �����ж� */  
	/* ʹ��request_irq����ע���ж� */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		err = request_irq(button_irqs[i].irq, key_interrupt, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
                          button_irqs[i].name, (void *)&button_irqs[i]);
	}
	/* ע���ж�ʧ�ܴ��� */
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

/* �������ں��� */
static void buttons_input_exit(void)
{
	int i;
	/* ע���ж� */
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++)
	{
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}
	/* ж�ض�ʱ�� */
	del_timer(&buttons_timer);
	/* ע�����������豸 */
	input_unregister_device(buttons_dev);
	/* �ͷ�input_dev�ṹ�� */
	input_free_device(buttons_dev);
}

module_init(buttons_input_init);  
module_exit(buttons_input_exit);  

MODULE_LICENSE("GPL");
MODULE_AUTHOR("webee");
MODULE_DESCRIPTION("Input subsystem hardware drivers for key");