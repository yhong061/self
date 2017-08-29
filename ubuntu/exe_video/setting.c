#include "video.h"

int camera_get_query(int cmd, int *min, int *max, int *step)
{
	int ret = 0;
	struct v4l2_queryctrl queryctrl;

	queryctrl.id = cmd;
	queryctrl.minimum = 0;
	queryctrl.maximum = 0;
	queryctrl.step = 0;
	queryctrl.default_value = 0;
	queryctrl.flags = 0;

	ret = ioctl(gFd, VIDIOC_QUERYCTRL, &queryctrl);
sleep(1);DBG("\n\n");
	if(ret == 0) {
		DBG("min = %d\n", queryctrl.minimum);
		DBG("max = %d\n", queryctrl.maximum);
		DBG("default = %d\n", queryctrl.default_value);
		DBG("step = %d\n", queryctrl.step);
		DBG("flag = %d\n", queryctrl.flags);

		*min = queryctrl.minimum;
		*max = queryctrl.maximum;
		*step = queryctrl.step;
	}
	else
		DBG("Get QueryCtrl fail\n");
sleep(1);DBG("\n\n");
	
	return ret;
}

int camera_get_cmd(int cmd, int *value)
{
	int ret = 0;
	struct v4l2_control ctrl;

	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = cmd;

	ret = ioctl(gFd, VIDIOC_G_CTRL, &ctrl);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	else {
		*value = ctrl.value;
		DBG("Set VIDIOC_G_CTRL Ok: value = %d\n", ctrl.value);
	}
	return ret;
}

int camera_set_cmd(int cmd, int value)
{
	int ret = 0;
	struct v4l2_control ctrl;

	ctrl.id = cmd;
	ctrl.value = value;

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	else {
		DBG("Set VIDIOC_S_CTRL Ok\n");
	}
	return ret;
}


int camera_get_framerate(int *framerate)
{
	int ret = 0;
	struct v4l2_streamparm streamparm;

	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(gFd, VIDIOC_G_PARM, &streamparm);
	if(ret) {
		DBG("[%d]fail: VIDIOC_G_PARM\n", __LINE__);
		return -1;
	}
	*framerate = streamparm.parm.capture.timeperframe.denominator;	
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

sleep(1);DBG("\n\n");
	ret = ioctl(gFd, VIDIOC_S_PARM, &streamparm);
sleep(1);DBG("\n\n");
	if(ret) {
		DBG("[%d]fail: VIDIOC_S_PARM\n", __LINE__);
		return -1;
	}
	return ret;
}

int test_framerate(int fps)
{
    int ret = 0;
    int framerate;

    ret = camera_get_framerate(&framerate);
    if(ret) 
        printf("get framerate fail, ret = %d\n", ret);

    framerate = fps;
    ret = camera_set_framerate(framerate);
    if(ret) {
        printf("set framerate [%d] fail, ret = %d\n", framerate, ret);
    }

    ret = camera_get_framerate(&framerate);
    if(ret) 
        printf("get framerate fail, ret = %d\n", ret);

	printf("get framerate = %d, set framerate = %d\n", framerate, fps);

    return ret;
}

int camera_get_exposure_query(int *min, int *max, int *step)
{
	int ret = camera_get_query(V4L2_CID_EXPOSURE, min, max, step);
	if(ret != 0) {
		DBG("Set VIDIOC_G_QUERY fail\n");
	}
	return ret;
}

int camera_get_exposure(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_EXPOSURE, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_exposure(int value)
{
	int ret = camera_set_cmd(V4L2_CID_EXPOSURE, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_exposure(int exposure)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //get exposure
    ret = camera_get_exposure_query(&min, &max, &step);
    if(ret) {
        printf("get exposure_query fail\n");
	return ret;
    }
    printf("min=%d, max=%d, step=%d\n", min, max, step);

    //set exposure

    printf("set exposure = %d\n", exposure);
    ret = camera_set_exposure(exposure);
    if(ret) {
        printf("set exposure [%d] fail, ret = %d\n", exposure, ret);
    }
    printf("\n");
    
    camera_get_exposure(&gvalue);
    if(ret) 
        printf("get exposure fail, ret = %d\n", ret);
    else
        printf("get exposure OK: gvalue = %d\n", gvalue);

    return ret;
}

int test_exposure2(int exposure)
{
	int ret;
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	ctrl.value = V4L2_EXPOSURE_MANUAL;
	printf("set exposure auto informations:\n");
	printf("id = %d\t value = %d\n", ctrl.id, ctrl.value);

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set exposure auto failed (%d)\n", ret);
		return ret;
	}
	else
	{
		printf("set exposure auto ok!! (%d)\n", ret);
	}

	ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	ctrl.value = exposure;
	printf("set exposure absolute informations:\n");
	printf("id = %d\t value = %d\n", ctrl.id, ctrl.value);

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set exposure absolute failed (%d)\n", ret);
		return ret;
	}
	else
	{
		printf("set exposure absolute ok!! (%d)\n", ret);
	}

	return 0;
}


