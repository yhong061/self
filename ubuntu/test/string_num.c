#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int a = atoi(argv[1]);
	printf("atoi : %s = %d, errno = %d\n", (char *)argv[1], a, errno);

	int b;
	int ret = sscanf((char *)argv[1], "%d", &b);
	printf("sscanf: %s = %d, errno = %d, ret = %d\n", (char *)argv[1], b, errno, ret);

}
