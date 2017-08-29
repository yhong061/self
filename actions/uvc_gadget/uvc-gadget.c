/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/videodev2.h>
#include <linux/usbdevice_fs.h>

#include "uvc.h"

#define DEFAULT_CAPTURE_DEVICE	"/dev/video0"
#define DEFAULT_OUTPUT_DEVICE	"/dev/video1"

#define DEBUG
#if defined(DEBUG)
#define DPRINTF(args...) printf(args)
#else
#define DPRINTF(args...)
#endif

#define CLAMP(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))

#define NR_VIDEO_BUF (4)

enum {
	VIDEO_STREAM_OFF = 0,
	VIDEO_STREAM_ON,
};

struct uvc_gadget_device {
	enum v4l2_buf_type type;
	int fd;
	struct v4l2_buffer v4l2_buf;

	unsigned int fcc;
	unsigned int width;
	unsigned int height;

	void **buf;
	unsigned int nbufs;
	unsigned int bufsize;
};

struct uvc_gadget {
	struct uvc_gadget_device *out;

	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;

	int control;

	/* YUYV source */
	uint8_t color;

	/* NV12 source */
	struct uvc_gadget_device *in;

	/* MJPEG source */
	void *imgdata;
	unsigned int imgsize;
};

/* ---------------------------------------------------------------------------
 * systemcall wrapper
 */
static int
xioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while ((ret < 0) && (errno == EINTR));

	return ret;
}

static ssize_t
xread(int fd, void *buf, size_t count) {
        char *p = buf;
        ssize_t out = 0;
        ssize_t ret;

        while (count) {
                ret = read(fd, p, count);
                if (ret < 0) {
                        if (errno == EINTR || errno == EAGAIN)
                                continue;
                        return out ? out : -1;
                } else if (ret == 0) {
                        return out;
                }

                p += ret;
                out += ret;
                count -= ret;
        }

        return out;
}

/* ---------------------------------------------------------------------------
 * ioctrl wrapper
 */
static inline int
v4l2_dequeue_buf(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_DQBUF, buf);
	if (ret < 0)
		perror("Unable to dequeue buffer");

	return ret;
}

static inline int
v4l2_queue_buf(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QBUF, buf);
	if (ret < 0)
		perror("Unable to queue buffer");

	return ret;
}

static int
v4l2_dequeue_latest_buf(int fd, struct v4l2_buffer *buf)
{
	struct v4l2_buffer tmp[NR_VIDEO_BUF];
	int count;
	int ret;

	for (count = 0; count < NR_VIDEO_BUF; count++) {

		memcpy(&tmp[count], buf, sizeof(struct v4l2_buffer));
		ret = xioctl(fd, VIDIOC_DQBUF, &tmp[count]);
		if (ret < 0)
		{
		    printf("v4l2_dequeue_latest_buf 2\n");
			break;
		}
		if (count > 0) {
			ret = v4l2_queue_buf(fd, &tmp[count - 1]);
			if (ret < 0)
			{
			    printf("v4l2_dequeue_latest_buf 3\n");
				return ret;
			}
		}
	}

	if (count) {
		memcpy(buf, &tmp[count - 1], sizeof(struct v4l2_buffer));
		return 0;
	}


    printf("v4l2_dequeue_latest_buf 4\n");
	return ret;
}

static inline int
v4l2_dequeue_event(int fd, struct v4l2_event *event)
{
	int ret;

	ret = xioctl(fd, VIDIOC_DQEVENT, event);
	if (ret < 0)
		perror("Unable to dequeue event");

	return ret;
}

static inline int
v4l2_request_buffer(int fd, struct v4l2_requestbuffers *rbuf)
{
	int ret;
	unsigned int nbufs;

	nbufs = rbuf->count;

	ret = xioctl(fd, VIDIOC_REQBUFS, rbuf);
	if (ret < 0)
		perror("Unable to send response");

	if (rbuf->count != nbufs) {
		fprintf(stderr, "rbuf->count: %d nbufs: %d.\n", rbuf->count, nbufs);
		ret = -1;
	}

	return ret;
}

