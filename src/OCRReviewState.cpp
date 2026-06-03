#include "OCRReviewState.h"

#include "Solver.h"

#include <algorithm>
#include <array>
#include <sstream>

namespace {
constexpr size_t indexOf(int row, int col) {
    return static_cast<size_t>(row * Board::Size + col);
}
}

OCRReviewState::OCRReviewState() {
    clear();
}

void OCRReviewState::clear() {
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            OCRCell& cell = cells_[indexOf(r, c)];
            cell = OCRCell{};
            cell.row = r;
            cell.col = c;
        }
    }
    selectedRow_ = -1;
    selectedCol_ = -1;
    rebuild();
    validationMessage_ = "OCR has not run.";
}

void OCRReviewState::loadResult(const OCRResult& result) {
    cells_ = result.cells;
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            OCRCell& cell = cells_[indexOf(r, c)];
            cell.row = r;
            cell.col = c;
        }
    }
    selectedRow_ = -1;
    selectedCol_ = -1;
    rebuild();
}

void OCRReviewState::editCell(int row, int col, int digit) {
    if (!Board::isInside(row, col) || digit < 1 || digit > 9) {
        return;
    }
    OCRCell& cell = cellAt(row, col);
    cell.digit = digit;
    cell.isEmpty = false;
    cell.lowConfidence = false;
    cell.confidence = 1.0f;
    cell.rawText = std::to_string(digit);
    rebuild();
}

void OCRReviewState::clearCell(int row, int col) {
    if (!Board::isInside(row, col)) {
        return;
    }
    OCRCell& cell = cellAt(row, col);
    cell.digit = 0;
    cell.isEmpty = true;
    cell.lowConfidence = false;
    cell.confidence = 1.0f;
    cell.rawText.clear();
    rebuild();
}

void OCRReviewState::selectCell(int row, int col) {
    if (Board::isInside(row, col)) {
        selectedRow_ = row;
        selectedCol_ = col;
    }
}

bool OCRReviewState::selectedCell(int& row, int& col) const {
    if (!Board::isInside(selectedRow_, selectedCol_)) {
        return false;
    }
    row = selectedRow_;
    col = selectedCol_;
    return true;
}

const std::array<OCRCell, 81>& OCRReviewState::cells() const {
    return cells_;
}

std::string OCRReviewState::puzzleString() const {
    return puzzleString_;
}

Board OCRReviewState::toBoard() const {
    std::array<std::array<int, Board::Size>, Board::Size> values{};
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            values[static_cast<size_t>(r)][static_cast<size_t>(c)] = cellAt(r, c).digit;
        }
    }
    Board board;
    board.load(values, true);
    return board;
}

bool OCRReviewState::canConfirmImport() const {
    return puzzleString_.size() == 81 && conflictCount_ == 0;
}

int OCRReviewState::givens() const {
    return givens_;
}

int OCRReviewState::lowConfidenceCount() const {
    return lowConfidenceCount_;
}

int OCRReviewState::conflictCount() const {
    return conflictCount_;
}

int OCRReviewState::selectedRow() const {
    return selectedRow_;
}

int OCRReviewState::selectedCol() const {
    return selectedCol_;
}

SolveResult OCRReviewState::validationResult() const {
    return validationResult_;
}

std::string OCRReviewState::validationMessage() const {
    return validationMessage_;
}

void OCRReviewState::rebuild() {
    puzzleString_.clear();
    puzzleString_.reserve(81);
    givens_ = 0;
    lowConfidenceCount_ = 0;
    conflictCount_ = 0;
    for (OCRCell& cell : cells_) {
        cell.conflict = false;
        if (cell.digit >= 1 && cell.digit <= 9) {
            puzzleString_.push_back(static_cast<char>('0' + cell.digit));
            ++givens_;
            if (cell.lowConfidence) {
                ++lowConfidenceCount_;
            }
        } else {
            cell.digit = 0;
            cell.isEmpty = true;
            cell.lowConfidence = false;
            puzzleString_.push_back('0');
        }
    }
    markConflicts();
    validateWithSolver();
}

void OCRReviewState::markConflicts() {
    auto markUnit = [&](const std::array<int, Board::Size>& indexes) {
        for (int n = 1; n <= 9; ++n) {
            int count = 0;
            for (int idx : indexes) {
                if (cells_[static_cast<size_t>(idx)].digit == n) {
                    ++count;
                }
            }
            if (count > 1) {
                for (int idx : indexes) {
                    if (cells_[static_cast<size_t>(idx)].digit == n) {
                        cells_[static_cast<size_t>(idx)].conflict = true;
                    }
                }
            }
        }
    };

    for (int r = 0; r < Board::Size; ++r) {
        std::array<int, Board::Size> indexes{};
        for (int c = 0; c < Board::Size; ++c) {
            indexes[static_cast<size_t>(c)] = r * Board::Size + c;
        }
        markUnit(indexes);
    }
    for (int c = 0; c < Board::Size; ++c) {
        std::array<int, Board::Size> indexes{};
        for (int r = 0; r < Board::Size; ++r) {
            indexes[static_cast<size_t>(r)] = r * Board::Size + c;
        }
        markUnit(indexes);
    }
    for (int br = 0; br < Board::Size; br += Board::BoxSize) {
        for (int bc = 0; bc < Board::Size; bc += Board::BoxSize) {
            std::array<int, Board::Size> indexes{};
            int i = 0;
            for (int dr = 0; dr < Board::BoxSize; ++dr) {
                for (int dc = 0; dc < Board::BoxSize; ++dc) {
                    indexes[static_cast<size_t>(i++)] = (br + dr) * Board::Size + (bc + dc);
                }
            }
            markUnit(indexes);
        }
    }

    conflictCount_ = 0;
    for (const OCRCell& cell : cells_) {
        if (cell.conflict) {
            ++conflictCount_;
        }
    }
}

void OCRReviewState::validateWithSolver() {
    if (conflictCount_ > 0) {
        validationResult_ = SolveResult::InvalidInput;
        validationMessage_ = "Invalid conflict. Correct red cells before confirming.";
        return;
    }
    if (givens_ == 0) {
        validationResult_ = SolveResult::MultipleSolutions;
        validationMessage_ = "Multiple solutions. Empty puzzles are allowed but not useful.";
        return;
    }

    Board board = toBoard();
    std::string reason;
    if (!board.validateInitial(&reason)) {
        validationResult_ = SolveResult::InvalidInput;
        validationMessage_ = reason;
        return;
    }

    Solver solver;
    validationResult_ = solver.solveUniqueOrMultiple(board, SolverMode::Turbo);
    switch (validationResult_) {
    case SolveResult::InvalidInput:
        validationMessage_ = "Invalid conflict. Correct red cells before confirming.";
        break;
    case SolveResult::NoSolution:
        validationMessage_ = "No solution. Import is allowed, but review OCR mistakes first.";
        break;
    case SolveResult::SolvedUnique:
        validationMessage_ = "Unique solution. Review low-confidence cells, then confirm.";
        break;
    case SolveResult::MultipleSolutions:
        validationMessage_ = "Multiple solutions. Import is allowed, but clues may be missing.";
        break;
    }
}

OCRCell& OCRReviewState::cellAt(int row, int col) {
    return cells_[indexOf(row, col)];
}

const OCRCell& OCRReviewState::cellAt(int row, int col) const {
    return cells_[indexOf(row, col)];
}
