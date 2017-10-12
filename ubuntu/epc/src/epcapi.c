#include "epcthread.h"
#include <math.h>

extern EpcCalcCfg_st *gEpcCalcCfg;


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
	
	pthread_mutex_destroy(&calccfg->mMutex);

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
	
    if(pthread_mutex_init(&calccfg->mMutex, NULL) < 0) {
    	ERR("calccfg->mMutex init fail!, return\n");
		free(calccfg);
		calccfg = NULL;
		return NULL;
    }

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

void EpcCalcConfig_MutexLock(EpcCalcCfg_st *calccfg)
{
	pthread_mutex_lock(&calccfg->mMutex);
}

void EpcCalcConfig_MutexUnLock(EpcCalcCfg_st *calccfg)
{
	pthread_mutex_unlock(&calccfg->mMutex);
}


EpcCalcCfg_st *EpcCalcConfig_Get(void)
{
	return (EpcCalcCfg_st *)gEpcCalcCfg;
}

int EpcCalc_pruGetNCols() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nCols;
}

int EpcCalc_pruGetNRowsPerHalf() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nRowsPerHalf;
}

int EpcCalc_pruGetNumberOfHalves() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->nHalves;
}



int EpcCalc_calibrationGetOffsetPhase() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->offsetPhase;
}

int32_t* EpcCalc_calibrationGetCorrectionMap() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->subCorrectionMap;
}

int EpcCalc_pruGetMinAmplitude() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->minAmplitude;
}

int EpcCalc_GetpixelMask() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->pixelMask;
}

int EpcCalc_saturationCheck(uint16_t saturationData){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (!calccfg->saturation){
		return 0;
	}
	return saturationData & calccfg->saturationBit;
}

int EpcCalc_adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (!calccfg->adcOverflow){
		return 0;
	}
	return (dcs0 < calccfg->adcMin) || (dcs0 > calccfg->adcMax) || (dcs1 < calccfg->adcMin) || (dcs1 > calccfg->adcMax) || (dcs2 < calccfg->adcMin)	|| (dcs2 > calccfg->adcMax) || (dcs3 < calccfg->adcMin) || (dcs3 > calccfg->adcMax);
}


int EpcCalc_hysteresisUpdate(const int position, const int value) {
	int hysteresis;
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	EpcCalcConfig_MutexLock(calccfg);
	if (value >= calccfg->upperThreshold) {
		calccfg->hysteresisMap[position] = 1;
	} else if (value < calccfg->lowerThreshold) {
		calccfg->hysteresisMap[position] = 0;
	}
	hysteresis = calccfg->hysteresisMap[position];
	EpcCalcConfig_MutexUnLock(calccfg);
	return hysteresis;
}

