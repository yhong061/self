#include "epcthread.h"
EpcCalcCfg_st *gEpcCalcCfg = NULL;

EpcConfig_st *EpcConfig_Get(void)
{
	return (EpcConfig_st *)&gEpccfg;
}


//EpcList_Is_Empty():
// 1: is empty
// 0: not empty
bool EpcList_Is_Empty(EpcList_st *epclist)
{
	if(epclist == NULL)
		return 1;
	return 0;
}

void EpcList_Printf(EpcList_st *epclist)
{
	if(EpcList_Is_Empty(epclist)) {
		DBG("epclist = %p\n", epclist);
	}
	else {
		EpcList_Printf(epclist->pNext);
		DBG("epclist = %p, epclist->pBuf = %p, epclist->pNext = %p\n", epclist, epclist->pBuf, epclist->pNext);	
	}
	return;
}

//EpcList_AddTail():
// NULL: error
//other: return list head
EpcList_st * EpcList_AddTail(EpcList_st *epclist, EpcList_st *list)
{
	if(EpcList_Is_Empty(epclist)) {
		epclist = list;
	}
	else {
		epclist->pNext = EpcList_AddTail(epclist->pNext, list);
	}
	return epclist;
}

//EpcList_Add():
// NULL: error
//other: return list head
EpcList_st * EpcList_Add(EpcList_st *epclist, char *buf)
{
	EpcList_st *list = (EpcList_st *)malloc(sizeof(EpcList_st));
	if(list == NULL) {
		ERR("malloc list fail, return\n");
		return NULL;
	}
	//DBG("malloc list[%p]\n", list);

	list->pBuf = buf;
	list->pNext = NULL;
	
	epclist = EpcList_AddTail(epclist, list);
	return epclist;
}

//EpcList_RemoveHead():
// NULL: list empty
//other: get list head
EpcList_st *EpcList_RemoveHead(EpcList_st *epclist, EpcList_st **list)
{
	if(EpcList_Is_Empty(epclist)) {
		return NULL;
	}

	*list = epclist;
	epclist = epclist->pNext;
	return epclist;
}

void EpcBuffer_MutexLock(EpcBuffer_st *epcbuf)
{
	pthread_mutex_lock(&epcbuf->mMutex);
}

void EpcBuffer_MutexUnLock(EpcBuffer_st *epcbuf)
{
	pthread_mutex_unlock(&epcbuf->mMutex);
}

//EpcBuffer_EmptyBuf_Dec():
// NULL: list empty
//other: get list pBuf
char *EpcBuffer_EmptyBuf_Dec(EpcBuffer_st *epcbuf)
{
	EpcList_st *list = NULL;
	epcbuf->pEmpty = EpcList_RemoveHead(epcbuf->pEmpty, (EpcList_st **)&list);
	if(list == NULL){
		//DBG("epc_empty is empty, return\n");
		return NULL;
	}

	char *buf = list->pBuf;
	free(list);
	//DBG("free list[%p]\n", list);
	list = NULL;
	return buf;
}

int EpcBuffer_EmptyBuf_Inc(EpcBuffer_st *epcbuf, char *buf)
{
	epcbuf->pEmpty = EpcList_Add(epcbuf->pEmpty, buf);
	return 0;
}

//EpcBuffer_FullBuf_Dec():
// NULL: list empty
//other: get list pBuf
char *EpcBuffer_FullBuf_Dec(EpcBuffer_st *epcbuf)
{
	EpcList_st *list = NULL;
	epcbuf->pFull = EpcList_RemoveHead(epcbuf->pFull, (EpcList_st **)&list);
	if(list == NULL){
		//DBG("pFull is empty, return\n");
		return NULL;
	}

	char *buf = list->pBuf;
	free(list);
	list = NULL;
	return buf;
}

int EpcBuffer_FullBuf_Inc(EpcBuffer_st *epcbuf, char *buf)
{
	//DBG("epcbuf->pFull = %p, buf = %p\n", epcbuf->pFull, buf);
	epcbuf->pFull = EpcList_Add(epcbuf->pFull, buf);
	return 0;
}

