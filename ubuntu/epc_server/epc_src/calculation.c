
#define _GNU_SOURCE
#include "calculation.h"
#include "calc_def_pi_delay_sorted.h"
#include "calc_def_no_pi_delay_sorted.h"
#include "calc_mgx_pi_delay_sorted.h"
#include "calc_mgx_no_pi_delay_sorted.h"
#include "calc_pn_def_pi_delay_sorted.h"
#include "calc_pn_def_no_pi_delay_sorted.h"
#include "calc_pn_mgx_pi_delay_sorted.h"
#include "calc_pn_mgx_no_pi_delay_sorted.h"
#include "calc_bw_sorted.h"
#include "calc_def_pi_delay.h"
#include "calc_def_no_pi_delay.h"
#include "calc_mgx_pi_delay.h"
#include "calc_mgx_no_pi_delay.h"
#include "calc_pn_def_pi_delay.h"
#include "calc_pn_def_no_pi_delay.h"
#include "calc_pn_mgx_pi_delay.h"
#include "calc_pn_mgx_no_pi_delay.h"
#include "adc_overflow.h"
#include "pru.h"
#include "api.h"
#include "config.h"
#include "calibration.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "image_correction.h"
#include "image_processing.h"
#include "image_averaging.h"
#include "image_difference.h"
#include "helper.h"
#include <sys/types.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START 1
#define STOP  0

#define BRIGHT 1
#define DARK   0
#define SPEED_OF_LIGHT_DIV_2 150000000.0
#define MAX_PHASE 3000.0


static int piDelay = 1;
static int dualMGX = 0;
static int hdr = 0;
static int linear = 0;
static int pn = 0;

static double pixelFluxData[328 * 252 * 4];
static double pixelBGData[328 * 252 * 4];
static double koefA[4];
static double koefB[4];

static double grayScaleOffset[328 * 252];
static double grayScaleGain[328 * 252];

static int grayscaleBright[328 * 252];
static int grayscaleDark[328 * 252];

static int correctGrayscaleOffset = 0;
static int correctGrayscaleGain   = 0;

static int enableDRNU = 0;


/*!

 */
int calculationBWSorted(uint16_t **data) {
	int size = calcBWSorted(data);

	if( (correctGrayscaleOffset !=0)  || (correctGrayscaleGain !=0) ){ //if use grayscale offset or/and gain correction
		calculateGrayscaleCorrection(*data);
	}

	return size;
}

/*!

 */
int calculationDCSSorted(uint16_t **data) {

	if(configGetCorrectionBGMode() !=0 ){
		uint16_t *pMem = NULL;
		int deviceAddress = configGetDeviceAddress();
		apiLoadConfig(0, deviceAddress);	//Grayscale
		apiGetBWSorted(&pMem);
		apiLoadConfig(1, deviceAddress);	//THREED
	}

	int size = calculationBWSorted(data);

	if(configGetCorrectionBGMode() !=0 ){
		calculationBGCorrection(data, 0);
	}

	return size;
}

/*!

 */
int calculationDistanceSorted(uint16_t **data) {
	int size;

//printf("dualMGX = %d, piDelay = %d, pn = %d\n", dualMGX, piDelay, pn); //0,1,0
	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				size = calcPNMGXPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcMGXPiDelayGetDataSorted(DIST, data);
			}
		} else {
			if (pn) {
				size = calcPNMGXNoPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcMGXNoPiDelayGetDataSorted(DIST, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				size =  calcPNDefPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcDefPiDelayGetDataSorted(DIST, data);
			}
		} else {
			if (pn) {
				size = calcPNDefNoPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcDefNoPiDelayGetDataSorted(DIST, data);
			}
		}
	}

	test_saveFile("pidelay", (char *)*data, size*sizeof(uint16_t));

		printf("[%s:%d]\n", __func__, __LINE__);
	imageCorrect(data);
	test_saveFile("correct", (char *)*data, size*sizeof(uint16_t));
	imageProcess(data);
	test_saveFile("Process", (char *)*data, size*sizeof(uint16_t));
	imageAverage(data);
	test_saveFile("Average", (char *)*data, size*sizeof(uint16_t));
	imageDifference(data);
	test_saveFile("Diff", (char *)*data, size*sizeof(uint16_t));
	//printf("calculationDistanceSorted\n");
	calculationCorrectDRNU(data);
	test_saveFile("drnu", (char *)*data, size*sizeof(uint16_t));
	return size;
}

