#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "uvc_gadget_video.h"
#include "uvc.h"

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))
#define CLAMP(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })
#define NR_VIDEO_BUF (4)

enum {
	VIDEO_STREAM_OFF = 0,
	VIDEO_STREAM_ON,
};

struct uvc_gadget_frame_info
{
	unsigned int image;
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_gadget_format_info
{
	unsigned int fcc;
	const struct uvc_gadget_frame_info *frames;
};

static const struct uvc_gadget_frame_info uvc_gadget_frames_yuyv[] = {
	{VIDEO_IMAGE_GRAY_DEPTH,  IMG_GRAY_DEPTH_W, IMG_GRAY_DEPTH_H, { 333333, 666666, 0 }, },	//Gary+depth
	{VIDEO_IMAGE_DEPTH_YUV ,  IMG_DEPTH_YUV_W , IMG_DEPTH_YUV_H , { 333333, 666666, 0 }, },	//depth_yuv
	{VIDEO_IMAGE_DEPTH_RGB ,  IMG_DEPTH_RGB_W , IMG_DEPTH_RGB_H , { 333333, 666666, 0 }, },	//depth_rgb
	{VIDEO_IMAGE_DEPTH     ,  IMG_DEPTH_W     , IMG_DEPTH_H     , { 333333, 666666, 0 }, },	//Depth
	{VIDEO_IMAGE_CLOUD     ,  IMG_CLOUD_W     , IMG_CLOUD_H     , { 333333, 666666, 0 }, },	//Cloud
	{VIDEO_IMAGE_RAW_MONO  ,  IMG_RAW_MONO_W  , IMG_RAW_MONO_H  , { 333333, 666666, 0 }, }, //Raw Mono
	{VIDEO_IMAGE_RAW_MULTI ,  IMG_RAW_MULTI_W , IMG_RAW_MULTI_H , { 333333, 666666, 0 }, },	//Raw Dual
	{0,  0, 0, { 0, }, },
};

static const struct uvc_gadget_format_info uvc_gadget_formats[] = {
	{ V4L2_PIX_FMT_YUYV,  uvc_gadget_frames_yuyv },
};

static int xioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while ((ret < 0) && (errno == EINTR));

	return ret;
}

static inline int v4l2_subscribe_event(int fd, struct v4l2_event_subscription *sub)
{
	int ret;
	ret = xioctl(fd, VIDIOC_SUBSCRIBE_EVENT, sub);
	if (ret < 0)
		ERR("Unable to subscribe event");

	return ret;
}


static void uvc_gadget_free(struct uvc_gadget *gadget)
{
	if(gadget != NULL) {
		if(gadget->out != NULL)  {
			free(gadget->out);
			gadget->out = NULL;
		}
		if(gadget->in != NULL)  {
			free(gadget->in);
			gadget->in = NULL;
		}
		free(gadget);
		gadget = NULL;
	}
	return;
}

static struct uvc_gadget * uvc_gadget_alloc(void)
{
	struct uvc_gadget *gadget = NULL;
	unsigned int size;

	gadget = malloc(sizeof(struct uvc_gadget));
	if (gadget == NULL)
		return NULL;
	memset(gadget, 0, sizeof(struct uvc_gadget));
	
	gadget->out = malloc(sizeof(struct uvc_gadget_device));
	if (gadget->out == NULL)
		goto uvc_do_free;
	memset(gadget->out, 0, sizeof(struct uvc_gadget_device));
	
	gadget->in = malloc(sizeof(struct uvc_gadget_camera));
	if (gadget->in == NULL)
		goto uvc_do_free;
	memset(gadget->in, 0, sizeof(struct uvc_gadget_camera));
	
	return gadget;

uvc_do_free:
	ERR("mallco buf fail\n");
	uvc_gadget_free(gadget);

	return NULL;
}

static void uvc_gadget_device_close(struct uvc_gadget *gadget)
{
	if(gadget && gadget->out) {
		if(gadget->out->fd > 0) {
			close(gadget->out->fd);
			gadget->out->fd = 0;
		}
	}
	return;
}

