#pragma once

#include <string>
#include <vector>

enum class SolveResult {
    InvalidInput,
    NoSolution,
    SolvedUnique,
    MultipleSolutions
};

enum class SolverMode {
    HumanLogic,
    Smart,
    Turbo
};

enum class UnitType {
    None = 0,
    Row = 1,
    Column = 2,
    Box = 3
};

enum class StepType {
    AnalyzeCell,
    RemoveCandidate,
    NakedSingle,
    HiddenSingle,
    LockedCandidate,
    BoxLineReduction,
    NakedPair,
    HiddenPair,
    XWing,
    CandidateRemovedByLogic,
    ModeChanged,
    Guess,
    PlaceNumber,
    Contradiction,
    Backtrack,
    Solved,
    NoSolution,
    MultipleSolutions,
    InvalidInput,
    TurboSolved
};

struct SolveStep {
    StepType type;
    int row = -1;
    int col = -1;
    int number = 0;
    int relatedRow = -1;
    int relatedCol = -1;
    int row2 = -1;
    int col2 = -1;
    int unitType = static_cast<int>(UnitType::None);
    int unitIndex = -1;
    int maskBefore = 0;
    int maskAfter = 0;
    int removedMask = 0;
    int depth = 0;
    std::string reason;
};

class StepRecorder {
public:
    void addStep(StepType type,
                 int row,
                 int col,
                 int number,
                 int relatedRow,
                 int relatedCol,
                 int depth,
                 const std::string& reason);
    void addStep(StepType type,
                 int row,
                 int col,
                 int number,
                 int relatedRow,
                 int relatedCol,
                 int row2,
                 int col2,
                 UnitType unitType,
                 int unitIndex,
                 int maskBefore,
                 int maskAfter,
                 int removedMask,
                 int depth,
                 const std::string& reason);
    void addStep(const SolveStep& step);
    void clear();
    const std::vector<SolveStep>& getSteps() const;

private:
    std::vector<SolveStep> steps;
};
