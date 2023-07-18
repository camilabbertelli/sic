#ifndef _UTILS_
#define _UTILS_

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

#define stringify(name) #name

/// @brief namespace for all functions created by Camila Bertelli
namespace camicasa
{

    /// @brief enum camicasa::BinarizationMode used for defining whether the highlighted information will be white or black 
    enum BinarizationMode {HIGHLIGHT_IN_BLACK, HIGHLIGHT_IN_WHITE};
    /// @brief enum camicasa::ComparisonOperation used for defining which comparison to execute
    enum ComparisonOperation {CHECK_BIGGER, CHECK_SMALLER};
    /// @brief enum camicasa::ProgramType used for defining whether a section of a television program is an Ad segment or a News segment
    enum TVChannelType {AD, PROGRAM};
    /// @brief enum camicasa::ScreenCorner used for defining which corner a certain element is located on the screen
    enum ScreenCorner {TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, NONE};

    /// @brief struct camicasa::Segment containing information about a segment present in a television program
    struct Segment {
        int id = 1;
        int startTimestamp = 0;
        int endTimestamp = 0;
        TVChannelType type = AD;
        int logoAssociated = -1;
    };

    /// @brief struct camicasa::Logo containing information about logos found in a television program
    struct Logo{
        int id = 1;
        Mat image;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        ScreenCorner screenCorner = NONE;
    };

    /// @brief class camicasa::TVChannel containing pertinent information of the television programs
    class TVChannel {
    private:
        /// @brief number of horizontal pixels of the screen
        int frameWidth;
        /// @brief number of vertical pixels of the screen
        int frameHeight;
        /// @brief number of frames per second of a television excerpt
        int fps;
        /// @brief minimum time in seconds for a segment to be considered a program
        int minimumTime;
        /// @brief vector of cv::Points that define all four corners of the screen
        vector<cv::Point> corners;
        /// @brief vector of camicasa::Segment containing all segments in a television program
        vector<camicasa::Segment> segments;
        /// @brief vector of camicasa::Logo containing all logos in a television program
        vector<camicasa::Logo> logos;
        /// @brief vector with positions representing the four corners of the screen containing the number of consecutive still frames
        vector<int> frameStillCount;

        /// @brief method for finding all four corners of the screen with some margin
        void populateCorners();
        
    public:
        /**
            @brief constructor of class camicasa::TVChannel
            @param frameWidth number of horizontal pixels of the screen
            @param frameHeight number of vertical pixels of the screen
            @param fps number of frames per second of a television excerpt
            @param minimumTime optional minimum time in seconds for a segment to be considered a program
        */
        TVChannel(int frameWidth, int frameHeight, int fps, int minimumTime = 120);
        
        /// @brief destructor of class camicasa::TVChannel
        ~TVChannel();

        /// @returns Program::corners
        vector<Point> getCorners();

        /// @returns Program::segments
        vector<Segment> getSegments();

        /// @returns Program::logos
        vector<Logo> getLogos();

        /// @returns Program::frameStillCount
        vector<int> getFrameStillCount();

        /**
            @brief The function camicasa::TVChannel::minimumTimePassed checks if the given time surpasses the minimum time defined
            @param time time to compare (in seconds)
            @returns returns true if the time if equal or greater than the minimum time specified in the constructor of camicasa::TVCHannel
            @note See more in camicasa::TVChannel::TVChannel(int frameWidth, int frameHeight, int fps, int minimumTime)
        */
        bool hasMinimumTimePassed(int time);

        /**
            @brief The function camicasa::TVChannel::addSegment adds a new segment to the vector of segments
            @param segment camicasa::Segment to add
        */
        void addSegment(Segment segment);

        /**
            @brief The function camicasa::TVChannel::addLogo adds a new logo to the vector of logos
            @param logo camicasa::Logo to add
        */
        void addLogo(Logo logo);

        /**
            @brief The function camicasa::TVChannel::checkForSaturatedCorners is a method for detecting if any corner on the screen has their
            average pixels
            under/above the specified threshold
            @param frame image frame input cv::Mat
            @param operation defines which camicasa::ComparisonOperation to execute (default is CHECK_SMALLER)
            @param threshold optional threshold to compare (default is 1)
            @returns returns true if any of the four corners have saturated pixels under/above the specified threshold
            @note See more in camicasa::TVChannel::getCorners()
        */
        void checkForSaturatedCorners(Mat &frame, ComparisonOperation operation = CHECK_SMALLER, int threshold = 1);

