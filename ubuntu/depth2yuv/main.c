#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define FILE_INPUT "1.depth"
#define FILE_OUTPUT "2.depth"
#define FILE_SIZE (224*172*4*2)

#define GRBFILE_SIZE (224*172*4)
#define GRBFILE_INPUT "2.depth"
#define GRBFILE_OUTPUT "3.yuv"

#define YUVFILE_SIZE (224*172*2)
#define YUVFILE_INPUT "3.rgb"
#define YUVFILE_OUTPUT "4.yuv"

#define TXTFILE_SIZE (224*172*4)
#define TXTFILE_INPUT "2.depth"
#define TXTFILE_OUTPUT "3.txt"

#define DEPTHFILE_SIZE (224*172*4)

#define  CATCH_LEFT     (107)
#define  CATCH_RIGHT    (117)
#define  CATCH_TOP      (81)
#define  CATCH_BOTTOM   (91)

#define   DISTANCE_STEP_INT  (30)

#define SRC_PIC_WIDTH     (224)
#define SRC_PIC_HEIGHT  (172)
#define SRC_DEPTH_HEIGHT  (172)

#define PIC_SIZE_Z  (616448)

typedef unsigned char uint_8;

#define  CALI_DEGREE_TOTAL  (8)

float disDivide[CALI_DEGREE_TOTAL] = {0.23, 0.55, 0.8, 1.15, 1.4, 1.75, 2.0};

float factor[CALI_DEGREE_TOTAL][2] = {
	{ 1.0625, -0.0456 }, { 0.8039, 0.0172 },
	{ 1.2538, -0.2246 }, { 0.8310, 0.1111 },
	{ 1.2538, -0.3821 }, { 0.8242, 0.2234 },
	{ 1.1952, -0.4378 }, { 1.0,   -0.5676 }
};

static inline float calibrateDis(float dis) {
	int fact_index = CALI_DEGREE_TOTAL -1;
	int i;
	for (i = 0; i < (CALI_DEGREE_TOTAL - 1); i++) {
		if (dis <= disDivide[i]) {
			fact_index = i;
			break;
		}
	}

	dis *= factor[fact_index][0];
	dis += factor[fact_index][1];
	return dis;
}


float bytesToFloat(unsigned char *src, int offset) {
	float *fp;
	fp = (float *) (&src[offset]);
	return *fp;
}

int cloud2depth(unsigned char *in, int *pOutDepth) {
	int i;
	int sampleNum = 0;
	int sampleNumOrg = 0;
	float averageDis = 0.0;
	float averageOrg = 0.0;

	int left = CATCH_LEFT;
	int right = CATCH_RIGHT;
	int top = CATCH_TOP;
	int bottom = CATCH_BOTTOM;
	FILE *fpCatch = NULL;
	char catchName[128];


	for (i = 0; i < PIC_SIZE_Z; i = i + 16) {
		float dis_org = bytesToFloat(in, i + 8);
		float distance = calibrateDis(dis_org);
		float distance_mm = distance;
		distance *= 1000;
		int color;
		int dis = (int) distance;

		if (dis >= 0 && dis < 65536) {
			color = dis;
		} else {
			color = 0;
		}
		pOutDepth[i / 16] = color & 0xFFFF;
	}
	
	return (int) averageDis;
}