unsigned int EpcBuffer_GetBufferSize(EpcBuffer_st *epcbuf)
{
	return epcbuf->mBufferSize;
}

unsigned int EpcBuffer_GetBufferOffset(EpcBuffer_st *epcbuf)
{
	return epcbuf->mBufferOffset;
}

int EpcBuffer_UnInit(EpcBuffer_st* epcbuf)
{
	char *buf = NULL;
	if(epcbuf == NULL)
		return 0;
	
	//EpcList_Printf(epcbuf->pEmpty);
	//EpcList_Printf(epcbuf->pFull);
	do{
		buf = EpcBuffer_FullBuf_Dec(epcbuf);
	}while(buf != NULL);
	
	do{
		buf = EpcBuffer_EmptyBuf_Dec(epcbuf);
	}while(buf != NULL);

	if(epcbuf->pData) {
		free(epcbuf->pData);
		epcbuf->pData = NULL;
	}

	pthread_mutex_destroy(&epcbuf->mMutex);

	free(epcbuf);
	//DBG("free epcbuf[%p]\n", epcbuf);
	epcbuf = NULL;

	return 0;
}

//EpcBuffer_Init():
// NULL: error
//other: OK
EpcBuffer_st* EpcBuffer_Init(unsigned int size, int cnt)
{
	int i;
	char *buf = NULL;
	EpcDataInfo_st *datainfo = NULL;
	EpcBuffer_st* epcbuf = (EpcBuffer_st*)malloc(sizeof(EpcBuffer_st));
	if(epcbuf == NULL) {
		ERR("mallco epcbuf fail, return\n");
		return NULL;
	}
	//DBG("malloc epcbuf[%p]\n", epcbuf);
	
	memset(epcbuf, 0, sizeof(EpcBuffer_st));
	epcbuf->mBufferSize = size + DATA_INFO_SIZE;
	epcbuf->mBufferOffset = DATA_INFO_SIZE;
	epcbuf->mBufferCnt  = cnt;
	epcbuf->mBufferUsed = 0;
    if(pthread_mutex_init(&epcbuf->mMutex, NULL) < 0) {
    	ERR("epcbuf->mMutex init fail!, return\n");
		free(epcbuf);
		epcbuf = NULL;
		return NULL;
    }
		
	epcbuf->pData = malloc(epcbuf->mBufferSize * epcbuf->mBufferCnt);
	if(epcbuf->pData == NULL) {
		ERR("mallco epcbuf->pData fail, return\n");
		EpcBuffer_UnInit(epcbuf);
		epcbuf = NULL;
		return NULL;
	}
	//DBG("malloc pData[%p]\n", epcbuf->pData);

	epcbuf->pEmpty = NULL;
	epcbuf->pFull = NULL;
	for(i=0; i<cnt; i++) {
		buf = epcbuf->pData + i*epcbuf->mBufferSize;
		datainfo = (EpcDataInfo_st *)buf;
		datainfo->mBufIdx = i;
		epcbuf->pEmpty = EpcList_Add(epcbuf->pEmpty, buf);
	}
	//EpcList_Printf(epcbuf->pEmpty);
	//EpcList_Printf(epcbuf->pFull);

	return epcbuf;
}

void EpcData_FilesRangeinit(EpcData_st * epcdata, unsigned int min, unsigned int max)
{
	epcdata->mFilesRange.mMin = min;
	epcdata->mFilesRange.mMax = max;
	epcdata->mFilesRange.mCur = min;
}


unsigned int EpcData_FilesRange_GetCur(EpcData_st * epcdata)
{
	EpcData_Range_st *range = (EpcData_Range_st *)&epcdata->mFilesRange;
	int cur = range->mCur;
	range->mCur++;
	if(range->mCur > range->mMax)
		range->mCur = range->mMin;
	return cur;
}


int EpcData_UnInit(EpcData_st * epcdata)
{
	if(epcdata == NULL )
		return 0;

	free(epcdata);
	//DBG("free epcdata[%p]\n", epcdata);
	
	return 0;
}

