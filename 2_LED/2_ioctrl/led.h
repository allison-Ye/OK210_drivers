/*方法:参照*/
#ifndef __ASM_BUF_H
#define __ASM_BUF_H

#define MAGIC	'L'

#define LED_ON 	_IOW(MAGIC,1,int)
#define LED_OFF _IOW(MAGIC,0,int)

#define LED_MAJOR 249
#define OK210_PA_GPH2CON	0xE0200C40		//物理地址
#define OK210_GPH2CON		0x0
#define OK210_GPH2DAT		0x04

/*用来表示一个内存模拟的设备*/
struct led_device{	
	struct class	*led_class;	  //设备类属性结构体
	struct device 	*led_device;  //设备属性结构体	
	void		*led_base;
};

#endif
