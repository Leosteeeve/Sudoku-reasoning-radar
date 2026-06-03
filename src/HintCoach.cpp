#include "HintCoach.h"

#include <sstream>

Hint HintCoach::getHint(const Board& board, HintLevel level) const {
    Hint hint;
    hint.level = level;

    std::string reason;
    if (!board.validateInitial(&reason)) {
        hint.message = "The current grid has a rule conflict.";
        hint.explanation = reason;
        return hint;
    }

    Solver solver;
    solver.solveUniqueOrMultiple(board, SolverMode::HumanLogic);
    for (const SolveStep& step : solver.getSteps()) {
        if (!isActionablePlacement(step)) {
            continue;
        }

        hint.available = true;
        hint.technique = step.type;
        hint.row = step.row;
        hint.col = step.col;
        hint.number = step.number;
        hint.highlightedCells = highlightsForStep(step);

        const std::string technique = techniqueName(step.type);
        const std::string cell = cellName(step.row, step.col);
        if (level == HintLevel::Gentle) {
            std::ostringstream msg;
            msg << "Focus around " << cell << " and its row, column, and box.";
            hint.message = msg.str();
            hint.explanation = "Look for a forced candidate without placing it yet.";
        } else if (level == HintLevel::Technique) {
            hint.message = "Technique hint: " + technique + ".";
            hint.explanation = "The move is logical and does not require guessing.";
        } else {
            std::ostringstream msg;
            msg << cell << " must be " << step.number << ".";
            hint.message = msg.str();
            hint.explanation = step.reason.empty()
                ? ("The solver found a " + technique + " here.")
                : step.reason;
        }
        return hint;
    }

    hint.message = "No clear human-style move was found.";
    hint.explanation = "Smart Solver may need a branch assumption.";
    return hint;
}

bool HintCoach::applyHint(Board& board, const Hint& hint, std::string* status) const {
    if (!hint.available || !Board::isInside(hint.row, hint.col) || hint.number < 1 || hint.number > 9) {
        if (status) {
            *status = "No direct hint is available to apply.";
        }
        return false;
    }

    if (!board.placeNumber(hint.row, hint.col, hint.number, false)) {
        if (status) {
            *status = "Hint could not be applied because the cell conflicts with the current grid.";
        }
        return false;
    }
    if (status) {
        *status = "Applied hint: " + cellName(hint.row, hint.col) + " = " + std::to_string(hint.number) + ".";
    }
    return true;
}

std::string HintCoach::levelName(HintLevel level) {
    switch (level) {
    case HintLevel::Gentle:
        return "Gentle";
    case HintLevel::Technique:
        return "Technique";
    case HintLevel::Direct:
        return "Direct";
    }
    return "Gentle";
}

std::string HintCoach::techniqueName(StepType type) {
    switch (type) {
    case StepType::NakedSingle:
        return "Naked Single";
    case StepType::HiddenSingle:
        return "Hidden Single";
    case StepType::LockedCandidate:
        return "Locked Candidate";
    case StepType::BoxLineReduction:
        return "Box-Line Reduction";
    case StepType::NakedPair:
        return "Naked Pair";
    case StepType::HiddenPair:
        return "Hidden Pair";
    case StepType::XWing:
        return "X-Wing";
    case StepType::PlaceNumber:
        return "Placement";
    default:
        return "Human Logic";
    }
}

bool HintCoach::isActionablePlacement(const SolveStep& step) {
    return Board::isInside(step.row, step.col)
        && step.number >= 1
        && step.number <= 9
        && (step.type == StepType::NakedSingle
            || step.type == StepType::HiddenSingle
            || step.type == StepType::PlaceNumber);
}

std::vector<CellRef> HintCoach::highlightsForStep(const SolveStep& step) {
    std::vector<CellRef> cells;
    if (!Board::isInside(step.row, step.col)) {
        return cells;
    }
    cells.push_back(CellRef{step.row, step.col});
    for (int c = 0; c < Board::Size; ++c) {
        cells.push_back(CellRef{step.row, c});
    }
    for (int r = 0; r < Board::Size; ++r) {
        cells.push_back(CellRef{r, step.col});
    }
    const int box = Board::boxId(step.row, step.col);
    const int br = (box / Board::BoxSize) * Board::BoxSize;
    const int bc = (box % Board::BoxSize) * Board::BoxSize;
    for (int dr = 0; dr < Board::BoxSize; ++dr) {
        for (int dc = 0; dc < Board::BoxSize; ++dc) {
            cells.push_back(CellRef{br + dr, bc + dc});
        }
    }
    return cells;
}

std::string HintCoach::cellName(int row, int col) {
    std::ostringstream out;
    out << "r" << (row + 1) << "c" << (col + 1);
    return out.str();
}
