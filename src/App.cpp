#include "App.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
std::array<std::array<int, Board::Size>, Board::Size> puzzleFromRows(
    std::initializer_list<std::array<int, Board::Size>> rows) {
    std::array<std::array<int, Board::Size>, Board::Size> grid{};
    int r = 0;
    for (const auto& row : rows) {
        if (r < Board::Size) {
            grid[static_cast<size_t>(r)] = row;
        }
        ++r;
    }
    return grid;
}
}

bool App::initialize() {
    puzzles = {
        Puzzle{"Easy - Singles Practice", "Easy", puzzleFromRows({
            {{5, 3, 0, 0, 7, 0, 0, 0, 0}},
            {{6, 0, 0, 1, 9, 5, 0, 0, 0}},
            {{0, 9, 8, 0, 0, 0, 0, 6, 0}},
            {{8, 0, 0, 0, 6, 0, 0, 0, 3}},
            {{4, 0, 0, 8, 0, 3, 0, 0, 1}},
            {{7, 0, 0, 0, 2, 0, 0, 0, 6}},
            {{0, 6, 0, 0, 0, 0, 2, 8, 0}},
            {{0, 0, 0, 4, 1, 9, 0, 0, 5}},
            {{0, 0, 0, 0, 8, 0, 0, 7, 9}}
        })},
        Puzzle{"Medium - Logic Techniques", "Medium", puzzleFromRows({
            {{0, 0, 0, 2, 6, 0, 7, 0, 1}},
            {{6, 8, 0, 0, 7, 0, 0, 9, 0}},
            {{1, 9, 0, 0, 0, 4, 5, 0, 0}},
            {{8, 2, 0, 1, 0, 0, 0, 4, 0}},
            {{0, 0, 4, 6, 0, 2, 9, 0, 0}},
            {{0, 5, 0, 0, 0, 3, 0, 2, 8}},
            {{0, 0, 9, 3, 0, 0, 0, 7, 4}},
            {{0, 4, 0, 0, 5, 0, 0, 3, 6}},
            {{7, 0, 3, 0, 1, 8, 0, 0, 0}}
        })},
        Puzzle{"Hard - MRV Search", "Hard", puzzleFromRows({
            {{1, 0, 0, 0, 0, 7, 0, 9, 0}},
            {{0, 3, 0, 0, 2, 0, 0, 0, 8}},
            {{0, 0, 9, 6, 0, 0, 5, 0, 0}},
            {{0, 0, 5, 3, 0, 0, 9, 0, 0}},
            {{0, 1, 0, 0, 8, 0, 0, 0, 2}},
            {{6, 0, 0, 0, 0, 4, 0, 0, 0}},
            {{3, 0, 0, 0, 0, 0, 0, 1, 0}},
            {{0, 4, 0, 0, 0, 0, 0, 0, 7}},
            {{0, 0, 7, 0, 0, 0, 3, 0, 0}}
        })},
        Puzzle{"Expert - Turbo Friendly", "Expert", puzzleFromRows({
            {{0, 0, 5, 3, 0, 0, 0, 0, 0}},
            {{8, 0, 0, 0, 0, 0, 0, 2, 0}},
            {{0, 7, 0, 0, 1, 0, 5, 0, 0}},
            {{4, 0, 0, 0, 0, 5, 3, 0, 0}},
            {{0, 1, 0, 0, 7, 0, 0, 0, 6}},
            {{0, 0, 3, 2, 0, 0, 0, 8, 0}},
            {{0, 6, 0, 5, 0, 0, 0, 0, 9}},
            {{0, 0, 4, 0, 0, 0, 0, 3, 0}},
            {{0, 0, 0, 0, 0, 9, 7, 0, 0}}
        })},
        Puzzle{"Invalid - Duplicate In Row", "Invalid", puzzleFromRows({
            {{5, 3, 0, 5, 7, 0, 0, 0, 0}},
            {{6, 0, 0, 1, 9, 5, 0, 0, 0}},
            {{0, 9, 8, 0, 0, 0, 0, 6, 0}},
            {{8, 0, 0, 0, 6, 0, 0, 0, 3}},
            {{4, 0, 0, 8, 0, 3, 0, 0, 1}},
            {{7, 0, 0, 0, 2, 0, 0, 0, 6}},
            {{0, 6, 0, 0, 0, 0, 2, 8, 0}},
            {{0, 0, 0, 4, 1, 9, 0, 0, 5}},
            {{0, 0, 0, 0, 8, 0, 0, 7, 9}}
        })},
        Puzzle{"Multiple - Two Rows Open", "Multiple", puzzleFromRows({
            {{5, 3, 4, 6, 7, 8, 9, 1, 2}},
            {{6, 7, 2, 1, 9, 5, 3, 4, 8}},
            {{1, 9, 8, 3, 4, 2, 5, 6, 7}},
            {{8, 5, 9, 7, 6, 1, 4, 2, 3}},
            {{4, 2, 6, 8, 5, 3, 7, 9, 1}},
            {{7, 1, 3, 9, 2, 4, 8, 5, 6}},
            {{9, 6, 1, 5, 3, 7, 2, 8, 4}},
            {{0, 0, 0, 0, 0, 0, 0, 0, 0}},
            {{0, 0, 0, 0, 0, 0, 0, 0, 0}}
        })}
    };

    editBoard.clear();
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;

    return renderer.initialize();
}

