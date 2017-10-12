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

static uint16_t pixelData[328 * 252 * 2];

int calcDefPiDelayGetDataSorted(enum calculationType type, uint16_t **data){

	if(configGetCorrectionBGMode() !=0 ){
		uint16_t *pMem = NULL;
		int deviceAddress = configGetDeviceAddress();
		apiLoadConfig(0, deviceAddress);	//Grayscale
		apiGetBWSorted(&pMem);
		apiLoadConfig(1, deviceAddress);	//THREED
	}

	int size = pruGetImage(data);
	//int size = configGetImage(data);

	if(configGetCorrectionBGMode() !=0 ){
		calculationBGCorrection(data, 1);
	}

	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 4;
	calculatorInit(2);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves());

	//printf("cols= %d  size= %d  nPixelPerDCS= %d  rows = %d\n", pruGetNCols(), size, nPixelPerDCS, pruGetNRowsPerHalf() * pruGetNumberOfHalves());
	int tmp_idx = 0;
	int tmp_offset = 0x10;
	while(iteratorDefaultHasNext()){
		struct Position p = iteratorDefaultNext();
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];
		uint16_t pixelDCS2 = pMem[p.indexMemory + 2 * nPixelPerDCS];
		uint16_t pixelDCS3 = pMem[p.indexMemory + 3 * nPixelPerDCS];
		if(p.indexSorted == tmp_offset/2) {
			printf("dsc[%x %x %x %x]\n", pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3);
		}
		//if(tmp_idx%1000 == 0) {
		//	printf("p.indexMemory = %d, p.indexSorted = %d, nPixelPerDCS = %d\n", p.indexMemory, p.indexSorted, nPixelPerDCS);
		//}

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
		if(p.indexSorted == tmp_offset/2) {
			printf("test[%x]\n", tofResult.dist);
		}
			pixelData[p.indexSorted] = tofResult.dist;
			pixelData[p.indexSorted + nPixelPerDCS] = tofResult.amp;
			break;
		}
			tmp_idx++;
	}
	*data = pixelData;
	test_saveFile("Dist", (char *)*data,  nPixelPerDCS * 2 * sizeof(uint16_t));
	if (type == INFO){
		return nPixelPerDCS * 2;
	}
	

	return nPixelPerDCS;
}
