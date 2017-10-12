#include "api.h"
#include "evalkit_constants.h"
#include "config.h"
#include "pru.h"
#include "dll.h"
#include "evalkit_illb.h"
#include "i2c.h"
#include "calculation.h"
#include "read_out.h"
#include "calibration.h"
#include "chip_info.h"
#include "hysteresis.h"
#include "temperature.h"
#include "pn.h"
#include "image_averaging.h"
#include "image_correction.h"
#include "image_processing.h"
#include "image_difference.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "flim.h"
#include "logger.h"
#include "version.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static unsigned int nDCSTOF = 4;
static unsigned int rowReductionBeforeBinning = 1;
unsigned char* pData;

/*!
 Sets the ROI (Region of Interest) and updates the pointers to the DDR memory
 @param topLeftX x coordinate of the top left point (0 - 322), always less than bottomRightX, must be even
 @param bottomRightX x coordinate of the bottom right point (5 - 327), always greater than topLeftX, must be odd
 @param topLeftY y coordinate of the top left point (0 - 124), always less than bottomRightY, must be even
 @param bottomRightY y coordinate of the bottom right point (1-125), always greater than topLeftY, must be odd
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set the full ROI -> setROI 0 327 0 125
 .
 default (epc660/epc502):
 - setROI 4 323 6 125
 @warning ROI can be set to min 6 by 2
 */
int16_t apiSetROI(const int topLeftX, const int bottomRightX, const int topLeftY, const int bottomRightY, const int deviceAddress) {
	if (topLeftX < 0 || topLeftX > bottomRightX || topLeftX % 2 == 1 || bottomRightX < 5 || bottomRightX >= MAX_NUMBER_OF_COLS || bottomRightX % 2 == 0) {
		printf("bad x coordinates\n");
		return -1;
	}
	if (topLeftY < 0 || topLeftY > bottomRightY || topLeftY % 2 == 1 || bottomRightY < 1 || bottomRightY >= MAX_NUMBER_OF_DOUBLE_ROWS || bottomRightY % 2 == 0) {
		printf("bad y coordinates\n");
		return -1;
	}
	int width = bottomRightX - topLeftX + 1;
	int height = bottomRightY - topLeftY + 1;
	int minWidth = 6 * pruGetColReduction();
	int minHeight = 2 * pruGetRowReduction();
	printf("width = %i, height = %i\n", width, height);
	if (width < minWidth || height < minHeight) {
		printf("size too small: min size = %i x %i\n", minWidth, minHeight);
		return -1;
	}
	readOutSetROI(topLeftX, bottomRightX, topLeftY, bottomRightY, deviceAddress);
	pruSetSize(bottomRightX - topLeftX + 1, bottomRightY - topLeftY + 1);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return 0;
}

/*!
 Loads a config for the mode with the given index into registers, enables or disables the illumination and updates pointers to the DDR memory
 @param configIndex 0 for GRAYSCALE, 1 for THREED, everything else results in loading the GRAYSCALE config
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns index of loaded config
 @note
 usage with server:
 - loading GRAYCALE config -> loadConfig 0
 .
 default:
 - loadConfig 1 (THREED)
 */
int16_t apiLoadConfig(const int configIndex, const int deviceAddress) {
	op_mode_t mode;
	switch (configIndex) {
	case 1:
		mode = THREED;
		pruSetDCS(nDCSTOF);
		break;
	case 0:
	default:
		mode = GRAYSCALE;
		pruSetDCS(1);
		break;
	}
	configLoad(mode, deviceAddress);
	return configIndex;
}

/*!
 Reads Bytes from the registers over I2C
 @param registerAddress address of the register it starts to read from
 @param nBytes number of Bytes to read
 @param values pointer to the array where values will be stored to
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - reading ROI (6Bytes) -> readRegister 96 6   or   read 96 6   or   r 96 6
 - reading 1 Byte from Register AE -> read AE 1   or   read AE
 */
int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	if (i2c(deviceAddress, 'r', registerAddress, nBytes, &values) > 0) {
		return nBytes;
	}
	return -1;
}

