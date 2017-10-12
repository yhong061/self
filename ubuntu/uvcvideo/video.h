#ifndef __VIDEO_H__
#define __VIDEO_H__

enum FMT_TYPE {
	FMT_TYPE_MULTIFREQ = 0,
	FMT_TYPE_MONOFREQ,
	FMT_TYPE_MONOFREQ_80M,
	FMT_TYPE_GRAY_DEPTH,
	FMT_TYPE_END
};

int camera_open(char *filename, enum FMT_TYPE fmttype);
int camera_close(void);
int camera_getbuf(char *p, unsigned int size);
int camera_getbuf2(char *p1, unsigned int *s1, char *p2, unsigned int *s2);
int camera_changefmt(enum FMT_TYPE fmttype);

int camera_get_framerate(int *framerate);
int camera_set_framerate(int framerate);
int camera_get_exposure_query(int *min, int *max, int *step);
int camera_get_exposure(int *value);
int camera_set_exposure(int value);

int camera_reg_open(void);
int camera_reg_close(void);
float temperature_read(void);

/* filename: eeprom data save to filename*/
int eeprom_read(char *filename);

#endif //__VIDEO_H__
