#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>		/* getopt_long() */
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>

//#include "dma_mem.h"

//#define V4L2_CID_CAM_CV_MODE (BASE_VIDIOC_PRIVATE+0)

#define V4L2_CID_CAM_CV_MODE _IOW('v', BASE_VIDIOC_PRIVATE + 0, int)
//#define CROP_BY_JACK
#define BUF_COUNT 6
#define FRAME_NUM 50

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
	IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void *start;
	size_t length;		//buffer's length is different from cap_image_size
};

static char *dev_name = NULL;
static io_method io = IO_METHOD_MMAP;	//IO_METHOD_READ;//IO_METHOD_MMAP;
static int fd = -1;
static int cfile = -1;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static FILE *outf = 0;
static unsigned int cap_image_size = 0;
static int selectflag = 1;
//to keep the real image size ! !
//////////////////////////////////////////
static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
	int r;
	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

static int change_control(int fd, int request, int val)
{
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	int ret;

	memset(&queryctrl, 0, sizeof (queryctrl));
	queryctrl.id = request;
    
	ret = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
	if (-1 == ret) {
		if (errno != EINVAL) {
			printf("error: VIDIOC_QUERYCTRL, in %s, %d\n", __func__, __LINE__);
			goto error;
		} else {
			printf("error: request[%d] is not supported\n", request);
			goto error;
		}
	} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
		printf("error: request[%d] is not supported\n", request);
		goto error;
	}
	printf("v4l2_queryctrl: \n");
	printf("name:%s, min:%d, max:%d, step:%d, default:%d\n", queryctrl.name,
		   queryctrl.minimum, queryctrl.maximum, queryctrl.step, 
		   queryctrl.default_value);
    
	memset(&control, 0, sizeof (control));
	control.id = request;
	if (0 == ioctl(fd, VIDIOC_G_CTRL, &control)) {
		printf("current value is : %d\n", control.value);
	} else if (errno != EINVAL) {
		printf("error: VIDIOC_G_CTRL failed\n");
		goto error;
	}

	// rm detect for test driver
	//if (queryctrl.minimum <= val && val <= queryctrl.maximum) {
		control.id = request;
		control.value = val;
		ret = ioctl(fd, VIDIOC_S_CTRL, &control); 
		if (ret) {
			printf("error: fail to set ctrl:req[%d]-val[%d]\n", request, val);
			goto error;
		}
	//} else {
	//	printf("error: request value %d is out of range[%d,%d]\n", val,
	//		   queryctrl.minimum, queryctrl.maximum);
	//}

    return (0);
error:
    return (-1);
}

