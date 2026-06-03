#pragma once

#include <string>

enum class CommandAction {
    Solve,
    GentleHint,
    TechniqueHint,
    DirectHint,
    ApplyHint,
    GeneratePuzzle,
    ChangeGeneratorDifficulty,
    ToggleSolverMode,
    ToggleCandidateDisplay,
    ToggleMistakeMode,
    OCRImportImage,
    ImportClipboard,
    CopyPuzzle,
    CopySolution,
    SaveLibrary,
    OpenLibrary,
    OpenAnalytics,
    OpenSettings,
    ClearBoard,
    ResetBoard,
    TurboSolve
};

struct CommandItem {
    CommandAction action = CommandAction::GeneratePuzzle;
    std::string label;
    std::string description;
    bool enabled = true;
};

class CommandDeck {
public:
    static int count();
    static CommandAction actionAt(int index);
    static CommandItem describe(CommandAction action, bool enabled = true);
    static int indexOf(CommandAction action);
};
