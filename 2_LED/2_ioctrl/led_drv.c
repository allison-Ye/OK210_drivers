#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>		//file_operations
#include<linux/device.h>	//deivce_create
#include<linux/slab.h>		//kmalloc
#include<asm/uaccess.h>		//copy_to_user copy_form_user
#include<asm/io.h>			//ioremap iounmap
#include "led.h"

static struct led_device *OK210_led_dev;

static int led_open(struct inode *inode,struct file *file)
{
	unsigned long val=0;
	/*初始化硬件*/
	/*将GPH2[0-3]设置为输出*/
//	val =readl(OK210_led_dev->led_base);  //读gph2con寄存器
//	val &=0xFFFF0000;
	val = 0xFFFF1111;
	writel(val,OK210_led_dev->led_base+OK210_GPH2CON);

	return 0;
}

static int led_close(struct inode *inode,struct file *file)
{
	//iounmap(gpc0con);
	
	return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t size, loff_t *opps)
{	
	return 0;
}

//write(fd,buf,4)
static ssize_t led_write(struct file *file, const char __user *buf, size_t size, loff_t *opps)
{
	int ret=0;
	unsigned int val=0;
	/*获取用户空间的数据
     *param1:目的
     *param2:源
     *param3:大小
     *返回值:失败返回>0,返回剩余的没有拷贝成功的字节数
     *       成功返回0
	 */
	ret =copy_from_user(&val,buf,size);
	if(ret)
		printk(KERN_ERR "copy data from user failed!\n");

//	if(val){
		/*点灯*/
//		writel(readl(OK210_led_dev->led_base+OK210_GPH2DAT)|(0x1<<4)|(0x1<<5)|(0x1<<6)|(0x1<<7),OK210_led_dev->led_base+OK210_GPH2DAT);
//	}else{
//		/*灭灯*/
//		writel(readl(OK210_led_dev->led_base+OK210_GPH2DAT)&~((0x1<<4)|(0x1<<5)|(0x1<<6)|(0x1<<7)),OK210_led_dev->led_base+OK210_GPH2DAT);
//	}

	return ret?-EINVAL:size;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long val =arg;
	printk("cmd =%d,val=%ld\n",cmd,val);
	switch(cmd){
		case LED_OFF:
			/*OFF灯*/
			writel(readl(OK210_led_dev->led_base+OK210_GPH2DAT)|(0x1<<val),OK210_led_dev->led_base+OK210_GPH2DAT);
			break;
		case LED_ON:
			/*ON灯*/
			writel(readl(OK210_led_dev->led_base+OK210_GPH2DAT)&~(0x1<<val),OK210_led_dev->led_base+OK210_GPH2DAT);
			break;
		default:
			break;
	}
	return 0;
}

struct file_operations led_fops ={
	.owner			=THIS_MODULE,
	.open			=led_open,
	.release		=led_close,
	.write 			=led_write,
	.read			=led_read,
	.unlocked_ioctl 	=led_ioctl,
};

static int __init led_dev_init(void)
{
	int ret;

	/*1.为本地结构体分配空间*/
	OK210_led_dev =kmalloc(sizeof(struct led_device),GFP_KERNEL);
	if(OK210_led_dev==NULL){
		printk(KERN_ERR "no memory for alloc!\n");
		return -ENOMEM;
	}
	
	/*2.注册*/
	ret =register_chrdev(LED_MAJOR,"led_module",&led_fops);
	if(ret<0){
		printk(KERN_ERR "register major failed!\n");
		ret= -EINVAL;
		goto err1;
	}

	/*3.创建设备文件*/
	OK210_led_dev->led_class =class_create(THIS_MODULE, "led_class");
	if(IS_ERR(OK210_led_dev->led_class)){
		printk(KERN_ERR "failed to create class!\n");
		ret =PTR_ERR(OK210_led_dev->led_class);
		goto err2;
	}

	OK210_led_dev->led_device =device_create(OK210_led_dev->led_class,NULL,MKDEV(LED_MAJOR,0),NULL,"led");
	if(IS_ERR(OK210_led_dev->led_device)){
		printk(KERN_ERR "failed to create class!\n");
		ret =PTR_ERR(OK210_led_dev->led_device);
		goto err3;
	}

	/*4.将物理地址映射为虚拟地址*/
	OK210_led_dev->led_base=ioremap(OK210_PA_GPH2CON, 8);
	if(OK210_led_dev->led_base==NULL){
		printk(KERN_ERR "no memory for ioremap!\n");
		ret =-ENOMEM;
		goto err4;
	} 
	
	return 0;
err4:
	device_destroy(OK210_led_dev->led_class,MKDEV(LED_MAJOR,0));
err3:
	class_destroy(OK210_led_dev->led_class);
err2:
	unregister_chrdev(LED_MAJOR,"led_module");
err1:
	kfree(OK210_led_dev);
	return ret;
}

static void __exit led_dev_exit(void)
{
	unregister_chrdev(LED_MAJOR,"led_module");
	device_destroy(OK210_led_dev->led_class,MKDEV(LED_MAJOR,0));
	class_destroy(OK210_led_dev->led_class);
	iounmap(OK210_led_dev->led_base);
	kfree(OK210_led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");



