USER_DIR := /home/yhong
PROJ_DIR := $(USER_DIR)/work/GS700E_5110_AR_160907/android/
KERNEL_DIR := $(PROJ_DIR)/owl/out/v700_android_v700_ar_linux/kernel/source
I_KERNEL_DIR := $(KERNEL_DIR)/include
ARCH_CONFIG := arm64



CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static -I$(I_KERNEL_DIR)

#OBJ = video.o ion.o setting.o
OBJ = uvc-gadget+0505ok.o
TARGET = gadget_ok

%.o: %.c
	 $(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	 $(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o $(TARGET)

.PHONY:clean  
