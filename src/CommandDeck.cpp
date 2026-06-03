#include "CommandDeck.h"

#include <array>

namespace {
constexpr std::array<CommandAction, 20> kActions = {
    CommandAction::Solve,
    CommandAction::GentleHint,
    CommandAction::TechniqueHint,
    CommandAction::DirectHint,
    CommandAction::ApplyHint,
    CommandAction::GeneratePuzzle,
    CommandAction::ChangeGeneratorDifficulty,
    CommandAction::ToggleSolverMode,
    CommandAction::ToggleCandidateDisplay,
    CommandAction::ToggleMistakeMode,
    CommandAction::ImportClipboard,
    CommandAction::CopyPuzzle,
    CommandAction::CopySolution,
    CommandAction::SaveLibrary,
    CommandAction::OpenLibrary,
    CommandAction::OpenAnalytics,
    CommandAction::OpenSettings,
    CommandAction::ClearBoard,
    CommandAction::ResetBoard,
    CommandAction::TurboSolve
};
}

int CommandDeck::count() {
    return static_cast<int>(kActions.size());
}

CommandAction CommandDeck::actionAt(int index) {
    if (index < 0) {
        index = count() - 1;
    }
    index %= count();
    return kActions[static_cast<size_t>(index)];
}

CommandItem CommandDeck::describe(CommandAction action, bool enabled) {
    CommandItem item;
    item.action = action;
    item.enabled = enabled;
    switch (action) {
    case CommandAction::Solve:
        item.label = "Solve";
        item.description = "Run the selected solver mode and play the reasoning trace.";
        break;
    case CommandAction::GentleHint:
        item.label = "Hint: Gentle";
        item.description = "Point to a useful area without revealing the answer.";
        break;
    case CommandAction::TechniqueHint:
        item.label = "Hint: Technique";
        item.description = "Name the next human-style technique.";
        break;
    case CommandAction::DirectHint:
        item.label = "Hint: Direct";
        item.description = "Show the exact next logical placement.";
        break;
    case CommandAction::ApplyHint:
        item.label = "Apply Hint";
        item.description = "Apply the current direct move to the board.";
        break;
    case CommandAction::GeneratePuzzle:
        item.label = "Generate Puzzle";
        item.description = "Create a unique puzzle at the current generator difficulty.";
        break;
    case CommandAction::ChangeGeneratorDifficulty:
        item.label = "Change Difficulty";
        item.description = "Cycle generator difficulty: Easy, Medium, Hard, Expert.";
        break;
    case CommandAction::ToggleSolverMode:
        item.label = "Solver Mode";
        item.description = "Cycle Human Logic, Smart Solver, and Turbo modes.";
        break;
    case CommandAction::ToggleCandidateDisplay:
        item.label = "Candidate Display";
        item.description = "Cycle candidates: Off, Focused, All.";
        break;
    case CommandAction::ToggleMistakeMode:
        item.label = "Mistake Mode";
        item.description = "Cycle Off, RuleCheck, and SolutionCheck.";
        break;
    case CommandAction::ImportClipboard:
        item.label = "Import / Export";
        item.description = "Open the puzzle string text box and clipboard tools.";
        break;
    case CommandAction::CopyPuzzle:
        item.label = "Copy Puzzle";
        item.description = "Copy the current puzzle as an 81-character string.";
        break;
    case CommandAction::CopySolution:
        item.label = "Copy Solution";
        item.description = "Copy the solved board when a unique solution is available.";
        break;
    case CommandAction::SaveLibrary:
        item.label = "Save To Library";
        item.description = "Save the current puzzle into data/puzzles.txt.";
        break;
    case CommandAction::OpenLibrary:
        item.label = "Open Library";
        item.description = "Open the local puzzle library drawer.";
        break;
    case CommandAction::OpenAnalytics:
        item.label = "Open Analytics";
        item.description = "Open difficulty and technique analysis.";
        break;
    case CommandAction::OpenSettings:
        item.label = "Open Settings";
        item.description = "Open visual, solver, and control settings.";
        break;
    case CommandAction::ClearBoard:
        item.label = "Clear Board";
        item.description = "Clear all cells and return to editing.";
        break;
    case CommandAction::ResetBoard:
        item.label = "Reset Board";
        item.description = "Restore the current puzzle snapshot.";
        break;
    case CommandAction::TurboSolve:
        item.label = "Turbo Solve";
        item.description = "Use the exact-cover solver for a fast uniqueness solve.";
        break;
    }
    return item;
}

int CommandDeck::indexOf(CommandAction action) {
    for (int i = 0; i < count(); ++i) {
        if (kActions[static_cast<size_t>(i)] == action) {
            return i;
        }
    }
    return 0;
}
