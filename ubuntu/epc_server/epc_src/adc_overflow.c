#include "adc_overflow.h"
#include "calculation.h"

static int adcOverflow = 0;

static uint16_t adcMin = 1;    // epc660, epc502
static uint16_t adcMax = 4094; // epc660, epc502


uint16_t adcOverflowGetMin() {
	return adcMin;
}

void adcOverflowSetMin(uint16_t value) {
	adcMin = value;
}

uint16_t adcOverflowGetMax() {
	return adcMax;
}

void adcOverflowSetMax(uint16_t value) {
	adcMax = value;
}

int16_t adcOverflowEnable(int state){
	adcOverflow = state;
	return state;
}

int adcOverflowIsEnabled(){
	return adcOverflow;
}

int adcOverflowCheck1DCS(uint16_t dcs0){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax);
}

int adcOverflowCheck2DCS(uint16_t dcs0, uint16_t dcs1){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax) || (dcs1 < adcMin) || (dcs1 > adcMax);
}

int adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax) || (dcs1 < adcMin) || (dcs1 > adcMax) || (dcs2 < adcMin)	|| (dcs2 > adcMax) || (dcs3 < adcMin) || (dcs3 > adcMax);
}
