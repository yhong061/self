#include "calc_mgx_no_pi_delay_sorted.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_mgx.h"
#include <math.h>

static uint16_t pixelData[328 * 252 * 2];

int calcMGXNoPiDelayGetDataSorted(enum calculationType type, uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size;
	int nCols = pruGetNCols();
	uint16_t nHalves = pruGetNumberOfHalves();
	calculatorInit(1);
	iteratorMGXInit(nPixelPerDCS, nCols, pruGetNRowsPerHalf() * pruGetNumberOfHalves());
	while(iteratorMGXHasNext()){
		struct Position p = iteratorMGXNext();
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nHalves * nCols];

		calculatorSetArgs2DCS(pixelDCS0, pixelDCS1, p.indexMemory, p.indexCalibration);

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
	*data = pixelData;
	if (type == INFO){
		return nPixelPerDCS;
	}
	return nPixelPerDCS / 2;
}
