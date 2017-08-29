#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include "uvc_gadget_main.h"
#include "uvc_gadget_func.cpp"

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

static int uvc_gadget_check_cameratype(struct uvc_gadget *gadget)
{
	int changed = 0;
	int type = gadget->in->type;
	int source = gadget->in->source;

	if(gadget->out->image == VIDEO_IMAGE_RAW_MONO || gadget->out->image == VIDEO_IMAGE_RAW_MULTI)
		source = OPEN_CAMERA;
	if(source !=  gadget->in->source) {
		gadget->in->source = source;
		changed = 1;
	}

	if(gadget->freq == UVC_FREQ_MONO)
		type = TANGO_FMT_TYPE_MONOFREQ;
	else if(gadget->freq == UVC_FREQ_MONO_80M)
		type = TANGO_FMT_TYPE_MONOFREQ_80M;
	else if(gadget->freq == UVC_FREQ_MULTI)
		type = TANGO_FMT_TYPE_MULTIFREQ;
	if(type != gadget->in->type) {
		gadget->in->type = type;
		changed = 1;
	}
	return changed;
}

#define INDEX_SIZE (7)
unsigned int gIndex[INDEX_SIZE] = {0};

static unsigned int depthdata_adjust(int *pDepth, int *pDepth_a)
{
	unsigned int i;
	int *in = pDepth;
	int *out = pDepth_a;
	int *idx = pDepth_a + (IMG_DEPTH_YUV_W*IMG_DEPTH_YUV_H);

	for (i = 0; i < IMG_DEPTH_YUV_W * IMG_DEPTH_YUV_H; i++) {
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

int calc_win(int *pDepth, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
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
	int val;
	int sum = 0;
	int cnt = 0;
	int *in = pDepth;
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
			sum += val;
		}
	}
	return sum/cnt;
}

