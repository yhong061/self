
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int file_open(char *infile, char *inbuf, unsigned int insize)
{
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		return -1;
	}
	printf("open %s OK\n", infile);
	
	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d != insize = %d\n", readsize, insize);
	}
	fclose(fIn);

	return readsize;
}


int file_save(char *outfile, char *bufout, unsigned int outsize)
{
	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		return -1;
	}
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d != outsize = %d\n", writesize, outsize);
	}

	fclose(fOut);

	return 0;
}


int main_raw2rgb(char *infile, char *outfile)
{
	unsigned int bufsize = 320 * 240 * 4; //YUYV
	unsigned int insize = bufsize/4;
	unsigned int outsize = bufsize;
	unsigned int readsize;
	char *inbuf = (char *)malloc(bufsize);
	char *outbuf = (char *)malloc(bufsize);
	char *src;
	char *dst;
	unsigned int i;
	assert(inbuf);
	assert(outbuf);

	readsize = file_open(infile, inbuf, insize);

	src = inbuf;
	dst = outbuf;
	for(i=0; i<insize; i++) {
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = 0;
		src++;
	}

	file_save(outfile, outbuf, outsize);


	free(inbuf);
	free(outbuf);

	return 0;

}

int main_raw2rgb2(char *infile, char *outfile)
{
	unsigned int bufsize = 320 * 240 * 4; //YUYV
	unsigned int insize = bufsize/2;
	unsigned int outsize = bufsize;
	unsigned int readsize;
	char *inbuf = (char *)malloc(bufsize);
	char *outbuf = (char *)malloc(bufsize);
	char *src;
	char val;
	char *dst;
	unsigned int i;
	assert(inbuf);
	assert(outbuf);

	readsize = file_open(infile, inbuf, insize);

	src = inbuf;
	dst = outbuf;
	for(i=0; i<insize; i++) {
		val = *src++ >> 4 | *src++ << 4;
		*dst++ = val;
		*dst++ = val;
		*dst++ = val;
		*dst++ = 0;
	}

	file_save(outfile, outbuf, outsize);


	free(inbuf);
	free(outbuf);

	return 0;

}

int main_raw2txt(char *infile, char *outfile)
{
	unsigned int width = 320;
	unsigned int height = 240;
	unsigned int bufsize = 173040; //YUYV
	unsigned int insize = 320*240;
	unsigned int outsize = bufsize;
	unsigned int readsize;
	char *inbuf = (char *)malloc(bufsize);
	char *outbuf = (char *)malloc(bufsize);
	int *src;
	char *dst;
	unsigned int i, j;
	char tmp[32];
	assert(inbuf);
	assert(outbuf);

	readsize = file_open(infile, inbuf, insize);

	src = (int *)inbuf;
	dst = outbuf;

	tmp[0] = '\0';
	outbuf[0] = '\0';

	for (i = 0; i < height ; i++) {
		for (j = 0; j < width ; j+=4) {
			sprintf(tmp, "%02x", *src++);
			strcat(outbuf, tmp);
		}
if(i<5)
printf("strlen = %d\n", (int)strlen(outbuf));
		sprintf(tmp, "\n");
		strcat(outbuf, tmp);
if(i<5)
printf("strlen = %d\n", (int)strlen(outbuf));
	}
printf("strlen = %d\n", (int)strlen(outbuf));
	file_save(outfile, outbuf, outsize);


	free(inbuf);
	free(outbuf);

	return 0;

}

void usage(void)
{
	printf("---------------------\n");
	printf("./test idx infile outfile\n");
	printf("idx : \n");
	printf("0: raw to rgb32\n");
	printf("1: raw to txt\n");
	printf("---------------------\n");
}


int main(int argc, char* argv[])
{
	if(argc<=3) {
		usage();
		return 0;
	}

	int idx = atoi(argv[1]);
	char *infile = argv[2];
	char *outfile = argv[3];

	switch(idx){
	case 0:
		main_raw2rgb(infile, outfile);
	break;
	
	case 1:
		main_raw2txt(infile, outfile);
	break;
	
	case 2:
		main_raw2rgb2(infile, outfile);
	break;
	
	
	}

	return 0;
}

