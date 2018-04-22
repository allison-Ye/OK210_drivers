#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
 /* ./led_test on 
 * ./led_test off
 * */
int main(int argc, char *argv[])
{
    int fd;
    int cmd; //用户缓冲区
    int led_status; //用户缓冲区

    if (argc != 2 ) {
        printf("usage:\n %s <on|off>\n", argv[0]);
        return -1;
    }
    fd = open("/dev/myled", O_RDWR);
    if (fd < 0) {
        printf("open led device failed.\n");
        return -1;
    }
    if (strcmp(argv[1], "on") == 0) {
        cmd = 1;
        write(fd, &cmd, sizeof(cmd));
    }
    else if (strcmp(argv[1], "off") == 0) {
       	cmd = 0;
        write(fd, &cmd, sizeof(cmd));
    }

    read(fd, &led_status, sizeof(led_status));
    printf("led is %s\n", led_status ? "on" : "off"); 
    
    close(fd);
    return 0;
}


