#include "PuzzleString.h"
#include "Solver.h"

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* CanonicalPuzzle =
    "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* CanonicalSolution =
    "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
constexpr const char* InitialDuplicatePuzzle =
    "530570000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* UnsolvablePuzzle =
    "531070000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* MultipleSolutionPuzzle =
    "534678912672195348198342567859761423426853791713924856961537284000000000000000000";

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

template <typename Actual, typename Expected>
void expectEqual(const Actual& actual,
                 const Expected& expected,
                 const std::string& message) {
    if (!(actual == expected)) {
        throw TestFailure(message);
    }
}

Board parsePuzzle(const std::string& text) {
    Board board;
    std::string error;
    expect(PuzzleString::parse(text, board, &error), "parse failed: " + error);
    return board;
}

void canonicalUniquePuzzleKeepsItsSolution() {
    const Board puzzle = parsePuzzle(CanonicalPuzzle);
    Solver solver;

    const SolveResult result = solver.solveUniqueOrMultiple(puzzle, SolverMode::Smart);

    expectEqual(result, SolveResult::SolvedUnique, "canonical puzzle was not uniquely solved");
    expectEqual(PuzzleString::serialize(solver.getFinalBoard()),
                std::string(CanonicalSolution),
                "canonical puzzle solution changed");
}

void initialDuplicateIsInvalidInput() {
    const Board puzzle = parsePuzzle(InitialDuplicatePuzzle);
    Solver solver;

    expectEqual(solver.solveUniqueOrMultiple(puzzle, SolverMode::Smart),
                SolveResult::InvalidInput,
                "initial duplicate was not reported as invalid input");
}

void unsolvablePuzzleHasNoSolution() {
    const Board puzzle = parsePuzzle(UnsolvablePuzzle);
    Solver solver;

    expect(puzzle.validateInitial(), "unsolvable fixture must be valid initial input");
    expectEqual(solver.solveUniqueOrMultiple(puzzle, SolverMode::Smart),
                SolveResult::NoSolution,
                "unsolvable puzzle did not report no solution");
}

void twoOpenRowsHaveMultipleSolutions() {
    const Board puzzle = parsePuzzle(MultipleSolutionPuzzle);
    Solver solver;

    expectEqual(solver.solveUniqueOrMultiple(puzzle, SolverMode::Smart),
                SolveResult::MultipleSolutions,
                "legacy multiple-solution puzzle was not reported as multiple");
}

void puzzleStringRoundTripsWithoutUiDependencies() {
    const Board puzzle = parsePuzzle(CanonicalPuzzle);

    expectEqual(PuzzleString::serialize(puzzle),
                std::string(CanonicalPuzzle),
                "puzzle string did not round-trip");
}
}

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"canonical unique puzzle keeps its solution", canonicalUniquePuzzleKeepsItsSolution},
        {"initial duplicate is invalid input", initialDuplicateIsInvalidInput},
        {"unsolvable puzzle has no solution", unsolvablePuzzleHasNoSolution},
        {"two open rows have multiple solutions", twoOpenRowsHaveMultipleSolutions},
        {"puzzle string round-trips without UI dependencies", puzzleStringRoundTripsWithoutUiDependencies},
    };

    int failed = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "PASS: " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL: " << test.first << ": " << error.what() << '\n';
        }
    }

    if (failed != 0) {
        std::cerr << failed << " test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " test(s) passed\n";
    return 0;
}