static int uvc_gadget_device_open(struct uvc_gadget *gadget, const char *dev)
{
	struct v4l2_capability cap;
	struct stat st;
	int fd;
	int ret;

	ret = stat(dev, &st);
	if (ret < 0) {
		ERR("stat");
		return ret;
	}

	if (!S_ISCHR(st.st_mode)) {
		ERR("%s is no device\n", dev);
		return -1;
	}

	fd = open(dev, O_RDWR);
	if (fd < -1) {
		ERR("open %s fail\n", dev);
		return -1;
	}
	
	gadget->out->fd = fd;
	gadget->out->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	gadget->out->fcc = V4L2_PIX_FMT_YUYV;

	ret = xioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		ERR("get VIDIOC_QUERYCAP fail, ret = %d\n", ret);
		goto uvcdev_do_close;
	}
	DBG("'%s' is %s on bus %s\n", dev, cap.card, cap.bus_info);

	return 0;

uvcdev_do_close:
	ERR("\n");
	uvc_gadget_device_close(gadget);
	return -1;
}

static void uvc_gadget_fill_streaming_control(struct uvc_streaming_control *ctrl, int iframe, int iformat)
{
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	unsigned int nframes;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_gadget_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_gadget_formats))
		return;
	format = &uvc_gadget_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof(struct uvc_streaming_control));

	ctrl->bmHint = 1; /* dwFrameInterval */
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = frame->intervals[0];
	switch (format->fcc) {
		default:
		case V4L2_PIX_FMT_YUYV:
			ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
			break;
		case V4L2_PIX_FMT_NV12:
			ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 3 / 2;
			break;
	}
	ctrl->dwMaxPayloadTransferSize = 1024;

	ctrl->bmFramingInfo = 3; /* Support FID/EOF bit of the payload headers */
	ctrl->bPreferedVersion = 1; /* Payload header version 1.1 */
	ctrl->bMaxVersion = 1;
}

static void uvc_gadget_init_video_output(struct uvc_gadget_device *out, int iframe, int iformat)
{
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	unsigned int nframes;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_gadget_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_gadget_formats))
		return;
	format = &uvc_gadget_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	out->fcc   = format->fcc;
	out->image = frame->image;
	out->width = frame->width;
	out->height = frame->height;
	return;
}

static int uvc_gadget_events_init(struct uvc_gadget *gadget)
{
	struct v4l2_event_subscription sub;
	int ret;

	memset(&sub, 0, sizeof(struct v4l2_event_subscription));
	sub.type = UVC_EVENT_SETUP;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
	sub.type = UVC_EVENT_DATA;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
	sub.type = UVC_EVENT_STREAMON;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
	sub.type = UVC_EVENT_STREAMOFF;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
	return 0;
}

void uvc_gadget_close(struct uvc_gadget *gadget)
{
	uvc_gadget_device_close(gadget);
	uvc_gadget_free(gadget);
}

struct uvc_gadget * uvc_gadget_open(const char *dev)
{
	int ret;
	struct uvc_gadget *gadget;

	gadget = uvc_gadget_alloc();
	if(gadget == NULL) {
		ERR("malloc uvc_gadget fail\n");
		return NULL;
	}

	ret = uvc_gadget_device_open(gadget, dev);
	if(ret < 0) {
		ERR("open %s fail\n", dev);
		uvc_gadget_free(gadget);
		return NULL;
	}

	uvc_gadget_fill_streaming_control(&gadget->probe, 0, 0);
	uvc_gadget_fill_streaming_control(&gadget->commit, 0, 0);
	uvc_gadget_init_video_output(gadget->out, 0, 0);

	ret = uvc_gadget_events_init(gadget);
	if (ret < 0) {
		ERR("set event fail\n");
		uvc_gadget_close(gadget);
		return NULL;
	}
	
	return gadget;
}

static int uvc_gadget_set_format(struct uvc_gadget_device *dev, unsigned int imgsize)
{
	int ret, ret1;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = dev->type;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.pixelformat = dev->fcc;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (imgsize)
		fmt.fmt.pix.sizeimage = imgsize;

	ret = xioctl(dev->fd, VIDIOC_S_FMT, &fmt);
	return ret;
}

