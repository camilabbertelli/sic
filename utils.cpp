#include "utils.hpp"

using namespace camicasa;

camicasa::TVChannel::TVChannel(int frameWidth,  int frameHeight, int fps, int minimumTime) {
    this->frameWidth = frameWidth;
    this->frameHeight = frameHeight;
    this->fps = fps;
    this->minimumTime = minimumTime;
    this->frameStillCount = {0, 0, 0, 0};
    this->populateCorners(); 
}

camicasa::TVChannel::~TVChannel() {
    corners.clear();
    segments.clear();
    frameStillCount.clear();
}

void camicasa::TVChannel::populateCorners(){
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

    this->corners.push_back(Point(x, y));
    this->corners.push_back(Point(w, y));
    this->corners.push_back(Point(x, z));
    this->corners.push_back(Point(w, z));
}

void camicasa::TVChannel::addSegment(Segment segment){
    this->segments.push_back(segment);
}

void camicasa::TVChannel::addLogo(Logo logo)
{
    this->logos.push_back(logo);
}

vector<Point> camicasa::TVChannel::getCorners() {
    return this->corners;
}

vector<int> camicasa::TVChannel::getFrameStillCount()
{
    return this->frameStillCount;
}

vector<Segment> camicasa::TVChannel::getSegments() {
    return this->segments;
}

vector<Logo> camicasa::TVChannel::getLogos() {
    return this->logos;
}

bool camicasa::screenThresholdDetection(Mat &frame, ComparisonOperation operation /*CHECK_SMALLER*/, int threshold /*1*/)
{
    Scalar mean = cv::mean(frame);

    int avg = (mean(0) + mean(1) + mean(2)) / (frame.channels());

    if (operation == CHECK_SMALLER)
        return (avg <= threshold);

    return (avg >= threshold);
}

void camicasa::TVChannel::checkForSaturatedCorners(Mat &frame, ComparisonOperation operation /*CHECK_SMALLER*/, int threshold /*1*/)
{
    // get the four cropped frames representing the corners

    Mat topLeft = frame(Range(0, this->corners.at(TOP_LEFT).y), Range(0, this->corners.at(TOP_LEFT).x));
    Mat topRight = frame(Range(0, this->corners.at(TOP_RIGHT).y), Range(this->frameWidth - this->corners.at(TOP_RIGHT).x, this->frameWidth));
    Mat bottomLeft = frame(Range(this->frameHeight - this->corners.at(BOTTOM_LEFT).y, this->frameHeight), Range(0, this->corners.at(BOTTOM_LEFT).x));
    Mat bottomRight = frame(Range(0, this->corners.at(BOTTOM_RIGHT).y), Range(this->frameWidth - this->corners.at(BOTTOM_RIGHT).x, this->frameWidth));

    this->frameStillCount[TOP_LEFT] = (screenThresholdDetection(topLeft, operation, threshold)) ? 0 : this->frameStillCount[TOP_LEFT] + 1;
    this->frameStillCount[TOP_RIGHT] = (screenThresholdDetection(topRight, operation, threshold)) ? 0 : this->frameStillCount[TOP_RIGHT] + 1;
    this->frameStillCount[BOTTOM_LEFT] = (screenThresholdDetection(bottomLeft, operation, threshold)) ? 0 : this->frameStillCount[BOTTOM_LEFT] + 1;
    this->frameStillCount[BOTTOM_RIGHT] = (screenThresholdDetection(bottomRight, operation, threshold)) ? 0 : this->frameStillCount[BOTTOM_RIGHT] + 1;
}

