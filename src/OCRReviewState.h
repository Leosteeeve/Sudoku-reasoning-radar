#pragma once

#include "Board.h"
#include "StepRecorder.h"

#include <array>
#include <string>

#include <opencv2/core.hpp>

struct OCRCell {
    int row = 0;
    int col = 0;
    int digit = 0;
    float confidence = 0.0f;
    bool isEmpty = true;
    bool lowConfidence = false;
    bool conflict = false;
    std::string rawText;
};

struct OCRResult {
    bool success = false;
    std::string errorMessage;
    std::string puzzleString;
    std::array<OCRCell, 81> cells{};
    cv::Mat originalPreview;
    cv::Mat warpedGrid;
    int givens = 0;
    int lowConfidenceCount = 0;
    int conflictCount = 0;
    double processingMs = 0.0;
    SolveResult validationResult = SolveResult::NoSolution;
    std::string validationMessage = "OCR has not run.";
};

class OCRReviewState {
public:
    OCRReviewState();

    void clear();
    void loadResult(const OCRResult& result);
    void editCell(int row, int col, int digit);
    void clearCell(int row, int col);
    void selectCell(int row, int col);
    bool selectedCell(int& row, int& col) const;

    const std::array<OCRCell, 81>& cells() const;
    std::string puzzleString() const;
    Board toBoard() const;
    bool canConfirmImport() const;

    int givens() const;
    int lowConfidenceCount() const;
    int conflictCount() const;
    int selectedRow() const;
    int selectedCol() const;
    SolveResult validationResult() const;
    std::string validationMessage() const;

private:
    void rebuild();
    void markConflicts();
    void validateWithSolver();
    OCRCell& cellAt(int row, int col);
    const OCRCell& cellAt(int row, int col) const;

    std::array<OCRCell, 81> cells_{};
    std::string puzzleString_;
    int givens_ = 0;
    int lowConfidenceCount_ = 0;
    int conflictCount_ = 0;
    int selectedRow_ = -1;
    int selectedCol_ = -1;
    SolveResult validationResult_ = SolveResult::NoSolution;
    std::string validationMessage_ = "OCR has not run.";
};
