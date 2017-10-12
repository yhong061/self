#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <stdint.h>

#define DLL_LOCKS 4
#define PHASE_RANGE_HALF 1500
#define N_MEASUREMENTS_PHASE_SHIFT 10

int drnuLut[50][252][328];

int16_t calibrationLoadADCTestSeq();
int16_t calibrationLoadDefaultSeq();
int16_t calibrationSearchBadPixelsWithSat();
int16_t calibrationSearchBadPixelsWithMin();
uint16_t calibrationGetBadPixels(int16_t* badPixels);
int calibrationPixel(const int distance, const int FOV, const int deviceAddress, const int factory);
int32_t* calibrationGetCorrectionMap();
void calibrationSetCorrectionMap(const int deviceAddress);
void calibrationSetEnable(int state);
int calibrationLoad(const int storeID, const int deviceAddress);
int calibrationLoadOffset(const int storeID, const int deviceAddress);
int calibrationLoadFPN(const int storeID, const int deviceAddress);
int calibrationRestore(const int deviceAddress);
int calibrationSave(const int storeID, const int factory);
int calibrationSetFOV(const int angle);
int calibrationSetOffset(const int value, const int deviceAddress);
int calibrationGetOffsetPhase();
int calibrationGetOffset(const int deviceAddress);
int calibrationEnableDefaultOffset(int state);
unsigned int calibrationCalculateStoreID(const int deviceAddress);
int calibrationGetCalibrationList(int *calibrationList, const int deviceAddress);
int calibrationMarathon(const int distance, const int FOV);
int calibrationIsCalibrationAvailable();
int calibrationIsDefaultOffsetEnabled();
int calibrationIsCalibrationEnabled();
void calibrationSetNumberOfColumnsMax(const uint16_t value);
void calibrationSetNumberOfRowsMax(const uint16_t value);

int  calibrationGetOffsetDRNU();
int  calibrationDRNU();
void calibrationCreateDRNU_LUT(int indx);
int  calibrationSaveDRNU_LUT();
int  calibrationSaveTemperatureDRNU();
int  calibrationSaveDRNU_LUT_Test(char* fileName);
int  calibrationSaveDRNU_LUT_TestYX(char* fileName);
int  calibrationLoadDRNU_LUT();
void calibrationLinearRegresion(double* slope, double *offset);
void calibrationFindLine(double* slope, double *offset);
void calibrationCorrectDRNU_LUT(double slope, double offset);
void calibrationCorrectDRNU_Error();



#endif
