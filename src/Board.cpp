#include "Board.h"

#include <algorithm>
#include <iterator>
#include <sstream>

Board::Board() {
    clear();
}

void Board::clear() {
    for (int r = 0; r < Size; ++r) {
        rowMask[r] = 0;
        colMask[r] = 0;
        boxMask[r] = 0;
        for (int c = 0; c < Size; ++c) {
            grid[r][c] = 0;
            candidateMask[r][c] = AllMask;
            fixed[r][c] = false;
        }
    }
}

bool Board::load(const int puzzle[Size][Size], bool markGivens) {
    clear();
    bool valuesInRange = true;
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            const int value = puzzle[r][c];
            if (value < 0 || value > 9) {
                valuesInRange = false;
                continue;
            }
            grid[r][c] = value;
            fixed[r][c] = markGivens && value != 0;
        }
    }
    rebuildMasksAndCandidates();
    return valuesInRange;
}

bool Board::load(const std::array<std::array<int, Size>, Size>& puzzle, bool markGivens) {
    int raw[Size][Size] = {};
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            raw[r][c] = puzzle[r][c];
        }
    }
    return load(raw, markGivens);
}

bool Board::setCellValue(int row, int col, int number, bool markFixed) {
    if (!isInside(row, col) || number < 0 || number > 9) {
        return false;
    }

    grid[row][col] = number;
    fixed[row][col] = markFixed && number != 0;
    rebuildMasksAndCandidates();
    return true;
}

bool Board::placeNumber(int row, int col, int number, bool markFixed) {
    if (!isInside(row, col) || number < 1 || number > 9) {
        return false;
    }
    if (grid[row][col] != 0) {
        return grid[row][col] == number;
    }

    const int bit = bitForNumber(number);
    const int box = boxId(row, col);
    if ((rowMask[row] & bit) || (colMask[col] & bit) || (boxMask[box] & bit)) {
        return false;
    }

    grid[row][col] = number;
    fixed[row][col] = markFixed;
    rowMask[row] |= bit;
    colMask[col] |= bit;
    boxMask[box] |= bit;
    updateCandidatesAfterPlacement(row, col, number);
    return true;
}

void Board::removeNumber(int row, int col, bool clearFixed) {
    if (!isInside(row, col)) {
        return;
    }
    grid[row][col] = 0;
    if (clearFixed) {
        fixed[row][col] = false;
    }
    rebuildMasksAndCandidates();
}

int Board::getCell(int row, int col) const {
    return isInside(row, col) ? grid[row][col] : 0;
}

bool Board::isFixed(int row, int col) const {
    return isInside(row, col) && fixed[row][col];
}

int Board::getUsedMask(int row, int col) const {
    if (!isInside(row, col)) {
        return 0;
    }
    return rowMask[row] | colMask[col] | boxMask[boxId(row, col)];
}

int Board::getCandidates(int row, int col) const {
    if (!isInside(row, col) || grid[row][col] != 0) {
        return 0;
    }
    return candidateMask[row][col] & AllMask;
}

int Board::getBaseCandidates(int row, int col) const {
    if (!isInside(row, col) || grid[row][col] != 0) {
        return 0;
    }
    const int used = getUsedMask(row, col);
    return (~used) & AllMask;
}

int Board::getCandidateCount(int row, int col) const {
    return popCount(getCandidates(row, col));
}

void Board::initializeCandidates() {
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            candidateMask[r][c] = (grid[r][c] == 0) ? getBaseCandidates(r, c) : 0;
        }
    }
}

int Board::removeCandidates(int row, int col, int mask) {
    if (!isInside(row, col) || grid[row][col] != 0) {
        return 0;
    }
    const int before = candidateMask[row][col] & AllMask;
    const int removed = before & mask;
    candidateMask[row][col] = before & ~mask & AllMask;
    return removed;
}

bool Board::setCandidateMask(int row, int col, int mask) {
    if (!isInside(row, col) || grid[row][col] != 0) {
        return false;
    }
    candidateMask[row][col] = mask & AllMask;
    return true;
}

int Board::emptyCount() const {
    int count = 0;
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (grid[r][c] == 0) {
                ++count;
            }
        }
    }
    return count;
}

bool Board::isSolved() const {
    if (!validateInitial()) {
        return false;
    }
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (grid[r][c] == 0) {
                return false;
            }
        }
    }
    return true;
}

bool Board::hasContradiction(std::string* reason, int* row, int* col) const {
    std::string validationReason;
    int vr = -1;
    int vc = -1;
    if (!validateInitial(&validationReason, &vr, &vc, nullptr, nullptr)) {
        if (reason) {
            *reason = validationReason;
        }
        if (row) {
            *row = vr;
        }
        if (col) {
            *col = vc;
        }
        return true;
    }

    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (grid[r][c] == 0 && getCandidates(r, c) == 0) {
                if (reason) {
                    std::ostringstream out;
                    out << "Cell r" << (r + 1) << "c" << (c + 1)
                        << " has no legal candidates.";
                    *reason = out.str();
                }
                if (row) {
                    *row = r;
                }
                if (col) {
                    *col = c;
                }
                return true;
            }
        }
    }
    return false;
}

