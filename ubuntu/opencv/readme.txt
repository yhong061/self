===============================================================================
下载：
https://sourceforge.net/projects/opencvlibrary/

===============================================================================
编译：

cd ~/opencv-3.0.0-rc1
mkdir release
cd release
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install


===============================================================================
测试：

1. 新建文件夹
mkdir ~/opencv-lena
cd ~/opencv-lena

2. 编写程序
gedit DisplayImage.cpp

***************************************
#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace cv;
int main(int argc, char** argv )
{
	if ( argc != 2 )
	{
		printf("usage: DisplayImage.out <Image_Path>\n");
		return -1;
	}
	Mat image;
	image = imread( argv[1], 1 );
	if ( !image.data )
	{
		printf("No image data \n");
		return -1;
	}
	namedWindow("Display Image", WINDOW_AUTOSIZE );
	imshow("Display Image", image);
	waitKey(0);
	return 0;
}

***************************************

3. 编写makefile

gedit CMakeLists.txt
***************************************
cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
find_package( OpenCV REQUIRED )
add_executable( DisplayImage DisplayImage.cpp )
target_link_libraries( DisplayImage ${OpenCV_LIBS} )
***************************************

4. 编译

cd ~/opencv-lena
cmake .
make

5. 执行

此时opencv-lena文件夹中已经产生了可执行文件DisplayImage，下载lena.jpg放在opencv-lena下，运行

./DisplayImage lena.jpg



===============================================================================


