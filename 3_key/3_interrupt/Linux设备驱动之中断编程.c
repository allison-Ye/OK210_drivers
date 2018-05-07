linux设备驱动之中断编程
========================================================
如果想用中断:
1) 申请中断/注册中断
	int request_irq(unsigned int irq,irq_handler_t handler,unsigned long flags,const char * name,void * dev)
   /*param1:执行要申请的中断号:芯片厂商做了对应的宏定义 plat/irqs.h ,mach/irqs.h  linux/irqs.h
    *param2:中断服务程序  : typedef irqreturn_t (*irq_handler_t)(int, void *);
	*param3:标号:中断触发方式  linux/interrupt.h
	*			 #define IRQF_TRIGGER_NONE	0x00000000       //对于内部中断,比如定时器中断或者RTC中断...
	*			 #define IRQF_TRIGGER_RISING	0x00000001	 //上升沿触发
	*			 #define IRQF_TRIGGER_FALLING	0x00000002	 //下降沿触发
	*			 #define IRQF_TRIGGER_HIGH	0x00000004		 //高电平触发
	*			 #define IRQF_TRIGGER_LOW	0x00000008		 //低电平触发
	*param4:中断名字
	*param5:中断id号,共享中断的时候才会用.此数据最后会传给中断服务程序
	*返回值:成功0,失败返回一个非0值
    */
   request_irq(IRQ_EINT(0), buttons_irq_func,IRQF_TRIGGER_FALLING,
	    const char *name, void *dev)

2) 实现中断服务程序
   /*param1:中断号
    *param2:中断id号
	*返回值:中断处理成功则返回IRQ_HANDLED,处理失败返回IRQ_NONE
	*	enum irqreturn {                 linux/irqreturn.h
	*		IRQ_NONE		= (0 << 0),
	*		IRQ_HANDLED		= (1 << 0),
	*		IRQ_WAKE_THREAD		= (1 << 1),
	*	};
	*
	*	typedef enum irqreturn irqreturn_t;
    */


   irqreturn_t buttons_irq_func(int irq, void *dev_id)
   {
		//处理
		return IRQ_HANDLED;
   }
   参数1: 中断号

3) 释放中断
   /*param1:中断号
    *param2:id号
	*/	
   void free_irq(unsigned int irq, void *dev_id);