void App::run() {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }
        update();
        render();
        SDL_Delay(8);
    }
}

void App::shutdown() {
    renderer.shutdown();
}

void App::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }
    if (event.type == SDL_MOUSEMOTION) {
        mouseX = event.motion.x;
        mouseY = event.motion.y;
        return;
    }
    if (event.type == SDL_KEYDOWN) {
        handleKeyDown(event.key.keysym.sym);
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        handleMouseDown(event.button.x, event.button.y);
        return;
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        handleMouseUp(event.button.x, event.button.y);
    }
}

void App::handleKeyDown(SDL_Keycode key) {
    if (key == SDLK_ESCAPE) {
        running = false;
        return;
    }
    if (key == SDLK_SPACE) {
        startSolve();
        return;
    }
    if (key == SDLK_t) {
        startTurboSolve();
        return;
    }
    if (key == SDLK_F11 || key == SDLK_f) {
        renderer.toggleFullscreen();
        return;
    }
    if (key == SDLK_m) {
        cycleSolverMode();
        return;
    }
    if (key == SDLK_h) {
        cycleCandidateDisplayMode();
        return;
    }
    if (key == SDLK_a) {
        toggleAutoPlayback();
        return;
    }
    if (key == SDLK_s || key == SDLK_RIGHT) {
        paused = true;
        playing = false;
        advanceStep();
        return;
    }
    if (key == SDLK_b || key == SDLK_LEFT) {
        paused = true;
        playing = false;
        retreatStep();
        return;
    }
    if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) {
        adjustSpeed(1.25);
        return;
    }
    if (key == SDLK_MINUS || key == SDLK_UNDERSCORE || key == SDLK_KP_MINUS) {
        adjustSpeed(0.8);
        return;
    }
    if (key == SDLK_r) {
        resetBoard();
        return;
    }
    if (key == SDLK_c) {
        clearBoard();
        return;
    }
    if (key == SDLK_n) {
        loadNextPuzzle();
        return;
    }
    if (key == SDLK_p) {
        toggleAutoPlayback();
        return;
    }

    const int number = keyToNumber(key);
    if (number >= 0) {
        editSelectedCell(number);
    }
}

void App::handleMouseDown(int x, int y) {
    mouseX = x;
    mouseY = y;
    pressedButtonId.clear();

    int row = -1;
    int col = -1;
    if (renderer.cellFromPoint(x, y, row, col)) {
        selectedRow = row;
        selectedCol = col;
        return;
    }

    const std::vector<UIButton> buttons = makeButtons();
    const UIButton* button = hitButton(x, y, buttons);
    if (!button || !button->enabled) {
        return;
    }

    pressedButtonId = button->id;
}

void App::handleMouseUp(int x, int y) {
    mouseX = x;
    mouseY = y;
    if (pressedButtonId.empty()) {
        return;
    }

    const std::string id = pressedButtonId;
    const std::vector<UIButton> buttons = makeButtons();
    const UIButton* button = hitButton(x, y, buttons);
    pressedButtonId.clear();

    if (!button || !button->enabled || button->id != id) {
        return;
    }
    activateButton(id);
}

