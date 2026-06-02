#pragma once

#include "Board.h"
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
    enum class Mode {
        Editing,
        PlayingSteps,
        SolvedUnique,
        NoSolution,
        MultipleSolutions,
        InvalidInput
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
    std::string selectedCellText() const;
    std::string selectedCandidatesText() const;
    int keyToNumber(SDL_Keycode key) const;
    void editSelectedCell(int number);

    Renderer renderer;
    Solver solver;
    Board editBoard;
    Board initialBoard;
    Board replayBoard;
    Board resetSnapshot;
    Board finalBoard;
    std::vector<SolveStep> steps;
    std::vector<Puzzle> puzzles;

    Mode mode = Mode::Editing;
    SolverMode solverMode = SolverMode::Smart;
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
    std::string puzzleName = "Custom Empty";
    std::string pressedButtonId;
    Uint32 stepStartedTicks = 0;
};
