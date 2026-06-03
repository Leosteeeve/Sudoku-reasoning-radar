#pragma once

#include "Board.h"
#include "CommandDeck.h"
#include "DifficultyAnalyzer.h"
#include "HintCoach.h"
#include "OCRImport.h"
#include "OverlayPages.h"
#include "PuzzleGenerator.h"
#include "PuzzleIO.h"
#include "PuzzleLibrary.h"
#include "Renderer.h"
#include "Solver.h"

#include <SDL.h>

#include <array>
#include <string>
#include <vector>

class App {
public:
    bool initialize();
    void run();
    void shutdown();

private:
    static constexpr const char* AppVersion = "v0.2.1";

    enum class Mode {
        Editing,
        PlayingSteps,
        SolvedUnique,
        NoSolution,
        MultipleSolutions,
        InvalidInput
    };

    enum class CellSource {
        Empty,
        Given,
        Player,
        Solver,
        Hint
    };

    enum class MistakeMode {
        Off,
        RuleCheck,
        SolutionCheck
    };

    struct Puzzle {
        std::string name;
        std::string difficulty;
        std::array<std::array<int, Board::Size>, Board::Size> grid;
    };

    void handleEvent(const SDL_Event& event);
    void handleKeyDown(SDL_Keycode key);
    void handleMouseDown(int x, int y);
    void handleMouseUp(int x, int y);
    void activateButton(const std::string& id);
    void update();
    void render();

    void startSolve();
    void startTurboSolve();
    void clearBoard();
    void resetBoard();
    void loadNextPuzzle();
    void cycleSolverMode();
    void cycleCandidateDisplayMode();
    void cycleGeneratorDifficulty();
    void generatePuzzle();
    void requestHint(HintLevel level);
    void applyCurrentHint();
    void cycleMistakeMode();
    void importPuzzleFromClipboard();
    void importPuzzleFromTextBox();
    void pasteClipboardToImportBox();
    void appendImportText(const std::string& text);
    void openOCRImportOverlay();
    void chooseOCRImage();
    void importOCRPathFromClipboard();
    void detectOCRGrid();
    void runOCRCells();
    void autoProcessOCR();
    void confirmOCRImport();
    void clearSelectedOCRCell();
    void toggleOCRDebug();
    void editSelectedOCRCell(int number);
    void copyCurrentPuzzleString();
    void copyCurrentSolutionString();
    void saveCurrentPuzzleToLibrary();
    void toggleLibraryPanel();
    void moveLibrarySelection(int delta);
    void loadSelectedLibraryPuzzle();
    void analyzeCurrentBoard();
    void openOverlay(OverlayPage page);
    void closeOverlay();
    void cycleCommand(int delta);
    CommandItem currentCommandItem() const;
    bool commandEnabled(CommandAction action) const;
    void executeCurrentCommand();
    void executeCommand(CommandAction action);
    void adjustSpeed(double factor);
    void toggleAutoPlayback();
    void clearSolutionState();
    void setModeFromResult();
    void advanceStep();
    void retreatStep();
    void rebuildReplayBoard();
    void applyStepToReplay(const SolveStep& step, int stepIndex);
    Uint32 delayForCurrentStep() const;
    std::vector<UIButton> makeButtons() const;
    const UIButton* hitButton(int x, int y, const std::vector<UIButton>& buttons) const;
    std::string statusText() const;
    std::string resultText() const;
    std::string solverModeText() const;
    std::string candidateModeText() const;
    std::string mistakeModeText() const;
    std::string generatorText() const;
    std::string hintText() const;
    std::string difficultyText() const;
    std::string ioText() const;
    std::string libraryText() const;
    std::string focusText() const;
    std::string ocrText() const;
    std::string shortStatusText() const;
    std::string overlayBodyText() const;
    std::vector<std::string> overlayLines() const;
    std::string selectedCellText() const;
    std::string selectedCandidatesText() const;
    int keyToNumber(SDL_Keycode key) const;
    void editSelectedCell(int number);
    void markAllGivens();
    void clearCellSources();
    void updateMistakeDetection();
    void ensureSolutionCache();
    std::string puzzleStringPreview() const;

    Renderer renderer;
    Solver solver;
    PuzzleGenerator puzzleGenerator;
    HintCoach hintCoach;
    DifficultyAnalyzer difficultyAnalyzer;
    PuzzleLibrary puzzleLibrary;
    OCRImport ocrImport;
    OCRReviewState ocrReview;
    OCRResult ocrResult;
    Board editBoard;
    Board initialBoard;
    Board replayBoard;
    Board resetSnapshot;
    Board finalBoard;
    std::vector<SolveStep> steps;
    std::vector<Puzzle> puzzles;
    CellSource cellSources[Board::Size][Board::Size] = {};
    std::vector<CellRef> mistakeCells;
    Board solutionCache;
    bool hasSolutionCache = false;
    Hint currentHint;
    DifficultyReport difficultyReport;
    GeneratedPuzzle lastGenerated;

    Mode mode = Mode::Editing;
    SolverMode solverMode = SolverMode::Smart;
    PuzzleDifficulty generatorDifficulty = PuzzleDifficulty::Easy;
    MistakeMode mistakeMode = MistakeMode::Off;
    SolveResult lastResult = SolveResult::NoSolution;
    bool hasResult = false;
    bool running = true;
    bool paused = false;
    bool playing = false;
    CandidateDisplayMode candidateDisplayMode = CandidateDisplayMode::Focused;
    double speedMultiplier = 1.0;
    int lastSolveMs = 0;
    int currentStep = -1;
    int selectedRow = -1;
    int selectedCol = -1;
    int mouseX = -1;
    int mouseY = -1;
    int nextPuzzleIndex = 0;
    int librarySelection = 0;
    int commandIndex = 5;
    std::string puzzleName = "Custom Empty";
    std::string pressedButtonId;
    std::string ioStatus = "Ready.";
    std::string generatorStatus = "Difficulty Easy.";
    std::string hintStatus = "No hint requested.";
    std::string libraryStatus = "Library closed.";
    std::string mistakeStatus = "Mistake detection off.";
    std::string lastSeed;
    std::string importTextBuffer;
    std::string ocrImagePath;
    std::string ocrStatus = "OCR: not run.";
    bool libraryVisible = false;
    bool generating = false;
    bool importTextEditing = false;
    bool ocrDebug = false;
    int ocrPreviewVersion = 0;
    OverlayPage currentOverlay = OverlayPage::None;
    Uint32 stepStartedTicks = 0;
};
