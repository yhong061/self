#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/videodev2.h>


#define CAMERA_DEVICE "/dev/video"
#define CAPTURE_FILE "frame"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define BUFFER_COUNT 4

typedef unsigned short __u16;
typedef unsigned int	__u32;
typedef unsigned char __u8;
typedef unsigned long long	__u64;	

typedef struct VideoBuffer {
    void   *start;
    size_t  length;
} VideoBuffer;

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

int framesize_index_i = 0;
int format_index_i = 0;
int framesave_index = 1;
int framerate_index = 30;
int gFd = -1;


int camera_get_framerate(int *framerate)
{
	int ret = 0;
	struct v4l2_streamparm streamparm;

	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(gFd, VIDIOC_G_PARM, &streamparm);
	if(ret) {
		printf("[%d]fail: VIDIOC_G_PARM\n", __LINE__);
		return -1;
	}
	*framerate = streamparm.parm.capture.timeperframe.denominator;
	
	printf("=======================[VIDIOC_G_PARM: S]=======================\n");
	printf("denominator = %d\n", streamparm.parm.capture.timeperframe.denominator);
	printf("=======================[VIDIOC_G_PARM: E]=======================\n");
	
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
	if(ret) {
		printf("[%d]fail: VIDIOC_S_PARM\n", __LINE__);
		return -1;
	}
	printf("=======================[VIDIOC_S_PARM: S]=======================\n");
	printf("denominator = %d, framerate = %d\n", streamparm.parm.capture.timeperframe.denominator, framerate);
	printf("=======================[VIDIOC_S_PARM: E]=======================\n");
	return ret;
}


int test_framerate(int fps)
{
    int ret = 0;
    int framerate;

    ret = camera_get_framerate(&framerate);
    if(ret) 
        printf("get framerate fail, ret = %d\n", ret);
    else
        printf("get framerate OK: framerate = %d\n", framerate);

    framerate = fps;
    ret = camera_set_framerate(framerate);
    if(ret) {
        printf("set framerate [%d] fail, ret = %d\n", framerate, ret);
    }

    ret = camera_get_framerate(&framerate);
    if(ret) 
        printf("get framerate fail, ret = %d\n", ret);
    else
        printf("get framerate OK: framerate = %d\n", framerate);

    return ret;
}



