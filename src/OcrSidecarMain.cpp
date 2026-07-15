#include "OcrSidecarProtocol.h"

#ifdef SRR_OCR_ENABLED
#include "OCRImport.h"
#endif

#include <iostream>
#include <string>
#include <vector>

int main() {
    std::string line;
    if (!std::getline(std::cin, line) || line.size() > 64 * 1024) {
        std::cout << serializeOcrSidecarError("invalid request") << '\n';
        return 0;
    }
    OcrSidecarRequest request;
    std::string error;
    if (!parseOcrSidecarRequest(line, request, error)) {
        std::cout << serializeOcrSidecarError(error) << '\n';
        return 0;
    }
#ifdef SRR_OCR_ENABLED
    OCRImport importer;
    OCRResult result;
    OCRProcessOptions options;
    options.debug = false;
    if (!importer.autoProcess(request.imagePath, result, options)) {
        std::cout << serializeOcrSidecarError(result.errorMessage) << '\n';
        return 0;
    }
    std::vector<OcrSidecarCell> cells;
    cells.reserve(result.cells.size());
    for (const OCRCell& cell : result.cells) {
        cells.push_back(OcrSidecarCell{cell.digit, cell.confidence * 100.0, cell.lowConfidence});
    }
    std::cout << serializeOcrSidecarSuccess(result.puzzleString, cells) << '\n';
#else
    std::cout << serializeOcrSidecarError("recognizer-unavailable") << '\n';
#endif
    return 0;
}