void App::activateButton(const std::string& id) {
    if (id == "solve") {
        startSolve();
    } else if (id == "mode") {
        cycleSolverMode();
    } else if (id == "turbo") {
        startTurboSolve();
    } else if (id == "auto") {
        toggleAutoPlayback();
    } else if (id == "prev") {
        paused = true;
        playing = false;
        retreatStep();
    } else if (id == "next") {
        paused = true;
        playing = false;
        advanceStep();
    } else if (id == "reset") {
        resetBoard();
    } else if (id == "clear") {
        clearBoard();
    } else if (id == "puzzle") {
        loadNextPuzzle();
    } else if (id == "candidates") {
        cycleCandidateDisplayMode();
    } else if (id == "full") {
        renderer.toggleFullscreen();
    }
}

void App::update() {
    if (!playing || paused || steps.empty()) {
        return;
    }
    const Uint32 now = SDL_GetTicks();
    if (now - stepStartedTicks >= delayForCurrentStep()) {
        advanceStep();
    }
}

void App::render() {
    RenderInfo info;
    info.solverModeText = solverModeText();
    info.statusText = statusText();
    info.resultText = resultText();
    info.puzzleName = puzzleName;
    info.selectedCellText = selectedCellText();
    info.selectedCandidatesText = selectedCandidatesText();
    info.candidateModeText = candidateModeText();
    info.solvingTimeMs = lastSolveMs;
    info.selectedRow = selectedRow;
    info.selectedCol = selectedCol;
    info.paused = paused;
    info.playing = playing;
    info.candidateMode = candidateDisplayMode;
    info.speedMultiplier = speedMultiplier;
    info.stepAgeMs = SDL_GetTicks() - stepStartedTicks;
    renderer.render(replayBoard, steps, currentStep, info, makeButtons());
}

void App::startSolve() {
    initialBoard = editBoard;
    replayBoard = initialBoard;
    resetSnapshot = initialBoard;
    finalBoard.clear();
    steps.clear();
    currentStep = -1;
    paused = false;
    playing = false;
    hasResult = false;
    mode = Mode::PlayingSteps;

    const Uint32 started = SDL_GetTicks();
    lastResult = solver.solveUniqueOrMultiple(initialBoard, solverMode);
    lastSolveMs = solver.getLastSolveMs();
    if (lastSolveMs == 0) {
        lastSolveMs = static_cast<int>(SDL_GetTicks() - started);
    }
    hasResult = true;
    finalBoard = solver.getFinalBoard();
    steps = solver.getSteps();

    if (!steps.empty()) {
        advanceStep();
        playing = currentStep + 1 < static_cast<int>(steps.size());
    } else {
        setModeFromResult();
    }
}

void App::startTurboSolve() {
    solverMode = SolverMode::Turbo;
    startSolve();
}

void App::clearBoard() {
    editBoard.clear();
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = "Custom Empty";
    selectedRow = -1;
    selectedCol = -1;
    lastSolveMs = 0;
    clearSolutionState();
}

void App::resetBoard() {
    editBoard = resetSnapshot;
    initialBoard = resetSnapshot;
    replayBoard = resetSnapshot;
    lastSolveMs = 0;
    clearSolutionState();
}

void App::loadNextPuzzle() {
    if (puzzles.empty()) {
        return;
    }
    const Puzzle& puzzle = puzzles[static_cast<size_t>(nextPuzzleIndex)];
    editBoard.load(puzzle.grid, true);
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = puzzle.name + " (" + puzzle.difficulty + ")";
    nextPuzzleIndex = (nextPuzzleIndex + 1) % static_cast<int>(puzzles.size());
    selectedRow = -1;
    selectedCol = -1;
    lastSolveMs = 0;
    clearSolutionState();
}

void App::cycleSolverMode() {
    if (solverMode == SolverMode::HumanLogic) {
        solverMode = SolverMode::Smart;
    } else if (solverMode == SolverMode::Smart) {
        solverMode = SolverMode::Turbo;
    } else {
        solverMode = SolverMode::HumanLogic;
    }
    clearSolutionState();
}

void App::cycleCandidateDisplayMode() {
    if (candidateDisplayMode == CandidateDisplayMode::Off) {
        candidateDisplayMode = CandidateDisplayMode::Focused;
    } else if (candidateDisplayMode == CandidateDisplayMode::Focused) {
        candidateDisplayMode = CandidateDisplayMode::All;
    } else {
        candidateDisplayMode = CandidateDisplayMode::Off;
    }
}

void App::adjustSpeed(double factor) {
    speedMultiplier = std::clamp(speedMultiplier * factor, 0.25, 4.0);
}

void App::toggleAutoPlayback() {
    if (steps.empty()) {
        return;
    }
    paused = !paused;
    playing = !paused && currentStep + 1 < static_cast<int>(steps.size());
}

