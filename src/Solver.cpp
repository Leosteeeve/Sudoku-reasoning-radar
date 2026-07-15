#include "Solver.h"

#include "DLXSolver.h"

#include <chrono>
#include <sstream>

SolveResult Solver::solve(const Board& input, SolverMode mode) {
    return solveUniqueOrMultiple(input, mode);
}

SolveResult Solver::solveUniqueOrMultiple(const Board& input, SolverMode mode) {
    recorder.clear();
    finalBoard.clear();
    solutionCount = 0;
    lastSolveMs = 0;
    activeMode = mode;

    const auto start = std::chrono::steady_clock::now();
    recorder.addStep(StepType::ModeChanged, -1, -1, 0, -1, -1, 0,
                     "Solver mode: " + modeName(mode));

    if (mode == SolverMode::Turbo) {
        return solveWithTurbo(input);
    }

    std::string reason;
    int row = -1;
    int col = -1;
    int relatedRow = -1;
    int relatedCol = -1;
    if (!input.validateInitial(&reason, &row, &col, &relatedRow, &relatedCol)) {
        recorder.addStep(StepType::InvalidInput, row, col, 0, relatedRow, relatedCol, 0, reason);
        return SolveResult::InvalidInput;
    }

    Board board = input;
    board.initializeCandidates();
    if (board.hasContradiction(&reason, &row, &col)) {
        recorder.addStep(StepType::Contradiction, row, col, 0, -1, -1, 0, reason);
        recorder.addStep(StepType::NoSolution, row, col, 0, -1, -1, 0,
                         "Initial constraints leave at least one cell impossible.");
        return SolveResult::NoSolution;
    }

    const SolveResult result = (mode == SolverMode::HumanLogic)
        ? solveWithLogicOnly(board)
        : solveWithSearch(board);
    lastSolveMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
    return result;
}

bool Solver::applyNakedSingles(Board& board, int depth) {
    return nakedSingles(board, depth) == MoveResult::Changed;
}

bool Solver::applyHiddenSingles(Board& board, int depth) {
    return hiddenSingles(board, depth) == MoveResult::Changed;
}

bool Solver::applyLockedCandidates(Board& board, int depth) {
    return lockedCandidates(board, depth) == MoveResult::Changed;
}

bool Solver::applyBoxLineReduction(Board& board, int depth) {
    return boxLineReduction(board, depth) == MoveResult::Changed;
}

bool Solver::applyNakedPairs(Board& board, int depth) {
    return nakedPairs(board, depth) == MoveResult::Changed;
}

bool Solver::applyHiddenPairs(Board& board, int depth) {
    return hiddenPairs(board, depth) == MoveResult::Changed;
}

bool Solver::applyXWing(Board& board, int depth) {
    return xWing(board, depth) == MoveResult::Changed;
}

bool Solver::search(Board& board, int depth) {
    if (solutionCount >= 2) {
        return true;
    }

    const MoveResult propagated = propagate(board, depth);
    if (propagated == MoveResult::Contradiction) {
        return false;
    }

    if (board.isSolved()) {
        ++solutionCount;
        if (solutionCount == 1) {
            finalBoard = board;
        }
        std::ostringstream out;
        out << "Found complete solution #" << solutionCount << ".";
        recorder.addStep(StepType::Solved, -1, -1, 0, -1, -1, depth, out.str());
        return solutionCount >= 2;
    }

    const auto [row, col] = findMRVCell(board);
    if (row < 0 || col < 0 || board.getCandidates(row, col) == 0) {
        recordContradiction(row, col, 0, depth, "MRV could not find a viable empty cell.");
        return false;
    }

    const int candidates = board.getCandidates(row, col);
    for (int n : candidateNumbers(candidates)) {
        const int beforeSolutions = solutionCount;
        Board branch = board;

        std::ostringstream guessReason;
        guessReason << "MRV chose " << cellName(row, col) << " with "
                    << Board::popCount(candidates) << " candidates; trying " << n << ".";
        recorder.addStep(StepType::Guess, row, col, n, -1, -1, depth + 1, guessReason.str());

        if (!placeWithRecording(branch, row, col, n, StepType::Guess, depth + 1,
                                "Placed assumption for MRV search.")) {
            continue;
        }

        search(branch, depth + 1);
        if (solutionCount >= 2) {
            return true;
        }

        const std::string reason = (solutionCount == beforeSolutions)
            ? "Branch failed. Reverting assumption."
            : "Solution recorded; reverting branch to continue uniqueness check.";
        recordBacktrackDiff(board, branch, depth + 1, reason);
    }

    return solutionCount >= 2;
}

