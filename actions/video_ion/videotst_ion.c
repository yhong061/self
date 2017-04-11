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

#define DBG(fmt, ...)  printf("[video_ion][%d]" fmt, __LINE__, ## __VA_ARGS__)

#define CAMERA_DEVICE "/dev/video0"
#define CAPTURE_FILE "frame.yuv"
#define FRAME_RATE	(5)
#define FRAME_WIDTH	(1920)
#define FRAME_HEIGTH	(1080)

#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define BUFFER_COUNT 3

#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

#define USER_ION_H
#ifdef USER_ION_H
struct ion_handle;
enum {
	//OWL_ION_GET_PHY = 0,
	ASOC_ION_GET_PHY = 0,
};

struct asoc_ion_phys_data {
        struct ion_handle *handle;
        unsigned long phys_addr;
        size_t size;
};

struct ion_handle_data {
        struct ion_handle *handle;
};

struct ion_fd_data {
        struct ion_handle *handle;
        int fd; 
};

struct ion_custom_data {
        unsigned int cmd;
        unsigned long arg;
};

struct ion_allocation_data {
        size_t len;
        size_t align;
        unsigned int heap_id_mask;
        unsigned int flags;
        struct ion_handle *handle;
};

#define ION_IOC_MAGIC           'I'
#define ION_IOC_ALLOC           _IOWR(ION_IOC_MAGIC, 0, \
                                      struct ion_allocation_data)
#define ION_IOC_FREE            _IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP             _IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_CUSTOM          _IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)
#endif

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



int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                DBG("open /dev/ion failed!\n");
        return fd;
}

int ion_close(int fd)
{
        return close(fd);
}

static int ion_ioctl(int fd, int req, void *arg)
{
        int ret = ioctl(fd, req, arg);
        if (ret < 0) {
                DBG("ioctl %x failed with code %d: %s\n", req,
                       ret, strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
	      unsigned int flags, struct ion_handle **handle)
{
        int ret;
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
		.heap_id_mask = heap_mask, //ActionsCode(authro:songzhining, comment: update ion.h from kernel)
                .flags = flags,
        };

        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret < 0)
                return ret;
        *handle = (struct ion_handle*)data.handle;
        return ret;
}

int ion_free(int fd, struct ion_handle *handle)
{
        struct ion_handle_data data = {
                .handle = handle,
        };
        return ion_ioctl(fd, ION_IOC_FREE, &data);
}

