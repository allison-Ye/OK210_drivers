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

/*��ĳ���������ߺ������뵽�ں˷��ű�*/
EXPORT_SYMBOL(add_int);
EXPORT_SYMBOL(sub_int);

MODULE_LICENSE("GPL");

