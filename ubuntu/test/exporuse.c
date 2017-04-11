#include <stdio.h>

int main(int argc, char *argv[])
{
	char *p = argv[1];
	unsigned int date_80;
	unsigned int date_60;
	unsigned int val_80;
	unsigned int val_60;
	unsigned int base_80 = 0x3ec;
	unsigned int base_60 = 0x2f1;
	int offset_80 = 0x4000;
	int offset_60 = 0x4000;

	if(strcmp(p, "10") == 0) {
		date_80 = strtoul(argv[2], NULL, 10);
		date_60 = date_80;
		if(date_80 > 1600) {
			base_80 = 0xfb;
			offset_80 = 0x8fb0;
			date_80 -= 1600;
		}
		val_80  = (date_80*base_80)/100 + offset_80;
		val_60  = (date_60*base_60)/100 + offset_60;
		printf("p = %s, date_60 = %d, val_80 = 0x%x, val_60 = 0x%x\n", p, date_60, val_80, val_60);
	}
	else if(strcmp(p, "16") == 0){
		date_80 = strtoul(argv[2], NULL, 16);
		date_60 = date_80;

		if(date_80 >= 0x8fb0) {
			base_80 = 0xfb;
			offset_80 = 0x8fb0;
		}
		val_80  = (date_80 + 1 - offset_80) * 100 /  base_80;
		val_60  = (date_60 + 1 - offset_60) * 100 /  base_60;
		if(date_80 >= 0x8fb0) {
			val_80 += 1600;
		}
		printf("p = %s, date_60 = 0x%x, val_80 = %d, val_60 = %d\n", p, date_60, val_80, val_60);
	}

	return 0;
}
