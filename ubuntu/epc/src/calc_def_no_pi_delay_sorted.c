#include "calc_def_no_pi_delay_sorted.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_default.h"
#include <math.h>

static uint16_t pixelData[328 * 252 * 2];

int calcDefNoPiDelayGetDataSorted(enum calculationType type, uint16_t **data) {
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
//		test_saveFile("Dist2", (char *)*data,  nPixelPerDCS * 2 * sizeof(uint16_t));
		if (type == INFO){
			return nPixelPerDCS * 2;
		}
		return nPixelPerDCS;
}
