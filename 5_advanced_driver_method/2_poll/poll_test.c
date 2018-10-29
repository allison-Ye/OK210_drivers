#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

/* 键值: 按下时, 0x01, 0x02, 0x03, 0x04,0x05,0x06,0x07,0x08 */  
/* 键值: 松开时, 0x81, 0x82, 0x83, 0x84,0x85,0x86,0x87,0x88 */  
int main(void)
{
	int buttons_fd;
	char key_val;
	char key_num;
	struct pollfd fds; 
	int ret;

	/* 打开设备 */
	buttons_fd = open("/dev/IRQ_KEY", O_RDWR);
	if (buttons_fd < 0)
	{
		printf("Can not open device key\n");
		return -1;
	}
	printf(" Please press buttons on webe210 board\n");
	
	fds.fd = buttons_fd;  
	fds.events = POLLIN; 
	while(1)
	{
		/* poll函数返回0时，表示5s时间到了，而这段时间里，没有事件发生"数据可读" */  
        ret = poll(&fds,1,5000);  
        if(ret == 0)  
        {  
            printf("time out\n");  
        } 
		else
		{
			/*读取按键驱动发出的数据，注意key_value和按键驱动中定义为一致的类型*/
			read(buttons_fd, &key_val, 1);
			
			key_num = key_val;
			if(key_num & 0x80) 	
			{
				key_num = key_num - 0x80; 
				printf("S%d is release on!     key_val = 0x%x\n",key_num ,key_val);
			}
			else
			{
				printf("S%d is pressed down!   key_val = 0x%x\n", key_num,key_val);
			}	
		  }
	}
	close(buttons_fd);
	return 0;
}