static void uvc_gadget_freebufs(struct uvc_gadget_device *dev)
{
	unsigned int i;
	if(dev->buf != NULL) {
		if(dev->nbufs > 0) {
			for (i = 0; i < dev->nbufs; ++i) {
				if(dev->buf[i] != NULL) {
					munmap(dev->buf[i], dev->bufsize);
					dev->buf[i] = NULL;
				}
			}
		}

		free(dev->buf);
		dev->buf = NULL;
	}

	dev->nbufs= 0;
	return;
}

static int uvc_gadget_reqbufs(struct uvc_gadget_device *dev, unsigned int nbufs)
{
	struct v4l2_requestbuffers rbuf;
	unsigned int i;
	int ret;

	uvc_gadget_freebufs(dev);
	if(nbufs == 0)
		return 0;

	memset(&rbuf, 0, sizeof(struct v4l2_requestbuffers));
	rbuf.count = nbufs;
	rbuf.type = dev->type;
	rbuf.memory = V4L2_MEMORY_MMAP;
	ret = xioctl(dev->fd, VIDIOC_REQBUFS, &rbuf);
	if (ret < 0) {
		ERR("VIDIOC_REQBUFS fail, ret = %d\n", ret);
		return ret;
	}
	
	DBG("%u buffers allocated.\n", rbuf.count);

	/* Map the buffers. */
	dev->buf = malloc(rbuf.count * sizeof(void *));
	dev->nbufs = 0;
	for (i = 0; i < rbuf.count; i++) {
		memset(&dev->v4l2_buf, 0, sizeof(struct v4l2_buffer));
		dev->v4l2_buf.index = i;
		dev->v4l2_buf.type = dev->type;
		dev->v4l2_buf.memory = V4L2_MEMORY_MMAP;

		ret = xioctl(dev->fd, VIDIOC_QUERYBUF, &dev->v4l2_buf);
		if (ret < 0) {
			ERR("[%d] VIDIOC_QUERYBUF fail, ret = %d\n", i, ret);
			uvc_gadget_freebufs(dev);
			return -1;
		}
		//DBG("length: %u offset: %u\n", dev->v4l2_buf.length, dev->v4l2_buf.m.offset);

		dev->buf[i] = mmap(0, dev->v4l2_buf.length,PROT_READ | PROT_WRITE,
				   MAP_SHARED, dev->fd, dev->v4l2_buf.m.offset);
		if (dev->buf[i] == MAP_FAILED) {
			ERR("[%d] buffer mmap fail, ret = %d\n", i, ret);
			uvc_gadget_freebufs(dev);
			return -1;
		}
		//DBG("Buffer %u mapped at address %p.\n", i, dev->buf[i]);
		dev->nbufs++;

		dev->v4l2_buf.index = i;
		dev->v4l2_buf.type = dev->type;
		dev->v4l2_buf.memory = V4L2_MEMORY_MMAP;

		ret = xioctl(dev->fd, VIDIOC_QBUF, &dev->v4l2_buf);
		if (ret < 0) {
			ERR("[%d] VIDIOC_QBUF fail, ret = %d\n", i, ret);
			uvc_gadget_freebufs(dev);
			return ret;
		}
	}

	dev->bufsize = dev->v4l2_buf.length;

	return 0;
}

static int uvc_gadget_stream(struct uvc_gadget_device *dev, int enable)
{
	int ret;

	if (enable) {
		DBG("Starting video stream.\n");
		ret = xioctl(dev->fd, VIDIOC_STREAMON, &dev->type);
	} else {
		DBG("Stopping video stream.\n");
		ret = xioctl(dev->fd, VIDIOC_STREAMOFF, &dev->type);
	}

	return ret;
}

static inline int uvc_send_response(int fd, struct uvc_request_data *resp)
{
	int ret;

	ret = xioctl(fd, UVCIOC_SEND_RESPONSE, resp);
	if (ret < 0)
		ERR("Unable to send response");

	return ret;
}

#define UVC_CT_AE_MODE_MANUAL	(1)
#define UVC_CT_AE_MODE_AUTO	(1<<1)

struct uvc_control_st videoc_it[] = {
	{
	.id = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
	.cur = 200,
	.min = 1,
	.max = 2000,
	.def = 200,
	.res = 1,
	.len = 4,
	},
	{
	.id = UVC_CT_AE_MODE_CONTROL,
	.cur = UVC_CT_AE_MODE_MANUAL,
	.min = UVC_CT_AE_MODE_MANUAL,
	.max = UVC_CT_AE_MODE_MANUAL,
	.def = UVC_CT_AE_MODE_MANUAL,
	.res = UVC_CT_AE_MODE_MANUAL,
	.len = 1,
	}
};

