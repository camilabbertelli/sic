#ifndef _UTILS_
#define _UTILS_

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

namespace camicasa
{
    #define CHECK_SMALLER 1
    #define CHECK_BIGGER  0 
    bool screenThresholdDetection(Mat& frame, int operation = CHECK_SMALLER, int threshold = 1);
    bool hasSaturatedCorners(Mat& frame, vector<Point> corners, int frameWidth, int frameHeight, int operation = CHECK_SMALLER, int threshold = 1);
    string formatTimestamp(int duration);
    vector<Point> getCornersScreen(int frameWidth, int frameHeight);
}

#endif