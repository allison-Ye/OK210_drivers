#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>


/* ��ֵ: ����ʱ, 0x01, 0x02, 0x03, 0x04,0x05,0x06,0x07,0x08 */  
/* ��ֵ: �ɿ�ʱ, 0x81, 0x82, 0x83, 0x84,0x85,0x86,0x87,0x88 */  
int main(void)
{
	int buttons_fd;
	char key_val;
	char key_num;

	/* ���豸 */
	buttons_fd = open("/dev/IRQ_KEY", O_RDWR);
	if (buttons_fd < 0)
	{
		printf("Can not open device key\n");
		return -1;
	}
	printf(" Please press buttons on webe210 board\n");

	while(1)
	{
		/*��ȡ�����������������ݣ�ע��key_value�ͼ��������ж���Ϊһ�µ�����*/
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
	close(buttons_fd);
	return 0;
}
