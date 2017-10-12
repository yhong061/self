#define _GNU_SOURCE
#include "config.h"
#include "i2c.h"
#include "evalkit_illb.h"
#include "dll.h"
#include "read_out.h"
#include "registers.h"
#include "seq_prog_loader.h"
#include "api.h"
#include "calibration.h"
#include "temperature.h"
#include "pru.h"
#include "flim.h"
#include "image_correction.h"
#include "calculation.h"
#include "calculator.h"
#include "saturation.h"
#include "adc_overflow.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "helper.h"

static int integrationTime2D = 5000;
static int integrationTime3D = 700;
static int integrationTime3DHDR = 700;
static unsigned char nDCSTOF = 4;
static int correctionBGMode  = 0;
static op_mode_t currentMode = GRAYSCALE;
static mod_freq_t currentFreq;
static int modulationFrequency;
static int16_t modulationFrequencies[6];
static unsigned int devAddress = 0;
static unsigned int absEnabled = 1;
static unsigned int isFLIM = 0;
static unsigned int icType = 0;
static unsigned int partType = 0;
static unsigned int partVersion = 0;
static uint16_t waferID = 0;
static uint16_t chipID  = 0;

static int enabledDRNU = 0;
static int enabledGrayscale = 0;

static unsigned char gX;
static unsigned char gY;
static unsigned char gM;
static unsigned char gC;
static unsigned char gL;
static int			 gZ;

static uint16_t gDCS[4][72][168];
static uint16_t pDCS[4*72*168];

/*!
 Initializes the configuration.
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configInit(int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		sequencerProgramLoad("epc635_Seq_Prog-V9.bin", deviceAddress);
	} else {
		sequencerProgramLoad("epc660_Seq_Prog-V8.bin", deviceAddress);
	}

	i2cData[0] = 0x04;
	i2c(deviceAddress, 'w', DLL_MEASUREMENT_RATE_LO, 1, &i2cData); // Set DLL measurement rate to 4.
	i2cData[0] = 0xCC;
	i2c(deviceAddress, 'w', LED_DRIVER, 1, &i2cData); // Enable LED preheat.

	// new implementation for temperature sensor readout
	i2c(deviceAddress, 'r', 0xD3, 1, &i2cData);
	gX = 16;//15;//i2cData[0];

	i2c(deviceAddress, 'r', 0xD5, 1, &i2cData);
	gY = 59;//56;//i2cData[0];

	i2c(deviceAddress, 'r', 0x92, 1, &i2cData);
	gM = 52;//i2cData[0];

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x3A, 1, &i2cData);	//grayscale differential readout

	i2c(deviceAddress, 'r', 0xE8, 1, &i2cData);
	gC = 114;//106;//i2cData[0];

	gZ = -272; //0x110 default value

	if(gC != 0xFF){
		gZ = ((double)gC / 4.68) - 299; //0x12B;
	}
	printf("gX = %d, gY = %d, gM = %d, gC = %d, gZ = %d\n", gX, gY, gM, gC, gZ);

	i2cData[0] = 0x03;
	i2c(deviceAddress, 'w', SEQ_CONTROL, 1, &i2cData); // Enable pixel and TCMI sequencers.

	mod_freq_t freq = MOD_FREQ_10000KHZ;
	configFindModulationFrequencies();
	configSetModulationFrequency(freq, deviceAddress);

	// Check if FLIM chip:
	if (configGetPartType() == EPC502_OBS || configGetPartType() == EPC502 || configGetPartType() == EPC503) {
		isFLIM = 1;
	}
	//dllDetermineAndSetDemodulationDelay(deviceAddress);
	//dllBypassDLL(1, deviceAddress); // bypass DLL
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		readOutSetROI(4, 163, 6, 65, deviceAddress);
	} else {
		readOutSetROI(4, 323, 6, 125, deviceAddress);
	}
	calibrationLoad(20, deviceAddress);
	//temperatureInit(deviceAddress); //
	op_mode_t mode;
	pruInit(deviceAddress);
	if (isFLIM) {
#if 0		
		printf("FLIM enabled\n");
		mode = FLIM;
		// FLIM needs 20Mhz
		mod_freq_t freq = MOD_FREQ_20000KHZ;
		configSetModulationFrequency(freq, deviceAddress);
		// set delay to 0, not sure what's better
		// dllResetDemodulationDelay(deviceAddress);
		flimSetT1(2);
		flimSetT2(1);
		flimSetT3(1);
		flimSetT4(4);
		flimSetTREP(6);
		flimSetRepetitions(1000);
		flimSetFlashDelay(1);
		flimSetFlashWidth(1);
#endif		
	} else { // TOF
		mode = THREED;
	}
	//imageCorrectionLoad("image_correction.txt");
	//imageCorrectionTransform();
	configLoad(mode, deviceAddress);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		calculatorSetPixelMask(0xFFFF);
		saturationSetSaturationBit(0x0001);
		adcOverflowSetMin(16);
		adcOverflowSetMax(65519-16);
		i2cData[0] = 0x63;
		i2c(deviceAddress, 'w', I2C_TCMI_CONTROL, 1, &i2cData); // Enable tcmi_8bit_data_sat_en.
		calibrationSetNumberOfColumnsMax(168);
		calibrationSetNumberOfRowsMax(72);
	}

	/* dcs0/1 2/3 and ABCD correction*/
	/*gKoef[0][0] =  2.4858;
	gKoef[0][1] = -1.3223e-02;
	gKoef[0][2] =  1.0371e-05;
	gKoef[0][3] = -2.0599e-09;
	gKoef[1][0] =  2.7585e+01;
	gKoef[1][1] =  1.1026e-01;
	gKoef[1][2] = -6.6614e-05;
	gKoef[1][3] =  1.6573e-08;
	gKoef[2][0] = -1.2017;
	gKoef[2][1] = -3.4702e-02;
	gKoef[2][2] =  1.6169e-05;
	gKoef[2][3] = -4.2810e-09;
	gKoef[3][0] =  2.5055e+01;
	gKoef[3][1] =  8.6958e-02;
	gKoef[3][2] = -6.0328e-05;
	gKoef[3][3] =  1.4310e-08;*/

	free(i2cData);

	enabledGrayscale = 1;
	if(calculateLoadGrayscaleGainOffset() < 0 ){
		enabledGrayscale = 0;
	}

	enabledDRNU = 1;
	if(calibrationLoadDRNU_LUT() < 0){
		enabledDRNU = 0;
	}

	return 0;
}

