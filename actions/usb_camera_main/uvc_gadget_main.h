#ifndef _USB_CAMERA_MAIN_H_
#define _USB_CAMERA_MAIN_H_
extern "C" {
#include "../video/video2.h"
#include "../usb_camera/uvc_gadget_video.h"
}
#include "../tango/tango.h"

#define TANGO_FMT_TYPE_MULTIFREQ     0
#define TANGO_FMT_TYPE_MONOFREQ      1
#define TANGO_FMT_TYPE_MONOFREQ_80M      2

#define OPEN_TANGO	(1)
#define OPEN_CAMERA	(2)

#define  CATCH_LEFT     (107)
#define  CATCH_RIGHT    (117)
#define  CATCH_TOP      (81)
#define  CATCH_BOTTOM   (91)

#define   DISTANCE_STEP_INT  (300)
//#define   DISTANCE_STEP_INT  (350)

#define STEP_W (2)
#define STEP_H (2)
#define STEP_OFFSET (16)

struct uvc_video {
	unsigned int   base_serial;
	struct timeval base_tv;
	unsigned int   last_serial;
	struct timeval last_tv;
	unsigned int   serial;
	struct timeval tv;
};

static int is_exit = 0;
static int video_output_streamon = 0;

static char DEFAULT_OUTPUT_DEVICE[] = "/dev/video1";
static char CAMERA_DEVICE[] =  "/dev/video0";


static int uvc_gadget_camera_open(uvc_gadget *gadget);
static void uvc_gadget_camera_close(uvc_gadget *gadget, int destroy);

#endif
