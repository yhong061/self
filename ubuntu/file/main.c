#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef unsigned short int uint16_t;

#define SAVE_PATH "."

int test_saveFile(char *name, char *buf, unsigned int size)
{
	char filename[64];
	FILE *fp = NULL;

	filename[0] = '\0';
	sprintf(filename, "%s/%s.bin", SAVE_PATH, name);
	printf("save: %s\n", filename);
	fp = fopen(filename, "a+");
	if(fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
	return 0;
}
															
int main()
{
	int size = 24;
	uint16_t *data = (uint16_t *)malloc(size*sizeof(uint16_t));

	test_saveFile("Image", (char *)data, size*sizeof(uint16_t));
	return 0;
}


