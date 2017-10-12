#include <pthread.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>

#define DBG(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)
#define ERR(fmt, ...) printf("[EPC][%d]" fmt, __LINE__, ## __VA_ARGS__)

#define DATA_PATH "."

#define WIDTH	(320)
#define HEIGHT	(240)

typedef unsigned int bool;

typedef struct _EpcDataInfo_ {
	char				*pDataBuf;
	unsigned int 		sequence;
	struct timeval		tv;
}EpcDataInfo;

struct _EpcList_ {
	char				*pBuf;
	struct _EpcList_	*pNext;	
};
typedef struct _EpcList_ EpcList_st;

typedef struct _EpcBuffer_ {
	unsigned int	mBufferSize;
	int    			mBufferCnt;
	int    			mBufferUsed;
	char			*pData;
	
	pthread_mutex_t mMutex;

	EpcList_st 		*pEmpty;
	EpcList_st 		*pFull;
}EpcBuffer_st;

typedef struct _EpcData_Range_ {
	unsigned int 	mMin;
	unsigned int 	mMax;
	unsigned int 	mCur;
}EpcData_Range_st;

typedef struct _EpcData_ {
	unsigned int 		mWidth;
	unsigned int 		mHeight;
	unsigned int 		mPixel;
	unsigned int 		mDcs;
	unsigned int 		mImageSize;
	
	EpcData_Range_st	mFilesRange;
}EpcData_st;

typedef struct _EpcConfig_ {
	EpcData_st		*pData;
	EpcBuffer_st	*pDataBuffer;
}EpcConfig_st;

EpcConfig_st gEpccfg;

EpcConfig_st *EpcConfig_Get(void)
{
	return (EpcConfig_st *)&gEpccfg;
}

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

