#define  LOG_TAG  "UVCVIDEO"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <ctype.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/videodev2.h>
#include <ion.h>
#include "video.h"
//#include <linux/v4l2-controls.h>

typedef unsigned short __u16;
typedef unsigned int	__u32;
typedef unsigned char __u8;
typedef unsigned long long	__u64;	

struct ion_buf {
	unsigned int		size;
	struct ion_handle 	*handle;
	void 			*vir;
	int 			phy;
	int 			map_fd;
};

//#define DBG(fmt, ...) printf("[video_test][%d]" fmt, __LINE__, ## __VA_ARGS__)

#define CAMERA_DEVICE "/dev/video1"
#define BUFFER_COUNT 4
struct ion_buf framebuf[BUFFER_COUNT];
int gFd = -1;
int gIndex = 0;

#define EEPROM_FILE "/data/eeprom.bin"
#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

unsigned int gWidth = 224;
unsigned int gHeight = 1557;
unsigned int gImgsize = 224*1557*2;

int test_eeprom_file(void) {
    if(!access(EEPROM_FILE, R_OK)) {
        return 0;
    }

    ALOGD("================read_eeprom: start============\n");
    eeprom_read(EEPROM_FILE); 
    ALOGD("================read_eeprom: end  ============\n");

    return 0;
}

int ion_buf_alloc(struct ion_buf *buf, unsigned int size)
{
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED;
	struct ion_handle *handle = NULL;
	int map_fd = -1;
	int ret;
    	void *vir = NULL;
	int phy = 0;

	memset(buf, 0, sizeof(struct ion_buf));

	if (size & ALIGN_MASK){
		size += (ALIGN_BYTES - (size & ALIGN_MASK));
		ALOGD("(size-1280*960)=%d\n",size-1280*960);
	}

	map_fd = system_mmap(size, prot, flags, &handle, (unsigned char **)&vir, &phy);
	if(map_fd < 0) {
		ALOGD("system map fail: map_fd = %d\n", map_fd);
		system_munmap(handle, size, vir, &phy, map_fd);
		return -1;
	}
	buf->size = size;
	buf->handle = handle;
	buf->vir = vir;
	buf->phy = phy;
	buf->map_fd = map_fd;

	return 0;
}

int ion_buf_free(struct ion_buf *buf)
{
	system_munmap(buf->handle, buf->size, buf->vir, &buf->phy, buf->map_fd);
	return 0;
}

