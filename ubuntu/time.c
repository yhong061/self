#include<stdio.h>
#include<time.h>

//char *ctime(const time_t *timep);
int main(int argc, char* argv[])
{
	time_t ttime = atoi(argv[1]);
	printf("ttime = %d, buf = %s\n", (int)ttime, ctime(&ttime));
	getchar();
	return 0;
}
