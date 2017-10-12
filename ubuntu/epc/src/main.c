
#include "boot.h"
#include "i2c.h"
#include "config.h"
#include "server.h"
#include "bbb_led.h"
#include "version.h"
#include "evalkit_illb.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int MGX = 0;
int PI = 1;
int PN = 0;

void usage(void)
{
	printf("--------------------------\n");
	printf("./tof-imager MGX PI PN\n");
	printf("--------------------------\n");
	return;
}

int main_test(int argc, char* argv[]) {
#if 1
	printf("Version %i.%i.%i\n", getMajor(), getMinor(), getPatch());
	bbbLEDEnable(3, 1);
	int deviceAddress = boot();
	if (deviceAddress < 0){
		bbbLEDEnable(1, 1);
		bbbLEDEnable(2, 0);
		bbbLEDEnable(3, 1);
		printf("Could not boot chip.\n");
		exit(-1);
	}
	printf("init\n");
	//illuminationDisable();
	configSetDeviceAddress(deviceAddress);
	configInit(deviceAddress);
	//illuminationEnable();
#endif
	printf("Starting server...\n");
	if(argc == 4) {
		MGX = atoi(argv[1]);
		PI  = atoi(argv[2]);
		PN = atoi(argv[3]);
	}
	else {
		usage();
	}
	printf("MGX = %d, PI = %d, PN = %d\n", MGX, PI, PN);
	startServer(0/*deviceAddress*/);
	return 0;
}
