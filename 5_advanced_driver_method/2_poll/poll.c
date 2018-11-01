#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void main(){

	fd_set rfds;
	struct timeval tv;
	int retval;

	char buf[1024] = {0};


 while (1) {
        FD_ZERO(&rfds); //清空读文件描述符集合
        FD_SET(0, &rfds); //监听标准输入

        //设置超时时间为5s
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        printf("Main Process go to Sleeping...\n");
        //启动监听
        retval = select(1, &rfds, NULL, NULL, &tv);
        if (retval == -1) //错误
            perror("select()");
        else if (retval) {//有输入的数据到来
            if (FD_ISSET(0, &rfds)){ //判断是否是标准输入的数据
                read(0, buf, 1024); //读取标准输入
                printf("msg: %s\n", buf);
            }
        }
        else    //超时
            printf("No data within five seconds.\n");
    }
    exit(EXIT_SUCCESS);
}
