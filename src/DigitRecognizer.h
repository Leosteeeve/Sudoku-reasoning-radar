#pragma once

#include "OCRReviewState.h"

#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <tesseract/baseapi.h>

class DigitRecognizer {
public:
    DigitRecognizer();
    ~DigitRecognizer();

    bool initialize(std::string* errorMessage = nullptr);
    bool isReady() const;
    OCRCell recognizeCell(const cv::Mat& cellImage, int row, int col) const;
    std::string tessdataPath() const;

private:
    struct Candidate {
        int digit = 0;
        float confidence = 0.0f;
        std::string rawText;
    };

    std::vector<cv::Mat> makeVariants(const cv::Mat& cellImage) const;
    Candidate recognizeVariant(const cv::Mat& image) const;
    bool isEmptyCell(const cv::Mat& cellImage, float& confidence) const;
    static std::string findTessdataPath();
    static std::string trimOCRText(const char* raw);

    std::unique_ptr<tesseract::TessBaseAPI> api_;
    bool ready_ = false;
    std::string tessdataPath_;
};
