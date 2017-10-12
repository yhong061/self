#include "calc_def_pi_delay_sorted.h"
#include "pru.h"
#include "api.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "config.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_default.h"
#include <math.h>
#include <sys/time.h>

static uint16_t pixelData[328 * 252 * 2];
static uint16_t pixelData_Mem[328 * 252 * 2 * 4];

int calcDefPiDelayGetDataSorted(enum calculationType type, uint16_t **data){
	
	struct timeval tv1,tv2,tv3;

	//printf("configGetCorrectionBGMode = %d\n", configGetCorrectionBGMode());  //0
	if(configGetCorrectionBGMode() !=0 ){
		uint16_t *pMem = NULL;
		int deviceAddress = configGetDeviceAddress();
		apiLoadConfig(0, deviceAddress);	//Grayscale
		apiGetBWSorted(&pMem);
		apiLoadConfig(1, deviceAddress);	//THREED
	}

	gettimeofday(&tv1, NULL);
	int size = pruGetImage(data);
	//int size = configGetImage(data);
	gettimeofday(&tv2, NULL);

	if(configGetCorrectionBGMode() !=0 ){
		calculationBGCorrection(data, 1);
	}
	memcpy(pixelData_Mem, *data, size*sizeof(uint16_t));

	//uint16_t *pMem = *data;
	uint16_t *pMem = pixelData_Mem;
	int nPixelPerDCS = size / 4;
	calculatorInit(2);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves());

	//printf("cols= %d  size= %d  nPixelPerDCS= %d  rows = %d\n", pruGetNCols(), size, nPixelPerDCS, pruGetNRowsPerHalf() * pruGetNumberOfHalves());

	printf("\n");
	while(iteratorDefaultHasNext()){
		struct Position p = iteratorDefaultNext();
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];
		uint16_t pixelDCS2 = pMem[p.indexMemory + 2 * nPixelPerDCS];
		uint16_t pixelDCS3 = pMem[p.indexMemory + 3 * nPixelPerDCS];

		calculatorSetArgs4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3, p.indexMemory, p.indexMemory);
		
		struct TofResult tofResult;
		switch(type){
		case DIST:
			pixelData[p.indexSorted] = calculatorGetDistAndAmpSine().dist;
			break;
		case AMP:
			pixelData[p.indexSorted] = calculatorGetAmpSine();
			break;
		case INFO:
			tofResult = calculatorGetDistAndAmpSine();
			pixelData[p.indexSorted] = tofResult.dist;
			pixelData[p.indexSorted + nPixelPerDCS] = tofResult.amp;
			break;
		}
	}
	printf("\n");
	gettimeofday(&tv3, NULL);
	unsigned long laspe32 = (tv3.tv_sec*1000000 + tv3.tv_usec) - (tv2.tv_sec*1000000 + tv2.tv_usec);
	unsigned long laspe21 = (tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec);

	printf("%lu.%06lu - %lu.%06lu - %10lu.%06lu, 3-2=%lu, 2-1=%lu\n"
		, (unsigned long)tv3.tv_sec, (unsigned long)tv3.tv_usec
		, (unsigned long)tv2.tv_sec, (unsigned long)tv2.tv_usec
		, (unsigned long)tv1.tv_sec, (unsigned long)tv1.tv_usec
		, laspe32, laspe21);
	*data = pixelData;
	if (type == INFO){
		return nPixelPerDCS * 2;
	}
	

	return nPixelPerDCS;
}
