#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int convert_yuv_to_rgb_pixel(int y, int u, int v)
{
        unsigned int pixel32 = 0;
        unsigned char *pixel = (unsigned char *)&pixel32;
        int r, g, b;
        r = y + (1.370705 * (v-128));
        g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
        b = y + (1.732446 * (u-128));
        if(r > 255) r = 255;
        if(g > 255) g = 255;
        if(b > 255) b = 255;
        if(r < 0) r = 0;
        if(g < 0) g = 0;
        if(b < 0) b = 0;
        pixel[0] = r ;
        pixel[1] = g ;
        pixel[2] = b ;
        return pixel32;
}

int convert_rgb_to_yuv_pixel(int r, int g, int b)
{
        unsigned int pixel32 = 0;
        unsigned char *pixel = (unsigned char *)&pixel32;
        int y, u, v;
        r = y + (1.370705 * (v-128));
        g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
        b = y + (1.732446 * (u-128));

		y = (int)( 0.257*r + 0.504*g + 0.098*b +16);
		u = (int)(-0.148*r - 0.291*g + 0.439*b +128);
		v = (int)( 0.439*r - 0.368*g - 0.071*b +128);
		
        if(y > 255) y = 255;
        if(v > 255) v = 255;
        if(v > 255) v = 255;
        if(y < 0) y = 0;
        if(u < 0) u = 0;
        if(v < 0) v = 0;
        pixel[0] = y ;
        pixel[1] = v ;
        pixel[2] = v ;
        return pixel32;
}

int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
        unsigned int in, out = 0;
        unsigned int pixel_16;
        unsigned char pixel_24[3];
        unsigned int pixel32;
        int y0, u, y1, v;

        for(in = 0; in < width * height * 2; in += 4)
        {
                pixel_16 =
                                yuv[in + 3] << 24 |
                                yuv[in + 2] << 16 |
                                yuv[in + 1] <<  8 |
                                yuv[in + 0];
                y0 = (pixel_16 & 0x000000ff);
                u  = (pixel_16 & 0x0000ff00) >>  8;
                y1 = (pixel_16 & 0x00ff0000) >> 16;
                v  = (pixel_16 & 0xff000000) >> 24;
                pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
                pixel_24[0] = (pixel32 & 0x000000ff);
                pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
                pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
                rgb[out++] = pixel_24[0];
                rgb[out++] = pixel_24[1];
                rgb[out++] = pixel_24[2];
                pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
                pixel_24[0] = (pixel32 & 0x000000ff);
                pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
                pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
                rgb[out++] = pixel_24[0];
                rgb[out++] = pixel_24[1];
                rgb[out++] = pixel_24[2];
        }
        return 0;

}

int convert_yuv2rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
        unsigned int in, out = 0;
        unsigned int pixel_16;
        unsigned char pixel_24[3];
        unsigned int pixel32;
        unsigned char R,G,B;
		unsigned char Y0,Y1,Cb,Cr;
		int offset = 0;

        for(in = 0; in < 224*172*4; in += 4)
        {
			Y0 = yuv[in + 1];
			Cb = 128;
			Y1 = yuv[in + 0];
			Cr = 128;
			
			R = (unsigned char)1.164*(Y0-16)+1.596*(Cr-128);
			G = (unsigned char)1.164*(Y0-16)-0.392*(Cb-128)-0.813*(Cr-128);
			B = (unsigned char)1.164*(Y0-16)+2.017*(Cb-128);
			if(in ==0) {
				printf("rgb=%d %d %d, yuv = %d %d %d %d\n", R, G, B, Y0, Cb, Y1, Cr);
			}
			rgb[offset++] = R;
			rgb[offset++] = G;
			rgb[offset++] = B;

			R = 1.164*(Y1-16)+1.596*(Cr-128);
			G = 1.164*(Y1-16)-0.392*(Cb-128)-0.813*(Cr-128);
			B = 1.164*(Y1-16)+2.017*(Cb-128);
			rgb[offset++] = R;
			rgb[offset++] = G;
			rgb[offset++] = B;

		}
		printf("in = %d, offset = %d\n", in, offset);
       return 0;

}



