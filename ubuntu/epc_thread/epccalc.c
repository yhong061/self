#include "epcthread.h"
#include <math.h>

int EpcCalcInfo_Init(EpcCalcInfo_st *calcinfo)
{
	unsigned int dcs = 4;
	unsigned int imagesize = WIDTH * HEIGHT * sizeof(unsigned short) * dcs;
	
	calcinfo->mDatabufOffset = DATA_INFO_SIZE;
	calcinfo->mDatabufSize = imagesize + DATA_INFO_SIZE;
	calcinfo->pDatabuf = malloc(calcinfo->mDatabufSize);
	if(calcinfo->pDatabuf == NULL) {
		ERR("mallco calcinfo->pDatabuf fail, return\n");
		return -1;
	}
	FILE *fp = NULL;
	char filename[128];
	filename[0] = '\0';
	sprintf(filename, "%s/in.bin", DATA_PATH);
	fp = fopen(filename, "rb");
	if(fp == NULL) {
		ERR("open %s fail\n", filename);
		free(calcinfo->pDatabuf);
		calcinfo->pDatabuf = NULL;
		return -1;
	}

	char *buf = calcinfo->pDatabuf + calcinfo->mDatabufOffset;
	unsigned int readsize = fread(buf, 1, imagesize, fp);
	if(readsize != imagesize)
		DBG("readsize[%d] != imagesize[%d]\n", readsize, imagesize);
	fclose(fp);

	EpcDataInfo_st *datainfo = (EpcDataInfo_st *)calcinfo->pDatabuf;
	datainfo->mBufIdx = 0;
	datainfo->mWidth = WIDTH;
	datainfo->mHeight = HEIGHT;
	datainfo->mDcs = dcs;
	datainfo->mSequence = 0;
	datainfo->mDatasize = readsize;

	imagesize = WIDTH * HEIGHT * sizeof(unsigned short) * 2;
	calcinfo->mCalcbufOffset = DATA_INFO_SIZE;
	calcinfo->mCalcbufSize = imagesize + DATA_INFO_SIZE;
	calcinfo->pCalcbuf = malloc(calcinfo->mCalcbufSize);
	if(calcinfo->pCalcbuf == NULL) {
		free(calcinfo->pDatabuf);
		calcinfo->pDatabuf = NULL;
		ERR("mallco calcinfo->pCalcbuf fail, return\n");
		return -1;
	}

	return 0;
}

int EpcCalc_CalibrationLoadOffset(const int storeID, EpcPruInfo_st *pruinfo){
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	char filename[64];
	FILE *fp ;
	
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
	fread(&calccfg->offsetPhaseDefault, sizeof(int), 1, fp);
	if (!fread(&calccfg->calibrationTemperature, sizeof(double), 1, fp)) {
		calccfg->calibrationTemperature = 45.0;
	}
	if (calccfg->offsetPhaseDefaultEnabled) {
		calccfg->offsetPhase = calccfg->offsetPhaseDefault;
	}
	fclose(fp);
	return 1;
}

void EpcCalc_CalibrationSetCorrectionMap(EpcPruInfo_st *pruinfo) {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	if (calccfg->calibrationEnable) {	
		int i;
		int rowReduction = calccfg->rowReduction;
		int colReduction = calccfg->colReduction;
		int numberOfHalves = calccfg->numberOfHalves;

		const int topLeftX = 4;
		const int bottomRightX = 323;
		const int topLeftY = 6;
		const int bottomRightY = 125;
		int middlePart;
		int topBottomPart;

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
	fread(calccfg->correctionMap, sizeof(int32_t), calccfg->nColsMax * calccfg->nRowsMax, fp);
	fclose(fp);
	EpcCalc_CalibrationSetCorrectionMap(pruinfo);
	return 1;
}


int EpcCalc_CalibrationGetOffsetPhase() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->offsetPhase;
}

int32_t* EpcCalc_CalibrationGetCorrectionMap() {
	EpcCalcCfg_st *calccfg = EpcCalcConfig_Get();
	return calccfg->subCorrectionMap;
}

