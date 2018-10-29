#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	//sleep
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

int buttons_fd;

void mysignal_fun(int signum)
{
	char key_num,key_val;

	/*��ȡ�����������������ݣ�ע��key_val�Ͱ��������ж���Ϊһ�µ�����*/
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


/* fasync_test
 */ 
int main(int argc ,char *argv[])
{
	int flag;
	signal(SIGIO,mysignal_fun);

	buttons_fd = open("/dev/IRQ_KEY",O_RDWR);
	if (buttons_fd < 0)
	{
		printf("open error\n");
	}

	/* F_SETOWN:  Set the process ID
	 * �����ںˣ�����˭
	 */
	fcntl(buttons_fd, F_SETOWN, getpid());

	/*  F_GETFL :Read the file status flags
	 *  ������ǰ�ļ���״̬
	 */
	flag = fcntl(buttons_fd,F_GETFL);

	/* F_SETFL: Set the file status flags to the value specified by arg
	 * int fcntl(int fd, int cmd, long arg);
	 * �޸ĵ�ǰ�ļ���״̬������첽֪ͨ����
	 */
	fcntl(buttons_fd,F_SETFL,flag | FASYNC);
	
	while(1)
	{
		/* Ϊ�˲��ԣ��������ʲôҲ���� */
		sleep(1000);
	}
	return 0;
}
