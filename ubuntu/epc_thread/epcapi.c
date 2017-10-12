#include "epcthread.h"
#include <math.h>

extern EpcCalcCfg_st *gEpcCalcCfg;

/*static*/ inline int32_t __fp_mul(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((int32_t)((int32_t)a * b)>>fp_fract);

    return r;
}

/*static*/ inline int32_t __fp_div(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((((int32_t)a<<fp_fract))/b);

    return r;
}


/*static*/ inline int32_t fp_atan2(int16_t y, int16_t x)
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
        r = __fp_div((fp_x - fp_abs_y), (fp_x + fp_abs_y));

        /* angle = coeff_1 * r^3 - coeff_2 * r + coeff_3 */
        angle =
            __fp_mul(coeff_1, __fp_mul(r, __fp_mul(r, r))) -
            __fp_mul(coeff_2, r) + coeff_3;
    } else {
        /* r = (x + abs_y) / (abs_y - x); */
        r = __fp_div((fp_x + fp_abs_y), (fp_abs_y - fp_x));
        /*        angle = coeff_1 * r*r*r - coeff_2 * r + 3*coeff_3; */
        angle =
            __fp_mul(coeff_1, __fp_mul(r, __fp_mul(r, r))) -
            __fp_mul(coeff_2, r) + 3 * coeff_3;
    }

    if (y < 0)
        return(-angle);     // negate if in quad 3 or 4
    else
        return(angle);
}

int EpcCalcConfig_UnInit(EpcCalcCfg_st* calccfg)
{
	if(calccfg == NULL )
		return 0;
	
	if(calccfg->hysteresisMap) {
		free(calccfg->hysteresisMap);
		calccfg->hysteresisMap = NULL;
	}

	if(calccfg->correctionMap) {
		free(calccfg->correctionMap);
		calccfg->correctionMap = NULL;
	}
	if(calccfg->subCorrectionMap) {
		free(calccfg->subCorrectionMap);
		calccfg->subCorrectionMap = NULL;
	}

	free(calccfg);
	calccfg = NULL;
	return 0;
}

EpcCalcCfg_st* EpcCalcConfig_Init(void)
{
	EpcCalcCfg_st* calccfg = (EpcCalcCfg_st*)malloc(sizeof(EpcCalcCfg_st));
	if(calccfg == NULL) {
		ERR("mallco calccfg fail, return\n");
		return NULL;
	}

	memset(calccfg, 0, sizeof(EpcCalcCfg_st));
	
	calccfg->nColsMax = WIDTH_MAX;
	calccfg->nCols    = WIDTH;
	calccfg->nRowsMax = HEIGHT_MAX;
	calccfg->nRows    = HEIGHT;
	calccfg->nHalves    = 2;
	calccfg->nRowsPerHalf    = calccfg->nRows/calccfg->nHalves;

	calccfg->dualMGX = 0;
	calccfg->piDelay = 1;
	calccfg->pn = 0;
	calccfg->minAmplitude = 100;

	calccfg->pixelMask = 0xFFF;

	calccfg->saturation = 1;
	calccfg->saturationBit = 0x1000;

	calccfg->adcOverflow =1;
	calccfg->adcMin = 1;
	calccfg->adcMax = 4094;
	
	calccfg->hysteresis = 20;
	calccfg->upperThreshold = 100;
	calccfg->lowerThreshold = calccfg->upperThreshold - calccfg->hysteresis;
	calccfg->hysteresisMap =(unsigned char *)malloc(calccfg->nColsMax * calccfg->nRowsMax);
	if(calccfg->hysteresisMap == NULL) {
		ERR("malloc hysteresisMap fail, return\n");
		EpcCalcConfig_UnInit(calccfg);
		calccfg = NULL;
		return NULL;
	}
	memset(calccfg->hysteresisMap, 0, sizeof(calccfg->nColsMax * calccfg->nRowsMax));
	
	calccfg->rowReduction = 1;
	calccfg->colReduction = 1;
	calccfg->numberOfHalves = 2;

	calccfg->calibrationEnable = 1;
	calccfg->availableCalibration    = 0;
	calccfg->offsetPhaseDefault    = 0;
	calccfg->calibrationTemperature    = 0.0;
	calccfg->offsetPhaseDefaultEnabled    = 1;
	calccfg->offsetPhase    = 0;

	calccfg->correctionMap =(int32_t *)malloc(calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
	if(calccfg->correctionMap == NULL) {
		ERR("malloc correctionMap fail, return\n");
		EpcCalcConfig_UnInit(calccfg);
		calccfg = NULL;
		return NULL;
	}

	calccfg->subCorrectionMap =(int32_t *)malloc(calccfg->nCols * calccfg->nRows * sizeof(int32_t));
	if(calccfg->subCorrectionMap == NULL) {
		ERR("malloc subCorrectionMap fail, return\n");
		EpcCalcConfig_UnInit(calccfg);
		calccfg = NULL;
		return NULL;
	}
	
	return calccfg;
}

EpcCalcCfg_st *EpcCalcConfig_Get(void)
{
	return (EpcCalcCfg_st *)gEpcCalcCfg;
}

int pruGetNCols() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nCols;
}

int pruGetNRowsPerHalf() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nRowsPerHalf;
}

int pruGetNumberOfHalves() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nHalves;
}



int calibrationGetOffsetPhase() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->offsetPhase;
}

int32_t* calibrationGetCorrectionMap() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->subCorrectionMap;
}

int pruGetMinAmplitude() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->minAmplitude;
}

int Epc_GetpixelMask() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->pixelMask;
}

int saturationCheck(uint16_t saturationData){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (!calccfg->saturation){
		return 0;
	}
	return saturationData & calccfg->saturationBit;
}

int adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (!calccfg->adcOverflow){
		return 0;
	}
	return (dcs0 < calccfg->adcMin) || (dcs0 > calccfg->adcMax) || (dcs1 < calccfg->adcMin) || (dcs1 > calccfg->adcMax) || (dcs2 < calccfg->adcMin)	|| (dcs2 > calccfg->adcMax) || (dcs3 < calccfg->adcMin) || (dcs3 > calccfg->adcMax);
}


int hysteresisUpdate(const int position, const int value) {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (value >= calccfg->upperThreshold) {
		calccfg->hysteresisMap[position] = 1;
	} else if (value < calccfg->lowerThreshold) {
		calccfg->hysteresisMap[position] = 0;
	}
	return calccfg->hysteresisMap[position];
}


