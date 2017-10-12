int specture_save_buf2(void *data, char *buf)
{
	struct depthpoint *p = (struct depthpoint *) data;
	unsigned short *v = (unsigned short *) buf;
	unsigned int i;
	FILE *fp = NULL;

	#ifdef DEBUG_DUMP_FILE_RAW
	fp = fopen("specure_pixels_xyz.txt", "wb+");
	if (!fp) {
		printf("dnx(%d) open specure_pixels_xyz.txt fail\n", __LINE__);
	}   
	#endif

	int cnt = 10;

	for (i = 0; i < WIDTH * HEIGHT; i++) {
		*v++ = (unsigned short) (p[i].x * 2048);
		*v++ = (unsigned short) (p[i].y * 2048);
		*v++ = (unsigned short) (p[i].z * 2048);
		*v++ = (unsigned short) (p[i].noise * 2048);
		#ifdef DEBUG_DUMP_FILE
		if (fp) { 
			if (p[i].z > 1.0)
				fprintf(fp, "%f;%f;%f\n", 0, 0 , 0); 
			else
				fprintf(fp, "%f;%f;%f\n", p[i].x, p[i].y, p[i].z);
		}   
		#endif
	}   
	if (fp != NULL)
		fclose(fp);

	#if DEBUG_DUMP_FILE_RAW
	{   
		static unsigned int idx = 0;

		if (idx == 0)
			fp = fopen("specure_pixels_x0.raw", "wb+");
		else if (idx == 1)
			fp = fopen("specure_pixels_x1.raw", "wb+");

		fwrite(buf, 2, WIDTH * HEIGHT, fp);
		fclose(fp);
		idx++;
	}
	#endif
	return 0;
}


#define WIDTH (224)
#define HEIGHT (172)
int main_parsecloud(char *infile, char *outfile)
{
	unsigned int insize = WIDTH *HEIGHT * sizeof(float) * 4;
	unsigned int outsize = WIDTH *HEIGHT * sizeof(short int) * 4;
	char filename[128];
	char *inbuf = (char *)malloc(insize);
	char *outbuf = (char *)malloc(outsize);
	assert(inbuf);
	assert(outbuf);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}
	fclose(fIn);

	main_parsecloud_xyz(buf, filename, 0);
	specture_save_buf2(inbuf, outbuf);


end0:
	free(inbuf);
	free(outbuf);

	return 0;

}

