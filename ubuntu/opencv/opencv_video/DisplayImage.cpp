#include <stdio.h>
#include <vector>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

void usage(int argc, char *argv[])
{
	cout << argv[0] << " video w h" <<endl;
}

int main(int argc, char**argv)
{
	if(argc < 3) {
		usage(argc, argv);
		return 0;
	}

	int video = atoi(argv[1]);
	int w = atoi(argv[2]);
	int h = atoi(argv[3]);

	cout << "video=" << video << " w=" << w << " h=" << h << endl;

    VideoCapture cap(video); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;

    cap.set(CV_CAP_PROP_FRAME_WIDTH,w);  
//    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 320);  
//    cap.set(CV_CAP_PROP_FRAME_HEIGHT,240);  
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, h);  
//    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);  
  
    cout << "Frame Width: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << endl;  
    cout << "Frame Height: " << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;  
    cout << "Frame expo: " << cap.get(CV_CAP_PROP_CONTRAST) << endl;  


    Mat edges;
    namedWindow("edges",1);
    for(;;)
    {
        Mat frame;
        cap >> edges; // get a new frame from camera
        imshow("edges", edges);
        if(waitKey(30) >= 0) break;
    }

    cap.release();
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}

int main_display_video0(int, char**)
{
    VideoCapture cap(0); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;

    Mat edges;
    namedWindow("edges",1);
    for(;;)
    {
        Mat frame;
        cap >> edges; // get a new frame from camera
        imshow("edges", edges);
        if(waitKey(30) >= 0) break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}

int main_display_video0_r_g_b(int argc,char* argv[])  
{  
    VideoCapture cap;  
    cap.open(0);  
  
    if(!cap.isOpened())   
    {  
        exit(0);  
    }  
  
    cap.set(CV_CAP_PROP_FRAME_WIDTH,250);  
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,250);  
  
    cout << "Frame Width: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << endl;  
    cout << "Frame Height: " << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;  
  
    Mat frame;  
    vector<Mat> rgb;  
    cap >> frame;  
  
    namedWindow("original", 1);  
    namedWindow("red", 1);  
    namedWindow("green", 1);  
    namedWindow("blue", 1);  
  
    for(;;)  
    {  
        cap >> frame;  
        imshow("original", frame);  
        split(frame, rgb);  
  
        imshow("red", rgb.at(2));  
        imshow("green", rgb.at(1));  
        imshow("blue", rgb.at(0));  
  
        if(waitKey(30) >= 0)   
            break;  
    }  
  
    waitKey(0);  
    return 1;  
}  

int main_dispaly_jpg(int argc, char** argv )
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