int EpcCalc_SaturationCheck(uint16_t saturationData){
	uint16_t saturationBit = 0x1000;
	int saturation = 1;
	if (!saturation){
		return 0;
	}
	return saturationData & saturationBit;
}

int EpcCalc_AdcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	int adcOverflow = 1;
	uint16_t adcMin = 1;  
	uint16_t adcMax = 4094;
	
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax) || (dcs1 < adcMin) || (dcs1 > adcMax) || (dcs2 < adcMin)	|| (dcs2 > adcMax) || (dcs3 < adcMin) || (dcs3 > adcMax);
}

int EpcCalc_Calc4Dcs(EpcCalcInfo_st *calcinfo)
{
	unsigned int i;
		
	EpcDataInfo_st *datainfo = (EpcDataInfo_st *)calcinfo->pDatabuf;
	char *databuf = (char *)calcinfo->pDatabuf + calcinfo->mDatabufOffset;
	char *calcbuf = (char *)calcinfo->pCalcbuf + calcinfo->mCalcbufOffset;
	unsigned int dcssize = datainfo->mDatasize/datainfo->mDcs;
	dcssize /= sizeof(uint16_t);
	
	uint16_t pixelMask = 0xFFF; 
	char amplitudeDivider = 2;
	int minAmplitude = 80;//100;
	int offset = EpcCalc_CalibrationGetOffsetPhase();
	int32_t *calibrationMap = EpcCalc_CalibrationGetCorrectionMap();

	int32_t arg1, arg2;
	uint16_t saturationData;
	uint16_t data0,data1,data2,data3;
	uint16_t dcs0,dcs1,dcs2,dcs3;
	uint16_t *src1,*src2,*src3,*src4;
	uint16_t *dst1,*dst2;
	uint16_t h = 240;
	uint16_t w = 320;
	uint32_t x = 0, y = 0;
	uint32_t x1 = 0, y1 = 0, pos = 0;
	uint16_t nHalves = 2;
	int16_t dist;
	int16_t amp;

	src1 = (uint16_t *)databuf;
	src2 = (uint16_t *)databuf + dcssize;
	src3 = (uint16_t *)databuf + dcssize * 2;
	src4 = (uint16_t *)databuf + dcssize * 3;

	dst1 = (uint16_t *)calcbuf;
	dst2 = (uint16_t *)calcbuf + dcssize;

	int tmp_offset = 0x19576;//0x10;

	for(i=0; i<dcssize; ++i) {	
		data0  = *src1++;
		data1  = *src2++;
		data2  = *src3++;
		data3  = *src4++;
		dcs0  = data0 & pixelMask;
		dcs1  = data1 & pixelMask;
		dcs2  = data2 & pixelMask;
		dcs3  = data3 & pixelMask;
		saturationData = data0 | data1 | data2 | data3;
				
		if(EpcCalc_SaturationCheck(saturationData)) {
			amp = SATURATION;
			dist = SATURATION;
		}
		else if(EpcCalc_AdcOverflowCheck4DCS(dcs0, dcs1, dcs2, dcs3)) {
			amp = ADC_OVERFLOW;
			dist = ADC_OVERFLOW;
		}
		else {
			arg1 = dcs3 - dcs1;
			arg2 = dcs2 - dcs0;
			
			amp = sqrt(arg1 * arg1 + arg2 * arg2) / amplitudeDivider;
			//if(amp >= minAmplitude){
			if(hysteresisUpdate(i, amp)) {
				int32_t distance = fp_atan2(arg1, arg2) + FP_M_PI;
				distance = ((distance * MAX_DIST_VALUE) / (double)FP_M_2_PI) + offset + calibrationMap[i] + 0.5;			
				distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
				dist = (int16_t) distance;
			}
			else {
				dist = LOW_AMPLITUDE;
				amp  = LOW_AMPLITUDE;
			}
		}

		if (nHalves == 2) {
			if (x < w){
				x1 = x;
				y1 = h / 2 - 1 - y;
			}else{
				x1 = x - w;
				y1 = h / 2 + y;
			}
		} else {
			x1 = x;
			y1 = h - 1 - y;
		}
		
		pos = y1 * w + x1;

		++x;
		if (x == nHalves * w){
			x = 0;
			++y;
		}

		if(pos == tmp_offset/2) {
		//if(dist == 0x7918) {
			printf("dsc[%x %x %x %x] [%x %x %x %x]  dist = %x, saturationData = %x, %d\n", dcs0, dcs1, dcs2, dcs3, 
				*src1, *src2, *src3, *src4, 
				dist, saturationData, EpcCalc_SaturationCheck(saturationData));
		}
			
		dst1[pos] = dist;
		dst2[pos] = amp;
	}
}

