#include<linux/init.h>
#include<linux/module.h>

/*1.实现模块加载函数,执行insmod xxx.ko就会执行此函数*/
static int __init hello_init(void)
{
	printk(KERN_INFO "%s()-%d:\n",__func__,__LINE__);
	return 0;
}

/*2.实现模块卸载，执行rmmod xxx就会执行此函数*/
static void __exit hello_exit(void)
{
	printk(KERN_INFO "%s()-%d:\n",__func__,__LINE__);
}

/*3.申明加载函数与卸载函数*/
module_init(hello_init);
module_exit(hello_exit);

/*4.权限许可声明*/
MODULE_LICENSE("GPL");

