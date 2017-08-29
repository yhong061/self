#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "video.h"

#define CAMERA_DEVICE "/dev/video1"
#define CAPTURE_FILE "frame.raw"
#define DEPTH_FILE "depth.raw"
#define FRAME_RATE	(15)
#define FRAME_WIDTH	(224)
#define FRAME_HEIGTH	(1557)

#define EEPROM_FILE "/data/eeprom.bin"

int test_eeprom(void) {
    if(!access(EEPROM_FILE, R_OK)) {
    	printf("%s file exist, return\n", EEPROM_FILE);
        return 0;
    }

    sleep(1);printf("\n\n");
    eeprom_read(EEPROM_FILE); 
    sleep(1);printf("\n\n");

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
    int count = 0;
    enum FMT_TYPE fmttype = FMT_TYPE_MULTIFREQ;
    int bufsize;

    if(!buf) {
    	printf("malloc buf fail\n");
	return -1;
    }

    if(sec > 0)
        frame = FRAME_RATE*sec;

    //get frame data
    for(i=0; i<frame; i++) {
        bufsize = camera_getbuf(buf, size);
        if(bufsize < 0) {
	    goto framebuf_end;
        }


	if(i%FRAME_RATE == 0) {
		gettimeofday(&cur, NULL);
        	printf("[%10u.%06u] frame = %d\n", (unsigned int)cur.tv_sec, (unsigned int)cur.tv_usec, i);
	}
	
    	if(save_file && i%FRAME_RATE == 0)  {
                sprintf(filename, "%d%s", i, CAPTURE_FILE);
                FILE *fp = fopen(filename, "wb");
                unsigned int writesize = fwrite(buf, 1, bufsize, fp);
                fclose(fp);
                printf("Capture one frame saved in %s, writesize = %d, bufsize = %d\n", filename, writesize, bufsize);  

		if(save_file == 2) {
			if(fmttype == FMT_TYPE_MULTIFREQ)
				fmttype = FMT_TYPE_MONOFREQ;
			else
				fmttype = FMT_TYPE_MULTIFREQ;

			camera_changefmt(fmttype);
		}
    	}
    }
framebuf_end:
    if(buf) {
        free(buf);
	buf = NULL;
    }
    return ret;
}

int test_framebuf2(int sec, int save_file)
{
    char filename[32];
    int frame = FRAME_RATE;
    int i;
    int ret = 0;
    unsigned int size = FRAME_WIDTH * FRAME_HEIGTH * 2;
    char *buf = malloc(size);
    char *depth = malloc(size);
    unsigned int bufsize, depthsize;
    struct timeval cur;
    int count = 0;
    enum FMT_TYPE fmttype = FMT_TYPE_MULTIFREQ;

    if(!buf || !depth) {
    	printf("malloc buf fail\n");
        if(buf) {
            free(buf);
	    buf = NULL;
        }
        if(depth) {
	    free(depth);
	    depth = NULL;
        }
	return -1;
    }

    if(sec > 0)
        frame = FRAME_RATE*sec + 5;

    //get frame data
    for(i=0; i<frame; i++) {
        ret = camera_getbuf2(buf, &bufsize, depth, &depthsize);
        if(ret <  0) {
	    goto framebuf_end;
        }
	
    	if(save_file && i%FRAME_RATE == 0)  {
                sprintf(filename, "%d%s", i, CAPTURE_FILE);
                FILE *fp = fopen(filename, "wb");
                unsigned int writesize = fwrite(buf, 1, bufsize, fp);
                fclose(fp);
                printf("Capture one frame saved in %s, writesize = %d, bufsize = %d\n", filename, writesize, bufsize);  

                sprintf(filename, "%d%s", i, DEPTH_FILE);
                FILE *fp1 = fopen(filename, "wb");
                unsigned int writesize1 = fwrite(depth, 1, depthsize, fp1);
                fclose(fp1);
                printf("Capture one frame saved in %s, writesize = %d, bufsize = %d\n", filename, writesize1, depthsize);  

		if(save_file == 2) {
			if(fmttype == FMT_TYPE_MULTIFREQ)
				fmttype = FMT_TYPE_MONOFREQ;
			else
				fmttype = FMT_TYPE_MULTIFREQ;

			camera_changefmt(fmttype);
		}
    	}
    }
framebuf_end:
    if(buf) {
        free(buf);
	buf = NULL;
    }
    if(depth) {
        free(depth);
	depth = NULL;
    }
    return ret;
}