int main_cloud2depth(char *infile, char *outfile)
{
	unsigned int insize = DEPTHFILE_SIZE*4;
	unsigned int outsize = DEPTHFILE_SIZE;
	char *buf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(buf);
	assert(bufout);

	memset(bufout, 0, outsize);
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = cloud2depth((unsigned char*)buf, (int*)bufout);
	printf("\nret = %d\n", ret);
	
	unsigned int writesize = fwrite(buf, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}


int rgb2yuv_test(int *in_rgb, int *out_depth) 
{

	int width  = SRC_PIC_WIDTH;
	int height = SRC_DEPTH_HEIGHT; 
	unsigned int size = width * height;
	char *ptry,*ptru,*ptrv;
	unsigned int i,j;
	
	ptry = (char *)out_depth;
	ptru = (char *)out_depth;
	ptrv = (char *)out_depth;
	unsigned int offset = 0;

	for(j=0; j<height; j++) {
		for(i=0; i<width; i++) {
			if(offset%2 == 0) {
				ptry[offset++] = 0x00;
				ptru[offset++] = 0x80;
			}
			else {
				ptry[offset++] = 0x00;
				ptrv[offset++] = 0x80;
			}
		}
	}

	return 0;
}

int rgb2yuv(int *in_rgb, int *out_depth) 
{
/*
Y =  0.299R + 0.587G + 0.114B
U = -0.147R - 0.289G + 0.436B
V =  0.615R - 0.515G - 0.100B

Y = 0.30*R + 0.59G + 0.11B
U = 0.493(B-Y)
V = 0.877(R-Y)

Y = 0.30*R + 0.59G + 0.11B
U = 0.493(B-Y)
V = 0.877(R-Y)

*/

	int width  = SRC_PIC_WIDTH;
	int height = SRC_DEPTH_HEIGHT; 
	int i,j;
	char *rgb;
	char *ptry,*ptru,*ptrv;
	uint_8 r,g,b;
	uint_8 y,u,v;
	unsigned int offset;
	unsigned int size = width * height;

	ptry = (char *)out_depth;
	ptru = (char *)out_depth;
	ptrv = (char *)out_depth;
	unsigned int rgb_offset = 0;
	unsigned int yuv_offset = 0;
	for(rgb_offset=0; rgb_offset<size; rgb_offset++) {
		rgb  = (char *)&in_rgb[rgb_offset];
		r = rgb[0];
		g = rgb[1];
		b = rgb[2];

		y = (uint_8)( 0.257*r + 0.504*g + 0.098*b +16);
		u = (uint_8)(-0.148*r - 0.291*g + 0.439*b +128);
		v = (uint_8)( 0.439*r - 0.368*g - 0.071*b +128);

		if(rgb_offset%2 == 0) {
			ptry[yuv_offset++] = y;
			ptru[yuv_offset++] = u;
		}
		else {
			ptry[yuv_offset++] = y;
			ptrv[yuv_offset++] = v;
		}
	}

	return 0;
}

int calc_win(int *pDepth, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, 
	unsigned int curi, unsigned int curj, unsigned int curw, unsigned int curh)
{
	unsigned int i,j;
	int val;
	int sum = 0;
	int cnt = 0;
	int *in = pDepth;

if(curi == 17 && curj == 30)
printf("x=%03d-%03d-%03d : %03d   x=%03d-%03d-%03d : %03d\n", x1, curi, x2, curh, y1, curj, y2, curw);

	for(i=x1; i<=x2; i++) {
if(curi == 17 && curj == 30)
	printf("\n");
		for(j=y1; j<=y2; j++){
			val = in[i*curw +j];
if(curi == 17 && curj == 30)
	printf("[%03d][%03d] %02x ", i, j, val);
			if(i == curi && j == curj)
				continue;

			if(val == 0)
				return 0;
			cnt++;
			sum += val;
		}
	}
	return sum/cnt;
}

int depthdata_adjust_win(int *pDepth, int *pDepth_a)
{
	unsigned int i,j;
	unsigned int x1,x2,y1,y2;
	int *in = pDepth;
	int *out = pDepth_a;
	int val;
	int step = 1;
	int w = SRC_PIC_WIDTH;
	int h = SRC_PIC_HEIGHT;

//int off = 30;

	for (i = 0; i < h; i++) {
//if(i == 18 || i == 17 ) 
//	printf("\n");
		for (j = 0; j < w; j++) {
			val = in[i*w + j];

//if(i == 18 || i == 17 ) 
///	if( j >= off && j < off+10)
//		printf("[%d][%d] %02x ", i,j,val);
			if(val != 0) 
				continue;

			//val == 0
			x1 = i-step;
			if(i == 0)
				x1 = i;
			
			x2 = i+step;
			if(i == h-1)
				x2 = i;
			
			y1 = j-step;
			if(j == 0)
				y1 = j;
			
			y2 = j+step;
			if(j == w-1)
				y2 = j;
			
			
			in[i*w +j] = calc_win(pDepth, x1, y1, x2, y2, i, j, w, h);
		}
	}
	return 0;
}

int depthdata_yuvrgb(int *pDepth, int *pDepth_a)
{
	unsigned int i;
	char *src = (char *)pDepth;
	char *dst = (char *)pDepth_a;
	int val;

	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++) {
		*dst++ = src[1];
		*dst++ = 0x80;
		*dst++ = src[0];
		*dst++ = 0x80;
		src+= 4;
	}
	return 0;
}

