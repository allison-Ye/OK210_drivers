#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


int my_probe(struct device *dev)
{
    printk("driver found the device，let's rock!\n");
    return 0;
}

extern struct bus_type my_bus_type;//需要在bus.c里面把这个设置为外部变量

struct device_driver my_driver = {
    .name = "my_dev",
    .bus = &my_bus_type,	
    .probe = my_probe,
};

int my_driver_init()
{
	int ret;
	
	ret = driver_register(&my_driver);
	
	return ret;
}


void my_driver_exit()
{
	driver_unregister(&my_driver);	
}


MODULE_LICENSE("GPL");
module_init(my_driver_init);
module_exit(my_driver_exit);