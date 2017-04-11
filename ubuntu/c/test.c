#include <stdio.h>
#include <time.h>

typedef long long int int64_t;

void usage(char *argv)
{
	printf("====================================\n");
	printf("%s count\n", argv);
	printf("====================================\n");
}

//unsigned long int strtoul(const char *nptr, char **endptr, int base);
//char *ctime(const time_t *timep);
int main(int argc, char *argv[])
{
	int64_t offset = 47500ll * 1000000ll;
	int64_t base = 33333ll;
	int64_t timeUs = 0ll;
	int64_t pcr;
	int64_t pts;
	int64_t pcr_300;

	int64_t limit = 0x105f3e484-0x2000;
	unsigned int display = 10;
	
	unsigned int i = 0;
	unsigned int count;
	
	usage(argv[0]);
	count = strtoul(argv[1], NULL, 10);
	printf("count = %d\n", count);

	for(i=0; i<count; i++) {
		timeUs = i*base + offset;
		pts = timeUs * 9ll / 100ll % 0x1ffffffff;
		pcr = (timeUs * 9ll / 100ll % 0x1ffffffff ) * 300;
		pcr_300 = pts * 300ll;
	
//		if(pcr != pcr_300)
		if(pts > limit && display > 0) {
			printf("time = %llu, pts = %llx, pcr = %llx, pcr_300 = %llx, limit = %llx, base = %llx\n", timeUs, pts, pcr, pcr_300, limit, base);
			if(display > 0)
				display--;
		}
	}


	return 0;
}