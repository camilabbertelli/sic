#ifndef _UTILS_
#define _UTILS_

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

namespace camicasa
{
    #define CHECK_SMALLER 1
    #define CHECK_BIGGER  0
    #define IMPORTANT_IN_WHITE 1
    #define IMPORTANT_IN_BLACK 0
    
    bool screenThresholdDetection(Mat& frame, int operation = CHECK_SMALLER, int threshold = 1);
    void hasSaturatedCorners(Mat& frame, vector<Point> corners, vector<int>& frameStillCount, int frameWidth, int frameHeight, int operation = CHECK_SMALLER, int threshold = 1);
    string formatTimestamp(int duration);
    vector<Point> getCornersScreen(int frameWidth, int frameHeight);
    void convertBinarizedFrame(Mat& frame, Mat& output, int mode = IMPORTANT_IN_WHITE);
    void cropLogo(Mat& inputOriginal, Mat& inputBitwise, Mat& output);
    void openingMorphOperation(Mat& input, Mat& output);
}

#endif