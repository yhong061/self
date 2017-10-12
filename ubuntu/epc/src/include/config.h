#ifndef _CONFIG_H_
#define _CONFIG_H_

#define EPC660_MAX_INTEGRATION_TIME 100000
#define EPC660_MAX_COL 327
#define EPC660_MAX_ROW 125

#define MAX_INTEGRATION_TIME_2D 1000000
#define MAX_INTEGRATION_TIME_3D 4000

#define CORR_BG_AVR_MODE   0
#define CORR_BG_LINE_MODE  2
#define CORR_BG_PIXEL_MODE 3


#include <stdint.h>

enum partType {
	EPC502_OBS = 1,
	EPC660     = 2,
	EPC502     = 3,
	EPC635     = 4,
	EPC503     = 5
};

typedef enum {
	GRAYSCALE, THREED, FLIM, NumberOfModes
} op_mode_t;
int getModeCount();

typedef enum {
	MOD_FREQ_20000KHZ,
	MOD_FREQ_10000KHZ,
	MOD_FREQ_5000KHZ,
	MOD_FREQ_2500KHZ,
	MOD_FREQ_1250KHZ,
	MOD_FREQ_625KHZ,
	NumberOfTypes
} mod_freq_t;


double gKoef[4][4]; //for BG correction
int configCorrMode;	// 0- 4AB, 1 - AB line, 2 - AB every pixel


int configGetModFreqCount();
int configInit(int deviceAddress);
int configLoad(const op_mode_t mode, const int deviceAddress);
op_mode_t configGetCurrentMode();
int configSetIntegrationTime(const int us, const int deviceAddress, const int valueAddress, const int loopAddress);
int configSetIntegrationTime2D(const int us, const int deviceAddress);
int configGetIntegrationTime2D();
int configGetIntegrationTime3D();
int configGetIntegrationTime3DHDR();
int configSetIntegrationTime3D(const int us, const int deviceAddress);
int configSetIntegrationTime3DHDR(const int us, const int deviceAddress);
void configSetIntegrationTimesHDR();
int configSetModulationFrequency(const mod_freq_t freq, const int deviceAddress);
void configFindModulationFrequencies();
void configEnableDualMGX(const int state, const int deviceAddress);
void configSetDCS(unsigned char nDCS, const int deviceAddress);
int16_t configGetModulationFrequency(const int deviceAddress);
int16_t* configGetModulationFrequencies();
mod_freq_t configGetModFreqMode();
void configEnableABS(const unsigned int state, const int deviceAddress);
int configIsABS(const int deviceAddress);
void configSetMode(const int select, const int deviceAddress);
void configSetDeviceAddress(const int deviceAddress);
int configGetDeviceAddress();
int configIsFLIM();
int configGetSysClk();
double configGetTModClk();
double configGetTPLLClk();
void configSetIcVersion(unsigned int value);
unsigned int configGetIcVersion();
void configSetPartType(unsigned int value);
void configSetPartVersion(unsigned int value);
unsigned int configGetPartVersion();
unsigned int configGetPartType();
void configSetWaferID(const uint16_t ID);
uint16_t configGetWaferID();
void configSetChipID(const uint16_t ID);
uint16_t configGetChipID();

unsigned char configGetGX();
unsigned char configGetGY();
unsigned char configGetGM();
unsigned char configGetGL();
unsigned char configGetGC();
int			  configGetGZ();

int configLoadCorrectionTable(int mode);
void configSetCorrectionBGMode(int mode);
int  configGetCorrectionBGMode();
int  configLoadImageFile();
int  configGetImage(uint16_t **data);
void configSaveDCSImage(uint16_t **data);
void configSetPhasePoint(int index);
void configSetPhasePointActivate(int mode);
void configDCSCorrection(int mode);
int configIsEnabledGrayscaleCorrection();
int configIsEnabledDRNUCorrection();



#endif
