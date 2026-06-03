#include "OCRImport.h"

#include "ImagePreprocessor.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <sstream>

bool OCRImport::autoProcess(const std::string& imagePath, OCRResult& result, const OCRProcessOptions& options) {
    const auto started = std::chrono::steady_clock::now();
    if (!loadImage(imagePath, result)) {
        return false;
    }
    if (!detectGrid(result, options)) {
        finalizeResult(result,
                       std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
        return false;
    }
    const bool ok = runOCR(result, options);
    finalizeResult(result,
                   std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
    return ok;
}

bool OCRImport::loadImage(const std::string& imagePath, OCRResult& result) {
    OCRResult next;
    next.originalPreview = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (next.originalPreview.empty()) {
        result.success = false;
        result.errorMessage = "Could not load image. Please check the file path or image format.";
        return false;
    }
    next.success = true;
    next.errorMessage = "Image loaded. Detect the grid or run Auto Process.";
    result = next;
    return true;
}

bool OCRImport::detectGrid(OCRResult& result, const OCRProcessOptions& options) {
    if (result.originalPreview.empty()) {
        result.success = false;
        result.errorMessage = "Could not load image. Please check the file path or image format.";
        return false;
    }

    cv::Mat threshold;
    std::string error;
    std::vector<cv::Point2f> corners;
    if (!ImagePreprocessor::detectGrid(result.originalPreview, corners, &threshold, &error)) {
        result.success = false;
        result.errorMessage = error;
        if (options.debug) {
            ImagePreprocessor::saveDebugImage(options.debugDir + "/threshold.png", threshold);
        }
        return false;
    }

    result.warpedGrid = ImagePreprocessor::warpGrid(result.originalPreview, corners, ImagePreprocessor::WarpSize);
    if (result.warpedGrid.empty()) {
        result.success = false;
        result.errorMessage = "Grid warp failed. Try a clearer screenshot or crop closer to the puzzle.";
        return false;
    }
    result.success = true;
    result.errorMessage = "Grid detected and normalized.";
    if (options.debug) {
        ImagePreprocessor::saveDebugImage(options.debugDir + "/original.png", result.originalPreview);
        ImagePreprocessor::saveDebugImage(options.debugDir + "/threshold.png", threshold);
        ImagePreprocessor::saveDebugImage(options.debugDir + "/warped.png", result.warpedGrid);
    }
    return true;
}

bool OCRImport::runOCR(OCRResult& result, const OCRProcessOptions& options) {
    const auto started = std::chrono::steady_clock::now();
    if (result.warpedGrid.empty()) {
        if (!detectGrid(result, options)) {
            return false;
        }
    }

    std::string error;
    if (!ensureRecognizer(&error)) {
        result.success = false;
        result.errorMessage = error;
        return false;
    }

    const std::vector<cv::Mat> cells = ImagePreprocessor::splitCells(result.warpedGrid, 12);
    if (cells.size() != 81) {
        result.success = false;
        result.errorMessage = "Failed to split the normalized grid into 81 cells.";
        return false;
    }

    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            result.cells[static_cast<size_t>(r * Board::Size + c)] =
                recognizer.recognizeCell(cells[static_cast<size_t>(r * Board::Size + c)], r, c);
        }
    }
    finalizeResult(result,
                   std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());

    OCRReviewState review;
    review.loadResult(result);
    result.puzzleString = review.puzzleString();
    result.givens = review.givens();
    result.lowConfidenceCount = review.lowConfidenceCount();
    result.conflictCount = review.conflictCount();
    result.validationResult = review.validationResult();
    result.validationMessage = review.validationMessage();

    result.success = true;
    std::ostringstream message;
    message << "OCR complete: " << result.givens << " givens, "
            << result.lowConfidenceCount << " low-confidence cells.";
    if (result.conflictCount > 0) {
        message << " Conflicts detected.";
    }
    result.errorMessage = message.str();
    if (options.debug) {
        saveDebugSet(result, options, cells);
    }
    return true;
}

std::string OCRImport::recognizerStatus() const {
    return lastRecognizerStatus;
}

bool OCRImport::ensureRecognizer(std::string* errorMessage) {
    if (recognizer.isReady()) {
        return true;
    }
    if (!recognizer.initialize(errorMessage)) {
        lastRecognizerStatus = errorMessage ? *errorMessage : "Tesseract initialization failed.";
        return false;
    }
    lastRecognizerStatus = "Tesseract ready: " + recognizer.tessdataPath();
    return true;
}

void OCRImport::finalizeResult(OCRResult& result, double elapsedMs) const {
    result.processingMs = elapsedMs;
    result.puzzleString.clear();
    result.puzzleString.reserve(81);
    result.givens = 0;
    result.lowConfidenceCount = 0;
    result.conflictCount = 0;
    for (OCRCell& cell : result.cells) {
        if (cell.digit >= 1 && cell.digit <= 9) {
            result.puzzleString.push_back(static_cast<char>('0' + cell.digit));
            ++result.givens;
            if (cell.lowConfidence) {
                ++result.lowConfidenceCount;
            }
        } else {
            cell.digit = 0;
            cell.isEmpty = true;
            result.puzzleString.push_back('0');
        }
    }
}

void OCRImport::saveDebugSet(const OCRResult& result,
                             const OCRProcessOptions& options,
                             const std::vector<cv::Mat>& cells) const {
    ImagePreprocessor::saveDebugImage(options.debugDir + "/original.png", result.originalPreview);
    ImagePreprocessor::saveDebugImage(options.debugDir + "/warped.png", result.warpedGrid);
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const size_t index = static_cast<size_t>(r * Board::Size + c);
            if (index >= cells.size()) {
                continue;
            }
            std::ostringstream name;
            name << options.debugDir << "/cell_r" << (r + 1) << "c" << (c + 1) << ".png";
            ImagePreprocessor::saveDebugImage(name.str(), cells[index]);
        }
    }
}
