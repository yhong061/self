#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char uint_8;
typedef unsigned short int uint_16;
typedef uint_16 iPIXEL_t;
typedef uint_16 oPIXEL_t;

#define IMAGE_W	(320)
#define IMAGE_H	(240)

#define IMAGE_480_W	(640)
#define IMAGE_480_H	(480)

#define IMAGE_AMP_W	IMAGE_480_W
#define IMAGE_AMP_H	(IMAGE_480_H)

int  DISTANCE_STEP_INT = 100;

struct uvc_gadget{
	unsigned int graysize;
	char *graybuf;
	
	unsigned int depthsize;
	char *depthbuf;
	char *depthbuf_adjust;

	unsigned int datasize;
	char *databuf;

	FILE *fIn;
	FILE *fOut;

};

#define STEP_W (2)
#define STEP_H (2)
#define STEP_OFFSET (16)


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



#define INDEX_SIZE (7)
unsigned int gIndex[INDEX_SIZE] = {0};

int gWidth = IMAGE_W;
int gHeight = IMAGE_H;

int getWidht(void)
{
	return gWidth;
}

int getHeight(void)
{
	return gHeight;
}


static void uvc_gadget_uninit(struct uvc_gadget *gadget)
{
	free(gadget->depthbuf);
	free(gadget->depthbuf_adjust);
	free(gadget->databuf);
	free(gadget->graybuf);
	if(gadget->fIn) {
		fclose(gadget->fIn);
		gadget->fIn = NULL;
	}
	if(gadget->fOut) {
		fclose(gadget->fOut);
		gadget->fOut = NULL;
	}
	return;
}