/*!
 Loads the configuration corresponding to the specified mode.
 @param mode
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configLoad(const op_mode_t mode, const int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	switch (mode) {
	case THREED:
		configSetDCS(nDCSTOF, deviceAddress);
		if (absEnabled) {
			if (calculationIsHDR()) {
				i2cData[0] = 0x30;
			} else {
				i2cData[0] = 0x34;
			}
		} else {
			if (calculationIsHDR()) {
				i2cData[0] = 0x00;
			} else {
				i2cData[0] = 0x04;
			}
		}
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData); // differential readout
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData); // LED modulated, modulation gates not forced
		currentMode = THREED;
		if (calculationIsHDR()) {
			configSetIntegrationTimesHDR();
		} else {
			configSetIntegrationTime3D(integrationTime3D, deviceAddress);
		}
		break;

	case GRAYSCALE:
	default:
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cData); // disable shutter
		i2cData[0] = 0x04;
		i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData); // only 1 DCS frame
		i2cData[0] = 0x00;  //0x14; - old
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData); // differential readout MGA
		i2cData[0] = 0x26;
		i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData); // suppress the LED, force MGA high and MGB low during integration
		currentMode = GRAYSCALE;
		configSetIntegrationTime2D(integrationTime2D, deviceAddress);
		break;
	case FLIM:
		i2cData[0] = 0x94;
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData); // FLIM, 2 DCSs
		currentMode = FLIM;
		break;
	}

	free(i2cData);
	return 0;
}

/*!
 Getter function for the mode
 @returns mode
 */
op_mode_t configGetCurrentMode() {
	return currentMode;
}