struct uvc_control_st videoc_pu[] = {
	{
	.id = UVC_PU_BRIGHTNESS_CONTROL,
	.cur = 0x0001,
	.min = 0x0001,
	.max = 0x0003,  //1:60M, 2:80M, 3:60M+80M
	.def = 0x0001,
	.res = 1,
	.len = 2,
	},
	{
	.id = UVC_PU_CONTRAST_CONTROL,
	.cur = 0x0028,
	.min = 0x0001,
	.max = 0x00ff,  //exposure: val*5
	.def = 0x0028,
	.res = 1,
	.len = 2,
	},
	{
	.id  = UVC_PU_SATURATION_CONTROL,
	.cur = 0x0001,
	.min = 0x0001,
	.max = 0x0002, //1: do nothing,  2: check eeprom
	.def = 0x0001,
	.res = 1,
	.len = 2,
	},
	{
	.id  = UVC_PU_SHARPNESS_CONTROL,
	.cur = 0x0001,
	.min = 0x0001,
	.max = 0x00ff,
	.def = 0x0001,
	.res = 1,
	.len = 2,
	},
	{
	.id  = UVC_PU_HUE_CONTROL,
	.cur = 0x0001,
	.min = 0x0001,
	.max = 0x0002, //1: do nothing, 2: unpack spectre
	.def = 0x0001,
	.res = 1,
	.len = 2,
	},
	{
	.id = UVC_PU_POWER_LINE_FREQUENCY_CONTROL,
	.cur = 1,
	.min = 0,
	.max = 3,
	.def = 1,
	.res = 1,
	.len = 1,
	}
};

struct uvc_control_st* uvc_gadget_videoc_get_it(unsigned int cs)
{
	unsigned int size = sizeof(videoc_it)/sizeof(videoc_it[0]);
	struct uvc_control_st *ptr = videoc_it;
	unsigned int i;
	for(i=0; i<size; i++) {
		if(cs == ptr[i].id) {
			return (struct uvc_control_st*)&ptr[i];
		}
	}
	return NULL;
}

struct uvc_control_st* uvc_gadget_videoc_get_pu(unsigned int cs)
{
	unsigned int size = sizeof(videoc_pu)/sizeof(videoc_pu[0]);
	struct uvc_control_st *ptr = videoc_pu;
	unsigned int i;
	for(i=0; i<size; i++) {
		if(cs == ptr[i].id) {
			return (struct uvc_control_st*)&ptr[i];
		}
	}
	return NULL;
}

static void printf_ctrl(struct usb_ctrlrequest *ctrl)
{
	char hybuf[256];

	hybuf[0] = '\0';
	sprintf(hybuf, "[S] %02x %02x %02x %02x %02x %02x %02x %02x"
		, ctrl->bRequestType, ctrl->bRequest
		, ctrl->wValue&0xff, (ctrl->wValue>>8)&0xff
		, ctrl->wIndex&0xff, (ctrl->wIndex>>8)&0xff
		, ctrl->wLength&0xff, (ctrl->wLength>>8)&0xff);
	
	printf("%s\n", hybuf);
	return;
}
static void printf_requ(struct uvc_request_data *resp)
{
	char hybuf[256];
	char hytmp[32];
	char *hyptr = (char *)resp->data;
	unsigned int hysize =  4 ; //resp->length;
	unsigned int hyi;

	hybuf[0] = '\0';
	hytmp[0] = '\0';
	sprintf(hybuf, "[D]");

	if(hysize > 0) {
		for(hyi=0; hyi<hysize; hyi++) {
			sprintf(hytmp, " %02x", hyptr[hyi]);
			strcat(hybuf, hytmp);
		}
	}
	printf("%s\n", hybuf);
	return;
}

