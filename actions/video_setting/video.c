#include "video.h"
#include "ion.c"
#include "setting.c"

int framesize_index_i = 0;
int format_index_i = 1;
int resolution_display = 0;

int camera_G_FMT(void)
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

sleep(1);DBG("\n\n");
	DBG("=======================[VIDIOC_G_FMT: S]=======================\n");
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
	DBG("=======================[VIDIOC_G_FMT: E]=======================\n");
sleep(1);DBG("\n\n");

	return 0;
}

int camera_S_FMT(unsigned int w, unsigned int h, unsigned int pixfmt)
{
	int ret = 0;
	struct v4l2_format fmt;

	if(w==0 || h==0) {
		DBG("unsuport resolution: wxh=%dx%d\n", w, h);
		return 0;
	}

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

sleep(1);DBG("\n");
	DBG("[%d] w = %d, h = %d, [%d]pixfmt = %d\n", framesize_index, w, h, fmt_index, pixfmt);
sleep(1);DBG("\n");

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
		DBG("[%d]fail: VIDIOC_ENUM_FMT\n", __LINE__);
		return ret;
	}

	v_efmt[index].pixfmt = fmtdest.pixelformat;
	memcpy(v_efmt[index].desc, fmtdest.description, 32);
	v_efmt[index].desc[31] = '\0';

	DBG(" index: %d\n", fmtdest.index);
	DBG(" pixelformat: %d, %d\n", fmtdest.pixelformat, v_efmt[index].pixfmt);
	DBG(" description: %s, %s\n", fmtdest.description, v_efmt[index].desc);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmtdest.pixelformat, 4);
	DBG(" pixelformat: %s\n", fmtstr);

	return 0;
}

int camera_enumfmt_all(void)
{
	int ret = 0;
	int index = 0;
	DBG("=======================[VIDIOC_ENUM_FMT: S]=======================\n");
	while(!ret){
		ret = camera_enumfmt(index);
		index++;
		DBG("\n");
	}
	gFmt_index = index-1;
	DBG("=======================[VIDIOC_ENUM_FMT: E (count = %d)]=======================\n", gFmt_index);
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
		DBG("[%d]fail: VIDIOC_ENUM_FRAMESIZES\n", __LINE__);
		return ret;
	}

	DBG(" index: %d\n", frmsize.index);
	DBG(" pixel_format: %d\n", frmsize.pixel_format);
	DBG(" type: %d\n", frmsize.type);
	if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {

		v_eframesize[index].w = frmsize.discrete.width;
		v_eframesize[index].h = frmsize.discrete.height;

		DBG(" width: %d, %d\n", frmsize.discrete.width, v_eframesize[index].w);
		DBG(" height: %d, %d\n", frmsize.discrete.height, v_eframesize[index].h);
	}
	else {
		DBG(" width: %d, %d\n", frmsize.stepwise.min_width, frmsize.stepwise.max_width);
		DBG(" height: %d, %d\n", frmsize.stepwise.min_height, frmsize.stepwise.max_height);
	}

	return 0;
}

int camera_enumframesize_all(void)
{
	int ret = 0;
	int index = 0;
	DBG("=======================[VIDIOC_ENUM_FRAMESIZES: S]=======================\n");
	while(!ret){
		ret = camera_enumframesize(index);
		index++;
		DBG("\n");
	}
	gFramesize_index = index-1;
	DBG("=======================[VIDIOC_ENUM_FRAMESIZES: E: (count = %d)]=======================\n", gFramesize_index);
	return index;
}

int camera_open(char *filename)
{
    int ret = 0;
    int index;
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

    if(resolution_display) {
        camera_G_FMT();
        camera_enumfmt_all();
        camera_enumframesize_all();
        camera_S_FMT_index(framesize_index_i, format_index_i);
    }


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
    memcpy(p, framebuf[buf.index].vir, size);
DBG("buf.lenght = %d\n", buf.length);

    // Re-queen buffer
    ret = ioctl(gFd, VIDIOC_QBUF, &buf);
    if(ret) {
        DBG("[%d]fail: VIDIOC_QBUF\n", __LINE__);
        return -1;	
    }
    gIndex++;
    if(gIndex >= BUFFER_COUNT)
        gIndex = 0;

    return size;
}


