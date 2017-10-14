#include "video.h"
#include "ion.c"
#include "setting.c"

int framesize_index_i = 0;
int format_index_i = 0;
int resolution_display = 0;
int allocbuffer_type = 0; //0: mmap, 1:userprt
int getbuf_en = 0;
int getbuf_sec = 0;
int getbuf_frame = 0;
int set_framerate_en = 0;
int set_framerate_val = 30;
int savefile_en = 0;
int savefile_frame = 0;
int savefile_intval = 0;
int print_fps = 0;
char capturename[64];
int videoidx = 0;
int freq = 1;
int expo = 200;

unsigned int gWidth = 1920;
unsigned int gHeight = 1080;
unsigned int gImagesize = 1920*1080;
unsigned int gFmt = V4L2_PIX_FMT_MJPEG;

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

	gWidth = fmt.fmt.pix.width;
	gHeight = fmt.fmt.pix.height;
	gFmt = fmt.fmt.pix.pixelformat;
	gImagesize = fmt.fmt.pix.sizeimage;

sleep(1);printf("\n\n");
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
		//DBG("[%d]fail: VIDIOC_ENUM_FMT\n", __LINE__);
		return ret;
	}

	v_efmt[index].pixfmt = fmtdest.pixelformat;
	memcpy(v_efmt[index].desc, fmtdest.description, 32);
	v_efmt[index].desc[31] = '\0';
	return 0;
}

int camera_enumfmt_all(void)
{
	int ret = 0;
	int index = 0;
	while(!ret){
		ret = camera_enumfmt(index);
		index++;
	}
	gFmt_index = index-1;
	
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
		//DBG("[%d]fail: VIDIOC_ENUM_FRAMESIZES\n", __LINE__);
		return ret;
	}
	if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {

		v_eframesize[index].w = frmsize.discrete.width;
		v_eframesize[index].h = frmsize.discrete.height;
	}

	return 0;
}

void display_resolution_info(void)
{
	char fmtstr[8];
	int index = 0;
	
sleep(1);printf("\n");
	DBG("=======================[RESOLUTION_INFO]=======================\n");
	DBG("Format: \n");
	for(index = 0; index < gFmt_index; index++) {
		memset(fmtstr, 0, 8);
		memcpy(fmtstr, &v_efmt[index].pixfmt, 4);
		DBG("\t [%02d] [Val=0x%08x] %s : %s\n", index, v_efmt[index].pixfmt, fmtstr, v_efmt[index].desc);
	}

	DBG("Resolution: \n");
	for(index = 0; index < gFramesize_index; index++) {
		memset(fmtstr, 0, 8);
		memcpy(fmtstr, &v_efmt[index].pixfmt, 4);
		DBG("\t [%02d] w = %d, h = %d\n", index, v_eframesize[index].w, v_eframesize[index].h);
	}

	DBG("Cur Resolution: \n"); 
		DBG("\t [%02d] w = %d, h = %d\n", framesize_index_i, v_eframesize[framesize_index_i].w, v_eframesize[framesize_index_i].h);
		memset(fmtstr, 0, 8);
		memcpy(fmtstr, &v_efmt[format_index_i].pixfmt, 4);
		DBG("\t [%02d] [Val=0x%08x] %s : %s\n", format_index_i, v_efmt[format_index_i].pixfmt, fmtstr, v_efmt[format_index_i].desc);
	DBG("=======================[RESOLUTION_INFO]=======================\n");

}



int camera_enumframesize_all(void)
{
	int ret = 0;
	int index = 0;
	while(!ret){
		ret = camera_enumframesize(index);
		index++;
	}
	gFramesize_index = index-1;
	return index;
}

int camera_freebuffer_userptr(void)
{
	int i;
    // Release the resource
    for (i=0; i< BUFFER_COUNT; i++) 
    {
        if(framebuf[i].vir) {
            ion_buf_free(&framebuf[i]);
        }
    }
	return 0;
}