/*!
 Writes Bytes into the registers over I2C
 @param registerAddress address of the register it starts to write from
 @param nBytes number of Bytes to write
 @param values pointer to the array where values are stored in
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - writing default ROI (6Bytes) -> writeRegister 96 00 04 01 43 06 70    or   write 96 ...   or   w 96 6 ...
 - writing 1 Byte to Register AE -> write AE 1   or   write AE
 */
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	i2c(deviceAddress, 'w', registerAddress, nBytes, &values);
	return nBytes;
}

/*!
 Enables or disables the horizontal binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of horizontal binning
 @note
 usage with server:
 - enable horizontal binning -> enableHorizontalBinning 1
 - disable horizontal binning -> enableHorizontalBinning 0
 .
 default:
 - enableHorizontalBinning 0
 */
int16_t apiEnableHorizontalBinning(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	readOutEnableHorizontalBinning(enable, deviceAddress);
	pruSetColReduction(enable);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables the vertical binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of vertical binning
 @note
 usage with server:
 - enable vertical binning -> enableVerticalBinning 1
 - disable vertical binning -> enableVerticalBinning 0
 .
 default:
 - enableVerticalBinning 0
 */
int16_t apiEnableVerticalBinning(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	if (enable) {
		rowReductionBeforeBinning = log2(pruGetRowReduction());
		// check if rowReduction is > 2 else set it to 2:
		if (rowReductionBeforeBinning < 2) {
			apiSetRowReduction(1, deviceAddress);
		}
	} else {
		apiSetRowReduction(0, deviceAddress);
	}
	readOutEnableVerticalBinning(enable, deviceAddress);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return enable;
}

/*!
 Sets the reduction of the rows
 @param value reduction factor = 2^value. Allowed values 0 - 3, other values will be changed to 0 or 3.
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the value of the row reduction
 @note
 usage with server:
 - show every row -> setRowReduction 0
 - only show every 4th row -> setRowReduction 2
 .
 default:
 - setRowReduction 0
 */
int16_t apiSetRowReduction(int value, const int deviceAddress) {
	if (value < 0) {
		value = 0;
	} else if (value > 3) {
		value = 3;
	}
	readOutSetRowReduction(value, deviceAddress);
	pruSetRowReduction(value);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return value;
}

/*!
 Enables or disables ABS
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of the ABS
 @note
 usage with server:
 - disable ABS -> enableABS 0
 .
 default:
 - enableABS 1
 */
int16_t apiEnableABS(const int state, const int deviceAddress) {
	configEnableABS(state, deviceAddress);
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return state;
}

/*!
 Sets the value of the integration time for the GRAYSCALE mode.
 @param us integration time in microseconds
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for GRAYSCALE mode to 280us -> setIntegrationTime2D 280
 .
 default
 - setIntegrationTime2D 5000
 */
int16_t apiSetIntegrationTime2D(const int us, const int deviceAddress) {
	return configSetIntegrationTime2D(us, deviceAddress);
}

/*!
 Sets the value of the integration time for the THREED mode.
 @param us integration time in microseconds
 @param deviceAddress address of the I2C device
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for THREED mode to 120us -> setIntegrationTime3D 120
 .
 default
 - setIntegrationTime3D 700
 */
int16_t apiSetIntegrationTime3D(const int us, const int deviceAddress) {
	return configSetIntegrationTime3D(us, deviceAddress);
}

/*!
 Sets the value of the integration time for the HDR mode.
 @param us integration time in microseconds
 @param deviceAddress address of the I2C device
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for HDR mode to 120us -> setIntegrationTime3DHDR 120
 .
 default
 - setIntegrationTime3DHDR 700
 */
int16_t apiSetIntegrationTime3DHDR(const int us, const int deviceAddress) {
	return configSetIntegrationTime3DHDR(us, deviceAddress);
}

/*!
 Sets the minimum amplitude for the calculation of the Distance (distance will be set to >30000 if min amplitude is higher than measured value)
 @param minAmplitude minimum value of the amplitude
 @returns value of the minimum Amplitude
 @note
 usage with server:
 - set min amplitude to 150 -> setMinAmplitude 150
 - show all distance values -> setMinAmplitude 0
 .
 default:
 - setMinAmplitude 0
 */
int16_t apiSetMinAmplitude(const int minAmplitude) {
	pruSetMinAmplitude(minAmplitude);
	hysteresisUpdateThresholds(minAmplitude);
	return minAmplitude;
}

/*!
 Gets the minimum amplitude.
 @returns minAmplitude minimum value of the amplitude
 @note
 usage with server:
 - getMinAmplitude
 */
int16_t apiGetMinAmplitude() {
	return pruGetMinAmplitude();
}

/*!
 Sets the modulation frequency.
 @param index modulation frequency index
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set the modulation frequency to 625kHz -> setModulationFrequency 5
 .....
 - set the modulation frequency to 10MHz -> setModulationFrequency 1
 - set the modulation frequency to 20MHz -> setModulationFrequency 0
 .
 default:
 - setModulationFrequency 1
 */
int16_t apiSetModulationFrequency(const int index, const int deviceAddress) {
	if (index >= 0 && index < configGetModFreqCount()) {
		mod_freq_t freq = (mod_freq_t) index;
		configSetModulationFrequency(freq, deviceAddress);
		calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
		imageDifferenceInit();
		return 0;
	}
	return -1;
}

/*!
 The modulation frequencies depend on the clks. Getter for the modulation frequencies.
 @returns the 6 frequencies in khz
 @note
 usage with server:
 - getModulationFrequencies
 */
int16_t* apiGetModulationFrequencies(){
	return configGetModulationFrequencies();
}

/*!
 Enables or disables Pi-Delay matching.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of Pi delay
 @note
 usage with server:
 - disable Pi delay matching -> enablePiDelay 0
 .
 default:
 - enablePiDelay 1
 */
int16_t apiEnablePiDelay(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
		if (calculationIsDualMGX()) {
			nDCSTOF = 1;
		} else {
			nDCSTOF = 2;
		}
	} else {
		if (calculationIsDualMGX()) {
			nDCSTOF = 2;
		} else {
			nDCSTOF = 4;
		}
	}
	configSetDCS(nDCSTOF, deviceAddress);
	pruSetDCS(nDCSTOF);
	calculationEnablePiDelay(enable);
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables Dual MGX Motion blur.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of dual MGX
 @note
 usage with server:
 - enable dual MGX -> enableDualMGX 1
 .
 default:
 - enableDualMGX 0
 */
int16_t apiEnableDualMGX(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
		if (calculationIsPiDelay()) {	// no dual mgx, pi delay
			nDCSTOF = 4;
		} else {	// no dual mgx, no pi delay
			nDCSTOF = 2;
		}
	} else {
		if (calculationIsPiDelay()) { // dual mgx, pi delay
			nDCSTOF = 2;
		} else {
			nDCSTOF = 1; // dual mgx, no pi delay
		}
	}
	readOutEnableDualMGX(enable, deviceAddress);
	configSetIntegrationTime3D(configGetIntegrationTime3D(), deviceAddress);
	calculationEnableDualMGX(enable);
	configSetDCS(nDCSTOF, deviceAddress);
	pruSetDCS(nDCSTOF);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables Dual MGX HDR.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of dual HDR
 @note
 usage with server:
 - enable HDR -> enableHDR 1
 .
 default:
 - enableDualMGX 0
 */
int16_t apiEnableHDR(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	readOutEnableHDR(enable, deviceAddress);
	calculationEnableHDR(enable);
	imageCorrectionTransform();
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	configSetIntegrationTimesHDR();
	imageDifferenceInit();
	return enable;
}

/*!
 Creates an array with correction values for the whole pixel field.
 @param distance reference distance in cm
 @param FOV horizontal field of view in deg
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns reference distance
 @note
 usage with server:
 - do calibration 1.20m away from a white wall with a horizontal field of view of 92 deg -> calibratePixel 120 92
 */
int16_t apiCalibratePixel(const int distance, const int FOV, const int deviceAddress) {
	calibrationPixel(distance, FOV, deviceAddress, 0);
	return distance;
}

/*!
 Enables or disables calibration.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state
 @note
 usage with server:
 - disable calibration -> enableCalibration 0
 .
 default:
 - enableCalibration 1
 */
int16_t apiEnableCalibration(const int state, const unsigned int deviceAddress) {
	calibrationSetEnable(state);
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	return state;
}

int16_t apiEnableCorrectionBG(const int state, const unsigned int deviceAddress) {
	configSetCorrectionBGMode(state);
	return state;
}


double apiSetCorrectionBG(int row, int col, double value){
	gKoef[row][col] = value;
	printf("row=%d  col= %d  value= %.4e\n", row, col, gKoef[row][col]);
	return value;
}


/*!
 Restores factory calibration. User calibration will be deleted.
 @param deviceAddress I2C address of the chip (default 0x20)
 @note
 usage with server:
 - restore factory calibration  -> restoreCalibration
 */
int16_t apiRestoreCalibration(const int deviceAddress) {
	return calibrationRestore(deviceAddress);
}

/*!
 Sets the offset in cm
 @param offset in cm
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns offset in cm
 @note
 usage with server
 - set Offset to -12cm -> setOffset -12
 .
 default:
 - value from the offset file inside /calibration/offset_user_20.bin or /calibration/offset_factory_20.bin
 */
int16_t apiSetOffset(const int offset, const int deviceAddress) {
	return calibrationSetOffset(offset, deviceAddress);
}

/*!
 Gets the offset.
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns offset in cm
 @note
 usage with server
 - get Offset -> getOffset
 */
int16_t apiGetOffset(const int deviceAddress) {
	return calibrationGetOffset(deviceAddress);
}

/*!
 Enables or disables the default offset.
 @param state if 0 the Offset set over apiSetOffset will be used else the offset from the calibration
 @returns state
 usage with server
 - take manual offset -> enableDefaultOffset 0
 .
 default:
 - enableDefaultOffset 1
 */
int16_t apiEnableDefaultOffset(const int state) {
	return calibrationEnableDefaultOffset(state);
}

/*!
 Shows if there is a calibration for the current mode.
 @returns -1 for no calibration, 0 for calibration disabled, 1 factory calibration, 2 for user calibration
 @note
 usage with server
 - check if calibration for this mode is available -> isCalibrationAvailable
 */
int16_t apiIsCalibrationAvailable() {
	return calibrationIsCalibrationAvailable();
}

/*!
 Sets the hysteresis for the amplitude (upper threshold will be minAmplitude + hysteresis and the lower threshold will be minAmplitude)
 @returns the value of the hysteresis
 @note
 usage with server
 - set Hysteresis to 20 -> setHysteresis 20
 .
 default:
 - setHysteresis 0
 */
int16_t apiSetHysteresis(const int value) {
	return hysteresisSetThresholds(pruGetMinAmplitude(), value);
}

/*!
 Gets a list of available calibration files (int = code for specific calibration, check calibrationCalculateStoreID in calibration.c for more information)
 @param calibrationList pointer to the list
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the number of available calibration files
 @note
 usage with server
 - getCalibrationList
 */
int16_t apiGetCalibrationList(int *calibrationList, const int deviceAddress) {
	return calibrationGetCalibrationList(calibrationList, deviceAddress);
}

/*!
 Get coordinates x,y of bad pixels.
 "searchBadPixels" has to be done first. (reads from file)
 @params temps pointer to the array badPixel coordinates.
 @returns the number of bad pixels
 @note usage with server:
 - getBadPixels
 */
int16_t apiGetBadPixels(int16_t *badPixels){
	return calibrationGetBadPixels(badPixels);
}

/*!
 Reads the temperature values out of the ROI (dummy rows) and the illumination board (ADC BBB). The values will be �C x 10 ( 254 -> 25.4�C).
 @param deviceAddress I2C address of the chip (default 0x20)
 @params temps pointer to the array of temperatures:
 [0] -> temp top left (chip)
 [1] -> temp top right (chip)
 [2] -> temp bottom left (chip)
 [3] -> temp bottom right (chip)
 [4] -> temp ADC4 (LED top)
 [5] -> temp ADC2 (LED bottom)
 @returns the number of stored temperatures
 @note usage with server:
 - getTemperature
 @warning offset / slope could be different per chip / has to be evaluated. ADC on Beaglebone shows quite unstable results at the moment.
 */
int16_t apiGetTemperature(unsigned int deviceAddress, int16_t *temps) {
	return temperatureGetTemperature(deviceAddress, temps);
}

/*!
 Reads the temperature values out of the ROI (dummy rows).
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the averaged temperature (tr, tl, br, bl). The value will be �C x 10 (421 -> 42.1�C)
 @note usage with server:
 - getAveragedTemperature
 @warning offset / slope could be different per chip / has to be evaluated.
 */
int16_t apiGetAveragedTemperature(unsigned int deviceAddress) {
	return 10 * temperatureGetAveragedChipTemperature(deviceAddress);
}

/*!
 Gets the chip information (IDs, types, versions)
 @param deviceAddress
 @params chipInfo pointer to the array of IDs:
 [0] -> IC_TYPE
 [1] -> IC_VERSION
 [2] -> ENGINEERING_ID
 [3] -> WAFER_ID
 [4] -> CHIP_ID
 [5] -> PART_TYPE
 [6] -> PART_VERSION
 @returns the number of stored IDs
 @note usage with server:
 - getChipInfo
 */
int16_t apiGetChipInfo(const int deviceAddress, int16_t *chipInfo) {
	return chipInfoGetInfo(deviceAddress, chipInfo);
}

/*!
 Selects the modulation mode
 @params select 0 for sinusoidal, 1 for PN
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns selected mode
 @note usage with server:
 - select PN -> selectMode 1
 .
 default:
 - selectMode 0
 */
int16_t apiSelectMode(int select, const int deviceAddress) {
	if (select < 0) {
		select = 0;
	} else if (select > 1) {
		select = 1;
	}
	configSetMode(select, deviceAddress);
	calculationEnablePN(select);
	calibrationLoad(calibrationCalculateStoreID(deviceAddress), deviceAddress);
	imageDifferenceInit();
	return select;
}

/*!
 Selects the polynomial with the given index. Each camera has to use an unique PN polynomial
 @params indexLSFR number of LSFR stages -> 1 = 14, 2 = 13 .... 7 = 8
 @param indexPolynomial index of the polynomial for the given LSFR stages (only 5 stored at the moment / 0-4)
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns selected polynomial
 @note usage with server:
 - selectPolynomial 3 3
 */
int16_t apiSelectPolynomial(unsigned int indexLFSR, unsigned int indexPolynomial, const int deviceAddress) {
	return pnSelectPolynomial(indexLFSR, indexPolynomial, deviceAddress);
}

/*!
 Replaces the value of a pixel with the value of a neighbour
 @returns state
 @note usage with server:
 - enable enableImageCorrection 1
 .
 default:
 - enableImageCorrection 0
 @warning
 - experimental
 */
int16_t apiEnableImageCorrection(const unsigned int state) {
	return imageCorrectionEnable(state);
}

/*!
 Averages images over time
 @param number of pictures you want to average over
 @returns number of pictures
 @note usage with server:
 - average over 10 pictures -> setImageAveraging 10
 .
 default:
 - setImageAveraging 10
 */
int16_t apiSetImageAveraging(const unsigned int averageOver) {
	return imageSetNumberOfPicture(averageOver);
}

/*!
 @warning
 - experimental
 */
int16_t apiSetImageProcessing(const unsigned int mode) {
	return imageSetProcessing(mode);
}

/*!
 @warning
 - experimental
 */
int16_t apiSetImageDifferenceThreshold(const unsigned int cm){
	return imageDifferenceSetThreshold(cm);
}

/*!
 Gets the IC_VERSION of the chip
 @returns IC_VERSION
 @note usage with server:
 - getIcVersion
 */
int16_t apiGetIcVersion(){
	return configGetIcVersion();
}

int16_t apiGetPartVersion(){
	return configGetPartVersion();
}


/*!
Version of the Server
 @returns server version
 @note
 usage with server:
 - version
 */
int16_t apiGetVersion(){
	return getVersion();
}

/*!
 Enables or disables saturation
 @param state if not 0, saturation information will be used and saturated pixels will have the value 31000, else the saturation information doesn't matter
 @returns state
 @note usage with server:
 - enableSaturation 1
 .
 default:
 - enableSaturation 0
 */
int16_t apiEnableSaturation(int state){
	return saturationEnable(state);
}

/*!
 Enables or disables adc overflow detection
 @param state if not 0, adc overflow detection information will be used and detected pixels will have the value 32000, else the adc overflow information doesn't matter
 @returns state
 @note usage with server:
 - enableAdcOverflow 1
 .
 default:
 - enableAdcOverflow 0
 */
int16_t apiEnableAdcOverflow(int state){
	return adcOverflowEnable(state);
}

int16_t apiCreateBGTable(int mode){

	printf("apiCreateBGTable: %d\n", mode);

	if( mode < 2 ){
		calculationCalcAB(mode);		 //averaging whole image
	} else if( mode < 4  ) {
		calculationCalcMultiAB(mode, 1); // line averaging
	} else {
		calculationCalcMultiAB(mode, 0); //every pixel
	}

	return mode;
}

int16_t apiDCSCorrection(char* str){

	if ( strcmp(str, "01") == 0 ){
		configDCSCorrection(CORR_BG_AVR_MODE);
		return 0;
	} else if ( strcmp(str, "0123") == 0 ){
		configDCSCorrection(CORR_BG_AVR_MODE);
		return 1;
	} else if ( strcmp(str, "line") == 0 ) {
		configDCSCorrection(CORR_BG_LINE_MODE);
		return 2;
	} else {
		configDCSCorrection(CORR_BG_PIXEL_MODE);
		return 3;
	}

}

/*!
 Check if FLIM is available
 @returns state
 @note usage with server:
 - isFLIM
 */
int16_t apiIsFLIM() {
	return configIsFLIM();
}

/*!
 Set t1 for FLIM
 @returns register value
 @note usage with server:
  - set t1 to 12.5ns - FLIMSetT1 12.5
  .
 default:
 - FLIMSetT1 25.0
 */
int16_t apiFLIMSetT1(uint16_t value) {
	return flimSetT1(value);
}

/*!
 Set t2 for FLIM
 @returns register value
 @note usage with server:
  - set t2 to 12.5ns - FLIMSetT2 12.5
    .
 default:
 - FLIMSetT2 12.5
 */
int16_t apiFLIMSetT2(uint16_t value) {
	return flimSetT2(value);
}

/*!
 Set t3 for FLIM
 @returns register value
 @note usage with server:
 - set t3 to 12.5ns - FLIMSetT3 12.5
   .
 default:
 - FLIMSetT3 12.5
 */
int16_t apiFLIMSetT3(uint16_t value) {
	return flimSetT3(value);
}

/*!
 Set t4 for FLIM
 @returns register value
 @note usage with server:
  - set t4 to 12.5ns - FLIMSetT4 12.5
    .
 default:
 - FLIMSetT4 50.0
 */
int16_t apiFLIMSetT4(uint16_t value) {
	return flimSetT4(value);
}

/*!
 Set tRep for FLIM
 @returns register value
 @note usage with server:
  - set tRep to 12.5ns - FLIMSetTREP 12.5
    .
 default:
 - FLIMSetTREP 125
 */
int16_t apiFLIMSetTREP(uint16_t value) {
	return flimSetTREP(value);
}

/*!
 Set repetitions for the FLIM sequence
 @returns register value
 @note usage with server:
  - set repetitions to 100 - FLIMSetRepetitions 100
    .
 default:
 - FLIMSetRepetitions 1000
 */
int16_t apiFLIMSetRepetitions(uint16_t value) {
	return flimSetRepetitions(value);
}

/*!
 Set tFlashDelay for FLIM
 @returns register value
 @note usage with server:
   - set tFlashDelay to 12.5ns - FLIMSetFlashDelay 12.5
     .
 default:
 - FLIMSetFlashDelay 12.5
 */
int16_t apiFLIMSetFlashDelay(uint16_t value) {
	return flimSetFlashDelay(value);
}

/*!
 Set tFlashWidth for FLIM
 @returns register value
 @note usage with server:
 - set tFlashWidth to 12.5ns - FLIMSetFlashWidth 12.5
   .
 default:
 - FLIMSetFlashWidth 12.5
 */
int16_t apiFLIMSetFlashWidth(uint16_t value) {
	return flimSetFlashWidth(value);
}

/*!
  Get the FLIM step in ps. (depends on clks)
  @returns FLIM step in ps.
  @note usage with server:
  - FLIMGetStep
 */
int16_t apiFLIMGetStep(){
	return flimGetStep();
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetBWSorted(uint16_t **data) {
	int size = calculationBWSorted(data);
	if(configGetCorrectionBGMode() !=0 ){

		if(configCorrMode == CORR_BG_AVR_MODE){
			calculationBGFlux(data, size);
		} else {
			calculationMultiBGFlux(data, size);
		}

	}
	return size;
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDCSSorted(uint16_t **data) {
	return calculationDCSSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDistanceSorted(uint16_t **data) {
	return calculationDistanceSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetAmplitudeSorted(uint16_t **data) {
	return calculationAmplitudeSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDistanceAndAmplitudeSorted(uint16_t **data) {
	return calculationDistanceAndAmplitudeSorted(data);
}

/*!
 Gets a GRAYSCALE picture. The values will be between 2048 and 4095 LSB.
 The pixel will be ordered like the ordering of the chip. Check the section "Pixel Field" in the datasheet.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns size of the data (uint16_t values) depends on mode and ROI
 @note
 usage with server:
 - getBW
 @warning
 - make sure you are in the GRAYSCALE mode (loadConfig 0) else you get the DCSs
 - if the grayscale picture uses mgb (default = mga) for acquiring the picture the return range will be 0 - 2047 LSB.
 \deprecated If possible use apiGetBWSorted
 */
int apiGetBW(int16_t **data) {
	return calculationBW(data);
}

/*!
 Gets all the DCSs depending on the mode. The values will be between 0 - 4095 LSB.
 The pixel will be ordered like the ordering of the chip. Check the section "Pixel Field" in the datasheet.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns size of the data (uint16_t values) depends on mode and ROI
 @note
 This command is identical with getBW.
 usage with server:
 - getDCS
 @warning make sure you are in THREED mode (loadConfig 1) else you get a single grayscale image
  \deprecated If possible use apiGetDCSSorted
 */
int apiGetDCS(uint16_t **data) {
	return calculationDCS(data);
}

/*!
 Gets the calculated distances. The distance values will be between 0 and 3000 LSB (0 - 2Pi).
 If the minimum amplitude is set (>0) values with a lower amplitude will be set to 30'000.
 The pixel will be ordered like the ordering of the chip. Check the section "Pixel Field" in the datasheet.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns size of the data (uint16_t values) depends on mode and ROI
 @note
 usage with server:
 - getDistance
  \deprecated If possible use apiGetDistanceSorted
 */
int apiGetDistance(uint16_t **data) {
	return calculationDistance(data);
}

/*!
 Gets the calculated amplitudes. The amplitude values will be between 0 and 2047 LSB.
 The pixel will be ordered like the ordering of the chip. Check the section "Pixel Field" in the datasheet.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns size of the data (uint16_t values) depends on mode and ROI
 @note
 usage with server:
 - getAmplitude
  \deprecated If possible use apiGetAmplitudeSorted
 */
int apiGetAmplitude(uint16_t **data) {
	return calculationAmplitude(data);
}

/*!
 Gets the calculated distances and amplitudes (first all distance, then all amplitude values).
 Check apiGetDistance / apiGetAmplitude for more information.
 The pixel will be ordered like the ordering of the chip. Check the section "Pixel Field" in the datasheet.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns size of the data (uint16_t values) depends on mode and ROI
 @note
 usage with server:
 - getDistanceAndAmplitude
  \deprecated If possible use apiGetDistanceAndAmplitudeSorted
 */
int apiGetDistanceAndAmplitude(uint16_t **data) {
	return calculationDistanceAndAmplitude(data);
}

int apiCalibrateGrayscale(int mode){
	return calculationCalibrateGrayscale(mode);
}

int apiCorrectGrayscaleOffset(int enable){
	setCorrectGrayscaleOffset(enable);
	return 0;
}

int apiCorrectGrayscaleGain(int enable){
	setCorrectGrayscaleGain(enable);
	return 0;
}

int apiCalibrateDRNU(){
	return calibrationDRNU();
}

int apiCorrectDRNU(int enable){
	calculationSetEnableDRNU(enable);
	return 0;
}

int16_t apiIsEnabledDRNUCorrection(){
	return configIsEnabledDRNUCorrection();
}

int16_t apiIsEnabledGrayscaleCorrection(){
	return configIsEnabledGrayscaleCorrection();
}