static inline int
v4l2_query_buffer(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QUERYBUF, buf);
	if (ret < 0)
		perror("Unable to query buffer");

	return ret;
}

static inline int
v4l2_query_capture(int fd, struct v4l2_capability *cap)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QUERYCAP, cap);
	if (ret < 0)
		perror("Unable to query capture");

	return ret;
}

static inline int
v4l2_subscribe_event(int fd, struct v4l2_event_subscription *sub)
{
	int ret;

	ret = xioctl(fd, VIDIOC_SUBSCRIBE_EVENT, sub);
	if (ret < 0)
		perror("Unable to subscribe event");

	return ret;
}

static inline int
v4l2_stream_on(int fd, int type)
{
	int ret;

	ret = xioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
		perror("Unable to start video stream 232323");

	return ret;
}

static inline int
v4l2_stream_off(int fd, int type)
{
	int ret;

	ret = xioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0)
		perror("Unable to stop video stream");

	return ret;
}

static inline int
v4l2_set_format(int fd, struct v4l2_format *fmt)
{
	int ret;

	ret = xioctl(fd, VIDIOC_S_FMT, fmt);
	if (ret < 0)
		perror("Unable to set format");

	return ret;
}

static inline int
uvc_send_response(int fd, struct uvc_request_data *resp)
{
	int ret;

	ret = xioctl(fd, UVCIOC_SEND_RESPONSE, resp);
	if (ret < 0)
		perror("Unable to send response");

	return ret;
}

/* ---------------------------------------------------------------------------
 * Video streaming
 */
static int
uvc_gadget_fill_buffer(struct uvc_gadget *gadget, struct v4l2_buffer *outbuf)
{
	struct v4l2_buffer inbuf;
	uint8_t *out = gadget->out->buf[outbuf->index];
	uint8_t *in;
	unsigned int width, height;
	unsigned int bpl;
	unsigned int size;
	unsigned int i;
	int ret;
	static int count = 0;

	width = gadget->out->width;
	height = gadget->out->height;

	//printf("fill the buffer\n");

	switch (gadget->out->fcc) {
	case V4L2_PIX_FMT_YUYV:

		size = width * height * 2;
		bpl = width * 2;

		//size = width * height * 3 / 2;

		memset(&inbuf, 0, sizeof(struct v4l2_buffer));
		inbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		inbuf.memory = V4L2_MEMORY_MMAP;
		ret = v4l2_dequeue_latest_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;

		in = gadget->in->buf[inbuf.index];
		memcpy(out, in, size);

		ret = v4l2_queue_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;
		break;


#if 1
        //内存空间出队列

        ret = ioctl(gadget->in->fd, VIDIOC_DQBUF, &gadget->in->v4l2_buf);
        if (ret < 0)
         {
            //printf("VIDIOC_DQBUF1 failed (%d)\n", ret);
            return ret;
         }


        //in = gadget->in->buf[0];
        //printf("1");
        //for (i = 0; i < 4; i++)
        //{
          //  if (gadget->in->buf[i] != NULL)
          //  {
                count = gadget->in->v4l2_buf.index;
                count = count -1;
                if (count < 0)
                {
                    count = 3;
                }


                printf("get a buffer :id=%d outbuf id = %d\n", count, outbuf->index);

              // count ++;
             //  if (count > 3)
              // {
             //       count = 0;
               //}
                in = gadget->in->buf[count];
                memcpy(out, in, size);
                break;
#if 0
            for (i=0; i < size; i++)
            {
                if (in[i] != 0)
                {
                    printf("%2x ", in[i]);
                }
            }

		    for (i = 0; i < height; ++i)
			memset(out + (i * bpl), gadget->color++, bpl);
#endif

                //break;
           // }
       // }




 #if 1
         // 内存重新入队列
        ret = ioctl(gadget->in->fd, VIDIOC_QBUF, &gadget->in->v4l2_buf);
        if (ret < 0)
          {
            printf("VIDIOC_QBUF failed2 (%d)\n", ret);
            return ret;
          }
        #endif

        break;
#endif

	    #if 0
		ret = v4l2_dequeue_latest_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
		{
		    printf("get lastest buffer error\n");
			return ret;
        }
		in = gadget->in->buf[inbuf.index];
		memcpy(out, in, size);
		ret = v4l2_queue_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
		{
		    printf("v4l2 queue buffer error\n");
			return ret;
        }
        #endif


		//break;
	case V4L2_PIX_FMT_NV12:
		size = width * height * 3 / 2;

		memset(&inbuf, 0, sizeof(struct v4l2_buffer));
		inbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		inbuf.memory = V4L2_MEMORY_MMAP;
		ret = v4l2_dequeue_latest_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;

		in = gadget->in->buf[inbuf.index];
		memcpy(out, in, size);

		ret = v4l2_queue_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;
		break;
	case V4L2_PIX_FMT_MJPEG:
		size = gadget->imgsize;
		memcpy(out, gadget->imgdata, size);
		break;
	default:
		size = 0;
		break;
	}

	outbuf->bytesused = size;

	return 0;
}

