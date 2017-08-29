#include <stdio.h>
#include <stdlib.h>
int main()
{
	unsigned int seed = rand();
	int k;
	printf("seed = %u\n", seed);
	//srand(seed);
	printf("Random Numbers are:\n");
	for(k = 1; k <= 10; k++)
	{
		printf("%i",rand());
		printf("\n");
	}
	return 0;
}