static void set_crop(int width, int height)
{
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c.left = cropcap.defrect.left;
		crop.c.top = cropcap.defrect.top;
		crop.c.width = width;
		crop.c.height = height;

		printf("----->has ability to crop!!\n");
		printf("cropcap.defrect = (%d, %d, %d, %d)\n",
		       cropcap.defrect.left, cropcap.defrect.top,
		       cropcap.defrect.width, cropcap.defrect.height);
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
			printf("-----!!but crop to (%d, %d, %d, %d) Failed!!\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		} else {
			printf("----->sussess crop to (%d, %d, %d, %d)\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		}
	} else {
		/* Errors ignored. */
		printf("!! has no ability to crop!!\n");
	}
}

static void process_image(const void *p, int len)
{
	int filed;
	static int i = 0;

	i++;

	if ((i % (/*BUF_COUNT*/4 + 1)) != 0) {
		return;
	}
#if 0
	char fname[256] = "file";

	sprintf(fname, "%s%04d(%p).yuv", fname, i / (/*BUF_COUNT*/0 + 1), p);

	filed = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (filed < 0) {
		printf("Create file error!\n");
	}

	write(filed, p, len);

	close(filed);
#endif
  write(cfile, p, len);
}

static int read_frame(int index)
{
	struct v4l2_buffer buf;
	unsigned int i;
	//printf("read_frame %d\n", __LINE__);
	switch (io) {
	case IO_METHOD_READ:
#if 0
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				printf("again\n");
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("read");
			}
		}
//		printf("length = %d\n", buffers[0].length);
//		process_image(buffers[0].start, buffers[0].length);
		printf("image_size = %d,\t IO_METHOD_READ buffer.length=%d\n",
		       cap_image_size, buffers[0].length);
		process_image(buffers[0].start, cap_image_size);
#endif
                printf("don't support\n");
		break;
	case IO_METHOD_MMAP:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				printf("again\n");
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		assert(buf.index < n_buffers);
//		printf("index = %d\n", buf.index);
//		process_image(buffers[buf.index].start, buffers[buf.index].length);
//		printf("image_size = %d,\t IO_METHOD_MMAP buffer.length=%d\n",
//		       cap_image_size, buffers[0].length);
                printf("[%d]bufw:%d, bufh:%d, used:%d, length:%d\n", buf.index, buf.reserved >> 16, buf.reserved & 0xffff, buf.bytesused, buf.length);
		if (index == 10){
			process_image(buffers[buf.index].start, buf.bytesused);
			}
		
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;
	case IO_METHOD_USERPTR:
#if 0
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				printf("again\n");
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long)sys_get_phyaddr(buffers[i].start)
				&& buf.length <= buffers[i].length) {
				break;
			}
		assert(i < n_buffers);
		printf("buf[%d]=%p(%#lx)\n", i, buffers[i].start, buf.m.userptr);
//		printf("length = %d\n", buffers[i].length);
//		process_image((void *) buf.m.userptr, buffers[i].length);
//		printf
//		    ("image_size = %d,\t IO_METHOD_USERPTR buffer.length=%d\n",
//		     cap_image_size, buffers[0].length);
		process_image(buffers[i].start, cap_image_size);
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
#endif
                printf("don't support\n");
		break;
	}
	return 1;
}

static void mainloop(void)
{
	unsigned int count;
	int saveindex = 1;
	//count = FRAME_NUM;
	count = FRAME_NUM;
	//printf("mainLOOP %d\n", __LINE__);
	while(saveindex-->0){
		count =FRAME_NUM;
		printf("read -------index=%d\n", saveindex);
	while (count-- > 0) {
		//printf("mainLOOP ---1-count=%d, %d\n", count, __LINE__);
		if(1 == selectflag){
		for (;;) {
			//#if selectflag
			fd_set fds;
			struct timeval tv;
			int r;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			/* Timeout. */
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			r = select(fd + 1, &fds, NULL, NULL, &tv);
			//printf("mainLOOP ---r=%d, %d\n", r, __LINE__);
			if (-1 == r) {
				if (EINTR == errno)
					continue;
				errno_exit("select");
			}
			if (0 == r) {
				fprintf(stderr, "select timeout\n");
				printf("mainLOOP ----count=%d, %d\n", count, __LINE__);
				exit(EXIT_FAILURE);
			}

			//#endif
			//printf("mainLOOP %d\n", __LINE__);
			if (read_frame(count))
				break;
			/* EAGAIN - continue select loop. */
		}
			}
		else{
			read_frame(saveindex);
			}
#if 0
		if (count == FRAME_NUM - 6) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 1);
		}
		if (count == FRAME_NUM - 12) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2);
		}
		if (count == FRAME_NUM - 18) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 3);
		}
		if (count == FRAME_NUM - 24) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 4);
		}
		if (count == FRAME_NUM - 30) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 5);
		}
		if (count == FRAME_NUM - 36) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 6);
		}
		if (count == FRAME_NUM - 42) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 0);
		}
		if (count == FRAME_NUM - 48) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 1);
		}
		if (count == FRAME_NUM - 54) {
			change_control(fd, V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2);
		}