        /**
            @brief The functions camicasa::findLogo is a method that tries to find a logo given the input frames and history of still frames
            @param[in] inputOriginal original image frame input cv::Mat
            @param[in] inputBitwise compared image frame with bitwise_and operation input cv::Mat
            @param[out] logo camicasa::Logo with image frame found and relevant information
            @note See more in camicasa::TVChannel::getFrameStillCount()
        */
        void findLogo(Mat &inputOriginal, Mat &inputBitwise, Logo &logo);

        /**
            @returns returns true if sum of vector camicasa::TVChannel::getFrameStillCount() if different than 0
            @note See more in camicasa::TVChannel::getFrameStillCount()
        */
        bool hasStillFrames();

        /**
            @brief The function camicasa::TVChannel::findPatternLogo is a method checking if the given logo 
            is present on the frame
            @param input image frame input cv::Mat
            @param pattern camicasa::Logo containing the image to search in the input frame
            @returns returns true if the pattern was found
        */
        bool findPatternLogo(Mat &input, Logo& pattern);
    };

    /**
        @brief The function camicasa::screenThresholdDetection is a method for detecting if the average of pixels(BGR) falls
        under/above a certain threshold
        @param frame image frame input cv::Mat
        @param operation defines camicasa::ComparisonOperation to execute (default is CHECK_SMALLER)
        @param threshold optional threshold to compare (default is 1)
        @returns returns true if frame presented has average pixels that fall under/above the specified threshold
    */
    bool screenThresholdDetection(Mat& frame, ComparisonOperation operation = CHECK_SMALLER, int threshold = 1);

    /**
        @brief The function camicasa::formatTimestamp is a method converting the duration in milliseconds for human reading
        @param duration in milliseconds
        @returns returns string with the format h:m:s
    */
    string formatTimestamp(int duration);

    /**
        @brief The functions camicasa::convertBinarizedFrame is a method for inverting ou maintaining the input image binarization 
        considering the premise that the most abundant color is not the focus of the frame
        @param[in] input binarized image frame input cv::Mat
        @param[out] output binarized image frame output cv::Mat with mode of choice
        @param mode optional mode that describes which color should the important information be (default is HIGHLIGHT_IN_WHITE)
    */
    void convertBinarizedFrame(Mat& frame, Mat& output, BinarizationMode mode = HIGHLIGHT_IN_WHITE);
    
    /**
        @brief The functions camicasa::cropLogo is a method for finding the logo present and cropping the input frame
        @param[in] inputOriginal original image frame input cv::Mat
        @param[in] inputBitwise compared image frame with bitwise_and operation input cv::Mat
        @param[out] output camicasa::Logo containing image frame and other relevant information
    */
    void cropLogo(Mat& inputOriginal, Mat& inputBitwise, Logo& output);

    /**
        @brief The functions camicasa::morphOperation is a method for removing dots and loose pixels in the input frame
        utilizing the cv::morphologyEx function
        @param[in] input image frame input cv::Mat
        @param[out] output image frame output cv::Mat
    */
    void morphOperation(Mat& input, Mat& output);

    /**
        @brief The functions camicasa::distancePointRectangle is a method for finding the smallest distance between a given point and
        a rectangle
        @param point cv::Point to calculate distance from
        @param rectangle vector of cv::Point with twwo elements, the first the minimum x and y, the second the maximum x and y
        @returns double representing distance
    */
    double distancePointRectangle(Point point, vector<Point> rectangle);

    /**
        @brief Distance of given two points
        @param a cv::Point
        @param b cv::Point
        @return returns the distance as a double
    */ 
    double distancePoints(Point a, Point b);

    /**
        @brief Convert enum camicasa::TVChannelType into a string (char*)
        @param item camicasa::TVChannelType
        @return returns a sequence of chars
    */  
    const char* stringifyTVChannelType(TVChannelType item);

    /**
        @brief Convert enum camicasa::ScreenCorner into a string (char*)
        @param item camicasa::ScreenCorner
        @return returns a sequence of chars
    */
    const char* stringifyScreenCorner(ScreenCorner item);

}

#endif