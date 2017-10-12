#include "hysteresis.h"
#include <stdio.h>
#include <string.h>

static unsigned char hysteresisMap[328 * 252];
static int hysteresis = 0;
static int upperThreshold = 0;
static int lowerThreshold = 0;

int hysteresisUpdate(const int position, const int value) {
	if (value >= upperThreshold) {
		hysteresisMap[position] = 1;
	} else if (value < lowerThreshold) {
		hysteresisMap[position] = 0;
	}
	return hysteresisMap[position];
}

int hysteresisSetThresholds(const int minAmplitude, const int value) {
	hysteresis = value;
	hysteresisUpdateThresholds(minAmplitude);
	return hysteresis;
}

void hysteresisUpdateThresholds(const int minAmplitude){
	upperThreshold = minAmplitude;
	lowerThreshold = minAmplitude - hysteresis;
	memset(hysteresisMap, 0, 328 * 252 * sizeof(unsigned char));
}

int hysteresisIsEnabled(const int position) {
	return hysteresisMap[position];
}
