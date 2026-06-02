#pragma once

#include "Board.h"
#include "StepRecorder.h"

#include <utility>
#include <vector>

class Solver {
public:
    SolveResult solve(const Board& input, SolverMode mode = SolverMode::Smart);
    SolveResult solveUniqueOrMultiple(const Board& input, SolverMode mode = SolverMode::Smart);

    bool applyNakedSingles(Board& board, int depth = 0);
    bool applyHiddenSingles(Board& board, int depth = 0);
    bool applyLockedCandidates(Board& board, int depth = 0);
    bool applyBoxLineReduction(Board& board, int depth = 0);
    bool applyNakedPairs(Board& board, int depth = 0);
    bool applyHiddenPairs(Board& board, int depth = 0);
    bool applyXWing(Board& board, int depth = 0);
    bool search(Board& board, int depth = 0);
    std::pair<int, int> findMRVCell(const Board& board) const;

    const Board& getFinalBoard() const;
    const std::vector<SolveStep>& getSteps() const;
    int getLastSolveMs() const;

private:
    enum class MoveResult {
        NoChange,
        Changed,
        Contradiction
    };

    MoveResult propagate(Board& board, int depth);
    MoveResult nakedSingles(Board& board, int depth);
    MoveResult hiddenSingles(Board& board, int depth);
    MoveResult lockedCandidates(Board& board, int depth);
    MoveResult boxLineReduction(Board& board, int depth);
    MoveResult nakedPairs(Board& board, int depth);
    MoveResult hiddenPairs(Board& board, int depth);
    MoveResult xWing(Board& board, int depth);

    SolveResult solveWithLogicOnly(Board& board);
    SolveResult solveWithSearch(Board& board);
    SolveResult solveWithTurbo(const Board& input);

    bool placeWithRecording(Board& board,
                            int row,
                            int col,
                            int number,
                            StepType technique,
                            int depth,
                            const std::string& reason,
                            UnitType unitType = UnitType::None,
                            int unitIndex = -1);
    bool removeCandidatesWithRecording(Board& board,
                                       int row,
                                       int col,
                                       int removeMask,
                                       StepType source,
                                       int depth,
                                       const std::string& reason,
                                       UnitType unitType = UnitType::None,
                                       int unitIndex = -1,
                                       int relatedRow = -1,
                                       int relatedCol = -1,
                                       int row2 = -1,
                                       int col2 = -1);

    std::vector<std::pair<int, int>> unitCells(UnitType unitType, int unitIndex) const;
    bool unitHasDigit(const Board& board, UnitType unitType, int unitIndex, int number) const;
    void recordContradiction(int row, int col, int number, int depth, const std::string& reason);
    void recordBacktrackDiff(const Board& base, const Board& branch, int depth, const std::string& reason);
    std::vector<int> candidateNumbers(int mask) const;
    std::string cellName(int row, int col) const;
    std::string maskName(int mask) const;
    std::string modeName(SolverMode mode) const;

    StepRecorder recorder;
    Board finalBoard;
    SolverMode activeMode = SolverMode::Smart;
    int solutionCount = 0;
    int lastSolveMs = 0;
};