bool Board::validateInitial(std::string* reason,
                            int* row,
                            int* col,
                            int* relatedRow,
                            int* relatedCol) const {
    auto setConflict = [&](int r1, int c1, int r2, int c2, const std::string& scope) {
        if (reason) {
            std::ostringstream out;
            out << "Duplicate " << grid[r1][c1] << " in " << scope << ".";
            *reason = out.str();
        }
        if (row) {
            *row = r1;
        }
        if (col) {
            *col = c1;
        }
        if (relatedRow) {
            *relatedRow = r2;
        }
        if (relatedCol) {
            *relatedCol = c2;
        }
    };

    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (grid[r][c] < 0 || grid[r][c] > 9) {
                if (reason) {
                    *reason = "Grid contains a value outside 0-9.";
                }
                if (row) {
                    *row = r;
                }
                if (col) {
                    *col = c;
                }
                return false;
            }
        }
    }

    for (int r = 0; r < Size; ++r) {
        int seen[10];
        std::fill(std::begin(seen), std::end(seen), -1);
        for (int c = 0; c < Size; ++c) {
            const int n = grid[r][c];
            if (n == 0) {
                continue;
            }
            if (seen[n] != -1) {
                setConflict(r, c, r, seen[n], "row");
                return false;
            }
            seen[n] = c;
        }
    }

    for (int c = 0; c < Size; ++c) {
        int seen[10];
        std::fill(std::begin(seen), std::end(seen), -1);
        for (int r = 0; r < Size; ++r) {
            const int n = grid[r][c];
            if (n == 0) {
                continue;
            }
            if (seen[n] != -1) {
                setConflict(r, c, seen[n], c, "column");
                return false;
            }
            seen[n] = r;
        }
    }

    for (int br = 0; br < Size; br += BoxSize) {
        for (int bc = 0; bc < Size; bc += BoxSize) {
            int seenR[10];
            int seenC[10];
            std::fill(std::begin(seenR), std::end(seenR), -1);
            std::fill(std::begin(seenC), std::end(seenC), -1);
            for (int dr = 0; dr < BoxSize; ++dr) {
                for (int dc = 0; dc < BoxSize; ++dc) {
                    const int r = br + dr;
                    const int c = bc + dc;
                    const int n = grid[r][c];
                    if (n == 0) {
                        continue;
                    }
                    if (seenR[n] != -1) {
                        setConflict(r, c, seenR[n], seenC[n], "3x3 box");
                        return false;
                    }
                    seenR[n] = r;
                    seenC[n] = c;
                }
            }
        }
    }

    return true;
}

bool Board::isInside(int row, int col) {
    return row >= 0 && row < Size && col >= 0 && col < Size;
}

int Board::boxId(int row, int col) {
    return (row / BoxSize) * BoxSize + (col / BoxSize);
}

int Board::bitForNumber(int number) {
    return 1 << (number - 1);
}

int Board::numberFromBit(int bit) {
    for (int n = 1; n <= 9; ++n) {
        if (bit == bitForNumber(n)) {
            return n;
        }
    }
    return 0;
}

int Board::popCount(int mask) {
    int count = 0;
    while (mask != 0) {
        mask &= (mask - 1);
        ++count;
    }
    return count;
}

bool Board::isSingle(int mask) {
    return mask != 0 && (mask & (mask - 1)) == 0;
}

void Board::rebuildMasks() {
    std::fill(std::begin(rowMask), std::end(rowMask), 0);
    std::fill(std::begin(colMask), std::end(colMask), 0);
    std::fill(std::begin(boxMask), std::end(boxMask), 0);

    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            const int n = grid[r][c];
            if (n < 1 || n > 9) {
                continue;
            }
            const int bit = bitForNumber(n);
            rowMask[r] |= bit;
            colMask[c] |= bit;
            boxMask[boxId(r, c)] |= bit;
        }
    }
}

void Board::rebuildMasksAndCandidates() {
    rebuildMasks();
    initializeCandidates();
}

void Board::updateCandidatesAfterPlacement(int row, int col, int number) {
    const int bit = bitForNumber(number);
    candidateMask[row][col] = 0;

    for (int i = 0; i < Size; ++i) {
        if (grid[row][i] == 0) {
            candidateMask[row][i] &= ~bit;
        }
        if (grid[i][col] == 0) {
            candidateMask[i][col] &= ~bit;
        }
    }

    const int br = (row / BoxSize) * BoxSize;
    const int bc = (col / BoxSize) * BoxSize;
    for (int dr = 0; dr < BoxSize; ++dr) {
        for (int dc = 0; dc < BoxSize; ++dc) {
            const int r = br + dr;
            const int c = bc + dc;
            if (grid[r][c] == 0) {
                candidateMask[r][c] &= ~bit;
            }
        }
    }
}
