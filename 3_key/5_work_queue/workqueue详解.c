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
                 该函数会一直等待，直到指定的等待队列中所有的任务都执行完毕并从等待队列中移除。
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