void App::clearSolutionState() {
    steps.clear();
    finalBoard.clear();
    currentStep = -1;
    paused = false;
    playing = false;
    hasResult = false;
    mode = Mode::Editing;
    stepStartedTicks = SDL_GetTicks();
}

void App::setModeFromResult() {
    playing = false;
    paused = false;
    switch (lastResult) {
    case SolveResult::InvalidInput:
        mode = Mode::InvalidInput;
        break;
    case SolveResult::NoSolution:
        mode = Mode::NoSolution;
        break;
    case SolveResult::SolvedUnique:
        mode = Mode::SolvedUnique;
        break;
    case SolveResult::MultipleSolutions:
        mode = Mode::MultipleSolutions;
        break;
    }
}

void App::advanceStep() {
    if (steps.empty()) {
        return;
    }
    if (currentStep + 1 < static_cast<int>(steps.size())) {
        ++currentStep;
        rebuildReplayBoard();
        stepStartedTicks = SDL_GetTicks();
    }
    if (currentStep + 1 >= static_cast<int>(steps.size())) {
        setModeFromResult();
    } else if (mode != Mode::PlayingSteps) {
        mode = Mode::PlayingSteps;
    }
}

void App::retreatStep() {
    if (steps.empty()) {
        return;
    }
    if (currentStep > -1) {
        --currentStep;
        rebuildReplayBoard();
        stepStartedTicks = SDL_GetTicks();
    }
    mode = Mode::PlayingSteps;
}

void App::rebuildReplayBoard() {
    replayBoard = initialBoard;
    for (int i = 0; i <= currentStep && i < static_cast<int>(steps.size()); ++i) {
        applyStepToReplay(steps[static_cast<size_t>(i)], i);
    }
}

void App::applyStepToReplay(const SolveStep& step, int stepIndex) {
    switch (step.type) {
    case StepType::PlaceNumber:
        if (Board::isInside(step.row, step.col) && step.number >= 1 && step.number <= 9) {
            if (!replayBoard.placeNumber(step.row, step.col, step.number, false)) {
                replayBoard.setCellValue(step.row, step.col, step.number, false);
            }
        }
        break;
    case StepType::CandidateRemovedByLogic:
    case StepType::RemoveCandidate:
        if (Board::isInside(step.row, step.col) && step.removedMask != 0) {
            replayBoard.removeCandidates(step.row, step.col, step.removedMask);
        }
        break;
    case StepType::Backtrack:
        if (Board::isInside(step.row, step.col)) {
            replayBoard.removeNumber(step.row, step.col, true);
        }
        break;
    case StepType::Solved:
    case StepType::TurboSolved:
    case StepType::MultipleSolutions:
        if (stepIndex + 1 == static_cast<int>(steps.size()) && finalBoard.isSolved()) {
            replayBoard = finalBoard;
        }
        break;
    default:
        break;
    }
}

Uint32 App::delayForCurrentStep() const {
    if (currentStep < 0 || currentStep >= static_cast<int>(steps.size())) {
        return 140;
    }
    const StepType type = steps[static_cast<size_t>(currentStep)].type;
    Uint32 base = 600;
    switch (type) {
    case StepType::AnalyzeCell:
    case StepType::CandidateRemovedByLogic:
    case StepType::RemoveCandidate:
        base = 80;
        break;
    case StepType::PlaceNumber:
    case StepType::NakedSingle:
    case StepType::HiddenSingle:
    case StepType::LockedCandidate:
    case StepType::BoxLineReduction:
    case StepType::NakedPair:
    case StepType::HiddenPair:
    case StepType::XWing:
        base = 280;
        break;
    case StepType::Guess:
    case StepType::Contradiction:
    case StepType::Backtrack:
        base = 360;
        break;
    default:
        base = 620;
        break;
    }
    return static_cast<Uint32>(std::max(25.0, base / speedMultiplier));
}

