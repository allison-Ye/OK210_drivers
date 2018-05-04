ROOTFS_DIR = /home/NFS
CROSS_COMPILE = arm-linux-
MODULE_NAME = button_drv
APP_NAME = button_app

ifeq ($(KERNELRELEASE),)
KERNEL_DIR =/home/linux/linux-ok210
CUR_DIR =$(shell pwd)

all :
	make -C $(KERNEL_DIR) M=$(CUR_DIR) modules
	$(CROSS_COMPILE)gcc  $(APP_NAME).c -o $(APP_NAME) -static
clean :
	make -C $(KERNEL_DIR) M=$(CUR_DIR) clean
	rm -rf $(APP_NAME)

install :
	cp -rvaf *ko  $(APP_NAME)  $(ROOTFS_DIR)/
else

obj-m = $(MODULE_NAME).o

endif

