#include <opencv2/opencv.hpp>
#include <iostream>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include "utils.hpp"

using namespace std;
using namespace cv;
using namespace camicasa;


struct segment {
    int id = 1;
    int startTimestamp = 0;
    int endTimestamp = 0;
};

int main(int argc, char** argv)
{
    VideoCapture vidCapture(argv[1]);

    if (!vidCapture.isOpened()){
		puts("Error opening video stream or file");
		// Release the video capture object
		vidCapture.release();
		cv::destroyAllWindows();
		return 0;
	}

    // Obtain frame size information using get() method
	int frameWidth = vidCapture.get(CAP_PROP_FRAME_WIDTH);
	int frameHeight = vidCapture.get(CAP_PROP_FRAME_HEIGHT);
    int fps = vidCapture.get(CAP_PROP_FPS);

    cout << "Frame width: " << frameWidth << "\n";
    cout << "Frame height: " << frameHeight << "\n";
    cout << "FPS: " << fps << "\n";

    system("rm -r videos");
    system("rm -r logos");
    mkdir("videos", 0777);
    mkdir("logos", 0777);

    // variables used to control states

    bool alreadyBlack = false;
    bool startSegment = false;

    segment currentSegment;

    Mat frame;
    int timestamp;

    Mat currentFrame;
    Mat previousFrame;
    Mat operationCompare;
    int logoCount = 1;
    bool logoFound = false;
    bool stillLogo = false;

    vector<Point> corners = getCornersScreen(frameWidth, frameHeight);
    vector<int> frameStillCount = {0, 0, 0, 0};

    list<segment> segments;

    // Initialize writer object
    stringstream videoName;
    videoName << "videos/segment" << currentSegment.id << ".mp4";
	VideoWriter writer;
    writer.open(videoName.str(), VideoWriter::fourcc('a', 'v', 'c', '1'), fps , Size(frameWidth, frameHeight));
    
    while (vidCapture.isOpened()){

        vidCapture >> frame;
        if(frame.empty())
        {
            if (startSegment){
                cout << "End of segment " << currentSegment.id << " at " << formatTimestamp(timestamp) << "\n";
                currentSegment.endTimestamp = timestamp;
                segments.push_back(currentSegment);
            }
            puts("Video has been disconnected");
            break;
        }
        

        timestamp = vidCapture.get(CAP_PROP_POS_MSEC);
        
        if (!alreadyBlack && screenThresholdDetection(frame)){
            
            if (startSegment){
                cout << "End of segment " << currentSegment.id << " at " << formatTimestamp(timestamp) << "\n";
                
                currentSegment.endTimestamp = timestamp;
                segments.push_back(currentSegment);
                
                videoName.str(std::string());
                videoName << "videos/segment" << ++currentSegment.id << ".mp4";
               
                writer.open(videoName.str(), VideoWriter::fourcc('a', 'v', 'c', '1'), fps , Size(frameWidth, frameHeight));
                
                startSegment = false;
            }

            alreadyBlack = true;
        } else if (alreadyBlack && !screenThresholdDetection(frame)){
            if (!startSegment){
                cout << "\nStart of segment " << currentSegment.id << " at " << formatTimestamp(timestamp) << "\n";
                startSegment = true;
                currentSegment.startTimestamp = timestamp;
            }
            alreadyBlack = false;
        } else{
            if (!startSegment){
                cout << "\nStart of segment " << currentSegment.id << " at " << formatTimestamp(timestamp) << "\n";
                startSegment = true;
                currentSegment.startTimestamp = timestamp;
            }

            int frameNumber = vidCapture.get(CAP_PROP_POS_FRAMES);

            if (frameNumber % fps == 0){

            if (previousFrame.empty()){
                currentFrame = frame.clone();

                previousFrame = currentFrame.clone();
                operationCompare = previousFrame.clone();
                continue;
            }

            previousFrame = currentFrame.clone();
            currentFrame = frame.clone();

            bitwise_and(operationCompare, currentFrame, operationCompare);

            hasSaturatedCorners(operationCompare, corners, frameStillCount, frameWidth, frameHeight, CHECK_SMALLER, 5);
            
            stringstream logoName;
            logoName.str("");
            Mat croppedOriginal;
            Mat croppedCompare;

            int minimumTime = 120;
            
            if (!stillLogo && frameStillCount[0] > minimumTime){
                logoFound = true;
                croppedOriginal = frame(Range(0, corners.at(0).y), Range(0, corners.at(0).x));
                croppedCompare = operationCompare(Range(0, corners.at(0).y), Range(0, corners.at(0).x));
                puts("Found potential logo top left!");
            } else if (!logoFound && frameStillCount[1] > minimumTime){
                logoFound = true;
                croppedOriginal = frame(Range(0, corners.at(1).y), Range(frameWidth - corners.at(1).x, frameWidth));
                croppedCompare = operationCompare(Range(0, corners.at(1).y), Range(frameWidth - corners.at(1).x, frameWidth));
                puts("Found potential logo top right!");
            } else if (!logoFound && frameStillCount[2] > minimumTime){
                logoFound = true;
                croppedOriginal = frame(Range(frameHeight - corners.at(2).y, frameHeight), Range(0, corners.at(2).x));
                croppedCompare = operationCompare(Range(frameHeight - corners.at(2).y, frameHeight), Range(0, corners.at(2).x));
                puts("Found potential logo bottom left!");
            } else if (!logoFound && frameStillCount[3] > minimumTime){
                logoFound = true;
                croppedOriginal = frame(Range(frameHeight - corners.at(3).y, frameHeight), Range(frameWidth - corners.at(3).x, frameWidth));
                croppedCompare = operationCompare(Range(frameHeight - corners.at(3).y, frameHeight), Range(frameWidth - corners.at(3).x, frameWidth));
                puts("Found potential logo bottom right!");
            }

            if (logoFound){
                logoFound = false;
                stillLogo = true;
                logoName << "logos/logo" << logoCount++ << ".jpg"; 
                
                Mat logo;
                cropLogo(croppedOriginal, croppedCompare, logo);
                
                imwrite(logoName.str(), logo);

                //imwrite("logos/croppedCompare.jpg", croppedCompare);
                //imwrite("logos/croppedOriginal.jpg", croppedOriginal);
            }

            if ((sum(frameStillCount))(0) == 0){
                operationCompare = previousFrame;
                stillLogo = false;
            }
            }
        }
        
        if (startSegment)
          writer.write(frame);

        /*imshow("Frame", frame);

        int key = waitKey(0);
		if (key == 'q')
		{
			puts("Quitting the video");
			break;
		}*/
     }

    vidCapture.release();
    writer.release();
    cv::destroyAllWindows();

    return 0;
}