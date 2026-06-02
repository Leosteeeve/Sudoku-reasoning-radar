#include "DLXSolver.h"

#include <chrono>

namespace {
int cellConstraint(int row, int col) {
    return row * 9 + col;
}

int rowDigitConstraint(int row, int number) {
    return 81 + row * 9 + (number - 1);
}

int colDigitConstraint(int col, int number) {
    return 162 + col * 9 + (number - 1);
}

int boxDigitConstraint(int row, int col, int number) {
    return 243 + Board::boxId(row, col) * 9 + (number - 1);
}
}

SolveResult DLXSolver::solve(const Board& input) {
    reset();
    sourceBoard = input;

    std::string reason;
    if (!input.validateInitial(&reason)) {
        return SolveResult::InvalidInput;
    }

    const auto start = std::chrono::steady_clock::now();
    if (!buildExactCoverRows(input)) {
        elapsedMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
        return SolveResult::NoSolution;
    }

    std::vector<int> chosenRows;
    for (int rowIndex = 0; rowIndex < static_cast<int>(placements.size()); ++rowIndex) {
        const Placement& p = placements[static_cast<size_t>(rowIndex)];
        if (input.getCell(p.row, p.col) == p.number && !choosePlacement(rowIndex, chosenRows)) {
            elapsedMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
            return SolveResult::NoSolution;
        }
    }

    search(chosenRows);
    elapsedMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());

    if (solutionCount == 0) {
        return SolveResult::NoSolution;
    }
    return solutionCount == 1 ? SolveResult::SolvedUnique : SolveResult::MultipleSolutions;
}

const Board& DLXSolver::getFinalBoard() const {
    return finalBoard;
}

int DLXSolver::getSolutionCount() const {
    return solutionCount;
}

int DLXSolver::getElapsedMs() const {
    return elapsedMs;
}

void DLXSolver::reset() {
    placements.clear();
    for (auto& rows : constraintRows) {
        rows.clear();
    }
    for (bool& value : covered) {
        value = false;
    }
    finalBoard.clear();
    sourceBoard.clear();
    solutionCount = 0;
    elapsedMs = 0;
}

bool DLXSolver::buildExactCoverRows(const Board& input) {
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int given = input.getCell(r, c);
            const int mask = given == 0 ? input.getBaseCandidates(r, c) : Board::bitForNumber(given);
            if (mask == 0) {
                return false;
            }

            for (int n = 1; n <= 9; ++n) {
                if ((mask & Board::bitForNumber(n)) == 0) {
                    continue;
                }

                Placement placement;
                placement.row = r;
                placement.col = c;
                placement.number = n;
                placement.constraints = {
                    cellConstraint(r, c),
                    rowDigitConstraint(r, n),
                    colDigitConstraint(c, n),
                    boxDigitConstraint(r, c, n)
                };

                const int rowIndex = static_cast<int>(placements.size());
                placements.push_back(placement);
                for (int constraint : placement.constraints) {
                    constraintRows[constraint].push_back(rowIndex);
                }
            }
        }
    }
    return true;
}

bool DLXSolver::choosePlacement(int rowIndex, std::vector<int>& chosenRows) {
    if (!rowIsAvailable(rowIndex)) {
        return false;
    }
    for (int constraint : placements[static_cast<size_t>(rowIndex)].constraints) {
        covered[constraint] = true;
    }
    chosenRows.push_back(rowIndex);
    return true;
}

void DLXSolver::unchoosePlacement(int rowIndex) {
    for (int constraint : placements[static_cast<size_t>(rowIndex)].constraints) {
        covered[constraint] = false;
    }
}

bool DLXSolver::search(std::vector<int>& chosenRows) {
    if (solutionCount >= 2) {
        return true;
    }

    const int constraint = chooseBestConstraint();
    if (constraint == -1) {
        ++solutionCount;
        if (solutionCount == 1) {
            buildBoardFromRows(sourceBoard, chosenRows);
        }
        return solutionCount >= 2;
    }
    if (constraint == -2) {
        return false;
    }

    for (int rowIndex : constraintRows[constraint]) {
        if (!choosePlacement(rowIndex, chosenRows)) {
            continue;
        }
        search(chosenRows);
        chosenRows.pop_back();
        unchoosePlacement(rowIndex);
        if (solutionCount >= 2) {
            return true;
        }
    }
    return false;
}

int DLXSolver::chooseBestConstraint() const {
    int bestConstraint = -1;
    int bestCount = 1000;

    for (int constraint = 0; constraint < 324; ++constraint) {
        if (covered[constraint]) {
            continue;
        }

        int count = 0;
        for (int rowIndex : constraintRows[constraint]) {
            if (rowIsAvailable(rowIndex)) {
                ++count;
            }
        }
        if (count == 0) {
            return -2;
        }
        if (count < bestCount) {
            bestCount = count;
            bestConstraint = constraint;
        }
    }
    return bestConstraint;
}

bool DLXSolver::rowIsAvailable(int rowIndex) const {
    for (int constraint : placements[static_cast<size_t>(rowIndex)].constraints) {
        if (covered[constraint]) {
            return false;
        }
    }
    return true;
}

void DLXSolver::buildBoardFromRows(const Board& input, const std::vector<int>& chosenRows) {
    finalBoard = input;
    for (int rowIndex : chosenRows) {
        const Placement& p = placements[static_cast<size_t>(rowIndex)];
        if (finalBoard.getCell(p.row, p.col) == 0) {
            finalBoard.placeNumber(p.row, p.col, p.number, false);
        }
    }
}