/*!

 */
int calculationAmplitudeSorted(uint16_t **data) {

	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				return calcPNMGXPiDelayGetDataSorted(AMP, data);
			} else {
				return calcMGXPiDelayGetDataSorted(AMP, data);
			}
		} else {
			if (pn) {
				return calcPNMGXNoPiDelayGetDataSorted(AMP, data);
			} else {
				return calcMGXNoPiDelayGetDataSorted(AMP, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				return calcPNDefPiDelayGetDataSorted(AMP, data);
			} else {
				return calcDefPiDelayGetDataSorted(AMP, data);
			}
		} else {
			if (pn) {
				return calcPNDefNoPiDelayGetDataSorted(AMP, data);
			} else {
				return calcDefNoPiDelayGetDataSorted(AMP, data);
			}
		}
	}
}

/*!

 */
int calculationDistanceAndAmplitudeSorted(uint16_t **data) {
	int size;
//printf("dualMGX = %d, piDelay = %d, pn = %d\n", dualMGX, piDelay, pn); //0,1,0
	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				size = calcPNMGXPiDelayGetDataSorted(INFO, data);
			} else {
				size =  calcMGXPiDelayGetDataSorted(INFO, data);
			}
		} else {
			if (pn) {
				size =  calcPNMGXNoPiDelayGetDataSorted(INFO, data);
			} else {
				size =  calcMGXNoPiDelayGetDataSorted(INFO, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				size =  calcPNDefPiDelayGetDataSorted(INFO, data);
			} else {
				size = calcDefPiDelayGetDataSorted(INFO, data);
			}
		} else {
			if (pn) {
				size = calcPNDefNoPiDelayGetDataSorted(INFO, data);
			} else {
				size = calcDefNoPiDelayGetDataSorted(INFO, data);
			}
		}
	}
	imageCorrect(data);
	imageProcess(data);
	imageAverage(data);
	imageDifference(data);
	//printf("calculationDistanceAndAmplitudeSorted\n");
	calculationCorrectDRNU(data);
	return size;
}
/*!
 Enables or disables the Pi delay matching.
 @param state 1 for enable, 0 for disable
 */
void calculationEnablePiDelay(const int state) {
	piDelay = state;
}

/*!
 Enables or disables the dual MGX mode.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableDualMGX(const int state) {
	dualMGX = state;
}

/*!
 Enables or disables the HDR mode.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableHDR(const int state) {
	hdr = state;
}

/*!
 Enables or disables the calculation with square wave demodulation.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableLinear(const int state) {
	linear = state;
}

/*!
 Shows if Pi delay matching is enabled or disabled.
 @return 0 if Pi delay matching is enabled or 0 if it is disabled.
 */
int calculationIsPiDelay() {
	return piDelay;
}

/*!
 Shows if dualMGX is enabled or disabled.
 @return 0 if dualMGX is enabled or 0 if it is disabled.
 */
int calculationIsDualMGX() {
	return dualMGX;
}

/*!
 Shows if HDR is enabled or disabled.
 @return 0 if HDR is enabled or 0 if it is disabled.
 */
int calculationIsHDR() {
	return hdr;
}

void calculationEnablePN(const int state) {
	pn = state;
}

int calculationIsPN() {
	return pn;
}