#endif
		//if (count == FRAME_NUM - 3) {
		//	set_crop(320, 240);
		//}
	}
		}
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;
	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
    close(cfile);
    cfile = -1;
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;
    static times = 0;
    char  fname[128] = "pictures";

	printf("start_capturing ---n_buffers = %d, %d\n", n_buffers, __LINE__);

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case IO_METHOD_MMAP:
		printf("start_capturing %d\n", __LINE__);
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;
			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		printf("start_capturing %d\n", __LINE__);
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	case IO_METHOD_USERPTR:
#if 0
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;
			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
//			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.m.userptr =
			    (unsigned long)sys_get_phyaddr(buffers[i].start);
			buf.length = buffers[i].length;

			if (0xFFFFFFFFUL == buf.m.userptr) {
				errno_exit("sys_get_phyaddr");
			}

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
#endif
		break;
	}

	sprintf(fname, "%s%04d.yuv", fname, times);
    cfile = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
		printf("Create file error!\n");
        errno_exit("file open failed\n");
	}
	printf("start_capturing %d\n", __LINE__);
    times++;
}

static void uninit_device(void)
{
	unsigned int i;
	switch (io) {
	case IO_METHOD_READ:
		//free(buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case IO_METHOD_USERPTR:
#if 0
		for (i = 0; i < n_buffers; ++i)
//                      free(buffers[i].start);
			if (0 != sys_mem_free(buffers[i].start))
				errno_exit("sys_mem_free");
#endif
		break;
	}
	free(buffers);
}

static void init_read(unsigned int buffer_size)
{
	buffers = calloc(1, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);
	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = BUF_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}
	buffers = calloc(req.count, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
						buf.length,
						PROT_READ | PROT_WRITE
						/* required */ ,
						MAP_SHARED /* recommended */ ,
						fd, buf.m.offset);
		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}
#if 0
static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
	CLEAR(req);
	req.count = BUF_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
	buffers = calloc(BUF_COUNT, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	for (n_buffers = 0; n_buffers < BUF_COUNT; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
//		buffers[n_buffers].start = memalign( /* boundary */ page_size,
//											buffer_size);

		buffers[n_buffers].start =
		    sys_mem_allocate(buffers[n_buffers].length,
				     MEM_CONTINUOUS | UNCACHE_MEM);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}

		printf("buffers[%d].length = %d\n", n_buffers, (unsigned int)buffers[n_buffers].length);
		printf("buffers[%d].start = %p\n", n_buffers, buffers[n_buffers].start);

		memset(buffers[n_buffers].start, 0xaa,
		       buffers[n_buffers].length);
		printf("memset complete.\n");
	}
}
#endif

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}
	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
#if 0
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
#endif
		break;
	}
	//////not all capture support crop!!!!!!!
	/* Select video input, video standard and tune here. */
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifndef CROP_BY_JACK
		crop.c = cropcap.defrect;	/* reset to default */
#else
		crop.c.left = cropcap.defrect.left;
		crop.c.top = cropcap.defrect.top;
		crop.c.width = 352;
		crop.c.height = 288;
