#ifndef __VIDEO__H__
#define __VIDEO__H__

#include <stdio.h>
#include <unistd.h>
#include <string.h> //memset
#include <stdlib.h>
#include <sys/time.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>  //MAP_SHARED

#define DBG(fmt, ...)  printf("[video_main][%d]" fmt, __LINE__, ## __VA_ARGS__)


#define CAMERA_DEVICE "/dev/video"
#define CAPTURE_FILE "frame.yuv"
#define FRAME_RATE	(30)
#define FRAME_WIDTH	(1920)
#define FRAME_HEIGTH	(1080)

#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define BUFFER_COUNT 4

#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

struct ion_buf {
	unsigned int		size;
	struct ion_handle 	*handle;
	void 			*vir;
	int 			phy;
	int 			map_fd;
};

int gFd = -1;
struct ion_buf framebuf[BUFFER_COUNT];
int gIndex = 0;


int ion_buf_alloc(struct ion_buf *buf, unsigned int size);
int ion_buf_free(struct ion_buf *buf);

///////////////////////////////////////////////////////////////////////////

struct video_enumframesizes {
	unsigned int w;
	unsigned int h;
};

struct video_enumframesizes v_eframesize[10];
int gFramesize_index = 0;

struct video_enumfmt {
	unsigned int pixfmt;
	unsigned char desc[32];

};

struct video_enumfmt v_efmt[6];
int gFmt_index = 0;



#endif
