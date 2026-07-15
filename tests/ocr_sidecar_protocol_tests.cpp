#include "OcrSidecarProtocol.h"

#include <cassert>
#include <string>
#include <vector>

int main() {
    OcrSidecarRequest request;
    std::string error;
    assert(parseOcrSidecarRequest(
        R"({"version":1,"operation":"recognize","imagePath":"C:\\Sudoku\\grid.png"})",
        request,
        error));
    assert(request.imagePath == "C:\\Sudoku\\grid.png");
    assert(!parseOcrSidecarRequest(
        R"({"version":1,"operation":"recognize","imagePath":"grid.png","shell":"cmd.exe"})",
        request,
        error));
    assert(!parseOcrSidecarRequest(
        R"({"version":2,"operation":"recognize","imagePath":"grid.png"})",
        request,
        error));

    std::vector<OcrSidecarCell> cells(81);
    cells[0] = OcrSidecarCell{5, 92.5, false};
    const std::string puzzle = "5" + std::string(80, '0');
    const std::string success = serializeOcrSidecarSuccess(puzzle, cells);
    assert(success.find(R"("version":1,"ok":true)") != std::string::npos);
    assert(success.find(R"("digit":5,"confidence":92.5,"lowConfidence":false)") != std::string::npos);
    assert(serializeOcrSidecarError("bad \"image\"") ==
           "{\"version\":1,\"ok\":false,\"error\":\"bad \\\"image\\\"\"}");
    return 0;
}
