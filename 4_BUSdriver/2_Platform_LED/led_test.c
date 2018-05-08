#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "led.h"
//#define LED_ON  0x100001
//#define LED_OFF 0x100002
int main(int argc, char *argv[])
{
     int fd;
     //int cmd;
     
     if (argc <2 )
     {
         printf("please enter the second para!\n");
         return -1;	
     }
     
     //cmd = atoi(argv[1]); 
     
     fd = open("/dev/myled",O_RDWR);
	 if(fd < 0)
		 printf("Open led failed.\n");
     
     if (strcmp(argv[1], "ON") == 0)
         ioctl(fd,LED_ON);
     else
         ioctl(fd,LED_OFF);	
     
	close(fd);
         
     return 0;
}