#include "utils.hpp"

using namespace camicasa;

/**
    @brief The function camicasa::screenThresholdDetection is a method for detecting if the average of pixels(BGR) falls under/above a certain threshold
    @param frame image frame input cv::Mat
    @param operation defines which comparison to execute (camicasa::CHECK_SMALLER, camicasa::CHECK_BIGGER)
    @param threshold optional threshold to compare
    @returns returns true if frame presented has average pixels that fall under/above the specified threshold
*/
bool camicasa::screenThresholdDetection(Mat &frame, int operation /*CHECK_SMALLER*/, int threshold /*1*/)
{
    Scalar mean = cv::mean(frame);
    int avg = (mean(0) + mean(1) + mean(2)) / 3;
    if (operation)
        return (avg < threshold);

    return (avg > threshold);
}

/**
    @brief The function camicasa::hasSaturatedCorners is a method for detecting if any corner on the screen has their average pixels under/above the specified threshold 
    @param frame image frame input cv::Mat
    @param corners vector of cv::Points that define all four corners of the screen 
    @param frameWidth number of horizontal pixels of the given frame
    @param frameHeight number of vertical pixels of the given frame
    @param operation defines which comparison to execute (camicasa::CHECK_SMALLER, camicasa::CHECK_BIGGER)
    @param threshold optional threshold to compare
    @returns returns true if any of the four corners have saturated pixels under/above the specified threshold
    @note See more in getCornersScreen(int frameWidth, int frameHeight)
*/
void camicasa::hasSaturatedCorners(Mat &frame, vector<Point> corners, vector<int>& frameStillCount, int frameWidth, int frameHeight, int operation /*CHECK_SMALLER*/, int threshold /*1*/)
{

    // get the four cropped frames representing the corners
    Mat topLeft = frame(Range(0, corners.at(0).y), Range(0, corners.at(0).x));
    Mat topRight = frame(Range(0, corners.at(1).y), Range(frameWidth - corners.at(1).x, frameWidth));
    Mat bottomLeft = frame(Range(frameHeight - corners.at(2).y, frameHeight), Range(0, corners.at(2).x));
    Mat bottomRight = frame(Range(0, corners.at(3).y), Range(frameWidth - corners.at(3).x, frameWidth));

    frameStillCount[0] = (screenThresholdDetection(topLeft, operation, threshold)) ? 0 : frameStillCount[0] + 1;
    frameStillCount[1] = (screenThresholdDetection(topRight, operation, threshold)) ? 0 : frameStillCount[1] + 1;
    frameStillCount[2] = (screenThresholdDetection(bottomLeft, operation, threshold)) ? 0 : frameStillCount[2] + 1;
    frameStillCount[3] = (screenThresholdDetection(bottomRight, operation, threshold)) ? 0 : frameStillCount[3] + 1;
}

/**
    @brief The function camicasa::formatTimestamp is a method converting the duration in milliseconds for human reading
    @param duration in milliseconds
    @returns returns string with the format h:m:s
*/
string camicasa::formatTimestamp(int duration)
{
    int hour = (int)((duration / (1000 * 60 * 60)) % 24);
    int mins = (int)((duration / (1000 * 60)) % 60);
    int secs = (int)(((duration / 1000) % 60));
    stringstream ss;
    ss << hour << ":" << mins << ":" << secs;
    return ss.str();
}

/**
    @brief The functions camicasa::getCornersScreen is a method for finding all four corners of the screen with some margin
    @param frameWidth number of horizontal pixels of the given frame
    @param frameHeight number of vertical pixels of the given frame
    @returns vector of cv::Points that define all four corners of the screen
 */
vector<Point> camicasa::getCornersScreen(int frameWidth, int frameHeight)
{
    vector<Point> vec;

    /*

    ---++++++---n
    --§++++++§--
    ++++++++++++
    --§++++++§--
    ---++++++---

    § : represents the points in this ficticious grid (Mat image)
    - : represents points of interest to check for logos

    */

    int widthFactor = 7;
    int heightFactor = 5;

    int x = 0 + frameWidth / widthFactor;
    int y = 0 + frameHeight / heightFactor;
    int w = frameWidth - x;
    int z = frameHeight - y;

    vec.push_back(Point(x, y));
    vec.push_back(Point(w, y));
    vec.push_back(Point(x, z));
    vec.push_back(Point(w, z));

    return vec;
}