int uvc_gadget_init(struct uvc_gadget *gadget, char *infile, char *outfile)
{
	unsigned int size;
	size = getWidht()*getHeight()*sizeof(oPIXEL_t);
	gadget->graybuf = malloc(size);
	gadget->depthbuf =(char *)malloc( size);
	gadget->depthbuf_adjust = (char *)malloc( size * 2);
	gadget->graysize = size;
	gadget->depthsize = size;
	gadget->databuf = (char *)malloc( size);;
	gadget->datasize = size;

	gadget->fIn = fopen(infile, "rb");
	if(gadget->fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	gadget->fOut = fopen(outfile, "wb");
	if(gadget->fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end0;
	}


	return 0;

end0: 
	uvc_gadget_uninit(gadget);
	return -1;
	
}

static unsigned int depth2yuv(struct uvc_gadget *gadget, char *pDepth, char *pYuv)
{
	//   color       alpha          B         G         R       distance
	//   black       255            0         0         0       // 0
	//   red         255            0         0       255       // 0 ---- 30
	//   yellow      255            0       255       255       // 30 ---- 60
	//   green       255            0       255         0       // 60 ---- 90
	//   cyan        255          255       255         0       // 90 ---- 120
	//   blue        255          255         0         0       // 120 ---- 150
	//   white       255          255       255       255       // > 150
	//

	int width  = getWidht();
	int height = getHeight();
	unsigned int size = width * height;

	unsigned int i,j;
	int averageDis = 0;
	int sampleNum = 0;
	int line, column;
	iPIXEL_t *pIn = (iPIXEL_t *)pDepth;
	iPIXEL_t distance;

	char *rgb;
	char *ptry,*ptru,*ptrv;
	unsigned char r,g,b;
	unsigned char y,u,v;
	unsigned int yuv_offset = 0;
	int color;
	int changeColor;
	int CATCH_LEFT = getWidht()/2-10;
	int CATCH_RIGHT = CATCH_LEFT + 20;
	int CATCH_TOP = getHeight()/2-10;
	int CATCH_BOTTOM = CATCH_TOP + 20;

	ptry = ptru = ptrv = (char *)pYuv;
	memset(gIndex, 0, INDEX_SIZE*sizeof(unsigned int));

	for (i = 0; i < size; i++) {
		distance = pIn[i];
		line = i / (getWidht());
		column = i % getWidht();
		
		if (line >= CATCH_TOP && line < CATCH_BOTTOM) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;
				} else {
					sampleNum++;
				}
				averageDis += (int)distance;
			}
		}

		changeColor = 0;
		if (distance == 0) {
			color = 0xFF000000;
			gIndex[0]++;
		} else if (distance > 0 && distance <= DISTANCE_STEP_INT) { //color change from black to red, red change from 0 to 255
			color       = 0xFF000000;
			changeColor = (int) (distance * 255 / DISTANCE_STEP_INT);
			color      += changeColor;
			gIndex[1]++;
		} else if (distance > DISTANCE_STEP_INT && distance <= 2 * DISTANCE_STEP_INT) { //color change from read to yellow, green change from 0 to 255
			distance   -= DISTANCE_STEP_INT;
			color       = 0xFF0000FF;
			changeColor = ((int) ((distance) * 255 / DISTANCE_STEP_INT)) << 8;
			color      += changeColor;
			gIndex[2]++;
		} else if (distance > 2 * DISTANCE_STEP_INT && distance <= 3 * DISTANCE_STEP_INT) { //color change from yellow to green, red change form 255 to 0
			distance   -= 2 * DISTANCE_STEP_INT;
			color       = 0xFF00FFFF;
			changeColor = (int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT);
			color      -= changeColor;
			gIndex[3]++;
		} else if (distance > 3 * DISTANCE_STEP_INT && distance <= 4 * DISTANCE_STEP_INT) { //color change from green to cyan, blue change from 0 to 255
			distance   -= 3 * DISTANCE_STEP_INT;
			color       = 0xFF00FF00;
			changeColor = ((int) (distance * 255 / DISTANCE_STEP_INT)) << 16;
			color      += changeColor;
			gIndex[4]++;
		} else if (distance > 4 * DISTANCE_STEP_INT && distance <= 5 * DISTANCE_STEP_INT) { //color change from cyan to blue, green change form 255 to 0
			distance   -= 4 * DISTANCE_STEP_INT;
			color       = 0xFFFFFF00;
			changeColor = ((int) ((DISTANCE_STEP_INT - distance) * 255 / DISTANCE_STEP_INT)) << 8;
			color      -= changeColor;
			gIndex[5]++;
		} else {
			color       = 0xffffffff;
			gIndex[6]++;
		}

		/*
		Y =  0.257R + 0.504G + 0.098B + 16
		U = -0.148R - 0.291G + 0.439B + 128
		V =  0.439R - 0.368G - 0.071B + 128
		*/

		if (line >= CATCH_TOP && line < CATCH_BOTTOM) 
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) 
				color       = 0xffffffff;  //white

		rgb = (char *)&color;

		r = rgb[0];
		g = rgb[1];
		b = rgb[2];

		y = (unsigned char)( 0.257*r + 0.504*g + 0.098*b +16);
		u = (unsigned char)(-0.148*r - 0.291*g + 0.439*b +128);
		v = (unsigned char)( 0.439*r - 0.368*g - 0.071*b +128);

		if(i%2 == 0) {
			ptry[yuv_offset++] = y;
			ptru[yuv_offset++] = u;
		}
		else {
			ptry[yuv_offset++] = y;
			ptrv[yuv_offset++] = v;
		}
	}
	if(0){
		unsigned int hyi;
		char hybuf[128];
		char hytmp[32];
		hybuf[0] = '\0';
		hytmp[0] = '\0';
		sprintf(hybuf, "[%d] ", __LINE__);
		for(hyi=0; hyi<INDEX_SIZE; hyi++) {
			sprintf(hytmp, " [%04d]%05d", hyi*DISTANCE_STEP_INT, gIndex[hyi]);
			strcat(hybuf, hytmp);
		}
		printf("%s\n", hybuf);
	}
	if (sampleNum > 0) {
		averageDis /= sampleNum;
	} else {
		averageDis = 0;
	}
	return averageDis;
}


static unsigned int depth2yuv_distance_xy(struct uvc_gadget *gadget, char *pYuv, int color, unsigned int xoff, unsigned int yoff)
{
	int w = getWidht();
	int h = getHeight();
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
	return 0;
}

