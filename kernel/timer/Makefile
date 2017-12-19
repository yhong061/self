#ROOT_DIR := /home/yhong/GS700C_5110_IPC_20160825/android
ROOT_DIR := /home/yhong/src/S700/S700/android
ARCH_CONFIG := arm64
CROSS_COMPILE_CONFIG := $(ROOT_DIR)/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-


KERNEL_DIR :=$(ROOT_DIR)/owl/out/s700_android_s700_cb6/kernel

PWD := $(shell pwd)

obj-m := pmudriver.o

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) ARCH=$(ARCH_CONFIG) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG)  modules                                      


clean:
	rm -rf *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY:clean  
