#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int calc(float x1, int x)
{
	int *fp1 = (int *)&x1;
	printf("%f - %08x - %d ", x1, *fp1, *fp1);

	float *fp = (float *)&x;
	printf("%f - %08x - %d\n", *fp, x, x);

	return 0;
}

int main(int argc, char *argv[])
{
	float x1 = 0.0;
	x1 = (float)atof(argv[1]);
	printf("x1 = %f\n",x1);
	int x;

	printf("%f = sqrt : %f\n", x1, sqrt(x1));
	x1 = 2.169095;
	x = 1074451061;
	calc(x1, x);
	
	x1 = 2.169095;
	x = 1081514456;
	calc(x1, x);
	
}
