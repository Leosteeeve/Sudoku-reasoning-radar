#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

class ImagePreprocessor {
public:
    static constexpr int WarpSize = 900;
    static constexpr int CellSize = 100;

    static cv::Mat preprocessForGrid(const cv::Mat& image);
    static bool detectGrid(const cv::Mat& image,
                           std::vector<cv::Point2f>& orderedCorners,
                           cv::Mat* thresholdPreview,
                           std::string* errorMessage);
    static cv::Mat warpGrid(const cv::Mat& image,
                            const std::vector<cv::Point2f>& orderedCorners,
                            int outputSize = WarpSize);
    static std::vector<cv::Mat> splitCells(const cv::Mat& warpedGrid, int margin = 12);
    static bool saveDebugImage(const std::string& path, const cv::Mat& image, std::string* warning = nullptr);

private:
    static std::vector<cv::Point2f> orderCorners(const std::vector<cv::Point>& corners);
    static std::vector<cv::Point2f> orderCorners(const std::vector<cv::Point2f>& corners);
    static bool houghFallback(const cv::Mat& threshold,
                              cv::Size imageSize,
                              std::vector<cv::Point2f>& orderedCorners);
};
