#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int hyi = 0x110/2;

unsigned int rgb24toyuv422(char *rgb, char *yuv, unsigned int w, unsigned int h)
{
	unsigned int yuvsize = w * h * 2 * 2;
	unsigned int rgbsize = w * h * 2 * 3;
	unsigned int i;
	char Y0,Y1,Cb,Cr;
	char R0,G0,B0,R1,G1,B1;
	int offset = 0;
	char *src = rgb;
	char *dst = yuv;

	for(i=0; i<rgbsize; i += 6){
		R0 = src[i + 0];
		G0 = src[i + 1];
		B0 = src[i + 2];
		R1 = src[i + 3];
		G1 = src[i + 4];
		B1 = src[i + 5];

		Y0 = (char)( 0.257*R0 + 0.564*G0 + 0.098*B0 +16);
		Cb = (char)(-0.148*R0 - 0.291*G0 + 0.439*B0 +128);
		Cr = (char)( 0.439*R0 - 0.368*G0 - 0.071*B0 +128);
		Y1 = (char)( 0.257*R1 + 0.564*G1 + 0.098*B1 +16);

if(i  == hyi * 3){
	printf("i = %x , RGB=%d %d %d %d %d %d - YUV = %d %d %d %d\n", i, R0, G0, B0, R1, G1, B1, Y0, Cb, Y1, Cr);
}

		*dst++ = Y0;
		*dst++ = Cb;
		*dst++ = Y1;
		*dst++ = Cr;
	}
	return yuvsize;
}

unsigned int yuv422torgb24(char *yuv, char *rgb, unsigned int w, unsigned int h)
{
	unsigned int yuvsize = w * h * 2 * 2;
	unsigned int rgbsize = w * h * 2 * 3;
	unsigned int i;
	char Y0,Y1,Cb,Cr;
	char R0,G0,B0,R1,G1,B1;
	int offset = 0;
	char *src = yuv;
	char *dst = rgb;

	for(i=0; i<yuvsize; i += 4){
		Y0 = src[i + 0];
		Cb = src[i + 1];
		Y1 = src[i + 2];
		Cr = src[i + 3];

		R0 = (char)1.164*(Y0-16)+1.596*(Cr-128);
		G0 = (char)1.164*(Y0-16)-0.392*(Cb-128)-0.813*(Cr-128);
		B0 = (char)1.164*(Y0-16)+2.017*(Cb-128);
		*dst++ = R0;
		*dst++ = G0;
		*dst++ = B0;

		R1 = (char)1.164*(Y1-16)+1.596*(Cr-128);
		G1 = (char)1.164*(Y1-16)-0.392*(Cb-128)-0.813*(Cr-128);
		B1 = (char)1.164*(Y1-16)+2.017*(Cb-128);
		*dst++ = R1;
		*dst++ = G1;
		*dst++ = B1;
if(i == hyi * 2){
	printf("i = %x , RGB=%d %d %d %d %d %d - YUV = %d %d %d %d\n", i, R0, G0, B0, R1, G1, B1, Y0, Cb, Y1, Cr);
}

	}
	return rgbsize;
}

unsigned int depth2yuv422(char *depth, char *yuv, unsigned int w, unsigned int h)
{
	unsigned int depthsize = w * h * 2 * 2;
	unsigned int yuvsize = w * h * 2 * 2;
	unsigned int i;
	char *src = depth;
	char *dst = yuv;
	for(i=0; i<depthsize; i += 4){
		*dst++ = src[i + 0];
		*dst++ = 116;;
		*dst++ = src[i + 1];
		*dst++ = 140;
	}
	return yuvsize;
}

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

int file_save_2(char *outfile, char *bufout, unsigned int outsize, int offset)
{
	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		return -1;
	}

	fseek(fOut, offset, SEEK_CUR);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d != outsize = %d\n", writesize, outsize);
	}

	fclose(fOut);

	return 0;
}

int main_depth(char *infile)
{
	unsigned int bufsize = 224 * 172 * 2 * 3; //YUYV
	unsigned int size;
	char *depth;
	char *yuv422;
	char *rgb24;
	char *inbuf = (char *)malloc(bufsize);
	char *bufout = (char *)malloc(bufsize);
	assert(inbuf);
	assert(bufout);

	size = file_open(infile, inbuf, bufsize);

	depth  = inbuf;
	yuv422 = bufout;
	size = depth2yuv422(depth, yuv422, 224, 172);
	file_save("1.yuv422", yuv422, size);

	yuv422 = bufout;
	rgb24  = inbuf;
	size = yuv422torgb24(yuv422, rgb24, 224, 172);
	file_save("2.rgb24", rgb24, size);

	rgb24  = inbuf;
	yuv422 = bufout;
	size = rgb24toyuv422(rgb24, yuv422, 224, 172);
	file_save("3.yuv422", yuv422, size);
	


	free(inbuf);
	free(bufout);

	return 0;

}

int main_bmp(char *infile)
{
	unsigned int bufsize = 224 * 173 * 2 * 3 + 54; //YUYV
	unsigned int size;
	char *depth;
	char *yuv422;
	char *rgb24;
	char *inbuf = (char *)malloc(bufsize);
	char *bufout = (char *)malloc(bufsize);
	assert(inbuf);
	assert(bufout);

	size = file_open(infile, inbuf, bufsize);

	rgb24  = inbuf+54;
	yuv422 = bufout;
	size = rgb24toyuv422(rgb24, yuv422, 224, 173);
	file_save("3.yuv422bmp", yuv422, size);


	free(inbuf);
	free(bufout);

	return 0;

}

int main_rgb(char *infile)
{
	unsigned int bufsize = 224 * 173 * 2 * 3 + 54; //YUYV
	unsigned int size, sizesave;
	char *depth;
	char *yuv422;
	char *rgb24;
	char *inbuf = (char *)malloc(bufsize);
	char *bufout = (char *)malloc(bufsize);
	assert(inbuf);
	assert(bufout);

	sizesave = file_open(infile, inbuf, bufsize);
	size = file_open("2.rgb24", bufout, bufsize);
	memcpy(inbuf+54, bufout, size);

	file_save(infile, inbuf, sizesave);


	free(inbuf);
	free(bufout);

	return 0;

}

void usage(void)
{
	printf("---------------------\n");
	printf("./test idx infile\n");
	printf("idx : \n");
	printf("0 depth: depth - yuv422 - rgb24 - yuv422\n");
	printf("1 bmp: rgb24 - yuv422\n");
	printf("2 bmp: rgb24 - bmp\n");
	printf("---------------------\n");
}


int main(int argc, char* argv[])
{
	if(argc<=2) {
		usage();
		return 0;
	}

	int idx = atoi(argv[1]);
	char *infile = argv[1];

	switch(idx){
	case 0:
		main_depth(infile);
	break;
	
	case 1:
		main_bmp(infile);
	break;
	
	case 2:
		main_rgb(infile);
	break;
	
	}

	return 0;
}