int EpcCalc_CalibrationLoadOffset(const int storeID, EpcPruInfo_st *pruinfo){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	char filename[64];
	FILE *fp ;
	unsigned int size;
	
	sprintf(filename, "./calibration/PT%02i_W%03i_C%03i_offset_user_%i.bin"
		, pruinfo->partType, pruinfo->waferID, pruinfo->chipID, storeID);
	fp = fopen(filename, "rb");
	if (fp == NULL) { //if user offset is not existing, load factory offset
	
		sprintf(filename, "./calibration/PT%02i_W%03i_C%03i_offset_factory_%i.bin"
			, pruinfo->partType, pruinfo->waferID, pruinfo->chipID, storeID);
		fp = fopen(filename, "rb");
		if (fp == NULL) {
			
			memset(calccfg->correctionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
			memset(calccfg->subCorrectionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
			calccfg->availableCalibration = 1;
			return -1;
		}
	}

	printf("open %s\n", filename);
	size = fread(&calccfg->offsetPhaseDefault, sizeof(int), 1, fp);
	if (!fread(&calccfg->calibrationTemperature, sizeof(double), 1, fp)) {
		calccfg->calibrationTemperature = 45.0;
	}
	if (calccfg->offsetPhaseDefaultEnabled) {
		calccfg->offsetPhase = calccfg->offsetPhaseDefault;
	}
	fclose(fp);
	return size;
}

void EpcCalc_CalibrationSetCorrectionMap(EpcPruInfo_st *pruinfo) {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (calccfg->calibrationEnable) {	
		unsigned int i;
		unsigned int rowReduction = calccfg->rowReduction;
		unsigned int colReduction = calccfg->colReduction;
		unsigned int numberOfHalves = calccfg->numberOfHalves;

		const unsigned int topLeftX = 4;
		const unsigned int bottomRightX = 323;
		const unsigned int topLeftY = 6;
		const unsigned int bottomRightY = 125;
		unsigned int middlePart;
		unsigned int topBottomPart;

		middlePart = ((calccfg->nRowsMax/2-1 - bottomRightY) / rowReduction) * numberOfHalves * (calccfg->nColsMax / colReduction);
		topBottomPart = (calccfg->nColsMax / colReduction) * (calccfg->nRowsMax / rowReduction) - (topLeftY / rowReduction) * numberOfHalves * (calccfg->nColsMax / colReduction);

		int sub = 0; // index for subCorrectionMap
		for (i = 0; i < (calccfg->nColsMax / colReduction) * (calccfg->nRowsMax / rowReduction); i++) {
			if (i < middlePart || i >= topBottomPart) { // check whether i is on a valid row
				//do nothing
			} else if ((i % (calccfg->nColsMax / colReduction) < topLeftX) || (i % (calccfg->nColsMax / colReduction) > bottomRightX)) {  // check whether i is on a valid column
				//do nothing
			} else {
				calccfg->subCorrectionMap[sub] = calccfg->correctionMap[i];
				sub++;
			}
		}
	}
}

int EpcCalc_CalibrationLoadFPN(const int storeID, EpcPruInfo_st *pruinfo) {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	char filename[64];
	FILE *fp ;
	unsigned int size;
	
	if (calccfg->calibrationEnable == 0) { //if FPN disabled
		memset(calccfg->correctionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
		memset(calccfg->subCorrectionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
		calccfg->availableCalibration = 0;
		return 0;
	}

	//open calibration user
	calccfg->availableCalibration = 3;
	sprintf(filename, "./calibration/PT%02i_W%03i_C%03i_calibration_user_%i.bin"
		, pruinfo->partType, pruinfo->waferID, pruinfo->chipID, storeID);
	fp = fopen(filename, "rb");
	if (fp == NULL) { //if user calibration is not existing, load factory calibration

		//open calibration factory
		calccfg->availableCalibration = 2;
		sprintf(filename, "./calibration/PT%02i_W%03i_C%03i_calibration_factory_%i.bin"
			, pruinfo->partType, pruinfo->waferID, pruinfo->chipID, storeID);
		fp = fopen(filename, "rb");
		if (fp == NULL ) {	// if error, try again
		
			fp = fopen(filename, "rb");
			if (fp == NULL ){
				
				memset(calccfg->correctionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
				memset(calccfg->subCorrectionMap, 0, calccfg->nColsMax * calccfg->nRowsMax * sizeof(int32_t));
				calccfg->availableCalibration = 1;
				return -1;
			}
		}
	}

	printf("open %s\n", filename);
	size = fread(calccfg->correctionMap, sizeof(int32_t), calccfg->nColsMax * calccfg->nRowsMax, fp);
	fclose(fp);
	EpcCalc_CalibrationSetCorrectionMap(pruinfo);
	return size;
}


int EpcCalc_CalibrationGetOffsetPhase() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->offsetPhase;
}

int32_t* EpcCalc_CalibrationGetCorrectionMap() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->subCorrectionMap;
}


int EpcPru_Init_waferID_160_chipID_49(EpcPruInfo_st *pruinfo)
{
	pruinfo->icVersion = 3;
	pruinfo->partType = 2;
	pruinfo->partVersion = 6;
	pruinfo->waferID = 160;
	pruinfo->chipID = 49;

	pruinfo->gX = 15;
	pruinfo->gY = 56;
	pruinfo->gM = 52;
	pruinfo->gC = 106;

	return 0;
}


int EpcPru_Init_waferID_172_chipID_110(EpcPruInfo_st *pruinfo)
{
	pruinfo->icVersion = 7;
	pruinfo->partType = 2;
	pruinfo->partVersion = 7;
	pruinfo->waferID = 172;
	pruinfo->chipID = 110;

	pruinfo->gX = 16;
	pruinfo->gY = 59;
	pruinfo->gM = 52;
	pruinfo->gC = 114;

	return 0;
}

int EpcPru_Init(EpcPruInfo_st *pruinfo)
{
	EpcPru_Init_waferID_172_chipID_110(pruinfo);

	EpcCalc_CalibrationLoadOffset(28, pruinfo);
	EpcCalc_CalibrationLoadFPN(28, pruinfo);

	return 0;
}


int EpcCalc_test_saveFile(char *buf, unsigned int size, unsigned int sequence)
{
	return 0;
	char filename[64];
	FILE *fp = NULL;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	filename[0] = '\0';
	sprintf(filename, "%s/out/Dist_307200_%ld_%06ld_%04d.bin", DATA_PATH, tv.tv_sec, tv.tv_usec,sequence);
	//printf("save: %s\n", filename);
	fp = fopen(filename, "wb");
	if(fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
	return 0;
}


