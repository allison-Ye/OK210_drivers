/*定义 tasklet 和底半部函数并关联*/
void xxx_do_tasklet(unsigned long);
DECLARE_TASKLET(xxx_tasklet, xxx_do_tasklet, 0);

/*中断处理底半部*/
void xxx_do_tasklet(unsigned long)
{
	...
}
 /*中断处理顶半部*/
 irqreturn_t xxx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
 {
	...
	tasklet_schedule(&xxx_tasklet);
	...
	return IRQ_HANDLED;
 }

 /*设备驱动模块加载函数*/
 int __init xxx_init(void)
 {
	...
	/*申请中断*/
	result = request_irq(xxx_irq, xxx_interrupt,SA_INTERRUPT, "xxx", NULL);
	...
 }

 /*设备驱动模块卸载函数*/
 void __exit xxx_exit(void)
 {
	...
	/*释放中断*/
	free_irq(xxx_irq, xxx_interrupt);
	...
 }