static int
uvc_gadget_video_process(struct uvc_gadget *gadget)
{
	struct v4l2_buffer outbuf;
	int ret;

	memset(&outbuf, 0, sizeof(struct v4l2_buffer));
	outbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	outbuf.memory = V4L2_MEMORY_MMAP;
	ret = v4l2_dequeue_buf(gadget->out->fd, &outbuf);
	if (ret < 0)
		return ret;

	uvc_gadget_fill_buffer(gadget, &outbuf);

	ret = v4l2_queue_buf(gadget->out->fd, &outbuf);
	if (ret < 0)
		return ret;

	return 0;
}


static int
uvc_gadget_reqbufs(struct uvc_gadget_device *dev, unsigned int nbufs)
{
	struct v4l2_requestbuffers rbuf;
	//struct v4l2_buffer buf;
	unsigned int i;
	int ret;

	for (i = 0; i < dev->nbufs; ++i)
		munmap(dev->buf[i], dev->bufsize);

	free(dev->buf);
	dev->buf = NULL;
	dev->nbufs = 0;

	memset(&rbuf, 0, sizeof(struct v4l2_requestbuffers));
	rbuf.count = nbufs;
	rbuf.type = dev->type;
	rbuf.memory = V4L2_MEMORY_MMAP;
	ret = v4l2_request_buffer(dev->fd, &rbuf);
	if (ret < 0)
		return ret;

	DPRINTF("%u buffers allocated.\n", rbuf.count);

	/* Map the buffers. */
	dev->buf = malloc(rbuf.count * sizeof(void *));

	for (i = 0; i < rbuf.count; i++) {
		memset(&dev->v4l2_buf, 0, sizeof(struct v4l2_buffer));
		dev->v4l2_buf.index = i;
		dev->v4l2_buf.type = dev->type;
		dev->v4l2_buf.memory = V4L2_MEMORY_MMAP;

		ret = v4l2_query_buffer(dev->fd, &dev->v4l2_buf);
		if (ret < 0)
			return -1;
		DPRINTF("length: %u offset: %u\n", dev->v4l2_buf.length, dev->v4l2_buf.m.offset);

		dev->buf[i] = mmap(0, dev->v4l2_buf.length,PROT_READ | PROT_WRITE,
				   MAP_SHARED, dev->fd, dev->v4l2_buf.m.offset);
		if (dev->buf[i] == MAP_FAILED) {
			perror("Unable to maped buffer");
			return -1;
		}
		DPRINTF("Buffer %u mapped at address %p.\n", i, dev->buf[i]);

		dev->v4l2_buf.index = i;
		dev->v4l2_buf.type = dev->type;
		dev->v4l2_buf.memory = V4L2_MEMORY_MMAP;

		ret = v4l2_queue_buf(dev->fd, &dev->v4l2_buf);
		if (ret < 0)
			return ret;
	}

	dev->bufsize = dev->v4l2_buf.length;
	dev->nbufs = rbuf.count;

	return 0;
}

static int
uvc_gadget_stream(struct uvc_gadget_device *dev, int enable)
{
	int ret;

	if (enable) {
		DPRINTF("Starting video stream.\n");
		ret = v4l2_stream_on(dev->fd, dev->type);
	} else {
		DPRINTF("Stopping video stream.\n");
		ret = v4l2_stream_off(dev->fd, dev->type);
	}

	return ret;
}