int camera_G_FMT(void)
{
	int ret = 0;
	struct v4l2_format fmt;
	char fmtstr[8];

	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		printf("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

sleep(1);printf("\n\n");
	printf("=======================[VIDIOC_G_FMT: S]=======================\n");
	printf(" type: %d\n", fmt.type);
	printf(" width: %d\n", fmt.fmt.pix.width);
	printf(" height: %d\n", fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	printf(" pixelformat: %s\n", fmtstr);
	printf(" field: %d\n", fmt.fmt.pix.field);
	printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
	printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
	printf(" priv: %d\n", fmt.fmt.pix.priv);
	printf("=======================[VIDIOC_G_FMT: E]=======================\n");
sleep(1);printf("\n\n");

	return 0;
}

int camera_S_FMT(unsigned int w, unsigned int h, unsigned int pixfmt)
{
	int ret = 0;
	struct v4l2_format fmt;

	if(w==0 || h==0) {
		printf("unsuport resolution: wxh=%dx%d\n", w, h);
		return 0;
	}

	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		printf("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}
	
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = w;
	fmt.fmt.pix.height      = h;
	fmt.fmt.pix.pixelformat = pixfmt;
	ret = ioctl(gFd, VIDIOC_S_FMT, &fmt);
	if(ret) {
		printf("[%d]fail: VIDIOC_S_FMT\n", __LINE__);
		return -1;
	}
	
	camera_G_FMT();

	return 0;
}

int camera_S_FMT_index(int framesize_index, int fmt_index)
{
	unsigned int w,h,pixfmt; 

	if(fmt_index > gFmt_index)
		fmt_index = 0;
	if(framesize_index > gFramesize_index)
		framesize_index = 0;


	w = v_eframesize[framesize_index].w; 
	h = v_eframesize[framesize_index].h;
	pixfmt  = v_efmt[fmt_index].pixfmt; 

sleep(1);printf("\n");
	printf("[%d] w = %d, h = %d, [%d]pixfmt = %d\n", framesize_index, w, h, fmt_index, pixfmt);
sleep(1);printf("\n");

	camera_S_FMT(w, h, pixfmt);

	return 0;

}

int camera_enumfmt(int index)
{
	int ret = 0;
	struct v4l2_fmtdesc fmtdest;
	char fmtstr[8];

	memset(&fmtdest, 0, sizeof(fmtdest));

	fmtdest.index               = index;
	fmtdest.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_ENUM_FMT, &fmtdest);
	if(ret) {
		printf("[%d]fail: VIDIOC_ENUM_FMT\n", __LINE__);
		return ret;
	}

	v_efmt[index].pixfmt = fmtdest.pixelformat;
	memcpy(v_efmt[index].desc, fmtdest.description, 32);
	v_efmt[index].desc[31] = '\0';

	printf(" index: %d\n", fmtdest.index);
	printf(" pixelformat: %d, %d\n", fmtdest.pixelformat, v_efmt[index].pixfmt);
	printf(" description: %s, %s\n", fmtdest.description, v_efmt[index].desc);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmtdest.pixelformat, 4);
	printf(" pixelformat: %s\n", fmtstr);

	return 0;
}

int camera_enumfmt_all(void)
{
	int ret = 0;
	int index = 0;
	printf("=======================[VIDIOC_ENUM_FMT: S]=======================\n");
	while(!ret){
		ret = camera_enumfmt(index);
		index++;
		printf("\n");
	}
	gFmt_index = index-1;
	printf("=======================[VIDIOC_ENUM_FMT: E (count = %d)]=======================\n", gFmt_index);
	return 0;
}

// #define VIDIOC_ENUM_FRAMESIZES  _IOWR('V', 74, struct v4l2_frmsizeenum)
int camera_enumframesize(int index)
{
	int ret = 0;
	struct v4l2_frmsizeenum frmsize;

	memset(&frmsize, 0, sizeof(frmsize));

	frmsize.index               = index;
	frmsize.pixel_format        = v_efmt[0].pixfmt;
	ret = ioctl(gFd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
	if(ret) {
		printf("[%d]fail: VIDIOC_ENUM_FRAMESIZES\n", __LINE__);
		return ret;
	}

	printf(" index: %d\n", frmsize.index);
	printf(" pixel_format: %d\n", frmsize.pixel_format);
	printf(" type: %d\n", frmsize.type);
	if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {

		v_eframesize[index].w = frmsize.discrete.width;
		v_eframesize[index].h = frmsize.discrete.height;

		printf(" width: %d, %d\n", frmsize.discrete.width, v_eframesize[index].w);
		printf(" height: %d, %d\n", frmsize.discrete.height, v_eframesize[index].h);
	}
	else {
		printf(" width: %d, %d\n", frmsize.stepwise.min_width, frmsize.stepwise.max_width);
		printf(" height: %d, %d\n", frmsize.stepwise.min_height, frmsize.stepwise.max_height);
	}

	return 0;
}

int camera_enumframesize_all(void)
{
	int ret = 0;
	int index = 0;
	printf("=======================[VIDIOC_ENUM_FRAMESIZES: S]=======================\n");
	while(!ret){
		ret = camera_enumframesize(index);
		index++;
		printf("\n");
	}
	gFramesize_index = index-1;
	printf("=======================[VIDIOC_ENUM_FRAMESIZES: E: (count = %d)]=======================\n", gFramesize_index);
	return index;
}



int main(int argc, char *argv[])
{
    int i, ret;
    VideoBuffer framebuf[BUFFER_COUNT];
    int idx = 0;
    char videoname[32];
    char savefile[64];
    int fd;


    printf("%s framesize(0) format(0:mjpeg;1:yuv) framerate(30) framesave(1) \n", argv[0]);
  
    if(argc > 1) {
        framesize_index_i = atoi(argv[1]);
    }
    if(argc > 2) {
        format_index_i = atoi(argv[2]);
    }
    if(argc > 3) {
        framerate_index = atoi(argv[3]);
    }
    if(argc > 4) {
        framesave_index = atoi(argv[4]);
    }
    
    printf("framesize_index_i = %d, format_index_i = %d, framerate_index = %d, framesave_index = %d\n", framesize_index_i, format_index_i, framerate_index, framesave_index);

    do{
        sprintf(videoname, "%s%d", CAMERA_DEVICE, idx++);
	fd = open(videoname, O_RDWR, 0);
	if(fd >=0) {
	    printf("open %s OK\n", videoname);
	    break;
	}
        printf("open %s fail, idx = %d, continue\n", videoname, idx);
    }while(idx < 4);
    if(fd < 0) {
        printf("open video fail\n");
	return -1;
    }

    gFd = fd;

    struct v4l2_capability cap;
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if(ret)
        printf("[%d]fail: VIDIOC_QUERYCAP\n", __LINE__);
    // Print capability infomations
    printf("Capability Informations:\n");
    printf(" driver: %s\n", cap.driver);
    printf(" card: %s\n", cap.card);
    printf(" bus_info: %s\n", cap.bus_info);
    printf(" version: %08X\n", cap.version);
    printf(" capabilities: %08X\n", cap.capabilities);

    camera_G_FMT();    
    camera_enumfmt_all();
    camera_enumframesize_all();
    camera_S_FMT_index(framesize_index_i, format_index_i);
    test_framerate(framerate_index);
    
    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP; 
    ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);
    if(ret)
        printf("[%d]fail: VIDIOC_REQBUFS, ret = %d\n", __LINE__, ret);

    struct v4l2_buffer buf;
    for (i = 0; i < reqbuf.count; i++) 
    {
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd , VIDIOC_QUERYBUF, &buf);  
	if(ret)
	    printf("[%d]fail: VIDIOC_QUERYBUF\n", __LINE__);

        // mmap buffer
        framebuf[i].length = buf.length;
        framebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    
        // Queen buffer
        ret = ioctl(fd , VIDIOC_QBUF, &buf); 
	if(ret)
	    printf("[%d]fail: VIDIOC_QBUF\n", __LINE__);

        printf("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].start, framebuf[i].length);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if(ret)
        printf("[%d]fail: VIDIOC_STREAMON\n", __LINE__);

    int cnt, frame = framesave_index + 10;
    int saved = 0;
   
    for(cnt = 0; cnt<frame; cnt++) {
    
	    // Get frame
	    ret = ioctl(fd, VIDIOC_DQBUF, &buf);   
	    if(ret)
		printf("[%d]fail: VIDIOC_DQBUF\n", __LINE__);
	    
	    printf("buf.index = %d\n", buf.index);

	    if(!saved && cnt == framesave_index) {
		    // Process the frame
		    if(format_index_i == 0)
		    	sprintf(savefile, "%s_wh%d_skip%d.jpeg", CAPTURE_FILE, framesize_index_i, framesave_index);
		    else
		    	sprintf(savefile, "%s_wh%d_skip%d.yuv", CAPTURE_FILE, framesize_index_i, framesave_index);
		    FILE *fp = fopen(savefile, "wb");
		    int writesize = 0;
		    writesize = fwrite(framebuf[buf.index].start, 1, buf.length, fp);
		    fclose(fp);
		    printf("Capture one frame saved in %s, size = %d\n", savefile, buf.length);
		    saved = 1;
	    }

	    // Re-queen buffer
	    ret = ioctl(fd, VIDIOC_QBUF, &buf);
	    if(ret)
		printf("[%d]fail: VIDIOC_QBUF\n", __LINE__);
    }

    // Release the resource
    for (i=0; i< BUFFER_COUNT; i++) 
    {
        munmap(framebuf[i].start, framebuf[i].length);
    }

    close(fd);
    printf("Camera test Done.\n");
    return 0;
}
