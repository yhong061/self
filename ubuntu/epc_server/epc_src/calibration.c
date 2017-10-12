#define _GNU_SOURCE
#include "calibration.h"
#include "calculation.h"
#include "config.h"
#include "i2c.h"
#include "api.h"
#include "calc_bw_sorted.h"
#include "pru.h"
#include "read_out.h"
#include "temperature.h"
#include "helper.h"
#include "dll.h"
#include "registers.h"
#include "seq_prog_loader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#define SPEED_OF_LIGHT_DIV_2 150000000.0
#define MAX_PHASE 3000.0
#define NUMBER_OF_MEASUREMENTS 100
#define REF_ROI_HALF_WIDTH 4
#define PI 3.14159265

#define	FREQUENCY_625	0
#define FREQUENCY_1250  1
#define FREQUENCY_2500	2
#define FREQUENCY_5000	3
#define	FREQUENCY_10000	4
#define FREQUENCY_20000	5
#define	ABS	8
#define PI_DELAY	16
#define BINNING_VERTICAL	32
#define BINNING_HORIZONTAL	64
#define ROW_REDUCTION_1	0
#define ROW_REDUCTION_2	128
#define ROW_REDUCTION_4	256
#define ROW_REDUCTION_8	384
#define DUAL_MGX_MOTION_BLUR	512
#define PN 1024;
#define DUAL_MGX_HDR	2048

static int32_t avgMap[328 * 252];
static int32_t correctionMap[328 * 252];
static int32_t subCorrectionMap[328 * 252];
static uint16_t nColsMax = 328;
static uint16_t nCols    = 320;
static uint16_t nRowsMax = 252;
static uint16_t nRows    = 240;
static uint16_t xCenter  = 164;

static int calibrationEnable = 1;
static int offsetPhase = 0;
static int offsetPhaseDefault = 0;
static int offsetPhaseDefaultEnabled = 1;
static int availableCalibration = 0;
static double calibrationTemperature = 0;

static int gWidth     = 160;
static int gHeight    = 60;
static int gNumDist   = 50;
static int offsetDRNU = 0;
static int temperatureDRNU[50];

unsigned char intData[4];

/*!
 Used to store the factory calibrations at epc.
 @param distance distance to a white wall in cm
 @param FOV horizontal field of view in beg
 @return 0 on success, -1 on error.
 */
