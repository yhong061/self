
#define FRAMERATE_PATH "/data/local/tmp"
pthread_t gThread_frame;
struct uvc_video gVideo_st;

void* thread_frame(void*)
{
	struct uvc_video *video_st;
	struct uvc_video uvc_frame;
	unsigned int serail_d;
	unsigned long long time;
	struct uvc_gadget *gadget;
	char *buf;
	char filename[32];
	FILE *fp;

	buf = (char *)malloc(256);
	if(buf == NULL) {
		ERR("malloc buf fail in thread_frame()\n");
		return NULL;
	}

	while (!is_exit) {
		if(video_output_streamon) {
			video_st = (struct uvc_video *)&gVideo_st;
			memcpy(&uvc_frame, video_st, sizeof(struct uvc_video));
			video_st->last_serial = uvc_frame.serial;
			memcpy(&video_st->last_tv, &uvc_frame.tv, sizeof(struct timeval));
			serail_d = uvc_frame.serial - uvc_frame.last_serial;
			time = (uvc_frame.tv.tv_sec*1000000 + uvc_frame.tv.tv_usec) - (uvc_frame.last_tv.tv_sec*1000000 + uvc_frame.last_tv.tv_usec);
			sprintf(buf, "Framerate : [T: %02u.%06u] [F: %02u]\n",  (unsigned int)time/1000000, (unsigned int)time%1000000, serail_d);
			//fprintf(stderr, "%s", buf);fflush(stderr);
			{
				sprintf(filename, "%s/framerate_%d", FRAMERATE_PATH, uvc_frame.serial%10);
				fp = fopen(filename, "wb");
				if(fp) {
					fwrite(buf, 256, 1, fp);
					fclose(fp);
				}
			}
		}
		sleep(1);
	}

	if(buf != NULL) {
		free(buf);
		buf = NULL;
	}

	return NULL;
}

int uvc_thread_framerate_create(void *param)
{
	int ret = pthread_create(&gThread_frame, NULL, &thread_frame, param);
	if(ret != 0) {
		ERR("create thread_frame fail\n");
		return -1;;
	}
	DBG("framerate info save to : %s\n", FRAMERATE_PATH);
	return 0;
}

int uvc_thread_framerate_join(void)
{
	pthread_join(gThread_frame,NULL);
	return 0;
}

static void uvc_gadget_frame_init(void)
{
	struct timeval tv;
	struct uvc_video *video_st = (struct uvc_video *)&gVideo_st;

	gettimeofday(&tv, NULL);

	memset(video_st, 0, sizeof(struct uvc_video));
	memcpy(&video_st->base_tv, &tv, sizeof(struct timeval));
	memcpy(&video_st->last_tv, &tv, sizeof(struct timeval));
	memcpy(&video_st->tv, &tv, sizeof(struct timeval));
	//DBG("tv:  %u.%u\n", (unsigned int)video_st->tv.tv_sec, (unsigned int)video_st->tv.tv_usec);

	return;
}

static void uvc_gadget_frame_add(void)
{
	struct uvc_video *video_st = (struct uvc_video *)&gVideo_st;
	video_st->serial++;
	gettimeofday(& video_st->tv, NULL);
	return;
}

#define SAVEFILE_PATH "/data/local/tmp"
static int gSavefile = 0;

void uvc_gadget_savefile_flag(int flag)
{
	gSavefile = flag;
}

static int uvc_gadget_savefile_Sub(char *filename, char *buf, unsigned int size)
{
	FILE *fp = fopen(filename, "wb");
	if(!fp) {
		printf("open %s fail\n", filename);
		return 0;
	}

	unsigned int writesize = fwrite(buf, 1, size, fp);
	if(writesize != size) {
		printf("write file %s : writesize is %d, realsize is %d\n", filename, writesize, size);
	}
	fclose(fp);
	printf("savefile %s OK\n", filename);
	return 0;
}

static int uvc_gadget_savefile(struct uvc_gadget *gadget)
{
	char filename[64];
	struct timeval tv;
	int *pDepth = (int *)gadget->depthbuf;
	int *pGray = (int *)gadget->graybuf;
	struct PointCloud *pCloud = (struct PointCloud *)gadget->databuf;
	char *ptr;
	unsigned int size;

	printf("\n");
	gettimeofday(&tv, NULL);
	filename[0] = '\0';
	sprintf(filename, "%s/file_%ld_%06ld", SAVEFILE_PATH, tv.tv_sec, tv.tv_usec);
	unsigned int len = strlen(filename);

	filename[len] = '\0';
	strcat(filename, ".gray");
	size = gadget->graysize;
	ptr = (char *)pGray;
	uvc_gadget_savefile_Sub(filename, ptr, size);

	filename[len] = '\0';
	strcat(filename, ".depth");
	size = gadget->depthsize;
	ptr = (char *)pDepth;
	uvc_gadget_savefile_Sub(filename, ptr, size);

	filename[len] = '\0';
	strcat(filename, ".cloud");
	size = 224 * 172 * sizeof(struct PointCloud);
	ptr = (char *)pCloud;
	uvc_gadget_savefile_Sub(filename, ptr, size);
	
	filename[len] = '\0';
	strcat(filename, ".z");
	size = 224 * 172 * sizeof(float);
	char *buftmp = (char *)malloc(size);
	assert(buftmp);
	{
		struct PointCloud *in = pCloud;
		float *out = (float *)buftmp;
		unsigned int i;
		for(i=0; i<224*172; i++,out++,in++)
			*out = in->z;
	}
	ptr = (char *)buftmp;
	uvc_gadget_savefile_Sub(filename, ptr, size);
	free(buftmp);


	return 0;
}