static int
uvc_gadget_set_format(struct uvc_gadget_device *dev, unsigned int imgsize)
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

	ret = v4l2_set_format(dev->fd, &fmt);

#if 0
    //获取视频格式
    ret1 = ioctl(dev->fd, VIDIOC_G_FMT, &fmt);
    if (ret1 < 0) {
        printf("VIDIOC_G_FMT failed (%d)\n", ret1);
        return ret1;
    }
    // Print Stream Format
    printf("Stream Format Informations:\n");
    printf(" deviceid: %d\n", dev->fd);
    printf(" type: %d\n", fmt.type);
    printf(" width: %d\n", fmt.fmt.pix.width);
    printf(" height: %d\n", fmt.fmt.pix.height);
    char fmtstr[8];
    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
    printf(" pixelformat: %s\n", fmtstr);
    printf(" field: %d\n", fmt.fmt.pix.field);
    printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
    printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
    printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
    printf(" priv: %d\n", fmt.fmt.pix.priv);
    printf(" raw_date: %s\n", fmt.fmt.raw_data);
#endif

    return ret;
}

/* ---------------------------------------------------------------------------
 * Request processing
 */

struct uvc_gadget_frame_info
{
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
    {  320, 240, { 666666, 1000000, 5000000, 0 }, },
	{  640, 360, { 666666, 1000000, 5000000, 0 }, },
	{ 1280, 720, { 50000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_gadget_frame_info uvc_gadget_frames_nv12[] = {
	{  640, 480, { 333333, 666666, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_gadget_frame_info uvc_gadget_frames_mjpg[] = {
	{  640, 360, { 666666, 1000000, 5000000, 0 }, },
	{ 1280, 720, { 5000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_gadget_format_info uvc_gadget_formats[] = {
	{ V4L2_PIX_FMT_YUYV,  uvc_gadget_frames_yuyv },
	//{ V4L2_PIX_FMT_NV12,  uvc_gadget_frames_nv12 },
	{ V4L2_PIX_FMT_MJPEG, uvc_gadget_frames_mjpg },
};

static void
uvc_gadget_fill_streaming_control(struct uvc_gadget *gadget,
			   struct uvc_streaming_control *ctrl, int iframe, int iformat)
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
	case V4L2_PIX_FMT_YUYV:
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_NV12:
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 3 / 2;
		break;
	case V4L2_PIX_FMT_MJPEG:
		ctrl->dwMaxVideoFrameSize = gadget->imgsize;
		break;
	}
	ctrl->dwMaxPayloadTransferSize = 1024;

	ctrl->bmFramingInfo = 3; /* Support FID/EOF bit of the payload headers */
	ctrl->bPreferedVersion = 1; /* Payload header version 1.1 */
	ctrl->bMaxVersion = 1;
}

static void
uvc_gadget_events_process_standard(struct uvc_gadget *gadget,
				   struct usb_ctrlrequest *ctrl,
				   struct uvc_request_data *resp)
{
	DPRINTF("standard request\n");
	(void)gadget;
	(void)ctrl;
	(void)resp;
}

static void
uvc_gadget_events_process_control(struct uvc_gadget *gadget, uint8_t req,
				  uint8_t cs, struct uvc_request_data *resp)
{
	DPRINTF("control request (req %02x cs %02x)\n", req, cs);
	(void)gadget;
	(void)req;
	(void)cs;
	(void)resp;
}

static void
uvc_gadget_events_process_streaming(struct uvc_gadget *gadget, uint8_t req,
				    uint8_t cs, struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	DPRINTF("streaming request (req %02x cs %02x)\n", req, cs);

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
			memcpy(ctrl, &gadget->probe,
			       sizeof(struct uvc_streaming_control));
		else
			memcpy(ctrl, &gadget->commit,
			       sizeof(struct uvc_streaming_control));
		break;
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		uvc_gadget_fill_streaming_control(gadget, ctrl,
					   (req == UVC_GET_MAX) ? -1 : 0,
					   (req == UVC_GET_MAX) ? -1 : 0);
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

static void
uvc_gadget_events_process_class(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl,
				struct uvc_request_data *resp)
{

    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		uvc_gadget_events_process_control(gadget, ctrl->bRequest,
						  ctrl->wValue >> 8, resp);
		break;
	case UVC_INTF_STREAMING:
		uvc_gadget_events_process_streaming(gadget, ctrl->bRequest,
						    ctrl->wValue >> 8, resp);
		break;
	default:
		break;
	}
}

static void
uvc_gadget_events_process_setup(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl,
				struct uvc_request_data *resp)
{
	gadget->control = 0;

	DPRINTF("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
		 "wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
		ctrl->wValue, ctrl->wIndex, ctrl->wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		uvc_gadget_events_process_standard(gadget, ctrl, resp);
		break;
	case USB_TYPE_CLASS:
		uvc_gadget_events_process_class(gadget, ctrl, resp);
		break;
	default:
		break;
	}
}

static void
uvc_gadget_events_process_data(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;

	switch (gadget->control) {
	case UVC_VS_PROBE_CONTROL:
		DPRINTF("setting probe control, length = %d\n", data->length);
		target = &gadget->probe;
		break;
	case UVC_VS_COMMIT_CONTROL:
		DPRINTF("setting commit control, length = %d\n", data->length);
		target = &gadget->commit;
		break;
	default:
		fprintf(stderr, "setting unknown control, length = %d\n",
			data->length);
		return;
	}

	ctrl = (struct uvc_streaming_control *)&data->data;

	iformat = CLAMP((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_gadget_formats));
	format = &uvc_gadget_formats[iformat - 1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		nframes++;

	iframe = CLAMP((unsigned int)ctrl->bFrameIndex, 1U, nframes);
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
	case V4L2_PIX_FMT_MJPEG:
		if (!gadget->imgsize)
			fprintf(stderr,
				"WARNING: MJPEG requested and no image loaded.\n");
		target->dwMaxVideoFrameSize = gadget->imgsize;
		break;
	}
	target->dwFrameInterval = *interval;

	if (gadget->control == UVC_VS_COMMIT_CONTROL) {
		gadget->out->fcc = format->fcc;
		gadget->out->width = frame->width;
		gadget->out->height = frame->height;
		uvc_gadget_set_format(gadget->out,
				      (format->fcc == V4L2_PIX_FMT_MJPEG) ?
				      gadget->imgsize : 0);
	}
}

static void
uvc_gadget_events_process(struct uvc_gadget *gadget)
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
	struct uvc_request_data resp;
	int ret;

	ret = v4l2_dequeue_event(gadget->out->fd, &v4l2_event);
	if (ret < 0)
		return;

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
	case UVC_EVENT_DISCONNECT:
		break;
	case UVC_EVENT_SETUP:
		memset(&resp, 0, sizeof(struct uvc_request_data));
		resp.length = -EL2HLT;
		uvc_gadget_events_process_setup(gadget, &uvc_event->req, &resp);
		uvc_send_response(gadget->out->fd, &resp);
		break;
	case UVC_EVENT_DATA:
		uvc_gadget_events_process_data(gadget, &uvc_event->data);
		break;
	case UVC_EVENT_STREAMON:
		//if (gadget->out->fcc == V4L2_PIX_FMT_NV12) {
		if (gadget->out->fcc == V4L2_PIX_FMT_YUYV) {
		     DPRINTF("##:%s_%d\n", __func__, __LINE__);
			//uvc_gadget_reqbufs(gadget->in, NR_VIDEO_BUF);
			//uvc_gadget_stream(gadget->in, VIDEO_STREAM_ON);
		}

		uvc_gadget_reqbufs(gadget->out, NR_VIDEO_BUF);
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_ON);
		break;
	case UVC_EVENT_STREAMOFF:
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_OFF);
		uvc_gadget_reqbufs(gadget->out, 0);

		//if (gadget->out->fcc == V4L2_PIX_FMT_NV12) {
		if (gadget->out->fcc == V4L2_PIX_FMT_YUYV) {
		     DPRINTF("##:%s_%d\n", __func__, __LINE__);
			uvc_gadget_stream(gadget->in, VIDEO_STREAM_OFF);
			uvc_gadget_reqbufs(gadget->in, 0);
		}
		break;
	}
}

static int
uvc_gadget_events_init(struct uvc_gadget *gadget)
{
	struct v4l2_event_subscription sub;
	int ret;

	uvc_gadget_fill_streaming_control(gadget, &gadget->probe, 0, 0);
	uvc_gadget_fill_streaming_control(gadget, &gadget->commit, 0, 0);
    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	memset(&sub, 0, sizeof(struct v4l2_event_subscription));
	sub.type = UVC_EVENT_SETUP;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
    //DPRINTF("##:%s_%d\n", __func__, __LINE__);
	sub.type = UVC_EVENT_DATA;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
    //DPRINTF("##:%s_%d\n", __func__, __LINE__);
	sub.type = UVC_EVENT_STREAMON;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
    //DPRINTF("##:%s_%d\n", __func__, __LINE__);
	sub.type = UVC_EVENT_STREAMOFF;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;
    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	return 0;
}

/* ---------------------------------------------------------------------------
 * main
 */
static int
uvc_gadget_outdev_reset(const char *dev)
{
	int fd;

	fd = open(dev, O_WRONLY);
	if (fd < 0) {
		perror("Error opening output file");
		return -1;
	}

	close(fd);

	return 0;
}

static int
uvc_gadget_device_open(const char *dev)
{
	struct v4l2_capability cap;
	struct stat st;
	int fd;
	int ret;

	ret = stat(dev, &st);
	if (ret < 0) {
		perror("stat");
		return ret;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev);
		return -1;
	}

	//fd = open(dev, O_RDWR | O_NONBLOCK);
	fd = open(dev, O_RDWR);
	if (fd < -1) {
		perror("open");
		return -1;
	}
	DPRINTF("'%s' open succeeded, file descriptor = %d\n", dev, fd);

	ret = v4l2_query_capture(fd, &cap);
	if (ret < 0)
		goto uvcdev_do_close;
	DPRINTF("'%s' is %s on bus %s\n", dev, cap.card, cap.bus_info);

	return fd;

uvcdev_do_close:
	close(fd);

	return -1;
}

static struct uvc_gadget *
uvc_gadget_init(const char *outdev, const char *indev)
{
	struct uvc_gadget *gadget;

	gadget = malloc(sizeof(struct uvc_gadget));
	if (gadget == NULL)
		return NULL;
	memset(gadget, 0, sizeof(struct uvc_gadget));

	gadget->out = malloc(sizeof(struct uvc_gadget_device));
	if (gadget->out == NULL)
		goto uvc_do_free;
	memset(gadget->out, 0, sizeof(struct uvc_gadget_device));

	gadget->in = malloc(sizeof(struct uvc_gadget_device));
	if (gadget->in == NULL)
		goto uvc_do_free;
	memset(gadget->in, 0, sizeof(struct uvc_gadget_device));

	gadget->out->fd = uvc_gadget_device_open(outdev);
	if (gadget->out->fd < 0)
		goto uvc_do_free;
	gadget->out->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	gadget->out->fcc = V4L2_PIX_FMT_YUYV;

	gadget->in->fd = uvc_gadget_device_open(indev);
	if (gadget->in->fd < 0)
		goto uvc_do_close;
	gadget->in->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	gadget->in->fcc = V4L2_PIX_FMT_YUYV;
	gadget->in->width = uvc_gadget_frames_yuyv[2].width;
	gadget->in->height = uvc_gadget_frames_yuyv[2].height;
	uvc_gadget_set_format(gadget->in, 0);

	return gadget;

uvc_do_close:
	close(gadget->out->fd);
uvc_do_free:
	free(gadget);
	free(gadget->out);
	free(gadget->in);

	return NULL;
}

static void
uvc_gadget_exit(struct uvc_gadget *gadget)
{
	close(gadget->in->fd);
	close(gadget->out->fd);
	free(gadget->in);
	free(gadget->out);
	free(gadget);
}

static int
uvc_gadget_image_load(struct uvc_gadget *gadget, const char *image)
{
	int fd;
	int ret;

	if (image == NULL)
		return 0;

	fd = open(image, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Unable to open JPEG image '%s'\n", image);
		return -1;
	}

	gadget->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	gadget->imgdata = malloc(gadget->imgsize);
	if (gadget->imgdata == NULL) {
		fprintf(stderr, "Unable to allocate memory for JPEG image\n");
		gadget->imgsize = 0;
		return -1;
	}

	ret = xread(fd, gadget->imgdata, gadget->imgsize);
	if (ret < 0)
		fprintf(stderr, "Unable to read JPEG image '%s'\n", image);

	close(fd);

	return ret;
}

static void
uvc_gadget_image_unload(struct uvc_gadget *gadget)
{
	if (gadget->imgdata != NULL) {
		free(gadget->imgdata);
		gadget->imgsize = 0;
		gadget->imgdata = NULL;
	}
}

static void
uvc_gadget_usage(const char *name)
{
	printf("Usage: %s [options]\n", name);
	printf("Available options are\n");
	printf(" -o		Output Video device\n");
	printf(" -c		Capture Video device\n");
	printf(" -i image	JPEG image\n");
	printf(" -h		Print this help screen and exit\n");
}

static int is_exit = 0;
static void
uvc_gadget_sighandler(int signum __attribute__((unused)))
{
	is_exit = 1;
}

int main(int argc, char *argv[])
{
	struct uvc_gadget *gadget;
	char *outdev = DEFAULT_OUTPUT_DEVICE;
	char *indev = DEFAULT_CAPTURE_DEVICE;
	char *mjpeg_image = NULL;
	fd_set fds;
	fd_set wfds;
	fd_set efds;
	int opt;
	int ret;

	int i;

	while ((opt = getopt(argc, argv, "o:c:i:h")) != -1) {
		switch (opt) {
		case 'o':
			outdev = optarg;
			break;
		case 'c':
			indev = optarg;
			break;
		case 'i':
			mjpeg_image = optarg;
			break;
		case 'h':
			uvc_gadget_usage(argv[0]);
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "Invalid option '-%c'\n", opt);
			uvc_gadget_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	DPRINTF("##:%s_%d\n", __func__, __LINE__);


	ret = uvc_gadget_outdev_reset(outdev);
	if (ret < 0)
		return EXIT_FAILURE;
    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	gadget = uvc_gadget_init(outdev, indev);
	if (gadget == NULL)
		return EXIT_FAILURE;


    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	ret = uvc_gadget_image_load(gadget, mjpeg_image);
	if (ret < 0)
		return EXIT_FAILURE;
    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	ret = uvc_gadget_events_init(gadget);
	if (ret < 0)
		return EXIT_FAILURE;
    DPRINTF("##:%s_%d\n", __func__, __LINE__);
	signal(SIGINT, uvc_gadget_sighandler);


  #if 1     // 打开摄像头设备
    int fd = gadget->in->fd;

    // 获取驱动信息
    struct v4l2_capability cap;
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("VIDIOC_QUERYCAP failed (%d)\n", ret);
        return ret;
    }

    printf("Capability Informations 23 :\n");
    printf(" driver: %s\n", cap.driver);
    printf(" card: %s\n", cap.card);
    printf(" bus_info: %s\n", cap.bus_info);
    printf(" version: %u.%u.%u\n", (cap.version>>16)&0XFF, (cap.version>>8)&0XFF,cap.version&0XFF);
    printf(" capabilities: %08X\n", cap.capabilities);


    // 设置视频格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 1280;
    fmt.fmt.pix.height      = 720;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_S_FMT failed (%d)\n", ret);
        return ret;
    }

    //获取视频格式
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_G_FMT failed (%d)\n", ret);
        return ret;
    }
    // Print Stream Format
    printf("Stream Format Informations:\n");
    printf(" type: %d\n", fmt.type);
    printf(" width: %d\n", fmt.fmt.pix.width);
    printf(" height: %d\n", fmt.fmt.pix.height);
    char fmtstr[8];
    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
    printf(" pixelformat: %s\n", fmtstr);
    printf(" field: %d\n", fmt.fmt.pix.field);
    printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
    printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
    printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
    printf(" priv: %d\n", fmt.fmt.pix.priv);
    printf(" raw_date: %s\n", fmt.fmt.raw_data);



