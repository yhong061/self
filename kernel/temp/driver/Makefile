obj-m := owl_si.o

ROOT_DIR := /home/yhong/GS700C_5110_IPC_20160825/android
ARCH_CONFIG := arm64
CROSS_COMPILE_CONFIG := $(ROOT_DIR)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-


KERNEL_DIR :=/home/yhong/GS700C_5110_IPC_20160825/android/kernel

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) ARCH=$(ARCH_CONFIG) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG)  modules

clean:
	rm -rf *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY:clean
