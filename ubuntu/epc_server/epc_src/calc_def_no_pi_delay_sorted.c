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
#include <sys/time.h>

static uint16_t pixelData[328 * 252 * 2];

int calcDefNoPiDelayGetDataSorted(enum calculationType type, uint16_t **data) {
	struct timeval tv1,tv2,tv3;
	gettimeofday(&tv1, NULL);
	int size = pruGetImage(data);
	gettimeofday(&tv2, NULL);
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
	gettimeofday(&tv3, NULL);
	unsigned long laspe32 = (tv3.tv_sec*1000000 + tv3.tv_usec) - (tv2.tv_sec*1000000 + tv2.tv_usec);
	unsigned long laspe21 = (tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec);

	printf("%lu.%06lu - %lu.%06lu - %10lu.%06lu, 3-2=%lu, 2-1=%lu\n"
		, (unsigned long)tv3.tv_sec, (unsigned long)tv3.tv_usec
		, (unsigned long)tv2.tv_sec, (unsigned long)tv2.tv_usec
		, (unsigned long)tv1.tv_sec, (unsigned long)tv1.tv_usec
		, laspe32, laspe21);
		if (type == INFO){
			return nPixelPerDCS * 2;
		}
		return nPixelPerDCS;
}
