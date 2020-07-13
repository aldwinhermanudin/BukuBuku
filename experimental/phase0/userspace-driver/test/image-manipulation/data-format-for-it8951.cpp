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
    // Mat image = imread("04.bmp");
    Mat image = imread("test.png");

    // Check for failure
    if (image.empty()){
        cout << "Could not open or find the image" << endl;
        cin.get(); //wait for any key press
        return -1;
    }

    cout << "Width : " << image.size().width << endl;
    cout << "Height: " << image.size().height << endl;
    cv::cvtColor(image, greyMat, cv::COLOR_BGR2GRAY);

    // lmabda to convert greyMat to array
    auto mat_to_vec = [](cv::Mat mat) -> std::vector<uchar> {
        std::vector<uchar> array;
        if (mat.isContinuous()) {
            array.assign(mat.data, mat.data + mat.total()*mat.channels());
        } else {
            for (int i = 0; i < mat.rows; ++i) {
                array.insert(array.end(), mat.ptr<uchar>(i), mat.ptr<uchar>(i)+mat.cols*mat.channels());
            }
        }
        return array;
    };

    // print original fb buffer
    std::cout << "Original Framebuffer" << std::endl;
    std::vector<uchar> frame_buffer = mat_to_vec(greyMat);
    for (auto& pixel : frame_buffer) printf("%x ", pixel);
    std::cout << std::endl;

    std::cout << std::endl;

    // convert original fb to packed fb according
    // to IT8951 protocol and print original fb buffer
    std::cout << "Packed IT8951 Framebuffer" << std::endl;
    // this might be overflow, but for simplicity this shoudl be good
    std::vector<uint8_t> packed_buffer;
    for (size_t i = 0; i < frame_buffer.size(); i+=4){
        packed_buffer.push_back( (frame_buffer[i+2] >> 4) | 
                                 (frame_buffer[i+3] & 0xF0) );
        packed_buffer.push_back( (frame_buffer[i] >> 4) | 
                                 (frame_buffer[i+1] & 0xF0) );
    }
    for (auto& pixel : packed_buffer) printf("%x ", pixel);
    std::cout << std::endl;

    // display both image 
    // String window_original = "Original Image"; //Name of the windo
    // namedWindow(window_original); // Create a window
    // imshow(window_original, image); // Show our image inside the created window.
    
    // String window_grey = "Grey Image"; //Name of the windo
    // namedWindow(window_grey); // Create a window
    // imshow(window_grey, greyMat); // Show our image inside the created window.
    
    // waitKey(0); // Wait for any keystroke in the window

    // destroyWindow(window_original); //destroy the created window
    // destroyWindow(window_grey); //destroy the created window

    return 0;
}