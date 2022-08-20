
#if !defined HARRISDETECTOR

#define HARRISDETECTOR

 

#include <core/core.hpp>

#include <highgui/highgui.hpp>

#include <imgproc/imgproc.hpp>

 

class HarrisDetector {

private:

 

    // 32-bit float image of corner strength

    cv::Mat cornerStrength;

    // 32-bit float image of threshold corners

    cv::Mat cornerTh;

    // image of local maxima (internal)

    cv::Mat localMax;

    // size of neighborhood for derivatives smoothing

    int neighborhood;

    // aperture for gradient computation

    int aperture;

    // Harris parameter

    double k;

    // maximum strength for threshold computation

    double maxStrength;

    // calculated threshold (internal)

    double threshold;

    // size of neighborhood for non-max supression

    int nonMaxSize;

    // kernel for non-max supression

    cv::Mat kernel;

 

public:

    HarrisDetector() : neighborhood(3), aperture(3), 

                        k(0.01), maxStrength(0.0),

                        threshold(0.01), nonMaxSize(3) {

        // create kernel used in non-max supression

        setLocalMaxWindowSize(nonMaxSize);

    }

 

    void setLocalMaxWindowSize(int nonMaxSize) {

        this->nonMaxSize = nonMaxSize;

    }

 

    // Compute Harris corners

    void detect(const cv::Mat &image) {

        // Harris computation

        cv::cornerHarris(image, cornerStrength,

            neighborhood,   // neighborhood size

            aperture,       // aperture size

            k               // Harris parameter

            );

        // internal threshold computation

        double minStrength; // not used

        cv::minMaxLoc(cornerStrength, &minStrength, &maxStrength);

        // local maxima detection

        cv::Mat dilated;    //temporary image

        cv::dilate(cornerStrength, dilated, cv::Mat());

        cv::compare(cornerStrength, dilated, localMax, cv::CMP_EQ);

    }

 

    // Get the corner map from the comuted Harris values

    cv::Mat getCornerMap(double qualityLevel) {

        cv::Mat cornerMap;

        // thresholding the corner strength

        threshold = qualityLevel * maxStrength;

        cv::threshold(cornerStrength, cornerTh, threshold, 255, cv::THRESH_BINARY);

        // convert to 8-bit image

        cornerTh.convertTo(cornerMap, CV_8U);

        // non-maxima suppression

        cv::bitwise_and(cornerMap, localMax, cornerMap);

        return cornerMap;

    }

 

    // Get the feature points from the computed Harris value

    void getCorners(std::vector<cv::Point> &points, double qualityLevel) {

        // Get the corner map

        cv::Mat cornerMap = getCornerMap(qualityLevel);

        // Get the corners

        getCorners(points, cornerMap);

    }

 

    // Get the features points from the computed corner map

    void getCorners(std::vector<cv::Point> &points, const cv::Mat &cornerMap) {

        // Iterate over the pixels to obtain all features

        for (int y = 0; y < cornerMap.rows; y++) {

            const uchar *rowPtr = cornerMap.ptr<uchar>(y);

            for (int x = 0; x < cornerMap.cols; x++) {

                // if it is a feature point

                if (rowPtr[x]) {

                    points.push_back(cv::Point(x, y));

                }

            }

        }

    }

 

    // Draw circles at feature point locations on an image

    void drawOnImage(cv::Mat &image, const std::vector<cv::Point> &points,

        cv::Scalar color = cv::Scalar(255, 255, 255),

        int radius = 3, int thickness = 2) {

        std::vector<cv::Point>::const_iterator it = points.begin();

        // for all corners

        while (it != points.end()) {

            // draw a circle at each corner location

            cv::circle(image, *it, radius, color, thickness);

            ++ it;

        }

    }

 

 

};

 

#endif