/*!
 Gets a grayscale picture and masks the values with 0xFFF (12bit values)
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationBW(int16_t **data) {
	int size = pruGetImage((uint16_t**)data);
	int32_t *pMem32 = (int32_t*) *data;
	int16_t *pMem16 = (int16_t*) *data;
	int16_t value;
	int i;
	if (configGetCurrentMode() == FLIM){
		for (i = 0; i < size / 2; i++) {
			value = (pMem16[i] & 0xFFF) - 2048;
			if (value < 0){
				pMem16[i] = 0;
			}
			else{
				pMem16[i] = value;
			}
			value = (-(((int16_t)(pMem16[i + size / 2] & 0xFFF)) - 2048));
			if (value < 0){
				pMem16[i + size / 2] = 0;
			}
			else{
				pMem16[i + size / 2] = value;
			}
		}
	}
	else{
		for (i = 0; i < size / 2; i++) {
			pMem32[i] &= 0x0FFF0FFF;
		}
	}
	return size;
}

/*!
 Gets all the DCSs and masks the values with 0xFFF (12bit values)
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationDCS(uint16_t **data) {
	return calculationBW((int16_t**)data);
}

/*!
 Gets the distance values for every pixel
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationDistance(uint16_t **data) {
	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				return calcPNMGXPiDelayGetDist(data);
			} else {
				return calcMGXPiDelayGetDist(data);
			}
		} else {
			if (pn) {
				return calcPNMGXNoPiDelayGetDist(data);
			} else {
				return calcMGXNoPiDelayGetDist(data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				return calcPNDefPiDelayGetDist(data);
			} else {
				return calcDefPiDelayGetDist(data);
			}
		} else {
			if (pn) {
				return calcPNDefNoPiDelayGetDist(data);
			} else {
				return calcDefNoPiDelayGetDist(data);
			}
		}
	}
}

/*!
 Gets the amplitude values for every pixel.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationAmplitude(uint16_t **data) {
	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				return calcPNMGXPiDelayGetAmp(data);
			} else {
				return calcMGXPiDelayGetAmp(data);
			}
		} else {
			if (pn) {
				return calcPNMGXNoPiDelayGetAmp(data);
			} else {
				return calcMGXNoPiDelayGetAmp(data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				return calcPNDefPiDelayGetAmp(data);
			} else {
				return calcDefPiDelayGetAmp(data);
			}
		} else {
			if (pn) {
				return calcPNDefNoPiDelayGetAmp(data);
			} else {
				return calcDefNoPiDelayGetAmp(data);
			}
		}
	}
}

/*!
 Gets all the distance and amplitude values for every pixel
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationDistanceAndAmplitude(uint16_t **data) {
	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				return calcPNMGXPiDelayGetInfo(data);
			} else {
				return calcMGXPiDelayGetInfo(data);
			}
		} else {
			if (pn) {
				return calcPNMGXNoPiDelayGetInfo(data);
			} else {
				return calcMGXNoPiDelayGetInfo(data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				return calcPNDefPiDelayGetInfo(data);
			} else {
				return calcDefPiDelayGetInfo(data);
			}
		} else {
			if (pn) {
				return calcPNDefNoPiDelayGetInfo(data);
			} else {
				return calcDefNoPiDelayGetInfo(data);
			}
		}
	}
}

double *calculationBGFlux(uint16_t **data, int size){
	int x, y, i, k, l, m, n, r;
	int numDCS = 4;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculate Flux table: width= %d  height= %d\n", width, height);

	/*for(i=0; i<4; i++){
		for(j=0; j<4; j++){
			printf("%.4e\t", gKoef[i][j]);
		}
		printf("\n");
	}*/

	int time2D = configGetIntegrationTime2D();
	int time3D = configGetIntegrationTime3D();
	uint16_t *pMem = *data;

	for (i = 0; i < size; i++) {
		double value = (double)(pMem[i]-2048);
		if(value > 1800){
			value = 1800;
		}

		pixelFluxData[i] = (value * (double)time3D) / (double)time2D;  //160x60
	}


	for (l=0, k = 0; k < numDCS; k++){
		for (r=0, y = 0; y < height; y++ ){

			m=1;				//odd
			if( y%2 == 0 ) m=0;	//even

			n = m; //DCS0, DCS1
			if ( k > 1 ) n += 2; //DCS2, DCS3

			for (x = 0; x < width; x++, l++, r++ ){
				double value = pixelFluxData[r];
				pixelBGData[l] = (gKoef[n][0] + gKoef[n][1] * value + gKoef[n][2] * pow(value, 2) + gKoef[n][3] * pow(value, 3));
			}

		}
	}

	/*printf("Flux Table\n");
	for (l=0, k = 0; k < numDCS; k++){
		for (y = 0; y < height; y++ ){
			printf("i= %d\t", y);
			for (x = 0; x < width; x++, l++){
				if(x<10){
					printf("%.2f\t", pixelBGData[l]);
				}
			}
			printf("\n");
		}
		printf("\n\n");
	}*/

	return pixelBGData;
}

//===================================================================================================

