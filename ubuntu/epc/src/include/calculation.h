#ifndef _CALCULATION_H_
#define _CALCULATION_H_

#include <stdint.h>

#define MAX_DIST_VALUE 	3000
#define LOW_AMPLITUDE 	30000
#define SATURATION 		31000
#define ADC_OVERFLOW 	32000
#define MODULO_SHIFT 	30000
#define FP_M_2_PI 		3217.0 	/*! 2 PI in fp_s22_9 format*/
#define FP_M_PI			1608.5;

enum calculationType {
	DIST, AMP, INFO
} ;

double gA[4][60][160];
double gB[4][60][160];


int calculationBWSorted(uint16_t **data);
int calculationDCSSorted(uint16_t **data);
int calculationDistanceSorted(uint16_t **data);
int calculationAmplitudeSorted(uint16_t **data);
int calculationDistanceAndAmplitudeSorted(uint16_t **data);
void calculationEnablePiDelay(const int state);
void calculationEnableDualMGX(const int state);
void calculationEnableHDR(const int state);
void calculationEnableLinear(const int state);
void calculationEnablePN(const int state);
int calculationIsPiDelay();
int calculationIsDualMGX();
int calculationIsHDR();
int calculationIsPN();
int calculationBW(int16_t **data);
int calculationDCS(uint16_t **data);
int calculationDistance(uint16_t **data);
int calculationAmplitude(uint16_t **data);
int calculationDistanceAndAmplitude(uint16_t **data);
double *calculationBGFlux(uint16_t **data, int size);
double *calculationMultiBGFlux(uint16_t **data, int size);
void   calculationCalcAB();
void   calculationSaveCorrectionTable(int mode);
int    calculationBGCorrection(uint16_t **data, int distMode);
void   calculationCreateBGTable(double *koefAB);
double calculationCalcGrayscaleMean();

void calculationCalcMultiAB(int flag, int lineMode);
void calculationCreateBGMultiTable(int flag, int lineMode);

void setCorrectGrayscaleGain(int enable);
void setCorrectGrayscaleOffset(int enable);
void calculationGrayscaleMean(int numAveragingImages, int* data);
int  calculationCalibrateGrayscale(int mode);
void calculationGrayscaleGainOffset();
void calculateGrayscaleCorrection(uint16_t* data);

int calculateSaveGrayscaleGainOffset();
int calculateLoadGrayscaleGainOffset();
int calculateLoadGrayscaleImage(int *data);
int calculateSaveGrayscaleImage(int *data);

void calculationSetEnableDRNU(int enable);
int  calculationCorrectDRNU(uint16_t** data);
int  calculationCorrectDistance(int x, int y, int dist);
int  calculationInterpolate(int x, int x0, int y0, int x1, int y1);


#endif