 //请求分配内存
    struct v4l2_requestbuffers reqbuf;

    reqbuf.count = NR_VIDEO_BUF;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);
    if(ret < 0) {
        printf("VIDIOC_REQBUFS failed (%d)\n", ret);
        return ret;
    }

   // VideoBuffer*  buffers = calloc( reqbuf.count, sizeof(*buffers) );
    //struct v4l2_buffer buf;

    printf("test1\n");
    gadget->in->buf = malloc(reqbuf.count * sizeof(void *));
    for (i = 0; i < reqbuf.count; i++)
    {
        gadget->in->v4l2_buf.index = i;
        gadget->in->v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        gadget->in->v4l2_buf.memory = V4L2_MEMORY_MMAP;
        //获取内存空间
        printf("test2\n");
        ret = ioctl(fd , VIDIOC_QUERYBUF, &gadget->in->v4l2_buf);
        if(ret < 0) {
            printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
            return ret;
        }
        printf("test3\n");

		gadget->in->buf[i] = mmap(0, gadget->in->v4l2_buf.length,PROT_READ | PROT_WRITE,
				   MAP_SHARED, gadget->in->fd, gadget->in->v4l2_buf.m.offset);
		if (gadget->in->buf[i] == MAP_FAILED) {
			perror("Unable to maped buffer");
			return -1;
		}
        printf("test4\n");
        //内存入队列
        ret = ioctl(fd , VIDIOC_QBUF, &gadget->in->v4l2_buf);
        if (ret < 0) {
            printf("VIDIOC_QBUF int (%d) failed (%d)\n", i, ret);
            return -1;
        }
    }
     printf("test5\n");
    // 启动视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("VIDIOC_STREAMON failed (%d)\n", ret);
        return ret;
    }

    printf("start camera testing...\n");