int pruGetImage(uint16_t **data, uint16_t **pixelData, EpcCalcInfo_st *calcinfo) {
	EpcDataInfo_st *datainfo = (EpcDataInfo_st *)calcinfo->pDatabuf;
	char *databuf = (char *)calcinfo->pDatabuf + calcinfo->mDatabufOffset;
	char *calcbuf = (char *)calcinfo->pCalcbuf + calcinfo->mCalcbufOffset;

	
	int size = (int)datainfo->mDatasize;
	*data = (uint16_t *)databuf;
	*pixelData = (uint16_t *)calcbuf;
	test_saveFile("Image", (char *)*data, size*sizeof(uint16_t));
	return size;		// size of all pixels
}

typedef struct _EpcCalcTmp_ {
int32_t *calibrationMap;
int offset;
int minAmplitude;
int8_t amplitudeDivider;
uint32_t index;
uint32_t nPxDcs;
uint16_t h;
uint16_t w;
uint32_t x;
uint32_t y;
uint16_t nHalves;
uint16_t pixelDCS0;
uint16_t pixelDCS1;
uint16_t pixelDCS2;
uint16_t pixelDCS3;
uint16_t pixelMask;
uint16_t saturationData;
unsigned int indexMemory;
unsigned int indexCal;
int32_t arg1;
int32_t arg2;

}EpcCalcTmp_st;

struct Position{
	uint16_t x;
	uint8_t y;
	uint32_t indexMemory;
	uint32_t indexSorted;
};

struct TofResult{
	int16_t dist;
	int16_t amp;
};


void calculatorInit(uint8_t const divideAmplitudeBy, EpcCalcCfg_st *calccfg, EpcCalcTmp_st *calctmp){
	calctmp->calibrationMap = calibrationGetCorrectionMap();
	calctmp->offset = calibrationGetOffsetPhase();
	calctmp->minAmplitude = pruGetMinAmplitude();
	calctmp->amplitudeDivider = divideAmplitudeBy;
	//printf("offset = %d, minAmplitude = %d, amplitudeDivider = %d\n"
	//	, calctmp->offset, calctmp->minAmplitude, calctmp->amplitudeDivider);
}

void iteratorDefaultInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height, EpcCalcTmp_st *calctmp){
	calctmp->nPxDcs = numberOfPixelsPerDCS;
	calctmp->w = width;
	calctmp->h = height;
	calctmp->index = 0;
	calctmp->x = 0;
	calctmp->y = 0;
	calctmp->nHalves = pruGetNumberOfHalves();
//printf("xywh=%d %d %d %d -- nPxDcs = %d, nHalves = %d\n", calctmp->x, calctmp->y, calctmp->w, calctmp->h, calctmp->nPxDcs, calctmp->nHalves);	

}

struct Position iteratorDefaultNext(EpcCalcTmp_st *calctmp){
	struct Position pos;
	if (calctmp->nHalves == 2) {
		if (calctmp->x < calctmp->w){
			pos.x = calctmp->x;
			pos.y = calctmp->h / 2 - 1 - calctmp->y;
		}else{
			pos.x = calctmp->x - calctmp->w;
			pos.y = calctmp->h / 2 + calctmp->y;
		}
	} else {
		pos.x = calctmp->x;
		pos.y = calctmp->h - 1 - calctmp->y;
	}
	

	pos.indexMemory = calctmp->index;
	pos.indexSorted = pos.y * calctmp->w + pos.x;