int ion_map(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = handle,
        };

        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0)
                return ret;
        *map_fd = data.fd;
        if (*map_fd < 0) {
                DBG("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                DBG("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}


int ion_phys(int fd, struct ion_handle *handle, unsigned long *phys)
{
	      int ret; 
        struct asoc_ion_phys_data phys_data = {
                .handle = handle,
        };

        struct ion_custom_data data = {
                .cmd = ASOC_ION_GET_PHY,
                .arg = (unsigned long)&phys_data,
        };
        
        ret = ion_ioctl(fd, ION_IOC_CUSTOM, &data);

        if (ret < 0)
            return ret;
        *phys = phys_data.phys_addr;
        return ret;
}


static unsigned int sCount = 0;
static int sFd = -1;

int system_mmap(unsigned int size, int prot, int flag, struct ion_handle **handle, unsigned char **vir, int *phy)
{

	int ret;
	int map_fd = -1;

	if(sFd < 0) {
		sFd  = ion_open();
		if(sFd < 0) {
			DBG("ion open fail, return \n");
			return sFd;
		}
	}
	
	ret = ion_alloc(sFd, size, 0, 1, 3, handle);
	if(ret < 0) {
		DBG("ion alloc fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_map(sFd, *handle, size, prot, flag, 0, vir, &map_fd);
	if(ret < 0) {
		DBG("ion map fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_phys(sFd, *handle, (unsigned long *)phy);
	if(ret < 0) {
		DBG("ion phys fail: ret = %d\n", ret);
	}
	sCount++;

	return map_fd;
}

void system_munmap(struct ion_handle *handle, unsigned int size, void *vir, int *phy, int map_fd)
{
	int ret = 0;

	if(sFd < 0)
		return;

	if(sCount) {
//		free((void *)phy);
		munmap(vir, size);
		close(map_fd);
		ion_free(sFd, handle);
		sCount--;
	}

	if(sCount == 0) {
		ion_close(sFd);
		sFd = -1;
	}

	return;
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
		DBG("(size-1280*960)=%d\n",size-1280*960);
	}

	map_fd = system_mmap(size, prot, flags, &handle, (unsigned char **)&vir, &phy);
	if(map_fd < 0) {
		DBG("system map fail: map_fd = %d\n", map_fd);
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

int camera_getimageinfo(void)
{
	int ret = 0;
	struct v4l2_format fmt;
	char fmtstr[8];

	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		DBG("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

	DBG(" type: %d\n", fmt.type);
	DBG(" width: %d\n", fmt.fmt.pix.width);
	DBG(" height: %d\n", fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	DBG(" pixelformat: %s\n", fmtstr);
	DBG(" field: %d\n", fmt.fmt.pix.field);
	DBG(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	DBG(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
	DBG(" colorspace: %d\n", fmt.fmt.pix.colorspace);
	DBG(" priv: %d\n", fmt.fmt.pix.priv);
	DBG(" raw_date: %s\n\n", fmt.fmt.raw_data);

#if 0
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 224;
	fmt.fmt.pix.height      = 865;
	ret = ioctl(gFd, VIDIOC_S_FMT, &fmt);
	if(ret) {
		DBG("[%d]fail: VIDIOC_S_FMT\n", __LINE__);
		return -1;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		DBG("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}

	DBG("=============Stream Format Informations:==============\n");
	DBG(" type: %d\n", fmt.type);
	DBG(" width: %d\n", fmt.fmt.pix.width);
	DBG(" height: %d\n", fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	DBG(" pixelformat: %s\n", fmtstr);
	DBG(" field: %d\n", fmt.fmt.pix.field);
	DBG(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	DBG(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
	DBG(" colorspace: %d\n", fmt.fmt.pix.colorspace);
	DBG(" priv: %d\n", fmt.fmt.pix.priv);
	DBG(" raw_date: %s\n\n", fmt.fmt.raw_data);
#endif
	return 0;
}


int camera_S_FMT(unsigned int w, unsigned int h, unsigned int pixfmt)
{
	int ret = 0;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(gFd, VIDIOC_G_FMT, &fmt);
	if(ret) {
		DBG("[%d]fail: VIDIOC_G_FMT\n", __LINE__);
		return -1;
	}
	
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = w;
	fmt.fmt.pix.height      = h;
	fmt.fmt.pix.pixelformat = pixfmt;
	ret = ioctl(gFd, VIDIOC_S_FMT, &fmt);
	if(ret) {
		DBG("[%d]fail: VIDIOC_S_FMT\n", __LINE__);
		return -1;
	}

	return 0;
}

int camera_open(char *filename)
{
    int ret;
    unsigned int i;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers reqbuf;
    unsigned int width = FRAME_WIDTH;
    unsigned int hight = FRAME_HEIGTH;
    unsigned int hight_16bytes_align = (((hight+15)/16)*16);
    
    unsigned int size = width*hight_16bytes_align*2;

    //gFd = open(filename, O_RDWR | O_NONBLOCK, 0);
    gFd = open(filename, O_RDWR, 0);
    if(gFd < 0) {
    	DBG("open %s fail\n", filename);
	return -1;
    }
 
    camera_getimageinfo();
    //camera_S_FMT(width, hight, V4L2_PIX_FMT_YVU420);

    memset(framebuf, 0, sizeof(struct ion_buf)*BUFFER_COUNT);
    for(i=0; i<BUFFER_COUNT; i++) {
        ret = ion_buf_alloc((struct ion_buf *)&framebuf[i], size);
	if(ret < 0) {
	    DBG("ion buf alloc fail\n");
	    return 0;
	}
    }

    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_USERPTR; 
    ret = ioctl(gFd , VIDIOC_REQBUFS, &reqbuf);
    if(ret) {
        DBG("[%d]fail: VIDIOC_REQBUFS\n", __LINE__);
	return -1;
    }

    DBG("reqbuf.count = %d,BUFFER_COUNT = %d\n", reqbuf.count, BUFFER_COUNT);
    for (i = 0; i < reqbuf.count; i++) 
    {
    	memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        ret = ioctl(gFd , VIDIOC_QUERYBUF, &buf);  
	if(ret) {
	    DBG("[%d]fail: VIDIOC_QUERYBUF\n", __LINE__);
	    return -1;    
	}

	buf.m.userptr = (unsigned long)framebuf[i].phy;
	buf.length = framebuf[i].size;

        // Queen buffer
        ret = ioctl(gFd , VIDIOC_QBUF, &buf); 
	if(ret)
	    DBG("[%d]fail: VIDIOC_QBUF, errno = %d\n", __LINE__, errno);

        DBG("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].phy, framebuf[i].size);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(gFd, VIDIOC_STREAMON, &type);
    if(ret) {
        DBG("[%d]fail: VIDIOC_STREAMON\n", __LINE__);
	return -1;
    }

    return 0;
}

int camera_close(void)
{
    int ret;
    int i;

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(gFd, VIDIOC_STREAMOFF, &type);
    if(ret) {
        DBG("[%d]fail: VIDIOC_STREAMOFF\n", __LINE__);
	return -1;
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

    DBG("Camera test Done.\n");

    return 0;

}


int skip = 5;
int camera_getbuf(char *p, unsigned int size)
{
    char filename[64];
    int ret;
    struct v4l2_buffer buf;

    buf.index = gIndex;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    // Get frame
    ret = ioctl(gFd, VIDIOC_DQBUF, &buf);   
    if(ret) {
        DBG("[%d]fail: VIDIOC_DQBUF: errno = %d\n", __LINE__, errno);
        return -1;
    }

    // Process the frame
    size = buf.length;
    //memcpy(p, framebuf[buf.index].vir, size);
    //sleep(1);
    if(skip > 5)
    	skip = 5;
    skip--;
    static int hyi = 0;
    if(skip == 0) {
                sprintf(filename, "%d%s", hyi++, CAPTURE_FILE);
                FILE *fp = fopen(filename, "wb");
                size_t write_size = fwrite(framebuf[buf.index].vir, 1, size, fp);
                fclose(fp);
                DBG("Capture one frame saved in %s, weite size = %d, size = %d\n", filename, write_size, size);
		skip = 5;
    }

    // Re-queen buffer
    ret = ioctl(gFd, VIDIOC_QBUF, &buf);
    if(ret) {
        DBG("[%d]fail: VIDIOC_QBUF\n", __LINE__);
        return -1;	
    }
    gIndex++;
    if(gIndex >= BUFFER_COUNT)
        gIndex = 0;

    return 0;
}


int test_framebuf(int sec, int save_file)
{
    char filename[32];
    int frame = FRAME_RATE;
    int i;
    int ret = 0;
    unsigned int size = FRAME_WIDTH * FRAME_HEIGTH * 2;
    char *buf = malloc(size);
    struct timeval cur;

    if(!buf) {
    	DBG("malloc buf fail\n");
	return -1;
    }

    if(sec > 0)
        frame = FRAME_RATE*sec;

    //get frame data
    for(i=0; i<frame; i++) {
        ret = camera_getbuf(buf, size);
        if(ret < 0) {
	    goto framebuf_end;
        }

	if(i%FRAME_RATE == 0) {
		gettimeofday(&cur, NULL);
        	DBG("[%10u.%06u] frame = %d\n", (unsigned int)cur.tv_sec, (unsigned int)cur.tv_usec, i);
	}
	
    	if(save_file== 1 && i%FRAME_RATE == 0)  {
#if 0	
                sprintf(filename, "%d%s", i, CAPTURE_FILE);
                FILE *fp = fopen(filename, "wb");
                fwrite(buf, 1, size, fp);
                fclose(fp);
                DBG("Capture one frame saved in %s\n", filename);    
#endif
}
    	else if(save_file== 2)  {
                sprintf(filename, "file_%s", CAPTURE_FILE);
                FILE *fp = fopen(filename, "a+");
                fwrite(buf, 1, size, fp);
                fclose(fp);
                //DBG("Capture one frame saved in %s\n", filename);    
    	}
    }
framebuf_end:
    if(buf) {
        free(buf);
	buf = NULL;
    }
    return ret;
}

void usage_help(void)
{
    DBG("===========================================================\n");
    DBG("video_main data sec              : get frame data for sec, such as 1 sec \n");
    DBG("video_main yuv sec               : save yuv image: 1 sec \n");
    DBG("video_main file sec               : save yuv image: 1 sec \n");
    DBG("===========================================================\n");
}

int main(int argc, char **argv)
{
    int ret;
    if(argc < 3) {
        usage_help();
	return 0;
    }    

    ret = camera_open(CAMERA_DEVICE);
    if(ret < 0) {
	return -1;
    }

sleep(1);DBG("\n\n");
    if(strcmp(argv[1], "data") == 0)  {
	int sec = atoi(argv[2]);
	int save_file = 0;
	DBG("sec = %d\n", sec);

        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "yuv") == 0)  {
	int sec = atoi(argv[2]);
	int save_file = 1;

        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "file") == 0)  {
	int sec = atoi(argv[2]);
	int save_file = 2;

        test_framebuf(sec, save_file);
    }

main_end:
    camera_close();

    return 0;
}