int camera_checkimagefmt(unsigned int width, unsigned int height)
{
	int ret = 0;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		ALOGE("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

	ALOGD(" width: %d\n", fmt.fmt.pix.width);
	ALOGD(" height: %d\n", fmt.fmt.pix.height);

	if(fmt.fmt.pix.width == width && fmt.fmt.pix.height == height)
		return 0;
	else
		return 1;
}

int camera_set_freq(int value)
{
	int ret = 0;
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_AF_MODE;
	ctrl.value = value;

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if(ret != 0) {
		ALOGD("Set VIDIOC_S_CTRL fail: freq\n");
	}
	else {
		ALOGD("Set VIDIOC_S_CTRL Ok: freq\n");
	}
	return ret;
}

int camera_setimagefmt(enum FMT_TYPE fmttype)
{
	int ret = 0;
	struct v4l2_format fmt;
	unsigned int width, height;
	int freq = 0; //60M

	switch(fmttype){
		case FMT_TYPE_MONOFREQ_80M:
			freq = 1; //80M
		case FMT_TYPE_MONOFREQ:
			width = 224;
			height = 173*5;
		break;

		case FMT_TYPE_GRAY_DEPTH:
			width = 224;
			height = 172*4;
		break;

		default:
			width = 224;
			height = 173*9;
	}

	camera_set_freq(freq);

	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		ALOGD("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
	ret = ioctl(gFd, VIDIOC_S_FMT, &fmt);
	if(ret) {
		ALOGD("[%d]fail: VIDIOC_S_FMT\n", __LINE__);
		return -1;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		ALOGD("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

	gWidth = fmt.fmt.pix.width;
	gHeight = fmt.fmt.pix.height;
	gImgsize = fmt.fmt.pix.sizeimage;
	ALOGD("gWidth = %d, gHeight = %d, gImgsize = %d\n", gWidth, gHeight, gImgsize);

	return 0;
}

int camera_open(char *filename, enum FMT_TYPE fmttype)
{
    int ret;
    unsigned int i;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers reqbuf;
    unsigned int width = 1280;
    unsigned int hight = 1557;
    unsigned int hight_16bytes_align = (((hight+15)/16)*16);
    
    unsigned int size = width*hight_16bytes_align*2;

    //gFd = open(filename, O_RDWR | O_NONBLOCK, 0);
    gFd = open(filename, O_RDWR, 0);
    if(gFd < 0) {
    	ALOGD("open %s fail\n", filename);
	return -1;
    }

    camera_reg_open();

    camera_setimagefmt(fmttype);

    memset(framebuf, 0, sizeof(struct ion_buf)*BUFFER_COUNT);
    for(i=0; i<BUFFER_COUNT; i++) {
        ret = ion_buf_alloc((struct ion_buf *)&framebuf[i], size);
	if(ret < 0) {
	    ALOGD("ion buf alloc fail\n");
	    return 0;
	}
    }

    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_USERPTR; 
    ret = ioctl(gFd , VIDIOC_REQBUFS, &reqbuf);
    if(ret) {
        ALOGD("[%d]fail: VIDIOC_REQBUFS\n", __LINE__);
	return -1;
    }

    ALOGD("reqbuf.count = %d,BUFFER_COUNT = %d\n", reqbuf.count, BUFFER_COUNT);
    for (i = 0; i < reqbuf.count; i++) 
    {
    	memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        ret = ioctl(gFd , VIDIOC_QUERYBUF, &buf);  
        if(ret) {
		ALOGD("[%d]fail: VIDIOC_QUERYBUF\n", __LINE__);
		return -1;    
        }

	buf.m.userptr = (unsigned long)framebuf[i].phy;
	buf.length = gImgsize; //framebuf[i].size;

        // Queen buffer
        ret = ioctl(gFd , VIDIOC_QBUF, &buf); 
	if(ret)
	    ALOGD("[%d]fail: VIDIOC_QBUF, errno = %d\n", __LINE__, errno);

        ALOGD("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].phy, framebuf[i].size);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(gFd, VIDIOC_STREAMON, &type);
    if(ret) {
        ALOGD("[%d]fail: VIDIOC_STREAMON\n", __LINE__);
	return -1;
    }

    return 0;
}

int camera_close(void)
{
    int i;

    camera_reg_close();

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(gFd, VIDIOC_STREAMOFF, &type);
    if(ret) {
        ALOGD("[%d]fail: VIDIOC_STREAMOFF, ret = %d\n", __LINE__, ret);
	//return -1;
    }

    // Release the resource
    for (i=0; i< BUFFER_COUNT; i++) 
    {
        if(framebuf[i].vir) {
	    ion_buf_free(&framebuf[i]);
	}
    }

    if(gFd)
        close(gFd);

    return 0;

}

int camera_get_framerate(int *framerate)
{
	int ret = 0;
	struct v4l2_streamparm streamparm;

	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(gFd, VIDIOC_G_PARM, &streamparm);
	if(ret == 0) {
		*framerate = streamparm.parm.capture.timeperframe.denominator;
		ALOGD("denominator = %d, framerate = %d\n", streamparm.parm.capture.timeperframe.denominator, *framerate);
	}
	else {
		ALOGD("Get QueryCtrl fail\n");
	}
	
	return ret;

}

int camera_set_framerate(int framerate)
{
	int ret = 0;
	struct v4l2_streamparm streamparm;

	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.timeperframe.numerator = 1;
	streamparm.parm.capture.timeperframe.denominator = framerate;

	ret = ioctl(gFd, VIDIOC_S_PARM, &streamparm);
	if(ret != 0) {
		ALOGD("Set VIDIOC_S_PARM fail\n");
	}
	else {
		ALOGD("Set VIDIOC_S_PARM Ok\n");
	}
	return ret;
}

int camera_get_exposure_query(int *min, int *max, int *step)
{
	int ret = 0;
	struct v4l2_queryctrl queryctrl;

	queryctrl.id = V4L2_CID_EXPOSURE;
	queryctrl.minimum = 0;
	queryctrl.maximum = 0;
	queryctrl.step = 0;
	queryctrl.default_value = 0;
	queryctrl.flags = 0;

	ret = ioctl(gFd, VIDIOC_QUERYCTRL, &queryctrl);
	if(ret == 0) {
		ALOGD("min = %d\n", queryctrl.minimum);
		ALOGD("max = %d\n", queryctrl.maximum);
		ALOGD("default = %d\n", queryctrl.default_value);
		ALOGD("step = %d\n", queryctrl.step);
		ALOGD("flag = %d\n", queryctrl.flags);

		*min = queryctrl.minimum;
		*max = queryctrl.maximum;
		*step = queryctrl.step;
	}
	else
		ALOGD("Get QueryCtrl fail\n");
	
	return ret;
}

int camera_get_exposure(int *value)
{
	int ret = 0;
	struct v4l2_control ctrl;

	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = V4L2_CID_EXPOSURE;

	ret = ioctl(gFd, VIDIOC_G_CTRL, &ctrl);
	if(ret != 0) {
		ALOGD("Set VIDIOC_G_CTRL fail\n");
	}
	else {
		*value = ctrl.value;
		ALOGD("Set VIDIOC_G_CTRL Ok: value = %d\n", ctrl.value);
	}
	return ret;
}

int camera_set_exposure(int value)
{
	int ret = 0;
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_EXPOSURE;
	ctrl.value = value;

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if(ret != 0) {
		ALOGD("Set VIDIOC_S_CTRL fail\n");
	}
	else {
		ALOGD("Set VIDIOC_S_CTRL Ok\n");
	}
	return ret;
}

int camera_getbuf(char *p, unsigned int size)
{
    int ret;
    struct v4l2_buffer buf;

    buf.index = gIndex;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    // Get frame
    ret = ioctl(gFd, VIDIOC_DQBUF, &buf);   
    if(ret) {
        ALOGD("[%d]fail: VIDIOC_DQBUF: errno = %d\n", __LINE__, errno);
        return -1;
    }

    // Process the frame
    size = buf.length;
    memcpy(p, framebuf[buf.index].vir, size);

    // Re-queen buffer
    ret = ioctl(gFd, VIDIOC_QBUF, &buf);
    if(ret) {
        ALOGD("[%d]fail: VIDIOC_QBUF\n", __LINE__);
        return -1;	
    }
    gIndex++;
    if(gIndex >= BUFFER_COUNT)
        gIndex = 0;

    return size;
}

int camera_getbuf2(char *p1, unsigned int *s1, char *p2, unsigned int *s2)
{
    int ret;
    unsigned int size;
    struct v4l2_buffer buf;
    char *ptr;

    buf.index = gIndex;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    // Get frame
    ret = ioctl(gFd, VIDIOC_DQBUF, &buf);   
    if(ret) {
        ALOGE("[%d]fail: VIDIOC_DQBUF: errno = %d\n", __LINE__, errno);
        return -1;
    }

    // Process the frame
    ptr = (char *)framebuf[buf.index].vir;
    size = buf.length/2;
    *s1 = size;
    *s2 = size;
    memcpy(p1, ptr, size);
    memcpy(p2, ptr+size, size);

    // Re-queen buffer
    ret = ioctl(gFd, VIDIOC_QBUF, &buf);
    if(ret) {
        ALOGE("[%d]fail: VIDIOC_QBUF\n", __LINE__);
        return -1;	
    }
    gIndex++;
    if(gIndex >= BUFFER_COUNT)
        gIndex = 0;

    return size;
}

int camera_changefmt(enum FMT_TYPE fmttype)
{
	int ret;
	unsigned int width, height;

	switch(fmttype){
		case FMT_TYPE_MONOFREQ:
		case FMT_TYPE_MONOFREQ_80M:
			width = 224;
			height = 173*5;
		break;

		case FMT_TYPE_GRAY_DEPTH:
			width = 224;
			height = 172*4;
		break;
		
		default:
		case FMT_TYPE_MULTIFREQ:
			width = 224;
			height = 173*9;
		break;
	}

	if(!camera_checkimagefmt(width, height))
		return 0;

	ALOGD("reopen camera to change fmt\n");
	camera_close();
    	ret = camera_open(CAMERA_DEVICE, fmttype);
    	if(ret < 0) {
		return -1;
    	}
	return 0;
}
