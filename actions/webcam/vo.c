#include <errno.h>

/* Verification Test Environment Include Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>

//#include <linux/mxcfb.h>
//#include <linux/mxc_v4l2.h>
//#include <linux/ipu.h>

#define DISPLAY
#define        ONLY_DISPLAY
//#define CAPTURE


#define TEST_BUFFER_NUM 4
#define PASS    0
#define FAIL    -1

struct testbuffer
{
    unsigned char *start;
    size_t offset;
    unsigned int length;
};

struct testbuffer capture_buffers[TEST_BUFFER_NUM];
struct testbuffer display_buffers[TEST_BUFFER_NUM];

int capture_in_width = 640;
int capture_in_height = 480;
int capture_in_top = 0;
int capture_in_left = 0;
int capture_out_width = 640;
int capture_out_height = 480;
#ifdef ONLY_DISPLAY
int display_in_width = 640;//352;
int display_in_height = 480;//288;
#else
int display_in_width = 640;
int display_in_height = 480;
#endif
int display_out_width = 1024;
int display_out_height = 768;
int display_out_top = 0;
int display_out_left = 0;
int capture_count = 100;
int capture_input = 0;
int camera_framerate = 30;
int capture_fmt = V4L2_PIX_FMT_YUYV;
int display_fmt = V4L2_PIX_FMT_YUYV;

int mem_mmap_type = V4L2_MEMORY_MMAP;
int g_num_buffers = 0;
int capture_mode = 0;

int capture_frame_size;

char capture_device[100] = "/dev/video0";
char display_device[100] = "/dev/video0";

FILE * fd_file = 0;

int fd_capture = 0;
int fd_display = 0;


int capture_open(void)
{
    if ((fd_capture = open(capture_device, O_RDWR, 0)) < 0)
    {
        printf("Unable to open %s\n", capture_device);
        return FAIL;
    }
    return PASS;
}

int display_open(void)
{
    fd_display = open(display_device, O_RDWR, 0);
    if (fd_display < 0)
    {
        printf("Unable to open %s\n", display_device);
        return FAIL;
    }

    return PASS;
}

static void print_pixelformat(char *prefix, int val)
{
    printf("%s: %c%c%c%c\n", prefix ? prefix : "pixelformat",
           val & 0xff,
           (val >> 8) & 0xff,
           (val >> 16) & 0xff,
           (val >> 24) & 0xff);
}

int capture_set(void)
{
    struct v4l2_dbg_chip_ident chip;
    struct v4l2_frmsizeenum fsize;
    struct v4l2_fmtdesc ffmt;
    struct v4l2_streamparm parm;
    struct v4l2_crop crop;
    struct v4l2_format fmt;

    if (ioctl(fd_capture, VIDIOC_DBG_G_CHIP_IDENT, &chip))//获取摄像头名字
    {
        printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
        return FAIL;
    }
    printf("sensor chip is %s\n", chip.match.name);

    printf("sensor supported frame size:\n");
    fsize.index = 0;
    while (ioctl(fd_capture, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0)// 枚举所有帧大小(即像素宽度和高度),给定像素的设备支持的格式。
    {
        printf(" %dx%d\n", fsize.discrete.width,
               fsize.discrete.height);
        fsize.index++;
    }

    ffmt.index = 0;
    while (ioctl(fd_capture, VIDIOC_ENUM_FMT, &ffmt) >= 0) //获取当前驱动支持的视频格式
    {
        print_pixelformat("sensor frame format", ffmt.pixelformat);
        ffmt.index++;
    }

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = camera_framerate;
    parm.parm.capture.capturemode = capture_mode;

    if (ioctl(fd_capture, VIDIOC_S_PARM, &parm) < 0)    //设置Stream信息。如帧数等。
    {
        printf("VIDIOC_S_PARM failed\n");
        return FAIL;
    }

    if (ioctl(fd_capture, VIDIOC_S_INPUT, &capture_input) < 0)//选择视频输入
    {
        printf("VIDIOC_S_INPUT failed\n");
        return FAIL;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_capture, VIDIOC_G_CROP, &crop) < 0) //读取视频信号的边框
    {
        printf("VIDIOC_G_CROP failed\n");
        return FAIL;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.width = capture_in_width;
    crop.c.height = capture_in_height;
    crop.c.top = capture_in_top;
    crop.c.left = capture_in_left;
    if (ioctl(fd_capture, VIDIOC_S_CROP, &crop) < 0) //设置视频信号的边框
    {
        printf("VIDIOC_S_CROP failed\n");
        return FAIL;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = capture_fmt;
    fmt.fmt.pix.width = capture_out_width;
    fmt.fmt.pix.height = capture_out_height;
    fmt.fmt.pix.bytesperline = capture_out_width;
    fmt.fmt.pix.priv = 0;
    fmt.fmt.pix.sizeimage = 0;


    if (ioctl(fd_capture, VIDIOC_S_FMT, &fmt) < 0) //设置捕获格式
    {
        printf("set format failed\n");
        return FAIL;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_capture, VIDIOC_G_FMT, &fmt) < 0) //读取当前驱动的频捕获格式
    {
        printf("get format failed\n");
        return FAIL;
    }
    else
    {
        printf("\t Width = %d\n", fmt.fmt.pix.width);
        printf("\t Height = %d\n", fmt.fmt.pix.height);
        printf("\t Image size = %d\n", fmt.fmt.pix.sizeimage);
        printf("\t pixelformat = %d\n", fmt.fmt.pix.pixelformat);
    }
    capture_frame_size = fmt.fmt.pix.sizeimage;
        printf("**********capture frame size=%d\n",fmt.fmt.pix.sizeimage);
    return PASS;
}

int display_set(void)
{
    int ret;
//   struct v4l2_control ctrl;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_capability cap;
    struct v4l2_framebuffer fb;

printf("[%s:%d] VIDIOC_QUERYCAP \n", __func__, __LINE__);
    ret = ioctl(fd_display, VIDIOC_QUERYCAP, &cap);
sleep(1);
    if(ret < 0){
        printf("[%s:%d] VIDIOC_QUERYCAP fail, ret = %d\n", __func__, __LINE__, ret);
    }
    if (!ret)
    {
        printf("[%s:%d] driver=%s, card=%s, bus=%s, "
               "version=0x%08x, "
               "capabilities=0x%08x\n\n",
	       __func__, __LINE__,
               cap.driver, cap.card, cap.bus_info,
               cap.version,
               cap.capabilities);
    }

    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    while (1)
    {
printf("[%s:%d] VIDIOC_ENUM_FMT \n", __func__, __LINE__);
        ret = ioctl(fd_display, VIDIOC_ENUM_FMT, &fmtdesc);
        if(ret < 0){
            printf("[%s:%d] VIDIOC_ENUM_FMT fail, ret = %d\n", __func__, __LINE__, ret);
	    break;
        }
	if(!ret) {
            printf("[%s:%d]fmt %s: fourcc = 0x%08x\n", __func__, __LINE__,
               fmtdesc.description,
               fmtdesc.pixelformat);
            fmtdesc.index++;
	}
    }
    printf("[%s:%d] fmtdesc.index = %d\n", __func__, __LINE__,fmtdesc.index);

#if 0
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(fd_display, VIDIOC_CROPCAP, &cropcap) < 0)
    {
        printf("get crop capability failed\n");
        return FAIL;
    }
    printf("cropcap.bounds.width = %d\ncropcap.bound.height = %d\n" \
           "cropcap.defrect.width = %d\ncropcap.defrect.height = %d\n",
           cropcap.bounds.width, cropcap.bounds.height,
           cropcap.defrect.width, cropcap.defrect.height);

    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    crop.c.top = display_out_top;
    crop.c.left = display_out_left;
    crop.c.width = display_out_width;
    crop.c.height = display_out_height;
    if (ioctl(fd_display, VIDIOC_S_CROP, &crop) < 0)
    {
        printf("set crop failed\n");
        return FAIL;
    }
#endif    
    fb.capability = V4L2_FBUF_CAP_EXTERNOVERLAY;
    fb.flags = V4L2_FBUF_FLAG_PRIMARY;
printf("[%s:%d] VIDIOC_S_FBUF \n", __func__, __LINE__);
    ret = ioctl(fd_display, VIDIOC_S_FBUF, &fb);
    if(ret < 0){
        printf("[%s:%d] VIDIOC_S_FBUF fail, ret = %d\n", __func__, __LINE__, ret);
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width=display_in_width;
    fmt.fmt.pix.height=display_in_height;
    fmt.fmt.pix.pixelformat = display_fmt;
    fmt.fmt.pix.priv = 0;
printf("[%s:%d] VIDIOC_S_FMT \n", __func__, __LINE__);
    ret = ioctl(fd_display, VIDIOC_S_FMT, &fmt);
    if(ret < 0){
        printf("[%s:%d] VIDIOC_S_FBUF fail, ret = %d\n", __func__, __LINE__, ret);
        printf("set format failed\n");
        return FAIL;
    }

printf("[%s:%d] VIDIOC_G_FMT \n", __func__, __LINE__);
    ret = ioctl(fd_display, VIDIOC_G_FMT, &fmt);
    if (ret < 0)
    {
        printf("get format failed\n");
        return FAIL;
    }
    printf("[%s:%d] VIDIOC_G_FMT :\n", __func__, __LINE__);
    printf("\t fmt.fmt.pix.width = %d\n", fmt.fmt.pix.width);
    printf("\t fmt.fmt.pix.height = %d\n", fmt.fmt.pix.height);

    printf("**********display frame size=%d\n",fmt.fmt.pix.sizeimage);

    return PASS;
}

void capture_munmap()
{
    int i;

    for (i = 0; i < TEST_BUFFER_NUM; i++)
    {
        munmap (capture_buffers[i].start, capture_buffers[i].length);
    }
}

void display_munmap(void)
{
    int i;

    for (i = 0; i < g_num_buffers; i++)
    {
        munmap (display_buffers[i].start, display_buffers[i].length);
    }

}

int capture_mmap(void)
{
    unsigned int i;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof (req));
    req.count = TEST_BUFFER_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = mem_mmap_type;
    if (ioctl(fd_capture, VIDIOC_REQBUFS, &req) < 0) //向驱动提出申请内存的请求
    {
        printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
        return FAIL;
    }
    for (i = 0; i < TEST_BUFFER_NUM; i++)
    {
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = mem_mmap_type;
        buf.index = i;
        if (ioctl(fd_capture, VIDIOC_QUERYBUF, &buf) < 0) //把VIDIOC_REQBUFS中分配的数据缓存转换成物理地址
        {
            printf("VIDIOC_QUERYBUF error\n");
            capture_munmap();
            return FAIL;
        }

        capture_buffers[i].length = buf.length;
        capture_buffers[i].offset = (size_t) buf.m.offset;
        capture_buffers[i].start = mmap (NULL, capture_buffers[i].length,
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 fd_capture, capture_buffers[i].offset);
        memset(capture_buffers[i].start, 0xFF, capture_buffers[i].length);
    }

    for (i = 0; i < TEST_BUFFER_NUM; i++)
    {
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = mem_mmap_type;
        buf.index = i;
        buf.m.offset = capture_buffers[i].offset;
        if (ioctl (fd_capture, VIDIOC_QBUF, &buf) < 0) //把缓冲区放回缓存队列
        {
            printf("VIDIOC_QBUF error\n");
            capture_munmap();
            return FAIL;
        }
    }
    return PASS;
}
int display_mmap(void)
{
    int ret;
    struct v4l2_requestbuffers buf_req;
    struct v4l2_buffer buf;
    int i;

    memset(&buf_req, 0, sizeof(buf_req));
    buf_req.count = TEST_BUFFER_NUM;
    buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_req.memory = mem_mmap_type;
printf("[%s:%d] VIDIOC_REQBUFS \n", __func__, __LINE__);
    ret = ioctl(fd_display, VIDIOC_REQBUFS, &buf_req);
    if (ret < 0)
    {
        printf("request display_buffers failed\n");
        return FAIL;
    }
    g_num_buffers = buf_req.count;
    printf("v4l2_output test: Allocated %d display_buffers\n", buf_req.count);

    memset(&buf, 0, sizeof(buf));

    for (i = 0; i < g_num_buffers; i++)
    {
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = mem_mmap_type;
        buf.index = i;
printf("[%s:%d] VIDIOC_QUERYBUF \n", __func__, __LINE__);
        ret = ioctl(fd_display, VIDIOC_QUERYBUF, &buf);
        if (ret < 0)
        {
            printf("VIDIOC_QUERYBUF error\n");
            display_munmap();
            return FAIL;
        }

        display_buffers[i].length = buf.length;
        display_buffers[i].offset = (size_t) buf.m.offset;
        printf("VIDIOC_QUERYBUF: length = %d, offset = %d\n", display_buffers[i].length, display_buffers[i].offset);
        display_buffers[i].start = mmap (NULL, display_buffers[i].length,
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 fd_display, display_buffers[i].offset);
        if (display_buffers[i].start == NULL)
        {
            printf("v4l2_out test: mmap failed\n");
            display_munmap();
            return FAIL;
        }
    }
    return PASS;
}

int capture_starting()
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl (fd_capture, VIDIOC_STREAMON, &type) < 0) //开始视频显示函数
    {
        printf("VIDIOC_STREAMON error\n");
        return FAIL;
    }
    return PASS;
}

/*
void fb_setup(void)
{
    struct mxcfb_gbl_alpha alpha;
    int fd;

    fd = open("/dev/graphics/fb0",O_RDWR);

    alpha.alpha = 0;
    alpha.enable = 1;
    if (ioctl(fd, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
    {
        printf("set alpha %d failed for fb0\n",  alpha.alpha);
    }

    close(fd);
}
*/
int display_starting(void)
{
    int ret;
    int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    /*simply set fb0 alpha to 0*/
    //fb_setup();

    for (i = 0; i < g_num_buffers; i++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = mem_mmap_type;
        buf.index = i;

printf("[%s:%d] VIDIOC_QBUF i = %d\n", __func__, __LINE__, i);
        ret = ioctl(fd_display, VIDIOC_QBUF, &buf);
        if (ret < 0)
        {
            printf("VIDIOC_QBUF failed\n");
            display_munmap();
            return FAIL;
        }
    }
    
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
printf("[%s:%d] VIDIOC_STREAMON \n", __func__, __LINE__);
    sleep(1);
    ret = ioctl (fd_display, VIDIOC_STREAMON, &type);
    sleep(1);
    if (ret < 0)
    {
        printf("Could not start stream\n");
        display_munmap();
        return FAIL;
    }
    return PASS;

}