/*!
 Sets the integration time for the grayscale mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime2D(const int us, const int deviceAddress) {
	if (us < 0 || us > MAX_INTEGRATION_TIME_2D) {
		return -1;
	}
	if (currentMode == GRAYSCALE) {
		if (configSetIntegrationTime(us, deviceAddress, 0xA2, 0xA0) == 0) {
			integrationTime2D = us;
			return 0;
		}
	} else {
		integrationTime2D = us;
		return 0;
	}
	return -1;
}

/*!
 Sets the integration time for the TOF mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime3D(const int us, const int deviceAddress) {
	if (us < 0 || us > MAX_INTEGRATION_TIME_3D) {
		return -1;
	}

	if (calculationIsHDR()) {
		integrationTime3D = us;
		configSetIntegrationTimesHDR();
	} else if (currentMode == THREED) {
		if (configSetIntegrationTime(us, deviceAddress, 0xA2, 0xA0) == 0) {
			integrationTime3D = us;
		}
	} else {
		integrationTime3D = us;
	}
	return 0;
}

/*!
 Sets the integration time for the HDR mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime3DHDR(const int us, const int deviceAddress) {
	if (us < 0 || us > MAX_INTEGRATION_TIME_3D) {
		return -1;
	}

	integrationTime3DHDR = us;
	configSetIntegrationTimesHDR();
	return 0;
}

/*!
 Returns the (1st) TOF integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime3D() {
	return integrationTime3D;
}

/*!
 Returns the (2nd) HDR TOF integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime3DHDR() {
	return integrationTime3DHDR;
}

/*!
 Returns the Grayscale integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime2D() {
	return integrationTime2D;

}

void configSetCorrectionBGMode(int mode){
	correctionBGMode = mode;
}

int configGetCorrectionBGMode(){
	return correctionBGMode;
}


/*!
 Sets the integration time for the given mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @param valueAddress register for the value
 @param loopAddress register for the loop
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime(const int us, const int deviceAddress,
		const int valueAddress, const int loopAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	uint8_t intmHi;
	uint8_t intmLo;
	uint8_t intLenHi;
	uint8_t intLenLo;

	double tModClk = configGetTModClk();
	double max = 0x10000 * tModClk / 1000.0;

	if (us <= max) {
		intmHi = 0x00;
		intmLo = 0x01;

		if (us <= 0) {
			intLenHi = 0x00;
			intLenLo = 0x00;
		} else {
			int intLen = us * 1000 / tModClk - 1;
			intLenHi = (intLen >> 8) & 0xFF;
			intLenLo = intLen & 0xFF;
		}
	} else {
		int intm = us / max + 1;
		intmHi = (intm >> 8) & 0xFF;
		intmLo = intm & 0xFF;
		//TODO: -0.5 because of round?
		int intLen = (double) us / intm * 1000.0 / tModClk - 1;
		intLenHi = (intLen >> 8) & 0xFF;
		intLenLo = intLen & 0xFF;
	}

	i2cData[0] = (unsigned char) intmHi;
	i2cData[1] = (unsigned char) intmLo;
	i2c(deviceAddress, 'w', loopAddress, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi;
	i2cData[1] = (unsigned char) intLenLo;
	i2c(deviceAddress, 'w', valueAddress, 2, &i2cData);

	free(i2cData);

	return 0;
}

void configSetIntegrationTimesHDR() {
	//TODO: more precise
	unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
	unsigned int deviceAddress = configGetDeviceAddress();
	double tModClk = configGetTModClk();
	double max = 0x10000 * tModClk / 1000.0;

	int intm0 = integrationTime3D / max + 1;
	int intm1 = integrationTime3DHDR / max + 1;
	int intm = intm0;
	uint8_t intmHi;
	uint8_t intmLo;
	int intLen0;
	int intLen1;
	uint8_t intLenHi0;
	uint8_t intLenLo0;
	uint8_t intLenHi1;
	uint8_t intLenLo1;

	if (intm1 > intm) {
		intm = intm1;
	}
	intmHi = (intm >> 8) & 0xFF;
	intmLo = intm & 0xFF;
	if (integrationTime3D <= 0) {
		intLenHi0 = 0x00;
		intLenLo0 = 0x00;
	} else {
		intLen0 = (double) integrationTime3D / intm * 1000.0 / tModClk - 1;
		intLenHi0 = (intLen0 >> 8) & 0xFF;
		intLenLo0 = intLen0 & 0xFF;
	}

	if (integrationTime3DHDR <= 0) {
		intLenHi1 = 0x00;
		intLenLo1 = 0x00;
	} else {
		intLen1 = (double) integrationTime3DHDR / intm * 1000.0 / tModClk - 1;
		intLenHi1 = (intLen1 >> 8) & 0xFF;
		intLenLo1 = intLen1 & 0xFF;
	}

	i2cData[0] = (unsigned char) intmHi;
	i2cData[1] = (unsigned char) intmLo;
	i2c(deviceAddress, 'w', INTM_HI, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi0;
	i2cData[1] = (unsigned char) intLenLo0;
	i2c(deviceAddress, 'w', INT_LEN_HI, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi1;
	i2cData[1] = (unsigned char) intLenLo1;
	i2c(deviceAddress, 'w', INT_LEN_MGX_HI, 2, &i2cData);

	free(i2cData);
}

/*!
 Sets the integration time for the given mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @param valueAddress register for the value
 @param loopAddress register for the loop
 @returns On success, 0, on error -1
 */
