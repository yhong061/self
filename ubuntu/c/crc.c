#include <stdio.h>

#define POLY 0x8408
unsigned short crc16(char *data_p, unsigned short length)
{
	unsigned char i;
	unsigned int data;
	unsigned int crc = 0xffff;
	unsigned int crc_old = crc;

	if(length == 0)
		return (unsigned short)(crc);
	
	do{
		printf("data_p = 0x%02x, ", *data_p);
		for(i=0, data=(unsigned int)0xff & *data_p++; i<8; i++,data>>=1)
		{
			printf("[%d: D=0x%02x C=0x%04x] ", i, data, crc);
			if((crc & 0x0001) ^ (data & 0x0001)) {
				printf(" + ");
				crc = (crc >> 1) ^ POLY;
			}
			else {
				printf(" - ");
				crc >>= 1;
			}
		}
		printf("\n");
	
	}while(--length);

	return (unsigned short)(crc);
}

int main(int argc, char *argv[])
{
	unsigned short crc = 0;
	char array[2] = {0x12, 0x34};
	printf("array = 0x%02x, 0x%02x\n", array[0], array[1]);
	crc = crc16(array, 2);
	printf("crc16 = 0x%02x\n", crc);

	return 0;
}

/* print:
array = 0x12, 0x34
data_p = 0x12, [0: D=0x12 C=0xffff]  + [1: D=0x09 C=0xfbf7]  - [2: D=0x04 C=0x7dfb]  + [3: D=0x02 C=0xbaf5]  + [4: D=0x01 C=0xd972]  + [5: D=0x00 C=0xe8b1]  + [6: D=0x00 C=0xf050]  - [7: D=0x00 C=0x7828]  - 
data_p = 0x34, [0: D=0x34 C=0x3c14]  - [1: D=0x1a C=0x1e0a]  - [2: D=0x0d C=0x0f05]  - [3: D=0x06 C=0x0782]  - [4: D=0x03 C=0x03c1]  - [5: D=0x01 C=0x01e0]  + [6: D=0x00 C=0x84f8]  - [7: D=0x00 C=0x427c]  - 
crc16 = 0x213e
*/