char *EpcBuffer_EmptyBuf_Dec(EpcBuffer_st *epcbuf)
{
	EpcList_st *list = NULL;
	epcbuf->pEmpty = EpcList_RemoveHead(epcbuf->pEmpty, (EpcList_st **)&list);
	if(list == NULL){
		DBG("epc_empty is empty, return\n");
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

char *EpcBuffer_FullBuf_Dec(EpcBuffer_st *epcbuf)
{
	EpcList_st *list = NULL;
	epcbuf->pFull = EpcList_RemoveHead(epcbuf->pFull, (EpcList_st **)&list);
	if(list == NULL){
		DBG("pFull is empty, return\n");
		return NULL;
	}

	char *buf = list->pBuf;
	//DBG("epcbuf->pFull = %p, list = %p, buf = %p\n", epcbuf->pFull, list, buf);
	free(list);
	//DBG("free list[%p]\n", list);
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

int EpcBuffer_Uninit(EpcBuffer_st* epcbuf)
{
	char *buf = NULL;
	if(epcbuf == NULL)
		return 0;
	
	//EpcList_Printf(epcbuf->pFull);
	do{
		buf = EpcBuffer_FullBuf_Dec(epcbuf);
	}while(buf != NULL);
	
	//EpcList_Printf(epcbuf->pEmpty);
	do{
		buf = EpcBuffer_EmptyBuf_Dec(epcbuf);
	}while(buf != NULL);

	if(epcbuf->pData) {
		free(epcbuf->pData);
		//DBG("free epcbuf->pData[%p]\n", epcbuf->pData);
		epcbuf->pData = NULL;
	}

	pthread_mutex_destroy(&epcbuf->mMutex);

	free(epcbuf);
	//DBG("free epcbuf[%p]\n", epcbuf);
	epcbuf = NULL;

	return 0;
}

EpcBuffer_st* EpcBuffer_Init(unsigned int size, int cnt)
{
	int i;
	char *buf = NULL;
	EpcBuffer_st* epcbuf = (EpcBuffer_st*)malloc(sizeof(EpcBuffer_st));
	if(epcbuf == NULL) {
		ERR("mallco epcbuf fail, return\n");
		return NULL;
	}
	//DBG("malloc epcbuf[%p]\n", epcbuf);

	epcbuf->mBufferSize = size;
	epcbuf->mBufferCnt  = cnt;
	epcbuf->mBufferUsed = 0;
    if(pthread_mutex_init(&epcbuf->mMutex, NULL) < 0) {
    	ERR("epcbuf->mMutex init fail!, return\n");
		free(epcbuf);
		//DBG("free epcbuf[%p]\n", epcbuf);
		epcbuf = NULL;
		return NULL;
    }
		
	epcbuf->pData = malloc(epcbuf->mBufferSize * epcbuf->mBufferCnt);
	if(epcbuf->pData == NULL) {
		ERR("mallco epcbuf->pData fail, return\n");
		EpcBuffer_Uninit(epcbuf);
		return NULL;
	}
	//DBG("malloc pData[%p]\n", epcbuf->pData);

	epcbuf->pEmpty = NULL;
	epcbuf->pFull = NULL;
	for(i=0; i<cnt; i++) {
		buf = epcbuf->pData + i*epcbuf->mBufferSize;
		//DBG("epcbuf->pEmpty = %p, buf = %p\n", epcbuf->pEmpty, buf);
		epcbuf->pEmpty = EpcList_Add(epcbuf->pEmpty, buf);
	}

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


int EpcData_Uninit(EpcData_st * epcdata)
{
	if(epcdata == NULL )
		return 0;

	free(epcdata);
	//DBG("free epcdata[%p]\n", epcdata);
	epcdata = NULL;
	
	return 0;
}

EpcData_st*  EpcData_Init(unsigned int dcs)
{
	EpcData_st * epcdata = (EpcData_st *)malloc(sizeof(EpcData_st));
	if(epcdata == NULL) {
		ERR("mallco epcdata fail, return\n");
		return NULL;
	}
	//DBG("malloc epcdata[%p]\n", epcdata);

	epcdata->mWidth  = WIDTH;
	epcdata->mHeight = HEIGHT;
	epcdata->mPixel  = sizeof(unsigned short);
	epcdata->mDcs    = dcs;
	epcdata->mImageSize = epcdata->mWidth * epcdata->mHeight * epcdata->mPixel * epcdata->mDcs;

	// 4: 1-410
	// 2: 1-519

	EpcData_FilesRangeinit(epcdata, 1, 2);
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

int EpcData_GetDataFromFile(char *filename, char *buf, unsigned int size)
{
	FILE *fp = NULL;
	fp = fopen(filename, "rb");
	if(fp == NULL) {
		ERR("open %s fail\n", filename);
		return -1;
	}

	fread(buf, 1, size, fp);
	fclose(fp);

	return size;
}

int EpcData_GetDataFromFiles(EpcData_st * epcdata, char *buf, unsigned int size)
{
	char filename[128];
	FILE *fp = NULL;
	unsigned int imagesize = epcdata->mImageSize;
	unsigned int cur = EpcData_FilesRange_GetCur(epcdata);

	filename[0] = '\0';
	sprintf(filename, "%s/Image_%d_%04d.bin", DATA_PATH, imagesize, cur);  //1400120962_351755_Dis.bin
	//DBG("get: %s\n", filename);	
	return EpcData_GetDataFromFile(filename, buf, imagesize);
}

int EpcData_GetData(void)
{
	int ret;
	char *buf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int bufsize = EpcBuffer_GetBufferSize(epccfg->pDataBuffer);

	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	buf = EpcBuffer_EmptyBuf_Dec(epccfg->pDataBuffer);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	if(buf == NULL) {
		DBG("no buf empty, return\n");
		return -1;
	}
	DBG("Get buf = %p\n", buf);
	
	ret = EpcData_GetDataFromFiles(epccfg->pData, buf, bufsize);
	
	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	EpcBuffer_FullBuf_Inc(epccfg->pDataBuffer, buf);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	
	if(ret < 0) {
		DBG("Get FileData Fail, return\n");
		return ret;
	}
	return 0;
}


int EpcConfig_Uninit()
{
	EpcConfig_st *epccfg = EpcConfig_Get();
	if(epccfg->pDataBuffer) {
		EpcBuffer_Uninit(epccfg->pDataBuffer);
	}

	if(epccfg->pData) {
		EpcData_Uninit(epccfg->pData);
	}

	return 0;
}

int EpcConfig_Init()
{
	EpcConfig_st *epccfg = EpcConfig_Get();
	
	int dcs = 4;
	epccfg->pData = EpcData_Init(dcs);
	if(epccfg->pData == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

	int cnt = 6;	
	unsigned int imagesize = epccfg->pData->mImageSize;
	epccfg->pDataBuffer = EpcBuffer_Init(imagesize, cnt);
	if(epccfg->pDataBuffer == NULL){
		EpcConfig_Uninit();
		return -1;		
	}

}

int EpcCalc_ProcessData(char *buf, unsigned int size)
{
	//DBG("clac buf = %p\n", buf);
	return 0;
}

int EpcCalc_GetData(void)
{
	int ret;
	char *buf = NULL;
	EpcConfig_st *epccfg = EpcConfig_Get();
	unsigned int bufsize = EpcBuffer_GetBufferSize(epccfg->pDataBuffer);
	
	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	buf = EpcBuffer_FullBuf_Dec(epccfg->pDataBuffer);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	if(buf == NULL) {
		DBG("no buf full, return\n");
		return -1;
	}
	DBG("Calc buf = %p\n", buf);
	
	ret = EpcCalc_ProcessData(buf, bufsize);
	
	EpcBuffer_MutexLock(epccfg->pDataBuffer);
	EpcBuffer_EmptyBuf_Inc(epccfg->pDataBuffer, buf);
	EpcBuffer_MutexUnLock(epccfg->pDataBuffer);
	
	if(ret < 0) {
		DBG("EpcClac_ProcessData Fail, return\n");
		return ret;
	}
	return 0;
}

#if 0
int main(void) 
{
	EpcConfig_Init();

	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcData_GetData();
	EpcCalc_GetData();
	
	EpcData_GetData();
	EpcCalc_GetData();

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

	printf("Bye from %d\n", my_number);  
	pthread_exit(NULL);  

}

void *thread_Calc(void *arg)  
{
	int *ptr = (int *)arg;  
	int my_number = (int )*ptr; 
	printf("thread_function is running. Argument was %d\n", my_number); 
	
	while(!gExit) {
		EpcCalc_GetData();
		usleep(33000);
	}

	printf("Bye from %d\n", my_number);  
	pthread_exit(NULL);  

}


int main(void)
{  
	int res;  
	pthread_t a_thread_data[NUM];  
	pthread_t a_thread_calc[NUM];  
	void *thread_result;  
	int index;  
	
	EpcConfig_Init();
	
	int data_num = 1;
	for (index = 0; index < data_num; ++index) {  
		res = pthread_create(&a_thread_data[index], NULL, thread_Data, (void *)&index);  
		if (res != 0)  
		{  
			perror("a_thread_data create failed!"); 
			goto end_main; 
		}  
		sleep(1);  
	}  

	int calc_num = 1;
	for (index = 0; index < calc_num; ++index) {  
		res = pthread_create(&a_thread_calc[index], NULL, thread_Calc, (void *)&index);  
		if (res != 0)  
		{  
			perror("a_thread_calc create failed!"); 
			goto end_data;  
		}  
		sleep(1);  
	}  

	printf("Waiting for threads to finished...\n"); 
	int flag;
	FILE *fp = NULL;
	
	while(1) {
		sleep(1);

		fp = fopen("test", "rb");
		if(fp) {
			fclose(fp);
			gExit = 1;
			sleep(2);
			break;
		}
	}

end_calc:
	for (index = calc_num - 1; index >= 0; --index)  
	{  
		res = pthread_join(a_thread_calc[index], &thread_result);  
		if (res == 0)  
		{  
			printf("Picked up a thread:%d\n", index + 1);  
		}  
		else  
		{  
			perror("pthread_join failed\n");  
		}  
	} 
	
end_data:
	for (index = data_num - 1; index >= 0; --index)  
	{  
		res = pthread_join(a_thread_data[index], &thread_result);  
		if (res == 0)  
		{  
			printf("Picked up a thread:%d\n", index + 1);  
		}  
		else  
		{  
			perror("pthread_join failed\n");  
		}  
	}  


end_main:
	printf("All done\n");  
	EpcConfig_Uninit();
	exit(EXIT_SUCCESS);  
}
#endif
