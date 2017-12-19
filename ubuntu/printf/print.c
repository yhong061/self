#include <stdlib.h>   
#include <stdio.h>   
#include <unistd.h>   
  
int main()  
{  
	printf("test stdout\n");  
	freopen("test1.txt","w",stdout); //注: 不要使用这类的代码 stdout = fopen("test1.txt","w");   这样的话输出很诡异的. 最好使用  freopen 这类的函数来替换它.  
	printf("test file\n");  
	freopen("/dev/console","w",stdout);  
	printf("test tty\n");  

	return 0;
}  
