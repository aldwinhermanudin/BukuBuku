//Uncomment the following line if you are compiling this code in Visual Studio
//#include "stdafx.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <utility>

using namespace cv;
using namespace std;

int main(int argc, char** argv){
    // Read the image file
    cv::Mat greyMat;
    Mat image = imread("04.bmp");
    // Mat image = imread("test.png");

    // Check for failure
    if (image.empty()){
        cout << "Could not open or find the image" << endl;
        cin.get(); //wait for any key press
        return -1;
    }

    // width=800; height=600
    std::pair<int,int> driver_w_h = std::make_pair(800,600);
    Mat driver_fb(driver_w_h.second,driver_w_h.first, CV_8UC3, Scalar(0,0,0));
    
    // copy the photo to driver_fb, photo size need to be smaller than driver_fb
    image.copyTo(driver_fb(cv::Rect(0,0,image.cols, image.rows)));
    image = driver_fb;

    cout << "Width : " << image.size().width << endl;
    cout << "Height: " << image.size().height << endl;
    cv::cvtColor(image, greyMat, cv::COLOR_BGR2GRAY);

    String window_original = "Original Image"; //Name of the windo
    namedWindow(window_original); // Create a window
    imshow(window_original, image); // Show our image inside the created window.
    
    String window_grey = "Grey Image"; //Name of the windo
    namedWindow(window_grey); // Create a window
    imshow(window_grey, greyMat); // Show our image inside the created window.
    
    waitKey(0); // Wait for any keystroke in the window

    destroyWindow(window_original); //destroy the created window
    destroyWindow(window_grey); //destroy the created window

    return 0;
}