int test_freq(int freq)
{
	int ret;
	struct v4l2_control ctrl;
	
	ctrl.id = V4L2_CID_BRIGHTNESS;
	ctrl.value = freq;
	printf("set freq : %d\n", freq);

	ret = ioctl(gFd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set freq failed (%d)\n", ret);
		return ret;
	}
	else
	{
		printf("set freq ok!! (%d)\n", ret);
	}

	return 0;
}

///////////////////////////////////////////////////////////////

int camera_get_gain_query(int *min, int *max, int *step)
{
	int ret = camera_get_query(V4L2_CID_GAIN, min, max, step);
	if(ret != 0) {
		DBG("Set VIDIOC_G_QUERY fail\n");
	}
	return ret;
}

int camera_get_gain(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_GAIN, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_gain(int value)
{
	int ret = camera_set_cmd(V4L2_CID_GAIN, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_gain(int gain)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //get gain
    ret = camera_get_gain_query(&min, &max, &step);
    if(ret) {
        printf("get gain_query fail\n");
	return ret;
    }
    printf("min=%d, max=%d, step=%d\n", min, max, step);

    //set gain

    printf("set gain = %d\n", gain);
    ret = camera_set_gain(gain);
    if(ret) {
        printf("set gain [%d] fail, ret = %d\n", gain, ret);
    }
    printf("\n");
    
    camera_get_gain(&gvalue);
    if(ret) 
        printf("get gain fail, ret = %d\n", ret);
    else
        printf("get gain OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////

int camera_get_ev_query(int *min, int *max, int *step)
{
return 0;
//	int ret = camera_get_query(V4L2_CID_EXPOSURE_COMP, min, max, step);
//	if(ret != 0) {
//		DBG("Set VIDIOC_G_QUERY fail\n");
//	}
//	return ret;
}


int camera_get_ev(int *value)
{
return 0;
//	int ret = camera_get_cmd(V4L2_CID_EXPOSURE_COMP, value);
//	if(ret != 0) {
//		DBG("Set VIDIOC_G_CTRL fail\n");
//	}
//	return ret;
}

int camera_set_ev(int value)
{
return 0;
//	int ret = camera_set_cmd(V4L2_CID_EXPOSURE_COMP, value);
//	if(ret != 0) {
//		DBG("Set VIDIOC_S_CTRL fail\n");
//	}
//	return ret;
}

int test_ev(int ev)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //get ev
    ret = camera_get_ev_query(&min, &max, &step);
    if(ret) {
        printf("get ev_query fail\n");
	return ret;
    }
    printf("min=%d, max=%d, step=%d\n", min, max, step);

    //set ev

    printf("set ev = %d\n", ev);
    ret = camera_set_ev(ev);
    if(ret) {
        printf("set ev [%d] fail, ret = %d\n", ev, ret);
    }
    printf("\n");
    
    camera_get_ev(&gvalue);
    if(ret) 
        printf("get ev fail, ret = %d\n", ret);
    else
        printf("get ev OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////

int camera_get_brightness_query(int *min, int *max, int *step)
{
	int ret = camera_get_query(V4L2_CID_BRIGHTNESS, min, max, step);
	if(ret != 0) {
		DBG("Set VIDIOC_G_QUERY fail\n");
	}
	return ret;
}


int camera_get_brightness(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_BRIGHTNESS, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_brightness(int value)
{
	int ret = camera_set_cmd(V4L2_CID_BRIGHTNESS, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_brightness(int brightness)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //get brightness
    ret = camera_get_brightness_query(&min, &max, &step);
    if(ret) {
        printf("get brightness_query fail\n");
	return ret;
    }
    printf("min=%d, max=%d, step=%d\n", min, max, step);

    //set brightness

    printf("set brightness = %d\n", brightness);
    ret = camera_set_brightness(brightness);
    if(ret) {
        printf("set brightness [%d] fail, ret = %d\n", brightness, ret);
    }
    printf("\n");
    
    camera_get_brightness(&gvalue);
    if(ret) 
        printf("get brightness fail, ret = %d\n", ret);
    else
        printf("get brightness OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////

int camera_get_saturation_query(int *min, int *max, int *step)
{
	int ret = camera_get_query(V4L2_CID_SATURATION, min, max, step);
	if(ret != 0) {
		DBG("Set VIDIOC_G_QUERY fail\n");
	}
	return ret;
}


int camera_get_saturation(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_SATURATION, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_saturation(int value)
{
	int ret = camera_set_cmd(V4L2_CID_SATURATION, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_saturation(int saturation)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //get saturation
    ret = camera_get_saturation_query(&min, &max, &step);
    if(ret) {
        printf("get saturation_query fail\n");
	return ret;
    }
    printf("min=%d, max=%d, step=%d\n", min, max, step);

    //set saturation

    printf("set saturation = %d\n", saturation);
    ret = camera_set_saturation(saturation);
    if(ret) {
        printf("set saturation [%d] fail, ret = %d\n", saturation, ret);
    }
    printf("\n");
    
    camera_get_saturation(&gvalue);
    if(ret) 
        printf("get saturation fail, ret = %d\n", ret);
    else
        printf("get saturation OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////

int camera_get_mirror(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_HFLIP, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_mirror(int value)
{
	int ret = camera_set_cmd(V4L2_CID_HFLIP, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_mirror(int mirror)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //set mirror

    printf("set mirror = %d\n", mirror);
    ret = camera_set_mirror(mirror);
    if(ret) {
        printf("set mirror [%d] fail, ret = %d\n", mirror, ret);
    }
    printf("\n");
    
    camera_get_mirror(&gvalue);
    if(ret) 
        printf("get mirror fail, ret = %d\n", ret);
    else
        printf("get mirror OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////

int camera_get_colorfx(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_COLORFX, value);
	if(ret != 0) {
		DBG("Set VIDIOC_G_CTRL fail\n");
	}
	return ret;
}

int camera_set_colorfx(int value)
{
	int ret = camera_set_cmd(V4L2_CID_COLORFX, value);
	if(ret != 0) {
		DBG("Set VIDIOC_S_CTRL fail\n");
	}
	return ret;
}

int test_colorfx(int colorfx)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //set colorfx

    printf("set colorfx = %d\n", colorfx);
    ret = camera_set_colorfx(colorfx);
    if(ret) {
        printf("set colorfx [%d] fail, ret = %d\n", colorfx, ret);
    }
    printf("\n");
    
    camera_get_colorfx(&gvalue);
    if(ret) 
        printf("get colorfx fail, ret = %d\n", ret);
    else
        printf("get colorfx OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////
int camera_get_flip(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_VFLIP, value);
	if(ret != 0) {
		DBG("Get V4L2_CID_VFLIP fail\n");
	}
	return ret;
}

int camera_set_flip(int value)
{
	int ret = camera_set_cmd(V4L2_CID_VFLIP, value);
	if(ret != 0) {
		DBG("Set V4L2_CID_VFLIP fail\n");
	}
	return ret;
}

int test_flip(int flip)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //set flip

    printf("set flip = %d\n", flip);
    ret = camera_set_flip(flip);
    if(ret) {
        printf("set flip [%d] fail, ret = %d\n", flip, ret);
    }
    printf("\n");
    
    camera_get_flip(&gvalue);
    if(ret) 
        printf("get flip fail, ret = %d\n", ret);
    else
        printf("get flip OK: gvalue = %d\n", gvalue);

    return ret;
}


///////////////////////////////////////////////////////////////
int camera_get_ae(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_EXPOSURE_AUTO, value);
	if(ret != 0) {
		DBG("Get V4L2_CID_EXPOSURE_AUTO fail\n");
	}
	return ret;
}

int camera_set_ae(int value)
{
	int ret = camera_set_cmd(V4L2_CID_EXPOSURE_AUTO, value);
	if(ret != 0) {
		DBG("Set V4L2_CID_EXPOSURE_AUTO fail\n");
	}
	return ret;
}

int test_ae(int ae)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //set ae

    printf("set ae = %d\n", ae);
    ret = camera_set_ae(ae);
    if(ret) {
        printf("set ae [%d] fail, ret = %d\n", ae, ret);
    }
    printf("\n");
    
    camera_get_ae(&gvalue);
    if(ret) 
        printf("get ae fail, ret = %d\n", ret);
    else
        printf("get ae OK: gvalue = %d\n", gvalue);

    return ret;
}

///////////////////////////////////////////////////////////////
int camera_get_awb(int *value)
{
	int ret = camera_get_cmd(V4L2_CID_AUTO_WHITE_BALANCE, value);
	if(ret != 0) {
		DBG("Get V4L2_CID_AUTO_WHITE_BALANCE fail\n");
	}
	return ret;
}

int camera_set_awb(int value)
{
	int ret = camera_set_cmd(V4L2_CID_AUTO_WHITE_BALANCE, value);
	if(ret != 0) {
		DBG("Set V4L2_CID_AUTO_WHITE_BALANCE fail\n");
	}
	return ret;
}

int test_awb(int awb)
{
    int ret = 0;
    int min=0;
    int max=0;
    int step=0;
    int value, gvalue;

    //set awb

    printf("set awb = %d\n", awb);
    ret = camera_set_awb(awb);
    if(ret) {
        printf("set awb [%d] fail, ret = %d\n", awb, ret);
    }
    printf("\n");
    
    camera_get_awb(&gvalue);
    if(ret) 
        printf("get awb fail, ret = %d\n", ret);
    else
        printf("get awb OK: gvalue = %d\n", gvalue);

    return ret;
}



