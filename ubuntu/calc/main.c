#include <stdio.h>
typedef unsigned short int uint16_t;
static uint16_t saturationData;
static uint16_t saturationBit = 0x1000;
static int saturation = 1;

void saturationSet4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	saturationData = dcs0 | dcs1 | dcs2 | dcs3;
	printf("saturationData = 0x%x\n", saturationData);
}

int saturationCheck(){
	if (!saturation){
		return 0;
	}
	return saturationData & saturationBit;
}

int main(int argc, char * argv[])
{
	uint16_t x1 = 1;
	uint16_t x2 = 2;
	uint16_t x3 = 4;
	uint16_t x4 = 8;
printf("argc = %d \n", argc);
	if(argc == 5) {
		x1 = atoi(argv[1]);
		printf("%x\n", x1);
		x2 = atoi(argv[2]);
		printf("%x\n", x2);
		x3 = atoi(argv[3]);
		printf("%x\n", x3);
		x4 = atoi(argv[4]);
		printf("%x\n", x4);
	}
	printf("xn = %x, %x, %x, %x\n", x1, x2, x3, x4);

	saturationSet4DCS(x1,x2,x3,x4);
	int val = saturationCheck();
	printf("val = 0x%x\n", (unsigned int)val);
	return 0;
}


