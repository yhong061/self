#include <stdio.h>
#include <stdlib.h>

#define IMG_DEPTH_YUV_W (224)
#define IMG_DEPTH_YUV_H (172)

struct uvc_gadget {
	int x;
};

unsigned char matrix[10][6][4] = {
	{{1,1,1,0},{1,0,1,0},{1,0,1,0},{1,0,1,0},{1,1,1,0},{0,0,0,0}},  // 0
	{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},  // 1
	{{1,1,1,0},{0,0,1,0},{1,1,1,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},  // 2
	{{1,1,1,0},{0,0,1,0},{1,1,1,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},  // 3
	{{1,0,1,0},{1,0,1,0},{1,1,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}},  // 4
	{{1,1,1,0},{1,0,0,0},{1,1,1,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},  // 5
	{{1,1,1,0},{1,0,0,0},{1,1,1,0},{1,0,1,0},{1,1,1,0},{0,0,0,0}},  // 6
	{{1,1,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}},  // 7
	{{1,1,1,0},{1,0,1,0},{1,1,1,0},{1,0,1,0},{1,1,1,0},{0,0,0,0}},  // 8
	{{1,1,1,0},{1,0,1,0},{1,1,1,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},  // 9
};

#define STEP_W (2)
#define STEP_H (2)
#define STEP_OFFSET (16)

static unsigned int depth2yuv_distance_xy(struct uvc_gadget *gadget, char *pYuv, int color, unsigned int xoff, unsigned int yoff)
{
	int w = IMG_DEPTH_YUV_W;
	int h = IMG_DEPTH_YUV_H;
	int i,j,k;
	char *rgb;
	char *ptry,*ptru,*ptrv;
	unsigned char r,g,b;
	unsigned char y,u,v;
	unsigned int offset = yoff*w+xoff;
	unsigned int yuv_offset;

	ptry = ptru = ptrv = (char *)pYuv;
	for(i=0; i<5; i++) {
		yuv_offset = 2 * (offset + i*w);
		k = 0;
		for(j=0; j<5; j++) {
			rgb = (char *)&color;
			r = rgb[0];
			g = rgb[1];
			b = rgb[2];
			y = (unsigned char)( 0.257*r + 0.504*g + 0.098*b +16);
			u = (unsigned char)(-0.148*r - 0.291*g + 0.439*b +128);
			v = (unsigned char)( 0.439*r - 0.368*g - 0.071*b +128);
			
			if(k%2 == 0) {
				ptry[yuv_offset++] = y;
				ptru[yuv_offset++] = u;
			}
			else {
				ptry[yuv_offset++] = y;
				ptrv[yuv_offset++] = v;
			}
			k++;
		}
	}
	return 0;
}

static unsigned int depth2yuv_distance_val(struct uvc_gadget *gadget, char *pYuv, int num, int idx)
{
	int i,j;
	unsigned int x,y;
	int color; 
	unsigned char *matrix_p = &matrix[num][0][0]; 

printf("\t[%d] idx = %d, num = %d\n", __LINE__, idx, num);
	for(i=0; i<6; i++) {
		for(j=0; j<4; j++) {
			if(*matrix_p == 1) 
				color = 0xffffffff; //white
			else
				color = 0xff000000; //black

			x = STEP_OFFSET + STEP_W*(idx*4+j);
			y = STEP_OFFSET + STEP_H*i;
			depth2yuv_distance_xy(gadget, pYuv, color, x, y);
			matrix_p++;
		}
	}
	return 1;		
}


static unsigned int depth2yuv_distance(struct uvc_gadget *gadget, char *pYuv, int distance, int idx)
{
	int num, rest;
	int i;

	num = distance%10;
	rest = distance/10;
	if(rest > 0) {
		idx += depth2yuv_distance(gadget, pYuv, rest, idx);
printf("[%d] idx = %d, num = %d, rest = %d\n", __LINE__, idx, num, rest);
	}
	
	depth2yuv_distance_val(gadget, pYuv, num, idx);
	return idx+1;

}

static unsigned int rgb2yuv(struct uvc_gadget *gadget, char *pYuv)
{
	int w = IMG_DEPTH_YUV_W;
	int h = IMG_DEPTH_YUV_H;
	int i,j,k;
	char *rgb;
	char *ptry,*ptru,*ptrv;
	unsigned char r,g,b;
	unsigned char y,u,v;
	unsigned int yuv_offset;
	int color = 0xff000000;

	ptry = ptru = ptrv = (char *)pYuv;
	yuv_offset = 0;
	for(i=0; i<h; i++) {
		for(j=0; j<w; j++) {
			rgb = (char *)&color;
			r = rgb[0];
			g = rgb[1];
			b = rgb[2];
			y = (unsigned char)( 0.257*r + 0.504*g + 0.098*b +16);
			u = (unsigned char)(-0.148*r - 0.291*g + 0.439*b +128);
			v = (unsigned char)( 0.439*r - 0.368*g - 0.071*b +128);
			
			if(k%2 == 0) {
				ptry[yuv_offset++] = y;
				ptru[yuv_offset++] = u;
			}
			else {
				ptry[yuv_offset++] = y;
				ptrv[yuv_offset++] = v;
			}
		}
	}
	return 0;
}


int main(int argc, char *argv[])
{
	struct uvc_gadget gadget;
	int w = IMG_DEPTH_YUV_W;
	int h = IMG_DEPTH_YUV_H;
	int size = w*h*2;
	int distance = atoi(argv[1]);

	printf("distance =%d\n", distance);
	
	char *yuv = malloc(size);
	rgb2yuv(&gadget, yuv);
	depth2yuv_distance(&gadget, yuv, distance, 0);

	FILE *fp = fopen("224_172.yuv", "wb");
	if(fp) {
		fwrite(yuv, 1, size, fp);
		fclose(fp);
	}

	return 0;
}

