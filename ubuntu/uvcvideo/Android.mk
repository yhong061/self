LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(TOPDIR)
KERNEL_PATH :=$(ROOT_PATH)/kernel

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=\
   video.c \
   ion.c \
   camera_reg.c

LOCAL_SHARED_LIBRARIES :=       \
    libcutils                   \
    libutils                    \
    liblog                      \
    


LOCAL_C_INCLUDES := $(KERNEL_PATH)/include	               \
    $(ROOT_PATH)/external/kernel-headers/original/uapi/asm-generic/

LOCAL_CPPFLAGS += -g  -DDEBUG

LOCAL_MODULE := libuvcvideo

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=\
   video_main.c 

LOCAL_SHARED_LIBRARIES :=       \
    libcutils                   \
    liblog			\
    libuvcvideo


LOCAL_C_INCLUDES := $(KERNEL_PATH)/include	               \
    $(ROOT_PATH)/external/kernel-headers/original/uapi/asm-generic/

LOCAL_CPPFLAGS += -g  -DDEBUG

LOCAL_MODULE := uvc_video_main

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
