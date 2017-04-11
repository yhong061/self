#include <stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>


#define ENDMARKER { 0xff, 0xff, 0xff }

struct regval_list {
	unsigned short data_width;
	unsigned short reg_num;
	unsigned short value;
};

 enum {
	CHIPID_XC6130_OV2710,
	REG_READ_XC6130,
	REG_WRITE_XC6130,
	REG_READ_OV2710,
	REG_WRITE_OV2710,
	ARRAY_WRITE_XC6130,
	ARRAY_READ_XC6130,
	ARRAY_WRITE_OV2710,
	ARRAY_READ_OV2710,
	IO_END,
};


#define DEV_NAME "/dev/camera_reg"
static int gFd = 0;


#define REG_BUF_LEN (100)
struct reg_buf {
  unsigned int i2c_addr;
  unsigned int reg;
  unsigned int reg_width;
  unsigned int data;
  unsigned int data_width;
  struct regval_list array[REG_BUF_LEN];
};

int camera_reg_read(unsigned int i2c_addr, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned int *data)
{

	struct reg_buf arg;

	arg.i2c_addr = i2c_addr;
	arg.reg_width = reg_width;
	arg.reg = i2c_reg;
	arg.data_width = data_width;
	arg.data = 0;

    	cmd = CAMERA_REG_READ;
    	if (ioctl(gFd, cmd, &arg) < 0)
        {
            printf("Call cmd MEMDEV_IOCPRINT fail\n");
            return -1;
    	}

	printf("reg read : %d\n", arg.data);

	return 0;
}

int xc6130_write_array(struct regval_list *array, int size)
{
#if 0
    int cmd = 0;
	struct reg_buf arg;

	if(size/sizeof(struct regval_list) > REG_BUF_LEN) {
    	printf("[%s:%s] size/sizeof(struct regval_list)=%d > REG_BUF_LEN=%d\n", __FILE__, __func__, size/sizeof(struct regval_list), REG_BUF_LEN);
		return 0;
	}
	
	printf("write array size = %d\n", size);
	arg.size = size;
	memcpy(arg.array, array, size);

    cmd = ARRAY_WRITE_XC6130;
    if (ioctl(gFd, cmd, &arg) < 0)
        {
            printf("Call cmd MEMDEV_IOCPRINT fail\n");
            return -1;
    }

#endif
	xc6130_ioctl(ARRAY_WRITE_XC6130, array, size);
	return 0;
}

#if 0
int xc6130_reg_read_sub(struct regval_list *array, int size)
{

    int cmd = 0;
	struct reg_buf arg;

	if(size/sizeof(struct regval_list) > REG_BUF_LEN) {
    	printf("[%s:%s] size/sizeof(struct regval_list)=%d > REG_BUF_LEN=%d\n", __FILE__, __func__, size/sizeof(struct regval_list), REG_BUF_LEN);
		return 0;
	}

	printf("read array size = %d\n", size);
	arg.size = size;
	memcpy(arg.array, array, size);

    cmd = REG_READ_XC6130;
    if (ioctl(gFd, cmd, &arg) < 0)
        {
            printf("Call cmd MEMDEV_IOCPRINT fail\n");
            return -1;
    }

	array->value = arg.array[0].value;
	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__, arg.array[0].reg_num, arg.array[0].value);

	return 0;
}

int xc6130_reg_witer_sub(struct regval_list *array, int size)
{

    int cmd = 0;
	struct reg_buf arg;

	if(size/sizeof(struct regval_list) > REG_BUF_LEN) {
    	printf("[%s:%s] size/sizeof(struct regval_list)=%d > REG_BUF_LEN=%d\n", __FILE__, __func__, size/sizeof(struct regval_list), REG_BUF_LEN);
		return 0;
	}

	printf("read array size = %d\n", size);
	arg.size = size;
	memcpy(arg.array, array, size);

    cmd = REG_WRITE_XC6130;
    if (ioctl(gFd, cmd, &arg) < 0)
        {
            printf("Call cmd MEMDEV_IOCPRINT fail\n");
            return -1;
    }

	array->value = arg.array[0].value;
	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__, arg.array[0].reg_num, arg.array[0].value);

	return 0;
}
#endif

int xc6130_reg_read(unsigned short reg_num)
{

    int cmd = 0;
	struct regval_list array;
	array.data_width = 1;
	array.reg_num = reg_num;
	array.value = 0;

	xc6130_ioctl(REG_READ_XC6130, &array, sizeof(array));

	//xc6130_reg_read_sub(&array, sizeof(array));
	//printf("read array value = %d\n", array.value);
	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__,  reg_num, array.value);

	return 0;
}

int xc6130_reg_write(unsigned short reg_num, unsigned short value)
{

    int cmd = 0;
	struct regval_list array;
	array.data_width = 1;
	array.reg_num = reg_num;
	array.value = value;

	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__,  reg_num, value);
	xc6130_ioctl(REG_WRITE_XC6130, &array, sizeof(array));

	return 0;
}