double *calculationMultiBGFlux(uint16_t **data, int size){
	int x, y, i, k, l;
	int numDCS = 4;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculate Flux table: width= %d  height= %d\n", width, height);

	int time2D = configGetIntegrationTime2D();
	int time3D = configGetIntegrationTime3D();
	uint16_t *pMem = *data;

	for (i = 0; i < size; i++) {
		double value = (double)(pMem[i]-2048);
		if(value > 1800){
			value = 1800;
		}

		pixelFluxData[i] = (value * (double)time3D) / (double)time2D;  //160x60
	}

	for (l=0, k = 0; k < numDCS; k++){
		for (y = 0; y < height; y++ ){
			for (x = 0; x < width; x++, l++){
				double value = pixelFluxData[l];
				pixelBGData[l] = (gA[k][y][x] + gB[k][y][x] * value);
			}

		}
	}

	/*printf("Flux Table\n");
	for (l=0, k = 0; k < numDCS; k++){
		for (y = 0; y < height; y++ ){
			printf("i= %d\t", y);
			for (x = 0; x < width; x++, l++){
				if(x<10){
					printf("%.2f\t", pixelBGData[l]);
				}
			}
			printf("\n");
		}
		printf("\n\n");
	}*/

	return pixelBGData;
}


int calculationBGCorrection(uint16_t **data, int distMode){
	uint16_t *pMem = *data;
	int x, y, k, l;
	int numDCS = 4;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculate BG Correction: width= %d  height= %d\n", width, height);

	for (l=0, k = 0; k < numDCS; k++){
		for (x = 0; x < width; x++ ){
			for (y = 0; y < height; y++, l++ ){

				if(distMode == 1){ //for distance

					if( ((pMem[l] & 0x0001) == 0)  && (pMem[l]< adcOverflowGetMax()) && (pMem[l] > adcOverflowGetMin())  ){

						int16_t corrData = (int16_t)(16*pixelBGData[l]);
						pMem[l] -= (corrData & 0xFFF0);

					} else {

						printf("Saturated or ADC overflow\n");
					}

				} else {	//for DCS

					if( pMem[l] < LOW_AMPLITUDE) { //do no correct if value >= 30000: LOW_AMPLITUE or SATURATION or ADC_OVERFLOW
						double val = (pMem[l] - pixelBGData[l]);
						pMem[l] = (uint16_t)val;
					} else {
						printf("pMem[l] >= LOW_AMPLITUDE: %d  %d\n", l, pMem[l]);
					}
				}
			}
		}
    }

	return 0;
}

void calculationSaveCorrectionTable(int mode){
	FILE *file;

	if ( mode == CORR_BG_AVR_MODE ){

		file = fopen("./correctionTable.txt", "w");
		for(int i=0; i<4; i++){
			fprintf(file, "%.4e  %.4e  0.0000  0.0000\n", koefA[i], koefB[i]);
		}

	} else {

		char* filename;

		if(mode == CORR_BG_LINE_MODE) filename = "./correctionTableLine.txt";
		else 		  				  filename = "./correctionTablePixel.txt";

		file = fopen(filename, "w");
		for(int k=0; k<4; k++){
			for(int y= 0; y< 60; y++){
				for(int x=0; x<160; x++){
					fprintf(file, " %.4e", gA[k][y][x]);
				}
				fprintf(file, "\n");
			}
		}

		for(int k=0; k<4; k++){
			for(int y= 0; y< 60; y++){
				for(int x=0; x<160; x++){
					fprintf(file, " %.4e", gB[k][y][x]);
				}
				fprintf(file, "\n");
			}
		}
	}	// end else

	fclose(file);
}


double calculationCalcGrayscaleMean(){
	int x, y, l, m;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculationCalcGrayscaleMean: width= %d  height= %d\n", width, height);
	uint16_t *data = NULL;
	double  maxBGFlux = 0;

	int numMean = 20;		//number of averaged frames
	double tempBGFlux[20];

	apiLoadConfig(0, configGetDeviceAddress());

	for(m=0; m < numMean; m++){		//take numMean frames for averaging

		calcBWSorted(&data); 			//take gray scale image
		tempBGFlux[m] = 0.0;			//initial reset

		for(l=0, y=0; y< height; y++){
			for(x=0; x< width; x++, l++){
				tempBGFlux[m] += data[l];
			}
		}

		tempBGFlux[m] /= l;
	}


	for(m=0; m < numMean; m++){		//averaging BGflux over numMean frames
		maxBGFlux += tempBGFlux[m];
		printf("%.4e\n", (tempBGFlux[m]-2048));
	}

	maxBGFlux = (maxBGFlux/numMean)-2048;
	printf("%.4e\n", maxBGFlux);

	return maxBGFlux;
}