int convert_rgb_to_yuv_buffer(unsigned char *rgb, unsigned char *yuv, unsigned int width, unsigned int height)
{
        unsigned int in, out = 0;
        unsigned int pixel_16;
        unsigned char pixel_24[3];
        unsigned int pixel32;
        char r,g,b;
		char y,u,v;
		int yuv_offset = 0;

        for(in = 0; in < width * height * 2 * 3; in += 3)
        {
			r = rgb[in + 0];
			g = rgb[in + 1];
			b = rgb[in + 2];

			y = (char)( 0.257*r + 0.564*g + 0.098*b+16);
			u = (char)(-0.148*r - 0.291*g + 0.439*b +128);
			v = (char)( 0.439*r - 0.368*g - 0.071*b +128);
if(in == 0)
	printf("rgb = %x %x %x, yuv = %x %x %x", r,g,b,y,u,v);

			if(in%2 != 0) {
				yuv[yuv_offset++] = y;
				//yuv[yuv_offset++] = u;
			}
			else {

				yuv[yuv_offset++] = y;
				yuv[yuv_offset++] = 0;
				yuv[yuv_offset++] = 0;
			}        
		}
       return 0;

}

//平面YUV422转平面RGB24
static void YUV422_to_RGB24(unsigned char *yuv422, unsigned char *rgb24)
{
 int width = 224 * 2;
 int height = 173 * 2;
 int R,G,B,Y,U,V;
 int x,y;
 int nWidth = width>>1; //色度信号宽度
 unsigned char *ptry = yuv422;
 unsigned char *ptru = yuv422+1;
 unsigned char *ptrv = yuv422+3;
 
 for (y=0;y<height;y++)
 {
  for (x=0;x<width;x+=2)
  {
   Y = *(ptry + y*width + x);
   U = *(ptru + y*nWidth + (x>>1));
   V = *(ptrv + y*nWidth + (x>>1));

if(y ==0 && x < 4) {
	printf("YUV = %x %x %x\n", Y, U, V);
}
   
   R = Y + 1.402*(V-128);
   G = Y - 0.34414*(U-128) - 0.71414*(V-128);
   B = Y + 1.772*(U-128);
   
   //防止越界
   if (R>255)R=255;
   if (R<0)R=0;
   if (G>255)G=255;
   if (G<0)G=0;
   if (B>255)B=255;
   if (B<0)B=0;
   
   *(rgb24 + ((height-y-1)*width + x)*3) = B;
   *(rgb24 + ((height-y-1)*width + x)*3 + 1) = G;
   *(rgb24 + ((height-y-1)*width + x)*3 + 2) = R;  
  }
 }
}

//平面YUV420转平面YUV422
static void YUV420p_to_YUV422p(unsigned char *yuv420[3], unsigned char *yuv422, int width, int height)  
{
 int x, y;
 //亮度信号Y复制
 int Ylen = width*height;
 memcpy(yuv422, yuv420[0], Ylen);
 //色度信号U复制
 unsigned char *pU422 = yuv422 + Ylen; //指向U的位置
 int Uwidth = width>>1; //422色度信号U宽度
 int Uheight = height>>1; //422色度信号U高度 
 for (y = 0; y < Uheight; y++) 
 {         
  memcpy(pU422 + y*width,          yuv420[1] + y*Uwidth, Uwidth);
  memcpy(pU422 + y*width + Uwidth, yuv420[1] + y*Uwidth, Uwidth);
 }
 //色度信号V复制
 unsigned char *pV422 = yuv422 + Ylen + (Ylen>>1); //指向V的位置
 int Vwidth = Uwidth; //422色度信号V宽度
 int Vheight = Uheight; //422色度信号U宽度
 for (y = 0; y < Vheight; y++) 
 {  
  memcpy(pV422 + y*width,          yuv420[2] + y*Vwidth, Vwidth);
  memcpy(pV422 + y*width + Vwidth, yuv420[2] + y*Vwidth, Vwidth);
 } 
}

//平面YUV420转RGB24
static void YUV420p_to_RGB24(unsigned char *yuv420[3], unsigned char *rgb24, int width, int height) 
{
//  int begin = GetTickCount();
 int R,G,B,Y,U,V;
 int x,y;
 int nWidth = width>>1; //色度信号宽度
 for (y=0;y<height;y++)
 {
  for (x=0;x<width;x++)
  {
   Y = *(yuv420[0] + y*width + x);
   U = *(yuv420[1] + ((y>>1)*nWidth) + (x>>1));
   V = *(yuv420[2] + ((y>>1)*nWidth) + (x>>1));
   R = Y + 1.402*(V-128);
   G = Y - 0.34414*(U-128) - 0.71414*(V-128);
   B = Y + 1.772*(U-128);

   //防止越界
   if (R>255)R=255;
   if (R<0)R=0;
   if (G>255)G=255;
   if (G<0)G=0;
   if (B>255)B=255;
   if (B<0)B=0;
   
   *(rgb24 + ((height-y-1)*width + x)*3) = B;
   *(rgb24 + ((height-y-1)*width + x)*3 + 1) = G;
   *(rgb24 + ((height-y-1)*width + x)*3 + 2) = R;
//    *(rgb24 + (y*width + x)*3) = B;
//    *(rgb24 + (y*width + x)*3 + 1) = G;
//    *(rgb24 + (y*width + x)*3 + 2) = R;   
  }
 }
}

