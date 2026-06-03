#include "DigitRecognizer.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
std::filesystem::path executableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

void addTessdataCandidate(std::vector<std::filesystem::path>& candidates, const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }
    candidates.push_back(path);
    if (path.filename() != "tessdata") {
        candidates.push_back(path / "tessdata");
    }
}
}

DigitRecognizer::DigitRecognizer()
    : api_(std::make_unique<tesseract::TessBaseAPI>()) {}

DigitRecognizer::~DigitRecognizer() {
    if (api_) {
        api_->End();
    }
}

bool DigitRecognizer::initialize(std::string* errorMessage) {
    if (ready_) {
        return true;
    }

    tessdataPath_ = findTessdataPath();
    const char* path = tessdataPath_.empty() ? nullptr : tessdataPath_.c_str();
    if (api_->Init(path, "eng") != 0) {
        if (errorMessage) {
            *errorMessage =
                "Tesseract OCR initialization failed. Make sure tessdata/eng.traineddata exists next to the executable.";
        }
        return false;
    }

    api_->SetVariable("tessedit_char_whitelist", "123456789");
    api_->SetVariable("classify_bln_numeric_mode", "1");
    api_->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    ready_ = true;
    return true;
}

bool DigitRecognizer::isReady() const {
    return ready_;
}

OCRCell DigitRecognizer::recognizeCell(const cv::Mat& cellImage, int row, int col) const {
    OCRCell cell;
    cell.row = row;
    cell.col = col;
    cell.digit = 0;
    cell.isEmpty = true;
    cell.confidence = 0.0f;
    cell.lowConfidence = false;

    float emptyConfidence = 0.0f;
    if (isEmptyCell(cellImage, emptyConfidence)) {
        cell.confidence = emptyConfidence;
        cell.isEmpty = true;
        return cell;
    }

    Candidate best;
    for (const cv::Mat& variant : makeVariants(cellImage)) {
        const Candidate candidate = recognizeVariant(variant);
        if (candidate.confidence > best.confidence) {
            best = candidate;
        }
    }

    cell.digit = best.digit;
    cell.confidence = best.confidence;
    cell.rawText = best.rawText;
    cell.isEmpty = best.digit == 0;
    cell.lowConfidence = best.digit != 0 && best.confidence < 0.65f;
    return cell;
}

std::string DigitRecognizer::tessdataPath() const {
    return tessdataPath_;
}

std::vector<cv::Mat> DigitRecognizer::makeVariants(const cv::Mat& cellImage) const {
    std::vector<cv::Mat> variants;
    if (cellImage.empty()) {
        return variants;
    }

    cv::Mat gray;
    if (cellImage.channels() == 1) {
        gray = cellImage.clone();
    } else {
        cv::cvtColor(cellImage, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);

    cv::Mat binaryInv;
    cv::threshold(blurred, binaryInv, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::Mat binary;
    cv::bitwise_not(binaryInv, binary);

    auto centeredLarge = [](const cv::Mat& src, int borderValue) {
        cv::Mat cleaned = src.clone();
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::morphologyEx(cleaned, cleaned, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1);

        cv::Mat resized;
        cv::resize(cleaned, resized, cv::Size(140, 140), 0, 0, cv::INTER_CUBIC);
        cv::Mat padded;
        cv::copyMakeBorder(resized,
                           padded,
                           20,
                           20,
                           20,
                           20,
                           cv::BORDER_CONSTANT,
                           cv::Scalar(borderValue));
        return padded;
    };

    variants.push_back(centeredLarge(binary, 255));
    variants.push_back(centeredLarge(binaryInv, 0));
    variants.push_back(centeredLarge(gray, 255));
    return variants;
}

DigitRecognizer::Candidate DigitRecognizer::recognizeVariant(const cv::Mat& image) const {
    Candidate candidate;
    if (!ready_ || image.empty()) {
        return candidate;
    }

    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image;
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    api_->Clear();
    api_->SetImage(gray.data, gray.cols, gray.rows, 1, static_cast<int>(gray.step));
    char* raw = api_->GetUTF8Text();
    candidate.rawText = trimOCRText(raw);
    if (raw) {
        delete[] raw;
    }
    const int conf = api_->MeanTextConf();
    candidate.confidence = std::clamp(static_cast<float>(conf) / 100.0f, 0.0f, 1.0f);
    for (char ch : candidate.rawText) {
        if (ch >= '1' && ch <= '9') {
            candidate.digit = ch - '0';
            return candidate;
        }
    }
    candidate.digit = 0;
    candidate.confidence *= 0.35f;
    return candidate;
}

bool DigitRecognizer::isEmptyCell(const cv::Mat& cellImage, float& confidence) const {
    confidence = 0.0f;
    if (cellImage.empty()) {
        confidence = 1.0f;
        return true;
    }

    cv::Mat gray;
    if (cellImage.channels() == 1) {
        gray = cellImage.clone();
    } else {
        cv::cvtColor(cellImage, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat binaryInv;
    cv::threshold(gray, binaryInv, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int components = cv::connectedComponentsWithStats(binaryInv, labels, stats, centroids, 8, CV_32S);
    int foregroundPixels = 0;
    int largestComponent = 0;
    for (int i = 1; i < components; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < 6) {
            continue;
        }
        foregroundPixels += area;
        largestComponent = std::max(largestComponent, area);
    }

    const double total = static_cast<double>(gray.rows) * static_cast<double>(gray.cols);
    const double foregroundRatio = total <= 0.0 ? 0.0 : static_cast<double>(foregroundPixels) / total;
    const bool empty = foregroundPixels < 34 || foregroundRatio < 0.018 || largestComponent < 22;
    confidence = empty ? 1.0f : 0.0f;
    return empty;
}

std::string DigitRecognizer::findTessdataPath() {
    const char* env = std::getenv("TESSDATA_PREFIX");
    std::vector<std::filesystem::path> candidates;
    addTessdataCandidate(candidates, executableDirectory());
    addTessdataCandidate(candidates, std::filesystem::current_path());
    addTessdataCandidate(candidates, std::filesystem::path("tessdata"));
    addTessdataCandidate(candidates, std::filesystem::path("D:/MSYS2/ucrt64/share/tessdata"));
    if (env && *env) {
        addTessdataCandidate(candidates, std::filesystem::path(env));
    }

    for (const std::filesystem::path& path : candidates) {
        try {
            if (std::filesystem::exists(path / "eng.traineddata")) {
                return path.generic_string();
            }
        } catch (...) {
        }
    }
    return {};
}

std::string DigitRecognizer::trimOCRText(const char* raw) {
    if (!raw) {
        return {};
    }
    std::string text(raw);
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char ch) {
                   return std::isspace(ch) != 0;
               }),
               text.end());
    return text;
}