int temp_reg_read(unsigned short reg_num, unsigned short value)
{
	struct reg_buf arg;

	arg.size = size;
	memcpy(arg.array, array, size);

int cmd = ARRAY_WRITE_XC6130;
if (ioctl(gFd, cmd, &arg) < 0)
{
printf("Call cmd MEMDEV_IOCPRINT fail\n");
return -1;
}

	struct regval_list * ptr = array;
	int i;
	for(i=0; i<size/sizeof(struct regval_list); i++,ptr++) {
		ptr->value = arg.array[i].value;
	}


    int cmd = 0;
	struct regval_list array;
	array.data_width = 1;
	array.reg_num = reg_num;
	array.value = value;

	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__,  reg_num, value);
	xc6130_ioctl(REG_WRITE_XC6130, &array, sizeof(array));

	return 0;
}

void usage_help(void)
{
	printf("===============================\n");
	printf("sample: \n");
	printf("./test_mem reg r 0xreg      : reg read\n");
	printf("./test_mem reg w 0xreg 0xval: reg wreat\n");
	printf("./test_mem eeprom r             : eeprom read\n");
	printf("./test_mem temp   r             : temprature read\n");
	printf("===============================\n");
	
}



int main(int argc, char* argv[])
{
	int addr = 0;
	int val = 0;
	struct regval_list *array;   
	unsigned int i2c_addr;
	unsigned int reg_width;
	unsigned int reg;
	unsigned int data_width;
	unsigned int data;
	

	if(argc <= 1) {
		usage_help();
		return 0;
	}
    
	/*打开设备文件*/
	gFd = open(DEV_NAME,O_RDWR);
	if (gFd < 0)
	{
		printf("Open Dev Mem0 Error!\n");
		return -1;
	}

	char *cmd = argv[1];
//	printf("[%s:%d]argc = %d, argv[1] = %s\n",__FILE__, __LINE__, argc, cmd);
	if(strcmp(cmd, "reg") == 0 && argc > 2) {
		char b = *argv[2];
		if((b == 'r'  && argc > 3)) {
			int c = 0;//*argv[3];
			sscanf(argv[3], "0x%x", &c);
			printf("[%s:%d] cmd= %s %c 0x%x\n",__func__, __LINE__, cmd, b, c);
			xc6130_reg_read(c);
		}
		else if((b == 'w'  && argc > 4)) {
			int c = 0;//*argv[3];
			int d = 0;//*argv[4];
			sscanf(argv[3], "0x%x", &c);
			sscanf(argv[4], "0x%x", &d);
			printf("[%s:%d] cmd= %s %c 0x%x 0x%x\n",__func__, __LINE__, cmd, b, c, d);
			xc6130_reg_write(c, d);
		}
		else {
			printf("[%s:%d] error: cmd= %s\n",__func__, __LINE__, cmd);
		}

	}
	else if(strcmp(cmd, "wb") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] cmd= %s %c\n",__func__, __LINE__, cmd, b);
		if(b == '0') {
		    printf("------------wb_array_old-----------\n");
			xc6130_write_array(wb_array_old, sizeof(wb_array_old));
		}
		else if(b == '1') {
		    printf("------------wb_array_new-----------\n");
			xc6130_write_array(wb_array_new, sizeof(wb_array_new));
		}
		else {
			printf("[%s:%d] error: cmd= %s\n",__func__, __LINE__, cmd);
		}
			
	}
	else if(strcmp(cmd, "nr") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] cmd= %s %c\n",__func__, __LINE__, cmd, b);
		if(b == '0') {
		    printf("------------nr_array_old-----------\n");
			xc6130_write_array(XC6130_VER_000, sizeof(XC6130_VER_000));
		}
		else if(b == '1') {
		    printf("------------nr_array_new-----------\n");
			xc6130_write_array(XC6130_VER_010, sizeof(XC6130_VER_010));
		}
		else {
			printf("[%s:%d] error: cmd= %s\n",__func__, __LINE__, cmd);
		}
			
	}
	else if(strcmp(cmd, "temp") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] cmd= %s %c\n",__func__, __LINE__, cmd, b);
		if(b == 'r') {
		    printf("------------temp read-----------\n");
		    
		    i2c_addr = 0x90;
		    reg_width = 1;
		    reg = 0x00;
		    data_width = 1;
		    data = 0;
		    camera_reg_read(i2c_addr, reg_width, reg, data_width, &data);
		}
		else {
			printf("[%s:%d] temp error: cmd= %s\n",__func__, __LINE__, cmd);
		}
			
	}
	else {
		printf("[%s:%d] error: cmd= %s\n",__func__, __LINE__, cmd);
	}
    
    
    close(gFd);
    return 0;    
}