#endif
		printf("----->has ability to crop!!\n");
		printf("cropcap.defrect = (%d, %d, %d, %d)\n",
		       cropcap.defrect.left, cropcap.defrect.top,
		       cropcap.defrect.width, cropcap.defrect.height);
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
			printf("-----!!but crop to (%d, %d, %d, %d) Failed!!\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		} else {
			printf("----->sussess crop to (%d, %d, %d, %d)\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		}
	} else {
		/* Errors ignored. */
		printf("!! has no ability to crop!!\n");
	}
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	printf("\n");
	{
	    int mode = 1;
	    xioctl(fd, V4L2_CID_CAM_CV_MODE, &mode); // preview
  }
	////////////crop finished!
	//////////set the format
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	//V4L2_PIX_FMT_YVU420, V4L2_PIX_FMT_YUV420 ！ Planar formats with 1/2 horizontal and vertical chroma resolution, also known as YUV 4:2:0
	//V4L2_PIX_FMT_YUYV ！ Packed format with 1/2 horizontal chroma resolution, also known as YUV 4:2:2
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_YUV422P; //V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	{
		printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
		printf("=====will set fmt to (%d, %d)--", fmt.fmt.pix.width,
		       fmt.fmt.pix.height);
		if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			printf("V4L2_PIX_FMT_YUYV\n");
		} else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
			printf("V4L2_PIX_FMT_YUV420\n");
		} else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			printf("V4L2_PIX_FMT_NV12\n");
		}
	}
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");
	{
		printf("=====after set fmt\n");
		printf("	fmt.fmt.pix.width = %d\n", fmt.fmt.pix.width);
		printf("	fmt.fmt.pix.height = %d\n", fmt.fmt.pix.height);
		printf("	fmt.fmt.pix.sizeimage = %d\n",
		       fmt.fmt.pix.sizeimage);
		cap_image_size = fmt.fmt.pix.sizeimage;
		printf("	fmt.fmt.pix.bytesperline = %d\n",
		       fmt.fmt.pix.bytesperline);
		printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
		printf("\n");
	}
	cap_image_size = fmt.fmt.pix.sizeimage;
	/* Note VIDIOC_S_FMT may change width and height. */
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 3 / 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	printf("After Buggy driver paranoia\n");
	printf("	>>fmt.fmt.pix.sizeimage = %d\n", fmt.fmt.pix.sizeimage);
	printf("	>>fmt.fmt.pix.bytesperline = %d\n",
	       fmt.fmt.pix.bytesperline);
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	printf("\n");
	switch (io) {
	case IO_METHOD_READ:
		//init_read(fmt.fmt.pix.sizeimage);
		break;
	case IO_METHOD_MMAP:
		init_mmap();
		break;
	case IO_METHOD_USERPTR:
		//init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}

static void reinit_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}
	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
#if 0
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
#endif
		break;
	}
	//////not all capture support crop!!!!!!!
	/* Select video input, video standard and tune here. */
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifndef CROP_BY_JACK
		crop.c = cropcap.defrect;	/* reset to default */
#else
		crop.c.left = cropcap.defrect.left;
		crop.c.top = cropcap.defrect.top;
		crop.c.width = 352;
		crop.c.height = 288;
#endif
		printf("----->has ability to crop!!\n");
		printf("cropcap.defrect = (%d, %d, %d, %d)\n",
		       cropcap.defrect.left, cropcap.defrect.top,
		       cropcap.defrect.width, cropcap.defrect.height);
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
			printf("-----!!but crop to (%d, %d, %d, %d) Failed!!\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		} else {
			printf("----->sussess crop to (%d, %d, %d, %d)\n",
			       crop.c.left, crop.c.top, crop.c.width,
			       crop.c.height);
		}
	} else {
		/* Errors ignored. */
		printf("!! has no ability to crop!!\n");
	}
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	printf("\n");
	{
	    int mode = 2;
	    xioctl(fd, V4L2_CID_CAM_CV_MODE, &mode); // video
  }
	////////////crop finished!
	//////////set the format
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;//800;
	fmt.fmt.pix.height = 480;//600;
	//V4L2_PIX_FMT_YVU420, V4L2_PIX_FMT_YUV420 ！ Planar formats with 1/2 horizontal and vertical chroma resolution, also known as YUV 4:2:0
	//V4L2_PIX_FMT_YUYV ！ Packed format with 1/2 horizontal chroma resolution, also known as YUV 4:2:2
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;	//V4L2_PIX_FMT_YUV420;//V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	{
		printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
		printf("=====will set fmt to (%d, %d)--", fmt.fmt.pix.width,
		       fmt.fmt.pix.height);
		if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			printf("V4L2_PIX_FMT_YUYV\n");
		} else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
			printf("V4L2_PIX_FMT_YUV420\n");
		} else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			printf("V4L2_PIX_FMT_NV12\n");
		} else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P) {
			printf("V4L2_PIX_FMT_YUV422P\n");
		}
	}
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");
	{
		printf("=====after set fmt\n");
		printf("	fmt.fmt.pix.width = %d\n", fmt.fmt.pix.width);
		printf("	fmt.fmt.pix.height = %d\n", fmt.fmt.pix.height);
		printf("	fmt.fmt.pix.sizeimage = %d\n",
		       fmt.fmt.pix.sizeimage);
		cap_image_size = fmt.fmt.pix.sizeimage;
		printf("	fmt.fmt.pix.bytesperline = %d\n",
		       fmt.fmt.pix.bytesperline);
		printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
		printf("\n");
	}
	cap_image_size = fmt.fmt.pix.sizeimage;
	/* Note VIDIOC_S_FMT may change width and height. */
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 3 / 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	printf("After Buggy driver paranoia\n");
	printf("	>>fmt.fmt.pix.sizeimage = %d\n", fmt.fmt.pix.sizeimage);
	printf("	>>fmt.fmt.pix.bytesperline = %d\n",
	       fmt.fmt.pix.bytesperline);
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	printf("\n");
	switch (io) {
	case IO_METHOD_READ:
		//init_read(fmt.fmt.pix.sizeimage);
		break;
	case IO_METHOD_MMAP:
		init_mmap();
		break;
	case IO_METHOD_USERPTR:
		//init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}


static void close_device(void)
{
	if (-1 == close(fd))
		errno_exit("close");
	fd = -1;
}

static void open_device(void)
{
	struct stat st;
	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name,
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}
	fd = open(dev_name, O_RDWR /* required */  | O_NONBLOCK, 0);
	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

}

