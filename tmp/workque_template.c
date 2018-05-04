/*定义工作队列和关联函数*/
struct work_struct xxx_wq;
void xxx_do_work(unsigned long);

/*中断处理底半部*/
void xxx_do_work(unsigned long)
{
	...
}

 /*中断处理顶半部*/
 irqreturn_t xxx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
 {
	...
	schedule_work(&xxx_wq);
	...
	return IRQ_HANDLED;
 }

 /*设备驱动模块加载函数*/
 int xxx_init(void)
 {
	...
	/*申请中断*/
	result = request_irq(xxx_irq, xxx_interrupt,
	SA_INTERRUPT, "xxx", NULL);
	...
	/*初始化工作队列*/
	INIT_WORK(&xxx_wq, (void (*)(void *)) xxx_do_work, NULL);
	...
 }

 /*设备驱动模块卸载函数*/
 void xxx_exit(void)
 {
	...
	/*释放中断*/
	free_irq(xxx_irq, xxx_interrupt);
	...
 }