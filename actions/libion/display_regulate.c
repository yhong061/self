#ifdef ANDROID
//#include <utils/Log.h>
#endif
//#ifndef ANDROID
#define ALOGV(fmt, args...) do{printf("V " fmt "\n", ##args);}while(0)
#define ALOGD(fmt, args...) do{printf("D " fmt "\n", ##args);}while(0)
#define ALOGI(fmt, args...) do{printf("I " fmt "\n", ##args);}while(0)
#define ALOGW(fmt, args...) do{printf("W " fmt "\n", ##args);}while(0)
#define ALOGE(fmt, args...) do{printf("E " fmt "\n", ##args);}while(0)
//#endif


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>         /* for write */
#include <errno.h>
#include <pthread.h>        /* pthread_cond_*, pthread_mutex_* */
#include <semaphore.h>      /* sem_wait, sem_post */
#include <signal.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdarg.h>

#include <sys/types.h>

//#include <linux/fb.h>

#include "display_regulate.h"

////////////////////////////////////

static void read_str(const char* file_name, char* str)
{
	FILE* fp = fopen(file_name, "r"); 
	if (fp == NULL){
		ALOGE("open file(%s) error", file_name);
		return ;
	}
	
	const int MAX_SIZE=256;
	
	fgets(str, MAX_SIZE, fp);
	fclose(fp);
}

static void write_str(const char* file, const char* str)
{
	#define BUFFER_LENGTH		256
    const int SIZE=BUFFER_LENGTH;
	char tmp[BUFFER_LENGTH];

	int fd = open(file, O_WRONLY);	
	if (fd < 0){
		ALOGE("open file(%s) error", file);
		return ;
	}

	memset(tmp, 0, BUFFER_LENGTH);
    strcat(tmp, str);

    write(fd, tmp, SIZE);
    fsync(fd); //sync to disk
    close(fd);

	//ALOGD("write %s to %s", tmp, file);

	#undef BUFFER_LENGTH
}

int set_output_display_resolution(int resolution)
{

	#define BUFFER_LENGTH		256
	char file_name[BUFFER_LENGTH];
	char result_buf[BUFFER_LENGTH];
	
	memset(file_name, 0, BUFFER_LENGTH);
	strcpy(file_name, "/sys/devices/platform/owlfb.0/graphics/fb0");
	strcat(file_name, "/mode");


	memset(result_buf, 0, BUFFER_LENGTH);
	read_str(file_name, result_buf);
	//ALOGD("ori resolution:=%s",result_buf);
	//fflush(stdout);

	memset(result_buf, 0, BUFFER_LENGTH);
	if(resolution==S1920x1080p) {
		//ALOGD("want to change to 1080p");
		strcat(result_buf, "S:1920x1080p-60\n");
	} else if(resolution==S1280x720p) {
		//ALOGD("want to change to 720p");
		strcat(result_buf, "S:1280x720p-60\n");
	}
	write_str(file_name, result_buf);

	memset(result_buf, 0, BUFFER_LENGTH);
	read_str(file_name, result_buf);
	//ALOGD("now new resolution:=%s",result_buf);

	#undef BUFFER_LENGTH

	return 0;
}

int init_display_config(unsigned int width, unsigned int hight)
{
	FILE *fd = NULL;
	#define BUFFER_LENGTH		256
	char buf[BUFFER_LENGTH];

	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, "/sys/devices/platform/owldss.0");
	strcat(buf, "/kuxin_init");
	fd = fopen(buf, "w");
	if (fd == NULL) {
		ALOGE("fopen(kuxin_init) error!\n");
		return -1;
	}

	memset(buf, 0, BUFFER_LENGTH);
	snprintf(buf, BUFFER_LENGTH, "%d-%d", width, hight);
	fputs(buf, fd);

	fclose(fd);
	#undef BUFFER_LENGTH

	return 0;
}


int display_flush_frame(int phy_addY, int phy_addUV)
{
	FILE *fd = NULL;
	#define BUFFER_LENGTH		256
	char buf[BUFFER_LENGTH];

	memset(buf, 0, BUFFER_LENGTH);
	strcpy(buf, "/sys/devices/platform/owldss.0");
	strcat(buf, "/kuxin_display_flush");
	fd = fopen(buf, "w");
	if (fd == NULL) {
		ALOGE("fopen(kuxin_display_flush) error!\n");
		return -1;
	}

	memset(buf, 0, BUFFER_LENGTH);
	snprintf(buf, BUFFER_LENGTH, "%x-%x", phy_addY, phy_addUV);
	fputs(buf, fd);

	fclose(fd);
	#undef BUFFER_LENGTH

	return 0;
}


////////////////////////////////////

