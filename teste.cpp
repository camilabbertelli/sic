#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "utils.hpp"

using namespace std;
using namespace cv;
using namespace camicasa;


int main(int argc, char** argv)
{
    VideoCapture vid_capture(argv[1]);

    if (!vid_capture.isOpened()){
		cout << "Error opening video stream or file\n";
		// Release the video capture object
		vid_capture.release();
		cv::destroyAllWindows();
		return 0;
	}

    // Obtain frame size information using get() method
	int frame_width = vid_capture.get(CAP_PROP_FRAME_WIDTH);
	int frame_height = vid_capture.get(CAP_PROP_FRAME_HEIGHT);
    int fps = vid_capture.get(CAP_PROP_FPS);

    cout << "Frame width: " << frame_width << "\n";
    cout << "Frame height: " << frame_height << "\n";
    cout << "FPS: " << fps << "\n";

    bool already_in_all_black = false;
    vector<int> timestamps;
    Mat frame;
    Mat currentFrame;
    Mat previousFrame;
    Mat operationCompare;

    vector<Point> corners = getCornersScreen(frame_width, frame_height);
    vector<int> frameStillCount = {0, 0, 0, 0}


    while (vid_capture.isOpened()){
        // TODO: detect sic logo to classify news/program vs ads
        // TODO: time all that and write in txt file/stdout

        
        bool success = vid_capture.read(frame);
        vid_capture >> frame;
        if(frame.empty())
        {
            puts("Video has been disconnected");
            break;
        }
        
        if (previousFrame.empty()){
            currentFrame = frame.clone();

            previousFrame = currentFrame.clone();
            operationCompare = previousFrame.clone();
            continue;
        }

        previousFrame = currentFrame.clone();
        currentFrame = frame.clone();

        bitwise_and(operationCompare, currentFrame, operationCompare);

        if (hasSaturatedCorners(operationCompare, corners, frame_width, frame_height, CHECK_SMALLER, 5)){
            puts("Resetting operationCompare...");
            operationCompare = previousFrame;
        }

        imshow("Frame", operationCompare);
        /*
        bitwise_and(previousFrame, currentFrame, differenceBitwiseCurrent);

        if (differenceBitwisePrevious.empty())
            differenceBitwisePrevious = differenceBitwiseCurrent.clone();

        bitwise_and(differenceBitwisePrevious, currentFrame, difference);

        //previousFrame = differenceBitwise.clone();
        differenceBitwisePrevious = differenceBitwiseCurrent.clone();

        imshow("Bitwise image", difference);
        */

/*
        cvtColor(previousFrame, grayscalePrevious, COLOR_BGR2GRAY);
        cvtColor(currentFrame, grayscaleCurrent, COLOR_BGR2GRAY);

        for (int row = 0; row < grayscaleCurrent.rows; row++)
        for (int column = 0; column < grayscaleCurrent.cols; column++)
        {
            int valueCurrent = (int)grayscaleCurrent.at<uchar>(row, column);
            int valuePrevious = (int)grayscalePrevious.at<uchar>(row, column);

            int valueDiffer = abs(valueCurrent - valuePrevious);

            uchar value = (valueDiffer < 10) ? (uchar)255 : (uchar)0;

            difference.at<uchar>(row, column) = value;
        }

        imshow("Frame", difference);
*/
        int key = waitKey(0);
		if (key == 'q')
		{
			cout << "Quitting the video\n";
			break;
		}
     }

    vid_capture.release();
    cv::destroyAllWindows();

    return 0;
}