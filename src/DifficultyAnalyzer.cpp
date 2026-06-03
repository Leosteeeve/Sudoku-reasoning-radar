#include "DifficultyAnalyzer.h"

#include <algorithm>
#include <sstream>

DifficultyReport DifficultyAnalyzer::analyze(const Board& puzzle,
                                             SolveResult result,
                                             const std::vector<SolveStep>& steps) const {
    DifficultyReport report;
    report.givens = Board::Size * Board::Size - puzzle.emptyCount();
    report.emptyCells = puzzle.emptyCount();
    report.stats.totalSteps = static_cast<int>(steps.size());

    int hardestWeight = 0;
    int score = 0;
    for (const SolveStep& step : steps) {
        report.maxBranchDepth = std::max(report.maxBranchDepth, step.depth);
        const int weight = weightForStep(step.type);
        if (weight > 0) {
            score += weight;
            if (weight > hardestWeight) {
                hardestWeight = weight;
                report.hardestTechnique = techniqueName(step.type);
            }
        }

        switch (step.type) {
        case StepType::NakedSingle:
            ++report.stats.nakedSingles;
            break;
        case StepType::HiddenSingle:
            ++report.stats.hiddenSingles;
            break;
        case StepType::LockedCandidate:
            ++report.stats.lockedCandidates;
            break;
        case StepType::BoxLineReduction:
            ++report.stats.boxLineReductions;
            break;
        case StepType::NakedPair:
            ++report.stats.nakedPairs;
            break;
        case StepType::HiddenPair:
            ++report.stats.hiddenPairs;
            break;
        case StepType::XWing:
            ++report.stats.xWings;
            break;
        case StepType::Guess:
            ++report.stats.guesses;
            break;
        case StepType::Backtrack:
            ++report.stats.backtracks;
            break;
        case StepType::Contradiction:
            ++report.stats.contradictions;
            break;
        default:
            break;
        }
    }

    score += report.maxBranchDepth * 10;
    score += std::max(0, report.emptyCells - 45);
    report.score = score;

    if (result == SolveResult::InvalidInput) {
        report.grade = DifficultyGrade::Invalid;
        report.summary = "Invalid puzzle input.";
        return report;
    }
    if (result == SolveResult::MultipleSolutions) {
        report.grade = DifficultyGrade::Multiple;
        report.summary = "Multiple solutions detected.";
        return report;
    }
    if (result == SolveResult::NoSolution) {
        report.grade = DifficultyGrade::Unsolvable;
        report.summary = "No valid solution found.";
        return report;
    }

    if (score <= 60) {
        report.grade = DifficultyGrade::Easy;
    } else if (score <= 130) {
        report.grade = DifficultyGrade::Medium;
    } else if (score <= 240) {
        report.grade = DifficultyGrade::Hard;
    } else {
        report.grade = DifficultyGrade::Expert;
    }

    std::ostringstream out;
    out << gradeName(report.grade) << " puzzle, score " << report.score
        << ", hardest technique " << report.hardestTechnique << ".";
    report.summary = out.str();
    return report;
}

std::string DifficultyAnalyzer::gradeName(DifficultyGrade grade) {
    switch (grade) {
    case DifficultyGrade::Easy:
        return "Easy";
    case DifficultyGrade::Medium:
        return "Medium";
    case DifficultyGrade::Hard:
        return "Hard";
    case DifficultyGrade::Expert:
        return "Expert";
    case DifficultyGrade::Invalid:
        return "Invalid";
    case DifficultyGrade::Multiple:
        return "Multiple";
    case DifficultyGrade::Unsolvable:
        return "Unsolvable";
    }
    return "Easy";
}

std::string DifficultyAnalyzer::statsSummary(const TechniqueStats& stats) {
    std::ostringstream out;
    out << "Steps " << stats.totalSteps
        << ", NS " << stats.nakedSingles
        << ", HS " << stats.hiddenSingles
        << ", LC " << stats.lockedCandidates
        << ", Pairs " << (stats.nakedPairs + stats.hiddenPairs)
        << ", XW " << stats.xWings
        << ", Guess " << stats.guesses
        << ", Back " << stats.backtracks;
    return out.str();
}

int DifficultyAnalyzer::weightForStep(StepType type) {
    switch (type) {
    case StepType::NakedSingle:
        return 1;
    case StepType::HiddenSingle:
        return 2;
    case StepType::LockedCandidate:
    case StepType::BoxLineReduction:
        return 4;
    case StepType::NakedPair:
    case StepType::HiddenPair:
        return 6;
    case StepType::XWing:
        return 10;
    case StepType::Guess:
        return 18;
    case StepType::Backtrack:
        return 25;
    default:
        return 0;
    }
}

std::string DifficultyAnalyzer::techniqueName(StepType type) {
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
    case StepType::Guess:
        return "MRV Guess";
    case StepType::Backtrack:
        return "Backtrack";
    default:
        return "None";
    }
}
