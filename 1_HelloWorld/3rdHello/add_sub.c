#include<linux/init.h>
#include<linux/module.h>

int add_int(int x,int y)
{
	return (x+y);
}


int sub_int(int x,int y)
{
	return (x-y);
}

/*将某个变量或者函数导入到内核符号表*/
EXPORT_SYMBOL(add_int);
EXPORT_SYMBOL(sub_int);

MODULE_LICENSE("GPL");

