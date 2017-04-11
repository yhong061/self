#include <stdio.h>
#if 1
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define DEV_NAME "/dev/yhong"
static int gFd = 0;


enum YHONG_IOCTL_CMD {
	MEM_READ,
	MEM_WRITE,
	IO_END,
};

struct yhong_mem {
	unsigned int mem;
	unsigned int data;
};

int mem_ioctl(unsigned int cmd, struct yhong_mem *mem)
{
	int ret;
	ret = ioctl(gFd, cmd, mem);
	if(ret < 0)
		printf("Call cmd = %d fail\n", cmd);
	return ret;
}

int mem_read(unsigned int cmd, unsigned int addr, unsigned int *data)
{
	int ret;
	struct yhong_mem mem;
	mem.mem = addr;
	mem.data = 0;
	printf("[%s:%d][R] addr = 0x%08X, data = 0x%08X\n",__func__, __LINE__, addr, data);
	ret = mem_ioctl(cmd, &mem);
	if(!ret) {
		*data = mem.data;
	}
	printf("[R] 0x%08x = 0x%08x\n", mem.mem, mem.data);
	return ret;
}

int mem_write(unsigned int cmd, unsigned int addr, unsigned int data)
{
	int ret;
	struct yhong_mem mem;
	mem.mem = addr;
	mem.data = data;
	ret = mem_ioctl(cmd, &mem);
	printf("[W] 0x%08x = 0x%08x\n", mem.mem, mem.data);
	return ret;
}

void usage_help(void)
{
	printf("===============================\n");
	printf("sample: \n");
	printf("./devmem addr      	: read value of addr, such as 0xe0268000 \n");
	printf("./devmem addr data   : write value of addr, such as 0xe0268000 \n");
	printf("===============================\n");
}
int main(int argc, char* argv[])
{
	int addr = 0;
	int val = 0;
	unsigned int mem = 0;
	unsigned int data = 0;

	gFd = open(DEV_NAME,O_RDWR);
	if (gFd < 0)
	{
		printf("Open Dev Mem0 Error!\n");
		return -1;
	}

	if(argc == 2) {
		char *str = argv[1];
		char *val;
		int base = 16;

		mem = strtoul(str, &val, base);
		printf("[%s:%d][R] mem = 0x%08X, data = 0x%08X\n",__func__, __LINE__, mem, data);
		mem_read(MEM_READ, mem, &data);
	}
	else if(argc == 3) {
		char *str = argv[1];
		char *val;
		int base = 16;

		mem = strtoul(str, &val, base);
		str = argv[2];
		data = strtoul(str, &val, base);
		mem_write(MEM_WRITE, mem, data);
		printf("[W] mem = 0x%08X, data = 0x%08X\n", mem, data);
	}
	else {
		usage_help();
	}
    
	close(gFd);
	return 0;    
}
#else

int main(int argc, char* argv[])
{
	int addr = 0;
	unsigned int mem = 0;
	unsigned int data = 0;
	char *str = argv[1];
	int base = 16;
	char *val;
	int ret;

	if(argc == 2) {
	mem = strtoul(str, &val, base);
	//mem = *val;
	printf("mem = 0x%x, str = %s, val = %s\n", mem, str, val);
	}
	else if(argc == 3) {
		mem = atoi(argv[1]);
		data = atoi(argv[2]);
	}
	return 0;    
}
#endif
