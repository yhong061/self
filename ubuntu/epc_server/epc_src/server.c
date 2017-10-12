
#include "server.h"
#include "api.h"
#include "evalkit_constants.h"
#include "i2c.h"
#include "config.h"
#include "evalkit_illb.h"
#include "boot.h"
#include "pru.h"
#include "bbb_led.h"
#include "dll.h"
#include "read_out.h"
#include "log.h"
#include "version.h"
#include "helper.h"
#include "calibration.h"
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <time.h>

void handleRequest(int sock);
void signalHandler(int sig);

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

static unsigned int deviceAddress;
unsigned char* pData;

int16_t temps[6];
int16_t chipInfo[7];

/*!
 Starts TCP Server for communication with client application
 @param addressOfDevice i2c address of the chip (default 0x20)
 @return On error, -1 is returned.
 */
int startServer(const unsigned int addressOfDevice) {
	deviceAddress = addressOfDevice;
	signal(SIGQUIT, signalHandler); 	// add signal to signalHandler to close socket on crash or abort
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	signal(SIGSEGV, signalHandler);
	pData = malloc(MAX_COMMAND_ARGUMENTS - 3 * sizeof(unsigned char));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	int optval = 1;
	// set SO_REUSEADDR on a socket to true
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
		printf("Error setting socket option\n");
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TCP_PORT_NO);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {	// bind server address to socket
		printf("Binding error: please try again in 20sec and make sure no other instance is running\n");
		return -1;
	}
	printf("Socket successfully opened\n");
	listen(sockfd, 5);						// tell socket that we want to listen for communication
	clilen = sizeof(cli_addr);

	// handle single connection
	printf("waiting for requests\n");
	bbbLEDEnable(1, 0);
	bbbLEDEnable(2, 0);
	bbbLEDEnable(3, 0);
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			printf("couldn't accept : newsockfd = %d\n", newsockfd);
			return -1;
		}
		handleRequest(newsockfd);
		close(newsockfd);
	}
	close(sockfd);
	return 0;
}

/*!
 Handles Requests and writes answer into TCP socket
 @param sock TCP-socket to write response.
 @return On error, -1 is returned.
 */
