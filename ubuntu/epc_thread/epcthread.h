#ifndef __EPCTHREAD_H__
#define __EPCTHREAD_H__
#include <pthread.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define DBG(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)
#define ERR(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)

#define DATA_PATH "."

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

EpcConfig_st *EpcConfig_Get(void);
EpcCalcCfg_st *EpcCalcConfig_Get(void);
EpcCalcCfg_st* EpcCalcConfig_Init(void);
int32_t* calibrationGetCorrectionMap();
#endif
