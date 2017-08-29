#ifndef __UVC_GADGET_VIDEO_H__
#define __UVC_GADGET_VIDEO_H__

#include <linux/usb/video.h>
#include "v4l2-controls.h"
#include "videodev2.h"

#ifndef DEBUG
#define DEBUG
#endif

#if defined(DEBUG)
#define DBG(args, ...) printf("[%s:%d]" args, __func__, __LINE__, ## __VA_ARGS__)
#else
#define DBG(args...)
#endif
#define ERR(args, ...) printf("[%s:%d]" args, __func__, __LINE__, ## __VA_ARGS__)

//depth data display yuv422 format
#define IMG_DEPTH_YUV_W	(224)
#define IMG_DEPTH_YUV_H	(172)

//depth
#define IMG_DEPTH_RGB_W	(224)
#define IMG_DEPTH_RGB_H	(173*2)
//depth
#define IMG_DEPTH_W	(224)
#define IMG_DEPTH_H	(172*2)
//gray+depth
#define IMG_GRAY_DEPTH_W	(224)
#define IMG_GRAY_DEPTH_H	(172*4)
//cloud
#define IMG_CLOUD_W	(224)
#define IMG_CLOUD_H	(172*8)
//raw mono
#define IMG_RAW_MONO_W	(224)
#define IMG_RAW_MONO_H	(173*5)
//raw multi
#define IMG_RAW_MULTI_W	(224)
#define IMG_RAW_MULTI_H	(173*9)

typedef enum _video_event_st{
	VIDEO_EVENT_STATE_ERR = -1,
	VIDEO_EVENT_STATE_OK = 0,
	VIDEO_EVENT_STREAM_ON,
	VIDEO_EVENT_STREAM_OFF,
	VIDEO_EVENT_DATA_CONTROL,
	VIDEO_EVENT_DATA_STREAM,
}video_event_st;

enum {
	VIDEO_IMAGE_RAW_MONO = 1,
	VIDEO_IMAGE_RAW_MULTI,
	VIDEO_IMAGE_DEPTH,
	VIDEO_IMAGE_GRAY_DEPTH,
	VIDEO_IMAGE_CLOUD,
	VIDEO_IMAGE_DEPTH_YUV,
	VIDEO_IMAGE_DEPTH_RGB,
};

struct uvc_control_st {
	unsigned int id;
	unsigned int cur;
	unsigned int min;
	unsigned int max;
	unsigned int def;
	unsigned int res;
	unsigned int len;
};

struct uvc_gadget_device {
	enum v4l2_buf_type type;
	int fd;
	struct v4l2_buffer v4l2_buf;

	unsigned int fcc;
	unsigned int image;
	unsigned int width;
	unsigned int height;

	void **buf;
	unsigned int nbufs;
	unsigned int bufsize;
};

struct uvc_gadget_camera {
	int opened;
	int source;  //OPEN_CAMERA, OPEN_TANGO
	int type;	 //TANGO_FMT_TYPE_MONOFREQ, TANGO_FMT_TYPE_MONOFREQ_80M, TANGO_FMT_TYPE_MULTI
	unsigned int width;
	unsigned int height;
};

typedef enum _uvc_freq_{
	UVC_FREQ_MONO = 1,
	UVC_FREQ_MONO_80M,
	UVC_FREQ_MULTI,
	UVC_FREQ_END,
}uvc_freq_enum;

struct uvc_gadget {
	int control;
	
	int videoc_index;
	int videoc_value;
	int videoc_length;

	int    restart;
	int    freq; //UVC_FREQ_MONO : 1=60M, 2=80M, 3=Multi
	int    spectre; // 1=do nothing, 2=unpack spectre
	int    eeprom_calib; // 1=do nothing, 2=check eeprom
	int    framerate;
	int    exposure;

	int    exposure_max;
	
	struct uvc_gadget_device *out;
	struct uvc_gadget_camera *in;

	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;

	unsigned int graysize;
	char *graybuf;
	
	unsigned int depthsize;
	char *depthbuf;
	char *depthbuf_adjust;

	unsigned int datasize;
	char *databuf;
};

struct uvc_gadget * uvc_gadget_open(const char *dev);
void uvc_gadget_close(struct uvc_gadget *gadget);

video_event_st uvc_gadget_events_process(struct uvc_gadget *gadget);

struct uvc_control_st* uvc_gadget_videoc_get_it(unsigned int cs);
struct uvc_control_st* uvc_gadget_videoc_get_pu(unsigned int cs);

int uvc_gadget_video_process(struct uvc_gadget *gadget , char *imgptr, unsigned int imgsize);
int uvc_gadget_video_process2(struct uvc_gadget *gadget , char *imgptr1, unsigned int imgsize1, char *imgptr2, unsigned int imgsize2);
#endif //__UVC_GADGET_VIDEO_H__
