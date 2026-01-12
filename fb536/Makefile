# Makefile for fb536 framebuffer char module
# Based on scullc from "Linux Device Drivers" by Alessandro Rubini
# and Jonathan Corbet, published by O'Reilly & Associates.
#
# Modified for CEng 536 - Fall 2025 - Homework 3

ifneq ($(KERNELRELEASE),)
# Called from kernel build system
obj-m := fb536.o
fb536-objs := main.o

else
# Called from command line
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers modules.order

endif
