#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef unsigned short int uint16_t;

#define SAVE_PATH "."

int test_fileexist(char *filename)
{
/*mode：
 * 0 （F_OK） 只判断是否存在
 * 2 （R_OK） 判断写入权限
 * 4 （W_OK） 判断读取权限
 * 6 （X_OK） 判断执行权限 
 * */
    if ( !access(filename,0) ) {
        printf("%s EXISITS!\n", filename);
        return 1;
    }
    else {
        printf("%s DOESN'T EXISIT!\n", filename);
        return 0; 
    }
}

unsigned int test_filesize(char *filename)
{
    /*
     * SEEK_SET, SEEK_CUR, SEEK_ENDi
     * SEEK_SET： 文件开头 : fseek(fp,100L,SEEK_SET);把fp指针移动到离文件开头100字节处；
     * SEEK_CUR： 当前位置 : fseek(fp,100L,SEEK_CUR);把fp指针移动到离文件当前位置100字节处；
     * SEEK_END： 文件结尾 : fseek(fp,100L,2);把fp指针退回到离文件结尾100字节处。
     * */
    FILE *fp = fopen(filename, "rb"); 
    unsigned int filesize = 0;
    if(fp) {
        fseek(fp, 0L,SEEK_END); //定位到文件末 
        filesize = ftell(fp); //文件长度 
    
        fseek(fp, 0L, SEEK_SET);  //定位到文件头

        fclose(fp);
    }
    return filesize;
}

int test_parse_txtfile(char *filename)
{
    
    char line[256];
    FILE *file = fopen(filename, "rb"); 
    char *ptr ;
    if(file) {
        while (fgets(line, sizeof(line), file)) {
            printf("filebuf : %s\n", line);
        }

        fclose(file);
    }

}

int test_saveFile(char *filename, char *buf, unsigned int size)
{
	FILE *fp = NULL;

	printf("save: %s\n", filename);
	fp = fopen(filename, "a+");
	if(fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
	return 0;
}
															
int main()
{
	char filename[64];
	unsigned int size = 24;
	uint16_t *data = (uint16_t *)malloc(size*sizeof(uint16_t));
	
    filename[0] = '\0';
	sprintf(filename, "config.txt");
    test_parse_txtfile(filename);
return 0;



	sprintf(filename, "%s/Image.bin", SAVE_PATH);

    printf("size : %u, sizeof(uint16_t) = %ld\n", size, sizeof(uint16_t));

    test_fileexist(filename);
	test_saveFile(filename, (char *)data, size*sizeof(uint16_t));
    test_fileexist(filename);
    size = test_filesize(filename);
    printf("test_filesize = %d\n", size);
	return 0;
}


