#include <pthread.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#define NUM 6

pthread_mutex_t mutex;
void *thread_function(void *arg);  
int gExit = 0;

int get_flag()
{
	int flag;
	pthread_mutex_lock(&mutex);
	flag = gExit;
	pthread_mutex_unlock(&mutex);
	return flag;
}

void set_flag()
{
	pthread_mutex_lock(&mutex);
	gExit++;
	pthread_mutex_unlock(&mutex);
}

void signalHandler(int sig) {
	printf("caught signal %i .... going to close application\n", sig);
	set_flag();
}

int main()  
{  
	int res;  
	pthread_t a_thread[NUM];  
	void *thread_result;  
	int index;  

	signal(SIGQUIT, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	signal(SIGSEGV, signalHandler);
				
	res = pthread_mutex_init(&mutex, NULL);  
	if (res != 0)  
	{  
		perror("Mutex init failed!");  
		exit(EXIT_FAILURE);  
	}  

	for (index = 0; index < NUM; ++index) {  
		res = pthread_create(&a_thread[index], NULL, thread_function, (void *)&index);  
		if (res != 0)  
		{  
			perror("Thread create failed!"); 
			pthread_mutex_destroy(&mutex);
			exit(EXIT_FAILURE);  
		}  
		sleep(1);  
	}  

	printf("Waiting for threads to finished...\n"); 
	int flag;
	
	while(1) {
		sleep(1);
		flag = get_flag();
		if(flag != 0) {
			printf("flag = %d\n", flag);
		}
		if(flag == NUM+1)
			break;
	}

	for (index = NUM - 1; index >= 0; --index)  
	{  
		res = pthread_join(a_thread[index], &thread_result);  
		if (res == 0)  
		{  
			printf("Picked up a thread:%d\n", index + 1);  
		}  
		else  
		{  
			perror("pthread_join failed\n");  
		}  
	}  

	printf("All done\n");  
	pthread_mutex_destroy(&mutex);
	exit(EXIT_SUCCESS);  
}  

double piCaculate(unsigned int paramInt) {
	double d1 = 0.0D;
	double d2 = 1.0D;
	for (unsigned int i = 0; i < paramInt; i++) {
		d1 += d2 * (1.0D / (1.0D + 2.0D * i));
		d2 = -d2;
	}
	return 4.0D * d1;
}

void *thread_function(void *arg)  
{
	int *ptr = (int *)arg;  
	int my_number = (int )*ptr;  
	int rand_num;  
	struct timeval tv1, tv2;

	printf("thread_function is running. Argument was %d\n", my_number); 
	while(1) {
		int idx = 0;
		int loop = 150;
		unsigned int cnt = 1000000;
		double val = 0.0;
		gettimeofday(&tv1, NULL);
		while(idx++ <= loop) {
			val = piCaculate(cnt);
		}
		gettimeofday(&tv2, NULL);
		unsigned long int laspe21 = (tv2.tv_sec*1000000 + tv2.tv_usec) - (tv1.tv_sec*1000000 + tv1.tv_usec);
		if(my_number == 0)
			printf("[%d] %lu\n", my_number, laspe21);
		else
			printf("[%d] %lu ", my_number, laspe21);
		fflush(stdout);
		if(get_flag()) {
			break;
		}
	}

	set_flag();
	printf("Bye from %d\n", my_number);  
	pthread_exit(NULL);  
}  
