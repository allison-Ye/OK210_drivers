中断下半部：
a下半部产生的原因：
        1.中断上下文中不能阻塞，这也限制了中断上下文中能干的事
        2.中断处理函数执行过程中仍有可能被其他中断打断，都希望中断处理函数执行得越快越好。
        基于上面的原因，内核将整个的中断处理流程分为了上半部和下半部。上半部就是之前所说的中断处理函数，它能最快的响应中断，并且做一些必须在中断响应之后马上要做的事情。而一些需要在中断处理函数后继续执行的操作，内核建议把它放在下半部执行。　
　　　　比如：在linux内核中，当网卡一旦接受到数据，网卡会通过中断告诉内核处理数据，内核会在网卡中断处理函数（上半部）执行一些网卡硬件的必要设置，因为这是在中断响应后急切要干的事情。接着，内核调用对应的下半部函数来处理网卡接收到的数据，因为数据处理没必要在中断处理函数里面马上执行，可以将中断让出来做更紧迫的事情
b.中断下半部实现的机制分类
    tasklet：          
    workqueue：工作队列
    timer：定时器
	其实还有一种，叫softirq，但要写代码的话，就必须修改原来的内核框架代码，在实际开发中用的比较少，tasklet内部实现就是用softeirq
c.中断下半部实现方法
 1. tasklet的编程方式
         1.1 : 定义并初始化结构体tasklet_struct(一般在哪里初始化：是在模块卸载方法中)
              struct tasklet_struct
				{
					struct tasklet_struct *next; // l链表
					unsigned long state;
					atomic_t count;
					void (*func)(unsigned long); // 下半部处理方法的实现
					unsigned long data;//给处理函数的传参
				};
           初始化方式：
			静态：DECLARE_TASKLET(name, func, data)；
              DCLARE_TASKLET_DISABLED初始化后的处于禁止状态，暂时不能被使用（不是中断），除非被激活           
				参数1:tasklet_struct 的变量名字，自定义
				参数2：中断下半部执行的处理函数.类型为函数指针
				参数3：处理函数带的参数
			动态：void tasklet_init(struct tasklet_struct * t, void(* func)(unsigned long), unsigned long data);
				参数1:tasklet_struct 对象
				参数2：中断下半部执行的处理函数
				参数3：处理函数带的参数
   1.2: 在中断上半部中调度下半部
        void tasklet_schedule(struct tasklet_struct * t);
   1.3: 在模块卸载时，销毁这个tasklet_struct 对象
        void tasklet_kill(struct tasklet_struct *t) 
   1.4:原理：初始化好struct tasklet_struct对象后，tasklet_schedule()会将tasklet对象加到链表中，内核稍后会去调度这个tasklet对象
   1.5： 特点：优先级高，调度快，运行在中断上下文中，所以在处理方法中，不能执行阻塞/睡眠的操作
            
