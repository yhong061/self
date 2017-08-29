USER_DIR := /home/yhong
PROJ_DIR := $(USER_DIR)/work/S300A_Camera/leopard
KERNEL_DIR := $(PROJ_DIR)/kernel
I_KERNEL_DIR := $(KERNEL_DIR)/linux-3.4-release/include
ARCH_CONFIG := arm64



CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static -I$(I_KERNEL_DIR)

#OBJ = video.o ion.o setting.o
OBJ = uvc-gadget.o
TARGET = gadget

%.o: %.c
	 $(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	 $(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o $(TARGET)

.PHONY:clean  
