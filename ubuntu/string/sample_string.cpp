 #include <iostream> 
 #include <string.h> 
 
 using namespace::std;
 int main(void) 
 {
 	char str1[20] = { "TsinghuaOK"}; 
	char str2[20] = { "Computer"}; 
	cout <<"str1 = "<<str1<<","<<"str2 = "<<str2<<endl; 
	cout <<"new str1 = "<<strcpy(str1,str2)<<endl; 

 	return 0;
 }