	++calctmp->x;
	if (calctmp->x == calctmp->nHalves * calctmp->w){
		calctmp->x = 0;
		++calctmp->y;
	}
	++calctmp->index;
	return pos;
}

bool iteratorDefaultHasNext(EpcCalcTmp_st *calctmp){
	return calctmp->index < calctmp->nPxDcs;
}

void saturationSet4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3, EpcCalcTmp_st *calctmp){
	calctmp->saturationData = dcs0 | dcs1 | dcs2 | dcs3;
}

void calculatorSetArgs4DCS(uint16_t const dcs0, uint16_t const dcs1, uint16_t const dcs2, uint16_t const dcs3, unsigned int const index, unsigned int const indexCalibration, EpcCalcTmp_st *calctmp){
	calctmp->pixelMask = Epc_GetpixelMask();

	calctmp->pixelDCS0  = dcs0 & calctmp->pixelMask;
	calctmp->pixelDCS1  = dcs1 & calctmp->pixelMask;
	calctmp->pixelDCS2  = dcs2 & calctmp->pixelMask;
	calctmp->pixelDCS3  = dcs3 & calctmp->pixelMask;
	saturationSet4DCS(dcs0, dcs1, dcs2, dcs3, calctmp);
	calctmp->indexMemory = index;
	calctmp->indexCal = indexCalibration;

	calctmp->arg1 = calctmp->pixelDCS3 - calctmp->pixelDCS1;
	calctmp->arg2 = calctmp->pixelDCS2 - calctmp->pixelDCS0;
}

int16_t calculatorGetAmpSine(EpcCalcTmp_st *calctmp){
	if (saturationCheck(calctmp->saturationData)){
	//if (0){
		return SATURATION;
	}else if(adcOverflowCheck4DCS(calctmp->pixelDCS0, calctmp->pixelDCS1, calctmp->pixelDCS2, calctmp->pixelDCS3)){
	//}else if(0){
		return ADC_OVERFLOW;
	}else{
		return sqrt(calctmp->arg1 * calctmp->arg1 + calctmp->arg2 * calctmp->arg2) / calctmp->amplitudeDivider;
	}
}


struct TofResult calculatorGetDistAndAmpSine(EpcCalcTmp_st *calctmp){
	struct TofResult tofResult;

	tofResult.amp = calculatorGetAmpSine(calctmp);
	if (saturationCheck()){
	//if (0){
		tofResult.dist = SATURATION;
		tofResult.amp  = SATURATION;
	}else if(adcOverflowCheck4DCS(calctmp->pixelDCS0, calctmp->pixelDCS1, calctmp->pixelDCS2, calctmp->pixelDCS3)){
	//}else if(0){
		tofResult.dist = ADC_OVERFLOW;
		tofResult.amp  = ADC_OVERFLOW;
	//}else if(hysteresisUpdate(calctmp->indexMemory, tofResult.amp)){
	}else if(1){
		//int32_t distance = fp_atan2(calctmp->arg1, calctmp->arg2) + FP_M_PI;
		//distance = ((distance * MAX_DIST_VALUE) / (double)FP_M_2_PI) +calctmp->offset + calctmp->calibrationMap[calctmp->indexCal] + 0.5;
		//distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
		tofResult.dist = 0;//(int16_t) distance;
	}else{
		tofResult.dist = LOW_AMPLITUDE;
		tofResult.amp  = LOW_AMPLITUDE;
	}
	return tofResult;
}

int calcDefPiDelayGetDataSorted(enum calculationType type, EpcCalcInfo_st *calcinfo){
	uint16_t *calcdata;
	uint16_t *data;
	EpcCalcTmp_st 	stcalctmp;
	EpcCalcTmp_st 	*calctmp = (EpcCalcTmp_st *)&stcalctmp;
	EpcCalcCfg_st	*calccfg = calcinfo->pCalcCfg;

//printf("[%s:%d] ---run-----\n", __func__, __LINE__);			
	
	int size = pruGetImage(&data, &calcdata, calcinfo);

	uint16_t *pMem = data;
	uint16_t *pixelData = calcdata;
	int nPixelPerDCS = size / 4;
	calculatorInit(2, calccfg, calctmp);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves(), calctmp);
