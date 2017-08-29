LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(TOPDIR)
KERNEL_PATH :=$(ROOT_PATH)/kernel

include $(CLEAR_VARS)
ARCH=arm64

LOCAL_SRC_FILES :=\
   uvc_gadget_video.c 

LOCAL_SHARED_LIBRARIES :=       \
    libcutils                   \
    liblog


LOCAL_C_INCLUDES := $(KERNEL_PATH)/include	               \
	$(ROOT_PATH)/external/kernel-headers/original/uapi/asm-generic/

LOCAL_CPPFLAGS += -g  -DDEBUG

LOCAL_MODULE := libusbvideo

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
