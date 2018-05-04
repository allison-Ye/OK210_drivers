#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "led.h"

/*./led_test on num���
 *./led_test off num���
 */
int main(int argc,char **argv)
{
	int fd;
	int cmd=0;
	int val=0;
	/*1.������/���豸�ļ�*/
	fd=open("/dev/led",O_RDWR);
	if(fd<0){
		perror("open failed");
		exit(1);
	}	

	val =atoi(argv[2]);
	printf("val =%d\n",val);
	
	if(strcmp(argv[1],"on")==0){
		cmd =LED_ON;
	}else{
		cmd =LED_OFF;
	}
	
	/*2.д����*/
	if(ioctl(fd,cmd,(unsigned long)val)<0){
		perror("ioctl failed");
		exit(1);
	}
	
	close(fd);
	return 0;
}

