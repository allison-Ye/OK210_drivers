linux�豸����֮�жϱ��
========================================================
��������ж�:
1) �����ж�/ע���ж�
	int request_irq(unsigned int irq,irq_handler_t handler,unsigned long flags,const char * name,void * dev)
   /*param1:ִ��Ҫ������жϺ�:оƬ�������˶�Ӧ�ĺ궨�� plat/irqs.h ,mach/irqs.h  linux/irqs.h
    *param2:�жϷ������  : typedef irqreturn_t (*irq_handler_t)(int, void *);
	*param3:���:�жϴ�����ʽ  linux/interrupt.h
	*			 #define IRQF_TRIGGER_NONE	0x00000000       //�����ڲ��ж�,���綨ʱ���жϻ���RTC�ж�...
	*			 #define IRQF_TRIGGER_RISING	0x00000001	 //�����ش���
	*			 #define IRQF_TRIGGER_FALLING	0x00000002	 //�½��ش���
	*			 #define IRQF_TRIGGER_HIGH	0x00000004		 //�ߵ�ƽ����
	*			 #define IRQF_TRIGGER_LOW	0x00000008		 //�͵�ƽ����
	*param4:�ж�����
	*param5:�ж�id��,�����жϵ�ʱ��Ż���.���������ᴫ���жϷ������
	*����ֵ:�ɹ�0,ʧ�ܷ���һ����0ֵ
    */
   request_irq(IRQ_EINT(0), buttons_irq_func,IRQF_TRIGGER_FALLING,
	    const char *name, void *dev)

2) ʵ���жϷ������
   /*param1:�жϺ�
    *param2:�ж�id��
	*����ֵ:�жϴ���ɹ��򷵻�IRQ_HANDLED,����ʧ�ܷ���IRQ_NONE
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
		//����
		return IRQ_HANDLED;
   }
   ����1: �жϺ�

3) �ͷ��ж�
   /*param1:�жϺ�
    *param2:id��
	*/	
   void free_irq(unsigned int irq, void *dev_id);