static void usage(FILE * fp, int argc, char **argv)
{
	fprintf(fp, "Usage: %s [options]\n\n"
		"Options:\n"
		"-d | --device name   Video device name [/dev/video0]\n"
		"-h | --help          Print this message\n"
		"-m | --mmap          Use memory mapped buffers\n"
		/*"-r | --read          Use read() calls\n"
		"-u | --userp         Use application allocated buffers\n" */
		"", argv[0]);
}

static const char short_options[] = "d:hmrus";
static const struct option long_options[] = {
	{"device", required_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{"mmap", no_argument, NULL, 'm'},
	{"slecet", no_argument, NULL, 's'},
	//{"read", no_argument, NULL, 'r'},
	//{"userp", no_argument, NULL, 'u'},
	{0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	
	dev_name = "/dev/video1";
	outf = fopen("out.yuv", "wb");
	for (;;) {
		int index;
		int c;
		c = getopt_long(argc, argv, short_options, long_options,
				&index);
		if (-1 == c)
			break;
		switch (c) {
		case 0:	/* getopt_long() flag */
			break;
		case 'd':
			dev_name = optarg;
			break;
		case 'h':
			usage(stdout, argc, argv);
			exit(EXIT_SUCCESS);
		case 'm':
			printf("in m-----\n");
			io = IO_METHOD_MMAP;
			break;
		case 's':
			printf("in slecet-----\n");
			selectflag  = 0;
			break;
#if 0
		case 'r':
			io = IO_METHOD_READ;
			break;
		case 'u':
			io = IO_METHOD_USERPTR;
			break;
#endif
		default:
			usage(stderr, argc, argv);
			exit(EXIT_FAILURE);
		}
	}
	//printf("open dev begain-----\n");
	open_device();
	//printf("open dev end-----\n");
	init_device();
	//printf("open init dev end-----\n");
	start_capturing();
	//printf("open start_capturing end-----\n");
	mainloop();
	//printf("open mainloop end-----\n");
	//printf("\n");
	stop_capturing();
	//printf("open stop_capturing end-----\n");
#if 0
	sleep(2);
	reinit_device();
	start_capturing();
	mainloop();
	printf("\n");
	stop_capturing();
#endif
//	fclose(outf);
	uninit_device();
//printf("open uninit_device end-----\n");
	close_device();
	//printf("open close_device end-----\n");
	exit(EXIT_SUCCESS);
	return 0;
}
