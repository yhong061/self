#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define DBG(fmt,...) fprintf(stderr, "[%s:%d]"fmt, __func__, __LINE__, ## __VA_ARGS__)

static int gCalib_enable = 0;
static short unsigned int *gCalib_data = NULL;
static unsigned int gCalib_size = 0;
static short unsigned int *gDual_data = NULL;
static unsigned int gDual_size = 0;
#define CALIB_FILE_NAME "calib_fpn.bin"
#define CALIB_DUAL_NAME "dual.raw"

int camera_dual_init(void)
{
	int sfile = 0;
	int sread = 0;
	FILE *fp = fopen(CALIB_DUAL_NAME, "rb");
	if(fp == NULL) {
		DBG("open file fail: %s\n", CALIB_DUAL_NAME);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	sfile = ftell(fp)/sizeof(short unsigned int);
	DBG("sfile = %d\n", sfile);

	gDual_size = sfile;
	gDual_data = (short unsigned int *)malloc(sfile*sizeof(short unsigned int));
	if(gDual_data == NULL) {
		DBG("malloc gDual_data fail\n");
		goto malloc_fail;
	}

	fseek(fp, 0, SEEK_SET);
	sread = fread(gDual_data, sizeof(short unsigned int), sfile, fp);
	DBG("sread = %d, sfile = %d, err = %s\n", sread, sfile, strerror(errno));

malloc_fail:
	fclose(fp);
	fp = NULL;

	return 0;
}

void camera_dual_exit(void)
{
	if(gDual_data) {
		free(gDual_data);
		gDual_data = NULL;
	}
	return;
}

int camera_calib_init(void)
{
	int sfile = 0;
	int sread = 0;
	FILE *fp = fopen(CALIB_FILE_NAME, "rb");
	if(fp == NULL) {
		DBG("open file fail: %s\n", CALIB_FILE_NAME);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	sfile = ftell(fp)/sizeof(short unsigned int);
	DBG("sfile = %d, sizeof(short unsigned int) = %ld\n", sfile, sizeof(short unsigned int));

	gCalib_size = sfile;
	gCalib_data = (short unsigned int *)malloc(sfile*sizeof(short unsigned int));
	if(gCalib_data == NULL) {
		DBG("malloc gCalib_data fail\n");
		goto malloc_fail;
	}

	fseek(fp, 0, SEEK_SET);
	sread = fread(gCalib_data, sizeof(short unsigned int), sfile, fp);
	DBG("sread = %d, sfile = %d, err = %s\n", sread, sfile, strerror(errno));
	
	gCalib_enable = 0;

malloc_fail:
	fclose(fp);
	fp = NULL;

	return 0;
}

void camera_calib_exit(void)
{
	if(gCalib_data) {
		free(gCalib_data);
		gCalib_data = NULL;
	}
	return;
}


int camera_calib_enable(int enable)
{
	gCalib_enable = enable;
	return 0;
}

int camera_calib(short unsigned int *p, unsigned int sdata)
{
	DBG("sdata = %d\n", sdata);
	if(!gCalib_enable)
		return 0;

	short unsigned int *calib = gCalib_data;
	short unsigned int *ptr[9];
	int count = sdata/gCalib_size;
	unsigned int i,j;
	unsigned int offset = 173*sizeof(short unsigned int);
	DBG("count = %d\n", count);

	for(i=0; i<count; i++)
		ptr[i] = p + i*gCalib_size + 173*sizeof(short unsigned int);

	for(i=0; i<gCalib_size; i++){
		for(j=1; j<count; j++)
			ptr[j][i] -= calib[i];
	}

	FILE *fp = fopen("save.raw", "wb");
	if(fp) {
		fwrite(gDual_data, sizeof(short unsigned int), sdata, fp);
		fclose(fp);
	}
	
	return 0;
}

int main(void)
{
	int ret;
	short unsigned int *p = NULL;
	unsigned int size;

	ret  = camera_calib_init();
	ret |= camera_dual_init();
	if(ret)
		goto fail;

	camera_calib_enable(1);
	p = gDual_data;
	size = gDual_size;
	camera_calib(p, size);

fail:
	camera_dual_exit();
	camera_calib_exit();

	return 0;
}