/**
    @brief The functions camicasa::convertBinarizedFrame is a method for inverting ou maintaining the input image binarization 
    considering the premise that the most abundant color is not the focus of the frame
    @param[in] input binarized image frame input cv::Mat
    @param[out] output binarized image frame output cv::Mat with mode of choice
    @param mode optional mode that describes which color should the impoortant information be (default IMPORTANT_WHITE)
 */
void camicasa::convertBinarizedFrame(Mat& input, Mat& output, int mode){
    output = input.clone();

    bool mostWhite = false;

    Scalar mean = cv::mean(input);
    if (mean(0) > 127)
        mostWhite = true;

    if ((mostWhite && mode == IMPORTANT_IN_WHITE)
    || (!mostWhite && mode == IMPORTANT_IN_BLACK))
        output = ~input;
}

/**
    @brief The functions camicasa::cropLogo is a method for finding the logo present and cropping the input frame
    @param[in] inputOriginal original image frame input cv::Mat
    @param[in] inputBitwise compared image frame with bitwise_and operation input cv::Mat
    @param[out] output image frame output cv::Mat with only the logo
 */
void camicasa::cropLogo(Mat& inputOriginal, Mat& inputBitwise, Mat& output){
    Mat bitwise;
    bitwise_and(inputOriginal, inputBitwise, bitwise);

    Mat bitwise_gray;
    cvtColor(bitwise, bitwise_gray, COLOR_BGR2GRAY);

    Mat otsuThresh;
    threshold(bitwise_gray, otsuThresh, 0, 255, THRESH_OTSU);
    convertBinarizedFrame(otsuThresh, otsuThresh, IMPORTANT_IN_WHITE);
    imshow("Otsu", otsuThresh);

    Mat opening;
    morphOperation(otsuThresh, otsuThresh);

    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;

    int margin = 15;

    bool foundStartX = false;
    bool foundStartY = false;

    vector<vector<Point>> possibleSquares;

    for (int row = 0; row < inputOriginal.rows; row++){
        for (int column = 0; column < inputOriginal.cols; column++){
            int value = (int)otsuThresh.at<uchar>(row, column);

            // FIXME: tolerancia para pixels pretos/brancos

            if (value && row < startY){
                startY = row;
                foundStartY = true;
            }

            if (value && column < startX){
                startX = column;
                foundStartX = true;
            }

            if (!foundStartX && value){
                startX = column;
                foundStartX = true;
            }

            if (foundStartX && value && column >= endX){
                endX = column;
            }

            if (!foundStartY && value){
                startY = row;
                foundStartY = true;
            }

            if (foundStartY && value && row >= endY){
                endY = row;
            }
        }
    }

    // margin calculation, making sure it doesn't overflow
    startX = (startX - margin >= 0) ? startX - margin : 0;
    startY = (startY - margin >= 0) ? startY - margin : 0;
    endX = (endX + margin < inputOriginal.cols) ? endX + margin : inputOriginal.cols - 1;
    endY = (endY + margin < inputOriginal.rows) ? endY + margin : inputOriginal.rows - 1;

    output = inputOriginal(Range(startY, endY), Range(startX, endX));
}


/**
    @brief The functions camicasa::morphOperation is a method for removing dots and loose pixels in the input frame
    utilizing the cv::morphologyEx function
    @param[in] input image frame input cv::Mat
    @param[out] output image frame output cv::Mat
 */
void camicasa::morphOperation(Mat& input, Mat& output){
    // Create a structuring element
    int morph_size = 1;
    Mat element1 = getStructuringElement(
        MORPH_CROSS,
        Size(2 * morph_size + 1,
             2 * morph_size + 1));

    morph_size = 2;
    Mat element2 = getStructuringElement(
        MORPH_RECT,
        Size(2 * morph_size + 1,
             2 * morph_size + 1));
  
    // Transformations
    Mat aux;
    erode(input, aux, element1);
    dilate(aux, output, element2);

    // Display the image
    imshow("Aux", aux);
    imshow("Output", output);
}