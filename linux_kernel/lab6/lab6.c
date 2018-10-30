#include <linux/capability.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/cpu.h>


MODULE_AUTHOR("support@ingben.com");
MODULE_DESCRIPTION("Per-CPU test");
unsigned long my_percpu[NR_CPUS];

static int __init lab6_init(void)
{	
	int id = 0,node = -1;

	struct cpu *point = NULL;
	printk(KERN_INFO " lab6_init \n");
	printk(KERN_INFO "NR_CPUS = %d\n",NR_CPUS);
	id = get_cpu();//get current processor and disable kernel preemption    
	my_percpu[id]++; 
	printk(KERN_INFO "my_percpu on cpu=%d is %lu\n", id, my_percpu[id]);
 	put_cpu();//enable kernel preemption 

	point = smp_processor_id();//get the current processor number, kernel preemption is not disabledalways 
//	printk(KERN_INFO "current cpu id = %d\n",point->node_id); 
	node = cpu_to_node(point);
	printk(KERN_INFO " current cpu node is %d\n",node);
	return 0;
}



static void __exit lab6_finish(void)
{
	printk(KERN_INFO " lab6_finish \n");
}
module_init(lab6_init);
module_exit(lab6_finish);

MODULE_LICENSE("GPL");
