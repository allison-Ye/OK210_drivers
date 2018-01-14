#include<linux/init.h>
#include<linux/module.h>
#include"add_sub.h"

static int a =3;
static int b =2;
static int c =1;
static char *str ="hello Allison";

/*1.实现模块加载函数,执行insmod xxx.ko就会执行此函数*/
static int __init hello_init(void)
{
	printk(KERN_INFO "%s()-%d:\n",__func__,__LINE__);
	if(c>0){
		printk(KERN_INFO "result =%d\n",add_int(a,b));
	}else{
		printk(KERN_INFO "result =%d\n",sub_int(a,b));
	}
	return 0;
}

/*2.实现模块卸载，执行rmmod xxx就会执行此函数*/
static void __exit hello_exit(void)
{
	printk(KERN_INFO "%s()-%d:\n",__func__,__LINE__);
}

/*param1:变量
  *param2:类型
  *param3:权限
  */
module_param(a,int,S_IRWXU);
module_param(b,int,S_IRWXU);
module_param(c,int,S_IRWXU);
module_param(str,charp,S_IRWXU);


/*3.申明加载函数与卸载函数*/
module_init(hello_init);
module_exit(hello_exit);

/*4.权限许可声明*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allison");
MODULE_VERSION("V0.1");
MODULE_DESCRIPTION("this is a simple module");