static unsigned int depth2yuv_distance(struct uvc_gadget *gadget, char *pYuv, int distance, int idx)
{
	int num, rest;
	int i;

	if(distance < 0)
		return 0;

	num = distance%10;
	rest = distance/10;
	if(rest > 0) {
		idx += depth2yuv_distance(gadget, pYuv, rest, idx);
	}

	depth2yuv_distance_val(gadget, pYuv, num, idx);
	return idx+1;
}


static unsigned int uvc_gadget_getdata_tango_depth_yuv(struct uvc_gadget *gadget, char **pOut)
{
	char *pDepth_a = (char *)gadget->depthbuf_adjust;
	char *pData = gadget->databuf;
	unsigned int size = gadget->depthsize;
	int distance = 0;

	distance = depth2yuv(gadget, pDepth_a, pData);
	depth2yuv_distance(gadget, pData, distance, 0);

	if(distance < 0) {
		printf("can not fetch depth yuv video in data\n");
		return 0;
	}

	*pOut = gadget->databuf;
	return size;
}

static unsigned int uvc_gadget_getdata_tango(struct uvc_gadget *gadget)
{
	int ret;
	int *pDepth = (int *)gadget->depthbuf;
	int *pGray = (int *)gadget->graybuf;
	unsigned int insize = gadget->graysize;

	unsigned int readsize = fread(pDepth, 1, insize, gadget->fIn);
	printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	}

	return ret;
}

static unsigned int depthdata_adjust(char *pDepth, char *pDepth_a)
{
	unsigned int i;
	iPIXEL_t *in = (iPIXEL_t *)pDepth;
	iPIXEL_t *out = (iPIXEL_t *)pDepth_a;
	iPIXEL_t *idx = (iPIXEL_t *)(pDepth_a + (getWidht()*getHeight()));

	for (i = 0; i < getWidht() * getHeight(); i++) {
		if(*in != 0) {  //*in != 0
			*out = *in;
		}
		else {		//*in == 0
			*out = *idx;
		}
		*idx++ = *in++;
		out++;
	}
	return 0;
}

iPIXEL_t calc_win(char *pDepth, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	/*
	matrix = [x,x,x]
		 [x,0,x]
		 [x,x,x]
	in[x,y] = matrix[1][1];
	in[x1,y1] = matrix[0][0]
	in[x2,y3] = matrix[2][2]

	for [x1->x2] -> [y1->y2]
		if in[i][j] == 0 , give up to change in[x][y]
		else sum += in[i][j]  new in[x][y] = sum/8
	*/
	unsigned int i,j;
	iPIXEL_t val;
	int sum = 0;
	int cnt = 0;
	iPIXEL_t *in = (iPIXEL_t *)pDepth;
	int step = 1;
	unsigned int x1,x2,y1,y2;

	if(x == 0)
		x1 = x;
	else
		x1 = x-step;

	if(x == h-1)
		x2 = x;
	else
		x2 = x+step;

	if(y == 0)
		y1 = y;
	else
		y1 = y-step;

	if(y == w-1)
		y2 = y;
	else
		y2 = y+step;

	for(i=x1; i<=x2; i++) {
		for(j=y1; j<=y2; j++){
			if(i == x && j == y)
				continue;

			val = in[i*w +j];
			if(val == 0)
				return 0;
			cnt++;
			sum += (int )val;
		}
	}
	return (iPIXEL_t)(sum/cnt);
}

int depthdata_adjust_win(char *pDepth)
{
	unsigned int i,j;
	iPIXEL_t *in = (iPIXEL_t *)pDepth;
	iPIXEL_t val;
	unsigned int w = getWidht();
	unsigned int h = getHeight();

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			val = in[i*w + j];
			if(val != 0) 
				continue;

			//update in[x] : if in[x] == 0, use val = sum[3][3]/9
			in[i*w +j] = calc_win(pDepth, i, j, w, h);
		}
	}
	return 0;
}