//EpcData_Init():
// NULL: error
//other: OK
EpcData_st*  EpcData_Init(unsigned int dcs)
{
	EpcData_st * epcdata = (EpcData_st *)malloc(sizeof(EpcData_st));
	if(epcdata == NULL) {
		ERR("mallco epcdata fail, return\n");
		return NULL;
	}
	//DBG("malloc epcdata[%p]\n", epcdata);

	memset(epcdata, 0, sizeof(EpcData_st));
	epcdata->mWidth  = WIDTH;
	epcdata->mHeight = HEIGHT;
	epcdata->mPixel  = sizeof(unsigned short);
	epcdata->mDcs    = dcs;
	epcdata->mImageSize = epcdata->mWidth * epcdata->mHeight * epcdata->mPixel * epcdata->mDcs;

	// 4: 1-410
	// 2: 1-519
	EpcData_FilesRangeinit(epcdata, 1, 20);
	return epcdata;
}

unsigned int EpcData_GetImageSize(EpcData_st * epcdata)
{
	return epcdata->mImageSize;
}


int EpcData_GetDataEnable()
{
	return 0;
}

int EpcData_GetDataDisable()
{
	return 0;
}

//EpcData_GetDataFromFile():
//     0: error
// other: imagesize
unsigned int EpcData_GetDataFromFile(char *filename, char *buf, unsigned int imagesize)
{
	FILE *fp = NULL;
	fp = fopen(filename, "rb");
	if(fp == NULL) {
		ERR("open %s fail\n", filename);
		return 0;
	}

	unsigned int readsize = fread(buf, 1, imagesize, fp);
	if(readsize != imagesize)
		DBG("readsize[%d] != imagesize[%d]\n", readsize, imagesize);
	fclose(fp);

	return readsize;
}

//EpcData_GetDataFromFiles():
//-1: error
// 0: OK
int EpcData_GetDataFromFiles(EpcData_st * epcdata, char *buf, unsigned int bufsize, unsigned int bufoffset)
{
	char filename[128];
	FILE *fp = NULL;
	unsigned int imagesize = epcdata->mImageSize;
	unsigned int cur = EpcData_FilesRange_GetCur(epcdata);
	EpcDataInfo_st *datainfo = (EpcDataInfo_st *)buf;
	char *databuf = (char *)buf+bufoffset;

	if(imagesize+bufoffset > bufsize) {
		DBG("imagesize[%d]+bufoffset[%d] > bufsize[%d]\n", imagesize, bufoffset, bufsize);
		return -1;
	}

	datainfo->mWidth  = epcdata->mWidth;
	datainfo->mHeight = epcdata->mHeight;
	datainfo->mDcs    = epcdata->mDcs;

	filename[0] = '\0';
	sprintf(filename, "%s/Image_%d_%04d.bin", DATA_PATH, imagesize, cur);
	//sprintf(filename, "%s/in.bin", DATA_PATH);
	//printf("open %s\n", filename);
	unsigned int readsize = EpcData_GetDataFromFile(filename, databuf, imagesize);
	if(readsize == 0) {
		return -1;
	}
	
	datainfo->mSequence = cur;
	datainfo->mDatasize = readsize;
	gettimeofday(&datainfo->mTv, NULL);
	return 0;
	
}

