obj-m = led_driver.o
KDIR := /home/user/linux-kernel
PWD := $(shell pwd)

default:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(KDIR) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CR_C) -C $(KDIR) M=$(PWD) clean
