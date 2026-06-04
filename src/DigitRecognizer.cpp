#include "DigitRecognizer.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>

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

cv::Mat toGray(const cv::Mat& image) {
    if (image.empty()) {
        return {};
    }
    if (image.channels() == 1) {
        return image.clone();
    }
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

cv::Rect insetRect(const cv::Mat& image, double ratio) {
    const int insetX = std::clamp(static_cast<int>(std::lround(image.cols * ratio)), 1, std::max(1, image.cols / 4));
    const int insetY = std::clamp(static_cast<int>(std::lround(image.rows * ratio)), 1, std::max(1, image.rows / 4));
    return cv::Rect(insetX,
                    insetY,
                    std::max(1, image.cols - insetX * 2),
                    std::max(1, image.rows - insetY * 2));
}

double foregroundRatio(const cv::Mat& binaryInk) {
    if (binaryInk.empty()) {
        return 0.0;
    }
    return static_cast<double>(cv::countNonZero(binaryInk))
        / static_cast<double>(std::max(1, binaryInk.rows * binaryInk.cols));
}

void pushUnique(std::vector<cv::Mat>& variants, const cv::Mat& image) {
    if (!image.empty()) {
        variants.push_back(image);
    }
}

int countInteriorHoles(const cv::Mat& binaryInk) {
    if (binaryInk.empty()) {
        return 0;
    }

    cv::Mat normalized;
    cv::threshold(binaryInk, normalized, 0, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(normalized.clone(), contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    int holes = 0;
    for (size_t i = 0; i < hierarchy.size(); ++i) {
        if (hierarchy[i][3] >= 0 && std::abs(cv::contourArea(contours[i])) > 8.0) {
            ++holes;
        }
    }
    return std::min(holes, 2);
}

double averageDistanceToInk(const cv::Mat& distanceField, const cv::Mat& sampleInk) {
    if (distanceField.empty() || sampleInk.empty() || cv::countNonZero(sampleInk) == 0) {
        return 999.0;
    }
    return cv::mean(distanceField, sampleInk)[0];
}

cv::Mat makeTemplateDigit(int digit, int fontFace, int thickness) {
    constexpr int canvasSize = 180;
    cv::Mat canvas(canvasSize, canvasSize, CV_8UC1, cv::Scalar(0));

    const std::string text = std::to_string(digit);
    int baseline = 0;
    cv::Size unitSize = cv::getTextSize(text, fontFace, 1.0, thickness, &baseline);
    if (unitSize.width <= 0 || unitSize.height <= 0) {
        return {};
    }

    const double scale = std::min(122.0 / static_cast<double>(unitSize.width),
                                  135.0 / static_cast<double>(unitSize.height));
    cv::Size textSize = cv::getTextSize(text, fontFace, scale, thickness, &baseline);
    const cv::Point origin((canvasSize - textSize.width) / 2,
                           (canvasSize + textSize.height) / 2 - baseline / 2);

    cv::putText(canvas, text, origin, fontFace, scale, cv::Scalar(255), thickness, cv::LINE_AA);
    cv::threshold(canvas, canvas, 16, 255, cv::THRESH_BINARY);
    return canvas;
}

struct DigitTemplate {
    int digit = 0;
    cv::Mat ink;
    cv::Mat distanceToInk;
    double ratio = 0.0;
    int holes = 0;
};

std::vector<DigitTemplate> buildDigitTemplates() {
    std::vector<DigitTemplate> templates;
    const std::array<int, 4> fonts{
        cv::FONT_HERSHEY_SIMPLEX,
        cv::FONT_HERSHEY_DUPLEX,
        cv::FONT_HERSHEY_COMPLEX,
        cv::FONT_HERSHEY_TRIPLEX
    };
    const std::array<int, 3> thicknesses{2, 3, 4};

    for (int digit = 1; digit <= 9; ++digit) {
        for (int font : fonts) {
            for (int thickness : thicknesses) {
                DigitTemplate entry;
                entry.digit = digit;
                entry.ink = makeTemplateDigit(digit, font, thickness);
                if (entry.ink.empty()) {
                    continue;
                }

                cv::Mat inverted;
                cv::bitwise_not(entry.ink, inverted);
                cv::distanceTransform(inverted, entry.distanceToInk, cv::DIST_L2, 3);
                entry.ratio = foregroundRatio(entry.ink);
                entry.holes = countInteriorHoles(entry.ink);
                templates.push_back(entry);
            }
        }
    }

    return templates;
}

const std::vector<DigitTemplate>& digitTemplates() {
    static const std::vector<DigitTemplate> templates = buildDigitTemplates();
    return templates;
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
    api_->SetVariable("load_system_dawg", "0");
    api_->SetVariable("load_freq_dawg", "0");
    api_->SetVariable("segment_penalty_dict_nonword", "0");
    api_->SetVariable("language_model_penalty_non_dict_word", "0");
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
    const bool emptyLikely = isEmptyCell(cellImage, emptyConfidence);

    std::vector<Candidate> candidates;
    for (const cv::Mat& variant : makeVariants(cellImage)) {
        const Candidate candidate = recognizeVariant(variant);
        candidates.push_back(candidate);
    }
    const Candidate templateCandidate = recognizeTemplateFallback(cellImage);
    if (templateCandidate.digit != 0) {
        candidates.push_back(templateCandidate);
    }
    const Candidate best = mergeCandidates(candidates);

    if (best.digit == 0 || (emptyLikely && best.confidence < 0.48f)) {
        cell.confidence = std::max(emptyConfidence, 1.0f - best.confidence);
        cell.isEmpty = true;
        cell.rawText = best.rawText;
        return cell;
    }

    cell.digit = best.digit;
    cell.confidence = best.confidence;
    cell.rawText = best.rawText;
    cell.isEmpty = best.digit == 0;
    cell.lowConfidence = best.digit != 0 && best.confidence < (emptyLikely ? 0.72f : 0.65f);
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

    cv::Mat gray = normalizeCellForDigit(cellImage);
    if (gray.empty()) {
        return variants;
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0.0);

    cv::Mat otsuInk;
    cv::threshold(blurred, otsuInk, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::Mat adaptiveInk;
    cv::adaptiveThreshold(blurred,
                          adaptiveInk,
                          255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          21,
                          8);

    cv::Mat sharpened;
    cv::Mat unsharp;
    cv::GaussianBlur(gray, unsharp, cv::Size(0, 0), 1.2);
    cv::addWeighted(gray, 1.65, unsharp, -0.65, 0.0, sharpened);
    cv::Mat sharpInk;
    cv::threshold(sharpened, sharpInk, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::array<cv::Mat, 3> inks{otsuInk, adaptiveInk, sharpInk};
    for (const cv::Mat& ink : inks) {
        cv::Mat isolated = isolateDigitInk(ink);
        cv::Mat clean = isolated.empty() ? ink.clone() : isolated.clone();
        if (foregroundRatio(clean) < 0.003 || foregroundRatio(clean) > 0.40) {
            continue;
        }
        cv::Mat kernel2 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::morphologyEx(clean, clean, cv::MORPH_OPEN, kernel2, cv::Point(-1, -1), 1);
        cv::morphologyEx(clean, clean, cv::MORPH_CLOSE, kernel2, cv::Point(-1, -1), 1);

        cv::Mat whiteBackground = centerInk(clean, 180, 255);
        cv::Mat blackBackground;
        cv::bitwise_not(whiteBackground, blackBackground);
        pushUnique(variants, whiteBackground);
        pushUnique(variants, blackBackground);

        cv::Mat dilated;
        cv::dilate(clean, dilated, kernel2, cv::Point(-1, -1), 1);
        pushUnique(variants, centerInk(dilated, 180, 255));
    }

    cv::Mat grayLarge;
    cv::resize(gray, grayLarge, cv::Size(180, 180), 0, 0, cv::INTER_CUBIC);
    pushUnique(variants, grayLarge);

    return variants;
}

cv::Mat DigitRecognizer::normalizeCellForDigit(const cv::Mat& cellImage) const {
    cv::Mat gray = toGray(cellImage);
    if (gray.empty()) {
        return {};
    }

    const cv::Rect inner = insetRect(gray, 0.12);
    cv::Mat cropped = gray(inner).clone();
    cv::normalize(cropped, cropped, 0, 255, cv::NORM_MINMAX);
    return cropped;
}

cv::Mat DigitRecognizer::isolateDigitInk(const cv::Mat& inkSource) const {
    if (inkSource.empty()) {
        return {};
    }

    cv::Mat ink;
    if (inkSource.channels() == 1) {
        ink = inkSource.clone();
    } else {
        cv::cvtColor(inkSource, ink, cv::COLOR_BGR2GRAY);
    }
    cv::threshold(ink, ink, 0, 255, cv::THRESH_BINARY);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int components = cv::connectedComponentsWithStats(ink, labels, stats, centroids, 8, CV_32S);
    if (components <= 1) {
        return {};
    }

    cv::Mat filtered = cv::Mat::zeros(ink.size(), CV_8UC1);
    const cv::Point2d center((ink.cols - 1) * 0.5, (ink.rows - 1) * 0.5);
    const double totalArea = static_cast<double>(std::max(1, ink.rows * ink.cols));
    double bestScore = 0.0;
    int bestLabel = -1;
    for (int i = 1; i < components; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        const int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
        const int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        if (area < 8 || h < ink.rows * 0.14 || w < ink.cols * 0.04) {
            continue;
        }
        if (w > ink.cols * 0.88 || h > ink.rows * 0.92 || area > totalArea * 0.45) {
            continue;
        }
        const double cx = centroids.at<double>(i, 0);
        const double cy = centroids.at<double>(i, 1);
        const double dx = (cx - center.x) / std::max(1.0, static_cast<double>(ink.cols));
        const double dy = (cy - center.y) / std::max(1.0, static_cast<double>(ink.rows));
        const double distancePenalty = std::sqrt(dx * dx + dy * dy);
        const double aspect = static_cast<double>(h) / static_cast<double>(std::max(1, w));
        const double aspectBonus = aspect > 0.7 && aspect < 5.8 ? 1.0 : 0.55;
        const double score = static_cast<double>(area) * aspectBonus * (1.0 - std::min(0.8, distancePenalty * 2.0));
        if (score > bestScore) {
            bestScore = score;
            bestLabel = i;
        }
    }

    if (bestLabel < 0) {
        return {};
    }

    const int bestX = stats.at<int>(bestLabel, cv::CC_STAT_LEFT);
    const int bestY = stats.at<int>(bestLabel, cv::CC_STAT_TOP);
    const int bestW = stats.at<int>(bestLabel, cv::CC_STAT_WIDTH);
    const int bestH = stats.at<int>(bestLabel, cv::CC_STAT_HEIGHT);
    const cv::Point2d bestCenter(centroids.at<double>(bestLabel, 0), centroids.at<double>(bestLabel, 1));
    for (int i = 1; i < components; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        const int x = stats.at<int>(i, cv::CC_STAT_LEFT);
        const int y = stats.at<int>(i, cv::CC_STAT_TOP);
        const int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
        const int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        const double cx = centroids.at<double>(i, 0);
        const double cy = centroids.at<double>(i, 1);
        const bool nearBest = std::abs(cx - bestCenter.x) < ink.cols * 0.26 && std::abs(cy - bestCenter.y) < ink.rows * 0.34;
        const bool overlapsBestBox = x < bestX + bestW + ink.cols * 0.08 && x + w > bestX - ink.cols * 0.08
            && y < bestY + bestH + ink.rows * 0.08 && y + h > bestY - ink.rows * 0.08;
        if (i == bestLabel || ((nearBest || overlapsBestBox) && area >= 5 && w < ink.cols * 0.75 && h < ink.rows * 0.9)) {
            filtered.setTo(255, labels == i);
        }
    }

    if (foregroundRatio(filtered) < 0.003) {
        return {};
    }
    return filtered;
}

cv::Mat DigitRecognizer::centerInk(const cv::Mat& ink, int outputSize, int borderValue) const {
    if (ink.empty()) {
        return {};
    }
    std::vector<cv::Point> points;
    cv::findNonZero(ink, points);
    if (points.empty()) {
        return {};
    }

    cv::Rect bounds = cv::boundingRect(points);
    bounds.x = std::max(0, bounds.x - 2);
    bounds.y = std::max(0, bounds.y - 2);
    bounds.width = std::min(ink.cols - bounds.x, bounds.width + 4);
    bounds.height = std::min(ink.rows - bounds.y, bounds.height + 4);
    cv::Mat digit = ink(bounds).clone();

    const int target = static_cast<int>(std::lround(outputSize * 0.80));
    const double scale = static_cast<double>(target) / static_cast<double>(std::max(digit.cols, digit.rows));
    cv::Mat resized;
    cv::resize(digit,
               resized,
               cv::Size(std::max(1, static_cast<int>(std::lround(digit.cols * scale))),
                        std::max(1, static_cast<int>(std::lround(digit.rows * scale)))),
               0,
               0,
               cv::INTER_CUBIC);

    cv::Mat canvas(outputSize, outputSize, CV_8UC1, cv::Scalar(borderValue));
    cv::Mat draw;
    if (borderValue == 255) {
        cv::bitwise_not(resized, draw);
    } else {
        draw = resized;
    }
    const int x = (outputSize - resized.cols) / 2;
    const int y = (outputSize - resized.rows) / 2;
    draw.copyTo(canvas(cv::Rect(x, y, resized.cols, resized.rows)));
    return canvas;
}

DigitRecognizer::Candidate DigitRecognizer::recognizeVariant(const cv::Mat& image) const {
    Candidate best;
    if (!ready_ || image.empty()) {
        return best;
    }

    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image;
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    const std::array<tesseract::PageSegMode, 3> modes{
        tesseract::PSM_SINGLE_CHAR,
        tesseract::PSM_SINGLE_WORD,
        tesseract::PSM_SINGLE_LINE
    };
    for (tesseract::PageSegMode mode : modes) {
        Candidate candidate;
        api_->Clear();
        api_->SetPageSegMode(mode);
        api_->SetImage(gray.data, gray.cols, gray.rows, 1, static_cast<int>(gray.step));
        api_->SetSourceResolution(300);
        char* raw = api_->GetUTF8Text();
        candidate.rawText = trimOCRText(raw);
        if (raw) {
            delete[] raw;
        }
        const int conf = api_->MeanTextConf();
        candidate.confidence = std::clamp(static_cast<float>(conf) / 100.0f, 0.0f, 1.0f);
        if (mode != tesseract::PSM_SINGLE_CHAR) {
            candidate.confidence *= 0.95f;
        }
        for (char ch : candidate.rawText) {
            if (ch >= '1' && ch <= '9') {
                candidate.digit = ch - '0';
                if (candidate.confidence > best.confidence) {
                    best = candidate;
                }
                break;
            }
        }
        if (candidate.digit == 0) {
            candidate.confidence *= 0.35f;
            if (candidate.confidence > best.confidence) {
                best = candidate;
            }
        }
    }
    api_->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    return best;
}

DigitRecognizer::Candidate DigitRecognizer::recognizeTemplateFallback(const cv::Mat& cellImage) const {
    Candidate result;

    cv::Mat gray = normalizeCellForDigit(cellImage);
    if (gray.empty()) {
        return result;
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0.0);

    std::vector<cv::Mat> inkSources;
    cv::Mat otsuInk;
    cv::threshold(blurred, otsuInk, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    inkSources.push_back(otsuInk);

    cv::Mat adaptiveInk;
    cv::adaptiveThreshold(blurred,
                          adaptiveInk,
                          255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          21,
                          8);
    inkSources.push_back(adaptiveInk);

    cv::Mat sharpened;
    cv::Mat unsharp;
    cv::GaussianBlur(gray, unsharp, cv::Size(0, 0), 1.2);
    cv::addWeighted(gray, 1.65, unsharp, -0.65, 0.0, sharpened);
    cv::Mat sharpInk;
    cv::threshold(sharpened, sharpInk, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    inkSources.push_back(sharpInk);

    cv::Mat bestInk;
    double bestInkScore = -1.0;
    for (const cv::Mat& source : inkSources) {
        cv::Mat isolated = isolateDigitInk(source);
        cv::Mat clean = isolated.empty() ? source.clone() : isolated.clone();
        cv::threshold(clean, clean, 0, 255, cv::THRESH_BINARY);

        const double ratio = foregroundRatio(clean);
        if (ratio < 0.0025 || ratio > 0.38) {
            continue;
        }

        std::vector<cv::Point> points;
        cv::findNonZero(clean, points);
        if (points.empty()) {
            continue;
        }

        const cv::Rect bounds = cv::boundingRect(points);
        const double heightRatio = static_cast<double>(bounds.height) / static_cast<double>(std::max(1, gray.rows));
        const double widthRatio = static_cast<double>(bounds.width) / static_cast<double>(std::max(1, gray.cols));
        if (heightRatio < 0.16 || widthRatio < 0.035) {
            continue;
        }

        const double centrality = 1.0
            - std::min(0.9,
                       (std::abs(bounds.x + bounds.width * 0.5 - gray.cols * 0.5) / std::max(1.0, static_cast<double>(gray.cols)))
                           + (std::abs(bounds.y + bounds.height * 0.5 - gray.rows * 0.5)
                              / std::max(1.0, static_cast<double>(gray.rows))));
        const double inkScore = static_cast<double>(cv::countNonZero(clean)) * std::max(0.1, centrality);
        if (inkScore > bestInkScore) {
            bestInkScore = inkScore;
            bestInk = clean;
        }
    }

    cv::Mat centered = centerInk(bestInk, 180, 0);
    if (centered.empty()) {
        return result;
    }
    cv::threshold(centered, centered, 0, 255, cv::THRESH_BINARY);

    cv::Mat invertedInput;
    cv::bitwise_not(centered, invertedInput);
    cv::Mat inputDistance;
    cv::distanceTransform(invertedInput, inputDistance, cv::DIST_L2, 3);

    const double inputRatio = foregroundRatio(centered);
    const int inputHoles = countInteriorHoles(centered);

    int bestDigit = 0;
    double bestScore = std::numeric_limits<double>::max();
    double secondScore = std::numeric_limits<double>::max();

    for (const DigitTemplate& tmpl : digitTemplates()) {
        const double sampleToTemplate = averageDistanceToInk(tmpl.distanceToInk, centered);
        const double templateToSample = averageDistanceToInk(inputDistance, tmpl.ink);
        const double ratioPenalty = std::abs(inputRatio - tmpl.ratio) * 120.0;
        const double holePenalty = static_cast<double>(std::abs(inputHoles - tmpl.holes)) * 2.2;
        const double score = sampleToTemplate * 0.58 + templateToSample * 0.42 + ratioPenalty + holePenalty;

        if (score < bestScore) {
            secondScore = bestScore;
            bestScore = score;
            bestDigit = tmpl.digit;
        } else {
            secondScore = std::min(secondScore, score);
        }
    }

    if (bestDigit == 0 || bestScore > 15.5) {
        return result;
    }

    const double separation = secondScore == std::numeric_limits<double>::max()
        ? 0.0
        : std::clamp((secondScore - bestScore) / std::max(1.0, secondScore), 0.0, 1.0);
    result.digit = bestDigit;
    result.confidence = std::clamp(static_cast<float>(0.78 - bestScore / 42.0 + separation * 0.08), 0.0f, 0.82f);
    if (result.confidence < 0.50f) {
        result.digit = 0;
        result.confidence = 0.0f;
        return result;
    }
    result.rawText = "template:" + std::to_string(bestDigit);
    return result;
}

DigitRecognizer::Candidate DigitRecognizer::mergeCandidates(const std::vector<Candidate>& candidates) {
    Candidate bestRaw;
    std::array<float, 10> score{};
    std::array<int, 10> votes{};
    std::array<Candidate, 10> bestByDigit{};

    for (const Candidate& candidate : candidates) {
        if (candidate.confidence > bestRaw.confidence) {
            bestRaw = candidate;
        }
        if (candidate.digit < 1 || candidate.digit > 9) {
            continue;
        }
        const float clamped = std::clamp(candidate.confidence, 0.0f, 1.0f);
        score[static_cast<size_t>(candidate.digit)] += clamped * clamped + 0.12f;
        votes[static_cast<size_t>(candidate.digit)] += 1;
        if (candidate.confidence > bestByDigit[static_cast<size_t>(candidate.digit)].confidence) {
            bestByDigit[static_cast<size_t>(candidate.digit)] = candidate;
        }
    }

    int bestDigit = 0;
    float bestScore = 0.0f;
    float secondScore = 0.0f;
    for (int digit = 1; digit <= 9; ++digit) {
        const float value = score[static_cast<size_t>(digit)];
        if (value > bestScore) {
            secondScore = bestScore;
            bestScore = value;
            bestDigit = digit;
        } else {
            secondScore = std::max(secondScore, value);
        }
    }

    if (bestDigit == 0) {
        return bestRaw;
    }

    Candidate merged = bestByDigit[static_cast<size_t>(bestDigit)];
    const int voteCount = votes[static_cast<size_t>(bestDigit)];
    const float agreementBonus = std::min(0.18f, 0.045f * static_cast<float>(std::max(0, voteCount - 1)));
    const float separation = bestScore <= 0.0f ? 0.0f : std::clamp((bestScore - secondScore) / bestScore, 0.0f, 1.0f);
    merged.confidence = std::clamp(merged.confidence * 0.78f + separation * 0.16f + agreementBonus, 0.0f, 1.0f);
    return merged;
}

bool DigitRecognizer::isEmptyCell(const cv::Mat& cellImage, float& confidence) const {
    confidence = 0.0f;
    if (cellImage.empty()) {
        confidence = 1.0f;
        return true;
    }

    cv::Mat gray = normalizeCellForDigit(cellImage);
    if (gray.empty()) {
        confidence = 1.0f;
        return true;
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0.0);

    cv::Mat binaryInv;
    cv::threshold(blurred, binaryInv, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::Mat ink = isolateDigitInk(binaryInv);
    if (ink.empty()) {
        const double rawRatio = foregroundRatio(binaryInv);
        confidence = rawRatio < 0.004 ? 0.99f : 0.70f;
        return rawRatio < 0.004;
    }

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int components = cv::connectedComponentsWithStats(ink, labels, stats, centroids, 8, CV_32S);
    int foregroundPixels = 0;
    int largestComponent = 0;
    cv::Rect bounds;
    for (int i = 1; i < components; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < 6) {
            continue;
        }
        foregroundPixels += area;
        largestComponent = std::max(largestComponent, area);
        const cv::Rect componentBounds(stats.at<int>(i, cv::CC_STAT_LEFT),
                                       stats.at<int>(i, cv::CC_STAT_TOP),
                                       stats.at<int>(i, cv::CC_STAT_WIDTH),
                                       stats.at<int>(i, cv::CC_STAT_HEIGHT));
        bounds = bounds.empty() ? componentBounds : bounds | componentBounds;
    }

    const double total = static_cast<double>(gray.rows) * static_cast<double>(gray.cols);
    const double foregroundRatio = total <= 0.0 ? 0.0 : static_cast<double>(foregroundPixels) / total;
    const bool tooSmall = bounds.empty()
        || bounds.height < gray.rows * 0.18
        || bounds.width < gray.cols * 0.045;
    const bool empty = foregroundPixels < 12 || foregroundRatio < 0.0035 || largestComponent < 9 || tooSmall;
    confidence = empty ? 0.90f : 0.0f;
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