//EpcData_GetData():
//-1: error
// 0: OK
int EpcData_GetData(void)
{
	int ret;
	char *databuf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int databufsize = EpcBuffer_GetBufferSize(epccfg->pDataBuffer);
	unsigned int databufoffset = EpcBuffer_GetBufferOffset(epccfg->pDataBuffer);
	

	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	databuf = EpcBuffer_EmptyBuf_Dec(epccfg->pDataBuffer);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	if(databuf == NULL) {
		//DBG("no buf empty, return\n");
		return -1;
	}
	
	ret = EpcData_GetDataFromFiles(epccfg->pData, databuf, databufsize, databufoffset);
	if(ret < 0) {
		DBG("Get FileData Fail, return\n");
		EpcBuffer_MutexLock(epccfg->pDataBuffer);
		EpcBuffer_EmptyBuf_Inc(epccfg->pDataBuffer, databuf);
		EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
		return ret;
	}
	else {
		EpcDataInfo_st *datainfo = (EpcDataInfo_st *)databuf;
		//DBG("[D:%d] [S:%04d] [T:%ld.%06ld]\n", datainfo->mBufIdx, datainfo->mSequence, datainfo->mTv.tv_sec, datainfo->mTv.tv_usec);
		EpcBuffer_MutexLock(epccfg->pDataBuffer);
		EpcBuffer_FullBuf_Inc(epccfg->pDataBuffer, databuf);
		EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
		return 0;
	}
}


int EpcCalc_UnInit(EpcCalc_st * epccalc)
{
	unsigned int i;
	
	if(epccalc == NULL )
		return 0;

	for(i=0; i<epccalc->mThrNum; i++) {
		if(epccalc->pThread[i] != NULL) {
			free(epccalc->pThread[i]);
			epccalc->pThread[i] = NULL;
		}
	}
	
	pthread_mutex_destroy(&epccalc->mMutex);
	
	free(epccalc);
	epccalc = NULL;
	
	return 0;
}


EpcCalc_st* EpcCalc_Init(unsigned int threadnum)
{
	unsigned int i;
	EpcCalc_st* epccalc = (EpcCalc_st*)malloc(sizeof(EpcCalc_st));
	if(epccalc == NULL) {
		ERR("mallco epccalc fail, return\n");
		return NULL;
	}

	memset(epccalc, 0, sizeof(EpcCalc_st));

    if(pthread_mutex_init(&epccalc->mMutex, NULL) < 0) {
    	ERR("epccalc->mMutex init fail!, return\n");
		free(epccalc);
		epccalc = NULL;
		return NULL;
    }

	epccalc->mThrNum = threadnum;
	for(i=0; i<epccalc->mThrNum; i++) {
		EpcCalcThr_st* calcthr = (EpcCalcThr_st*)malloc(sizeof(EpcCalcThr_st));
		if(calcthr == NULL) {
			ERR("mallco calcthr fail, return\n");
			EpcCalc_UnInit(epccalc);
			epccalc = NULL;
			return NULL;
		}
		memset(calcthr, 0, sizeof(EpcCalcThr_st));
		calcthr->mThreadIdx = i;
		epccalc->pThread[i] = calcthr;
	}

	epccalc->mWidth  = WIDTH;
	epccalc->mHeight = HEIGHT;
	epccalc->mPixel  = sizeof(unsigned short);
	epccalc->mDistSize = epccalc->mWidth * epccalc->mHeight * epccalc->mPixel;
	epccalc->mAmpSize = epccalc->mWidth * epccalc->mHeight * epccalc->mPixel;
	
	return epccalc;
}

void EpcCalc_MutexLock(EpcCalc_st *epccalc)
{
	pthread_mutex_lock(&epccalc->mMutex);
}

void EpcCalc_MutexUnLock(EpcCalc_st *epccalc)
{
	pthread_mutex_unlock(&epccalc->mMutex);
}


int EpcCalc_ProcessCalc(int threadidx, char *databuf, unsigned int databufsize, char *calcbuf, unsigned int calcbufsize)
{
	EpcCalcInfo_st stcalcinfo;
	EpcCalcInfo_st *calcinfo = (EpcCalcInfo_st *)&stcalcinfo;
	
	calcinfo->mDatabufOffset = DATA_INFO_SIZE;
	calcinfo->mDatabufSize = databufsize;
	calcinfo->pDatabuf = databuf;

	calcinfo->mCalcbufOffset = DATA_INFO_SIZE;
	calcinfo->mCalcbufSize = calcbufsize;
	calcinfo->pCalcbuf = calcbuf;

	calcinfo->pCalcCfg = EpcCalcConfig_Get();

	memcpy(calcbuf, databuf, sizeof(EpcDataInfo_st));
	//EpcCalc_Calc4Dcs((EpcCalcInfo_st *)&stcalcinfo);
	apiGetDistanceAndAmplitudeSorted((EpcCalcInfo_st *)&stcalcinfo);
	
	return 0;
}