void calculationCreateBGTable(double *koefAB){
	int x, y, i, j, k, l, m, cnt, pos;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculationCreateBGTable: width= %d  height= %d\n", width, height);
	int numPixel2DCS = 2 * width * height;
	int numMean = 20;		//number of averaged frames
	double tempKoef[4][20];

	apiLoadConfig(1, configGetDeviceAddress());	//THREED
	configSetPhasePointActivate(START);

	for(m=0; m < numMean; m++){		//take numMean frames for averaging

		for(k=0; k < 50; k++){	// distances

			uint16_t *data = NULL;
			configSetPhasePoint(k);
			calcBWSorted(&data);

			for(l=0, i=0; i<2; i++){				// DCS01, DCS23
				for(j=0; j<2; j++, l++){			// odd, even
					for(y=0; y< height; y+=2){
						for(x=0; x< width; x++){
							pos = (y+j)*width + x + numPixel2DCS*i;
							tempKoef[l][m] += data[pos];
						} // for x
					}	// for y
				}	//for j
			}	//for i
		}	//for k

		cnt = 50 * 60 * 160;

		for(l=0; l<4; l++){
			tempKoef[l][m] /= cnt;
			double val = tempKoef[l][m];
			tempKoef[l][m] -= 2048.0;
			printf("%.4e\t%.4e\n", val, tempKoef[l][m]);
		}

	}	// for m

	/*for(m=0; m < numMean; m++){		//take numMean frames for averaging

		for(l=0, i=0; i<2; i++){				// DCS01, DCS23
			for(j=0; j<2; j++, l++){			// odd, even

				tempKoef[l][m] = 0.0;			//initial reset

				for(cnt=0, k=0; k < 50; k++){	// distances
					uint16_t *data = NULL;
					configSetPhasePoint(k);
					calcBWSorted(&data);

					for(y=0; y< height; y+=2){
						for(x=0; x< width; x++, cnt++){
							pos = (y+j)*width + x + numPixel2DCS*i;
							tempKoef[l][m] += data[pos];
						}
					}
				}	//for k

				tempKoef[l][m] /= cnt;
				double val = tempKoef[l][m];
				tempKoef[l][m] -= 2048.0;
				printf("%.4e\t%.4e\n", val, tempKoef[l][m]);

			}	//for j
		}	//for i
	}*/


	for(l=0; l<4; l++){			//averaging coeff over numMean frames
		for(m=0; m<numMean; m++){
			koefAB[l] += tempKoef[l][m];
		}

		koefAB[l] /=numMean;
		printf("%.4e\t",koefAB[l]);
	}
	printf("\n");

	configSetPhasePointActivate(STOP);

}


void calculationCalcAB(int flag){

	if (flag == 0){ //first run

		for (int i = 0; i<4; i++){	//reset values
			koefA[i] = 0.0;
			koefB[i] = 0.0;
		}

		printf("calculationCreateBGTable A\n");
		calculationCreateBGTable(koefA);

	} else {	//second run

		printf("calculationCreateBGTable B\n");
		calculationCreateBGTable(koefB);
		double maxBGFlux = calculationCalcGrayscaleMean();

		for(int i=0; i<4; i++){
			koefB[i] = (koefB[i] - koefA[i]) / maxBGFlux;
		}

		calculationSaveCorrectionTable(CORR_BG_AVR_MODE);
		configLoadCorrectionTable(CORR_BG_AVR_MODE);
	}
}

//===========================================================================================================