int configSetModulationFrequency(const mod_freq_t freq, const int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	switch (freq) {
	case MOD_FREQ_20000KHZ:
		i2cData[0] = 0x00;
		modulationFrequency = modulationFrequencies[0];
		break;
	case MOD_FREQ_5000KHZ:
		i2cData[0] = 0x03;
		modulationFrequency = modulationFrequencies[2];
		break;
	case MOD_FREQ_2500KHZ:
		i2cData[0] = 0x07;
		modulationFrequency = modulationFrequencies[3];
		break;
	case MOD_FREQ_1250KHZ:
		i2cData[0] = 0x0F;
		modulationFrequency = modulationFrequencies[4];
		break;
	case MOD_FREQ_625KHZ:
		i2cData[0] = 0x1F;
		modulationFrequency = modulationFrequencies[5];
		break;
	case MOD_FREQ_10000KHZ:
	default:
		i2cData[0] = 0x01;
		modulationFrequency = modulationFrequencies[1];
		break;
	}
	currentFreq = freq;
	i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);
	free(i2cData);
	configSetIntegrationTime2D(integrationTime2D, deviceAddress);
	configSetIntegrationTime3D(integrationTime3D, deviceAddress);
	return 0;
}

/*!
 Getter function for the modulation frequency
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns the modulation frequency in kHz
 */
int16_t configGetModulationFrequency(const int deviceAddress) {
	printf("---- modulation frequency = %i ----\n", modulationFrequency);
	return modulationFrequency;
}

mod_freq_t configGetModFreqMode() {
	return currentFreq;
}

//in khz
void configFindModulationFrequencies() {
	int sysClk = configGetSysClk();
	printf("sysClk = %i\n", sysClk);
	int i;
	for (i = 0; i < 6; i++) {
		int modFrequency = sysClk / (4 * (pow(2, i)));
		modulationFrequencies[i] = modFrequency;
		printf("%i frequency -> %i\n", i, modFrequency);
	}
}

int16_t* configGetModulationFrequencies() {
	return modulationFrequencies;
}

