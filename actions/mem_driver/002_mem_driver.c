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


#define DEV_NAME "/dev/yhong"
static int gFd = 0;

struct regval_list wb_array_old[] = {
{1,0xFFFD,0x80},
{1,0xFFFE,0x80},
{1,0x01b0,0x85}, 
{1,0x01b1,0xb8}, 
{1,0x01b2,0x39}, 
{1,0x01b3,0x4a}, 
{1,0x01b4,0x6a}, 
{1,0x01b5,0xa1}, 
{1,0x01b6,0x54}, 
{1,0x01b7,0x6f}, 
{1,0x01b8,0x54}, 
{1,0x01b9,0x6e}, 
{1,0x01ba,0x62}, 
{1,0x01bb,0x7f}, 
{1,0x01bc,0x6b}, 
{1,0x01bd,0x92}, 
{1,0x01be,0x46}, 
{1,0x01bf,0x5a}, 
{1,0x01c0,0x6a}, 
{1,0x01c1,0x95}, 
{1,0x01c2,0x6c}, 
{1,0x01c3,0x87}, 

{1,0x01f1,0x22}, 
ENDMARKER,
};

struct regval_list wb_array_new[] = {
{1,0xFFFD,0x80},
{1,0xFFFE,0x80},
{1,0x01b0,0x60}, 
{1,0x01b1,0xa0}, 
{1,0x01b2,0x37}, 
{1,0x01b3,0x4c}, 
{1,0x01b4,0x66}, 
{1,0x01b5,0x96}, 
{1,0x01b6,0x58}, 
{1,0x01b7,0x77}, 
{1,0x01b8,0x52}, 
{1,0x01b9,0x67}, 
{1,0x01ba,0x5d}, 
{1,0x01bb,0x82}, 
{1,0x01bc,0x63}, 
{1,0x01bd,0x8d}, 
{1,0x01be,0x4b}, 
{1,0x01bf,0x5e}, 
{1,0x01c0,0x5d}, 
{1,0x01c1,0x79}, 
{1,0x01c2,0x76}, 
{1,0x01c3,0x97}, 

{1,0x01f1,0x21}, 
ENDMARKER,
};

#define REG_BUF_LEN (100)

struct reg_buf {
	int size;
	struct regval_list array[REG_BUF_LEN];
};

int xc6130_write_array(struct regval_list *array, int size)
{

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

	
	return 0;
}

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

int xc6130_reg_read(unsigned short reg_num)
{

    int cmd = 0;
	struct regval_list array;
	array.data_width = 1;
	array.reg_num = reg_num;
	array.value = 0;

	xc6130_reg_read_sub(&array, sizeof(array));
	printf("read array value = %d\n", array.value);
	printf("[%s]reg_num = 0x%x value = 0x%x\n",__func__,  reg_num, array.value);

	return 0;
}


void usage_help(void)
{
	printf("===============================\n");
	printf("sample: \n");
	printf("./test_mem wb 0          : old wb color\n");
	printf("./test_mem wb 1          : new wb color\n");
	printf("./test_mem reg r 0xreg      : reg read\n");
	printf("./test_mem reg w 0xreg 0xval: reg wreat\n");
	printf("===============================\n");
	
}



int main(int argc, char* argv[])
{
    int addr = 0;
    int val = 0;
	struct regval_list *array;   

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
#if 0 
    /* 调用命令MEMDEV_IOCPRINT */
    printf("<--- Call CHIPID_XC6130_OV2710 --->\n");
    cmd = CHIPID_XC6130_OV2710;
    if (ioctl(gFd, cmd, 0) < 0)
        {
            printf("Call cmd MEMDEV_IOCPRINT fail\n");
            return -1;
    }
#endif      

	char *cmd = argv[1];
	printf("[%s:%d]argc = %d, argv[1] = %s\n",__FILE__, __LINE__, argc, cmd);
	if(strcmp(cmd, "reg") == 0 && argc > 2) {
		char b = *argv[2];
		if((b == 'r'  && argc > 3)) {
			int c = *argv[3];
			sscanf(argv[3], "0x%x", &c);
			printf("[%s:%d] abc= %s %c 0x%x\n",__FILE__, __LINE__, cmd, b, c);
			xc6130_reg_read(c);
		}

	}
	else if(strcmp(cmd, "wb") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] ab= %s %c\n",__FILE__, __LINE__, cmd, b);
		if(b == '0') {
		    printf("------------wb_array_old-----------\n");
			xc6130_write_array(wb_array_old, sizeof(wb_array_old));
		}
		else if(b == '1') {
		    printf("------------wb_array_new-----------\n");
			xc6130_write_array(wb_array_new, sizeof(wb_array_new));
		}
			
	}
    
    
    close(gFd);
    return 0;    
}