void calculationCreateBGMultiTable(int flag, int lineMode){
	int x, y, d, k, m, pos, cnt;
	int nDCS = 4;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculationCreateBGTable: width= %d  height= %d\n", width, height);
	int numPixelDCS = width * height;
	int numDist = 50;
	int numMean = 20;		//number of averaged frames
	double tempKoef[4][60][160];

	for(d=0; d<nDCS; d++)	// DCS 0,1,2,3
		for(y=0; y< height; y++)
			for(x=0; x< width; x++)
				tempKoef[d][y][x] = 0.0;



	apiLoadConfig(1, configGetDeviceAddress());	//THREED
	configSetPhasePointActivate(START);

	for(m=0; m < numMean; m++){		//take numMean frames for averaging

		for(k=0; k < numDist; k++){	// distances

			uint16_t *data = NULL;
			configSetPhasePoint(k);
			calcBWSorted(&data);

			for(d=0; d<nDCS; d++){	// DCS 0,1,2,3
				for(y=0; y< height; y++){
					for(x=0; x< width; x++){
						pos = y*width + x + d*numPixelDCS;
						tempKoef[d][y][x] += data[pos];
					}	//for x
				}	//for y
			}	//for d dcs
		}	//for k dist
	}	//for m


	if(lineMode == 1){	//line averaging
		cnt = width * numDist * numMean;
		for(d=0; d<nDCS; d++){
			for(y=0; y<height; y++){
				for(x=1; x<width; x++){
					tempKoef[d][y][0] += tempKoef[d][y][x];
				}	// for x

				if((flag == 2) || (flag == 4)){
					gA[d][y][x] = tempKoef[d][y][0] / cnt - 2048.0;
					printf("A= %.4e\n", gA[d][y][0]);

				} else {
					gB[d][y][x] = tempKoef[d][y][0] / cnt - 2048.0;
					printf("B= %.4e\n", gB[d][y][0]);
				}

			}	//for y
		}	//for d


		for(d=0; d<nDCS; d++){
			for(y=0; y< height; y++){
				for(x=1; x<width; x++){

					if((flag == 2) || (flag == 4)){
						gA[d][y][x] = gA[d][y][0];
					} else {
						gB[d][y][x] = gB[d][y][0];
					}

				}
			}
		}

	} else {	//every pixel
		cnt = numDist * numMean;
		for(d=0; d<nDCS; d++){
			for(y=0; y<height; y++){
				for(x=0; x<width; x++){

					if((flag == 2) || (flag == 4)){
						gA[d][y][x] = tempKoef[d][y][x] / cnt - 2048.0;
						if(x>20 && x<30) printf("%.4e\t", gA[d][y][x]);
					} else {
						gB[d][y][x] = tempKoef[d][y][x] / cnt - 2048.0;
						if(x>20 && x<30) printf("%.4e\t", gB[d][y][x]);
					}

				}	//for x
				printf("\n");
			}	//for y
		} //for d
	}

	configSetPhasePointActivate(STOP);

}


void calculationCalcMultiAB(int flag, int lineMode){

	if (flag == 2 || flag == 4){ //first run

		for (int k=0; k < 4; k++){	//reset values
			for (int i=0; i < 60; i++){
				for (int j=0; j < 160; j++){
					gA[k][i][j] = 0.0;
					gB[k][i][j] = 0.0;
				}
			}
		}

		printf("calculationCreateBGMultiTable A %d\n", lineMode);
		calculationCreateBGMultiTable(flag, lineMode);

	} else {	//second run

		printf("calculationCreateMultiBGTable B %d\n", lineMode);
		calculationCreateBGMultiTable(flag, lineMode);
		double maxBGFlux = calculationCalcGrayscaleMean();

		for (int k=0; k<4; k++){
			for (int i=0; i<60; i++){
				for (int j=0; j<160; j++){
					gB[k][i][j] = (gB[k][i][j] - gA[k][i][j]) / maxBGFlux; //gMaxBGFlux[k][i][j];
				}
			}
		}

		if(lineMode){
			calculationSaveCorrectionTable(CORR_BG_LINE_MODE);
			configLoadCorrectionTable(CORR_BG_LINE_MODE);
		} else {
			calculationSaveCorrectionTable(CORR_BG_PIXEL_MODE);
			configLoadCorrectionTable(CORR_BG_PIXEL_MODE);

		}
	}
}


//=====================================================================================
//
//	Grayscale correction
//
//=====================================================================================

void setCorrectGrayscaleOffset(int enable){
	correctGrayscaleOffset = enable;
}

void setCorrectGrayscaleGain(int enable){
	correctGrayscaleGain = enable;
}

void calculateGrayscaleCorrection(uint16_t* data){
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numPixels = width * height;

	if( correctGrayscaleOffset !=0 ) {
		for(int i=0; i< numPixels; i++) {
			int16_t value = data[i];
			if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
				data[i] = value - grayScaleOffset[i];
			}
		}
	}

	if( correctGrayscaleGain !=0 ) {
		for(int i=0; i< numPixels; i++) {
			uint16_t value = data[i];
			if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
				double gainCorrGray = ((double) value - 2048) * grayScaleGain[i];
				data[i] = (uint16_t)(gainCorrGray + 2048 + 0.5);
			}
		}
	}
}

