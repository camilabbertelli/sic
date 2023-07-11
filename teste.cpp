#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "utils.hpp"

using namespace std;
using namespace cv;
using namespace camicasa;


int main(int argc, char** argv)
{
    Mat croppedOriginal;
    Mat croppedCompare;
    Mat logo;

    croppedOriginal = imread("logos/croppedOriginal.jpg");
    croppedCompare = imread("logos/croppedCompare.jpg");

    cropLogo(croppedOriginal, croppedCompare, logo);

    imshow("Original", croppedOriginal);
    imshow("Compared", croppedCompare);
    imshow("Final logo", logo);

    waitKey(0);
    cv::destroyAllWindows();

    return 0;
}