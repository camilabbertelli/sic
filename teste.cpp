#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <list>
#include "utils.hpp"

using namespace std;
using namespace cv;
using namespace camicasa;


int main(int argc, char** argv)
{
    Mat croppedOriginal;
    Mat croppedCompare;
    Mat golden;

    list<Mat> elements;

    croppedOriginal = imread("testeLogo/croppedOriginal.jpg");
    croppedCompare = imread("testeLogo/croppedCompare.jpg");
    Mat frame = imread("testeLogo/frame1.jpg");
    Mat logoImage = imread ("logos/logo1.jpg");
    Mat fake = imread("testeLogo/fake.jpg");
    Mat aaaa = imread("testeLogo/aaaa.jpg");
    Mat bbbb = imread("testeLogo/bbbb.jpg");

    Logo logo;

    logo.screenCorner = TOP_LEFT;
    logo.image = logoImage.clone();
    
    logo.height = 35;
    logo.width = 66;
    logo.x = 94;
    logo.y = 33;

    TVChannel *channel = new TVChannel(1280, 720, 30);

    if (channel->findPatternLogo(frame, logo))
        cout << "Nice frame!\n";
    if (channel->findPatternLogo(fake, logo))
        cout << "Ohh :( fake!\n";
    if (channel->findPatternLogo(aaaa, logo))
        cout << "Ohh :( aaaa!\n";
    if (channel->findPatternLogo(bbbb, logo))
        cout << "Ohh :( bbbb!\n";

    return 0;
}