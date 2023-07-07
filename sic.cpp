#include <opencv2/opencv.hpp>
#include <iostream>
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


    // variables used to control states

    bool alreadyBlack = false;
    bool startSegment = false;

    segment currentSegment;

    Mat frame;
    int timestamp;

    list<segment> segments;

    // Initialize writer object
    stringstream videoName;
    videoName << "videos/segment" << currentSegment.id << ".mp4";
	VideoWriter writer;
    writer.open(videoName.str(), VideoWriter::fourcc('a', 'v', 'c', '1'), fps , Size(frameWidth, frameHeight));
    
    while (vidCapture.isOpened()){
        // TODO: detect all black frames
        // TODO: detect sic logo to classify news/program vs ads
        // TODO: time all that and write in txt file/stdout

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
            cout << "\nStart of segment " << currentSegment.id << " at " << formatTimestamp(timestamp) << "\n";
            alreadyBlack = false;
            startSegment = true;
            currentSegment.startTimestamp = timestamp;
        } else
            startSegment = true;
        
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