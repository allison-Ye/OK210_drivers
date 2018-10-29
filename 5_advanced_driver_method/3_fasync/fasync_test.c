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

	/*读取按键驱动发出的数据，注意key_val和按键驱动中定义为一致的类型*/
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
	 * 告诉内核，发给谁
	 */
	fcntl(buttons_fd, F_SETOWN, getpid());

	/*  F_GETFL :Read the file status flags
	 *  读出当前文件的状态
	 */
	flag = fcntl(buttons_fd,F_GETFL);

	/* F_SETFL: Set the file status flags to the value specified by arg
	 * int fcntl(int fd, int cmd, long arg);
	 * 修改当前文件的状态，添加异步通知功能
	 */
	fcntl(buttons_fd,F_SETFL,flag | FASYNC);
	
	while(1)
	{
		/* 为了测试，主函数里，什么也不做 */
		sleep(1000);
	}
	return 0;
}
