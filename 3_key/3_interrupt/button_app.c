#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    int fd;
    unsigned char key_val;

    fd = open("/dev/mybuttons", O_RDWR);
    if (fd < 0) {
        printf("open buttons failed.\n");
        return -1;
    }

    while (1) {
        read(fd, &key_val, 1);    
        //printf("key_val = %#x\n", key_val);
    }

    close(fd);
    return 0;
}

