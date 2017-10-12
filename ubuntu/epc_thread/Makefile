ARCH_CONFIG := arm64



CC = gcc -m64 -g
#CFLAGS = -static -I$(I_KERNEL_DIR)
CFLAGS = -static -pthread  -lm

OBJ = epcthread.o epccalc.o epcapi.o
TARGET = epcthread

%.o: %.c
	 $(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJ)
	 $(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o $(TARGET)

.PHONY:clean  