static unsigned int uvc_gadget_getdata_tango_depth_denoise(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	char *pDepth = (char *)gadget->depthbuf;
	char *pDepth_a = (char*)gadget->depthbuf_adjust;
	unsigned int size = gadget->depthsize;

	ret = depthdata_adjust(pDepth, pDepth_a);
	depthdata_adjust_win(pDepth_a);
	if(ret < 0) {
		printf("can not denoise depth video in data\n");
		return 0;
	}

	*pOut = gadget->depthbuf_adjust;
	return size;
}



static unsigned int uvc_gadget_getdata_tango_depth(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	unsigned int size = gadget->graysize;
	ret = uvc_gadget_getdata_tango(gadget);
	if(ret < 0) {
		printf("can not fetch depth video in data\n");
		return 0;
	}

	*pOut = gadget->depthbuf;
	size = uvc_gadget_getdata_tango_depth_denoise(gadget, pOut);
	return size;
}

unsigned int uvc_gadget_getdata(struct uvc_gadget *gadget, char **pOut)
{
	unsigned int bufsize = 0;
	int getframe = 1;

	if(1) {
		if(1) { //26fps
			//getframe = 2;
			do{
				bufsize = uvc_gadget_getdata_tango_depth(gadget, pOut);
			}while(--getframe);

			if(1) {
				bufsize = uvc_gadget_getdata_tango_depth_yuv(gadget, pOut);
			}
		}
	}
	return bufsize;
}

int save_file(struct uvc_gadget *gadget, char *bufout, unsigned int outsize)
{
	unsigned int writesize = fwrite(bufout, 1, outsize, gadget->fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}
	return 0;
}

int main_epc(char *infile, char *outfile)
{
	struct uvc_gadget gadget;
	char *pBuf = NULL;
	unsigned int  size = 0;
	int ret;

	uvc_gadget_init(&gadget, infile, outfile);
	size = uvc_gadget_getdata(&gadget, &pBuf);
	save_file(&gadget, pBuf, size);
	uvc_gadget_uninit(&gadget);

	return 0;
}