/*!
 Enables or disables ABS
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void configEnableABS(const unsigned int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	if (state != 0) {
		absEnabled = 1;
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);
	} else {
		absEnabled = 0;
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);
	}
	free(i2cData);
}

int configIsABS(const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
	int enabled = 1;//(i2cData[0] & 0x30) == 0x30;
	free(i2cData);
	return enabled;
}

/*!
 Sets the number of DCSs
 @param nDCS number of DCSs
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void configSetDCS(unsigned char nDCS, const int deviceAddress) {
	nDCSTOF = nDCS;
	nDCS = (nDCS - 1) << 4;
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	i2cData[0] |= nDCS; //set 1
	i2cData[0] &= (nDCS | 0xCF); //set 0
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	free(i2cData);
}

void configSetMode(const int select, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	if (select) {
		i2cData[0] |= 0x40;
	} else {
		i2cData[0] &= 0xBF;
	}
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	free(i2cData);
}

int configGetModFreqCount() {
	return NumberOfTypes;
}

int configGetModeCount() {
	return NumberOfModes;
}

void configSetDeviceAddress(const int deviceAddress) {
	devAddress = deviceAddress;
}

int configGetDeviceAddress() {
	return devAddress;
}

int configIsFLIM() {
	return isFLIM;
}

int configGetSysClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	int clockKhz = 4000; //clock in kHz
	i2c(devAddress, 'r', PLL_FB_DIVIDER, 1, &i2cData);
	int pllFbDivider = 23;//(i2cData[0]) & 0x1F;
	i2c(devAddress, 'r', PLL_PRE_POST_DIVIDERS, 1, &i2cData);
	int pllPostDivider = 0;//(i2cData[0] >> 4) & 0x03;
	printf("pllFbDivider = %d, pllPostDivider = %d\n", pllFbDivider, pllPostDivider);
	free(i2cData);
	return (clockKhz * (pllFbDivider + 1)) >> pllPostDivider;
}

double configGetTModClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(devAddress, 'r', MOD_CLK_DIVIDER, 1, &i2cData);
	int modClkDivider = 1;//i2cData[0] & 0x1F;
	printf("modClkDivider = %d\n",modClkDivider);
	free(i2cData);
	return 1000000.0 / (configGetSysClk() / (modClkDivider + 1.0));
}

double configGetTPLLClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	int clockKhz = 4000; //clock in kHz
	i2c(devAddress, 'r', PLL_FB_DIVIDER, 1, &i2cData);
	int pllFbDivider = 23;//(i2cData[0]) & 0x1F;
	i2c(devAddress, 'r', PLL_PRE_POST_DIVIDERS, 1, &i2cData);
	int pllPostDivider = 0;//(i2cData[0] >> 4) & 0x03;
	int sysClk = (clockKhz * (pllFbDivider + 1)) >> pllPostDivider;
	free(i2cData);
	return 1000000.0 / sysClk;
}

void configSetIcVersion(unsigned int value) {
	icType = value;
}

unsigned int configGetIcVersion() {
	return icType;
}

void configSetPartType(unsigned int value) {
	partType = value;
}

void configSetPartVersion(unsigned int value) {
	partVersion = value;
}

unsigned int configGetPartVersion() {
	return partVersion;
}

unsigned int configGetPartType() {
	return partType;
}

void configSetWaferID(const uint16_t ID) {
	waferID = ID;
}

uint16_t configGetWaferID() {
	return waferID;
}

void configSetChipID(const uint16_t ID) {
	chipID = ID;
}

uint16_t configGetChipID() {
	return chipID;
}

unsigned char configGetGX(){
	return gX;
}

unsigned char configGetGY(){
	return gY;
}

int configGetGZ(){
	return gZ;
}

unsigned char configGetGM(){
	return gM;
}

unsigned char configGetGL(){
	return gL;
}

unsigned char configGetGC(){
	return gC;
}

int configLoadCorrectionTable(int mode){
	char    *line = NULL;
	char    *filename;
	size_t   len = 0;
	ssize_t  read;
	FILE    *file;

	if ( mode == CORR_BG_AVR_MODE ){

		file = fopen("./correctionTable.txt", "r");
		if (file == NULL){
			printf("Could not open correctionTable.txt file.\n");
			return -1;
		}

		int i=0;
		while((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			int j = 0;
			while (pch != NULL){
				if (j > 4){
					printf("File does not contain a proper ccorrectionTable coefficients\n");
					return -1;
				}
				double val = helperStringToDouble(pch);
				gKoef[i][j]= val;
				pch = strtok(NULL," ");
				j++;
			}
			i++;
		}

	} else {

		if( mode == CORR_BG_LINE_MODE )	filename = "./correctionTableLine.txt";
		else							filename = "./correctionTablePixel.txt";


		file = fopen(filename, "r");

		if (file == NULL){
			printf("Could not open %s file.\n", filename);
			return -1;
		}

		for(int l=0; l<2; l++){

			if(l==0) printf("Koef gA....\n");
			else 	 printf("Koef gB....\n");

			for(int k=0; k<4; k++){
				for(int y=0; y<60; y++){

					if((read = getline(&line, &len, file)) != -1){
						char *pch = strtok(line," ");
						for (int x=0; (pch != NULL) && (x<160); x++){
							if (x > 160){
								printf("File does not contain a proper ccorrectionTable coefficients\n");
								return -1;
							}
							double val = helperStringToDouble(pch);

							if( l == 0 ){

								gA[k][y][x]= val;

								if( x < 10 ){
									printf("%.4e\t",gA[k][y][x]);
								}

							} else {

								gB[k][y][x]= val;

								if( x < 10 ){
									printf("%.4e\t",gB[k][y][x]);
								}
							}

							pch = strtok(NULL," ");
						}	// end while
					} else {
						return 0; //error
					}

					printf("\n");
				}	//for y
			}	//for k
		}	//for l

	}	// end else

	fclose(file);
	return 0;

}

int configLoadImageFile(){
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int numCols = 160;
	char* str[4];

	str[0] = "./dcs0_withoutBG.txt";
	str[1] = "./dcs1_withoutBG.txt";
	str[2] = "./dcs2_withoutBG.txt";
	str[3] = "./dcs3_withoutBG.txt";

	/*str[0] = "./dcs0.txt";
	str[1] = "./dcs1.txt";
	str[2] = "./dcs2.txt";
	str[3] = "./dcs3.txt";*/

	int N = 4 * 72 * 168;
	for(int k=0; k<N; k++){
		pDCS[k] = 0;
	}

	FILE *file;

	for(int k = 0; k < 4; k++){

		printf("Open File: %s\n", str[k]);

		file = fopen(str[k], "r");
		if (file == NULL){
			printf("Could not open %s file.\n", str[k]);
			return -1;
		}

		int i=0;
		while((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			int j = 0;
			while (pch != NULL){
				if (j > numCols){
					printf("File does not contain a proper ccorrectionTable coefficients\n");
					return -1;
				}
				uint16_t val = helperStringToInteger(pch);
				gDCS[k][i][j]= val;
				pch = strtok(NULL," ");
				j++;
			}
			i++;
			printf("rows= %d cols = %d\n", i, j);
		}
		fclose(file);
	}

	int l=0;
	for(int k=0; k<4; k++){
		for(int i=0; i<60; i++){
			for(int j=0; j<160; j++, l++){
				pDCS[l] = (gDCS[k][i][j]<<4) & 0xFFF0;
			}
		}
	}

	return 0;
}

