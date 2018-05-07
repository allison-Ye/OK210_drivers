#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "led.h"

int main(int argc, char *argv[])
{
     int fd;
     int cmd;
     
     if (argc <2 )
     {
         printf("please enter the second para!\n");
         return 0;	
     }
     
     cmd = atoi(argv[1]); 
     
     fd = open("/dev/myled",O_RDWR);
     
     if (cmd == 1)
         ioctl(fd,LED_ON);
     else
         ioctl(fd,LED_OFF);	
         
         
     return 0;
}