int EpcCalc_ProcessData(int threadidx, char *databuf, unsigned int databufsize)
{
	int ret;
	char *calcbuf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int calcbufsize = EpcBuffer_GetBufferSize(epccfg->pCalcBuffer);
	
	EpcBuffer_MutexLock(epccfg->pCalcBuffer);
	calcbuf = EpcBuffer_EmptyBuf_Dec(epccfg->pCalcBuffer);
	EpcBuffer_MutexUnLock(epccfg->pCalcBuffer);
	if(calcbuf == NULL) {
		DBG("no buf full, return\n");
		return -1;
	}
	
	ret = EpcCalc_ProcessCalc(threadidx, databuf, databufsize, calcbuf, calcbufsize);	
	if(ret < 0) {
		DBG("Process CalcData, return\n");
		EpcBuffer_MutexLock(epccfg->pCalcBuffer);
		EpcBuffer_EmptyBuf_Inc(epccfg->pCalcBuffer, calcbuf);
		EpcBuffer_MutexUnLock(epccfg->pCalcBuffer);
		return ret;
	}
	else {
		EpcDataInfo_st *datainfo = (EpcDataInfo_st *)calcbuf;
		//DBG("\t[C:%d] [S:%04d] [T:%ld.%06ld]\n", threadidx, datainfo->mSequence, datainfo->mTv.tv_sec, datainfo->mTv.tv_usec);

		char *buf = calcbuf + DATA_INFO_SIZE;
		unsigned int size = calcbufsize - DATA_INFO_SIZE;
		test_saveFile(buf, size, datainfo->mSequence);

		EpcBuffer_MutexLock(epccfg->pCalcBuffer);
		EpcBuffer_FullBuf_Inc(epccfg->pCalcBuffer, calcbuf);
		EpcBuffer_MutexUnLock(epccfg->pCalcBuffer);

		return 0;
	}
	
	return 0;
}

int EpcCalc_Process(int threadidx)
{
	int ret;
	char *databuf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int databufsize = EpcBuffer_GetBufferSize(epccfg->pDataBuffer);

	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	databuf = EpcBuffer_FullBuf_Dec(epccfg->pDataBuffer);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	if(databuf == NULL) {
		//DBG("no buf full, return\n");
		return -1;
	}
	
	ret = EpcCalc_ProcessData(threadidx, databuf, databufsize);
	
	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	EpcBuffer_EmptyBuf_Inc(epccfg->pDataBuffer, databuf);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	
	if(ret < 0) {
		DBG("EpcClac_ProcessData Fail, return\n");
		return ret;
	}
	return 0;
}


unsigned int EpcCalc_ThreadRunningGet(int threadidx)
{
	unsigned int running;
	EpcConfig_st *epccfg = EpcConfig_Get();
	EpcCalc_st *epccalc = epccfg->pCalc;
	EpcCalc_MutexLock(epccalc);
	running = epccalc->pThread[threadidx]->mRunning;
	EpcCalc_MutexUnLock(epccalc);
	return running;
}

void EpcCalc_ThreadRunningSet(int threadidx, unsigned int running)
{
	EpcConfig_st *epccfg = EpcConfig_Get();
	EpcCalc_st *epccalc = epccfg->pCalc;
	EpcCalc_MutexLock(epccalc);
	epccalc->pThread[threadidx]->mRunning = running;
	EpcCalc_MutexUnLock(epccalc);
	return;
}

