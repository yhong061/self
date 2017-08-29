#include <stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

enum {
  CAMERA_REG_READ,
  CAMERA_REG_WRITE,
  CAMERA_REG_READ_ARRAY,
  CAMERA_REG_WRITE_ARRAY,
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
};

int camera_reg_read(unsigned int i2c_addr, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned int *data)
{
	int ret;
	int cmd;
	struct reg_buf arg;

	arg.i2c_addr = i2c_addr;
	arg.reg_width = reg_width;
	arg.reg = reg;
	arg.data_width = data_width;
	arg.data = 0;

    	cmd = CAMERA_REG_READ;
    	ret = ioctl(gFd, cmd, &arg);
	if(ret != 0)
        {
            printf("Call cmd CAMERA_REG_READ fail, ret = %d\n", ret);
            return -1;
    	}

//	printf("reg read : %x\n", arg.data);
	*data = arg.data;

	return 0;
}


int camera_reg_open(void)
{
	if(!gFd) {
		gFd = open(DEV_NAME,O_RDWR);
		if (gFd < 0)
		{
			printf("Open Dev Mem0 Error!\n");
			return -1;
		}
	}
	return 0;
}

int camera_reg_close(void)
{
	if(gFd) {
		close(gFd);
		gFd = 0;
	}
	return 0;
}

float temperature_read(void)
{
	unsigned int i2c_addr;
	unsigned int reg_width;
	unsigned int reg;
	unsigned int data_width;
	unsigned int data;
	float temperature;

	i2c_addr = 0x90;
	reg_width = 1;
	reg = 0x00;
	data_width = 2;
	data = 0;

	camera_reg_read(i2c_addr, reg_width, reg, data_width, &data);
	printf("get tmp = %x\n", data);
	
	/*byteorder*/
	data = (data  << 8 & 0xff00)  |  data >> 8;
	temperature = (data>>4)*0.065;
	
	return temperature;
}

#define EEPROM_SIZE  (64*1024)
#define EEPROM_NUM   (2)
#define EEPROM_I2C_ADDR_1 (0xAC)
#define EEPROM_I2C_ADDR_2 (0xAE)
int eeprom_read_byte(unsigned int eeprom_i2c_addr, unsigned int eeprom_reg, unsigned char *eeprom_data)
{
	int ret;
	unsigned int i2c_addr;
	unsigned int reg_width;
	unsigned int reg;
	unsigned int data_width;
	unsigned int data;
	
	i2c_addr = eeprom_i2c_addr;
	reg_width = 2;
	reg = eeprom_reg;
	data_width = 1;
	data = 0;

	ret = camera_reg_read(i2c_addr, reg_width, reg, data_width, &data);
	if(ret)
		return ret;
	*eeprom_data = (unsigned char)data;

	return 0;
}

int eeprom_readdata(char *filename)
{
	int ret = -1;
	unsigned int i,j;
	unsigned char eeprom_buf[EEPROM_SIZE*EEPROM_NUM];
	unsigned int eeprom_i2c_addr[EEPROM_NUM] = {EEPROM_I2C_ADDR_1, EEPROM_I2C_ADDR_2};
	//unsigned int eeprom_i2c_addr[EEPROM_NUM] = {EEPROM_I2C_ADDR_1};
	unsigned char *ptr;
	unsigned int i2c_addr;
	unsigned int reg;
	unsigned int offset;

	FILE *fp = NULL;
	unsigned int writesize;

	memset(eeprom_buf, 0, sizeof(eeprom_buf));

	for(i=0; i<EEPROM_NUM; i++) {
		offset = i*EEPROM_SIZE;
		i2c_addr = eeprom_i2c_addr[i];
		ptr = &eeprom_buf[offset];
		printf("[%d] i2c_addr = %x, offset = %x, ptr = %p\n", i, i2c_addr, offset, ptr);

		for(j=0; j<EEPROM_SIZE; j++) {
			//reg = offset+j;
			reg = j;
			ret = eeprom_read_byte(i2c_addr, reg, ptr);
			if(ret)
				return ret;
			ptr++;
			if(j%0x1000 == 0)
				printf("read eeprom: i2c addr[0x%x], reg[0x%x]\n", i2c_addr, reg);
		}
	}

	fp = fopen(filename, "w");
	if(fp) {
		writesize= fwrite(eeprom_buf, 1, EEPROM_SIZE*EEPROM_NUM, fp);
		fclose(fp);
		ret = 0;
	}

	return ret;
}

int eeprom_read(char *filename)
{ 
    printf("================read_eeprom: start============\n");
//    camera_reg_open();
    eeprom_readdata(filename); 
//    camera_reg_close();
    printf("================read_eeprom: end  ============\n");

    return 0;
}

#if 0
void usage_help(void)
{
	printf("===============================\n");
	printf("sample: \n");
	printf("./test_mem temp   r             : temprature read\n");
	printf("./test_mem eeprom   r             : eeprom read\n");
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
    
	gFd = open(DEV_NAME,O_RDWR);
	if (gFd < 0)
	{
		printf("Open Dev Mem0 Error!\n");
		return -1;
	}

	char *cmd = argv[1];
	if(strcmp(cmd, "temp") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] cmd= %s %c\n",__func__, __LINE__, cmd, b);
		if(b == 'r') {
		    printf("------------temp read-----------\n");
		    temperature_read();
		}
		else {
			printf("[%s:%d] temp error: cmd= %s\n",__func__, __LINE__, cmd);
		}
	}
	else if(strcmp(cmd, "eeprom") == 0 && argc > 2) {
		char b = *argv[2];

		printf("[%s:%d] cmd= %s %c\n",__func__, __LINE__, cmd, b);
		if(b == 'r') {
		    printf("------------eeprom read-----------\n");
		    char filename[] = "/data/eeprom.bin";
		    eeprom_read(filename);
		}
		else {
			printf("[%s:%d] eeprom error: cmd= %s\n",__func__, __LINE__, cmd);
		}
			
	}
	else {
		printf("[%s:%d] error: cmd= %s\n",__func__, __LINE__, cmd);
	}
    
    
    close(gFd);
    return 0;    
}
#endif