void camicasa::TVChannel::findLogo(Mat& inputOriginal, Mat& inputBitwise, Logo& logo){
    Mat croppedOriginal;
    Mat croppedBitwise;
    ScreenCorner corner = NONE;
    
    if (this->frameStillCount[TOP_LEFT] >= this->minimumTime)
    {
        corner = TOP_LEFT;
        croppedOriginal = inputOriginal(Range(0, this->corners.at(corner).y), Range(0, this->corners.at(corner).x));
        croppedBitwise = inputBitwise(Range(0, this->corners.at(corner).y), Range(0, this->corners.at(corner).x));    
    }
    else if (this->frameStillCount[1] >= this->minimumTime)
    {
        corner = TOP_RIGHT;
        croppedOriginal = inputOriginal(Range(0, this->corners.at(corner).y), Range(this->frameWidth - this->corners.at(corner).x, this->frameWidth));
        croppedBitwise = inputBitwise(Range(0, this->corners.at(corner).y), Range(this->frameWidth - this->corners.at(corner).x, this->frameWidth));
    }
    else if (this->frameStillCount[BOTTOM_LEFT] >= this->minimumTime)
    {
        corner = BOTTOM_LEFT;
        croppedOriginal = inputOriginal(Range(this->frameHeight - this->corners.at(corner).y, this->frameHeight), Range(0, this->corners.at(corner).x));
        croppedBitwise = inputBitwise(Range(this->frameHeight - this->corners.at(corner).y, this->frameHeight), Range(0, this->corners.at(corner).x));
    }
    else if (this->frameStillCount[BOTTOM_RIGHT] >= this->minimumTime)
    {
        corner = BOTTOM_RIGHT;
        croppedOriginal = inputOriginal(Range(this->frameHeight - this->corners.at(corner).y, this->frameHeight), Range(this->frameWidth - this->corners.at(corner).x, this->frameWidth));
        croppedBitwise = inputBitwise(Range(this->frameHeight - this->corners.at(corner).y, this->frameHeight), Range(this->frameWidth - this->corners.at(corner).x, this->frameWidth));
    }

    logo.screenCorner = corner;
    
    // no logo found
    if (corner == NONE)
        return;

    cropLogo(croppedOriginal, croppedBitwise, logo);
}

bool camicasa::TVChannel::hasStillFrames(){
    int sum = 0;

    for (int i = 0; i < this->frameStillCount.size(); i++)
        sum += this->frameStillCount.at(i);

    return sum;
}

bool camicasa::TVChannel::hasMinimumTimePassed(int time){
    return time >= this->minimumTime;
}

bool camicasa::TVChannel::findPatternLogo(Mat &input, Logo& pattern)
{
    // crop the same area as the pattern
    Mat cropped;
    cropped = input(Range(pattern.y, pattern.y + pattern.height), Range(pattern.x, pattern.x + pattern.width));

    // remove color factor, since it's not relevant
    Mat inputGray;
    cvtColor(cropped, inputGray, COLOR_BGR2GRAY);

    Mat patternGray;
    cvtColor(pattern.image, patternGray, COLOR_BGR2GRAY);

    // // blur the image, so the edge detection (canny) works better
    Mat inputBlur;
    GaussianBlur(inputGray, inputBlur, Size(5, 5), 0);

    Mat patternBlur;
    GaussianBlur(patternGray, patternBlur, Size(5, 5), 0);

    // canny edge detection
    Mat inputEdges;
    Canny(inputBlur, inputEdges, 50, 60, 3, true);

    Mat patternEdges;
    Canny(patternBlur, patternEdges, 50, 100, 3);

    Mat bitwise;
    bitwise_and(patternEdges, inputEdges, bitwise);
    
    return (screenThresholdDetection(bitwise, CHECK_BIGGER, 15));
}

string camicasa::formatTimestamp(int duration)
{
    int hour = (int)((duration / (1000 * 60 * 60)) % 24);
    int mins = (int)((duration / (1000 * 60)) % 60);
    int secs = (int)(((duration / 1000) % 60));
    stringstream ss;
    ss << hour << ":" << mins << ":" << secs;
    return ss.str();
}

void camicasa::convertBinarizedFrame(Mat &input, Mat &output, BinarizationMode mode)
{
    output = input.clone();

    bool mostWhite = false;

    Scalar mean = cv::mean(input);
    if (mean(0) > 127)
        mostWhite = true;

    if ((mostWhite && (mode == HIGHLIGHT_IN_WHITE)) || (!mostWhite && (mode == HIGHLIGHT_IN_BLACK)))
        output = ~input;
}

