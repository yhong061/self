#ifndef __EPCTHREAD_H__
#define __EPCTHREAD_H__
#include <pthread.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#define DBG(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)
#define ERR(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)

#define DATA_PATH "./tmp"

#define WIDTH_MAX	(328)
#define HEIGHT_MAX	(252)

#define WIDTH	(320)
#define HEIGHT	(240)
#define DATA_INFO_SIZE (256)

#define MAX_DIST_VALUE 	3000
#define LOW_AMPLITUDE 	30000
#define SATURATION 		31000
#define ADC_OVERFLOW 	32000
#define MODULO_SHIFT 	30000
#define FP_M_2_PI 		3217.0 	/*! 2 PI in fp_s22_9 format*/
#define FP_M_PI			1608.5;

#define fp_fract 9
#define coeff_1 101
#define coeff_2 503
#define coeff_3 402

enum calculationType {
	DIST, AMP, INFO
} ;



typedef unsigned int bool;
typedef unsigned short int uint16_t;
typedef short int int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
//typedef char int8_t;

typedef struct _EpcPruInfo_ {
	unsigned int 	icVersion;
	unsigned int 	partType;
	unsigned int 	partVersion;
	uint16_t 		waferID;
	uint16_t 		chipID;

	unsigned char gX;
	unsigned char gY;
	unsigned char gM;
	unsigned char gC;
}EpcPruInfo_st;

typedef struct _EpcCalcCfg_ {
	pthread_mutex_t mMutex;
	unsigned int nColsMax;
	unsigned int nCols;
	unsigned int nRowsMax;
	unsigned int nRows;
	unsigned int nRowsPerHalf;
	unsigned int nHalves;
	
	int rowReduction;
	int colReduction;
	int numberOfHalves;

	int dualMGX;
	int piDelay;
	int pn;
	int minAmplitude;

	uint16_t pixelMask;

	int saturation;
	uint16_t saturationBit;

	int adcOverflow;
	uint16_t adcMin;
	uint16_t adcMax;

	int hysteresis;
	int upperThreshold;
	int lowerThreshold;
	unsigned char *hysteresisMap;

	int calibrationEnable;
	int availableCalibration;
	int offsetPhaseDefault;
	double calibrationTemperature;
	int offsetPhaseDefaultEnabled;
	int offsetPhase;
	
	int32_t *correctionMap;
	int32_t *subCorrectionMap;
}EpcCalcCfg_st;



typedef struct _EpcDataInfo_ {
	unsigned int 		mBufIdx;
	unsigned int 		mWidth;
	unsigned int 		mHeight;
	unsigned int 		mDcs;
	unsigned int 		mSequence;
	unsigned int 		mDatasize;
	struct timeval		mTv;
}EpcDataInfo_st;

typedef struct _EpcCalcInfo_ {
	char 			*pDatabuf;
	unsigned int 	mDatabufSize;
	unsigned int 	mDatabufOffset;
	char 			*pCalcbuf;
	unsigned int 	mCalcbufSize;
	unsigned int 	mCalcbufOffset;
	EpcCalcCfg_st	*pCalcCfg;
}EpcCalcInfo_st;


struct _EpcList_ {
	char				*pBuf;
	struct _EpcList_	*pNext;	
};
typedef struct _EpcList_ EpcList_st;

typedef struct _EpcBuffer_ {
	unsigned int		mBufferSize;
	unsigned int		mBufferOffset;
	int    				mBufferCnt;
	int    				mBufferUsed;
	char				*pData;
	
	pthread_mutex_t 	mMutex;

	EpcList_st 			*pEmpty;
	EpcList_st 			*pFull;
}EpcBuffer_st;

typedef struct _EpcData_Range_ {
	unsigned int 		mMin;
	unsigned int 		mMax;
	unsigned int 		mCur;
}EpcData_Range_st;

typedef struct _EpcData_ {
	unsigned int 		mWidth;
	unsigned int 		mHeight;
	unsigned int 		mPixel;
	unsigned int 		mDcs;
	unsigned int 		mImageSize;
	
	EpcData_Range_st	mFilesRange;
}EpcData_st;

typedef struct _EpcCalcThr_ {
	unsigned int		mThreadIdx;
	pthread_t 			mThread;
	unsigned int		mCreated;
	unsigned int		mRunning;
}EpcCalcThr_st;