int depthdata_adjust_win(int *pDepth)
{
	unsigned int i,j;
	int *in = pDepth;
	int val;
	unsigned int w = IMG_DEPTH_YUV_W;
	unsigned int h = IMG_DEPTH_YUV_H;

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

static unsigned int depth2yuv(struct uvc_gadget *gadget, int *pDepth, char *pYuv)
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

	int width  = IMG_DEPTH_YUV_W;
	int height = IMG_DEPTH_YUV_H;
	unsigned int size = width * height;

	unsigned int i,j;
	int averageDis = 0;
	int sampleNum = 0;
	int line, column;
	int distance;

	char *rgb;
	char *ptry,*ptru,*ptrv;
	unsigned char r,g,b;
	unsigned char y,u,v;
	unsigned int yuv_offset = 0;
	int color;
	int changeColor;

	ptry = ptru = ptrv = (char *)pYuv;
	memset(gIndex, 0, INDEX_SIZE*sizeof(unsigned int));

	for (i = 0; i < size; i++) {
		distance = pDepth[i];
		line = i / (IMG_DEPTH_YUV_W);
		column = i % IMG_DEPTH_YUV_W;
		
		if (line >= CATCH_TOP && line < CATCH_BOTTOM) {
			if (column >= CATCH_LEFT && column < CATCH_RIGHT) {
				if (distance <= 0) {
					distance = 0;
				} else {
					sampleNum++;
				}
				averageDis += distance;
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

static unsigned int uvc_gadget_getdata_tango_depth_denoise(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	int *pDepth = (int *)gadget->depthbuf;
	int *pDepth_a = (int *)gadget->depthbuf_adjust;
	unsigned int size = gadget->depthsize;

	ret = depthdata_adjust(pDepth, pDepth_a);
	depthdata_adjust_win(pDepth_a);
	if(ret < 0) {
		ERR("can not denoise depth video in data\n");
		return 0;
	}

	*pOut = gadget->depthbuf_adjust;
	return size;
}

static unsigned int depth2yuv_rgb(struct uvc_gadget *gadget, int *pDepth, char *pYuv)
{
	unsigned int width  = IMG_DEPTH_YUV_W;
	unsigned int height = IMG_DEPTH_YUV_H;
	unsigned int size = width * height * 4;
	unsigned int i;
	char *src, *dst;

	src = (char *)pDepth;
	dst = (char *)pYuv;
	for (i = 0; i < size; i += 4) {
		*dst++ = src[i+0];
		*dst++ = 116;//0x80;
		*dst++ = src[i+1];
		*dst++ = 140;//0x80;
	}
	for(i = 0; i < width*4; i += 4) {
		*dst++ = 0;
		*dst++ = 116;//0x80;
		*dst++ = 0;
		*dst++ = 140;//0x80;
	}
	return (size+width*4);
}

int test(struct uvc_gadget *gadget)
{
	FILE *fp = fopen("/data/local/tmp/file_1483232327_699719.depth", "rb");
	if(fp != NULL) {
		fread((char *)gadget->depthbuf_adjust, 1, 224*172*4, fp);
		fclose(fp);
	}
	return 0;
}

static unsigned int uvc_gadget_getdata_tango_depth_rgb(struct uvc_gadget *gadget, char **pOut)
{
	//test(gadget);
	int *pDepth_a = (int *)gadget->depthbuf_adjust;
	char *pData = gadget->databuf;
	unsigned int size = depth2yuv_rgb(gadget,  pDepth_a, pData);
	*pOut = gadget->databuf;
	return size;
}

static unsigned int uvc_gadget_getdata_tango_depth_yuv(struct uvc_gadget *gadget, char **pOut)
{
	int *pDepth_a = (int *)gadget->depthbuf_adjust;
	char *pData = gadget->databuf;
	unsigned int size = gadget->depthsize/2;
	int distance;

	distance = depth2yuv(gadget, pDepth_a, pData);
	depth2yuv_distance(gadget, pData, distance, 0);

	if(distance < 0) {
		ERR("can not fetch depth yuv video in data\n");
		return 0;
	}

	*pOut = gadget->databuf;
	return size;
}


static unsigned int uvc_gadget_getdata_raw(struct uvc_gadget *gadget, char **pOut)
{
	unsigned int ret;
	unsigned int picsize = gadget->out->width * gadget->out->height *2;
	char *pData = gadget->databuf;
	ret = camera_getbuf(pData, picsize);
	if(ret != picsize) {
		ERR("get buf fail: ret = %d, picsize = %d, w=%d, h = %d\n", ret, picsize, gadget->out->width, gadget->out->height);
		return 0;
	}
	*pOut = gadget->databuf;
	return ret;
}


static unsigned int uvc_gadget_getdata_tango(struct uvc_gadget *gadget)
{
	int ret;
	int *pDepth = (int *)gadget->depthbuf;
	int *pGray = (int *)gadget->graybuf;
	struct PointCloud *pCloud = (struct PointCloud *)gadget->databuf;
	int exposure = gadget->exposure;
	ret = tango_fetchdepthdata(pGray, pDepth, pCloud, exposure, 1);
	if(ret < 0) {
		ERR("can not fetch tango data\n");
	}
	return ret;
}


static unsigned int uvc_gadget_getdata_tango_gray(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	unsigned int size = gadget->graysize;
	ret = uvc_gadget_getdata_tango(gadget);
	if(ret < 0) {
		ERR("can not fetch gray video in data\n");
		return 0;
	}

	*pOut = gadget->graybuf;
	return size;
}

static unsigned int uvc_gadget_getdata_tango_depth(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	unsigned int size = gadget->graysize;
	ret = uvc_gadget_getdata_tango(gadget);
	if(ret < 0) {
		ERR("can not fetch depth video in data\n");
		return 0;
	}

	*pOut = gadget->depthbuf;
	size = uvc_gadget_getdata_tango_depth_denoise(gadget, pOut);
	return size;
}

static unsigned int uvc_gadget_getdata_tango_cloud(struct uvc_gadget *gadget, char **pOut)
{
	int ret;
	unsigned int size = 224 * 172 * sizeof(struct PointCloud);
	ret = uvc_gadget_getdata_tango(gadget);
	if(ret < 0) {
		ERR("can not fetch cloud video in data\n");
		return 0;
	}
	*pOut = gadget->databuf;
	return size;
}

static unsigned int uvc_gadget_getdata(struct uvc_gadget *gadget, char **pOut)
{
	unsigned int bufsize = 0;
	int getframe = 1;

	if(gadget && gadget->out) {
		if(gadget->out->image == VIDEO_IMAGE_DEPTH 
		   || gadget->out->image == VIDEO_IMAGE_GRAY_DEPTH
		   || gadget->out->image == VIDEO_IMAGE_DEPTH_RGB) { //26fps
			//getframe = 2;
			do{
				bufsize = uvc_gadget_getdata_tango_depth(gadget, pOut);
				uvc_gadget_frame_add();
			}while(--getframe);
			
			if(gadget->out->image == VIDEO_IMAGE_DEPTH_RGB) {
				bufsize = uvc_gadget_getdata_tango_depth_rgb(gadget, pOut);
			}
		}
		else if(gadget->out->image == VIDEO_IMAGE_DEPTH_YUV) { //26fps
			//getframe = 2;
			do{
				bufsize = uvc_gadget_getdata_tango_depth(gadget, pOut);
				uvc_gadget_frame_add();
			}while(--getframe);

			if(gadget->out->image == VIDEO_IMAGE_DEPTH_YUV) {
				bufsize = uvc_gadget_getdata_tango_depth_yuv(gadget, pOut);
			}
		}
		else if(gadget->out->image == VIDEO_IMAGE_CLOUD) {//13fps
			//getframe = 4;
			do{
				bufsize = uvc_gadget_getdata_tango_cloud(gadget, pOut);
				uvc_gadget_frame_add();
			}while(--getframe);
		}
		else {
			if(gadget->out->image == VIDEO_IMAGE_RAW_MULTI)//11fps
				getframe = 4;
			else if(gadget->out->image == VIDEO_IMAGE_RAW_MONO)//21fps
				getframe = 3;

			do{
				bufsize = uvc_gadget_getdata_raw(gadget, pOut);
				uvc_gadget_frame_add();
			}while(--getframe);
		}

		uvc_gadget_savefile_check(gadget);

	}
	else {
		ERR("gadget is NULL\n");
		return 0;
	}
	return bufsize;
}

static void uvc_gadget_sighandler(int signo)
{
	printf("[%s]signo =%d \n", __func__, signo);
	is_exit = 1;
}
static void uvc_gadget_sighandler_usr1(int signo)
{
	printf("[%s]signo =%d \n", __func__, signo);
	uvc_gadget_savefile_flag(1);
}


static void uvc_gadget_usage(const char *name)
{
	ERR("Usage: %s [options]\n", name);
	ERR("Available options are\n");
	ERR(" -o		Output Video device\n");
	ERR(" -h		Print this help screen and exit\n");
}

static void uvc_gadget_uninit(struct uvc_gadget *gadget)
{
	delete gadget->graybuf;
	delete gadget->depthbuf;
	delete gadget->depthbuf_adjust;
	delete gadget->databuf;
	uvc_gadget_close(gadget);
	return;
}

static struct uvc_gadget * uvc_gadget_init(const char *dev)
{
	unsigned int size;
	struct uvc_gadget *gadget = uvc_gadget_open(dev);
	if(gadget == NULL) {
		ERR("uvc_gadget_open fail\n");
		return NULL;
	}
	
	size = 224*172*sizeof(int);
	gadget->graybuf = new char[size];
	gadget->depthbuf = new char[size];
	gadget->depthbuf_adjust = new char[size*2];
	gadget->graysize = size;
	gadget->depthsize = size;
	size = 224*173*9*2;
	gadget->databuf = new char[size];
	gadget->datasize = size;

	gadget->in->source = OPEN_TANGO;
	gadget->in->source = TANGO_FMT_TYPE_MONOFREQ;
	gadget->freq = UVC_FREQ_MONO;  //60M
	gadget->framerate  = 15;
	gadget->exposure  = 200;
	gadget->exposure_max  = 2000;
	gadget->restart = 0;
	return gadget;
}

static int uvc_gadget_get_exposure(void)
{
	unsigned int cs = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;
	int exposure;
	struct uvc_control_st* ptr = uvc_gadget_videoc_get_it(cs);
	if(ptr == NULL) {
		ERR("get videoc it control fail: cs=%d\n", cs);
		return -1;
	}

	exposure = (int)ptr->cur;
	return exposure;
}

static int uvc_gadget_set_exposure(uvc_gadget *gadget, int exposure)
{
	int ret;
	unsigned int cs = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;
	struct uvc_control_st* ptr = uvc_gadget_videoc_get_it(cs);
	if(ptr == NULL) {
		ERR("get videoc it control fail: cs=%d\n", cs);
		return -1;
	}

	if(exposure >= gadget->exposure_max) {
		ERR("exposure[%d] > gadget->exposure_max[%d], set exposure = %d\n", exposure, gadget->exposure_max, gadget->exposure_max);
		exposure = gadget->exposure_max-1;
	}
	
	ret = camera_set_exposure(exposure);
	if(!ret) {
		gadget->exposure = exposure;
		ptr->cur = (unsigned int)exposure;
	}
	else {
		ERR("set exposure[%d] fail, ret = %d\n", exposure, ret);
		exposure = gadget->exposure_max>>2;
		ret = camera_get_exposure(&exposure);
		if(ret)
			ERR("get exposure fail, ret = %d, set default exposure = %d\n", ret, exposure);
		gadget->exposure = exposure;
		ptr->cur = (unsigned int)exposure;
	}
	return ret;
}

static void uvc_gadget_exposure_max_update(uvc_gadget *gadget)
{
	int exposure_range[UVC_FREQ_END][2] = {
		{ 30,   15},
		{540,  950},
		{790, 1540},
		{340,  730},
	};
	int idx = gadget->framerate == 30 ? 0 : 1;
	
	if(gadget->freq >= UVC_FREQ_END) {
		ERR("gadget->freq[%d] >= UVC_FREQ_END[%d]\n", gadget->freq, UVC_FREQ_END);
		return;
	}

	gadget->exposure_max = exposure_range[gadget->freq][idx];
	if(gadget->restart) 
		return;

	DBG("--------exposure = %d, max = %d----------\n", gadget->exposure, gadget->exposure_max);
	if(gadget->exposure > gadget->exposure_max) {
		uvc_gadget_set_exposure(gadget, gadget->exposure_max);
	}
	return;
}

static void uvc_gadget_process_event_control_it(uvc_gadget *gadget, unsigned int cs)
{
	int ret;
	struct uvc_control_st* ptr = uvc_gadget_videoc_get_it(cs);
	if(ptr == NULL) {
		ERR("get videoc it control fail: cs=%d\n", cs);
		return;
	}
	switch(cs) {
	case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL: {
		int exposure = uvc_gadget_get_exposure();
		DBG("--------cur=%d, exposure = %d----------\n", ptr->cur, gadget->exposure);
		if(exposure != gadget->exposure){
			uvc_gadget_set_exposure(gadget, exposure);
		}
	}break;

	case UVC_CT_AE_MODE_CONTROL:
	break;
	
	default:
		ERR("unknown video it control: cs=%d\n", cs);
	}
	return;
}

static void uvc_gadget_process_event_control_pu(uvc_gadget *gadget, unsigned int cs)
{
	struct uvc_control_st* ptr = uvc_gadget_videoc_get_pu(cs);
	if(ptr == NULL) {
		ERR("get videoc pu control fail: cs=%d\n", cs);
		return;
	}
	switch(cs) {
	case UVC_PU_BRIGHTNESS_CONTROL: {
		int freq = ptr->cur;
		if(freq != gadget->freq) {
			DBG("--------cur=%x, freq=%d: [1-3]----------\n", ptr->cur, gadget->freq);
			gadget->freq = freq;
			gadget->restart = 1;
			uvc_gadget_exposure_max_update(gadget);
			gadget->freq = ptr->cur;
		}


	}break;

	case UVC_PU_CONTRAST_CONTROL: {
		int exposure = ptr->cur*5;
		if(exposure > 0) {
			DBG("--------cur=%d, exposure = %d----------\n", exposure, gadget->exposure);
			if(exposure != gadget->exposure){
				uvc_gadget_set_exposure(gadget, exposure);
			}
		}
	}break;

	case UVC_PU_HUE_CONTROL: {
		int spectre = ptr->cur;
		DBG("--------ptr->cur = %d, gadget->spectre = %d--------\n", ptr->cur, gadget->spectre);
		if(spectre != gadget->spectre) {
			gadget->spectre = spectre;
			if(2 == spectre)
				spectre_unpack(gadget, 0);
		}
	}break;
	
	case UVC_PU_SATURATION_CONTROL: {
		int eeprom_calib = ptr->cur;
		DBG("--------ptr->cur = %d, gadget->eeprom_calib = %d--------\n", ptr->cur, gadget->eeprom_calib);
		if(eeprom_calib != gadget->eeprom_calib) {
			gadget->eeprom_calib = eeprom_calib;
			if(2 == eeprom_calib)
				eeprom_calib_func(gadget);
		}
	}break;
	
	default:
		ERR("unknown video pu control: cs=%d\n", cs);
	}
	return;
}

static void uvc_gadget_process_event_control(uvc_gadget *gadget)
{
	unsigned int idx = (gadget->videoc_index>>8) & 0xff;
	unsigned int cs = (gadget->videoc_value>>8) & 0xff;
	switch(idx) {
	case 1: //IT
		uvc_gadget_process_event_control_it(gadget, cs);
	break;

	case 2: //PU
		uvc_gadget_process_event_control_pu(gadget, cs);
	break;

	default:
		ERR("unknown video control: idx=%d, cmd=%d\n", idx, cs);
	}
	return;
}

static void uvc_gadget_process_event_stream(uvc_gadget *gadget)
{
	int ret;
	struct uvc_streaming_control *target;
	int framerate;
	switch (gadget->control) {
	case UVC_VS_COMMIT_CONTROL:
		//DBG("setting commit control\n");
		target = &gadget->commit;
		break;
	default:
		ERR("setting unknown control, gadget->control = %d\n", gadget->control);
	case UVC_VS_PROBE_CONTROL:
		return;
	}

	framerate = 10000000L/target->dwFrameInterval;
	DBG("-------framerate = %d, gadget->framerate = %d----------------\n", framerate, gadget->framerate);
	if(framerate != 15 && framerate != 30)
		framerate = gadget->framerate;

	if(framerate != gadget->framerate) {
		ret = camera_set_framerate(framerate);
		if(!ret) {
			gadget->framerate = framerate;
			uvc_gadget_exposure_max_update(gadget);
		}
	}
	return;
}

static void uvc_gadget_process_event(uvc_gadget *gadget, video_event_st video_state)
{
	switch(video_state) {
	case VIDEO_EVENT_DATA_CONTROL: 
		uvc_gadget_process_event_control(gadget);
	break;

	case VIDEO_EVENT_DATA_STREAM:
		uvc_gadget_process_event_stream(gadget);
	break;
	
	case VIDEO_EVENT_STREAM_ON:
	case VIDEO_EVENT_STREAM_OFF:
	case VIDEO_EVENT_STATE_OK:
	break;

	case VIDEO_EVENT_STATE_ERR:
	default:
	break;
	}

	return;
}

static void uvc_gadget_camera_close(uvc_gadget *gadget, int destroy)
{
	if(gadget->in->source == OPEN_TANGO) {
		tango_close();
		if(destroy)
			tango_destroy();
	}
	else {
		camera_close();
	}
	gadget->in->opened = 0;
	return;
}

static void uvc_gadget_reset(uvc_gadget *gadget)
{
	unsigned int size = gadget->depthsize  * 2;
	memset(gadget->depthbuf_adjust, 0, size);
	return;
}

static int uvc_gadget_camera_open(uvc_gadget *gadget)
{
	int ret;
	enum FMT_TYPE fmttype; // FMT_TYPE_MONOFREQ; //FMT_TYPE_MULTIFREQ;
	int type = gadget->in->type;
	int source = gadget->in->source;

	gadget->restart = 0;

	ERR("-------source = %d, type = %d, wxh = %dx%d------\n", source, type, gadget->out->width, gadget->out->height);fflush(stdout);
	if(source == OPEN_TANGO) {
		ret = tango_open(type);
		if(ret < 0) {
			ERR("open tango : /dev/video0 fail\n");
			return -1;
		}
	}
	else {
		if(type == TANGO_FMT_TYPE_MONOFREQ)
			fmttype = FMT_TYPE_MONOFREQ;
		else if(type == TANGO_FMT_TYPE_MONOFREQ_80M)
			fmttype = FMT_TYPE_MONOFREQ_80M;
		else
			fmttype = FMT_TYPE_MULTIFREQ;

		ret = camera_open(CAMERA_DEVICE, fmttype);
		if(ret < 0) {
			ERR("open camera : /dev/video0 fail\n");
			return -1;
		}
	}

	gadget->in->opened = 1;
	camera_set_framerate(gadget->framerate);
	uvc_gadget_set_exposure(gadget, gadget->exposure);
	uvc_gadget_exposure_max_update(gadget);

	uvc_gadget_reset(gadget);
	ERR("open camera OK\n");
	return 0;
}

int main(int argc, char *argv[])
{
	struct uvc_gadget *gadget = NULL;
	char *outdev = DEFAULT_OUTPUT_DEVICE;
	fd_set fds;
	fd_set wfds;
	fd_set efds;
	struct timeval tv;
	int opt;
	int ret;
	video_event_st video_state;
	unsigned int  size = 0;
	char *pBuf = NULL;

	while ((opt = getopt(argc, argv, "o:h")) != -1) {
		switch (opt) {
		case 'o':
			outdev = optarg;
			break;
		case 'h':
			uvc_gadget_usage(argv[0]);
			return EXIT_SUCCESS;
		default:
			ERR("Invalid option '-%c'\n", opt);
			uvc_gadget_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	signal(SIGINT, uvc_gadget_sighandler);
	signal(SIGTERM, uvc_gadget_sighandler);
	signal(SIGUSR1, uvc_gadget_sighandler_usr1);
	gadget = uvc_gadget_init(outdev);
	if(gadget == NULL) {
		ERR("\n");
		return EXIT_FAILURE;
	}

	uvc_gadget_check_cameratype(gadget);
	ret = uvc_gadget_camera_open(gadget);
	if(ret < 0) {
		ERR("open /dev/video0 fail\n");
		goto main_fail;
	}
	
	uvc_gadget_frame_init();
	ret = uvc_thread_framerate_create(NULL);
	if(ret < 0) {
		goto main_fail2;
		
	}

	FD_ZERO(&fds);
	FD_SET(gadget->out->fd, &fds);
	while (!is_exit) {
		FD_ZERO(&fds);
		FD_SET(gadget->out->fd, &fds);
		wfds = fds;
		efds = fds;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = select(gadget->out->fd + 1, NULL, &wfds, &efds, &tv);
		if (ret < 0) {
			ERR("err: %s\n", strerror(errno));
			if (errno == EINTR)
				continue;
			else {
				ERR("select");
				exit(EXIT_FAILURE);
			}
		}
		
		if (FD_ISSET(gadget->out->fd, &efds)) {
			video_state = uvc_gadget_events_process(gadget);
			uvc_gadget_process_event(gadget, video_state);
			if(gadget->restart) {
				video_output_streamon = 0;
				uvc_gadget_frame_init();
				video_state = VIDEO_EVENT_STREAM_ON;
			}
			if(video_state == VIDEO_EVENT_STREAM_ON) {
				if(uvc_gadget_check_cameratype(gadget) == 1 || gadget->restart) {
					if(gadget->in->opened == 1) {
						uvc_gadget_camera_close(gadget, 0);
						sleep(2);
					}
					
					ret = uvc_gadget_camera_open(gadget);
					if(ret < 0) {
						ERR("open /dev/video0 fail\n");
						goto main_exit;
					}
					sleep(2);
				}
				video_output_streamon = 1;
				printf("stream on start....\n");fflush(stdout);
			}
			else if(video_state == VIDEO_EVENT_STREAM_OFF) {
				video_output_streamon = 0;
				uvc_gadget_frame_init();
				printf("stream off end......\n");fflush(stdout);
			}
		}
		
		if (video_output_streamon && FD_ISSET(gadget->out->fd, &wfds)) {		
			size = uvc_gadget_getdata(gadget, &pBuf);
			if(size == 0) {
				ERR("get data fail, size = 0\n");
				continue;
			}
			if(0){
				char *hyptr = pBuf;
				int i,j;
				int w=224;
				int h=173;
				for(i=0; i<h; i++)
					for(j=0; j<w; j++) {
						*hyptr++ = i;
						*hyptr++ = 0x80;
						*hyptr++ = i;
						*hyptr++ = 0x80;
					}

			}
			
			if(gadget->out->image == VIDEO_IMAGE_GRAY_DEPTH) { 
				char *pBuf2 = gadget->graybuf;
				unsigned int size2 = gadget->graysize;
				uvc_gadget_video_process2(gadget, pBuf2, size2, pBuf, size);	
			}
			else {
				uvc_gadget_video_process(gadget, pBuf, size);
			}
		}
	}

main_exit:
	uvc_thread_framerate_join();
	
main_fail2:
	if(gadget->in->opened == 1) {
		uvc_gadget_camera_close(gadget, 1);
	}

main_fail:
	uvc_gadget_uninit(gadget);
	return EXIT_SUCCESS;
}
