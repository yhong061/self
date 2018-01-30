===============================================================================
下载：
1） 安装依赖库
$ sudo apt install build-essential
$ sudo apt install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
$ sudo apt install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev
$ sudo apt install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev liblapacke-dev
$ sudo apt install libxvidcore-dev libx264-dev
$ sudo apt install libatlas-base-dev gfortran
$ sudo apt install ffmpeg
2）下载最新的opencv源代码
git clone https://github.com/opencv/opencv.git
注意：如果需要额外的支持库，请继续增加
git clone https://github.com/opencv/opencv_contrib.git

3) 进入opencv的目录，建立build目录
mkdir build
建议使用 cmake-gui 来生成 makefile
可以使用 cmake ..
可以有很多选项，通常根据个人情况不同选择不同的编译选项；
注意系统会下载 一些第三方支持库，如果不管的话，有些时候会报错。
4）生成makefile之后就可以
make -j4

5）之后 sudo make install即可。
注意各种库和头文件的目录位置，设置 LD_LIBRARY_PATH等，


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