std::pair<int, int> Solver::findMRVCell(const Board& board) const {
    int bestCount = 10;
    std::pair<int, int> best{-1, -1};
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            if (board.getCell(r, c) != 0) {
                continue;
            }
            const int count = board.getCandidateCount(r, c);
            if (count < bestCount) {
                bestCount = count;
                best = {r, c};
            }
        }
    }
    return best;
}

const Board& Solver::getFinalBoard() const {
    return finalBoard;
}

const std::vector<SolveStep>& Solver::getSteps() const {
    return recorder.getSteps();
}

int Solver::getLastSolveMs() const {
    return lastSolveMs;
}

Solver::MoveResult Solver::propagate(Board& board, int depth) {
    while (true) {
        MoveResult result = nakedSingles(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = hiddenSingles(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = lockedCandidates(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = boxLineReduction(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = nakedPairs(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = hiddenPairs(board, depth);
        if (result != MoveResult::NoChange) {
            if (result == MoveResult::Contradiction) {
                return result;
            }
            continue;
        }
        result = xWing(board, depth);
        if (result == MoveResult::Contradiction) {
            return result;
        }
        if (result == MoveResult::NoChange) {
            break;
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::nakedSingles(Board& board, int depth) {
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            if (board.getCell(r, c) != 0) {
                continue;
            }

            const int candidates = board.getCandidates(r, c);
            std::ostringstream scan;
            scan << "Scanning " << cellName(r, c) << " with "
                 << Board::popCount(candidates) << " persistent candidate(s).";
            recorder.addStep(StepType::AnalyzeCell, r, c, 0, -1, -1, depth, scan.str());

            if (candidates == 0) {
                recordContradiction(r, c, 0, depth, cellName(r, c) + " has no candidates.");
                return MoveResult::Contradiction;
            }
            if (Board::isSingle(candidates)) {
                const int n = Board::numberFromBit(candidates);
                std::ostringstream reason;
                reason << "Naked Single: " << cellName(r, c) << " has only candidate " << n << ".";
                recorder.addStep(StepType::NakedSingle, r, c, n, -1, -1, depth, reason.str());
                return placeWithRecording(board, r, c, n, StepType::NakedSingle, depth,
                                          "Placed by Naked Single.")
                    ? MoveResult::Changed
                    : MoveResult::Contradiction;
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::hiddenSingles(Board& board, int depth) {
    for (UnitType unitType : {UnitType::Row, UnitType::Column, UnitType::Box}) {
        for (int unit = 0; unit < Board::Size; ++unit) {
            for (int n = 1; n <= 9; ++n) {
                if (unitHasDigit(board, unitType, unit, n)) {
                    continue;
                }

                int count = 0;
                int foundRow = -1;
                int foundCol = -1;
                for (const auto& cell : unitCells(unitType, unit)) {
                    if (board.getCell(cell.first, cell.second) == 0
                        && (board.getCandidates(cell.first, cell.second) & Board::bitForNumber(n)) != 0) {
                        ++count;
                        foundRow = cell.first;
                        foundCol = cell.second;
                    }
                }

                if (count == 0) {
                    std::ostringstream reason;
                    reason << n << " has no legal position in this unit.";
                    recordContradiction(-1, -1, n, depth, reason.str());
                    return MoveResult::Contradiction;
                }
                if (count == 1) {
                    std::ostringstream reason;
                    reason << "Hidden Single: digit " << n << " appears only at "
                           << cellName(foundRow, foundCol) << " in this unit.";
                    recorder.addStep(StepType::HiddenSingle,
                                     foundRow,
                                     foundCol,
                                     n,
                                     -1,
                                     -1,
                                     -1,
                                     -1,
                                     unitType,
                                     unit,
                                     0,
                                     0,
                                     0,
                                     depth,
                                     reason.str());
                    return placeWithRecording(board, foundRow, foundCol, n, StepType::HiddenSingle, depth,
                                              "Placed by Hidden Single.", unitType, unit)
                        ? MoveResult::Changed
                        : MoveResult::Contradiction;
                }
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::lockedCandidates(Board& board, int depth) {
    for (int box = 0; box < Board::Size; ++box) {
        const int br = (box / 3) * 3;
        const int bc = (box % 3) * 3;
        for (int n = 1; n <= 9; ++n) {
            int count = 0;
            int sameRow = -1;
            int sameCol = -1;
            bool oneRow = true;
            bool oneCol = true;
            for (int dr = 0; dr < 3; ++dr) {
                for (int dc = 0; dc < 3; ++dc) {
                    const int r = br + dr;
                    const int c = bc + dc;
                    if (board.getCell(r, c) == 0 && (board.getCandidates(r, c) & Board::bitForNumber(n))) {
                        if (count == 0) {
                            sameRow = r;
                            sameCol = c;
                        }
                        oneRow = oneRow && r == sameRow;
                        oneCol = oneCol && c == sameCol;
                        ++count;
                    }
                }
            }
            if (count < 2) {
                continue;
            }

            if (oneRow) {
                bool changed = false;
                std::ostringstream reason;
                reason << "Locked Candidate: digit " << n << " in box " << (box + 1)
                       << " is restricted to row " << (sameRow + 1)
                       << ", so it can be removed from that row outside the box.";
                recorder.addStep(StepType::LockedCandidate, sameRow, -1, n, br, bc, -1, -1,
                                 UnitType::Box, box, 0, 0, 0, depth, reason.str());
                for (int c = 0; c < Board::Size; ++c) {
                    if (Board::boxId(sameRow, c) != box) {
                        changed = removeCandidatesWithRecording(board, sameRow, c, Board::bitForNumber(n),
                                                                StepType::LockedCandidate, depth, reason.str(),
                                                                UnitType::Row, sameRow) || changed;
                    }
                }
                if (changed) {
                    return MoveResult::Changed;
                }
            }

            if (oneCol) {
                bool changed = false;
                std::ostringstream reason;
                reason << "Locked Candidate: digit " << n << " in box " << (box + 1)
                       << " is restricted to column " << (sameCol + 1)
                       << ", so it can be removed from that column outside the box.";
                recorder.addStep(StepType::LockedCandidate, -1, sameCol, n, br, bc, -1, -1,
                                 UnitType::Box, box, 0, 0, 0, depth, reason.str());
                for (int r = 0; r < Board::Size; ++r) {
                    if (Board::boxId(r, sameCol) != box) {
                        changed = removeCandidatesWithRecording(board, r, sameCol, Board::bitForNumber(n),
                                                                StepType::LockedCandidate, depth, reason.str(),
                                                                UnitType::Column, sameCol) || changed;
                    }
                }
                if (changed) {
                    return MoveResult::Changed;
                }
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::boxLineReduction(Board& board, int depth) {
    for (UnitType unitType : {UnitType::Row, UnitType::Column}) {
        for (int unit = 0; unit < Board::Size; ++unit) {
            for (int n = 1; n <= 9; ++n) {
                int count = 0;
                int targetBox = -1;
                bool sameBox = true;
                for (const auto& cell : unitCells(unitType, unit)) {
                    const int r = cell.first;
                    const int c = cell.second;
                    if (board.getCell(r, c) == 0 && (board.getCandidates(r, c) & Board::bitForNumber(n))) {
                        if (count == 0) {
                            targetBox = Board::boxId(r, c);
                        }
                        sameBox = sameBox && Board::boxId(r, c) == targetBox;
                        ++count;
                    }
                }
                if (count < 2 || !sameBox) {
                    continue;
                }

                bool changed = false;
                const int br = (targetBox / 3) * 3;
                const int bc = (targetBox % 3) * 3;
                std::ostringstream reason;
                reason << "Box-Line Reduction: digit " << n << " in this "
                       << (unitType == UnitType::Row ? "row" : "column")
                       << " is confined to box " << (targetBox + 1)
                       << ", so other cells in the box lose that candidate.";
                recorder.addStep(StepType::BoxLineReduction, -1, -1, n, br, bc, -1, -1,
                                 unitType, unit, 0, 0, 0, depth, reason.str());
                for (int dr = 0; dr < 3; ++dr) {
                    for (int dc = 0; dc < 3; ++dc) {
                        const int r = br + dr;
                        const int c = bc + dc;
                        const bool sameUnit = unitType == UnitType::Row ? r == unit : c == unit;
                        if (!sameUnit) {
                            changed = removeCandidatesWithRecording(board, r, c, Board::bitForNumber(n),
                                                                    StepType::BoxLineReduction, depth, reason.str(),
                                                                    UnitType::Box, targetBox) || changed;
                        }
                    }
                }
                if (changed) {
                    return MoveResult::Changed;
                }
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::nakedPairs(Board& board, int depth) {
    for (UnitType unitType : {UnitType::Row, UnitType::Column, UnitType::Box}) {
        for (int unit = 0; unit < Board::Size; ++unit) {
            const auto cells = unitCells(unitType, unit);
            for (int i = 0; i < static_cast<int>(cells.size()); ++i) {
                const int r1 = cells[static_cast<size_t>(i)].first;
                const int c1 = cells[static_cast<size_t>(i)].second;
                const int mask = board.getCandidates(r1, c1);
                if (Board::popCount(mask) != 2) {
                    continue;
                }
                for (int j = i + 1; j < static_cast<int>(cells.size()); ++j) {
                    const int r2 = cells[static_cast<size_t>(j)].first;
                    const int c2 = cells[static_cast<size_t>(j)].second;
                    if (board.getCandidates(r2, c2) != mask) {
                        continue;
                    }

                    bool changed = false;
                    std::ostringstream reason;
                    reason << "Naked Pair: " << cellName(r1, c1) << " and " << cellName(r2, c2)
                           << " share " << maskName(mask) << ", removing those digits from the rest of the unit.";
                    recorder.addStep(StepType::NakedPair, r1, c1, 0, r2, c2, r2, c2,
                                     unitType, unit, mask, mask, 0, depth, reason.str());
                    for (const auto& cell : cells) {
                        if ((cell.first == r1 && cell.second == c1) || (cell.first == r2 && cell.second == c2)) {
                            continue;
                        }
                        changed = removeCandidatesWithRecording(board, cell.first, cell.second, mask,
                                                                StepType::NakedPair, depth, reason.str(),
                                                                unitType, unit, r1, c1, r2, c2) || changed;
                    }
                    if (changed) {
                        return MoveResult::Changed;
                    }
                }
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::hiddenPairs(Board& board, int depth) {
    for (UnitType unitType : {UnitType::Row, UnitType::Column, UnitType::Box}) {
        for (int unit = 0; unit < Board::Size; ++unit) {
            const auto cells = unitCells(unitType, unit);
            for (int a = 1; a <= 8; ++a) {
                for (int b = a + 1; b <= 9; ++b) {
                    std::vector<std::pair<int, int>> positionsA;
                    std::vector<std::pair<int, int>> positionsB;
                    for (const auto& cell : cells) {
                        const int mask = board.getCandidates(cell.first, cell.second);
                        if (mask & Board::bitForNumber(a)) {
                            positionsA.push_back(cell);
                        }
                        if (mask & Board::bitForNumber(b)) {
                            positionsB.push_back(cell);
                        }
                    }
                    if (positionsA.size() != 2 || positionsB != positionsA) {
                        continue;
                    }

                    const int keepMask = Board::bitForNumber(a) | Board::bitForNumber(b);
                    bool changed = false;
                    std::ostringstream reason;
                    reason << "Hidden Pair: digits " << a << " and " << b
                           << " appear only in the same two cells, so other candidates are removed there.";
                    recorder.addStep(StepType::HiddenPair,
                                     positionsA[0].first,
                                     positionsA[0].second,
                                     0,
                                     positionsA[1].first,
                                     positionsA[1].second,
                                     positionsA[1].first,
                                     positionsA[1].second,
                                     unitType,
                                     unit,
                                     0,
                                     keepMask,
                                     0,
                                     depth,
                                     reason.str());
                    for (const auto& cell : positionsA) {
                        const int removeMask = board.getCandidates(cell.first, cell.second) & ~keepMask;
                        changed = removeCandidatesWithRecording(board, cell.first, cell.second, removeMask,
                                                                StepType::HiddenPair, depth, reason.str(),
                                                                unitType, unit,
                                                                positionsA[0].first, positionsA[0].second,
                                                                positionsA[1].first, positionsA[1].second) || changed;
                    }
                    if (changed) {
                        return MoveResult::Changed;
                    }
                }
            }
        }
    }
    return MoveResult::NoChange;
}

Solver::MoveResult Solver::xWing(Board& board, int depth) {
    for (int n = 1; n <= 9; ++n) {
        for (int r1 = 0; r1 < 8; ++r1) {
            std::vector<int> cols1;
            for (int c = 0; c < 9; ++c) {
                if (board.getCandidates(r1, c) & Board::bitForNumber(n)) {
                    cols1.push_back(c);
                }
            }
            if (cols1.size() != 2) {
                continue;
            }
            for (int r2 = r1 + 1; r2 < 9; ++r2) {
                std::vector<int> cols2;
                for (int c = 0; c < 9; ++c) {
                    if (board.getCandidates(r2, c) & Board::bitForNumber(n)) {
                        cols2.push_back(c);
                    }
                }
                if (cols2 != cols1) {
                    continue;
                }

                bool changed = false;
                std::ostringstream reason;
                reason << "X-Wing: digit " << n << " forms a row-based rectangle, removing it from the two columns.";
                recorder.addStep(StepType::XWing, r1, cols1[0], n, r2, cols1[1], r2, cols1[1],
                                 UnitType::Row, r1, 0, 0, 0, depth, reason.str());
                for (int r = 0; r < 9; ++r) {
                    if (r == r1 || r == r2) {
                        continue;
                    }
                    changed = removeCandidatesWithRecording(board, r, cols1[0], Board::bitForNumber(n),
                                                            StepType::XWing, depth, reason.str(),
                                                            UnitType::Column, cols1[0], r1, cols1[0], r2, cols1[1]) || changed;
                    changed = removeCandidatesWithRecording(board, r, cols1[1], Board::bitForNumber(n),
                                                            StepType::XWing, depth, reason.str(),
                                                            UnitType::Column, cols1[1], r1, cols1[0], r2, cols1[1]) || changed;
                }
                if (changed) {
                    return MoveResult::Changed;
                }
            }
        }

        for (int c1 = 0; c1 < 8; ++c1) {
            std::vector<int> rows1;
            for (int r = 0; r < 9; ++r) {
                if (board.getCandidates(r, c1) & Board::bitForNumber(n)) {
                    rows1.push_back(r);
                }
            }
            if (rows1.size() != 2) {
                continue;
            }
            for (int c2 = c1 + 1; c2 < 9; ++c2) {
                std::vector<int> rows2;
                for (int r = 0; r < 9; ++r) {
                    if (board.getCandidates(r, c2) & Board::bitForNumber(n)) {
                        rows2.push_back(r);
                    }
                }
                if (rows2 != rows1) {
                    continue;
                }

                bool changed = false;
                std::ostringstream reason;
                reason << "X-Wing: digit " << n << " forms a column-based rectangle, removing it from the two rows.";
                recorder.addStep(StepType::XWing, rows1[0], c1, n, rows1[1], c2, rows1[1], c2,
                                 UnitType::Column, c1, 0, 0, 0, depth, reason.str());
                for (int c = 0; c < 9; ++c) {
                    if (c == c1 || c == c2) {
                        continue;
                    }
                    changed = removeCandidatesWithRecording(board, rows1[0], c, Board::bitForNumber(n),
                                                            StepType::XWing, depth, reason.str(),
                                                            UnitType::Row, rows1[0], rows1[0], c1, rows1[1], c2) || changed;
                    changed = removeCandidatesWithRecording(board, rows1[1], c, Board::bitForNumber(n),
                                                            StepType::XWing, depth, reason.str(),
                                                            UnitType::Row, rows1[1], rows1[0], c1, rows1[1], c2) || changed;
                }
                if (changed) {
                    return MoveResult::Changed;
                }
            }
        }
    }
    return MoveResult::NoChange;
}

SolveResult Solver::solveWithLogicOnly(Board& board) {
    const MoveResult result = propagate(board, 0);
    if (result == MoveResult::Contradiction) {
        recorder.addStep(StepType::NoSolution, -1, -1, 0, -1, -1, 0, "Human logic reached a contradiction.");
        return SolveResult::NoSolution;
    }
    if (board.isSolved()) {
        finalBoard = board;
        solutionCount = 1;
        recorder.addStep(StepType::Solved, -1, -1, 0, -1, -1, 0,
                         "Human logic solved the puzzle without guessing.");
        return SolveResult::SolvedUnique;
    }

    recorder.addStep(StepType::NoSolution, -1, -1, 0, -1, -1, 0,
                     "Human logic stopped: no available logical move found.");
    return SolveResult::NoSolution;
}

SolveResult Solver::solveWithSearch(Board& board) {
    search(board, 0);
    if (solutionCount == 0) {
        recorder.addStep(StepType::NoSolution, -1, -1, 0, -1, -1, 0, "All MRV branches failed.");
        return SolveResult::NoSolution;
    }
    if (solutionCount == 1) {
        recorder.addStep(StepType::Solved, -1, -1, 0, -1, -1, 0, "Final unique solution ready.");
        return SolveResult::SolvedUnique;
    }
    recorder.addStep(StepType::MultipleSolutions, -1, -1, 0, -1, -1, 0,
                     "A second solution was found, so the puzzle is not unique.");
    return SolveResult::MultipleSolutions;
}

SolveResult Solver::solveWithTurbo(const Board& input) {
    DLXSolver turbo;
    const SolveResult result = turbo.solve(input);
    finalBoard = turbo.getFinalBoard();
    solutionCount = turbo.getSolutionCount();
    lastSolveMs = turbo.getElapsedMs();

    std::ostringstream reason;
    reason << "Turbo Exact Cover finished in " << lastSolveMs << " ms with "
           << solutionCount << " solution(s) observed.";

    StepType type = StepType::TurboSolved;
    if (result == SolveResult::InvalidInput) {
        type = StepType::InvalidInput;
    } else if (result == SolveResult::NoSolution) {
        type = StepType::NoSolution;
    } else if (result == SolveResult::MultipleSolutions) {
        type = StepType::MultipleSolutions;
    }
    recorder.addStep(type, -1, -1, 0, -1, -1, 0, reason.str());
    return result;
}

bool Solver::placeWithRecording(Board& board,
                                int row,
                                int col,
                                int number,
                                StepType technique,
                                int depth,
                                const std::string& reason,
                                UnitType unitType,
                                int unitIndex) {
    int before[Board::Size][Board::Size];
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            before[r][c] = board.getCandidates(r, c);
        }
    }

    if (!board.placeNumber(row, col, number, false)) {
        recordContradiction(row, col, number, depth, "Placement conflicts with row/column/box masks.");
        return false;
    }

    recorder.addStep(StepType::PlaceNumber, row, col, number, -1, -1, -1, -1,
                     unitType, unitIndex, before[row][col], 0, 0, depth, reason);

    const int bit = Board::bitForNumber(number);
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int removed = before[r][c] & ~board.getCandidates(r, c);
            if ((removed & bit) != 0 && !(r == row && c == col)) {
                std::ostringstream out;
                out << "Placement of " << number << " removes it from " << cellName(r, c)
                    << " in the related row, column, or box.";
                recorder.addStep(StepType::CandidateRemovedByLogic, r, c, number, row, col, -1, -1,
                                 unitType, unitIndex, before[r][c], board.getCandidates(r, c),
                                 removed & bit, depth, out.str(), technique);
            }
        }
    }
    (void)technique;
    return true;
}

bool Solver::removeCandidatesWithRecording(Board& board,
                                           int row,
                                           int col,
                                           int removeMask,
                                           StepType source,
                                           int depth,
                                           const std::string& reason,
                                           UnitType unitType,
                                           int unitIndex,
                                           int relatedRow,
                                           int relatedCol,
                                           int row2,
                                           int col2) {
    const int before = board.getCandidates(row, col);
    const int removed = board.removeCandidates(row, col, removeMask);
    if (removed == 0) {
        return false;
    }
    const int after = board.getCandidates(row, col);
    recorder.addStep(StepType::CandidateRemovedByLogic,
                     row,
                     col,
                     Board::numberFromBit(removed & -removed),
                     relatedRow,
                     relatedCol,
                     row2,
                     col2,
                     unitType,
                     unitIndex,
                     before,
                     after,
                     removed,
                     depth,
                     reason,
                     source);
    if (after == 0) {
        std::ostringstream out;
        out << "Contradiction after " << maskName(removed) << " was removed from " << cellName(row, col) << ".";
        recordContradiction(row, col, 0, depth, out.str());
    }
    return true;
}

std::vector<std::pair<int, int>> Solver::unitCells(UnitType unitType, int unitIndex) const {
    std::vector<std::pair<int, int>> cells;
    if (unitType == UnitType::Row) {
        for (int c = 0; c < Board::Size; ++c) {
            cells.push_back({unitIndex, c});
        }
    } else if (unitType == UnitType::Column) {
        for (int r = 0; r < Board::Size; ++r) {
            cells.push_back({r, unitIndex});
        }
    } else if (unitType == UnitType::Box) {
        const int br = (unitIndex / 3) * 3;
        const int bc = (unitIndex % 3) * 3;
        for (int dr = 0; dr < 3; ++dr) {
            for (int dc = 0; dc < 3; ++dc) {
                cells.push_back({br + dr, bc + dc});
            }
        }
    }
    return cells;
}

bool Solver::unitHasDigit(const Board& board, UnitType unitType, int unitIndex, int number) const {
    for (const auto& cell : unitCells(unitType, unitIndex)) {
        if (board.getCell(cell.first, cell.second) == number) {
            return true;
        }
    }
    return false;
}

void Solver::recordContradiction(int row, int col, int number, int depth, const std::string& reason) {
    recorder.addStep(StepType::Contradiction, row, col, number, -1, -1, depth, reason);
}

void Solver::recordBacktrackDiff(const Board& base,
                                 const Board& branch,
                                 int depth,
                                 const std::string& reason) {
    for (int r = Board::Size - 1; r >= 0; --r) {
        for (int c = Board::Size - 1; c >= 0; --c) {
            if (base.getCell(r, c) == 0 && branch.getCell(r, c) != 0) {
                recorder.addStep(StepType::Backtrack, r, c, branch.getCell(r, c), -1, -1, depth, reason);
            }
        }
    }
}

std::vector<int> Solver::candidateNumbers(int mask) const {
    std::vector<int> numbers;
    for (int n = 1; n <= 9; ++n) {
        if ((mask & Board::bitForNumber(n)) != 0) {
            numbers.push_back(n);
        }
    }
    return numbers;
}

std::string Solver::cellName(int row, int col) const {
    if (!Board::isInside(row, col)) {
        return "unit";
    }
    std::ostringstream out;
    out << "r" << (row + 1) << "c" << (col + 1);
    return out.str();
}

std::string Solver::maskName(int mask) const {
    std::ostringstream out;
    out << "{";
    bool first = true;
    for (int n = 1; n <= 9; ++n) {
        if ((mask & Board::bitForNumber(n)) == 0) {
            continue;
        }
        if (!first) {
            out << ",";
        }
        first = false;
        out << n;
    }
    out << "}";
    return out.str();
}

std::string Solver::modeName(SolverMode mode) const {
    switch (mode) {
    case SolverMode::HumanLogic:
        return "Human Logic Mode";
    case SolverMode::Smart:
        return "Smart Solver Mode";
    case SolverMode::Turbo:
        return "Turbo Exact Cover Mode";
    }
    return "Smart Solver Mode";
}