//=========================================
// prepare
//=========================================

void calculationGrayscaleMean(int numAveragingImages, int* data){

	int x, y, l, m;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	printf("calculationGrayscaleMean: width= %d  height= %d\n", width, height);
	uint16_t *pData = NULL;
	int numMaxPixels = width * height;
	int tempImages[numMaxPixels];

	for(int i=0; i<numMaxPixels; i++){ 	//clean images
		tempImages[i] = 0;
	}

	apiLoadConfig(0, configGetDeviceAddress());

	for(m=0; m < numAveragingImages; m++){		//take numMean frames for averaging

		calcBWSorted(&pData); 			//take gray scale image

		for(l=0, y=0; y< height; y++){
			for(x=0; x< width; x++, l++){
				tempImages[l] += pData[l];
			}
		}
	}

	for(l=0, y=0; y< height; y++){
		for(x=0; x< width; x++, l++){
			data[l] = tempImages[l] / numAveragingImages;
		}
	}

}

void calculationGrayscaleGainOffset() {
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numPixels = width * height;
	double avr = 0.0;

	//calculate Offset and average
	for(int i=0; i<numPixels; i++){
		grayScaleOffset[i] = (double)grayscaleDark[i] - 2048.0;

		if(i<10){
			printf("grayScaleOffset[%d]= %.4e    %d\n", i, grayScaleOffset[i], grayscaleDark[i]);
		}

		avr +=  ((double)grayscaleBright[i] - (double)grayscaleDark[i]);
	}

	avr /= numPixels;
	printf("avr = %f ", avr);

	for(int i=0; i<numPixels; i++){

		double diff = (double)grayscaleBright[i] - (double)grayscaleDark[i];

		if(diff == 0) grayScaleGain[i] = 1;
		else          grayScaleGain[i] =  avr / diff;

		if(i<20){
			printf("grayScaleGain[%d]= %.4e\n", i, grayScaleGain[i]);
		}
	}

}

int calculationCalibrateGrayscale(int mode) {
	int numAveragingImages = 100;

	if (mode == BRIGHT){
		printf("save bright averaged %d images\n", numAveragingImages);
		calculationGrayscaleMean(numAveragingImages, grayscaleBright);
		calculateSaveGrayscaleImage(grayscaleBright); // save image
	} else {
		printf("save dark averaged %d images and calculate offset and gain\n", numAveragingImages);
		calculateLoadGrayscaleImage(grayscaleBright);
		calculationGrayscaleMean(numAveragingImages, grayscaleDark);
		calculateSaveGrayscaleImage(grayscaleDark);
		calculationGrayscaleGainOffset();
		calculateSaveGrayscaleGainOffset();
	}

	return 0;
}

//=========================================
// calculate
//=========================================

