obj-m := reg_irs16x5c.o

ROOT_DIR := /home/yhong/work/GS700E_5110_AR_160907/android
ARCH_CONFIG := arm64
CROSS_COMPILE_CONFIG := $(ROOT_DIR)/toolchain/aarch64-linux-gnu/bin/aarch64-linux-gnu-


KERNEL_DIR :=/home/yhong/work/GS700E_5110_AR_160907/android/owl/out/v700_android_v700_ar/kernel

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) ARCH=$(ARCH_CONFIG) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG)  modules

clean:
	rm -rf *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY:clean