int camera_allocbuffer_userptr(void)
{
	int i,ret;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers reqbuf;
    unsigned int width = gWidth;
    unsigned int hight = gHeight;
    unsigned int hight_16bytes_align = (((hight+15)/16)*16);
    unsigned int size = width*hight_16bytes_align*2;
	
    memset(framebuf, 0, sizeof(struct ion_buf)*BUFFER_COUNT);
    for(i=0; i<BUFFER_COUNT; i++) {
        ret = ion_buf_alloc((struct ion_buf *)&framebuf[i], size);
	DBG("size-w*h = (%d-%d*%d) =%d\n",framebuf[i].size ,gWidth, gHeight,  size-gWidth*gHeight);
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
        buf.length = gImagesize ;//framebuf[i].size;

        // Queen buffer
        ret = ioctl(gFd , VIDIOC_QBUF, &buf); 
        if(ret)
        	DBG("[%d]fail: VIDIOC_QBUF, errno = %d\n", __LINE__, errno);

        DBG("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].phy, buf.length);
    }
	return 0;
}

int camera_freebuffer_mmap(void)
{
	int i;
    for (i=0; i< BUFFER_COUNT; i++) 
    {
    	if(framebuf[i].vir != NULL)
        	munmap(framebuf[i].vir, framebuf[i].size);
    }
	return 0;
}

int camera_allocbuffer_mmap(void)
{
	int ret;
	int i;
    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP; 
    ret = ioctl(gFd , VIDIOC_REQBUFS, &reqbuf);
    if(ret) {
        DBG("[%d]fail: VIDIOC_REQBUFS, ret = %d\n", __LINE__, ret);
        return -1;
    }

    struct v4l2_buffer buf;
    for (i = 0; i < reqbuf.count; i++) 
    {
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(gFd , VIDIOC_QUERYBUF, &buf);  
        if(ret) {
			DBG("[%d]fail: VIDIOC_QUERYBUF\n", __LINE__);
			return -1;    
        }

        // mmap buffer
        framebuf[i].size = buf.length;
        framebuf[i].vir = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, gFd, buf.m.offset);
    
        // Queen buffer
        ret = ioctl(gFd , VIDIOC_QBUF, &buf); 
        if(ret)
        	printf("[%d]fail: VIDIOC_QBUF\n", __LINE__);

        printf("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].vir, framebuf[i].size);
    }

    return 0;
}

int camera_freebuffer(void)
{
	if(allocbuffer_type == 1)
    	camera_freebuffer_userptr();
	else
    	camera_freebuffer_mmap();

	return 0;
}

int camera_allocbuffer(void)
{
	if(allocbuffer_type == 1)
    		return camera_allocbuffer_userptr();
	else
    		return camera_allocbuffer_mmap();
}


int camera_streamoff(void)
{ 
	int ret;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(gFd, VIDIOC_STREAMOFF, &type);
    if(ret) {
        DBG("[%d]fail: VIDIOC_STREAMOFF\n", __LINE__);
        return -1;
    }

	return 0;
}

int camera_streamon(void)
{
	int ret;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(gFd, VIDIOC_STREAMON, &type);
    if(ret) {
        DBG("[%d]fail: VIDIOC_STREAMON\n", __LINE__);
        return -1;
    }

	return 0;
}

int camera_open_sub(char *filename)
{
    int ret = 0;
    int index;
    unsigned int i;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers reqbuf;
	
    //gFd = open(filename, O_RDWR | O_NONBLOCK, 0);
    gFd = open(filename, O_RDWR, 0);
    if(gFd < 0) {
    	DBG("open %s fail\n", filename);
        return -1;
    }

    camera_enumfmt_all();
    camera_enumframesize_all();
    camera_S_FMT_index(framesize_index_i, format_index_i);
    if(resolution_display)
        display_resolution_info();
    camera_G_FMT();
    
    ret =  camera_allocbuffer();
    if(ret) {
        close(gFd);
	return -1;
    }
    return 0;
}


int camera_open(void)
{
    int ret;
    char videoname[32];
    int idx = videoidx;
    do {
        sprintf(videoname, "%s%d", CAMERA_DEVICE, idx++);
        ret = camera_open_sub(videoname);
        if(ret >= 0) {
	    DBG("open %s OK!\n", videoname);
	    break;
        }
	    DBG("open %s fail, try next!\n", videoname);
    }while(idx < 4);

    if(ret < 0)
    {
        DBG("open video[0, 3] fail\n");
        return -1;
    }
    return 0;
}

int camera_close(void)
{
    camera_freebuffer();

    if(gFd)
        close(gFd);

    DBG("Camera test Done.\n");

    return 0;

}


int camera_getbuf(char *p, unsigned int size)
{
    int ret;
    struct v4l2_buffer buf;

    buf.index = gIndex;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(allocbuffer_type == 1)
        buf.memory = V4L2_MEMORY_USERPTR;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    // Get frame
    ret = ioctl(gFd, VIDIOC_DQBUF, &buf);   
    if(ret) {
        DBG("[%d]fail: VIDIOC_DQBUF: errno = %d\n", __LINE__, errno);
        return -1;
    }

    DBG("[%d] [seq=%d] [len=%d] [used=%d] [f=%x] [r=%d]\n", buf.index, buf.sequence, buf.length, buf.bytesused, buf.flags, buf.reserved);

    // Process the frame
    if(V4L2_PIX_FMT_MJPEG == gFmt)
        size = buf.bytesused;
    else
        size = buf.length;
    memcpy(p, framebuf[buf.index].vir, size);

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

int test_framebuf(void)
{
    int i;
    int ret = 0;
    unsigned int size = 0;
    struct timeval cur;
    char filename[32];
    int frame = 0;
    int framerate = set_framerate_val == 0 ? 30 : set_framerate_val;
    int saveframe_cnt = savefile_intval == 0 ? savefile_frame : savefile_frame * savefile_intval;
    int saveframe_idx = savefile_frame;

    unsigned int framesize = gWidth * gHeight * 2;
    char *buf = malloc(framesize);
    if(!buf) {
    	DBG("malloc buf fail\n");
    	return -1;
    }

    frame = set_framerate_val*getbuf_sec + getbuf_frame;
	if(saveframe_cnt >= frame)
			frame = saveframe_cnt + 2;

	printf("\n");
	DBG("get buf cnt = %d, framerate = %d\n", frame, framerate);
	if(savefile_en)
		DBG("save file en = %d, , saveframe_cnt = %d, savefile_intval = %d, savefile_frame = %d\n", savefile_en, saveframe_cnt, savefile_intval, savefile_frame);
	if(print_fps)
		DBG("print_fps = %d\n", print_fps);
		
	if(V4L2_PIX_FMT_MJPEG == gFmt)
		sprintf(capturename, ".jpeg");
	else
		sprintf(capturename, ".yuv");
		
    //get frame data
    for(i=0; i<frame; i++) {
        size = camera_getbuf(buf, framesize);
        if(size < 0) {
	    	goto framebuf_end;
        }

		if(print_fps) {
	        if(i%framerate == 0) {
	        	gettimeofday(&cur, NULL);
	        	DBG("[%10u.%06u] frame = %d\n", (unsigned int)cur.tv_sec, (unsigned int)cur.tv_usec, i);
	        }
		}
    	if(savefile_en == 1 && i == saveframe_idx)  {
                sprintf(filename, "frm%d_wh%d_fmt%d_file%s", i, framesize_index_i, format_index_i, capturename);
                FILE *fp = fopen(filename, "wb");
                unsigned int writesize = fwrite(buf, 1, size, fp);
		if(writesize != size)
		    DBG("writesize = %d, size = %d\n", writesize, size);

                fclose(fp);
                DBG("Capture one frame saved in %s, size = %d\n", filename, size);    
                
		sprintf(filename, "frm%d_wh%d_fmt%d_file_len%s", i, framesize_index_i, format_index_i, capturename);
                FILE *fp1 = fopen(filename, "wb");
                writesize = fwrite(buf, 1, framesize, fp1);
		if(writesize != framesize)
		    DBG("len:writesize = %d, framesize = %d\n", writesize, framesize);

                fclose(fp1);
                DBG("Capture one frame saved in %s, framesize = %d\n", filename, framesize);    
		saveframe_idx += savefile_frame;
    	}
    
    }
framebuf_end:
    if(buf) {
        free(buf);
        buf = NULL;
    }
    return ret;
}

void printf_info(void)
{
	printf("=================================================\n");
	printf("mmap: allocbuffer type = %s\n", allocbuffer_type == 1 ? "USERPTR" : "MMAP");
	printf("userptr: allocbuffer type = %s\n", allocbuffer_type == 1 ? "USERPTR" : "MMAP");
	printf("list_wh: dispaly resoution enable = %d\n", resolution_display);
	printf("set_wh [idx]: set resoution index = %d\n", framesize_index_i);
	printf("set_format [idx]: set format index = %d\n", format_index_i);
	printf("set_fps [val]: set framerate en = %d, fps = %d\n", format_index_i, set_framerate_val);
	printf("getbuf_sec [val]: get buf en = %d, set seconds  = %d\n", getbuf_en, getbuf_sec);
	printf("getbuf_frame [val]: get buf en = %d, set frame cnt = %d\n", getbuf_en, getbuf_frame);
	printf("savefile_frame [val]: get buf en = %d, savefile frame index = %d\n", getbuf_en, savefile_frame);
	printf("savefile_intval_frm [val] [cnt]: get buf en = %d, savefile frame index = %d, cnt = %d\n", getbuf_en, savefile_frame, savefile_intval);
	printf("print_fps: printf fps enable = %d\n", print_fps);
	printf("video [val]: open /dev/video%d\n", videoidx);
	printf("expo [val]: set exposure value = %d\n", expo);
	printf("freq [val]: set freq value = %d, 1=mono_60M, 2=mono_80M\n", freq);
	printf("=================================================\n");
	
}

int parse_argv(int argc, char **argv)
{
	int idx;
	for(idx = 1; idx < argc; idx++){
		if(argv[idx] == NULL)	break;

		if(strcmp(argv[idx], "mmap") == 0)	allocbuffer_type = 0;
		else if(strcmp(argv[idx], "userptr") == 0)	allocbuffer_type = 1;
		else if(strcmp(argv[idx], "list_wh") == 0)	resolution_display = 1;
		else if(strcmp(argv[idx], "print_fps") == 0)	{print_fps = 1;}
		
		else if(strcmp(argv[idx], "video") == 0 && argv[idx+1] != NULL)		videoidx = atoi(argv[++idx]);
		else if(strcmp(argv[idx], "freq") == 0 && argv[idx+1] != NULL)		freq = atoi(argv[++idx]);
		else if(strcmp(argv[idx], "expo") == 0 && argv[idx+1] != NULL)		expo = atoi(argv[++idx]);
		else if(strcmp(argv[idx], "set_wh") == 0 && argv[idx+1] != NULL)	framesize_index_i = atoi(argv[++idx]);
		else if(strcmp(argv[idx], "set_format") == 0  && argv[idx+1] != NULL)	format_index_i = atoi(argv[++idx]);
		else if(strcmp(argv[idx], "set_fps") == 0  && argv[idx+1] != NULL)	{set_framerate_en = 1; set_framerate_val = atoi(argv[++idx]);}
		else if(strcmp(argv[idx], "getbuf_sec") == 0  && argv[idx+1] != NULL)	{getbuf_en = 1; getbuf_sec = atoi(argv[++idx]);}
		else if(strcmp(argv[idx], "getbuf_frame") == 0  && argv[idx+1] != NULL)	{getbuf_en = 1; getbuf_frame = atoi(argv[++idx]);}
		else if(strcmp(argv[idx], "savefile_frame") == 0  && argv[idx+1] != NULL)	{getbuf_en = 1; savefile_en=1; savefile_frame = atoi(argv[++idx]);}
		else if(strcmp(argv[idx], "savefile_intval_frm") == 0  && argv[idx+1] != NULL  && argv[idx+2] != NULL)	
			{getbuf_en = 1; savefile_en=1; savefile_frame = atoi(argv[++idx]); savefile_intval = atoi(argv[++idx]);}
		
	}
	printf_info();
	return 0;
}

int main(int argc, char **argv)
{
    int ret;
    if(argc < 2) {
        printf_info();
        return 0;
    }    

    parse_argv(argc, argv);
    ret = camera_open();
    if(ret < 0)
    {
        return -1;
    }

    test_freq(freq);
    
    if(set_framerate_en)
        test_framerate(set_framerate_val);

#if 1 
 sleep(1);;printf("expo = %d\n", expo);
    test_exposure2(expo);
 sleep(1);;printf("expo = %d\n", expo);
#else
    sleep(1);printf("freq = 1\n");
    test_freq(freq);
    sleep(1);printf("freq = 1\n");


#endif


    camera_streamon();
   
    if(getbuf_en)
        test_framebuf();


    if(strcmp(argv[1], "expo") == 0)  {
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

sleep(1);printf("\n\n");
main_end:
	camera_streamoff();

    camera_close();

    return 0;
}