int calculateSaveGrayscaleImage(int *data){
	FILE* file;
	int x, y, l;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./grayscale_temp_image.txt";

	file = fopen(str, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x<width; x++, l++){
			fprintf(file, " %d", data[l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}


int calculateLoadGrayscaleImage(int *data){
	int x, y, l;
	char    *line = NULL;
	size_t   len = 0;
	ssize_t  read;
	FILE    *file;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./grayscale_temp_image.txt";

	file = fopen(str, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for(l=0, y=0; y < height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x < width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale offset table coefficients\n");
					return -1;
				}
				int val = helperStringToInteger(pch);
				data[l] = val;
				if( x < 10 ){
					printf("%d\t", data[l]);
				}
				pch = strtok(NULL," ");
			}	// end while
		} else {
			return -1; //error
		}

		printf("\n");
	}	//for y

	fclose (file);
	return 0;
}


//-------------------------------------------------------------------------------------

int calculateSaveGrayscaleGainOffset(){
	FILE* file;
	int x, y, l;
	char* str;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		str = "./grayscale635_gain_offset.txt";
	} else {
		str = "./grayscale660_gain_offset.txt";
	}

	file = fopen(str, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", grayScaleGain[l]);
		}
		fprintf(file, "\n");
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", grayScaleOffset[l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}


int calculateLoadGrayscaleGainOffset(){
	int x, y, l;
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	FILE*   file;
	char*   str;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int Num = width * height;

	for(int i=0; i< Num; i++){	//clean array
		grayScaleOffset[i] = 0.0;
		grayScaleGain[i]   = 1.0;
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		str = "./grayscale635_gain_offset.txt";
	} else {
		str = "./grayscale660_gain_offset.txt";
	}

	printf("Open %s\n", str);


	file = fopen(str, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	int error_gain=0;
	//load gain
	printf("Load grayscale gain\n");	//TODO remove
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale gain table coefficients\n");
					return -1;
				}

				grayScaleGain[l] = helperStringToDouble(pch);
				/*if( x < 10 ){	//TODO remove
					printf("%.4f\t", grayScaleGain[l]);
				}*/

				pch = strtok(NULL," ");
			}	// end while
		} else {
			error_gain++;
			printf("error gain: %d\n", error_gain);
			//return -1; //error
		}

		//printf("\n");
	}	//for y


	// load offset
	int error_offset = 0;

	printf("Load grayscale offset\n");	//TODO remove
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale offset table coefficients\n");
					return -1;
				}

				grayScaleOffset[l] = helperStringToDouble(pch);
				/*if( x < 10 ){	//TODO remove
					printf("%.4f\t", grayScaleOffset[l]);
				}*/

				pch = strtok(NULL," ");
			}	// end while
		} else {
			//return -1; //error
			error_offset++;
			printf("error offset: %d\n", error_offset);
		}

		//printf("\n");
	}	//for y

	fclose (file);
	return 0;
}

//=====================================================================================
//
//	DRNU correction
//
//=====================================================================================
void calculationSetEnableDRNU(int enable){
	enableDRNU = enable;
}

int calculationCorrectDRNU(uint16_t** data){
    int i, x, y, l, dist;
    int height;
    int width;
    int stepCM =  30;
    int offset = calibrationGetOffsetDRNU();
    uint16_t *pMem = *data;
    uint16_t maxDistanceCM = SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(configGetDeviceAddress()) / 10;
    uint16_t step = stepCM * MAX_PHASE / maxDistanceCM;

    if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
    	height = 60;
    	width = 160;
    } else {
    	height = 240;
    	width  = 320;
    }

    if( enableDRNU !=0 ){
		for(y=0; y< height; y++){
			for(x=0; x < width; x++){
				l = y * width + x;
				dist = pMem[l];

				if (dist < LOW_AMPLITUDE){	//correct if not saturated
					dist -= offset;

					if(dist<0){	//if phase jump
						dist += 3000;
					}

					i = (int)(round((double)dist / (double)step));

					if(i<0) i=0;
					else if (i>=50) i=49;

					pMem[l] -= drnuLut[i][y][x];

					if(x<8 && y<2){
						printf("dist = %d  drnuLut[%d]= %d  newDist= %d  offset= %d\n", dist, i, drnuLut[i][y][x], pMem[l], offset);
					}

				}
			}
		}
    }
    return 0;
}


int calculationCorrectDistance(int x, int y, int dist){
/*	int numDist = 49;
    int step = 60; //cm

    int MAX_PHASE = 3000.0;
    double SPEED_OF_LIGHT_DIV_2 = 150000000.0;

    //const int deviceAddress = configGetDeviceAddress();
    //const uint16_t maxDistanceCM = SPEED_OF_LIGHT_DIV_2 / configGetModulationFrequency(deviceAddress) / 10;
     * const uint16_t phase = MAX_PHASE / maxDistanceCM * dist1;

    int i = dist / step;
    dist -= drnuLut[i][y][x];
    return dist;

    for(int i=0; i<numDist; i++){

        int s0 = i * step;
        int s1 = (i+1) * step;
        int m0 = drnuLut[i][y][x];
        int m1 = drnuLut[i+1][y][x];
        if(dist >= m0 && dist < m1){
        	int dist1= calculationInterpolate(dist, m0, s0, m1, s1); //dist cm
        	const uint16_t phase = MAX_PHASE / maxDistanceCM * dist1;
            return (dist1 * 2);
        }

    printf("calculationCorrectDistance: -1\n");*/
    return -1;  //error
}

int calculationInterpolate(int x, int x0, int y0, int x1, int y1){
    if( x1 == x0 ){
        return y0;
    } else {
        return ((x-x0)*(y1-y0)/(x1-x0) + y0);
    }
}




