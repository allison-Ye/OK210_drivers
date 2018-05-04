/*xxx 设备结构体*/
struct xxx_dev
{
struct cdev cdev;
...
struct timer_list xxx_timer;/*设备要使用的定时器*/
};

/*xxx 驱动中的某函数*/
 xxx_func1(...)
 {
 struct xxx_dev *dev = filp->private_data;
 ...
 /*初始化定时器*/
 init_timer(&dev->xxx_timer);
 dev->xxx_timer.function = &xxx_do_timer;
 dev->xxx_timer.data = (unsigned long)dev;
 /*设备结构体指针作为定时器处理函数参数*/
 dev->xxx_timer.expires = jiffies + delay;
 /*添加（注册）定时器*/
 add_timer(&dev->xxx_timer);
 ...
 }

 /*xxx 驱动中的某函数*/
 xxx_func2(…)
 {
 ...
 /*删除定时器*/
 del_timer (&dev->xxx_timer);
 ...
 }

 /*定时器处理函数*/
 static void xxx_do_timer(unsigned long arg)
 {
 struct xxx_device *dev = (struct xxx_device *)(arg);
 ...
 /*调度定时器再执行*/
 dev->xxx_timer.expires = jiffies + delay;
 add_timer(&dev->xxx_timer);
 ...
 }