int capture_and_display()
{
    int ret;
    struct v4l2_buffer capture_buf;
    struct v4l2_buffer display_buf;
    int count = capture_count;
    int retval = PASS;
        
    while (count-- > 0)
    {
#ifdef CAPTURE
                memset(&capture_buf, 0, sizeof (capture_buf));
        capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        capture_buf.memory = mem_mmap_type;
        if (ioctl (fd_capture, VIDIOC_DQBUF, &capture_buf) < 0)//将已经捕获好视频的内存拉出已捕获视频的队列
        {
            printf("capture VIDIOC_DQBUF failed.\n");
            retval = FAIL;
            break;
        }
#endif
#ifdef DISPLAY
        display_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        display_buf.memory = mem_mmap_type;
printf("[%s:%d] VIDIOC_DQBUF \n", __func__, __LINE__);
        ret = ioctl(fd_display, VIDIOC_DQBUF, &display_buf);
        if (ret < 0)
        {
            printf("display VIDIOC_DQBUF failed\n");
            retval = -1;
            break;
        }
#ifndef        ONLY_DISPLAY
printf("[%s:%d] ========================================error \n", __func__, __LINE__);
                memcpy(display_buffers[display_buf.index].start,capture_buffers[capture_buf.index].start, capture_frame_size);
#endif
#endif

#ifdef         CAPTURE
                fwrite(capture_buffers[capture_buf.index].start, capture_frame_size, 1, fd_file);
#endif
#ifdef        ONLY_DISPLAY
printf("[%s:%d] VIDIOC_QUERYBUF \n", __func__, __LINE__);
                retval = fread(display_buffers[display_buf.index].start, 1, capture_frame_size, fd_file);
printf("[%s:%d] capture_frame_size = %d \n", __func__, __LINE__, capture_frame_size);
                if(retval < capture_frame_size)
                        {
                                printf("read file over!\n");
                                break;
                        }
#endif                        
#ifdef DISPLAY
printf("[%s:%d] VIDIOC_QBUF \n", __func__, __LINE__);
                if ((retval = ioctl(fd_display, VIDIOC_QBUF, &display_buf)) < 0)
        {
            printf("display VIDIOC_QBUF failed %d\n", retval);
            retval = -1;
            break;
        }
#endif
#ifdef        CAPTURE
        if (count >= TEST_BUFFER_NUM)
        {
            if (ioctl (fd_capture, VIDIOC_QBUF, &capture_buf) < 0)  //投放一个空的视频缓冲区到视频缓冲区输入队列中
            {
                printf("capture VIDIOC_QBUF failed\n");
                retval = FAIL;
                break;
            }
        }
        else
            printf("buf.index %d\n", capture_buf.index);
#endif
    }
    return retval;
}

