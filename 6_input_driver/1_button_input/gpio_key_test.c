/*
 * Name:gpio_key_test.c
 * Copyright (C) 2014 Webee.JY  (2483053468@qq.com)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc,char *argv[])
{
	int fd;
 	struct input_event ev_key;

	if (argc != 2)
	{
		printf("Usage:\n");
		printf("%s /dev/event0 or /dev/event1\n",argv[0]);
		return 0;
	}
	
	fd= open(argv[1], O_RDWR);

	if(fd < 0)
	{
		perror("open device buttons");
		exit(1);
	}

	while(1)
	{
		read(fd,&ev_key,sizeof(struct input_event));
		printf("type:%d,code:%d,value:%d\n",ev_key.type,ev_key.code,ev_key.value);
	}
	
	close(fd);
	return 0;
}