int test_framebuf(int sec, int save_file)
{
    char filename[32];
    int frame = 1;
    int i;
    int ret = 0;
    unsigned int framesize = FRAME_WIDTH * FRAME_HEIGTH * 2;
    unsigned int size = 0;
    char *buf = malloc(framesize);
    struct timeval cur;

    if(!buf) {
    	DBG("malloc buf fail\n");
	return -1;
    }

    if(sec > 0)
        frame = FRAME_RATE*sec;

    //get frame data
    for(i=0; i<frame; i++) {
        size = camera_getbuf(buf, framesize);
        if(size < 0) {
	    goto framebuf_end;
        }

	if(i%FRAME_RATE == 0) {
		gettimeofday(&cur, NULL);
        	DBG("[%10u.%06u] frame = %d\n", (unsigned int)cur.tv_sec, (unsigned int)cur.tv_usec, i);
	}
	
    	if(save_file== 1 && i%FRAME_RATE == 0)  {
#if 1	
                sprintf(filename, "%d%s", i, CAPTURE_FILE);
                FILE *fp = fopen(filename, "wb");
                fwrite(buf, 1, size, fp);
                fclose(fp);
                DBG("Capture one frame saved in %s, size = %d\n", filename, size);    
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
    DBG("video_main data sec               : get frame data for sec, such as 1 sec \n");
    DBG("video_main yuv sec res            : save yuv image: 1 sec \n");
    DBG("video_main file sec               : save yuv image: 1 frame \n");
    DBG("video_main reso                   : get camera resolution support\n");
    DBG("video_main expo val               : get exposure data\n");
    DBG("video_main gain val               : get gain data\n");
    DBG("video_main ev val                 : get ev data\n");
    DBG("video_main brightness val         : get brightness data\n");
    DBG("video_main saturation val         : get saturation data\n");
    DBG("video_main mirror val             : get mirror data\n");
    DBG("video_main flip val               : get flip data\n");
    DBG("video_main colorfx val            : get colorfx data\n");
    DBG("video_main ae val                 : get ae data\n");
    DBG("video_main awb val                : get awb data\n");
    DBG("video_main fps val                : get framerate\n");
    DBG("===========================================================\n");
}

int main(int argc, char **argv)
{
    int ret;
    char videoname[32];
    int idx;

    if(argc < 2) {
        usage_help();
        return 0;
    }    

    if(strcmp(argv[1], "reso") == 0)  {
        resolution_display = 1;
        argc = 3;
        DBG("resolution_display  = 1\n");
        sleep(1);DBG("\n\n");
    }

    if(argc < 3) {
        usage_help();
	return 0;
    }    

    if(strcmp(argv[1], "yuv") == 0)  {
        framesize_index_i = atoi(argv[3]);
	resolution_display = 1;
	if(argc> 4)
		format_index_i = atoi(argv[4]);
        DBG("framesize_index_i  = %d, format_index_i = %d\n", framesize_index_i, format_index_i);
        sleep(1);DBG("\n\n");
    }
   
    idx = 0;
    ret = 0;
    do {
        sprintf(videoname, "%s%d", CAMERA_DEVICE, idx++);
        ret = camera_open(videoname);
        if(ret >= 0) {
	    DBG("open %s OK!\n", videoname);
	    break;
        }
    }while(idx < 4);

    if(ret < 0)
    {
        DBG("open video[0, 3] fail\n");
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
    else if(strcmp(argv[1], "expo") == 0)  {
	int exposure = atoi(argv[2]);
	DBG("exposure = %d\n", exposure);

        test_exposure(exposure);
    }
    else if(strcmp(argv[1], "gain") == 0)  {
	int gain = atoi(argv[2]);
	DBG("gain = %d\n", gain);

        test_gain(gain);
    }
    else if(strcmp(argv[1], "ev") == 0)  {
	int ev = atoi(argv[2]);
	DBG("ev = %d\n", ev);

        test_ev(ev);
    }
    else if(strcmp(argv[1], "brightness") == 0)  {
	int brightness = atoi(argv[2]);
	DBG("brightness = %d\n", brightness);

        test_brightness(brightness);
    }
    else if(strcmp(argv[1], "saturation") == 0)  {
	int saturation = atoi(argv[2]);
	DBG("saturation = %d\n", saturation);

        test_saturation(saturation);
    }
    else if(strcmp(argv[1], "mirror") == 0)  {
	int mirror = atoi(argv[2]);
	DBG("mirror = %d\n", mirror);

        test_mirror(mirror);
    }
    else if(strcmp(argv[1], "flip") == 0)  {
	int flip = atoi(argv[2]);
	DBG("flip = %d\n", flip);

        test_flip(flip);
    }
    else if(strcmp(argv[1], "colorfx") == 0)  {
	int colorfx = atoi(argv[2]);
	DBG("colorfx = %d\n", colorfx);

        test_colorfx(colorfx);
    }
    else if(strcmp(argv[1], "ae") == 0)  {
	int ae = atoi(argv[2]);
	DBG("ae = %d\n", ae);

        test_ae(ae);
    }
    else if(strcmp(argv[1], "awb") == 0)  {
	int awb = atoi(argv[2]);
	DBG("awb = %d\n", awb);

        test_awb(awb);
    }
    else if(strcmp(argv[1], "fps") == 0) {
	int framerate = atoi(argv[2]);
	DBG("framerate = %d\n", framerate);
        
	test_framerate(framerate);
	sleep(10);
    }

sleep(1);DBG("\n\n");
main_end:
    camera_close();

    return 0;
}