static void uvc_gadget_events_process_control(struct uvc_gadget *gadget, uint8_t req,
				  uint8_t cs, uint16_t len, struct uvc_request_data *resp, struct usb_ctrlrequest *ctrl)
{
	//DBG("control request (req %02x cs %02x) ----------------------\n", req, cs);
	unsigned int size = sizeof(videoc_it)/sizeof(videoc_it[0]);
	struct uvc_control_st *ptr = videoc_it;
	unsigned int i;
	for(i=0; i<size; i++) {
		if(cs == ptr[i].id) {
			switch(req) {
			case UVC_SET_CUR:
				//DBG("UVC_SET_CUR \n");
				gadget->control = 0;
				gadget->videoc_index = ctrl->wIndex;
				gadget->videoc_value = ctrl->wValue;
				gadget->videoc_length = ctrl->wLength;
				//printf_ctrl(ctrl);
				break;
			case UVC_GET_CUR:
				//DBG("UVC_GET_CUR \n");
				resp->data[0] = ptr[i].cur & 0xff;
				resp->data[1] = (ptr[i].cur>>8) & 0xff;
				resp->data[2] = (ptr[i].cur>>16) & 0xff;
				resp->data[3] = (ptr[i].cur>>24) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_MIN:
				resp->data[0] = ptr[i].min & 0xff;
				resp->data[1] = (ptr[i].min>>8) & 0xff;
				resp->data[2] = (ptr[i].min>>16) & 0xff;
				resp->data[3] = (ptr[i].min>>24) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_MAX:
				resp->data[0] = ptr[i].max & 0xff;
				resp->data[1] = (ptr[i].max>>8) & 0xff;
				resp->data[2] = (ptr[i].max>>16) & 0xff;
				resp->data[3] = (ptr[i].max>>24) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_DEF:
				//DBG("UVC_GET_DEF \n");
				resp->data[0] = ptr[i].def & 0xff;
				resp->data[1] = (ptr[i].def>>8) & 0xff;
				resp->data[2] = (ptr[i].def>>16) & 0xff;
				resp->data[3] = (ptr[i].def>>24) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_RES:
				//DBG("UVC_GET_RES \n");
				resp->data[0] = ptr[i].res & 0xff;
				resp->data[1] = (ptr[i].res>>8) & 0xff;
				resp->data[2] = (ptr[i].res>>16) & 0xff;
				resp->data[3] = (ptr[i].res>>24) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_LEN:
				resp->data[0] = ptr[i].len & 0xff;
				resp->length = 1;
				break;
			case UVC_GET_INFO:
				resp->data[0] = 0x03; /* Supports GET/SET value request */
				resp->length = 1;
				break;
			}
			resp->length = len;
			break;
		}
	}
	if(i == size) {
		DBG("size = %d: bRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x\n"
			,size, ctrl->bRequestType, ctrl->bRequest,ctrl->wValue, ctrl->wIndex, ctrl->wLength);

		resp->length = -1;
	}
}

static void uvc_gadget_events_process_control_pu(struct uvc_gadget *gadget, uint8_t req,
				  uint8_t cs, uint16_t len, struct uvc_request_data *resp, struct usb_ctrlrequest *ctrl)
{
	//DBG("control request (req %02x cs %02x) ----------------------\n", req, cs);
	unsigned int size = sizeof(videoc_pu)/sizeof(videoc_pu[0]);
	struct uvc_control_st *ptr = videoc_pu;
	unsigned int i;
	for(i=0; i<size; i++) {
		if(cs == ptr[i].id) {
			switch(req) {
			case UVC_SET_CUR:
				//DBG("UVC_SET_CUR \n");
				gadget->control = 0;
				gadget->videoc_index = ctrl->wIndex;
				gadget->videoc_value = ctrl->wValue;
				gadget->videoc_length = ctrl->wLength;
				break;
			case UVC_GET_CUR:
				//DBG("UVC_GET_CUR \n");
				resp->data[0] = ptr[i].cur & 0xff;
				resp->data[1] = (ptr[i].cur >> 8) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_MIN:
				resp->data[0] = ptr[i].min & 0xff;
				resp->data[1] = (ptr[i].min >> 8) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_MAX:
				resp->data[0] = ptr[i].max & 0xff;
				resp->data[1] = (ptr[i].max >> 8) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_DEF:
				//DBG("UVC_GET_DEF \n");
				resp->data[0] = ptr[i].def & 0xff;
				resp->data[1] = (ptr[i].def >> 8) & 0xff;
				resp->length = ptr[i].len;
				break;
			case UVC_GET_RES:
				//DBG("UVC_GET_RES \n");
				resp->data[0] = ptr[i].res & 0xff;
				resp->length = 1;
				break;
			case UVC_GET_LEN:
				resp->data[0] = ptr[i].len & 0xff;
				resp->length = 1;
				break;
			case UVC_GET_INFO:
				resp->data[0] = 0x03; /* Supports GET/SET value request */
				resp->length = 1;
				break;
			}
			resp->length = len;
			break;
		}
	}
	if(i == size) {
		DBG("size = %d: bRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x\n"
			,size, ctrl->bRequestType, ctrl->bRequest,ctrl->wValue, ctrl->wIndex, ctrl->wLength);

		resp->length = -1;
	}
}

