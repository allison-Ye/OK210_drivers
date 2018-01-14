ROOTFS_DIR = /home/NFS
CROSS_COMPILE = arm-none-linux-gnueabi-
MODULE_NAME = hello
#APP_NAME = hello 

ifeq ($(KERNELRELEASE),)
KERNEL_DIR =/home/linux/linux-ok210
CUR_DIR =$(shell pwd)

all :
	make -C $(KERNEL_DIR) M=$(CUR_DIR) modules
#	$(CROSS_COMPILE)gcc  $(APP_NAME).c -o  $(APP_NAME)
clean :
	make -C $(KERNEL_DIR) M=$(CUR_DIR) clean
	rm -rf $(APP_NAME)

install :
	cp -rvaf *ko  $(APP_NAME)  $(ROOTFS_DIR)/
else

obj-m += $(MODULE_NAME).o add_sub.o
obj-m += add_sub.o

endif

