#include <opencv2/opencv.hpp>
#include <iostream>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.hpp" 
#include <jsoncpp/json/json.h>
#include <fstream>

using namespace std;
using namespace cv;
using namespace camicasa;


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

    // obtain frame information
	int frameWidth = vidCapture.get(CAP_PROP_FRAME_WIDTH);
	int frameHeight = vidCapture.get(CAP_PROP_FRAME_HEIGHT);
    int fps = vidCapture.get(CAP_PROP_FPS);

    cout << "Frame width: " << frameWidth << "\n";
    cout << "Frame height: " << frameHeight << "\n";
    cout << "FPS: " << fps << "\n\n";

    // reset folders to store retrieved data
    system("rm -r videos");
    system("rm -r logos");
    mkdir("videos", 0777);
    mkdir("logos", 0777);


    // minimum time in seconds for a segment to be considered a program
    int minimumTime = 60;
    int timestamp = 0;

    TVChannel *channel = new TVChannel(frameWidth, frameHeight, fps, minimumTime);

    /**
     **** LOGO DETECTION ****
     */
    // variables used to control states
    bool logoAlreadyFound = false;

    Logo currentLogo;

    // variables used to store frames
    Mat currentFrame;
    Mat previousFrame;
    Mat operationBitwise;

    /**
     **** SEGMENT CLASSIFICATION ****
     */
    // variables used to control states
    bool alreadyBlack = false;
    bool startSegment = false;

    Segment currentSegment;


    // json variables
    Json::Value json;
    Json::Value logoVec(Json::arrayValue);
    Json::Value segmentVec(Json::arrayValue);

    puts("**** LOGO DETECTION AND SEGMENT CLASSIFICATION ****");

    // first reading of all frames to detect logos and classify segments
    while (vidCapture.isOpened()){
        Mat frame;
        vidCapture >> frame;
        if (frame.empty()){
            if (startSegment)
            {
                currentSegment.endTimestamp = timestamp;

                channel->addSegment(currentSegment);
                logoAlreadyFound = false;
                
                // reset currentSegment
                currentSegment.id++;
                currentSegment.type = AD;
                currentSegment.logoAssociated = -1;
            }
            break;
        }

        timestamp = vidCapture.get(CAP_PROP_POS_MSEC);

        if (!alreadyBlack && screenThresholdDetection(frame))
        {
            if (startSegment)
            {
                currentSegment.endTimestamp = timestamp;

                channel->addSegment(currentSegment);

                startSegment = false;
                logoAlreadyFound = false;

                // reset currentSegment
                currentSegment.id++;
                currentSegment.type=AD;
                currentSegment.logoAssociated= -1;
            }

            alreadyBlack = true;
        }
        else
        {
            if (alreadyBlack && !screenThresholdDetection(frame))
                alreadyBlack = false;

            if (!startSegment)
            {
                currentSegment.startTimestamp = timestamp;
                startSegment = true;
            }

            if (alreadyBlack)
                continue;

            if (!logoAlreadyFound && currentSegment.type != PROGRAM)
            for (Logo logo : channel->getLogos())
                if (channel->findPatternLogo(frame, logo)){
                    cout << "Segment " << currentSegment.id << " changed to program because a logo was found\n";
                    currentSegment.type = PROGRAM;
                    logoAlreadyFound = true;
                    break;
                }

            // change segment type after a certain time has passed
            int passedTime = (timestamp - currentSegment.startTimestamp) / 1000;
            if (currentSegment.type != PROGRAM && channel->hasMinimumTimePassed(passedTime)){
                cout << "Segment " << currentSegment.id << " changed to program because of time\n";
                currentSegment.type = PROGRAM;
            }


            if (!logoAlreadyFound)
            {
                // reduce the number of frames to search for a logo if none found
                int frameNumber = vidCapture.get(CAP_PROP_POS_FRAMES);
                if (frameNumber % fps == 0){

                if (previousFrame.empty())
                {
                    currentFrame = frame.clone();

                    previousFrame = currentFrame.clone();
                    operationBitwise = previousFrame.clone();
                    continue;
                }

                previousFrame = currentFrame.clone();
                currentFrame = frame.clone();

                bitwise_and(operationBitwise, currentFrame, operationBitwise);

                Mat operationBitwiseGray;
                cvtColor(operationBitwise, operationBitwiseGray, COLOR_BGR2GRAY);

                channel->checkForSaturatedCorners(operationBitwiseGray, CHECK_SMALLER, 5);

                channel->findLogo(currentFrame, operationBitwise, currentLogo);
               
                // no logo found
                if (currentLogo.screenCorner == NONE)
                    continue;

                logoAlreadyFound = true;
                cout << "Segment " << currentSegment.id << " found a logo at the " << stringifyScreenCorner(currentLogo.screenCorner) << "!\n";

                stringstream logoWriter;
                logoWriter << "logos/logo" << currentLogo.id << ".jpg";
                imwrite(logoWriter.str(), currentLogo.image);

                channel->addLogo(currentLogo);
                 
                Json::Value logoJson;
                
                logoJson["id"] = currentLogo.id;
                logoJson["x"] = currentLogo.x;
                logoJson["y"] = currentLogo.y;
                logoJson["width"] = currentLogo.width;
                logoJson["height"] = currentLogo.height;
                logoJson["corner"] = stringifyScreenCorner(currentLogo.screenCorner);

                logoVec.append(logoJson);

                if (currentSegment.type != PROGRAM){
                    currentSegment.type = PROGRAM;
                    cout << "Segment " << currentSegment.id << " changed to program because a logo was detected\n";
                }

                currentSegment.logoAssociated = currentLogo.id;

                currentLogo.id++;

                }
            }

            if (!channel->hasStillFrames())
            {
                operationBitwise = previousFrame;

                if (logoAlreadyFound){
                    currentSegment.endTimestamp = timestamp;
                    logoAlreadyFound = false;
                }

            }
        }
    }

    // reading video again, but this time to write frames in disk relative to each segment, 
    // trimming start and end timestamps if necessary

    vidCapture.open(argv[1]);

    if (!vidCapture.isOpened())
    {
        puts("Error opening video stream or file");
        // Release the video capture object
        vidCapture.release();
        cv::destroyAllWindows();
        return 0;
    }

    puts("");
    puts("**** SEGMENT TRIMMING ****");

    for (int i = 0; i < channel->getSegments().size(); i++)
    {
        Segment segment = channel->getSegments()[i];

        if (segment.type != PROGRAM)
            continue;

        int time = 0;
        int newStart = segment.startTimestamp;
        int newEnd = segment.startTimestamp;
        bool foundNewStart = false;

        int frameStart = (segment.startTimestamp/1000) * fps;
        vidCapture.set(CAP_PROP_POS_FRAMES, frameStart);

        while (vidCapture.isOpened() && time < segment.endTimestamp){
            Mat frame;
            vidCapture >> frame;

            if (frame.empty())
            {
                puts("Video has been disconnected");
                break;
            }

            time = vidCapture.get(CAP_PROP_POS_MSEC);

            for (Logo logo : channel->getLogos())
                if (channel->findPatternLogo(frame, logo))
                {
                    if (!foundNewStart)
                    {
                        newStart = time;
                        foundNewStart = true;
                    }

                    newEnd = time;
                    break;
                }
            
        }

        channel->updateSegment(segment.id, newStart, newEnd);
        
    }


    vidCapture.open(argv[1]);
    timestamp = 0;

    for (int i = 0; i < channel->getSegments().size(); i++)
    {
        Segment segment = channel->getSegments().at(i);

        stringstream segmentWriter;
        segmentWriter << "videos/segment" << segment.id << ".mp4";

        VideoWriter writer(segmentWriter.str(), VideoWriter::fourcc('a', 'v', 'c', '1'), fps, Size(frameWidth, frameHeight));

        if (!writer.isOpened()){
            puts("Error opening video writer");
            writer.release();
            continue;
        }

        while (vidCapture.isOpened() && timestamp < segment.endTimestamp)
        {
            Mat frame;
            vidCapture >> frame;

            if (frame.empty()) {
                puts("Video has been disconnected");
                break;
            }

            timestamp = vidCapture.get(CAP_PROP_POS_MSEC);
            
            if (timestamp >= segment.startTimestamp && timestamp <= segment.endTimestamp)
                writer.write(frame);
        }

        Json::Value segmentJson;

        segmentJson["id"] = segment.id;
        segmentJson["startTimestamp"] = segment.startTimestamp;
        segmentJson["endTimestamp"] = segment.endTimestamp;
        segmentJson["type"] = stringifyTVChannelType(segment.type);
        segmentJson["logoFound"] = segment.logoAssociated;

        segmentVec.append(segmentJson);
        
        puts("");
        cout << "Start " << stringifyTVChannelType(segment.type) << " of segment " << segment.id << " at " << formatTimestamp(segment.startTimestamp) << "\n";
        if (segment.logoAssociated != -1)
            cout << "Logo " << segment.logoAssociated << " was found in this segment\n";
        cout << "End of segment " << segment.id << " at " << formatTimestamp(segment.endTimestamp) << "\n";

        writer.release();
    }

    json["logos"] = logoVec;
    json["segments"] = segmentVec;


    // writing thee json object

    if (!json.empty()){
        ofstream file;
        file.open("json.json");

        Json::StyledWriter jsonWriter;
        file << jsonWriter.write(json);

        file.close();
    }

    delete channel;
    vidCapture.release();
    cv::destroyAllWindows();

    return 0;
}