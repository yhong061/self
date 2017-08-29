#include <stdio.h>
#include <string.h>
#include <sys/time.h>

typedef unsigned short int uint16_t;
int test_saveFile(char *name, char *buf, unsigned int size)
{
	struct timeval tv;
	char filename[64];
	FILE *fp = NULL;

	gettimeofday(&tv, NULL);
	filename[0] = '\0';
	sprintf(filename, "%s_%10ld_%06ld.bin", name, tv.tv_sec, tv.tv_usec);
	fp = fopen(filename, "wb");
	if(fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char testbuf[] = "11111111222222222";
	uint16_t *buf = (uint16_t *)testbuf;
	unsigned int size = strlen(testbuf) + 1;
	test_saveFile("Image", (char *)buf, (unsigned int)size);
	return 0;
}