static void uvc_gadget_events_process_streaming(struct uvc_gadget *gadget, uint8_t req,
				    uint8_t cs, struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	//DBG("streaming request (req %02x cs %02x) +++++++++++++++++++++\n", req, cs);
	if ((cs != UVC_VS_PROBE_CONTROL) && (cs != UVC_VS_COMMIT_CONTROL))
		return;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof(struct uvc_streaming_control);

	switch (req) {
	case UVC_SET_CUR:
		gadget->control = cs;
		break;
	case UVC_GET_CUR:
		if (cs == UVC_VS_PROBE_CONTROL)
			memcpy(ctrl, &gadget->probe, sizeof(struct uvc_streaming_control));
		else
			memcpy(ctrl, &gadget->commit, sizeof(struct uvc_streaming_control));
		break;
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		uvc_gadget_fill_streaming_control(ctrl, (req == UVC_GET_MAX) ? -1 : 0, (req == UVC_GET_MAX) ? -1 : 0);
		break;
	case UVC_GET_RES:
		memset(ctrl, 0, sizeof(struct uvc_streaming_control));
		break;
	case UVC_GET_LEN:
		resp->data[0] = 0x00;
		resp->data[1] = sizeof(struct uvc_streaming_control);
		resp->length = 2;
		break;
	case UVC_GET_INFO:
		resp->data[0] = 0x03; /* Supports GET/SET value request */
		resp->length = 1;
		break;
	}
}

static void uvc_gadget_events_process_class(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{

	unsigned int idx;
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) {
		return;
	}

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		//DBG("UVC_INTF_CONTROL \n");
		idx = ctrl->wIndex >> 8;
		switch(idx) {
		case 1://IT
			uvc_gadget_events_process_control(gadget, ctrl->bRequest, ctrl->wValue >> 8, ctrl->wLength, resp, ctrl);
		break;

		case 2://PU
			uvc_gadget_events_process_control_pu(gadget, ctrl->bRequest, ctrl->wValue >> 8, ctrl->wLength, resp, ctrl);
		break;

		default:
			DBG("err: bRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
			ctrl->wValue, ctrl->wIndex, ctrl->wLength);

		}
		break;
	case UVC_INTF_STREAMING:
		//DBG("UVC_INTF_STREAMING \n");
		uvc_gadget_events_process_streaming(gadget, ctrl->bRequest, ctrl->wValue >> 8, resp);
		break;
	default:
		break;
	}
}

static void uvc_gadget_events_process_standard(struct uvc_gadget *gadget,
				   struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	ERR("standard request\n");
	(void)gadget;
	(void)ctrl;
	(void)resp;
}

static void uvc_gadget_events_process_setup(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	gadget->control = 0;

	//DBG("bRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
	//	ctrl->wValue, ctrl->wIndex, ctrl->wLength);
	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		DBG("USB_TYPE_STANDARD\n");
		uvc_gadget_events_process_standard(gadget, ctrl, resp);
		break;
	case USB_TYPE_CLASS:
		//DBG("USB_TYPE_CLASS\n");
		uvc_gadget_events_process_class(gadget, ctrl, resp);
		break;
	default:
		break;
	}
}

