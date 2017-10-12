#ifndef _API_H_
#define _API_H_

#include <stdint.h>

int16_t apiSetROI(const int topLeftX, const int bottomRightX, const int topLeftY, const int bottomRightY, const int deviceAddress);
int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress);
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress);
int16_t apiEnableVerticalBinning(const int state, const int deviceAddress);
int16_t apiEnableHorizontalBinning(const int state, const int deviceAddress);
int16_t apiSetRowReduction(int value, const int deviceAddress);
int16_t apiEnableABS(const int state, const int deviceAddress);
int16_t apiLoadConfig(const int configIndex, const int deviceAddress);
int16_t apiSetIntegrationTime2D(const int us, const int deviceAddress);
int16_t apiSetIntegrationTime3D(const int us, const int deviceAddress);
int16_t apiSetIntegrationTime3DHDR(const int us, const int deviceAddress);
int16_t apiSetMinAmplitude(const int minAmplitude);
int16_t apiGetMinAmplitude();
int16_t apiSetModulationFrequency(const int index, const int deviceAddress);
int16_t* apiGetModulationFrequencies();
int16_t apiFLIMGetStep();
int16_t apiEnableDualMGX(const int state, const int deviceAddress);
int16_t apiEnableHDR(const int state, const int deviceAddress);
int16_t apiEnablePiDelay(const int state, const int deviceAddress);
int16_t apiCalibratePixel(const int distance, const int FOV, const int deviceAddress);
int16_t apiEnableCalibration(int state, const unsigned int deviceAddress);
int16_t apiEnableCorrectionBG(const int state, const unsigned int deviceAddress);
double apiSetCorrectionBG(int row, int col, double value);
int16_t apiRestoreCalibration(const int deviceAddress);
int16_t apiSetOffset(const int offset, const int deviceAddress);
int16_t apiGetOffset(const int deviceAddress);
int16_t apiEnableDefaultOffset(const int state);
int16_t apiGetCalibrationList(int *calibrationList, const int deviceAddress);
int16_t apiGetBadPixels(int16_t *badPixels);
int16_t apiGetTemperature(unsigned int deviceAddress, int16_t *temps);
int16_t apiGetAveragedTemperature(unsigned int deviceAddress);
int16_t apiIsCalibrationAvailable();
int16_t apiGetChipInfo(const int deviceAddress, int16_t *chipInfo);
int16_t apiSelectMode(int select, const int deviceAddress);
int16_t apiSelectPolynomial(unsigned int indexLFSR, unsigned int indexPolynomial, const int deviceAddress);
int16_t apiSetHysteresis(const int value);
int16_t apiEnableImageCorrection(const unsigned int state);
int16_t apiSetImageProcessing(const unsigned int mode);
int16_t apiSetImageAveraging(const unsigned int averageOver);
int16_t apiSetImageDifferenceThreshold(const unsigned int cm);
int16_t apiGetIcVersion();
int16_t apiGetPartVersion();
int16_t apiGetVersion();
int16_t apiEnableSaturation(int state);
int16_t apiEnableAdcOverflow(int state);
int16_t apiCreateBGTable(int mode);
int16_t apiDCSCorrection(char* str);

int16_t apiIsFLIM();
int16_t apiFLIMSetT1(uint16_t value);
int16_t apiFLIMSetT2(uint16_t value);
int16_t apiFLIMSetT3(uint16_t value);
int16_t apiFLIMSetT4(uint16_t value);
int16_t apiFLIMSetTREP(uint16_t value);
int16_t apiFLIMSetRepetitions(uint16_t value);
int16_t apiFLIMSetFlashDelay(uint16_t value);
int16_t apiFLIMSetFlashWidth(uint16_t value);


int apiGetBW(int16_t **data);
int apiGetDCS(uint16_t **data);
int apiGetDistance(uint16_t **data);
int apiGetAmplitude(uint16_t **data);
int apiGetDistanceAndAmplitude(uint16_t **data);

int apiGetBWSorted(uint16_t **data);
int apiGetDCSSorted(uint16_t **data);
int apiGetDistanceSorted(uint16_t **data);
int apiGetAmplitudeSorted(uint16_t **data);
int apiGetDistanceAndAmplitudeSorted(uint16_t **data);

int apiCorrectGrayscaleGain(int enable);
int apiCorrectGrayscaleOffset(int enable);
int apiCalibrateGrayscale(int mode);
int apiCalibrateDRNU();
int apiCorrectDRNU(int enable);
int16_t apiIsEnabledGrayscaleCorrection();
int16_t apiIsEnabledDRNUCorrection();





#endif
