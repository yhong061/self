#include "iterator_default.h"
#include "pru.h"

static uint32_t index = 0;
static uint32_t nPxDcs;
static uint16_t h;
static uint16_t w;
static uint32_t x = 0, y = 0;
static uint16_t nHalves;
static uint32_t hymax = 6;

void iteratorDefaultInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height){
	nPxDcs = numberOfPixelsPerDCS;
	w = width;
	h = height;
	index = 0;
	x = 0;
	y = 0;
	nHalves = pruGetNumberOfHalves();
	printf("xywh=%d %d %d %d -- nPxDcs = %d, nHalves = %d\n", x, y, w, h, nPxDcs, nHalves);
}

bool iteratorDefaultHasNext(){
if(index < hymax) printf("\nindex[%d] < nPxDcs[%d]\n", index, nPxDcs);
	return index < nPxDcs;
}

struct Position iteratorDefaultNext(){
	struct Position pos;
if(index < hymax) printf("xywh=%d %d %d %d -- nPxDcs = %d, nHalves = %d\n", x, y, w, h, nPxDcs, nHalves);
	if (nHalves == 2) {
		if (x < w){
			pos.x = x;
			pos.y = h / 2 - 1 - y;
		}else{
			pos.x = x - w;
			pos.y = h / 2 + y;
		}
	} else {
		pos.x = x;
		pos.y = h - 1 - y;
	}

	pos.indexMemory = index;
	pos.indexSorted = pos.y * w + pos.x;

	++x;
	if (x == nHalves * w){
		x = 0;
		++y;
	}
	++index;
if(index < hymax) printf("pos : x,y = %d, %d, indexMemory = %d, indexSorted = %d\n", pos.x, pos.y, pos.indexMemory, pos.indexSorted);

	return pos;
}

