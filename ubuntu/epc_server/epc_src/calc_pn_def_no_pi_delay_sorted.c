#include "calc_pn_def_no_pi_delay_sorted.h"
#include "calculation.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_default.h"
#include <math.h>
#include <stdlib.h>

static uint16_t pixelData[328 * 252 * 2];

int calcPNDefNoPiDelayGetDataSorted(enum calculationType type, uint16_t **data){
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	calculatorInit(1);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves());
	while(iteratorDefaultHasNext()){
		struct Position p = iteratorDefaultNext();
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];

		calculatorSetArgs2DCS(pixelDCS0, pixelDCS1, p.indexMemory, p.indexMemory);
		struct TofResult tofResult;
		switch(type){
		case DIST:
			pixelData[p.indexSorted] = calculatorGetDistAndAmpPN().dist;
			break;
		case AMP:
			pixelData[p.indexSorted] = calculatorGetAmpPN();
			break;
		case INFO:
			tofResult = calculatorGetDistAndAmpPN();
			pixelData[p.indexSorted] = tofResult.dist;
			pixelData[p.indexSorted + nPixelPerDCS] = tofResult.amp;
			break;
		}
	}
	*data = pixelData;
	if (type == INFO){
		return nPixelPerDCS * 2;
	}
	return nPixelPerDCS;
}