//printf("[%s:%d] ---run-----\n", __func__, __LINE__);			

	while(iteratorDefaultHasNext(calctmp)){
		struct Position p = iteratorDefaultNext(calctmp);
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];
		uint16_t pixelDCS2 = pMem[p.indexMemory + 2 * nPixelPerDCS];
		uint16_t pixelDCS3 = pMem[p.indexMemory + 3 * nPixelPerDCS];

		calculatorSetArgs4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3, p.indexMemory, p.indexMemory,calctmp);
		
		struct TofResult tofResult;
		switch(type){
		case DIST:
			pixelData[p.indexSorted] = calculatorGetDistAndAmpSine(calctmp).dist;
			break;
		case AMP:
			pixelData[p.indexSorted] = calculatorGetAmpSine(calctmp);
			break;
		case INFO:
			tofResult = calculatorGetDistAndAmpSine(calctmp);
			pixelData[p.indexSorted] = tofResult.dist;
			pixelData[p.indexSorted + nPixelPerDCS] = tofResult.amp;
			break;
		}
	}
//printf("[%s:%d] ---run-----\n", __func__, __LINE__);			

	if (type == INFO){
		return nPixelPerDCS * 2;
	}
	
	return nPixelPerDCS;
}

int calculationDistanceAndAmplitudeSorted(EpcCalcInfo_st *calcinfo) {
//	static int idx = 0;
	EpcCalcCfg_st	*calccfg = calcinfo->pCalcCfg;
	int size;
	
	if (calccfg->dualMGX) {
		if (calccfg->piDelay) {
			if (calccfg->pn) {
				//size = calcPNMGXPiDelayGetDataSorted(INFO, data);
			} else {
				//size =  calcMGXPiDelayGetDataSorted(INFO, data);
			}
		} else {
			if (calccfg->pn) {
				//size =  calcPNMGXNoPiDelayGetDataSorted(INFO, data);
			} else {
				//size =  calcMGXNoPiDelayGetDataSorted(INFO, data);
			}
		}
	} else {
		if (calccfg->piDelay) {
			if (calccfg->pn) {
				//size =  calcPNDefPiDelayGetDataSorted(INFO, data);
			} else {
//printf("[%s:%d] ---start---[%d]\n", __func__, __LINE__, idx++);			
				size = calcDefPiDelayGetDataSorted(INFO, calcinfo);
//printf("[%s:%d] ---end-----\n", __func__, __LINE__);			
			}
		} else {
			if (calccfg->pn) {
				//size = calcPNDefNoPiDelayGetDataSorted(INFO, data);
			} else {
				//size = calcDefNoPiDelayGetDataSorted(INFO, data);
			}
		}
	}
	//imageCorrect(data);
	//imageProcess(data);
	//imageAverage(data);
	//imageDifference(data);
	//printf("calculationDistanceAndAmplitudeSorted\n");
	//calculationCorrectDRNU(data);
	return size;
}

int apiGetDistanceAndAmplitudeSorted(EpcCalcInfo_st *calcinfo) {
	return calculationDistanceAndAmplitudeSorted(calcinfo);
}

int test_saveFile(char *buf, unsigned int size, unsigned int sequence)
{
	char filename[64];
	FILE *fp = NULL;

	filename[0] = '\0';
	sprintf(filename, "%s/out/Dist_307200_%04d.bin", DATA_PATH, sequence);
	//printf("save: %s\n", filename);
	fp = fopen(filename, "wb");
	if(fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
	return 0;
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

int test(void)
{
	int ret;
	EpcCalcInfo_st calcinfo;
	EpcCalcInfo_Init((EpcCalcInfo_st *)&calcinfo);
		
	EpcCalc_Calc4Dcs((EpcCalcInfo_st *)&calcinfo);

	char *buf = calcinfo.pCalcbuf + calcinfo.mDatabufOffset;
	unsigned int size = calcinfo.mCalcbufSize - calcinfo.mCalcbufOffset;
	test_saveFile(buf, size, 0);

	return 0;
}