static void uvc_gadget_events_process_data_videoc_it(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	unsigned int value = (gadget->videoc_value>>8)&0xff;
	unsigned int size = sizeof(videoc_it)/sizeof(videoc_it[0]);
	struct uvc_control_st *ptr = videoc_it;
	unsigned int i;
	int j;
	for(i=0; i<size; i++) {
		if(value == ptr[i].id) {
			ptr[i].cur = 0;
			for(j=0; j<gadget->videoc_length;j++) {
				ptr[i].cur |= (data->data[j] << 8*j);
			}
			break;
		}
	}
	if(i == size) {
		ERR("unknown videoc control, index = %d, value = %d, length = %d\n"
			,gadget->videoc_index, gadget->videoc_value ,gadget->videoc_length);
		return;
	}

	return;
}

static void uvc_gadget_events_process_data_videoc_pu(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	unsigned int value = (gadget->videoc_value>>8)&0xff;
	unsigned int size = sizeof(videoc_pu)/sizeof(videoc_pu[0]);
	struct uvc_control_st *ptr = videoc_pu;
	unsigned int i;
	int j;
	for(i=0; i<size; i++) {
		if(value == ptr[i].id) {
			ptr[i].cur = 0;
			for(j=0; j<gadget->videoc_length;j++) {
				ptr[i].cur |= (data->data[j] << 8*j);
			}
			break;
		}
	}
	if(i == size) {
		ERR("unknown videoc control, index = %d, value = %d, length = %d\n"
			,gadget->videoc_index, gadget->videoc_value ,gadget->videoc_length);
		return;
	}

	return;
}

static void uvc_gadget_events_process_data_videoc(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	unsigned int idx = (gadget->videoc_index>>8)&0xff;
	switch (idx) {
	case 1://IT
		uvc_gadget_events_process_data_videoc_it(gadget, data);
	break;

	case 2://pu
		uvc_gadget_events_process_data_videoc_pu(gadget, data);
	break;

	default:
		ERR("setting unknown control, length = %d, gadget->videoc_index = %d\n", data->length, gadget->videoc_index);
		printf_requ(data);
	}
	return;
}


static video_event_st uvc_gadget_events_process_data(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;
	video_event_st video_state = VIDEO_EVENT_DATA_STREAM;

	switch (gadget->control) {
	case UVC_VS_PROBE_CONTROL:
		//DBG("setting probe control, length = %d\n", data->length);
		target = &gadget->probe;
		break;
	case UVC_VS_COMMIT_CONTROL:
		//DBG("setting commit control, length = %d\n", data->length);
		target = &gadget->commit;
		break;
	default:
		//ERR("setting  control, length = %d, gadget->control = %d\n", data->length, gadget->control);
		uvc_gadget_events_process_data_videoc(gadget, data);
		video_state = VIDEO_EVENT_DATA_CONTROL;
		//printf_requ(data);
		return video_state;
	}

	ctrl = (struct uvc_streaming_control *)&data->data;

	iformat = CLAMP((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_gadget_formats));
	format = &uvc_gadget_formats[iformat - 1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		nframes++;

	iframe = CLAMP((unsigned int)ctrl->bFrameIndex, (unsigned int)1U, nframes);
	frame = &format->frames[iframe - 1];

	interval = frame->intervals;
	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		interval++;

	target->bFormatIndex = iformat;
	target->bFrameIndex = iframe;
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_NV12:
		target->dwMaxVideoFrameSize = frame->width * frame->height * 3 / 2;
		break;
	}
	target->dwFrameInterval = *interval;
	//DBG("iframe = %d, ctrl->bFrameIndex = %d, target->dwFrameInterval = %d\n", iframe, ctrl->bFrameIndex, target->dwFrameInterval);
	if (gadget->control == UVC_VS_COMMIT_CONTROL) {
		gadget->out->fcc = format->fcc;
		gadget->out->image = frame->image;
		gadget->out->width = frame->width;
		gadget->out->height = frame->height;
		uvc_gadget_set_format(gadget->out, 0);
	}
	return video_state;
}