void *EpcCalc_ThreadRun(void *arg)  
{
	int ret = 0;
	int *ptr = (int *)arg;  
	int threadidx = (int )*ptr; 

	EpcCalc_ThreadRunningSet(threadidx, 1);
	//DBG("thread_function is running. Argument was %d\n", threadidx); 
	
	while(1) {
		if(!EpcCalc_ThreadRunningGet(threadidx))
			break;
		
		ret = -1;//EpcCalc_Process(threadidx);
		if(ret < 0) {
			usleep(10000);
		}
	}

	printf("Bye Calc from %d\n", threadidx);  
	pthread_exit(NULL);  
}

int EpcCalc_ThreadJoin(EpcCalc_st *epccalc)
{
	int ret = 0;
	void *result = NULL; 
	unsigned int i;
	EpcCalcThr_st* pThread = NULL;

	for (i = 0; i < epccalc->mThrNum; ++i) {
		pThread = epccalc->pThread[i];
		if(pThread->mCreated == 1) {
			ret = pthread_join(pThread->mThread, &result);
			if(ret != 0){
				ERR("calc[%d] thread_join failed\n", pThread->mThreadIdx);
			}
			pThread->mCreated = 0;
		}
	}  
	return ret;
}


int EpcCalc_ThreadCreate(EpcCalc_st *epccalc)
{
	int ret = 0;
	unsigned int i;
	EpcCalcThr_st	*pThread = NULL;

	for (i = 0; i < epccalc->mThrNum; ++i) {
		pThread = epccalc->pThread[i];
		ret = pthread_create(&pThread->mThread, NULL, EpcCalc_ThreadRun, (void *)&pThread->mThreadIdx);  
		if (ret != 0)  
		{
			pThread->mCreated = 0;
			ERR("calc thr[%d] create failed!\n", pThread->mThreadIdx); 
			EpcCalc_ThreadJoin(epccalc);
			break;  
		}  
		pThread->mCreated = 1;
	}  
	return ret;
}

void EpcApi_FilesRangeinit(unsigned int min, unsigned int max)
{
	EpcConfig_st *epccfg = EpcConfig_Get();
	EpcData_FilesRangeinit(epccfg->pData, min, max);
}

int EpcApi_ProcessData(char *calcbuf, unsigned int calcbufsize)
{
	EpcDataInfo_st *datainfo = (EpcDataInfo_st *)calcbuf;
	//DBG("\t\t[A:0] [S:%04d] [T:%ld.%06ld]\n", datainfo->mSequence, datainfo->mTv.tv_sec, datainfo->mTv.tv_usec);
	return 0;
}

int EpcApi_Process(void)
{
	int ret;
	char *calcbuf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int calcbufsize = EpcBuffer_GetBufferSize(epccfg->pCalcBuffer);
	
	EpcBuffer_MutexLock(epccfg->pCalcBuffer);
	calcbuf = EpcBuffer_FullBuf_Dec(epccfg->pCalcBuffer);
	EpcBuffer_MutexUnLock(epccfg->pCalcBuffer);
	if(calcbuf == NULL) {
		//DBG("no buf full, return\n");
		return -1;
	}
	
	ret = EpcApi_ProcessData(calcbuf, calcbufsize);
	
	EpcBuffer_MutexLock(epccfg->pCalcBuffer);
	EpcBuffer_EmptyBuf_Inc(epccfg->pCalcBuffer, calcbuf);
	EpcBuffer_MutexUnLock(epccfg->pCalcBuffer);
	
	if(ret < 0) {
		DBG("EpcClac_ProcessData Fail, return\n");
		return ret;
	}
	
	return 0;
}



int EpcConfig_Uninit()
{
	EpcConfig_st *epccfg = EpcConfig_Get();

	if(gEpcCalcCfg != NULL) {
		EpcCalcConfig_UnInit(gEpcCalcCfg);
		gEpcCalcCfg = NULL;
	}
	
	if(epccfg->pCalcBuffer) {
		EpcBuffer_UnInit(epccfg->pCalcBuffer);
		epccfg->pCalcBuffer = NULL;
	}

	if(epccfg->pCalc) {
		EpcCalc_UnInit(epccfg->pCalc);
		epccfg->pCalc = NULL;
	}
	
	if(epccfg->pDataBuffer) {
		EpcBuffer_UnInit(epccfg->pDataBuffer);
		epccfg->pDataBuffer = NULL;
	}

	if(epccfg->pData) {
		EpcData_UnInit(epccfg->pData);
		epccfg->pData = NULL;
	}

	
	return 0;
}

