# If KERNELRELEASE is defined, we've been invoked from the
# # kernel build system and can use its language.
ROOTFS_DIR = /home/NFS
ifneq ($(KERNELRELEASE),)
	obj-m := lab3.o
#         # Otherwise we were called directly from the command
# line; invoke the kernel build system.
else        
	KERNELDIR ?=/home/linux/linux-ok210
        PWD := $(shell pwd)
    
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

install :
	cp -rvaf *ko $(ROOTFS_DIR)/

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers *.unsigned