std::vector<UIButton> App::makeButtons() const {
    const bool hasSteps = !steps.empty();
    std::vector<UIButton> buttons = {
        UIButton{"solve", "Solve", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"mode", "Mode", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"turbo", "Turbo", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"auto", paused ? "Auto" : "Pause", SDL_Rect{0, 0, 0, 0}, hasSteps},
        UIButton{"prev", "Prev", SDL_Rect{0, 0, 0, 0}, hasSteps && currentStep > -1},
        UIButton{"next", "Next", SDL_Rect{0, 0, 0, 0},
                 hasSteps && currentStep + 1 < static_cast<int>(steps.size())},
        UIButton{"reset", "Reset", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"clear", "Clear", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"puzzle", "Puzzle", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"candidates", "Cand " + candidateModeText(), SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"full", renderer.isFullscreen() ? "Window" : "Full",
                 SDL_Rect{0, 0, 0, 0}, true}
    };
    renderer.layoutButtons(buttons, mouseX, mouseY, pressedButtonId);
    return buttons;
}

const UIButton* App::hitButton(int x, int y, const std::vector<UIButton>& buttons) const {
    for (const UIButton& button : buttons) {
        if (x >= button.rect.x && x < button.rect.x + button.rect.w
            && y >= button.rect.y && y < button.rect.y + button.rect.h) {
            return &button;
        }
    }
    return nullptr;
}

std::string App::statusText() const {
    switch (mode) {
    case Mode::Editing:
        return "Editing";
    case Mode::PlayingSteps:
        return paused ? "Playing Steps (Paused)" : "Playing Steps";
    case Mode::SolvedUnique:
        return "Solved Unique";
    case Mode::NoSolution:
        return solverMode == SolverMode::HumanLogic ? "Human Logic Stopped" : "No Solution";
    case Mode::MultipleSolutions:
        return "Multiple Solutions";
    case Mode::InvalidInput:
        return "Invalid Input";
    }
    return "Editing";
}

std::string App::resultText() const {
    if (!hasResult) {
        return "";
    }
    switch (lastResult) {
    case SolveResult::InvalidInput:
        return "The starting grid violates Sudoku constraints.";
    case SolveResult::NoSolution:
        return solverMode == SolverMode::HumanLogic
            ? "Human logic stopped: no available logical move found."
            : "No valid completion exists for this grid.";
    case SolveResult::SolvedUnique:
        return "A unique final answer was found.";
    case SolveResult::MultipleSolutions:
        return "At least two valid completions were detected.";
    }
    return "";
}

std::string App::solverModeText() const {
    switch (solverMode) {
    case SolverMode::HumanLogic:
        return "Human Logic Mode";
    case SolverMode::Smart:
        return "Smart Solver Mode";
    case SolverMode::Turbo:
        return "Turbo Exact Cover Mode";
    }
    return "Smart Solver Mode";
}

std::string App::candidateModeText() const {
    switch (candidateDisplayMode) {
    case CandidateDisplayMode::Off:
        return "Off";
    case CandidateDisplayMode::Focused:
        return "Focus";
    case CandidateDisplayMode::All:
        return "All";
    }
    return "Focus";
}

std::string App::selectedCellText() const {
    if (!Board::isInside(selectedRow, selectedCol)) {
        return "None";
    }
    std::ostringstream out;
    out << "r" << (selectedRow + 1) << "c" << (selectedCol + 1);
    const int value = replayBoard.getCell(selectedRow, selectedCol);
    if (value != 0) {
        out << " = " << value;
    }
    return out.str();
}

std::string App::selectedCandidatesText() const {
    if (!Board::isInside(selectedRow, selectedCol) || replayBoard.getCell(selectedRow, selectedCol) != 0) {
        return "-";
    }
    const int mask = replayBoard.getCandidates(selectedRow, selectedCol);
    std::ostringstream out;
    bool first = true;
    for (int n = 1; n <= 9; ++n) {
        if ((mask & Board::bitForNumber(n)) == 0) {
            continue;
        }
        if (!first) {
            out << " ";
        }
        first = false;
        out << n;
    }
    return first ? "none" : out.str();
}

int App::keyToNumber(SDL_Keycode key) const {
    if (key >= SDLK_1 && key <= SDLK_9) {
        return static_cast<int>(key - SDLK_0);
    }
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9) {
        return static_cast<int>(key - SDLK_KP_0);
    }
    if (key == SDLK_0 || key == SDLK_KP_0 || key == SDLK_BACKSPACE || key == SDLK_DELETE) {
        return 0;
    }
    return -1;
}

void App::editSelectedCell(int number) {
    if (!Board::isInside(selectedRow, selectedCol)) {
        return;
    }
    editBoard.setCellValue(selectedRow, selectedCol, number, number != 0);
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    lastSolveMs = 0;
    clearSolutionState();
    puzzleName = "Custom";
}
