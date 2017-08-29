LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(TOPDIR)
KERNEL_PATH :=$(ROOT_PATH)/kernel



include $(CLEAR_VARS)

LOCAL_SRC_FILES :=\
   uvc_gadget_main.cpp 

LOCAL_SHARED_LIBRARIES :=       \
    libcutils                   \
    liblog			\
    libusbvideo		\
    libvideo		\
    libtango_hal	\
    libspectre_custom


LOCAL_C_INCLUDES := $(KERNEL_PATH)/include	               \
    $(ROOT_PATH)/external/kernel-headers/original/uapi/asm-generic/

LOCAL_CPPFLAGS += -g  -DDEBUG

LOCAL_MODULE := gadget_tof

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