void camicasa::cropLogo(Mat &inputOriginal, Mat &inputBitwise, Logo& output)
{
    Mat bitwise;
    bitwise_and(inputOriginal, inputBitwise, bitwise);

    Mat bitwise_gray;
    cvtColor(bitwise, bitwise_gray, COLOR_BGR2GRAY);

    Mat otsuThresh;
    threshold(bitwise_gray, otsuThresh, 0, 255, THRESH_OTSU);
    convertBinarizedFrame(otsuThresh, otsuThresh, HIGHLIGHT_IN_WHITE);

    morphOperation(otsuThresh, otsuThresh);

    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;

    int margin = 5;

    bool foundStartX = false;
    bool foundStartY = false;

    for (int row = 0; row < inputOriginal.rows; row++)
    {
        for (int column = 0; column < inputOriginal.cols; column++)
        {
            int value = (int)otsuThresh.at<uchar>(row, column);

            if (value && row < startY)
            {
                startY = row;
                foundStartY = true;
            }

            if (value && column < startX)
            {
                startX = column;
                foundStartX = true;
            }

            if (!foundStartX && value)
            {
                startX = column;
                foundStartX = true;
            }

            if (foundStartX && value && column >= endX)
                endX = column;

            if (!foundStartY && value)
            {
                startY = row;
                foundStartY = true;
            }

            if (foundStartY && value && row >= endY)
                endY = row;
        }
    }

    // margin calculation, making sure it doesn't overflow
    startX = (startX - margin >= 0) ? startX - margin : 0;
    startY = (startY - margin >= 0) ? startY - margin : 0;
    endX = (endX + margin < inputOriginal.cols) ? endX + margin : inputOriginal.cols - 1;
    endY = (endY + margin < inputOriginal.rows) ? endY + margin : inputOriginal.rows - 1;

    output.x = startX;
    output.y = startY;
    output.width = endX - startX;
    output.height = endY - startY;
    output.image = inputBitwise(Range(startY, endY), Range(startX, endX));
}

void camicasa::morphOperation(Mat &input, Mat &output)
{
    // create structuring elements (more weight on dilate than erode)
    int morph_size = 2;
    Mat element = getStructuringElement(
        MORPH_RECT,
        Size(2 * morph_size + 1,
             2 * morph_size + 1));

    // transformations
    Mat aux;
    erode(input, aux, element);
    dilate(aux, output, element);
}

double camicasa::distancePointRectangle(Point point, vector<Point> rectangle)
{

    Point rect_min = rectangle.at(0);
    Point rect_max = rectangle.at(1);

    if (point.x < rect_min.x)
    {
        if (point.y < rect_min.y)
            return distancePoints(point, rect_min);
        if (point.y > rect_max.y)
            return distancePoints(point, Point(rect_min.x, rect_max.y));

        return abs(point.x - rect_min.x);
    }

    if (point.x > rect_max.x)
    {

        if (point.y < rect_min.y)
            return distancePoints(point, Point(rect_max.x, rect_min.y));
        if (point.y > rect_max.y)
            return distancePoints(point, rect_max);

        return abs(point.x - rect_max.x);
    }

    // point.x <= rect_max.x (potentially inside)
    if (point.y < rect_min.y)
        return abs(point.y - rect_min.y);
    if (point.y > rect_max.y)
        return abs(point.y - rect_max.y);

    // point is inside rectangle
    return 0;
}

double camicasa::distancePoints(Point a, Point b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

const char* camicasa::stringifyTVChannelType(TVChannelType item){

    const char *convert_enum[] = {
        stringify(AD),
        stringify(PROGRAM)};

    return convert_enum[item];
}

const char* camicasa::stringifyScreenCorner(ScreenCorner item){

    const char *convert_enum[] = {
        stringify(TOP_LEFT),
        stringify(TOP_RIGHT),
        stringify(BOTTOM_LEFT),
        stringify(BOTTOM_RIGHT)
    };

    return convert_enum[item];
}