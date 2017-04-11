#define LOG_TAG             "AL_LIBC"

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
#include <linux/asoc_ion.h>

//#define TRUE            1
//#define FALSE           0




#include <ion.h>//#include <ion/ion.h>
#include "display_regulate.h"


//////////////////////////////////

//////////////////////////////////


#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

//#define PRINT_BUF_SIZE      1024
struct actal_mem  {
	int fd;
	struct ion_handle * handle;
	int len;
	void *ptr;
	int map_fd;
	int phy_add;
	struct actal_mem *next;
};

int decode_one_frame(struct actal_mem * user_p, unsigned int width, unsigned int hight_16bytes_align)
{
		int count = 0;

		FILE *fd = NULL;
		#define BUFFER_LENGTH		256
		char buf[BUFFER_LENGTH];

		memset(buf, 0, BUFFER_LENGTH);
		strcpy(buf, "/data");
		strcat(buf, "/1920x1088_nv12.001.yuv");
		fd = fopen(buf, "rb");
		if (fd == NULL) {
			ALOGE("fopen(1920x1088_nv12) error!\n");
			return -1;
		}
		count = fread(user_p->ptr, width, hight_16bytes_align*1.5, fd);
		if(count!=(1088*1.5)) {
			ALOGE("fread return %d data\n", count);
		}
		fclose(fd);

		return 0;
}



int main(int argc, char* argv[])
{
	int prot = PROT_READ | PROT_WRITE;
	int map_flags = MAP_SHARED;
	struct ion_handle *handle;
	int map_fd, ret;
    void *ptr;
	
	int *phy_add = malloc(sizeof(int));


	int s_fd = 0;
	unsigned int width = 1920;//也应该16字节对齐
	unsigned int hight = 1080;
	unsigned int hight_16bytes_align = (((hight+15)/16)*16);

	int size = width*hight_16bytes_align*1.5;


	///////////////////////////////
	///1
	///////////////////////////////
	s_fd = ion_open();


	///////////////////////////////
	///2
	///////////////////////////////
	if (size & ALIGN_MASK){
		//4k对齐
		size += (ALIGN_BYTES - (size & ALIGN_MASK));

		ALOGE("(size-1920*1080)=%d\n",size-1920*1080);
	}

	struct actal_mem * user_p;

	user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
	user_p->next = NULL;

	ret = ion_alloc(s_fd, size, 0, 1, 3, &handle);
	if(ret < 0) {
		ALOGE("actal_malloc_wt: ion_alloc(size: %d)  failed(%d)!\n", size, ret);
		return ret;
	}

	ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

	user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
	user_p->ptr = ptr;
	user_p->map_fd = map_fd;


	ret = ion_phys(s_fd, handle, (unsigned long *)phy_add);
	if(ret < 0)
	{
		ALOGE("actal_malloc_wt: get phy_addr error!\n");
		return ret;
	}
	user_p->phy_add = *phy_add;



	///////////////////////////////
	///3.1 init display config
	///////////////////////////////
	ret = set_output_display_resolution(S1920x1080p);//first set output
	if(ret) {
		ALOGE("set_output_display_resolution return %d\n", ret);
		return ret;
	}

	ret = display_flush_frame(user_p->phy_add, user_p->phy_add+width*hight_16bytes_align);
	if(ret) {
		ALOGE("display_flush_frame return %d\n", ret);
		return ret;
	}

	ret = init_display_config(width, hight);
	if(ret) {
		ALOGE("init_display_config return %d\n", ret);//then set input
		return ret;
	}
	ALOGD("init display config done\n");




	///////////////////////////////
	///3.2 display this frame
	///////////////////////////////
	decode_one_frame(user_p, width, hight_16bytes_align);
	if(ret) {
		ALOGE("decode_one_frame return err: %d\n", ret);
		return ret;
	}

	ret = display_flush_frame(user_p->phy_add, user_p->phy_add+width*hight_16bytes_align);
	if(ret) {
		ALOGE("display_flush_frame return %d\n", ret);
		return ret;
	}

	ALOGD("display this frame done\n");




	///////////////////////////////
	///4
	///////////////////////////////
	{
		ret = ion_free(user_p->fd, user_p->handle);
		munmap(ptr, user_p->len);
		close(user_p->map_fd);

		free(user_p);

	}

	///////////////////////////////
	///5
	///////////////////////////////
	ion_close(s_fd);

	if(phy_add) {
		free(phy_add);
	}
	ALOGD("done\n");

	return 0;

}
