#include <stdio.h>
#define TS_STREAM_NAME_CHN0		"live555_chN0.ts"
#define TS_STREAM_NAME_CHN1 	"live555_chn1.ts"
#define SHM_DEVICE   "/mnt/media/live555shm"

int main(void)
{
  char const* fileName = "live555_chn0.ts";
  char shmname[128];

  if(!strcmp(fileName, TS_STREAM_NAME_CHN0))  
    sprintf(shmname, "%s_0", SHM_DEVICE);
  else if(!strcmp(fileName, TS_STREAM_NAME_CHN1)) 
    sprintf(shmname, "%s_1", SHM_DEVICE);
  else  
    sprintf(shmname, "%s", SHM_DEVICE);
  fprintf(stderr, "[%d]shmname = %s, fileName=%s\n", __LINE__, shmname, fileName);

}