int calibrationMarathon(const int distance, const int FOV) {
	const int deviceAddress = configGetDeviceAddress();
	apiEnableVerticalBinning(0, deviceAddress);
	apiEnableHorizontalBinning(0, deviceAddress);
	apiSetRowReduction(0, deviceAddress);
	apiSetModulationFrequency(1, deviceAddress);
	apiEnableABS(1, deviceAddress);
	apiEnablePiDelay(1, deviceAddress);
	apiEnableDualMGX(0, deviceAddress);
	printf("Calibrating 10 MHz PiDelay...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	int time3D    = configGetIntegrationTime3D(deviceAddress);
	int time3DHDR = configGetIntegrationTime3DHDR(deviceAddress);
	apiEnableDualMGX(1, deviceAddress);				//dualMGX 1
	printf("Calibrating 10 MHz PiDelay dualMGX...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnableDualMGX(0, deviceAddress);				//dualMGX 0
	apiEnableVerticalBinning(1, deviceAddress);		//binningV 1
	apiSetIntegrationTime3DHDR(time3DHDR/2, deviceAddress);
	apiSetIntegrationTime3D(time3D/2, deviceAddress);
	printf("Calibrating 10 MHz PiDelay vertical binning...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnableHorizontalBinning(1, deviceAddress);	//binningH 1
	apiSetIntegrationTime3DHDR(time3DHDR/4, deviceAddress);
	apiSetIntegrationTime3D(time3D/4, deviceAddress);
	printf("Calibrating 10 MHz PiDelay vertical and horizontal binning...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnableVerticalBinning(0, deviceAddress);		//binningV 0
	apiSetIntegrationTime3DHDR(time3DHDR/2, deviceAddress);
	apiSetIntegrationTime3D(time3D/2, deviceAddress);
	apiSetRowReduction(0, deviceAddress);
	printf("Calibrating 10 MHz PiDelay horizontal binning...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnableHorizontalBinning(0, deviceAddress);	//binningH 0
	apiSetIntegrationTime3DHDR(time3DHDR, deviceAddress);
	apiSetIntegrationTime3D(time3D, deviceAddress);
	apiEnablePiDelay(0, deviceAddress);
	apiEnableDualMGX(1, deviceAddress);				//dualMGX 1
	printf("Calibrating 10 MHz dualMGX...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnableDualMGX(0, deviceAddress);				//dualMGX 0
	printf("Calibrating 10 MHz...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiSetModulationFrequency(0, deviceAddress);
	printf("Calibrating 20 MHz...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnablePiDelay(1, deviceAddress);
	printf("Calibrating 20 MHz PiDelay...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiSetModulationFrequency(2, deviceAddress);
	apiEnablePiDelay(0, deviceAddress);
	printf("Calibrating 5 MHz...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnablePiDelay(1, deviceAddress);
	printf("Calibrating 5 MHz PiDelay...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiSetModulationFrequency(1, deviceAddress);
	apiSelectMode(1, deviceAddress);
	printf("Calibrating 10 MHz PiDelay PN...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnablePiDelay(0, deviceAddress);
	printf("Calibrating 10 MHz PN...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiSelectMode(0, deviceAddress);
	apiEnableHDR(1, deviceAddress);
	int timeHDR = configGetIntegrationTime3DHDR(); //save temporary original HDR time
	configSetIntegrationTime3DHDR(configGetIntegrationTime3D(), deviceAddress);
	printf("Calibrating 10 MHz HDR...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	apiEnablePiDelay(1, deviceAddress);
	printf("Calibrating 10 MHz HDR PiDelay...\n");
	if (calibrationPixel(distance, FOV, deviceAddress, 1) < 0) return -1;
	configSetIntegrationTime3DHDR(timeHDR, deviceAddress); // restore original HDR time
	printf("Switching back to 10 MHz PiDelay...\n");
	apiEnableHDR(0, deviceAddress);
	printf("Calibration finished.\n");
	return 0;
}

int16_t calibrationSearchBadPixelsWithMin(){
	uint16_t *pMem;
	uint8_t mt_0_hi_bkp = 0;
	uint8_t mt_0_lo_bkp = 0;
	struct stat st;
	unsigned char *i2cData    = malloc(8 * sizeof(unsigned char));
	unsigned char *i2cDataBkp = malloc(4 * sizeof(unsigned char));
	int deviceAddress = configGetDeviceAddress();

	apiEnableVerticalBinning(0, deviceAddress);
	apiEnableHorizontalBinning(0, deviceAddress);
	apiSetRowReduction(0, deviceAddress);
	apiSetModulationFrequency(1, deviceAddress);
	apiEnableABS(1, deviceAddress);
	apiEnablePiDelay(1, deviceAddress);
	apiEnableDualMGX(0, deviceAddress);

	op_mode_t currentMode = configGetCurrentMode();
	if (currentMode == THREED) {
		//device was in TOF mode, will change mode to GRAYSCALE for measurement
		apiLoadConfig(0, deviceAddress);
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		// Switch to differential read-out (ABS disabled):
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		mt_0_hi_bkp = i2cData[0];
		i2cData[0] = mt_0_hi_bkp & 0xCF;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
	}

	i2c(deviceAddress, 'r', ROI_TL_X_HI, 6, &i2cData);
	const int topLeftX = (i2cData[0] << 8) + i2cData[1];
	const int bottomRightX = (i2cData[2] << 8) + i2cData[3];
	const int topLeftY = i2cData[4];
	const int bottomRightY = i2cData[5];
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		apiSetROI(4, 163, 6, 65, deviceAddress);
	} else {
		apiSetROI(4, 323, 6, 125, deviceAddress);
	}

	// Set integration time to 0 (min. allowed value):
	i2c(deviceAddress, 'r', INTM_HI, 4, &i2cDataBkp);
	i2cData[0] = 0x00;
	i2cData[1] = 0x01;
	i2cData[2] = 0x00;
	i2cData[3] = 0x04;
	i2c(deviceAddress, 'w', INTM_HI, 4, &i2cData);

	// Force modulation gates off, i.e. charge transfer will be blocked.
	i2c(deviceAddress, 'r', MT_0_LO, 1, &i2cData);
	mt_0_lo_bkp = i2cData[0];
	i2cData[0] = mt_0_lo_bkp | 0x03;
	i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData);

	calcBWSorted(&pMem); // frame acquisition

	char path[64];
	sprintf(path, "./calibration/PT%02i_W%03i_C%03i_bad_pixels.txt", configGetPartType(), configGetWaferID(), configGetChipID());
	FILE *fileBadPixels = fopen(path, "wb");
	if (fileBadPixels == NULL){
		return -1;
	}
	
	int16_t count = 0;
	int x, y;
	for (y = 0; y < nRows; y++){
		for (x = 0; x < nCols; x++){
			if (pMem[y*nCols+x] > 2096 || pMem[y*nCols+x] < 2000){
				fprintf(fileBadPixels,"%i %i\n",x+4,y+6);
				++count;
			}
		}
	}
	fclose(fileBadPixels);

	// Restore modulation gate forcing:
	i2cData[0] = mt_0_lo_bkp;
	i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData);

	// Restore integration time settings:
	i2c(deviceAddress, 'w', INTM_HI, 4, &i2cDataBkp);
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		// Restore readout settings:
		i2cData[0] = mt_0_hi_bkp;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
	}

	if (currentMode == THREED) {
		//device was in THREED mode before measuring, will change mode to THREED
		apiLoadConfig(1, deviceAddress);
	}
	apiSetROI(topLeftX, bottomRightX, topLeftY, bottomRightY, deviceAddress);
	free(i2cData);
	free(i2cDataBkp);

	if (stat(path, &st) == 0) {
		if((int)st.st_size == 0 && count != 0) {
			remove(path);
			return -1;
		}
	} else {
		remove(path);
		return -1;
	}

	return count;
}

uint16_t calibrationGetBadPixels(int16_t* badPixels){
	FILE *fBadPixels;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char path[64];
	sprintf(path, "./calibration/PT%02i_W%03i_C%03i_bad_pixels.txt", configGetPartType(), configGetWaferID(), configGetChipID());

	fBadPixels = fopen(path, "r");
	if (fBadPixels == NULL){
		printf("Could not open file.\n");
		return -1;
	}
	uint16_t i = 0;
	while((read = getline(&line, &len, fBadPixels)) != -1){
		char *pch = strtok(line," ");
		int n = 0;
		while (pch != NULL){
			n++;
			if (n > 2){
				printf("File does not contain a proper calibration.\n");
				return -1;
			}
			int16_t val = helperStringToInteger(pch);
			badPixels[i] = val;
			i++;
			pch = strtok(NULL," ");
		}
	}
	fclose(fBadPixels);
	if (line){
		free(line);
	}
	return i;
}

/*!
 Performs a calibration.
 Offset will be set to the default (computed in this part)
 @param distance distance to a white wall in cm
 @param FOV horizontal field of view in deg
 @param deviceAddress i2c address of the chip (default 0x20)
 @param factory if factory != 0 the calibration files will be stored as factory calibration files else as user
 @return 0 on success, -1 on error.
 */
int calibrationPixel(const int distance, const int FOV, const int deviceAddress, const int factory) {
	op_mode_t currentMode = configGetCurrentMode();
	if (currentMode == GRAYSCALE) {
		//device was in GRAYSCALE mode, will change mode to TOF for measurement
		apiLoadConfig(1, deviceAddress);
	}
	int currentAmplitude = pruGetMinAmplitude();
	pruSetMinAmplitude(0);
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', ROI_TL_X_HI, 6, &i2cData);
	const int topLeftX = (i2cData[0] << 8) + i2cData[1];
	const int bottomRightX = (i2cData[2] << 8) + i2cData[3];
	const int topLeftY = i2cData[4];
	const int bottomRightY = i2cData[5];
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		apiSetROI(0, 167, 0, 71, deviceAddress);
	} else {
		apiSetROI(0, 327, 0, 125, deviceAddress);
	}

	uint16_t *pMem;
	const uint16_t maxDistanceCM = SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(deviceAddress) / 10;
	const uint16_t phase = MAX_PHASE / maxDistanceCM * distance;
	memset(correctionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
	memset(subCorrectionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
	memset(avgMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
	
	offsetPhase = 0;
	offsetPhaseDefault = 0;

	int l, i, x;
	int deltaX, deltaY;
	int bottomSide = 0;
	int topSide = 0;

	int leftSide = topLeftX / pruGetColReduction();
	int rightSide = (nColsMax-topLeftX) / pruGetColReduction();

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		topSide = topLeftY;
		bottomSide = (nColsMax / pruGetColReduction()) * ((nRows+topLeftY) / pruGetRowReduction());
	}else{
		bottomSide = (nColsMax / pruGetColReduction()) * (nRows / pruGetRowReduction());
	}

	// Determine phase difference between measured phase and center of phase range:
	int phaseShiftForAveraging = 0;
	for (l = 0; l < N_MEASUREMENTS_PHASE_SHIFT; l++) {
		calculationDistance(&pMem);
		for (i = 0; i < (nColsMax / pruGetColReduction()) * (nRowsMax / pruGetRowReduction()); i++) {
			if( (i>= topSide) && (i % (nColsMax/pruGetColReduction()) >= leftSide) &&  (i < bottomSide) && (i % (nColsMax/pruGetColReduction())< rightSide)){
				x = i % (nColsMax / pruGetColReduction());
				deltaX = pruGetColReduction() * (x - xCenter / pruGetColReduction());
				deltaY = i / (2*nColsMax / pruGetColReduction()) * pruGetRowReduction();
				if (calculationIsDualMGX()) {
					deltaY *= 2;
				}
				if (deltaX >= -REF_ROI_HALF_WIDTH * pruGetColReduction() && deltaX < REF_ROI_HALF_WIDTH * pruGetColReduction()
				    && deltaY < REF_ROI_HALF_WIDTH * pruGetRowReduction() * pow(2, calculationIsDualMGX())) {
					phaseShiftForAveraging += pMem[i];
				}
			}
		}
	}

	phaseShiftForAveraging = (double) phaseShiftForAveraging / (N_MEASUREMENTS_PHASE_SHIFT * REF_ROI_HALF_WIDTH * REF_ROI_HALF_WIDTH * 4) + 0.5 - PHASE_RANGE_HALF;
	calibrationTemperature = temperatureGetAveragedChipTemperature(deviceAddress);

	for(l = 0; l < NUMBER_OF_MEASUREMENTS; l++) {
		calculationDistance(&pMem);
		for (i = 0; i < (nColsMax / pruGetColReduction()) * (nRowsMax / pruGetRowReduction()); i++) {
			if( (i>= topSide) && (i % (nColsMax/pruGetColReduction())>=leftSide) && (i < bottomSide) && (i % (nColsMax/pruGetColReduction())< rightSide)){
				avgMap[i] += (pMem[i] - phaseShiftForAveraging + MODULO_SHIFT) % MAX_DIST_VALUE;
			}
		}
	}

	double distToCenterInPix;
	double phaseReal, phaseDiffToCenter;
	double radPerPix = PI * FOV / 180.0 / nCols;
	int measRefDistPhase = 0;
	// Computation of average measured distance:
	for (i = 0; i < (nColsMax / pruGetColReduction()) * (nRowsMax / pruGetRowReduction()); i++) {
		if( (i>= topSide) && (i % (nColsMax/pruGetColReduction()) >= leftSide) && (i< bottomSide) && (i % (nColsMax/pruGetColReduction()) < rightSide)){
			x = i % (nColsMax / pruGetColReduction());
			deltaX = pruGetColReduction() * (x - xCenter / pruGetColReduction());
			deltaY = i / (2*nColsMax / pruGetColReduction()) * pruGetRowReduction();
			if (calculationIsDualMGX()) {
				deltaY *= 2;
			}
			if (deltaX >= -REF_ROI_HALF_WIDTH * pruGetColReduction() && deltaX < REF_ROI_HALF_WIDTH * pruGetColReduction()
			 && deltaY < REF_ROI_HALF_WIDTH * pruGetRowReduction() * pow(2, calculationIsDualMGX())) {
				measRefDistPhase += avgMap[i];
			}
		}
	}
	measRefDistPhase = (double) measRefDistPhase / (NUMBER_OF_MEASUREMENTS * REF_ROI_HALF_WIDTH * REF_ROI_HALF_WIDTH * 4) + 0.5;
	measRefDistPhase = (measRefDistPhase + phaseShiftForAveraging + MODULO_SHIFT) % MAX_DIST_VALUE;
	offsetPhase = phase - measRefDistPhase;
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);

	for (i = 0; i < (nColsMax / pruGetColReduction()) * (nRowsMax / pruGetRowReduction()); i++)
	{
		if( (i>= topSide) && (i % (nColsMax/pruGetColReduction()) >= leftSide) && (i < bottomSide) && (i % (nColsMax/pruGetColReduction())< rightSide)){
			x = i % (nColsMax / pruGetColReduction());
			deltaX = pruGetColReduction() * (x - xCenter / pruGetColReduction());
			deltaY = i / (2*nColsMax / pruGetColReduction()) * pruGetRowReduction();
			if (calculationIsDualMGX()) {
				deltaY *= 2;
			}
			distToCenterInPix = sqrt(deltaX * deltaX + deltaY * deltaY);
			phaseDiffToCenter = tan(distToCenterInPix * radPerPix) * phase;
			phaseReal = sqrt(phase * phase + phaseDiffToCenter * phaseDiffToCenter);
			correctionMap[i] = phaseReal - (((int) ((double) avgMap[i] / NUMBER_OF_MEASUREMENTS) + phaseShiftForAveraging + MODULO_SHIFT) % MAX_DIST_VALUE + offsetPhase + 0.5);
		} else {
			correctionMap[i] = 0;
		}
	}
printf("[%s:%d]correctionMap[0] = %d\n", __func__, __LINE__, correctionMap[0]);
	calibrationTemperature = (calibrationTemperature + temperatureGetAveragedChipTemperature(deviceAddress)) / 2.0;

	int fileStat = calibrationSave(calibrationCalculateStoreID(deviceAddress), factory);

	pruSetMinAmplitude(currentAmplitude);

	if (currentMode == GRAYSCALE) {
		//device was in GRAYSCALE mode before measuring, will change mode to GRAYSCALE
		apiLoadConfig(0, deviceAddress);
	}
	apiSetROI(topLeftX, bottomRightX, topLeftY, bottomRightY, deviceAddress);
	free(i2cData);
	offsetPhaseDefault = offsetPhase; //set offsetDefault to this one
	offsetPhaseDefaultEnabled = 1; // take default offset
	return fileStat;
}

/*!
 Returns a number to identify the current mode.
 The number is structured as follows:
 (1 is enabled, 0 is disabled if not described)
 Bit 0 - 2 = modulation frequency (0 = 625kHz ... 5 = 20Mhz)
 Bit 3 = ABS
 Bit 4 = PI_DELAY
 Bit 5 = BINNING_VERTICAL
 Bit 6 = BINNNING_HORIZONTAL
 Bit 7 - 8 = ROW_REDUCTION
 Bit 9 = DUAL_MGX_MOTION_BLUR
 Bit 10 = PN
 Bit 11 = DUAL_MGX_HDR
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns storeID to identify the mode
 */
unsigned int calibrationCalculateStoreID(const int deviceAddress) {
	int storeID;
	switch (configGetModFreqMode()) {
	case MOD_FREQ_625KHZ:
		storeID = FREQUENCY_625;
		break;
	case MOD_FREQ_1250KHZ:
		storeID = FREQUENCY_1250;
		break;
	case MOD_FREQ_2500KHZ:
		storeID = FREQUENCY_2500;
		break;
	case MOD_FREQ_5000KHZ:
		storeID = FREQUENCY_5000;
		break;
	case MOD_FREQ_20000KHZ:
		storeID = FREQUENCY_20000;
		break;
	case MOD_FREQ_10000KHZ:
	default:
		storeID = FREQUENCY_10000;
		break;
	}
	if (configIsABS(deviceAddress)) {
		storeID |= ABS;
	}
	if (calculationIsPiDelay()) {
		storeID |= PI_DELAY;
	}
	if (calculationIsDualMGX()) {
		storeID |= DUAL_MGX_MOTION_BLUR;
	}
	if (readOutIsVerticalBinning(deviceAddress)) {
		storeID |= BINNING_VERTICAL;
	}
	if (readOutIsHorizontalBinning(deviceAddress)) {
		storeID |= BINNING_HORIZONTAL;
	}
	switch (pruGetRowReduction()) {
	case 2:
		storeID |= ROW_REDUCTION_2;
		break;
	case 4:
		storeID |= ROW_REDUCTION_4;
		break;
	case 8:
		storeID |= ROW_REDUCTION_8;
		break;
	case 1:
	default:
		storeID |= ROW_REDUCTION_1;
		break;
	}
	if (calculationIsPN()) {
		storeID |= PN;
	}
	if (calculationIsHDR()) {
		storeID |= DUAL_MGX_HDR;
	}
	return storeID;
}

/*!
 Gets the correction values (FPN)
 @returns an array with correction values
 */
int32_t* calibrationGetCorrectionMap() {
	return subCorrectionMap;
}

/*!
 Scales the correction array to the ROI.
 */
void calibrationSetCorrectionMap(const int deviceAddress) {
	if (calibrationEnable) {
		unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
		i2c(deviceAddress, 'r', ROI_TL_X_HI, 6, &i2cData);
		
		int rowReduction;
		if (calculationIsDualMGX()) {
			rowReduction = 2;
		} else {
			rowReduction = pruGetRowReduction();
		}

		const int topLeftX = ((i2cData[0] << 8) + i2cData[1]) / pruGetColReduction();
		const int bottomRightX = ((i2cData[2] << 8) + i2cData[3]) / pruGetColReduction();
		const int topLeftY = i2cData[4];
		const int bottomRightY = i2cData[5];
		int middlePart;
		int topBottomPart;

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
			middlePart = (topLeftY / rowReduction) * (nColsMax / pruGetColReduction());
			topBottomPart = (nColsMax / pruGetColReduction()) * (nRowsMax / rowReduction) - (topLeftY / rowReduction) * (nColsMax / pruGetColReduction());
		}else{
			middlePart = ((nRowsMax/2-1 - bottomRightY) / rowReduction) * pruGetNumberOfHalves() * (nColsMax / pruGetColReduction());
			topBottomPart = (nColsMax / pruGetColReduction()) * (nRowsMax / rowReduction) - (topLeftY / rowReduction) * pruGetNumberOfHalves() * (nColsMax / pruGetColReduction());
		}

		int sub = 0; // index for subCorrectionMap
		for (int i = 0; i < (nColsMax / pruGetColReduction()) * (nRowsMax / rowReduction); i++) {
			if (i < middlePart || i >= topBottomPart) { // check whether i is on a valid row
				//do nothing
			} else if ((i % (nColsMax / pruGetColReduction()) < topLeftX) || (i % (nColsMax / pruGetColReduction()) > bottomRightX)) {  // check whether i is on a valid column
				//do nothing
			} else {
				subCorrectionMap[sub] = correctionMap[i];
				sub++;
			}
		}
		free(i2cData);
	}
}

//TODO: Scale correction map to reduction.
/*!
 Loads the corresponding offset and calibration file for a given setting. If available the user offset and calibration will be loaded instead of the factory offset and calibration.
 @param storeID Id for the settings
 @param deviceAddress i2c address of the chip (default 0x20)
 @return 1 on success, 0 or -1 on error
 */
int calibrationLoad(const int storeID, const int deviceAddress) {

	if(calibrationLoadOffset(storeID, deviceAddress) == 1){
		return calibrationLoadFPN(storeID, deviceAddress);
	}else{
		return -1;
	}
}
/*!
 Loads the corresponding calibration file for a given setting. If available the user calibration will be loaded instead of the factory calibration.
 @param storeID Id for the settings
 @param deviceAddress i2c address of the chip (default 0x20)
 @return 1 on success, 0 or -1 on error.
 */
int calibrationLoadFPN(const int storeID, const int deviceAddress) {
	if (calibrationEnable == 0) { //if FPN disabled
		memset(correctionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
		memset(subCorrectionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
		availableCalibration = 0;
		return 0;
	}
	char pathUserFPN[64];
	sprintf(pathUserFPN, "./calibration/PT%02i_W%03i_C%03i_calibration_user_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
	FILE *fileUserFPN = fopen(pathUserFPN, "rb");
printf("open %s\n", pathUserFPN);
	if (fileUserFPN == NULL) { //if user calibration is not existing, load factory calibration
		char pathFPNFactory[64];
		sprintf(pathFPNFactory, "./calibration/PT%02i_W%03i_C%03i_calibration_factory_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
		FILE *fileFPNFactory = fopen(pathFPNFactory, "rb");
printf("open %s\n", pathFPNFactory);
		if (fileFPNFactory == NULL ) {	// if error, try again
			fileFPNFactory = fopen(pathFPNFactory, "rb");
			if (fileFPNFactory == NULL ){
				memset(correctionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
				memset(subCorrectionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
				availableCalibration = 1;
				return -1;
			}
		}
		fread(correctionMap, sizeof(int32_t), nColsMax * nRowsMax, fileFPNFactory);
printf("[%s:%d]correctionMap[0] = %d\n", __func__, __LINE__, correctionMap[0]);
		fclose(fileFPNFactory);
		calibrationSetCorrectionMap(deviceAddress);
		availableCalibration = 2;
		return 1;
	}

	fread(correctionMap, sizeof(int32_t), nColsMax * nRowsMax, fileUserFPN);
printf("[%s:%d]correctionMap[0] = %d\n", __func__, __LINE__, correctionMap[0]);
	fclose(fileUserFPN);
	calibrationSetCorrectionMap(deviceAddress);
	availableCalibration = 3;
	return 1;
}

/*!
 Loads the corresponding offset file for a given setting. If available the user offset will be loaded instead of the factory offset.
 @param storeID Id for the settings
 @param deviceAddress i2c address of the chip (default 0x20)
 @return 1 on success, -1 on error.
 */
int calibrationLoadOffset(const int storeID, const int deviceAddress){
	char pathUserOffset[64];
	sprintf(pathUserOffset, "./calibration/PT%02i_W%03i_C%03i_offset_user_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
	FILE *fileUserOffset = fopen(pathUserOffset, "rb");
printf("open %s\n", pathUserOffset);
	if (fileUserOffset == NULL) { //if user offset is not existing, load factory offset
		char pathOffsetFactory[64];
		sprintf(pathOffsetFactory, "./calibration/PT%02i_W%03i_C%03i_offset_factory_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
		FILE *fileOffsetFactory = fopen(pathOffsetFactory, "rb");
printf("open %s\n", pathOffsetFactory);
		if (fileOffsetFactory == NULL) {
			memset(correctionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
			memset(subCorrectionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
			availableCalibration = 1;
			return -1;
		}
		fread(&offsetPhaseDefault, sizeof(int), 1, fileOffsetFactory);
		if (!fread(&calibrationTemperature, sizeof(double), 1, fileOffsetFactory)) {
			calibrationTemperature = 45.0;
		}
		printf("---------------------------------\n");
		printf("calibrationTemperature = %.2f\n", calibrationTemperature);
		if (offsetPhaseDefaultEnabled) {
			offsetPhase = offsetPhaseDefault;
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);
		}
		fclose(fileOffsetFactory);
		return 1;
	}

	fread(&offsetPhaseDefault, sizeof(int), 1, fileUserOffset);
	if (!fread(&calibrationTemperature, sizeof(double), 1, fileUserOffset)) {
		calibrationTemperature = 45.0;
	}
	printf("calibrationTemperature = %.2f\n", calibrationTemperature);
	printf("---------------------------------\n");
	if (offsetPhaseDefaultEnabled) {
		offsetPhase = offsetPhaseDefault;
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);
	}
	fclose(fileUserOffset);
	return 1;
}

void calibrationSetEnable(int state) {
	calibrationEnable = state;
}

/*!
 Saves a user calibration.
 @param storeID ID for the settings
 @param factory if factory != 0 the calibration files will be stored as factory calibration files else as user.
 @return 0 on success, -1 on error.
 */
int calibrationSave(const int storeID, const int factory) {
	char pathFPN[64];
	char pathOffset[64];
	struct stat st;

	if (factory) {
		sprintf(pathFPN, "./calibration/PT%02i_W%03i_C%03i_calibration_factory_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
		sprintf(pathOffset, "./calibration/PT%02i_W%03i_C%03i_offset_factory_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
	} else {
		sprintf(pathFPN, "./calibration/PT%02i_W%03i_C%03i_calibration_user_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
		sprintf(pathOffset, "./calibration/PT%02i_W%03i_C%03i_offset_user_%i.bin", configGetPartType(), configGetWaferID(), configGetChipID(), storeID);
	}
	FILE *fileFPN = fopen(pathFPN, "wb");
	fwrite(correctionMap, sizeof(int32_t), nColsMax * nRowsMax, fileFPN);
	fclose(fileFPN);
	// Prevent empty calibration files due to wrong partition size:
	if (stat(pathFPN, &st) == 0) {
		if((int)st.st_size == 0) {
			remove(pathFPN);
			return -1;
		}
	} else {
		remove(pathFPN);
		return -1;
	}
	FILE *fileOffset = fopen(pathOffset, "wb");
	fwrite(&offsetPhase, sizeof(int), 1, fileOffset);
	fwrite(&calibrationTemperature, sizeof(double), 1, fileOffset);
	fclose(fileOffset);
	if (stat(pathOffset, &st) == 0) {
		if((int)st.st_size == 0) {
			remove(pathOffset);
			return -1;
		}
	} else {
		remove(pathOffset);
		return -1;
	}
	return 0;
}

/*!
 Restores the default calibration (user calibration will be lost after this).
 @returns On success, 0, on error -1
 */
int calibrationRestore(const int deviceAddress) {
	DIR *d;
	struct dirent *dir;
	d = opendir("./calibration");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strstr(dir->d_name, "user")) {
				char path[256] = "./calibration/";
				strcat(path, dir->d_name);
				remove(path);
			}
		}
		closedir(d);
	}
	memset(correctionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
	memset(subCorrectionMap, 0, nColsMax * nRowsMax * sizeof(int32_t));
	calibrationEnable = 0;
	return 0;
}

/*!
 Sets the offset.
 @param offset as phase
 @returns offset as phase
 */
int calibrationSetOffset(const int value, const int deviceAddress) {
	//offsetPhaseDefaultEnabled = 0;
	offsetPhase = MAX_PHASE / (SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(deviceAddress) / 10) * value;
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);
	return offsetPhase;
}

/*!
 Getter function for the Offset (is used for the calculation on the BeagleBone)
 @returns offset as phase
 */
int calibrationGetOffsetPhase() {
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);
	return offsetPhase;
}

/*!
 Getter function for the default offset in cm
 @returns offset in cm
 */
int calibrationGetOffset(const int deviceAddress) {
	return offsetPhaseDefault / MAX_PHASE * SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(deviceAddress) / 10;
}

/*!
 Enables or disables the default offset.
 @param state if 0 the Offset set over calibrationSetOffset will be used else the offset from the calibration
 @returns state
 */
int calibrationEnableDefaultOffset(int state) {
	if (state == 0) {
		offsetPhaseDefaultEnabled = 0;
	} else {
		offsetPhaseDefaultEnabled = 1;
		offsetPhase = offsetPhaseDefault;
printf("[%s:%d]offsetPhase = %d\n", __func__, __LINE__, offsetPhase);
	}
	return state;
}

/*!
 Gets the list of all the storeIDs a calibration file is available for.
 The unsigned int will be structured like:
 [storeID][factory = 1, user =  0][currently loaded = 1, currently not loaded = 0].
 Check the function calibrationCalculateStoreID to see how the storeID is structured.
 @param calibrationList pointer to the calibration list
 */
int calibrationGetCalibrationList(int *calibrationList, const int deviceAddress) {
	DIR *d;
	char fileName[256];
	char *codeBegin;
	char code[128];
	int length;
	struct dirent *dir;
	int nOfFiles = 0;
	int value;
	int currentCalibration = calibrationCalculateStoreID(deviceAddress);
	int currentType;
	char calibrationIdentifier[32]; // unique

	sprintf(calibrationIdentifier, "PT%02i_W%03i_C%03i_calibration", configGetPartType(), configGetWaferID(), configGetChipID());
	d = opendir("./calibration");

	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strstr(dir->d_name, calibrationIdentifier)) { // strstr returns a pointer to first occurrence of the calibration identifier.
				strcpy(fileName, dir->d_name);
				codeBegin = strrchr(fileName, '_') + 1; // strrchr returns a pointer to last occurrence of '_'.
				length = strlen(codeBegin) - 4;
				strncpy(code, codeBegin, length);
				if (strstr(dir->d_name, "factory")) {
					if (availableCalibration == 2) {
						currentType = 1;
					} else {
						currentType = 0;
					}
					code[length] = '1';
				} else {
					if (availableCalibration == 3) {
						currentType = 1;
					} else {
						currentType = 0;
					}
					code[length] = '0';
				}
				char str[16];
				sprintf(str, "_%d", currentCalibration);
				if (strstr(dir->d_name, str) && currentType) {
					code[length + 1] = '1';
				} else {
					code[length + 1] = '0';
				}
				code[length + 2] = '\0';
				value = strtol(code, NULL, 10);
				calibrationList[nOfFiles] = value;
				nOfFiles++;
			}
		}
	}
	closedir(d);
	return nOfFiles;
}

/*!
 Returns the loaded calibration type.
 @returns 0 for calibration disabled, 1 for no calibration, 2 for factory, 3 for user
 */
int calibrationIsCalibrationAvailable() {
	return availableCalibration;
}

int calibrationIsCalibrationEnabled() {
	return calibrationEnable;
}

int calibrationIsDefaultOffsetEnabled() {
	return offsetPhaseDefaultEnabled;
}

void calibrationSetNumberOfColumnsMax(const uint16_t value) {
	nColsMax = value;
	nCols    = nColsMax - 8;
	xCenter  = nColsMax / 2;
}

void calibrationSetNumberOfRowsMax(const uint16_t value) {
	nRowsMax = value;
	nRows    = nRowsMax - 12;
}

//=====================================================================================
//
//	calibrate DRNU correction
//
//=====================================================================================
int calibrationGetOffsetDRNU(){
	return offsetDRNU;
}

int calibrationDRNU(){
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(sizeof(unsigned char));
	op_mode_t currentMode = configGetCurrentMode();
	if (currentMode == GRAYSCALE) { //device was in GRAYSCALE mode, will change mode to TOF for measurement
		apiLoadConfig(1, deviceAddress);
	}
	int currentAmplitude = pruGetMinAmplitude();
	pruSetMinAmplitude(0);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		apiSetROI(4, 163, 6, 65, deviceAddress);
		gWidth  = 160;
		gHeight = 60;
	} else {
		apiSetROI(4, 323, 6, 125, deviceAddress);
		gWidth  = 320;
		gHeight = 240;

	}

	i2cData[0] = 0x04;
	i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);
	gNumDist = 50;

	int16_t tempSize;
	int16_t temps[4];
	int tempDiff = 10000;

	while(tempDiff > 10){	//repeat measurement till temperature difference 0.5 C
		for(int i=0; i<gNumDist; i++){
			i2cData[0] = i;
			i2c(deviceAddress, 'w', 0x73, 1, &i2cData);
			calibrationCreateDRNU_LUT(i);

			tempSize = apiGetTemperature(deviceAddress, temps);
			int temperature = 0;

			if (configGetPartType() == EPC660 || configGetPartType() == EPC502){
				for(int j=0; j<4; j++){
					temperature += temps[j];
				}
				temperatureDRNU[i] = temperature / 4;

			} else {
				temperatureDRNU[i] = temps[0];
			}


			printf("temperatureDRNU[%d] = %d  tempSize = %d\n", i, temperatureDRNU[i], tempSize);
		}

		tempDiff = temperatureDRNU[gNumDist-1] - temperatureDRNU[0];
		printf("tempDiff= %d\n", tempDiff);
	}



	double slope, offset;
	calibrationFindLine(&slope, &offset); //find slope and offset

	calibrationCorrectDRNU_LUT(slope, offset);	//correct slope

	calibrationSaveDRNU_LUT();

	calibrationSaveTemperatureDRNU();

	i2cData[0] = 0x01;
	i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x73, 1, &i2cData);

	free(i2cData);

	pruSetMinAmplitude(currentAmplitude);

	if (currentMode == GRAYSCALE) { //device was in GRAYSCALE mode, will change mode to TOF for measurement
		apiLoadConfig(0, deviceAddress);
	}

	return 0;
}

void calibrationCreateDRNU_LUT(int indx){
	int x, y, j, l;
	int stepCM = 30;	//distance cm
	int numAveraging = 100;	//number of averaged images
	static int offset = 0;

	printf("calibrationCreateDRNU_LUT: %d\n", indx);
	uint16_t  *pMem;
    uint16_t maxDistanceCM = SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(configGetDeviceAddress()) / 10;
    uint16_t step = stepCM * MAX_PHASE / maxDistanceCM;


	int M = gWidth * gHeight;
	int avr[M];

	for(l=0; l<M; l++) avr[l] = 0;

	for(j=0; j<numAveraging; j++){
		calculationDistanceSorted(&pMem);
		for(l=0; l < M; l++){
			int val = pMem[l];

			if(val <  offset  || indx > gNumDist-3 ){
				val += MAX_PHASE;
			}
			avr[l] += val;
		}
	}

	int average = 0;
	for(l=0; l < M; l++){
		pMem[l] = avr[l] / numAveraging;
		if(indx==0){
			average += pMem[l];
		}
	}

	if(indx==0){
		offset = average /M;
		offsetDRNU = offset;
	}

	for(l=0, y=0; y < gHeight; y++){
		for(x=0; x < gWidth; x++, l++){

			int value = pMem[l] - offsetDRNU;
			drnuLut[indx][y][x] = (int)(round(value - indx * step));	//1.0372  1.025

			if(x<5 && y<1){
				printf("value= %d  pMem[%d]= %d  drnuLut[%d]= %d  offset= %d\n", value, l, pMem[l], indx, drnuLut[indx][y][x], offsetDRNU);
			}
		}

		if(y<1){
			printf("\n");
		}
	}

}

int calibrationSaveDRNU_LUT(){
	int x, y, k;
	char* fileName;

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		fileName = "epc635_drnu.csv";
	} else {
		fileName = "epc660_drnu.csv";
	}

	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	fprintf(file,"%d\n", offsetDRNU);

	for (k=0; k < gNumDist; k++){
		for (y = 0; y < gHeight; y++){
			for (x = 0; x < gWidth-1; x++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		calibrationSaveDRNU_LUT_Test("epc635_drnu_test_xy.csv");
		calibrationSaveDRNU_LUT_TestYX("epc635_drnu_test_yx.csv");
	} else {
		calibrationSaveDRNU_LUT_Test("epc660_drnu_test_xy.csv");
		calibrationSaveDRNU_LUT_TestYX("epc660_drnu_test_yx.csv");
	}


	return 0;
}


int calibrationSaveTemperatureDRNU(){
	char* fileName;

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		fileName = "epc635_temperature_drnu.csv";
	} else {
		fileName = "epc660_temperature_drnu.csv";
	}

	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}


	for (int k=0; k < gNumDist; k++){
			fprintf(file,"%d\n", temperatureDRNU[k]);
	}

	fclose(file);
	return 0;
}



int calibrationSaveDRNU_LUT_Test(char* fileName){
	int x, y, k;
	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	for (y = 0; y < gHeight; y++){
		for (x = 0; x < gWidth; x++){
			for (k=0; k < gNumDist-1; k++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);
	return 0;
}

int calibrationSaveDRNU_LUT_TestYX(char* fileName){
	int x, y, k;
	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	for (x = 0; x < gWidth; x++){
		for (y = 0; y < gHeight; y++){
			for (k=0; k < gNumDist-1; k++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);
	return 0;
}



int calibrationLoadDRNU_LUT(){
	int x, y, k;
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	char* fileName;  //"epc635_drnu_w19_c236_int2500_T49.csv";
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		fileName = "epc635_drnu.csv";
	} else {
		fileName = "epc660_drnu.csv";
	}

	FILE *file = fopen(fileName, "r");
	if (file == NULL){
		printf("Can not open %s file\n", fileName);
		return -1;
	}

	if((read = getline(&line, &len, file)) != -1){
		char *pch = strtok(line,"\n");
		offsetDRNU = helperStringToInteger(pch);
	}

	printf("offsetDRNU= %d\n",offsetDRNU);

	for (k=0; k < gNumDist; k++){
		//printf("numDist: %d\n", k);
		for(y=0; y<gHeight; y++){
			if((read = getline(&line, &len, file)) != -1){
				char *pch = strtok(line," ");
				for (x=0; (pch != NULL) && (x<gWidth); x++){
					if (x > gWidth){
						printf("File does not contain a proper DRNU table coefficients\n");
						return -1;
					}

					drnuLut[k][y][x] = helperStringToInteger(pch);
					/*if( x < 10 && y < 2 ){	//TODO remove
						printf("%d\t", drnuLut[k][y][x]);
					}*/

					pch = strtok(NULL," ");
				}	// end for
			} else {
				printf("error calibrationLoadDRNU_LUT\n");
			}
			/*if(y < 2){
				printf("\n");
			}*/
		}	//for y
	}

	fclose(file);
	return 0;
}


void calibrationFindLine(double* slope, double *offset){
	int k, x, y;
	double avr1 = 0.0;
	double avr2 = 0.0;
	double numPixel = gHeight * gWidth;

	for(k=0, y=0; y<gHeight; y++){
		for(x=0; x<gWidth; x++){
			avr1 += drnuLut[k][y][x];
		}
	}

	avr1 /= numPixel;

	for(k=gNumDist-1, y=0; y<gHeight; y++){
		for(x=0; x<gWidth; x++){
			avr2 += drnuLut[k][y][x];
		}
	}

	avr2 /= numPixel;

	*slope = (avr2 - avr1) / gNumDist;
	*offset = 0;

}


void calibrationLinearRegresion(double* slope, double *offset){
	int i, j, x, y;
	double sumX  = 0;
	double sumY  = 0;
	double sumXX = 0;
	double sumXY = 0;

	for(x=0; x<gNumDist; x++){
		for(j=0; j<gHeight; j++){
			for(i=0; i<gWidth; i++){
				y = drnuLut[x][j][i];
				sumX  +=   x;
				sumXX += (x*x);
				sumY  +=   y;
				sumXY += (x*y);
			}
		}
	}

	double num = gNumDist * gWidth * gHeight;
	double m =  (num * sumXY - sumX*sumY)/(num*sumXX - sumX*sumX);
	double b =  (sumY - m * sumX) / num;

	*slope  =  m;
	*offset = b;
}


void calibrationCorrectDRNU_LUT(double slope, double offset){
	for(int k=0; k<gNumDist; k++){
		for(int y=0; y<gHeight; y++){
			for(int x=0; x<gWidth; x++){
				drnuLut[k][y][x] = drnuLut[k][y][x] - (k * slope + offset);
			}
		}
	}
}


void calibrationCorrectDRNU_Error(){
	for(int y=0; y<gHeight; y++){
		for(int x=0; x<gWidth; x++){
			int y0 = drnuLut[47][y][x];
			int y2 = drnuLut[49][y][x];
			drnuLut[48][y][x] = (y0+y2)/2;
		}
	}
}




