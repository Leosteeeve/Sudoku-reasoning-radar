#pragma once

#include <string>
#include <vector>

struct OcrSidecarRequest {
    int version = 0;
    std::string operation;
    std::string imagePath;
};

struct OcrSidecarCell {
    int digit = 0;
    double confidence = 0.0;
    bool lowConfidence = false;
};

bool parseOcrSidecarRequest(const std::string& line,
                            OcrSidecarRequest& request,
                            std::string& error);
std::string serializeOcrSidecarSuccess(const std::string& puzzle,
                                       const std::vector<OcrSidecarCell>& cells);
std::string serializeOcrSidecarError(const std::string& error);
