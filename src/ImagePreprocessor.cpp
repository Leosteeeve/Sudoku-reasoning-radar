#include "ImagePreprocessor.h"

#include "Board.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <filesystem>

cv::Mat ImagePreprocessor::preprocessForGrid(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image.clone();
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    cv::Mat threshold;
    cv::adaptiveThreshold(blurred,
                          threshold,
                          255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          15,
                          3);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(threshold, threshold, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
    cv::dilate(threshold, threshold, kernel, cv::Point(-1, -1), 1);
    return threshold;
}

bool ImagePreprocessor::detectGrid(const cv::Mat& image,
                                   std::vector<cv::Point2f>& orderedCorners,
                                   cv::Mat* thresholdPreview,
                                   std::string* errorMessage) {
    orderedCorners.clear();
    if (image.empty()) {
        if (errorMessage) {
            *errorMessage = "Could not load image. Please check the file path or image format.";
        }
        return false;
    }

    cv::Mat threshold = preprocessForGrid(image);
    if (thresholdPreview) {
        *thresholdPreview = threshold.clone();
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(threshold, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const double imageArea = static_cast<double>(image.cols) * static_cast<double>(image.rows);
    double bestArea = 0.0;
    std::vector<cv::Point> bestQuad;
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < imageArea * 0.08 || area <= bestArea) {
            continue;
        }

        const double perimeter = cv::arcLength(contour, true);
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, 0.02 * perimeter, true);
        if (approx.size() == 4 && cv::isContourConvex(approx)) {
            bestArea = area;
            bestQuad = approx;
        }
    }

    if (!bestQuad.empty()) {
        orderedCorners = orderCorners(bestQuad);
        return true;
    }

    if (houghFallback(threshold, image.size(), orderedCorners)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = "Grid detection failed. Try a clearer screenshot or crop the image closer to the Sudoku grid.";
    }
    return false;
}

cv::Mat ImagePreprocessor::warpGrid(const cv::Mat& image,
                                    const std::vector<cv::Point2f>& orderedCorners,
                                    int outputSize) {
    if (image.empty() || orderedCorners.size() != 4) {
        return {};
    }

    std::vector<cv::Point2f> target = {
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(static_cast<float>(outputSize - 1), 0.0f),
        cv::Point2f(static_cast<float>(outputSize - 1), static_cast<float>(outputSize - 1)),
        cv::Point2f(0.0f, static_cast<float>(outputSize - 1))
    };

    cv::Mat transform = cv::getPerspectiveTransform(orderedCorners, target);
    cv::Mat warped;
    cv::warpPerspective(image, warped, transform, cv::Size(outputSize, outputSize));
    return warped;
}

std::vector<cv::Mat> ImagePreprocessor::splitCells(const cv::Mat& warpedGrid, int margin) {
    std::vector<cv::Mat> cells;
    cells.reserve(81);
    if (warpedGrid.empty()) {
        return cells;
    }

    const int cellSize = warpedGrid.cols / Board::Size;
    const int safeMargin = std::clamp(margin, 4, std::max(4, cellSize / 3));
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int x = c * cellSize + safeMargin;
            const int y = r * cellSize + safeMargin;
            const int w = std::max(1, cellSize - safeMargin * 2);
            const int h = std::max(1, cellSize - safeMargin * 2);
            cv::Rect roi(x, y, std::min(w, warpedGrid.cols - x), std::min(h, warpedGrid.rows - y));
            cells.push_back(warpedGrid(roi).clone());
        }
    }
    return cells;
}

bool ImagePreprocessor::saveDebugImage(const std::string& path, const cv::Mat& image, std::string* warning) {
    if (image.empty()) {
        return true;
    }
    try {
        const std::filesystem::path outPath(path);
        if (outPath.has_parent_path()) {
            std::filesystem::create_directories(outPath.parent_path());
        }
        if (!cv::imwrite(path, image)) {
            if (warning) {
                *warning = "Warning: failed to save OCR debug image " + path + ".";
            }
            return false;
        }
    } catch (const std::exception& ex) {
        if (warning) {
            *warning = std::string("Warning: failed to save OCR debug image: ") + ex.what();
        }
        return false;
    }
    return true;
}

std::vector<cv::Point2f> ImagePreprocessor::orderCorners(const std::vector<cv::Point>& corners) {
    std::vector<cv::Point2f> points;
    points.reserve(corners.size());
    for (const cv::Point& p : corners) {
        points.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y));
    }
    return orderCorners(points);
}

std::vector<cv::Point2f> ImagePreprocessor::orderCorners(const std::vector<cv::Point2f>& corners) {
    std::vector<cv::Point2f> ordered(4);
    if (corners.size() != 4) {
        return {};
    }

    auto sum = [](const cv::Point2f& p) { return p.x + p.y; };
    auto diff = [](const cv::Point2f& p) { return p.x - p.y; };
    ordered[0] = *std::min_element(corners.begin(), corners.end(), [&](const auto& a, const auto& b) { return sum(a) < sum(b); });
    ordered[2] = *std::max_element(corners.begin(), corners.end(), [&](const auto& a, const auto& b) { return sum(a) < sum(b); });
    ordered[1] = *std::max_element(corners.begin(), corners.end(), [&](const auto& a, const auto& b) { return diff(a) < diff(b); });
    ordered[3] = *std::min_element(corners.begin(), corners.end(), [&](const auto& a, const auto& b) { return diff(a) < diff(b); });
    return ordered;
}

bool ImagePreprocessor::houghFallback(const cv::Mat& threshold,
                                      cv::Size imageSize,
                                      std::vector<cv::Point2f>& orderedCorners) {
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(threshold, lines, 1, CV_PI / 180.0, 120, imageSize.width / 5.0, 20);
    if (lines.size() < 8) {
        return false;
    }

    std::vector<cv::Point> points;
    points.reserve(lines.size() * 2);
    for (const cv::Vec4i& line : lines) {
        points.emplace_back(line[0], line[1]);
        points.emplace_back(line[2], line[3]);
    }

    cv::Rect bounds = cv::boundingRect(points);
    const double area = static_cast<double>(bounds.area());
    const double imageArea = static_cast<double>(imageSize.width) * static_cast<double>(imageSize.height);
    if (area < imageArea * 0.12 || bounds.width < imageSize.width * 0.25 || bounds.height < imageSize.height * 0.25) {
        return false;
    }

    std::vector<cv::Point2f> corners = {
        cv::Point2f(static_cast<float>(bounds.x), static_cast<float>(bounds.y)),
        cv::Point2f(static_cast<float>(bounds.x + bounds.width), static_cast<float>(bounds.y)),
        cv::Point2f(static_cast<float>(bounds.x + bounds.width), static_cast<float>(bounds.y + bounds.height)),
        cv::Point2f(static_cast<float>(bounds.x), static_cast<float>(bounds.y + bounds.height))
    };
    orderedCorners = orderCorners(corners);
    return true;
}