video_event_st uvc_gadget_events_process(struct uvc_gadget *gadget)
{
	struct v4l2_event v4l2_event;
	char *tmpptr = (char *)&v4l2_event+sizeof(__u32);
	struct uvc_event *uvc_event = (struct uvc_event *)tmpptr;
	struct uvc_request_data resp;
	int ret;
	video_event_st video_state = VIDEO_EVENT_STATE_OK;

	ret = xioctl(gadget->out->fd, VIDIOC_DQEVENT, &v4l2_event);
	if (ret < 0) {
		ERR("get VIDIOC_DQEVENT fail \n");
		return VIDEO_EVENT_STATE_ERR;
	}

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
	case UVC_EVENT_DISCONNECT:
		break;
	case UVC_EVENT_SETUP:
		//DBG("UVC_EVENT_SETUP \n");
		memset(&resp, 0, sizeof(struct uvc_request_data));
		resp.length = -EL2HLT;
		uvc_gadget_events_process_setup(gadget, &uvc_event->req, &resp);
		uvc_send_response(gadget->out->fd, &resp);
		break;
	case UVC_EVENT_DATA:
		//DBG("UVC_EVENT_DATA \n");
		video_state = uvc_gadget_events_process_data(gadget, &uvc_event->data);
		break;
	case UVC_EVENT_STREAMON:
		//DBG("UVC_EVENT_STREAMON \n");
		uvc_gadget_reqbufs(gadget->out, NR_VIDEO_BUF);
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_ON);
		video_state = VIDEO_EVENT_STREAM_ON;
		break;
	case UVC_EVENT_STREAMOFF:
		//DBG("UVC_EVENT_STREAMOFF \n");
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_OFF);
		uvc_gadget_reqbufs(gadget->out, 0);
		video_state = VIDEO_EVENT_STREAM_OFF;
		break;
	default:
		DBG("v4l2_event.type = %d --------------------------------\n", v4l2_event.type);

	}
	return video_state;
}

int uvc_gadget_video_process(struct uvc_gadget *gadget , char *imgptr, unsigned int imgsize)
{
	struct v4l2_buffer outbuf;
	int ret;
	uint8_t *out;

	if(imgptr == NULL) {
		ERR("imgptr is NULL,  wxh=%dx%d, size = %d\n", gadget->out->width,  gadget->out->height, imgsize);
		return -1;
	}

	memset(&outbuf, 0, sizeof(struct v4l2_buffer));
	outbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	outbuf.memory = V4L2_MEMORY_MMAP;
	ret = xioctl(gadget->out->fd, VIDIOC_DQBUF, &outbuf);
	if (ret < 0) {
		ERR("Unable to dequeue buffer\n");
		return ret;
	}

	//printf("fill the buffer: YUYV: %d, wxh=%d x %d, size = %d\n", outbuf.index,  gadget->out->width,  gadget->out->height, imgsize);
	out = gadget->out->buf[outbuf.index];
        memcpy(out, imgptr, imgsize);
	outbuf.bytesused = imgsize;

	ret = xioctl(gadget->out->fd, VIDIOC_QBUF, &outbuf);
	if (ret < 0) {
		ERR("Unable to queue buffer\n");
		return ret;
	}
	return 0;
}

int uvc_gadget_video_process2(struct uvc_gadget *gadget , char *imgptr1, unsigned int imgsize1, char *imgptr2, unsigned int imgsize2)
{
	struct v4l2_buffer outbuf;
	int ret;
	uint8_t *out;

	if(imgptr1 == NULL || imgptr2 == NULL) {
		ERR("imgptr is NULL,  wxh=%dx%d, size1 = %d, size2 = %d\n", gadget->out->width,  gadget->out->height, imgsize1, imgsize2);
		return -1;
	}

	memset(&outbuf, 0, sizeof(struct v4l2_buffer));
	outbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	outbuf.memory = V4L2_MEMORY_MMAP;
	ret = xioctl(gadget->out->fd, VIDIOC_DQBUF, &outbuf);
	if (ret < 0) {
		ERR("Unable to dequeue buffer\n");
		return ret;
	}

	//printf("fill the buffer: YUYV: %d, wxh=%d x %d, size = %d+%d\n", outbuf.index,  gadget->out->width,  gadget->out->height, imgsize1, imgsize2);
	out = gadget->out->buf[outbuf.index];
        memcpy(out, imgptr1, imgsize1);
	outbuf.bytesused = imgsize1;

        memcpy(out+imgsize1, imgptr2, imgsize2);
	outbuf.bytesused += imgsize2;

	ret = xioctl(gadget->out->fd, VIDIOC_QBUF, &outbuf);
	if (ret < 0) {
		ERR("Unable to queue buffer\n");
		return ret;
	}
	return 0;
}
