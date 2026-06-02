#pragma once

#include "Board.h"
#include "StepRecorder.h"

#include <array>
#include <vector>

class DLXSolver {
public:
    SolveResult solve(const Board& input);

    const Board& getFinalBoard() const;
    int getSolutionCount() const;
    int getElapsedMs() const;

private:
    struct Placement {
        int row = 0;
        int col = 0;
        int number = 0;
        std::array<int, 4> constraints{};
    };

    void reset();
    bool buildExactCoverRows(const Board& input);
    bool choosePlacement(int rowIndex, std::vector<int>& chosenRows);
    void unchoosePlacement(int rowIndex);
    bool search(std::vector<int>& chosenRows);
    int chooseBestConstraint() const;
    bool rowIsAvailable(int rowIndex) const;
    void buildBoardFromRows(const Board& input, const std::vector<int>& chosenRows);

    std::vector<Placement> placements;
    std::vector<int> constraintRows[324];
    bool covered[324]{};
    Board finalBoard;
    Board sourceBoard;
    int solutionCount = 0;
    int elapsedMs = 0;
};
