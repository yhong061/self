#include "saturation.h"

static int saturation = 0;
static uint16_t saturationBit = 0x1000; // epc660, epc502
static uint16_t saturationData;


void saturationSet1DCS(uint16_t dcs0){
	saturationData = dcs0;
}

void saturationSet2DCS(uint16_t dcs0, uint16_t dcs1){
	saturationData = dcs0 | dcs1;
}

void saturationSet4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	saturationData = dcs0 | dcs1 | dcs2 | dcs3;
}

int16_t saturationEnable(int state){
	saturation = state;
	return state;
}

int saturationCheck(){
	if (!saturation){
		return 0;
	}
	return saturationData & saturationBit;
}

void saturationSetSaturationBit(uint16_t bitMask) {
	saturationBit = bitMask;
}

