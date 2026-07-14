#include "PuzzleGenerator.h"

#include "DLXSolver.h"
#include "Solver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>
#include <sstream>
#include <vector>

GeneratedPuzzle PuzzleGenerator::generate(PuzzleDifficulty difficulty) {
    return generateWithSeed(difficulty, makeSeed());
}

GeneratedPuzzle PuzzleGenerator::generate(PuzzleDifficulty difficulty, std::uint32_t seed) {
    std::ostringstream encoded;
    encoded << std::hex << seed;
    return generateWithSeed(difficulty, encoded.str());
}

GeneratedPuzzle PuzzleGenerator::generateWithSeed(PuzzleDifficulty difficulty, const std::string& seed) {
    const auto started = std::chrono::steady_clock::now();
    GeneratedPuzzle generated;
    generated.difficulty = difficulty;
    generated.seed = seed;
    std::mt19937 rng(static_cast<unsigned int>(std::stoul(generated.seed, nullptr, 16)));

    generated.solution = generateSolvedBoard(rng);
    generated.puzzle = generated.solution;
    generated.unique = true;

    std::vector<int> cells(Board::Size * Board::Size);
    std::iota(cells.begin(), cells.end(), 0);
    std::shuffle(cells.begin(), cells.end(), rng);

    const int target = targetGivens(difficulty);
    const int limit = maxAttempts(difficulty);
    for (int cell : cells) {
        if (Board::Size * Board::Size - generated.puzzle.emptyCount() <= target) {
            break;
        }
        if (generated.attempts >= limit) {
            break;
        }
        ++generated.attempts;
        const int row = cell / Board::Size;
        const int col = cell % Board::Size;
        const int previous = generated.puzzle.getCell(row, col);
        if (previous == 0) {
            continue;
        }

        Board candidate = generated.puzzle;
        candidate.removeNumber(row, col, true);
        Board solution;
        if (hasUniqueSolution(candidate, &solution)) {
            generated.puzzle = candidate;
            generated.solution = solution;
            generated.unique = true;
        }
    }

    generated.givens = Board::Size * Board::Size - generated.puzzle.emptyCount();
    generated.puzzle.initializeCandidates();

    Solver solver;
    const SolveResult result = solver.solveUniqueOrMultiple(generated.puzzle, SolverMode::Smart);
    DifficultyAnalyzer analyzer;
    generated.report = analyzer.analyze(generated.puzzle, result, solver.getSteps());

    const auto ended = std::chrono::steady_clock::now();
    generated.generateTimeMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(ended - started).count());
    return generated;
}

std::string PuzzleGenerator::difficultyName(PuzzleDifficulty difficulty) {
    switch (difficulty) {
    case PuzzleDifficulty::Easy:
        return "Easy";
    case PuzzleDifficulty::Medium:
        return "Medium";
    case PuzzleDifficulty::Hard:
        return "Hard";
    case PuzzleDifficulty::Expert:
        return "Expert";
    }
    return "Easy";
}

PuzzleDifficulty PuzzleGenerator::nextDifficulty(PuzzleDifficulty difficulty) {
    switch (difficulty) {
    case PuzzleDifficulty::Easy:
        return PuzzleDifficulty::Medium;
    case PuzzleDifficulty::Medium:
        return PuzzleDifficulty::Hard;
    case PuzzleDifficulty::Hard:
        return PuzzleDifficulty::Expert;
    case PuzzleDifficulty::Expert:
        return PuzzleDifficulty::Easy;
    }
    return PuzzleDifficulty::Easy;
}

Board PuzzleGenerator::generateSolvedBoard(std::mt19937& rng) const {
    std::array<int, Board::Size> digits{};
    std::iota(digits.begin(), digits.end(), 1);
    std::shuffle(digits.begin(), digits.end(), rng);

    std::array<int, Board::Size> rowBands{0, 1, 2, 3, 4, 5, 6, 7, 8};
    std::array<int, Board::Size> colBands{0, 1, 2, 3, 4, 5, 6, 7, 8};

    auto shuffleBands = [&](std::array<int, Board::Size>& values) {
        for (int band = 0; band < 3; ++band) {
            std::shuffle(values.begin() + band * 3, values.begin() + band * 3 + 3, rng);
        }
        std::array<int, 3> order{0, 1, 2};
        std::shuffle(order.begin(), order.end(), rng);
        std::array<int, Board::Size> copy = values;
        int at = 0;
        for (int band : order) {
            for (int i = 0; i < 3; ++i) {
                values[static_cast<size_t>(at++)] = copy[static_cast<size_t>(band * 3 + i)];
            }
        }
    };

    shuffleBands(rowBands);
    shuffleBands(colBands);

    std::array<std::array<int, Board::Size>, Board::Size> grid{};
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int baseRow = rowBands[static_cast<size_t>(r)];
            const int baseCol = colBands[static_cast<size_t>(c)];
            const int index = (baseRow * 3 + baseRow / 3 + baseCol) % 9;
            grid[static_cast<size_t>(r)][static_cast<size_t>(c)] = digits[static_cast<size_t>(index)];
        }
    }

    Board board;
    board.load(grid, true);
    return board;
}

int PuzzleGenerator::targetGivens(PuzzleDifficulty difficulty) const {
    switch (difficulty) {
    case PuzzleDifficulty::Easy:
        return 42;
    case PuzzleDifficulty::Medium:
        return 34;
    case PuzzleDifficulty::Hard:
        return 28;
    case PuzzleDifficulty::Expert:
        return 24;
    }
    return 36;
}

int PuzzleGenerator::maxAttempts(PuzzleDifficulty difficulty) const {
    switch (difficulty) {
    case PuzzleDifficulty::Easy:
        return 90;
    case PuzzleDifficulty::Medium:
        return 140;
    case PuzzleDifficulty::Hard:
        return 190;
    case PuzzleDifficulty::Expert:
        return 230;
    }
    return 120;
}

bool PuzzleGenerator::hasUniqueSolution(const Board& board, Board* solution) const {
    DLXSolver dlx;
    const SolveResult result = dlx.solve(board);
    if (result != SolveResult::SolvedUnique) {
        return false;
    }
    if (solution) {
        *solution = dlx.getFinalBoard();
    }
    return true;
}

std::string PuzzleGenerator::makeSeed() const {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::ostringstream out;
    out << std::hex << static_cast<unsigned long long>(now & 0xffffffffULL);
    return out.str();
}
