#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>


int my_match(struct device *dev, struct device_driver *drv)
{
    return !strncmp(dev->kobj.name,drv->name,strlen(drv->name));
}  


struct bus_type my_bus_type = {
	.name = "my_bus",
	.match = my_match,
	};
	
EXPORT_SYMBOL(my_bus_type);

int my_bus_init()
{
	int ret;
	
	ret = bus_register(&my_bus_type);
	
	return ret;

}

void my_bus_exit()
{
	bus_unregister(&my_bus_type);
}


module_init(my_bus_init);
module_exit(my_bus_exit);
MODULE_LICENSE("GPL");