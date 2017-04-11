KO_NAME = reg_ctrl
obj-m := $(KO_NAME).o

#ROOT_DIR := /home/yhong/GS700C_5110_IPC_20160825/android
ROOT_DIR := /home/yhong/work/GS700E_5110_AR_160907/android/
ARCH_CONFIG := arm64
#CROSS_COMPILE_CONFIG := $(ROOT_DIR)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
CROSS_COMPILE_CONFIG := $(ROOT_DIR)/toolchain/aarch64-linux-gnu/bin/aarch64-linux-gnu-


#KERNEL_DIR :=/home/yhong/GS700C_5110_IPC_20160825/android/kernel
KERNEL_DIR := $(ROOT_DIR)/owl/out/v700_android_v700_ar/kernel 
INCLUDE_DIR := $(ROOT_DIR)/toolchain/aarch64-linux-gnu/aarch64-linux-gnu/libc/usr/

PWD := $(shell pwd)
MOD_DIR =.

EXTRA_FLAG += -DDEBUG

#$(KO_NAME)-objs += $(MOD_DIR)/reg_ctrl.o

all:
	make -C  $(KERNEL_DIR) SUBDIRS=$(PWD) ARCH=$(ARCH_CONFIG) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) EXTRA_FLAG="$(EXTRA_FLAG)"  modules

clean:
	rm -rf *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY:clean
