#pragma once

#include "Board.h"
#include "Solver.h"
#include "StepRecorder.h"

#include <string>
#include <vector>

enum class HintLevel {
    Gentle,
    Technique,
    Direct
};

struct Hint {
    bool available = false;
    HintLevel level = HintLevel::Gentle;
    StepType technique = StepType::AnalyzeCell;
    int row = -1;
    int col = -1;
    int number = 0;
    std::vector<CellRef> highlightedCells;
    std::string message;
    std::string explanation;
};

class HintCoach {
public:
    Hint getHint(const Board& board, HintLevel level) const;
    bool applyHint(Board& board, const Hint& hint, std::string* status = nullptr) const;

    static std::string levelName(HintLevel level);
    static std::string techniqueName(StepType type);

private:
    static bool isActionablePlacement(const SolveStep& step);
    static std::vector<CellRef> highlightsForStep(const SolveStep& step);
    static std::string cellName(int row, int col);
};
