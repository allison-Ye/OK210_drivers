ifneq ($(KERNELRELEASE),)

obj-m := irq_key.o

else
	
KDIR := /home/webee210v2/linux-3.8_webee210v2

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers

endif