#if 0
    i = 0;
    while (i < 100)
    {
        i ++;
        ret = ioctl(fd, VIDIOC_DQBUF, &gadget->in->v4l2_buf);
        if (ret < 0)
         {
            printf("VIDIOC_DQBUF1 failed (%d)\n", ret);
            return ret;
         }

        //in = gadget->in->buf[0];
		//memcpy(out, in, size);

		   printf("get buffer ok...., buffer id = %d\n", gadget->in->v4l2_buf.index);


         // 内存重新入队列
        ret = ioctl(fd, VIDIOC_QBUF, &gadget->in->v4l2_buf);
        if (ret < 0)
          {
            printf("VIDIOC_QBUF failed2 (%d)\n", ret);
            return ret;
          }

    }


            ret = ioctl(fd, VIDIOC_DQBUF, &gadget->in->v4l2_buf);
        if (ret < 0)
         {
            printf("VIDIOC_DQBUF1 failed (%d)\n", ret);
            return ret;
         }
         printf("get buffer ok22....buffer id = %d\n",gadget->in->v4l2_buf.index);

#endif

#endif




        //in = gadget->in->buf[0];
		//memcpy(out, in, size);




	FD_ZERO(&fds);
	FD_SET(gadget->out->fd, &fds);
	while (!is_exit) {
		wfds = fds;
		efds = fds;
        //DPRINTF("##:%s_%d\n", __func__, __LINE__);
		ret = select(gadget->out->fd + 1, NULL, &wfds, &efds, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else {
				perror("select");
				exit(EXIT_FAILURE);
			}
		}
         //DPRINTF("##:%s_%d\n", __func__, __LINE__);
		if (FD_ISSET(gadget->out->fd, &efds))
			uvc_gadget_events_process(gadget);
		if (FD_ISSET(gadget->out->fd, &wfds))
			uvc_gadget_video_process(gadget);
	     //DPRINTF("##:%s_%d\n", __func__, __LINE__);
	}

	uvc_gadget_image_unload(gadget);
	uvc_gadget_exit(gadget);

	return EXIT_SUCCESS;
}