static void uvc_gadget_savefile_check(struct uvc_gadget *gadget)
{
	if(gSavefile) {
		uvc_gadget_savefile(gadget);
		uvc_gadget_savefile_flag(0);
	}
	return;
}


static char EEPROM_FILE[]  = "/data/eeprom.bin";
#define SPECTRE_PATH	"/data"
#define RAW_WIDTH 224
#define RAW_HEIGHT 172
int spectre_update(void)
{
	unsigned int time = 0;
	FILE *fp;
	int size = RAW_WIDTH * RAW_HEIGHT * 4 * 4;
	char *spectre_buf = (char *)malloc(size + 256);
	char *gray_buf = (char *)malloc(size + 256);
	assert(spectre_buf);
	assert(gray_buf);

	camera_open(CAMERA_DEVICE, FMT_TYPE_MULTIFREQ);

	spectre_use_single_frame(false, 0);
	spectre_produce(spectre_buf, gray_buf, size, time);

	camera_close();

	free(spectre_buf);
	free(gray_buf);
	
	sleep(1);
	return 0;

}

int spectre_unpack(struct uvc_gadget *gadget, int use_eeprom_data)
{
	FILE *fp = NULL;
	char filename[128];
	char cmd[128];
	int ret;

	filename[0] = '\0';
	cmd[0] = '\0';

	video_output_streamon = 0;
	uvc_gadget_camera_close(gadget, 0);
	sleep(2);

	sprintf(cmd, "rm -rf %s/spectre\n", SPECTRE_PATH);
	system(cmd);

	sprintf(cmd, "busybox tar zxf /system/spectre/spectre.tar.gz -C %s\n", SPECTRE_PATH);
	system(cmd);

	if(use_eeprom_data) {
		sprintf(cmd, "rm -rf %s/spectre/sample/Process_Calib_test/*\n", SPECTRE_PATH);
		system(cmd);

		sprintf(cmd, "cp -rf %s %s/spectre/sample/Process_Calib_test/\n", EEPROM_FILE, SPECTRE_PATH);
		system(cmd);
	}
	else {
		sprintf(filename, "%s/spectre/sample/Process_Calib_test/calib_version.bin", SPECTRE_PATH);
		fp = fopen(filename, "rb");
		if(fp != NULL) {
			fclose(fp);
			ERR("%s exist, skip spectre init\n", filename);
			goto spectre_end;
		}
	}

	spectre_update();

	sprintf(cmd, "chmod 666 %s/spectre/sample/Process_Calib_test/calib_*\n", SPECTRE_PATH);
	system(cmd);

	sprintf(filename, "%s/dmgesture/JULI_model.bin", SPECTRE_PATH);
	fp = fopen(filename, "rb");
	if(fp != NULL) {
		fclose(fp);
		ERR("%s exist, skip unpack JULI_model.bin\n", filename);
		goto spectre_end;
	}

	sprintf(cmd, "busybox tar zxf /system/dmgesture/dmgesture.tar.gz -C %s\n", SPECTRE_PATH);
	system(cmd);

	sprintf(cmd, "chmod 666 %s/dmgesture/*\n", SPECTRE_PATH);
	system(cmd);

spectre_end:
	ret = uvc_gadget_camera_open(gadget);
	if(ret < 0) {
		ERR("open /dev/video0 fail\n");
		return -1;;
	}
	sleep(2);
	video_output_streamon = 1;
	DBG("unpack spectre OK\n");
	
	return 0;
}

#define EEPROM_SERIAL_SIZE (19)
#define EEPROM_SERIAL_OFFSET (0x48)

int eeprom_serial_compare(void) {
	int ret = -1;
	unsigned int offset = EEPROM_SERIAL_OFFSET;
	unsigned int size = EEPROM_SERIAL_SIZE;
	char *eeprom_buf = (char *)malloc(size);
	char *file_buf = (char *)malloc(size);

	FILE *fp = NULL;
	unsigned int readsize;
	char filename[128];

	assert(eeprom_buf);
	assert(file_buf);
	memset(eeprom_buf, 0, size);
	memset(file_buf, 0, size);

	filename[0] = '\0';
	sprintf(filename, "%s/spectre/sample/Process_Calib_test/calib_SensorSerial.bin", SPECTRE_PATH);
	fp = fopen(filename, "rb");
	if(fp == NULL) {
		ERR("open %s fail\n", filename);
		goto eeprom_exit;
	}
	
	readsize= fread(file_buf, 1, size, fp);
	fclose(fp);

	ret = eeprom_read_special((unsigned char *)eeprom_buf, offset, size);
	if(ret) {
		ERR("read eeprom data fail: offset = 0x%x, size = %d\n", offset, size);
		goto eeprom_exit;
	}

	if(strncmp(eeprom_buf, file_buf, size) != 0) {//different
		ERR("eeprom serial is different. %s != %s\n", eeprom_buf, file_buf);
		ret = 1;
	}
	else {
		ERR("eeprom serial is same. %s\n", eeprom_buf);
		ret = 0;
	}

eeprom_exit:
	free(eeprom_buf);
	free(file_buf);

	return ret;
}

int eeprom_calib_func(struct uvc_gadget *gadget)
{
	int flag;
	flag = eeprom_serial_compare();
	if(flag != 1) {
		return flag;
	}
	
	ERR("start read eeprom data from sensor, it will take about 2 minutes ...\n");
	sleep(1);
	eeprom_read(EEPROM_FILE); 
	ERR("read eeprom data end, unpacked start...\n");
	sleep(1);
	spectre_unpack(gadget, 1);
	ERR("read eeprom data end, unpacked end...\n");
	sleep(1);
	return 0;
}