int depthdata_adjust(int *pDepth, int *pDepth_a)
{
	unsigned int i;
	int *in = pDepth;
	int *out = pDepth_a;
	int *idx = pDepth_a + (SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT);
	int val;

	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++) {
		if(*in != 0) {
			*out = *in;
			*idx = 0;
		}
		else {
			if(*out > 0) {
				val = *idx;
				if(i < 50)
					printf("[i=%d] *idx = %u\n", i, *idx);
				*idx = val + 1;
				if(*idx > 10) {
					*out = *in;
					*idx = 0;
				}
			}
			else {
				*idx = 0;
			}
		}
		in++;
		out++;
		idx++;
	}
	return 0;
}


int depth2yuv(int *in_depth, int *out_depth) 
{
	int i;
	int averageDis = 0;
	int sampleNum = 0;
	
	char *rgb;
	char *ptry,*ptru,*ptrv;
	uint_8 r,g,b;
	uint_8 y,u,v;
	unsigned int yuv_offset = 0;
	
	ptry = (char *)out_depth;
	ptru = (char *)out_depth;
	ptrv = (char *)out_depth;
	
	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++) {
		int distance = in_depth[i];
		int line = i / (SRC_PIC_WIDTH);
		int column = i % SRC_PIC_WIDTH;
		if (0/*line >= CATCH_TOP && line < CATCH_BOTTOM*/) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;        
				} else {          
					sampleNum++;        
				}        
				averageDis += distance;      
			}    
		}    
		int color;    
		int changeColor = 0;   
		//   color       alpha          B         G         R       distance    
		//   black       255            0         0         0       // 0    
		//   red         255            0         0       255       // 0 ---- 30    
		//   yellow      255            0       255       255       // 30 ---- 60    
		//   green       255            0       255         0       // 60 ---- 90    
		//   cyan        255          255       255         0       // 90 ---- 120    
		//   blue        255          255         0         0       // 120 ---- 150    
		//   white       255          255       255       255       // > 150    
		//    
		if (distance == 0) {      
			color = 0xFF000000;    
		} else if (distance > 0 && distance <= DISTANCE_STEP_INT) { //color change from black to red, red change from 0 to 255 
			color = 0xFF000000;      
			changeColor = (int) (distance * 255 / DISTANCE_STEP_INT);      
			color += changeColor;    
		} else if (distance > DISTANCE_STEP_INT && distance <= 2 * DISTANCE_STEP_INT) { //color change from read to yellow, green change from 0 to 255      
			distance -= DISTANCE_STEP_INT;      
			color = 0xFF0000FF;      
			changeColor = ((int) ((distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color += changeColor;    
		} else if (distance > 2 * DISTANCE_STEP_INT && distance <= 3 * DISTANCE_STEP_INT) { //color change from yellow to green, red change form 255 to 0      
			distance -= 2 * DISTANCE_STEP_INT;      
			color = 0xFF00FFFF;      
			changeColor = (int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT);      
			color -= changeColor;    
		} else if (distance > 3 * DISTANCE_STEP_INT && distance <= 4 * DISTANCE_STEP_INT) { //color change from green to cyan, blue change from 0 to 255  
			distance -= 3 * DISTANCE_STEP_INT;      
			color = 0xFF00FF00;      
			changeColor = ((int) (distance * 255 / DISTANCE_STEP_INT)) << 16;      
			color += changeColor;    
		} else if (distance > 4 * DISTANCE_STEP_INT && distance <= 5 * DISTANCE_STEP_INT) { //color change from cyan to blue, green change form 255 to 0  
			distance -= 4 * DISTANCE_STEP_INT;      
			color = 0xFFFFFF00;      
			changeColor = ((int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color -= changeColor;    
		} else {      
			color = 0xffffffff;    
		}    

		/*
		Y =  0.257R + 0.504G + 0.098B + 16
		U = -0.148R - 0.291G + 0.439B + 128
		V =  0.439R - 0.368G - 0.071B + 128
		*/
		
		rgb = (char *)&color;
		
		r = rgb[0];
		g = rgb[1];
		b = rgb[2];

		y = (uint_8)( 0.257*r + 0.504*g + 0.098*b +16);
		u = (uint_8)(-0.148*r - 0.291*g + 0.439*b +128);
		v = (uint_8)( 0.439*r - 0.368*g - 0.071*b +128);

		if(i%2 == 0) {
			ptry[yuv_offset++] = y;
			ptru[yuv_offset++] = u;
		}
		else {
			ptry[yuv_offset++] = y;
			ptrv[yuv_offset++] = v;
		}
	}  
	if (sampleNum > 0) {    
		averageDis /= sampleNum;    
		if(averageDis)        
			printf("averageDis=%d, sampleNum=%d\n", averageDis, sampleNum);  
	} else {    
		averageDis = 0;  
	}  
	return averageDis;
}

int depth2yuv2(int *in_depth, int *out_depth) 
{
	int i;
	int averageDis = 0;
	int sampleNum = 0;
	
	char *rgb;
	char *ptry,*ptru,*ptrv;
	uint_8 r,g,b;
	uint_8 y,u,v;
	unsigned int yuv_offset = 0;
	
	ptry = (char *)out_depth;
	ptru = (char *)out_depth;
	ptrv = (char *)out_depth;
	
	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++) {
		int distance = in_depth[i];
		int line = i / (SRC_PIC_WIDTH);
		int column = i % SRC_PIC_WIDTH;
		if (0/*line >= CATCH_TOP && line < CATCH_BOTTOM*/) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;        
				} else {          
					sampleNum++;        
				}        
				averageDis += distance;      
			}    
		}    
		int color;    
		int changeColor = 0;   
		//   color       alpha          B         G         R       distance    
		//   black       255            0         0         0       // 0    
		//   red         255            0         0       255       // 0 ---- 30    
		//   yellow      255            0       255       255       // 30 ---- 60    
		//   green       255            0       255         0       // 60 ---- 90    
		//   cyan        255          255       255         0       // 90 ---- 120    
		//   blue        255          255         0         0       // 120 ---- 150    
		//   white       255          255       255       255       // > 150    
		//    
		if (distance == 0) {      
			color = 0xFF000000;    
		} else if (distance > 0 && distance <= DISTANCE_STEP_INT) { //color change from black to red, red change from 0 to 255 
			color = 0xFF000000;      
			changeColor = (int) (distance * 255 / DISTANCE_STEP_INT);      
			color += changeColor;    
		} else if (distance > DISTANCE_STEP_INT && distance <= 2 * DISTANCE_STEP_INT) { //color change from read to yellow, green change from 0 to 255      
			distance -= DISTANCE_STEP_INT;      
			color = 0xFF0000FF;      
			changeColor = ((int) ((distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color += changeColor;    
		} else if (distance > 2 * DISTANCE_STEP_INT && distance <= 3 * DISTANCE_STEP_INT) { //color change from yellow to green, red change form 255 to 0      
			distance -= 2 * DISTANCE_STEP_INT;      
			color = 0xFF00FFFF;      
			changeColor = (int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT);      
			color -= changeColor;    
		} else if (distance > 3 * DISTANCE_STEP_INT && distance <= 4 * DISTANCE_STEP_INT) { //color change from green to cyan, blue change from 0 to 255  
			distance -= 3 * DISTANCE_STEP_INT;      
			color = 0xFF00FF00;      
			changeColor = ((int) (distance * 255 / DISTANCE_STEP_INT)) << 16;      
			color += changeColor;    
		} else if (distance > 4 * DISTANCE_STEP_INT && distance <= 5 * DISTANCE_STEP_INT) { //color change from cyan to blue, green change form 255 to 0  
			distance -= 4 * DISTANCE_STEP_INT;      
			color = 0xFFFFFF00;      
			changeColor = ((int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color -= changeColor;    
		} else {      
			color = 0xffffffff;    
		}    

		/*
		Y =  0.257R + 0.504G + 0.098B + 16
		U = -0.148R - 0.291G + 0.439B + 128
		V =  0.439R - 0.368G - 0.071B + 128
		*/
		
		rgb = (char *)&color;
		
		r = rgb[0];
		g = rgb[1];
		b = rgb[2];

		y = (uint_8)( 0.257*r + 0.504*g + 0.098*b +16);
		u = (uint_8)(-0.148*r - 0.291*g + 0.439*b +128);
		v = (uint_8)( 0.439*r - 0.368*g - 0.071*b +128);

		if(i%2 == 0) {
			ptry[yuv_offset++] = y;
			ptru[yuv_offset++] = u;
		}
		else {
			ptry[yuv_offset++] = y;
			ptrv[yuv_offset++] = v;
		}
	}  
	if (sampleNum > 0) {    
		averageDis /= sampleNum;    
		if(averageDis)        
			printf("averageDis=%d, sampleNum=%d\n", averageDis, sampleNum);  
	} else {    
		averageDis = 0;  
	}  
	return averageDis;
}

int main_depthadjust_win(char *infile, char *outfile)
{
	unsigned int insize = DEPTHFILE_SIZE;
	unsigned int outsize = DEPTHFILE_SIZE;
	char *buf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(buf);
	assert(bufout);

	memset(bufout, 0, outsize);
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = depthdata_adjust_win((int*)buf, (int*)bufout);
	printf("\nret = %d\n", ret);
	
	unsigned int writesize = fwrite(buf, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int parsecloud_xyz(int *inbuf, int *outbuf, int offset)
{
	unsigned int i;
	int *in  = inbuf+offset;
	int *out = outbuf;
	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++,in+=4,out++) {
		*out = *in;
	}
	return 0;
}

int main_parsecloud_xyz(char *bufin, char *outfile, int offset)
{
	unsigned int outsize = DEPTHFILE_SIZE;
	char *bufout = (char *)malloc(outsize);
	assert(bufout);

	memset(bufout, 0, outsize);
	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	parsecloud_xyz((int*)bufin, (int*)bufout, offset);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}

	fclose(fOut);
end1:
end0:
	free(bufout);

	return 0;

}
int main_parsecloud(char *infile, char *outfile)
{
	unsigned int insize = DEPTHFILE_SIZE * 4;
	char filename[128];
	char *buf = (char *)malloc(insize);
	assert(buf);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	unsigned int readsize = fread(buf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	filename[0] = '\0';
	sprintf(filename, "%s.x", outfile);
	main_parsecloud_xyz(buf, filename, 0);
	
	filename[0] = '\0';
	sprintf(filename, "%s.y", outfile);
	main_parsecloud_xyz(buf, filename, 1);
	
	filename[0] = '\0';
	sprintf(filename, "%s.z", outfile);
	main_parsecloud_xyz(buf, filename, 2);
	
	filename[0] = '\0';
	sprintf(filename, "%s.noise", outfile);
	main_parsecloud_xyz(buf, filename, 3);

	fclose(fIn);
end0:
	free(buf);

	return 0;

}

int main_depth2yuvrgb(char *infile, char *outfile)
{
	unsigned int insize = DEPTHFILE_SIZE;
	unsigned int outsize = DEPTHFILE_SIZE;
	char *buf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(buf);
	assert(bufout);

	memset(bufout, 0, outsize);
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = depthdata_yuvrgb((int*)buf, (int*)bufout);
	printf("\nret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int main_depthadjust(char *infile, char *outfile)
{
	unsigned int insize = DEPTHFILE_SIZE;
	unsigned int outsize = DEPTHFILE_SIZE * 2;
	char *buf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(buf);
	assert(bufout);

	memset(bufout, 0, outsize);
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}


	FILE *fOut = fopen(outfile, "rb");
	if(fOut != NULL) {
		unsigned int tmpsize = fread(bufout, 1, insize, fOut);
		printf("tmpsize = %d, insize = %d\n", tmpsize, insize);
		if(tmpsize != insize) {
			printf("tmpsize = %d, insize = %d\n", tmpsize, insize);
		}
		fclose(fOut);
	}

	fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = depthdata_adjust((int*)buf, (int*)bufout);
	//int ret = rgb2yuv_test((int*)buf, (int*)bufout);
	printf("\nret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int main_depth2yuv(char *infile, char *outfile)
{
	unsigned int size = YUVFILE_SIZE;
	unsigned int rgbsize = YUVFILE_SIZE * 2;
	char *buf = (char *)malloc(rgbsize);
	char *bufout = (char *)malloc(size);
	assert(buf);
	assert(bufout);

	memset(bufout, 0, size);
	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, rgbsize, fIn);
	printf("readsize = %d, rgbsize = %d\n", readsize, rgbsize);
	if(readsize != rgbsize) {
		printf("readsize = %d, rgbsize = %d\n", readsize, rgbsize);
	}

	int ret = depth2yuv2((int*)buf, (int*)bufout);
	//int ret = rgb2yuv_test((int*)buf, (int*)bufout);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, size, fOut);
	printf("writesize = %d, size = %d\n", writesize, size);
	if(writesize != size) {
		printf("writesize = %d, size = %d\n", writesize, size);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int depth2rgb(int *out_depth, int *outBmpDepth) 
{
	int i;
	int averageDis = 0;
	int sampleNum = 0;
	for (i = 0; i < SRC_PIC_WIDTH * SRC_DEPTH_HEIGHT; i++) {
		int distance = out_depth[i];
		int line = i / (SRC_PIC_WIDTH);
		int column = i % SRC_PIC_WIDTH;
		if (0/*line >= CATCH_TOP && line < CATCH_BOTTOM*/) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;        
				} else {          
					sampleNum++;        
				}        
				averageDis += distance;      
			}    
		}    
		int color;    
		int changeColor = 0;   
		//   color       alpha          B         G         R       distance    
		//   black       255            0         0         0       // 0    
		//   red         255            0         0       255       // 0 ---- 30    
		//   yellow      255            0       255       255       // 30 ---- 60    
		//   green       255            0       255         0       // 60 ---- 90    
		//   cyan        255          255       255         0       // 90 ---- 120    
		//   blue        255          255         0         0       // 120 ---- 150    
		//   white       255          255       255       255       // > 150    
		//    
		if (distance == 0) {      
			color = 0xFF000000;    
		} else if (distance > 0 && distance <= DISTANCE_STEP_INT) { //color change from black to red, red change from 0 to 255 
			color = 0xFF000000;      
			changeColor = (int) (distance * 255 / DISTANCE_STEP_INT);      
			color += changeColor;    
		} else if (distance > DISTANCE_STEP_INT && distance <= 2 * DISTANCE_STEP_INT) { //color change from read to yellow, green change from 0 to 255      
			distance -= DISTANCE_STEP_INT;      
			color = 0xFF0000FF;      
			changeColor = ((int) ((distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color += changeColor;    
		} else if (distance > 2 * DISTANCE_STEP_INT && distance <= 3 * DISTANCE_STEP_INT) { //color change from yellow to green, red change form 255 to 0      
			distance -= 2 * DISTANCE_STEP_INT;      
			color = 0xFF00FFFF;      
			changeColor = (int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT);      
			color -= changeColor;    
		} else if (distance > 3 * DISTANCE_STEP_INT && distance <= 4 * DISTANCE_STEP_INT) { //color change from green to cyan, blue change from 0 to 255  
			distance -= 3 * DISTANCE_STEP_INT;      
			color = 0xFF00FF00;      
			changeColor = ((int) (distance * 255 / DISTANCE_STEP_INT)) << 16;      
			color += changeColor;    
		} else if (distance > 4 * DISTANCE_STEP_INT && distance <= 5 * DISTANCE_STEP_INT) { //color change from cyan to blue, green change form 255 to 0  
			distance -= 4 * DISTANCE_STEP_INT;      
			color = 0xFFFFFF00;      
			changeColor = ((int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT)) << 8;      
			color -= changeColor;    
		} else {      
			color = 0xffffffff;    
		}    
		outBmpDepth[i] = color;  
	}  
	if (sampleNum > 0) {    
		averageDis /= sampleNum;    
		if(averageDis)        
			printf("averageDis=%d, sampleNum=%d\n", averageDis, sampleNum);  
	} else {    
		averageDis = 0;  
	}  
	return averageDis;
}

int main_depth2rgb(char *infile, char *outfile)
{
	unsigned int size = GRBFILE_SIZE;
	char *buf = (char *)malloc(size);
	char *bufout = (char *)malloc(size);
	assert(buf);
	assert(bufout);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	int ret = depth2rgb((int*)buf, (int*)bufout);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, size, fOut);
	printf("writesize = %d, size = %d\n", writesize, size);
	if(writesize != size) {
		printf("writesize = %d, size = %d\n", writesize, size);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int depth2txt(int *out_depth, char *bufout) 
{
	int i,j;
	int width = SRC_PIC_WIDTH;
	int height = SRC_DEPTH_HEIGHT;

	char tmp[32];
	int distance;

	bufout[0] = '\0';
	tmp[0] = '\0';
	for (i = 0; i < height ; i++) {
		for (j = 0; j < width ; j++) {
			distance = out_depth[i*width +j];
			sprintf(tmp, "%04x ", distance);
			strcat(bufout, tmp);
		}  
		sprintf(tmp, "\n");
		strcat(bufout, tmp);
	}
	return 0;
}

int main_txt(char *infile, char *outfile)
{
	unsigned int size = TXTFILE_SIZE;
	char *buf = (char *)malloc(size);
	unsigned int sizeout = 224*5*172 + 224 + 172;
	char *bufout = (char *)malloc(sizeout);
	assert(buf);
	assert(bufout);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	printf("sizeout = %d\n", sizeout);
	int ret = depth2txt((int*)buf, bufout);
	
	unsigned int writesize = fwrite(bufout, 1, sizeout, fOut);
	printf("writesize = %d, sizeout = %d\n", writesize, sizeout);
	if(writesize != size) {
		printf("writesize = %d, size = %d\n", writesize, size);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);
	free(bufout);

	return 0;

}

int main_crop2gray(char *infile, char *outfile)
{
	unsigned int size = FILE_SIZE;
	char *buf = (char *)malloc(size);
	assert(buf);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	unsigned int offset = 0;
	size /= 2;
	unsigned int writesize = fwrite(buf+offset, 1, size, fOut);
	printf("writesize = %d, size = %d\n", writesize, size);
	if(writesize != size) {
		printf("writesize = %d, size = %d\n", writesize, size);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);

	return 0;
}

int main_crop2depth(char *infile, char *outfile)
{
	unsigned int size = FILE_SIZE;
	char *buf = (char *)malloc(size);
	assert(buf);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(buf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	unsigned int offset = size/2;
	size /= 2;
	unsigned int writesize = fwrite(buf+offset, 1, size, fOut);
	printf("writesize = %d, size = %d\n", writesize, size);
	if(writesize != size) {
		printf("writesize = %d, size = %d\n", writesize, size);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(buf);

	return 0;
}

void usage(void)
{
	printf("---------------------\n");
	printf("./test idx infile outfile\n");
	printf("idx : \n");
	printf("0 crop: 1.depth to 2.depth\n");
	printf("1 txt : 2.depth to 3.txt\n");
	printf("2 rgb : 2.depth to 3.yuv\n");
	printf("3 yuv : 2.depth to 4.yuv\n");
	printf("4 depthadjust : 2.depth to 4.depthadjust\n");
	printf("5 depthadjust : 2.depth to 4.depthadjust_win\n");
	printf("6 cloudparse  : 2.cloud to 2.cloud.x, 2.cloud.y, 2.cloud.z, 2.cloud.noise\n");
	printf("7 cloud to depth: 1.cloud to 2.depth\n");
	printf("8 depth to yuvrgb: 1.depth to 2.yuvrgb\n");
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
		main_crop2depth(infile, outfile); 	// 1.depth to 2.depth
	break;
	
	case 1: 
		main_txt(infile, outfile);			// 2.depth to 3.txt
	break;

	case 2:
		main_depth2rgb(infile, outfile);	// 2.depth to 3.yuv
	break;

	case 3:
		main_depth2yuv(infile, outfile);	// 2.depth to 4.yuv
	break;
	
	case 4:
		main_depthadjust(infile, outfile);	// 2.depth to 4.yuv
	break;
	
	case 5:
		main_depthadjust_win(infile, outfile);	// 2.depth to 4.yuv
	break;
	
	case 6:
		main_parsecloud(infile, outfile); 	// 1.depth to 2.depth
	break;
	
	case 7:
		main_cloud2depth(infile, outfile); 	// 1.depth to 2.depth
	break;
	
	case 8:
		main_depth2yuvrgb(infile, outfile);	// 2.depth to 4.yuv
	break;
	
	case 9:
		main_crop2gray(infile, outfile); 	// 1.depth to 2.depth
	break;
	
	
	}
	return 0;
}