void capture_stop(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (fd_capture, VIDIOC_STREAMOFF, &type); //结束视频显示函数
}

void display_stop(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl (fd_display, VIDIOC_STREAMOFF, &type);
}

int main(int argc, char *argv[])
{
    int idx = 0;
    int retval = PASS;
#ifdef        CAPTURE
    if((retval = capture_open())<0)
        goto err0;
#endif
#ifdef DISPLAY
    if((retval = display_open())< 0)
        goto err0;
#endif        
#ifdef        CAPTURE
    fd_file = fopen(argv[argc-1], "wb");
#else
    fd_file = fopen(argv[argc-1], "rb");
#endif
    if (fd_file == NULL)
    {        
            retval = FAIL;
            goto err0;
    }
#ifdef CAPTURE
    if((retval = capture_set())<0)
        goto err0;
#endif
#ifdef DISPLAY
    if((retval = display_set())< 0)
        goto err0;
#endif
#ifdef CAPTURE        
    if((retval = capture_mmap())< 0)
        goto err1;
#endif
#ifdef DISPLAY
    if((retval = display_mmap())< 0)
        goto err1;
#endif
#ifdef CAPTURE
    if ((retval = capture_starting()) < 0)
        goto err1;
#endif
#ifdef DISPLAY
    if((retval = display_starting())< 0)
        goto err1;
#endif

    sleep(1);
    if((retval = capture_and_display()) < 0)
        goto err1;


#ifdef CAPTURE
    capture_stop();
#endif
#ifdef DISPLAY
    display_stop();
#endif
err1:
#ifdef        CAPTURE
    capture_munmap();
#endif
#ifdef DISPLAY
    display_munmap();
#endif
    printf("*****capture_and_display_exit*******\n");
err0:
#ifdef CAPTURE
    close(fd_capture);
#endif
#ifdef DISPLAY
    close(fd_display);
#endif
    return retval;
}