//EpcConfig_Init():
//-1: error
// 0: OK
int EpcConfig_Init()
{
	EpcConfig_st *epccfg = EpcConfig_Get();
	
	gEpcCalcCfg = EpcCalcConfig_Init();
	if(gEpcCalcCfg == NULL){
		EpcConfig_Uninit();
		return -1;		
	}
	
	int dcs = 4;
	epccfg->pData = EpcData_Init(dcs);
	if(epccfg->pData == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

	int cnt = BUFFER_NUM;	
	unsigned int imagesize = epccfg->pData->mImageSize;
	epccfg->pDataBuffer = EpcBuffer_Init(imagesize, cnt);
	if(epccfg->pDataBuffer == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

	unsigned int threadnum = CALC_THR_NUM;
	epccfg->pCalc = EpcCalc_Init(threadnum);
	if(epccfg->pCalc == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

	cnt = BUFFER_NUM;	
	unsigned int calcsize = epccfg->pCalc->mDistSize + epccfg->pCalc->mAmpSize;
	epccfg->pCalcBuffer = EpcBuffer_Init(calcsize, cnt);
	if(epccfg->pCalcBuffer == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

	EpcPruInfo_st pruinfo;
	EpcPru_Init((EpcPruInfo_st *)&pruinfo);

	return 0;
}


#if 0
int main(void) 
{
	int ret;
	ret = EpcConfig_Init();
	if(ret < 0)
		return ret;

	EpcApi_FilesRangeinit(2, 2);

	EpcData_GetData();
	EpcCalc_Process(0);
	EpcApi_Process();

	//test();

	EpcConfig_Uninit();

	return 0;
}
#else
#define NUM 1
int gExit = 0;

void *thread_Data(void *arg)  
{
	int *ptr = (int *)arg;  
	int my_number = (int )*ptr; 
	printf("thread_function is running. Argument was %d\n", my_number); 
	
	while(!gExit) {
		EpcData_GetData();
		usleep(33000);
	}

	printf("Bye Data from %d\n", my_number);  
	pthread_exit(NULL);  

}

int main(int argc, char *argv[])
{  
	int ret;
	unsigned int i;
	pthread_t a_thread_data[NUM];  
	void *thread_result;  
	int index;  
	int cnt = 10;
	if(argc > 1) {
		int argcnt = atoi(argv[1]);
		if(cnt < argcnt)
			cnt = argcnt;
	}
	printf("cnt = %d\n", cnt);
	
	ret = EpcConfig_Init();
	if(ret < 0)
		return ret;
	
	int data_num = 1;
	for (index = 0; index < data_num; ++index) {  
		ret = pthread_create(&a_thread_data[index], NULL, thread_Data, (void *)&index);  
		if (ret != 0)  {  
			perror("a_thread_data create failed!"); 
			goto end_main; 
		}  
	}  

	EpcConfig_st *epccfg = EpcConfig_Get();
	EpcCalc_ThreadCreate(epccfg->pCalc);

	while(cnt--) {
		ret = EpcApi_Process();
		if(ret < 0)
			usleep(20000);
	}

	for(i=0; i<CALC_THR_NUM; ++i)
		EpcCalc_ThreadRunningSet(i, 0);
	
	sleep(1);

	EpcCalc_ThreadJoin(epccfg->pCalc);
	sleep(1);

	gExit = 1;
	sleep(1);
	
	for (index = data_num - 1; index >= 0; --index)  
	{  
		ret = pthread_join(a_thread_data[index], &thread_result);  
		if (ret != 0)    {  
			perror("pthread_join failed\n");  
		}  
	}  

end_main:
	printf("All done\n");  
	EpcConfig_Uninit();
	exit(EXIT_SUCCESS);  
}
#endif
