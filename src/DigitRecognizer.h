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
    cv::Mat normalizeCellForDigit(const cv::Mat& cellImage) const;
    cv::Mat isolateDigitInk(const cv::Mat& gray) const;
    cv::Mat centerInk(const cv::Mat& ink, int outputSize, int borderValue) const;
    Candidate recognizeVariant(const cv::Mat& image) const;
    Candidate recognizeTemplateFallback(const cv::Mat& cellImage) const;
    bool isEmptyCell(const cv::Mat& cellImage, float& confidence) const;
    static Candidate mergeCandidates(const std::vector<Candidate>& candidates);
    static std::string findTessdataPath();
    static std::string trimOCRText(const char* raw);

    std::unique_ptr<tesseract::TessBaseAPI> api_;
    bool ready_ = false;
    std::string tessdataPath_;
};