int depth2yuv2(char *in_depth, char *out_depth) 
{
	int i;
	int averageDis = 0;
	int sampleNum = 0;
	
	char *rgb;
	char *ptry,*ptru,*ptrv;
	uint_8 r,g,b;
	uint_8 y,u,v;
	unsigned int yuv_offset = 0;
	iPIXEL_t *in = (iPIXEL_t *)in_depth;
	int CATCH_LEFT = getWidht()/2-10;
	int CATCH_RIGHT = CATCH_LEFT + 20;
	int CATCH_TOP = getHeight()/2-10;
	int CATCH_BOTTOM = CATCH_TOP + 20;
	
	ptry = ptru = ptrv = out_depth;
	
	for (i = 0; i < getWidht() * getHeight(); i++) {
		iPIXEL_t distance = in[i];
		int line = i / (getWidht());
		int column = i % getWidht();
		if (0/*line >= CATCH_TOP && line < CATCH_BOTTOM*/) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;        
				} else {          
					sampleNum++;        
				}        
				averageDis += (int)distance;      
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

int main_depth2yuv(char *infile, char *outfile)
{
	unsigned int yuvsize = getWidht() * getHeight() * sizeof(oPIXEL_t);	
	unsigned int depthsize = getWidht() * getHeight() * sizeof(iPIXEL_t);	
	unsigned int outsize = yuvsize;
	unsigned int insize = depthsize;
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
	printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	}

	int ret = depth2yuv2(buf, bufout);
	//int ret = depth2yuv_epc(buf, bufout);
	printf("ret = %d\n", ret);
	
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

int depth2txt(char *out_depth, char *bufout, int w, int h) 
{
	int i,j;
	iPIXEL_t *in = (iPIXEL_t *)out_depth;
	char tmp[32];
	iPIXEL_t distance;

	bufout[0] = '\0';
	tmp[0] = '\0';
	for (i = 0; i < h ; i++) {
		for (j = 0; j < w ; j++) {
			distance = in[i*w +j];
			sprintf(tmp, "%04x ", distance);
			strcat(bufout, tmp);
		}  
		sprintf(tmp, "\n");
		strcat(bufout, tmp);
	}
	return 0;
}

int main_depth2txt(char *infile, char *outfile)
{
	unsigned int size = getWidht()*getHeight()*sizeof(iPIXEL_t);
	char *inbuf = (char *)malloc(size);
	unsigned int sizeout = getWidht()*getHeight()*5 + getWidht() + getHeight();
	char *bufout = (char *)malloc(sizeout);
	assert(inbuf);
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

	unsigned int readsize = fread(inbuf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	int ret = depth2txt(inbuf, bufout, getWidht(), getHeight());
	
	unsigned int writesize = fwrite(bufout, 1, sizeout, fOut);
	printf("writesize = %d, sizeout = %d\n", writesize, sizeout);
	if(writesize != sizeout) {
		printf("writesize = %d, sizeout = %d\n", writesize, sizeout);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(bufout);

	return 0;

}

int gray2rgb(char *iData, char *oData, int w, int h) 
{
	int i,j;
	iPIXEL_t *in = (iPIXEL_t *)iData;
	uint_8 *out = (uint_8 *)oData;
	iPIXEL_t val;


	for (i = 0; i < h ; i++) {
		for (j = 0; j < w ; j++) {
			val = (in[i*w +j]<<4 & 0xff00 ) >> 8;
			*out++ = val;
			*out++ = 0x80;
		}
	}
	return 0;
}

int main_gray2rgb(char *infile, char *outfile)
{
	unsigned int size = getWidht()*getHeight()*sizeof(iPIXEL_t);
	unsigned int sizeout = getWidht()*getHeight()*2;
	char *inbuf = (char *)malloc(size);
	char *bufout = (char *)malloc(sizeout);
	assert(inbuf);
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

	unsigned int readsize = fread(inbuf, 1, size, fIn);
	printf("readsize = %d, size = %d\n", readsize, size);
	if(readsize != size) {
		printf("readsize = %d, size = %d\n", readsize, size);
	}

	int ret = gray2rgb(inbuf, bufout, getWidht(), getHeight());
	
	unsigned int writesize = fwrite(bufout, 1, sizeout, fOut);
	printf("writesize = %d, sizeout = %d\n", writesize, sizeout);
	if(writesize != sizeout) {
		printf("writesize = %d, sizeout = %d\n", writesize, sizeout);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(bufout);

	return 0;

}

iPIXEL_t amplifyimage_mixtogether(char *indepth, unsigned int x, unsigned int y, unsigned int w, unsigned int h, int type)
{
	iPIXEL_t *in = (iPIXEL_t *)indepth;
	int val = 0;

	if(x == w)
		x -= 1;
	if(y == h)
		y -= 1;
	
	if(type == 1) {//x1+x2
		val = (int)(in[y*w + x] + in[y*w + (x+1)])/2;
	}
	else if(type == 2) {//y1+y2
		val = (int)(in[y*w + x] + in[(y+1)*w + x])/2;
	}
	else if(type == 3) {//x1+x2 y1+y2
		val = (int)(in[y*w + x] + in[y*w + (x+1)] + in[(y+1)*w + x] + in[(y+1)*w + (x+1)])/4;
	}
		
	return (iPIXEL_t)val;
}


int amplifyimage(char *indepth, char *outdepth) 
{
	int x,y, x1,y1,xr,yr;
	int w = IMAGE_AMP_W;
	int h = IMAGE_AMP_H;
	int w1 = getWidht();
	int h1 = getHeight();
	iPIXEL_t *in = (iPIXEL_t *)indepth;
	iPIXEL_t *out = (iPIXEL_t *)outdepth;

	for (y = 0; y < h ; y++) {
		y1 = y/2;
		yr = y%2;
		for (x = 0; x < w ; x++) {
			x1 = x/2;
			xr = x%2;
			
			if(yr == 0 && xr == 0) 
				*out = in[y1*w1 + x1];
			else 
				*out = amplifyimage_mixtogether(indepth, x1, y1, w1, h1, (yr<<1)|xr);

			out++;
		}	
	}
	return 0;
}


int main_amplifyimage(char *infile, char *outfile)
{
	unsigned int outsize = IMAGE_AMP_W * IMAGE_AMP_H * sizeof(oPIXEL_t);
	unsigned int insize = getWidht() * getHeight() * sizeof(iPIXEL_t);
	char *inbuf = (char *)malloc(insize);
	char *outbuf = (char *)malloc(outsize);
	assert(inbuf);
	assert(outbuf);

	memset(outbuf, 0, outsize);
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

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	}

	int ret = amplifyimage(inbuf, outbuf);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(outbuf, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(outbuf);

	return 0;

}

int max(int p1, int p2)
{
	return p1 > p2 ? p1 : p2;
}

int min(int p1, int p2)
{
	return p1 > p2 ? p2 : p1;
}

float gA = 1.0;
int gTh = 4;

iPIXEL_t amplifyimage_mixtogether2_x(char *indepth, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	iPIXEL_t *in = (iPIXEL_t *)indepth;
	unsigned int y0;
	int p0,p1,p2,p3;
	int pca, pce, pcm;

	if(x == 0) {
		p0 = (int)in[y*w + (x)];
		p1 = (int)in[y*w + (x)];
		p2 = (int)in[y*w + (x+1)];
		p3 = (int)in[y*w + (x+2)];
	}
	else if(x == w-1) {
		p0 = (int)in[y*w + (x-1)];
		p1 = (int)in[y*w + (x)];
		p2 = (int)in[y*w + (x)];
		p3 = (int)in[y*w + (x)];
	}
	else if(x == w-2) {
		p0 = (int)in[y*w + (x-1)];
		p1 = (int)in[y*w + (x)];
		p2 = (int)in[y*w + (x)];
		p3 = (int)in[y*w + (x+1)];
	}
	else {
		p0 = (int)in[y*w + (x-1)];
		p1 = (int)in[y*w + (x)];
		p2 = (int)in[y*w + (x+1)];
		p3 = (int)in[y*w + (x+2)];
	}
	
	pca = (int)(p1+p2)/2;
	pce = (int)(p1*3/2 - p0/2 + p2*3/2 - p3/2)/2;
	pcm = (int)(pce*gA + pca*(1-gA));
	if(pcm > (max(p1, p2) + gTh))
		pcm = (max(p1, p2) + gTh);
	else if(pcm < (min(p1, p2) - gTh))
		pcm = (min(p1, p2) - gTh);
	
	return (iPIXEL_t)pcm;
}


iPIXEL_t amplifyimage_mixtogether2_y(char *indepth, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	iPIXEL_t *in = (iPIXEL_t *)indepth;
	unsigned int y0;
	int p0,p1,p2,p3;
	int pca, pce, pcm;

	if(y == 0) {
		p0 = (int)in[y*w + x];
		p1 = (int)in[y*w + x];
		p2 = (int)in[(y+1)*w + x];
		p3 = (int)in[(y+2)*w + x];
	}
	else if(y == h-1) {
		p0 = (int)in[(y-1)*w + x];
		p1 = (int)in[y*w + x];
		p2 = (int)in[(y)*w + x];
		p3 = (int)in[(y)*w + x];
	}
	else if(y == h-2) {
		p0 = (int)in[(y-1)*w + x];
		p1 = (int)in[y*w + x];
		p2 = (int)in[(y)*w + x];
		p3 = (int)in[(y+1)*w + x];
	}
	else {
		p0 = (int)in[(y-1)*w + x];
		p1 = (int)in[y*w + x];
		p2 = (int)in[(y+1)*w + x];
		p3 = (int)in[(y+2)*w + x];
	}
	
	pca = (int)(p1+p2)/2;
	pce = (int)(p1*3/2 - p0/2 + p2*3/2 - p3/2)/2;
	pcm = (int)(pce*gA + pca*(1-gA));
	if(pcm > (max(p1, p2) + gTh))
		pcm = (max(p1, p2) + gTh);
	else if(pcm < (min(p1, p2) - gTh))
		pcm = (min(p1, p2) - gTh);
	
	return (iPIXEL_t)pcm;
}


int amplifyimage2(char *indepth, char *outdepth) 
{
	int x,y, x1,y1,xr,yr;
	int w1 = 320;
	int h1 = 240;
	int w;
	int h;
	iPIXEL_t *in;
	iPIXEL_t *out;
	char *tmp = NULL;
	iPIXEL_t pca, pce, pcm;

	w = w1;
	h = h1*2;
	tmp = (char *)malloc(w*h*sizeof(iPIXEL_t *));
	
	in = (iPIXEL_t *)indepth;
	out = (iPIXEL_t *)tmp;
	
	for (y = 0; y < h ; y++) {
		y1 = y/2;
		for (x = 0; x < w ; x++) {
			x1 = x;
			if(y%2 == 0) 
				*out = in[y1*w1 + x1];
			else 
				*out = amplifyimage_mixtogether2_y((char *)in, x1, y1, w1, h1);
			out++;
		}
	}

	w = w1*2;
	h = h1*2;
	
	in = (iPIXEL_t *)tmp;
	out = (iPIXEL_t *)outdepth;
	
	for (y = 0; y < h ; y++) {
		y1 = y;
		for (x = 0; x < w ; x++) {
			x1 = x/2;
			if(x%2 == 0) 
				*out = in[y1*w1 + x1];
			else 
				*out = amplifyimage_mixtogether2_x((char *)in, x1, y1, w1, h1);
			out++;
		}
	}

	return 0;
}

int main_amplifyimage2(char *infile, char *outfile)
{
	unsigned int outsize = IMAGE_AMP_W * IMAGE_AMP_H * sizeof(oPIXEL_t);
	unsigned int insize = getWidht() * getHeight() * sizeof(iPIXEL_t);
	char *inbuf = (char *)malloc(insize);
	char *outbuf = (char *)malloc(outsize);
	assert(inbuf);
	assert(outbuf);

	memset(outbuf, 0, outsize);
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

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, rgbsize = %d\n", readsize, insize);
	}

	int ret = amplifyimage2(inbuf, outbuf);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(outbuf, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(outbuf);

	return 0;

}

void usage(void)
{
	printf("---------------------\n");
	printf("test 0   in.bin out.yuv      //convert to txt file for amplify file\n");
	printf("test 10   in.bin out.yuv  amp    //convert to txt file for amplify file\n");
	printf("test 1/2 in.bin out.yuv      //convert to yuv file\n");
	printf("test 3/4 in.bin out.yuv step //convert to yuv file useing denoise, step default is %d\n", DISTANCE_STEP_INT);
	printf("test 5/6 in.bin out.yuv      //convert to txt file\n");
	printf("test 7/8 in.bin out.rgb      //convert to rgb file\n");
	printf("---------------------\n");
}


int main(int argc, char *argv[])
{

	if(argc<4) {
		usage();
		return 0;
	}

	int idx = atoi(argv[1]);
	char *infile = argv[2];
	char *outfile = argv[3];
	
	switch(idx){
	case 0:
		main_amplifyimage(infile, outfile);
	break;
	
	case 10:
		if(argc > 4)
			gA = atof(argv[4]);
		if(argc > 5)
			gTh = atof(argv[5]);
		printf("gA = %f, gTh = %d\n", gA, gTh);
		main_amplifyimage2(infile, outfile);
	break;

	case 2: 
		gWidth = IMAGE_AMP_W;
		gHeight = IMAGE_AMP_H;
	case 1:
		main_depth2yuv(infile, outfile);	// 2.depth to 4.yuv
	break;	
	
	case 4: 
		gWidth = IMAGE_AMP_W;
		gHeight = IMAGE_AMP_H;
	case 3:
		printf("argc = %d\n", argc);
		if(argc > 4)
			DISTANCE_STEP_INT = atoi(argv[4]);
		else {
			usage();
			return 0;
		}
		main_epc(infile, outfile);
 	break;

	case 6: 
		gWidth = IMAGE_AMP_W;
		gHeight = IMAGE_AMP_H;
	case 5: 
		main_depth2txt(infile, outfile);			// 2.depth to 3.txt
	break;

	case 7: 
		main_gray2rgb(infile, outfile);			// 2.depth to 3.txt
	break;

	default:
	break;	
	}
	return 0;
}
