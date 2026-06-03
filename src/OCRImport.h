#pragma once

#include "DigitRecognizer.h"
#include "OCRReviewState.h"

#include <string>

struct OCRProcessOptions {
    bool debug = false;
    std::string debugDir = "data/ocr_debug";
};

class OCRImport {
public:
    bool autoProcess(const std::string& imagePath, OCRResult& result, const OCRProcessOptions& options);
    bool loadImage(const std::string& imagePath, OCRResult& result);
    bool detectGrid(OCRResult& result, const OCRProcessOptions& options);
    bool runOCR(OCRResult& result, const OCRProcessOptions& options);
    std::string recognizerStatus() const;

private:
    bool ensureRecognizer(std::string* errorMessage);
    void finalizeResult(OCRResult& result, double elapsedMs) const;
    void saveDebugSet(const OCRResult& result, const OCRProcessOptions& options, const std::vector<cv::Mat>& cells) const;

    DigitRecognizer recognizer;
    std::string lastRecognizerStatus = "Tesseract not initialized.";
};
