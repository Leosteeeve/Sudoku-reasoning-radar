#pragma once

#include "Board.h"
#include "DifficultyAnalyzer.h"

#include <random>
#include <string>

enum class PuzzleDifficulty {
    Easy,
    Medium,
    Hard,
    Expert
};

struct GeneratedPuzzle {
    Board puzzle;
    Board solution;
    PuzzleDifficulty difficulty = PuzzleDifficulty::Easy;
    int givens = 0;
    int attempts = 0;
    int generateTimeMs = 0;
    bool unique = false;
    std::string seed;
    DifficultyReport report;
};

class PuzzleGenerator {
public:
    GeneratedPuzzle generate(PuzzleDifficulty difficulty);

    static std::string difficultyName(PuzzleDifficulty difficulty);
    static PuzzleDifficulty nextDifficulty(PuzzleDifficulty difficulty);

private:
    Board generateSolvedBoard(std::mt19937& rng) const;
    int targetGivens(PuzzleDifficulty difficulty) const;
    int maxAttempts(PuzzleDifficulty difficulty) const;
    bool hasUniqueSolution(const Board& board, Board* solution = nullptr) const;
    std::string makeSeed() const;
};