int test_temperature(void)
{
    float temperature;
    int i,count = 5;

    sleep(1);printf("\n\n");
    for(i=0; i<count; i++) {
	//get temperature
	temperature = temperature_read();
	printf("temperature = [%.4f]\n", temperature);
	sleep(1);
    }
    sleep(1);printf("\n\n");

    return 0;
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
#if 0    
    for(value=min; value<=max; value+=step){
	ret = camera_set_exposure(value);
	if(ret) {
	    printf("set exposure [%d] fail, ret = %d\n", value, ret);
	}
	printf("\n");

	camera_get_exposure(&gvalue);
	if(ret) 
	    printf("get exposure fail, ret = %d\n", ret);
	else
	    printf("get exposure OK: gvalue = %d\n", gvalue);
    }
#else

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
#endif

    return ret;
}

int test_framerate(void)
{
    int ret = 0;
    int framerate;

    ret = camera_get_framerate(&framerate);
    if(ret) 
        printf("get framerate fail, ret = %d\n", ret);
    else
        printf("get framerate OK: framerate = %d\n", framerate);

    framerate = 15;
    ret = camera_set_framerate(framerate);
    if(ret) {
        printf("set framerate [%d] fail, ret = %d\n", framerate, ret);
    }

    framerate = 30;
    ret = camera_set_framerate(framerate);
    if(ret) {
        printf("set framerate [%d] fail, ret = %d\n", framerate, ret);
    }
    return ret;
}


void usage_help(void)
{
    printf("===========================================================\n");
    printf("video_main depth 1             : get depth data for sec, such as 1 sec \n");
    printf("video_main data 1              : get frame data for sec, such as 1 sec \n");
    printf("video_main yuv 1               : save yuv image: 1 sec \n");
    printf("video_main fmt 1               : change image fmt and save data: 1 sec \n");
    printf("video_main temp                : get temperature data\n");
    printf("video_main eeprom              : get eeprom data\n");
    printf("video_main expo                : get exposure data\n");
    printf("video_main fps                 : get framerate\n");
    printf("===========================================================\n");
}

int main(int argc, char **argv)
{
    int ret;
    enum FMT_TYPE fmttype = FMT_TYPE_GRAY_DEPTH; // FMT_TYPE_MONOFREQ; //FMT_TYPE_MULTIFREQ;
    if(argc < 2) {
        usage_help();
	return 0;
    }    

    ret = camera_open(CAMERA_DEVICE, fmttype);
    if(ret < 0) {
	return -1;
    }

sleep(1);printf("\n\n");
    if(strcmp(argv[1], "data") == 0)  {
        if(argc < 3) {
	    usage_help();
	    goto main_end;
	}

	int sec = atoi(argv[2]);
	int save_file = 0;
	printf("sec = %d\n", sec);

        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "yuv") == 0)  {
        if(argc < 3) {
	    usage_help();
	    goto main_end;
	}

	int sec = atoi(argv[2]);
	int save_file = 1;

        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "depth") == 0)  {
        if(argc < 3) {
	    usage_help();
	    goto main_end;
	}

	int sec = atoi(argv[2]);
	int save_file = 1;

        test_framebuf2(sec, save_file);
        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "fmt") == 0)  {
        if(argc < 3) {
	    usage_help();
	    goto main_end;
	}

	int sec = atoi(argv[2]);
	int save_file = 2;

        test_framebuf(sec, save_file);
    }
    else if(strcmp(argv[1], "expo") == 0)  {
        if(argc < 3) {
	    usage_help();
	    goto main_end;
	}

	int exposure = atoi(argv[2]);
	printf("exposure = %d\n", exposure);

        test_exposure(exposure);
    }
    else if(strcmp(argv[1], "temp") == 0) 
        test_temperature();
    else if(strcmp(argv[1], "fps") == 0) 
        test_framerate();
    else if(strcmp(argv[1], "eeprom") == 0) 
        test_eeprom();

main_end:
    camera_close();

    return 0;
}
