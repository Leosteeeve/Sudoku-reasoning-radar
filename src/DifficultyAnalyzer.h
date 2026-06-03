#pragma once

#include "Board.h"
#include "StepRecorder.h"

#include <string>
#include <vector>

struct TechniqueStats {
    int nakedSingles = 0;
    int hiddenSingles = 0;
    int lockedCandidates = 0;
    int boxLineReductions = 0;
    int nakedPairs = 0;
    int hiddenPairs = 0;
    int xWings = 0;
    int guesses = 0;
    int backtracks = 0;
    int contradictions = 0;
    int totalSteps = 0;
};

enum class DifficultyGrade {
    Easy,
    Medium,
    Hard,
    Expert,
    Invalid,
    Multiple,
    Unsolvable
};

struct DifficultyReport {
    DifficultyGrade grade = DifficultyGrade::Easy;
    TechniqueStats stats;
    int score = 0;
    int givens = 0;
    int emptyCells = 0;
    int maxBranchDepth = 0;
    std::string hardestTechnique = "None";
    std::string summary = "No analysis yet.";
};

class DifficultyAnalyzer {
public:
    DifficultyReport analyze(const Board& puzzle,
                             SolveResult result,
                             const std::vector<SolveStep>& steps) const;

    static std::string gradeName(DifficultyGrade grade);
    static std::string statsSummary(const TechniqueStats& stats);

private:
    static int weightForStep(StepType type);
    static std::string techniqueName(StepType type);
};
