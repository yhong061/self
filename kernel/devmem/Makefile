obj-m := devmem.o

ROOT_DIR := /home/yhong/work/GS700E_5110_AR_160907/android
ARCH_CONFIG := arm
CROSS_COMPILE_CONFIG :=/opt/arm-2011.09/bin/arm-none-linux-gnueabi- 

KERNEL_DIR :=/home/yhong/work/S300A/leopard/kernel/linux-3.4-release

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) ARCH=$(ARCH_CONFIG) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG)  modules

clean:
	rm -rf *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY:clean