int YUV4222RGB24(char * inbuf, char * outbuf)
{
	//YUV422_to_RGB24((unsigned char *)inbuf, (unsigned char *)outbuf);
	convert_yuv_to_rgb_buffer((unsigned char *)inbuf, (unsigned char *)outbuf, 224, 173*2);
	
}

int RGB242YUV422(char * inbuf, char * outbuf)
{
	//YUV422_to_RGB24((unsigned char *)inbuf, (unsigned char *)outbuf);
	convert_rgb_to_yuv_buffer((unsigned char *)inbuf, (unsigned char *)outbuf, 224, 172);
	
}

int YUV422_2_RGB24(char * inbuf, char * outbuf)
{
	//YUV422_to_RGB24((unsigned char *)inbuf, (unsigned char *)outbuf);
	convert_yuv2rgb_buffer((unsigned char *)inbuf, (unsigned char *)outbuf, 224, 172);
	
}



int main_YUV422_to_RGB24(char *infile, char *outfile)
{
	unsigned int insize = 224 * 173 * 2 * 2; //YUYV
	unsigned int outsize = 224 * 173 * 2 * 3;
	char *inbuf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(inbuf);
	assert(bufout);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = YUV4222RGB24(inbuf, bufout);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(bufout);

	return 0;

}

int main_RGB24_to_YUV422(char *infile, char *outfile)
{
	unsigned int outsize = 224 * 172 * 2 * 2; //YUYV
	unsigned int insize = 224 * 172 * 2 * 3;
	char *inbuf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(inbuf);
	assert(bufout);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

	int ret = RGB242YUV422(inbuf, bufout);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(bufout);

	return 0;

}

int main_YUV422_TO_RGB24(char *infile, char *outfile)
{
	unsigned int outsize = 224 * 172 * 2 * 3; //YUYV
	unsigned int insize = 224 * 172 * 2 * 2;
	char *inbuf = (char *)malloc(insize);
	char *bufout = (char *)malloc(outsize);
	assert(inbuf);
	assert(bufout);

	FILE *fIn = fopen(infile, "rb");
	if(fIn == NULL) {
		printf("open %s fail\n", infile);
		goto end0;
	}

	FILE *fOut = fopen(outfile, "wb");
	if(fOut == NULL) {
		printf("open %s fail\n", outfile);
		goto end1;
	}

	unsigned int readsize = fread(inbuf, 1, insize, fIn);
	printf("readsize = %d, insize = %d\n", readsize, insize);
	if(readsize != insize) {
		printf("readsize = %d, insize = %d\n", readsize, insize);
	}

printf("size = %d, %d\n", insize, outsize);
	int ret = YUV422_2_RGB24(inbuf, bufout);
	printf("ret = %d\n", ret);
	
	unsigned int writesize = fwrite(bufout, 1, outsize, fOut);
	printf("writesize = %d, outsize = %d\n", writesize, outsize);
	if(writesize != outsize) {
		printf("writesize = %d, outsize = %d\n", writesize, outsize);
	}


	fclose(fOut);
end1:
	fclose(fIn);
end0:
	free(inbuf);
	free(bufout);

	return 0;

}

void usage(void)
{
	printf("---------------------\n");
	printf("./test idx infile outfile\n");
	printf("idx : \n");
	printf("0 crop: 1.yuv422 to 2.rgb24\n");
	printf("---------------------\n");
}

int main(int argc, char* argv[])
{

	if(argc<=3) {
		usage();
		return 0;
	}

	int idx = atoi(argv[1]);
	char *infile = argv[2];
	char *outfile = argv[3];
	switch(idx){
	case 0:
		main_YUV422_to_RGB24(infile, outfile); 	// 1.depth to 2.depth
	break;
	
	case 1:
		main_RGB24_to_YUV422(infile, outfile); 	// 1.depth to 2.depth
	break;

	case 2:
		main_YUV422_TO_RGB24(infile, outfile); 	// 1.depth to 2.depth
	break;

	}
	return 0;
}

