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
            
2.workqueque编程方式：
            2.1 ：定义并初始化workqueue_struct(一个队列)和work_struct(队列中的一项工作)对象
                  work_struct对象的初始化
                  struct work_struct {
	atomic_long_t data;  // 传递给work的参数
	struct list_head entry; // 所在队列的链表
	work_func_t func; // work对应的处理方法
};
静态：DECLARE_WORK(n, f)
     参数1: 变量名，自定义
     参数2：work对应的处理方法，类型为函数指针     
动态：INIT_WORK(_work, _func)
    参数1: 指针，先声明一个struct work_struct变量，将变量地址填入
    参数2：work对应的处理方法，类型为函数指针 
    返回值： 返回值为void   
	
                workqueue_struct对象的初始化：(其实就是一个内核线程)
                 1， 重新创建一个队列
                     create_workqueue(name)//这个本身就是一个宏
                      参数：名字，自定义,用于识别
　　　　　　　　　　　　　返回值：struct workqueue_struct *                    
                 2， 系统在开机的时候自动创建一个队列  
           2.2  将工作对象加入工作队列中，并参与调度(注意不是马上调度，该步骤也是中断上半部中调用)
               int queue_work(struct workqueue_struct *wq, struct work_struct *work)
               参数1：工作队列
　　　　　　　　　参数2: 工作对象
               返回值： 0表示已经放到队列上了(也即时说本次属于重复操作)，其实一般我们都不会去做出错处理


           2.3  在模块注销的时候，销毁工作队列和工作对象
                  void flush_workqueue(struct workqueue_struct * wq)
                 该函数会一直等待，知道指定的等待队列中所有的任务都执行完毕并从等待队列中移除。
                 void destroy_workqueue(struct workqueue_struct * wq);
                该函数是是创建等待队列的反操作，注销掉指定的等待队列。


           2.4： 对于使用内核自带的工作队列events， 操作步骤如下：
                 2.4.1 初始化工作对象，无需创建队列了
                       静态：DECLARE_WORK(n, f)   
                       动态：INIT_WORK(_work, _func)
                 2.4.2将工作加入队列并调度（在中断上半部中调度）
                        int schedule_work(struct work_struct * work)
              只要两步骤就完成，也不需要刷新，也不要销毁，因为这个工作队列是系统管理的，我们不用管
           2.5：原理梳理：在工作队列中，有专门的工作者线程来处理加入到工作对列中的任务。工作对列对应的工作者线程可能不止一个，每个处理器有且仅有一个工作队列
			对应的工作者线程，在内核中有一个默认的工作队列events，对于单处理器只有一个对应的工作者线程