void handleRequest(int sock) {
	bbbLEDEnable(3, 1);
	char buffer[TCP_BUFFER_SIZE];
	bzero(buffer, TCP_BUFFER_SIZE);
	int size = read(sock, buffer, TCP_BUFFER_SIZE);
	if (buffer[size - 1] == '\n'){
		buffer[size - 1] = '\0';
	}
	char stringArray[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH];
	int argumentCount = helperParseCommand(buffer, stringArray);
	unsigned char response[TCP_BUFFER_SIZE];
	bzero(response, TCP_BUFFER_SIZE);
	int16_t answer;

	// show requests
	int ir;
	printf("http = ");
	for (ir = 0; ir < argumentCount + 1; ir++) {
		printf("%s ", stringArray[ir]);
	}
	printf("\n");


	//COMMANDS
	if ((strcmp(stringArray[0], "readRegister") == 0 || strcmp(stringArray[0], "read") == 0 || strcmp(stringArray[0], "r") == 0) && argumentCount >= 1 && argumentCount < 3) {
		unsigned char *values;
		int v;
		int nBytes;
		if (argumentCount == 1) {
			nBytes = 1;
		} else {
			nBytes = helperStringToHex(stringArray[2]);
		}
		int registerAddress = helperStringToHex(stringArray[1]);
		if (nBytes > 0 && registerAddress >= 0) {
			values = malloc(nBytes * sizeof(unsigned char));
			int16_t responseValues[nBytes];
			apiReadRegister(registerAddress, nBytes, values, deviceAddress);
			for (v = 0; v < nBytes; v++) {
				responseValues[v] = values[v];
			}
			send(sock, responseValues, nBytes * sizeof(int16_t), MSG_NOSIGNAL);
			free(values);
		} else {
			answer = -1;
			send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		}
	} else if ((strcmp(stringArray[0], "writeRegister") == 0 || strcmp(stringArray[0], "write") == 0 || strcmp(stringArray[0], "w") == 0) && argumentCount > 1) {
		unsigned char *values = malloc((argumentCount - 1) * sizeof(unsigned char));
		int registerAddress = helperStringToHex(stringArray[1]);
		int i;
		int ok = 1;
		for (i = 0; i < argumentCount - 1; i++) {
			if (helperStringToHex(stringArray[i + 2]) >= 0) {
				values[i] = helperStringToHex(stringArray[i + 2]);
			} else {
				ok = 0;
			}
		}
		if (registerAddress > 0 && ok > 0) {
			answer = apiWriteRegister(registerAddress, argumentCount - 1, values, deviceAddress);
		} else {
			answer = -1;
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		free(values);
	} else if (strcmp(stringArray[0], "getBWSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetBWSorted(&pMem);
		//test_saveFile("BW", (char *)pMem, dataSize);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDCSSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDCSSorted(&pMem);
		//test_saveFile("DCS", (char *)pMem, dataSize);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDistanceSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDistanceSorted(&pMem);
		//test_saveFile("Dis", (char *)pMem, dataSize);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getAmplitudeSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetAmplitudeSorted(&pMem);
		//test_saveFile("Amp", (char *)pMem, dataSize);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDistanceAndAmplitudeSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDistanceAndAmplitudeSorted(&pMem);
		//test_saveFile("Dis_Amp", (char *)pMem, dataSize);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getBW") == 0 && !argumentCount) {
		int16_t *pMem = NULL;
		int dataSize = 2 * apiGetBW(&pMem);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDCS") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDCS(&pMem);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDistance") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDistance(&pMem);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getAmplitude") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetAmplitude(&pMem);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getDistanceAndAmplitude") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		int dataSize = 2 * apiGetDistanceAndAmplitude(&pMem);
		send(sock, pMem, dataSize, MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableIllumination") == 0 && argumentCount == 1) {
		if (!helperStringToInteger(stringArray[1])){
			illuminationDisable();
			answer = 0;
		} else {
			illuminationEnable();
			answer = 1;
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setABS") == 0 && argumentCount == 1) {
		answer = apiEnableABS(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableVerticalBinning") == 0 && argumentCount == 1) {
		answer = apiEnableVerticalBinning(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableHorizontalBinning") == 0 && argumentCount == 1) {
		answer = apiEnableHorizontalBinning(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setRowReduction") == 0 && argumentCount == 1) {
		answer = apiSetRowReduction(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "loadConfig") == 0 && argumentCount == 1) {
		answer = apiLoadConfig(helperStringToInteger(stringArray[1]), deviceAddress);
		if (answer == 1){
			illuminationEnable();
			printf("illumination enabled\n");
		}else{
			illuminationDisable();
			printf("illumination disable\n");
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setIntegrationTime3D") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime3D(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setIntegrationTime3DHDR") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime3DHDR(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "setIntegrationTime2D") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime2D(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setMinAmplitude") == 0 && argumentCount == 1) {
		answer = apiSetMinAmplitude(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getMinAmplitude") == 0 && argumentCount == 0) {
		answer = apiGetMinAmplitude();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableDualMGX") == 0 && argumentCount == 1) {
		answer = apiEnableDualMGX(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableHDR") == 0 && argumentCount == 1) {
		answer = apiEnableHDR(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enablePiDelay") == 0 && argumentCount == 1) {
		answer = apiEnablePiDelay(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setModulationFrequency") == 0 && argumentCount == 1) {
		answer = apiSetModulationFrequency(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getModulationFrequencies") == 0 && argumentCount == 0) {
		int16_t *answerp = apiGetModulationFrequencies();
		send(sock, answerp, 6 * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setROI") == 0 && argumentCount == 4) {
		answer = apiSetROI(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), helperStringToInteger(stringArray[3]), helperStringToInteger(stringArray[4]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibratePixel") == 0 && argumentCount == 2) {
		answer = apiCalibratePixel(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableCalibration") == 0 && argumentCount == 1) {
		answer = apiEnableCalibration(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if ((  (strcmp(stringArray[0], "enableCorrectionBG") == 0) || (strcmp(stringArray[0], "setCorrectionBGMode") == 0)  ) && argumentCount == 1) {
			answer = apiEnableCorrectionBG(helperStringToInteger(stringArray[1]), deviceAddress);
			send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setCorrectionBG") == 0 && argumentCount == 3) {
		answer = apiSetCorrectionBG(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), helperStringToDouble(stringArray[3]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "restoreCalibration") == 0) {
		answer = apiRestoreCalibration(deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setOffset") == 0 && argumentCount == 1) {
		answer = apiSetOffset(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getOffset") == 0) {
		answer = apiGetOffset(deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableDefaultOffset") == 0 && argumentCount == 1) {
		answer = apiEnableDefaultOffset(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getCalibrationList") == 0) {
		int calibrationList[1024];
		answer = apiGetCalibrationList(calibrationList, deviceAddress);
		send(sock, calibrationList, answer * sizeof(int), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "getBadPixels") == 0) {
		int16_t *badPixels = malloc(100000 * sizeof(int16_t));
		answer = apiGetBadPixels(badPixels);
		if (answer < 0){
			send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		}else{
			send(sock, badPixels, answer * sizeof(int16_t), MSG_NOSIGNAL);
		}
		free(badPixels);
	} else if (strcmp(stringArray[0], "getTemperature") == 0) {
		int16_t size = apiGetTemperature(deviceAddress, temps);
		send(sock, &temps, size * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getAveragedTemperature") == 0) {
		int16_t answer = apiGetAveragedTemperature(deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getChipInfo") == 0) {
		int16_t size = apiGetChipInfo(deviceAddress, chipInfo);
		send(sock, &chipInfo, size * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setHysteresis") == 0 && argumentCount == 1) {
		answer = apiSetHysteresis(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isCalibrationAvailable") == 0) {
		answer = apiIsCalibrationAvailable();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "selectMode") == 0 && argumentCount == 1) {
		answer = apiSelectMode(helperStringToInteger(stringArray[1]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "selectPolynomial") == 0 && argumentCount == 2) {
		answer = apiSelectPolynomial(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), deviceAddress);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "factoryCalibration") == 0 && argumentCount == 2) {

		bbbLEDEnable(1, 1);
		bbbLEDEnable(2, 1);
		answer = calibrationMarathon(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]));
		bbbLEDEnable(1, 0);
		bbbLEDEnable(2, 0);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableImageCorrection") == 0 && argumentCount == 1) {
		answer = apiEnableImageCorrection(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageAveraging") == 0 && argumentCount == 1) {
		answer = apiSetImageAveraging(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageProcessing") == 0 && argumentCount == 1) {
		answer = apiSetImageProcessing(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageDifferenceThreshold") == 0 && argumentCount == 1) {
		answer = apiSetImageDifferenceThreshold(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "getIcVersion") == 0 && argumentCount == 0) {
		answer = apiGetIcVersion();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "getPartVersion") == 0 && argumentCount == 0) {
		answer = apiGetPartVersion();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableSaturation") == 0 && argumentCount == 1) {
		answer = apiEnableSaturation(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableAdcOverflow") == 0 && argumentCount == 1) {
		answer = apiEnableAdcOverflow(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "isFLIM") == 0 && !argumentCount) {
		answer = apiIsFLIM();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT1") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT1(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT2") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT2(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT3") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT3(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT4") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT4(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetTREP") == 0 && argumentCount == 1) {
		answer = apiFLIMSetTREP(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetRepetitions") == 0 && argumentCount == 1) {
		answer = apiFLIMSetRepetitions(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetFlashDelay") == 0 && argumentCount == 1) {
		answer = apiFLIMSetFlashDelay(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetFlashWidth") == 0 && argumentCount == 1) {
		answer = apiFLIMSetFlashWidth(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMGetStep") == 0 && argumentCount == 0) {
		answer = apiFLIMGetStep();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		// LOG
	} else if (strcmp(stringArray[0], "searchBadPixels") == 0 && argumentCount == 0) {
		if (calibrationSearchBadPixelsWithMin() < 0){
			answer = -1;
		}else{
			answer = 0;
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "dumpAllRegisters") == 0 && argumentCount == 0) {
		int16_t dumpData[256];
		dumpAllRegisters(deviceAddress, dumpData);
		send(sock, dumpData, sizeof(int16_t) * 256, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "createBGTable") == 0 && argumentCount == 1) {
		answer = apiCreateBGTable(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "dcsCorrection") == 0 && argumentCount == 1) {
		answer = apiDCSCorrection(stringArray[1]);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibrateGrayscale") == 0 && argumentCount == 1) {
		answer = apiCalibrateGrayscale(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctGrayscaleOffset") == 0 && argumentCount == 1) {
		answer = apiCorrectGrayscaleOffset(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctGrayscaleGain") == 0 && argumentCount == 1) {
		answer = apiCorrectGrayscaleGain(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibrateDRNU") == 0 && argumentCount == 0) {
		answer = apiCalibrateDRNU();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctDRNU") == 0 && argumentCount == 1) {
		answer = apiCorrectDRNU(helperStringToInteger(stringArray[1]));
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isDRNUEnabled") == 0 && argumentCount == 0) {
		answer = apiIsEnabledDRNUCorrection();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isGrayscaleCorrectionEnabled") == 0 && argumentCount == 0) {
		answer = apiIsEnabledGrayscaleCorrection();
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "version") == 0 && !argumentCount) {
		int16_t version = apiGetVersion();
		send(sock, &version, sizeof(int16_t), MSG_NOSIGNAL);
	}
	// unknown command
	else {
		printf("unknown command -> %s\n", stringArray[0]);
		int16_t response = -1;
		send(sock, &response, sizeof(int16_t), MSG_NOSIGNAL);
	}
	bbbLEDEnable(3, 0);
}

/*!
 Handles signals
 @param sig signal ID
 */
void signalHandler(int sig) {
	printf("caught signal %i .... going to close application\n", sig);
	close(sockfd);
	close(newsockfd);
	bbbLEDEnable(1, 1);
	bbbLEDEnable(2, 1);
	bbbLEDEnable(3, 1);
	exit(0);
}