#define CALC_THR_NUM	(3)
#define BUFFER_NUM	(6)
typedef struct _EpcCalc_ {
	unsigned int 		mThrNum;
	pthread_mutex_t 	mMutex;
	unsigned int 		mWidth;
	unsigned int 		mHeight;
	unsigned int 		mPixel;
	unsigned int 		mDistSize;
	unsigned int 		mAmpSize;
//	EpcCalcCfg_st		*pCalcCfg;
	
	EpcCalcThr_st		*pThread[CALC_THR_NUM];
}EpcCalc_st;

typedef struct _EpcConfig_ {
	EpcData_st			*pData;
	EpcBuffer_st		*pDataBuffer;
	EpcCalc_st			*pCalc;
	EpcBuffer_st		*pCalcBuffer;
}EpcConfig_st;

EpcConfig_st gEpccfg;

static inline int32_t __EpcCalc_fp_mul(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((int32_t)((int32_t)a * b)>>fp_fract);

    return r;
}

static inline int32_t __EpcCalc_fp_div(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((((int32_t)a<<fp_fract))/b);

    return r;
}


static inline int32_t EpcCalc_fp_atan2(int16_t y, int16_t x)
{
    int32_t fp_y = ((int32_t)y<<fp_fract);
    int32_t fp_x = ((int32_t)x<<fp_fract);

    int32_t mask = (fp_y >> 31);
    int32_t fp_abs_y = (mask + fp_y) ^ mask;

    mask = (fp_x >> 31);
    int32_t fp_abs_x = (mask + fp_x) ^ mask;

    int32_t r, angle;

    if((fp_abs_y < 100) && (fp_abs_x < 100))
        return 0;


    if (x>=0) { /* quad 1 or 2 */
        /* r = (x - abs_y) / (x + abs_y)  */
        r = __EpcCalc_fp_div((fp_x - fp_abs_y), (fp_x + fp_abs_y));

        /* angle = coeff_1 * r^3 - coeff_2 * r + coeff_3 */
        angle =
            __EpcCalc_fp_mul(coeff_1, __EpcCalc_fp_mul(r, __EpcCalc_fp_mul(r, r))) -
            __EpcCalc_fp_mul(coeff_2, r) + coeff_3;
    } else {
        /* r = (x + abs_y) / (abs_y - x); */
        r = __EpcCalc_fp_div((fp_x + fp_abs_y), (fp_abs_y - fp_x));
        /*        angle = coeff_1 * r*r*r - coeff_2 * r + 3*coeff_3; */
        angle =
            __EpcCalc_fp_mul(coeff_1, __EpcCalc_fp_mul(r, __EpcCalc_fp_mul(r, r))) -
            __EpcCalc_fp_mul(coeff_2, r) + 3 * coeff_3;
    }

    if (y < 0)
        return(-angle);     // negate if in quad 3 or 4
    else
        return(angle);
}

EpcConfig_st *EpcConfig_Get(void);
EpcCalcCfg_st *EpcCalcConfig_Get(void);
EpcCalcCfg_st* EpcCalcConfig_Init(void);

int EpcCalc_calibrationGetOffsetPhase();
int32_t* EpcCalc_calibrationGetCorrectionMap();
int EpcCalc_pruGetMinAmplitude();

int EpcCalc_pruGetNCols();
int EpcCalc_pruGetNRowsPerHalf();
int EpcCalc_pruGetNumberOfHalves();
int EpcCalc_GetpixelMask();
int EpcCalc_saturationCheck(uint16_t saturationData);
int EpcCalc_adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3);
int EpcCalc_hysteresisUpdate(const int position, const int value);

int EpcCalc_CalibrationLoadOffset(const int storeID, EpcPruInfo_st *pruinfo);
void EpcCalc_CalibrationSetCorrectionMap(EpcPruInfo_st *pruinfo);
int EpcCalc_CalibrationLoadFPN(const int storeID, EpcPruInfo_st *pruinfo);
int EpcCalc_CalibrationGetOffsetPhase();
int32_t* EpcCalc_CalibrationGetCorrectionMap();


int EpcPru_Init_waferID_160_chipID_49(EpcPruInfo_st *pruinfo);
int EpcPru_Init_waferID_172_chipID_110(EpcPruInfo_st *pruinfo);
int EpcPru_Init(EpcPruInfo_st *pruinfo);
int EpcCalcConfig_UnInit(EpcCalcCfg_st* calccfg);

int EpcCalc_test_saveFile(char *buf, unsigned int size, unsigned int sequence);


int EpcCalc_apiGetDistanceAndAmplitudeSorted(EpcCalcInfo_st *calcinfo);

#endif