int configGetImage(uint16_t **data){

	*data = pDCS;
	return 4*60*160;

}

void configSaveDCSImage(uint16_t **data){
	uint16_t *pMem = *data;

	char* str[4];
	str[0] = "./dcs0.txt";
	str[1] = "./dcs1.txt";
	str[2] = "./dcs2.txt";
	str[3] = "./dcs3.txt";

	FILE *file;

	int l=0;
	for(int k = 0; k < 4; k++){
		file = fopen(str[k], "w");
		for(int  i = 0;  i < 60; i++){
			for(int j = 0; j < 160;  j++, l++){
				fprintf( file, "%d ", pMem[l]);
			}
			fprintf(file, "\n");
		}
		fclose(file);
	}

}

void configSetPhasePointActivate(int mode){
	unsigned char *i2cData  = malloc(1 * sizeof(unsigned char));
	if(mode == 0){ //stop
		i2cData[0] = 1;
	} else { //start
		i2cData[0] = 4;
	}
	i2c(devAddress, 'w', DLL_CONTROL, 1, &i2cData);
	free(i2cData);
}


void configSetPhasePoint(int index){
	unsigned char *i2cData  = malloc(1 * sizeof(unsigned char));
	i2cData[0] = (unsigned char)index;
	i2c(devAddress, 'w', DLL_COARSE_CTRL_EXT, 1, &i2cData); //0x72, 0x73
	free(i2cData);
}

void configDCSCorrection(int mode){

	if( mode < 2 ){
		configCorrMode = CORR_BG_AVR_MODE;
	configLoadCorrectionTable(CORR_BG_AVR_MODE);

		if ( mode == 0 ){
			for(int i=0; i<4; i++){
				gKoef[2][i] = 0.0;
				gKoef[3][i] = 0.0;
			}
		}

		for(int i=0; i<4; i++){
			for(int j=0; j<4; j++){
				printf("%.4e\t", gKoef[i][j]);
			}
			printf("\n");
		}
	} else if ( mode == CORR_BG_LINE_MODE ){
		configCorrMode = CORR_BG_LINE_MODE;
		configLoadCorrectionTable(CORR_BG_LINE_MODE);
	} else {
		configCorrMode = CORR_BG_PIXEL_MODE;
		configLoadCorrectionTable(CORR_BG_PIXEL_MODE);
	}

}

int configIsEnabledDRNUCorrection(){
	return enabledDRNU;
}

int configIsEnabledGrayscaleCorrection(){
	return enabledGrayscale;
}

