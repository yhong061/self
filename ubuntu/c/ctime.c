#include <stdio.h>
#include <time.h>

void usage(char *argv)
{
	printf("====================================\n");
	printf("%s time\n", argv);
	printf("====================================\n");
}

//unsigned long int strtoul(const char *nptr, char **endptr, int base);
//char *ctime(const time_t *timep);
int main(int argc, char *argv[])
{
	unsigned int val;

	usage(argv[0]);
	val = strtoul(argv[1], NULL, 10);

	printf("time[%10u] = %s\n", val, ctime((time_t *)&val));

	return 0;
}
