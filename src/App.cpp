#include "App.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
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

std::string sanitizePuzzleEntryText(const std::string& text) {
    std::string compact;
    compact.reserve(81);
    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            continue;
        }
        if ((ch >= '0' && ch <= '9') || ch == '.') {
            compact.push_back(static_cast<char>(ch));
        }
        if (compact.size() >= 81) {
            break;
        }
    }
    return compact;
}

std::string stripPathQuotes(const std::string& text) {
    std::string out = text;
    out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch) {
                  return std::isspace(ch) == 0;
              }));
    out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch) {
                  return std::isspace(ch) == 0;
              }).base(),
              out.end());
    if (out.size() >= 2 && out.front() == '"' && out.back() == '"') {
        out = out.substr(1, out.size() - 2);
    }
    return out;
}

std::string solveResultName(SolveResult result) {
    switch (result) {
    case SolveResult::InvalidInput:
        return "Invalid conflict";
    case SolveResult::NoSolution:
        return "No solution";
    case SolveResult::SolvedUnique:
        return "Unique solution";
    case SolveResult::MultipleSolutions:
        return "Multiple solutions";
    }
    return "Unknown";
}

std::string openImageFileDialog() {
    char fileName[MAX_PATH] = {};
    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;
    dialog.lpstrFilter = "Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = "Open Sudoku Image";
    if (GetOpenFileNameA(&dialog) == TRUE) {
        return fileName;
    }
    return {};
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
    finalBoard.clear();
    solutionCache.clear();
    clearCellSources();
    puzzleLibrary.load(&libraryStatus);

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
    if (event.type == SDL_TEXTINPUT) {
        if (currentOverlay == OverlayPage::ImportExport && importTextEditing) {
            appendImportText(event.text.text);
        }
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
    const SDL_Keymod mods = SDL_GetModState();
    const bool ctrl = (mods & KMOD_CTRL) != 0;
    const bool shift = (mods & KMOD_SHIFT) != 0;

    if (key == SDLK_ESCAPE) {
        if (currentOverlay != OverlayPage::None) {
            closeOverlay();
        } else {
            running = false;
        }
        return;
    }
    if (currentOverlay == OverlayPage::OCRImport) {
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            confirmOCRImport();
        } else if (key == SDLK_BACKSPACE || key == SDLK_DELETE) {
            clearSelectedOCRCell();
        } else {
            const int number = keyToNumber(key);
            if (number > 0) {
                editSelectedOCRCell(number);
            } else if (number == 0) {
                clearSelectedOCRCell();
            }
        }
        return;
    }
    if (currentOverlay == OverlayPage::ImportExport && importTextEditing) {
        if (ctrl && key == SDLK_v) {
            pasteClipboardToImportBox();
        } else if (key == SDLK_BACKSPACE) {
            if (!importTextBuffer.empty()) {
                importTextBuffer.pop_back();
                ioStatus = "Import text edited.";
            }
        } else if (key == SDLK_DELETE) {
            importTextBuffer.clear();
            ioStatus = "Import text cleared.";
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            importPuzzleFromTextBox();
        }
        return;
    }
    if (key == SDLK_o) {
        openOCRImportOverlay();
        return;
    }
    if (key == SDLK_TAB) {
        cycleCommand(shift ? -1 : 1);
        return;
    }
    if (ctrl && key == SDLK_i) {
        importPuzzleFromClipboard();
        return;
    }
    if (ctrl && key == SDLK_e) {
        if (shift) {
            copyCurrentSolutionString();
        } else {
            copyCurrentPuzzleString();
        }
        return;
    }
    if (ctrl && key == SDLK_s) {
        saveCurrentPuzzleToLibrary();
        return;
    }
    if (key == SDLK_F1) {
        requestHint(HintLevel::Gentle);
        return;
    }
    if (key == SDLK_F2) {
        requestHint(HintLevel::Technique);
        return;
    }
    if (key == SDLK_F3) {
        requestHint(HintLevel::Direct);
        return;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        if (currentOverlay == OverlayPage::Library && libraryVisible) {
            loadSelectedLibraryPuzzle();
        } else {
            executeCurrentCommand();
        }
        return;
    }
    if (key == SDLK_g) {
        if (shift) {
            openOverlay(OverlayPage::Generator);
        } else {
            generatePuzzle();
        }
        return;
    }
    if (key == SDLK_k) {
        cycleMistakeMode();
        return;
    }
    if (key == SDLK_l) {
        toggleLibraryPanel();
        return;
    }
    if (key == SDLK_UP && libraryVisible) {
        moveLibrarySelection(-1);
        return;
    }
    if (key == SDLK_DOWN && libraryVisible) {
        moveLibrarySelection(1);
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
    if (key == SDLK_s) {
        paused = true;
        playing = false;
        advanceStep();
        return;
    }
    if (key == SDLK_RIGHT) {
        if (!steps.empty() && currentStep + 1 < static_cast<int>(steps.size())) {
            paused = true;
            playing = false;
            advanceStep();
        } else {
            cycleCommand(1);
        }
        return;
    }
    if (key == SDLK_b) {
        paused = true;
        playing = false;
        retreatStep();
        return;
    }
    if (key == SDLK_LEFT) {
        if (!steps.empty() && currentStep > -1) {
            paused = true;
            playing = false;
            retreatStep();
        } else {
            cycleCommand(-1);
        }
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

    const std::vector<UIButton> buttons = makeButtons();
    const UIButton* button = hitButton(x, y, buttons);
    if (button && button->enabled) {
        pressedButtonId = button->id;
        return;
    }

    if (currentOverlay == OverlayPage::OCRImport) {
        int row = -1;
        int col = -1;
        if (renderer.ocrCellFromPoint(x, y, row, col)) {
            ocrReview.selectCell(row, col);
        }
        return;
    }

    if (currentOverlay != OverlayPage::None) {
        return;
    }

    int row = -1;
    int col = -1;
    if (renderer.cellFromPoint(x, y, row, col)) {
        selectedRow = row;
        selectedCol = col;
        return;
    }
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
    if (id == "deck_prev") {
        cycleCommand(-1);
    } else if (id == "deck_next") {
        cycleCommand(1);
    } else if (id == "deck_exec") {
        executeCurrentCommand();
    } else if (id == "icon_settings") {
        openOverlay(OverlayPage::Settings);
    } else if (id == "icon_analytics") {
        openOverlay(OverlayPage::Analytics);
    } else if (id == "icon_library") {
        openOverlay(OverlayPage::Library);
    } else if (id == "icon_help") {
        openOverlay(OverlayPage::Shortcuts);
    } else if (id == "overlay_close") {
        closeOverlay();
    } else if (id == "overlay_import_text") {
        importPuzzleFromTextBox();
    } else if (id == "overlay_paste_clipboard") {
        pasteClipboardToImportBox();
    } else if (id == "overlay_copy_puzzle") {
        copyCurrentPuzzleString();
    } else if (id == "overlay_copy_solution") {
        copyCurrentSolutionString();
    } else if (id == "overlay_ocr_open") {
        chooseOCRImage();
    } else if (id == "overlay_ocr_path_clip") {
        importOCRPathFromClipboard();
    } else if (id == "overlay_ocr_detect") {
        detectOCRGrid();
    } else if (id == "overlay_ocr_run") {
        runOCRCells();
    } else if (id == "overlay_ocr_auto") {
        autoProcessOCR();
    } else if (id == "overlay_ocr_clear") {
        clearSelectedOCRCell();
    } else if (id == "overlay_ocr_confirm") {
        confirmOCRImport();
    } else if (id == "overlay_ocr_debug") {
        toggleOCRDebug();
    } else if (id == "overlay_ocr_cancel") {
        closeOverlay();
    } else if (id == "solve") {
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
    } else if (id == "generate") {
        generatePuzzle();
    } else if (id == "difficulty") {
        cycleGeneratorDifficulty();
    } else if (id == "hint") {
        requestHint(HintLevel::Gentle);
    } else if (id == "explain") {
        requestHint(HintLevel::Technique);
    } else if (id == "applyhint") {
        applyCurrentHint();
    } else if (id == "import") {
        importPuzzleFromClipboard();
    } else if (id == "export") {
        copyCurrentPuzzleString();
    } else if (id == "save") {
        saveCurrentPuzzleToLibrary();
    } else if (id == "library") {
        toggleLibraryPanel();
    } else if (id == "mistake") {
        cycleMistakeMode();
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
    updateMistakeDetection();
    RenderInfo info;
    info.versionText = AppVersion;
    info.solverModeText = solverModeText();
    info.statusText = statusText();
    info.resultText = resultText();
    info.puzzleName = puzzleName;
    info.selectedCellText = selectedCellText();
    info.selectedCandidatesText = selectedCandidatesText();
    info.candidateModeText = candidateModeText();
    info.mistakeModeText = mistakeModeText();
    info.generatorText = generatorText();
    info.hintText = hintText();
    info.difficultyText = difficultyText();
    info.ioText = ioText();
    info.libraryText = libraryText();
    info.focusText = focusText();
    info.shortStatusText = shortStatusText();
    const CommandItem command = currentCommandItem();
    info.commandLabel = command.label;
    info.commandDescription = command.description;
    info.commandEnabled = command.enabled;
    info.commandIndex = commandIndex + 1;
    info.commandTotal = CommandDeck::count();
    info.overlayPage = currentOverlay;
    info.overlayTitle = overlayTitle(currentOverlay);
    info.overlayInputText = importTextBuffer;
    info.overlayInputActive = currentOverlay == OverlayPage::ImportExport && importTextEditing;
    info.overlayLines = overlayLines();
    info.ocrStatusText = ocrStatus;
    info.ocrValidationText = ocrReview.validationMessage();
    info.ocrImagePath = ocrImagePath;
    info.ocrCells = ocrReview.cells();
    info.ocrGivens = ocrReview.givens();
    info.ocrLowConfidenceCount = ocrReview.lowConfidenceCount();
    info.ocrConflictCount = ocrReview.conflictCount();
    info.ocrSelectedRow = ocrReview.selectedRow();
    info.ocrSelectedCol = ocrReview.selectedCol();
    info.ocrPreviewVersion = ocrPreviewVersion;
    info.ocrDebug = ocrDebug;
    info.ocrCanConfirm = ocrReview.canConfirmImport();
    info.ocrOriginalPreview = &ocrResult.originalPreview;
    info.ocrWarpedGrid = &ocrResult.warpedGrid;
    info.solvingTimeMs = lastSolveMs;
    info.selectedRow = selectedRow;
    info.selectedCol = selectedCol;
    info.paused = paused;
    info.playing = playing;
    info.candidateMode = candidateDisplayMode;
    info.speedMultiplier = speedMultiplier;
    info.stepAgeMs = SDL_GetTicks() - stepStartedTicks;
    info.hintCells = currentHint.highlightedCells;
    info.mistakeCells = mistakeCells;
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
    difficultyReport = difficultyAnalyzer.analyze(initialBoard, lastResult, steps);
    hasSolutionCache = lastResult == SolveResult::SolvedUnique && finalBoard.isSolved();
    if (hasSolutionCache) {
        solutionCache = finalBoard;
    }

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
    finalBoard.clear();
    solutionCache.clear();
    hasSolutionCache = false;
    clearCellSources();
    puzzleName = "Custom Empty";
    selectedRow = -1;
    selectedCol = -1;
    lastSolveMs = 0;
    ioStatus = "Board cleared.";
    hintStatus = "No hint requested.";
    clearSolutionState();
}

void App::resetBoard() {
    editBoard = resetSnapshot;
    initialBoard = resetSnapshot;
    replayBoard = resetSnapshot;
    markAllGivens();
    hasSolutionCache = false;
    lastSolveMs = 0;
    ioStatus = "Puzzle reset.";
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
    markAllGivens();
    hasSolutionCache = false;
    puzzleName = puzzle.name + " (" + puzzle.difficulty + ")";
    nextPuzzleIndex = (nextPuzzleIndex + 1) % static_cast<int>(puzzles.size());
    selectedRow = -1;
    selectedCol = -1;
    lastSolveMs = 0;
    ioStatus = "Loaded built-in puzzle.";
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

void App::cycleGeneratorDifficulty() {
    generatorDifficulty = PuzzleGenerator::nextDifficulty(generatorDifficulty);
    generatorStatus = "Generator difficulty: " + PuzzleGenerator::difficultyName(generatorDifficulty) + ".";
}

void App::generatePuzzle() {
    generating = true;
    generatorStatus = "Generating puzzle...";
    render();

    lastGenerated = puzzleGenerator.generate(generatorDifficulty);
    editBoard = lastGenerated.puzzle;
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    finalBoard = lastGenerated.solution;
    solutionCache = lastGenerated.solution;
    hasSolutionCache = lastGenerated.solution.isSolved();
    difficultyReport = lastGenerated.report;
    lastSolveMs = lastGenerated.generateTimeMs;
    lastSeed = lastGenerated.seed;
    puzzleName = "Generated " + PuzzleGenerator::difficultyName(generatorDifficulty);
    selectedRow = -1;
    selectedCol = -1;
    markAllGivens();
    clearSolutionState();
    finalBoard = lastGenerated.solution;
    solutionCache = lastGenerated.solution;
    hasSolutionCache = lastGenerated.solution.isSolved();
    difficultyReport = lastGenerated.report;
    generating = false;
    generatorStatus = "Generated " + PuzzleGenerator::difficultyName(generatorDifficulty)
        + ", givens " + std::to_string(lastGenerated.givens)
        + ", seed " + lastGenerated.seed
        + ", unique yes.";
    ioStatus = "Generated puzzle ready.";
}

void App::requestHint(HintLevel level) {
    currentHint = hintCoach.getHint(editBoard, level);
    hintStatus = currentHint.message;
    if (!currentHint.explanation.empty()) {
        hintStatus += " " + currentHint.explanation;
    }
}

void App::applyCurrentHint() {
    std::string status;
    if (!hintCoach.applyHint(editBoard, currentHint, &status)) {
        hintStatus = status;
        return;
    }
    if (Board::isInside(currentHint.row, currentHint.col)) {
        cellSources[currentHint.row][currentHint.col] = CellSource::Hint;
    }
    replayBoard = editBoard;
    initialBoard = editBoard;
    hintStatus = status;
    steps.clear();
    currentStep = -1;
    paused = false;
    playing = false;
    mode = Mode::Editing;
}

void App::cycleMistakeMode() {
    if (mistakeMode == MistakeMode::Off) {
        mistakeMode = MistakeMode::RuleCheck;
    } else if (mistakeMode == MistakeMode::RuleCheck) {
        mistakeMode = MistakeMode::SolutionCheck;
    } else {
        mistakeMode = MistakeMode::Off;
    }
    updateMistakeDetection();
}

void App::importPuzzleFromClipboard() {
    Board imported;
    std::string status;
    if (!PuzzleIO::importFromClipboard(imported, &status)) {
        ioStatus = status;
        return;
    }
    editBoard = imported;
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = "Imported Puzzle";
    selectedRow = -1;
    selectedCol = -1;
    markAllGivens();
    clearSolutionState();
    ioStatus = status;
}

void App::importPuzzleFromTextBox() {
    Board imported;
    std::string status;
    if (!PuzzleIO::parsePuzzleString(importTextBuffer, imported, &status)) {
        ioStatus = status;
        return;
    }

    editBoard = imported;
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = "Imported Puzzle";
    selectedRow = -1;
    selectedCol = -1;
    markAllGivens();
    clearSolutionState();
    ioStatus = "Imported puzzle string from text box.";
}

void App::pasteClipboardToImportBox() {
    if (!SDL_HasClipboardText()) {
        ioStatus = "Clipboard does not contain puzzle text.";
        return;
    }

    char* raw = SDL_GetClipboardText();
    if (!raw) {
        ioStatus = "Failed to read clipboard text.";
        return;
    }
    const std::string pasted(raw);
    SDL_free(raw);

    importTextBuffer = sanitizePuzzleEntryText(pasted);
    std::ostringstream status;
    status << "Pasted " << importTextBuffer.size() << " / 81 characters into import box.";
    ioStatus = status.str();
}

void App::appendImportText(const std::string& text) {
    const std::string sanitized = sanitizePuzzleEntryText(text);
    if (sanitized.empty()) {
        return;
    }

    const size_t remaining = importTextBuffer.size() >= 81 ? 0 : 81 - importTextBuffer.size();
    importTextBuffer.append(sanitized.substr(0, remaining));
    std::ostringstream status;
    status << "Import text " << importTextBuffer.size() << " / 81 characters.";
    ioStatus = status.str();
}

void App::openOCRImportOverlay() {
    openOverlay(OverlayPage::OCRImport);
    ocrStatus = ocrStatus.empty() ? "OCR: not run." : ocrStatus;
}

void App::chooseOCRImage() {
    const std::string path = openImageFileDialog();
    if (path.empty()) {
        ocrStatus = "OCR: image selection cancelled.";
        return;
    }

    ocrImagePath = path;
    if (ocrImport.loadImage(ocrImagePath, ocrResult)) {
        ocrReview.clear();
        ++ocrPreviewVersion;
        ocrStatus = "OCR: image loaded. Detect grid or run Auto Process.";
    } else {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: " + ocrResult.errorMessage;
    }
}

void App::importOCRPathFromClipboard() {
    if (!SDL_HasClipboardText()) {
        ocrStatus = "OCR: clipboard does not contain an image path.";
        return;
    }

    char* raw = SDL_GetClipboardText();
    if (!raw) {
        ocrStatus = "OCR: failed to read clipboard path.";
        return;
    }
    ocrImagePath = stripPathQuotes(raw);
    SDL_free(raw);

    if (ocrImagePath.empty()) {
        ocrStatus = "OCR: clipboard path is empty.";
        return;
    }
    if (ocrImport.loadImage(ocrImagePath, ocrResult)) {
        ocrReview.clear();
        ++ocrPreviewVersion;
        ocrStatus = "OCR: image loaded from clipboard path.";
    } else {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: " + ocrResult.errorMessage;
    }
}

void App::detectOCRGrid() {
    if (ocrImagePath.empty() && ocrResult.originalPreview.empty()) {
        chooseOCRImage();
        if (ocrResult.originalPreview.empty()) {
            return;
        }
    }
    OCRProcessOptions options;
    options.debug = ocrDebug;
    if (ocrResult.originalPreview.empty() && !ocrImport.loadImage(ocrImagePath, ocrResult)) {
        ocrStatus = "OCR: " + ocrResult.errorMessage;
        ++ocrPreviewVersion;
        return;
    }
    if (ocrImport.detectGrid(ocrResult, options)) {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: grid detected. Run OCR to recognize digits.";
    } else {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: " + ocrResult.errorMessage;
    }
}

void App::runOCRCells() {
    OCRProcessOptions options;
    options.debug = ocrDebug;
    if (ocrResult.warpedGrid.empty()) {
        detectOCRGrid();
        if (ocrResult.warpedGrid.empty()) {
            return;
        }
    }
    if (ocrImport.runOCR(ocrResult, options)) {
        ocrReview.loadResult(ocrResult);
        ++ocrPreviewVersion;
        std::ostringstream out;
        out << "OCR: " << ocrReview.givens() << " givens, "
            << ocrReview.lowConfidenceCount() << " low-confidence";
        if (ocrReview.conflictCount() > 0) {
            out << ", conflict detected";
        }
        out << ".";
        ocrStatus = out.str();
    } else {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: " + ocrResult.errorMessage;
    }
}

void App::autoProcessOCR() {
    if (ocrImagePath.empty()) {
        chooseOCRImage();
        if (ocrImagePath.empty()) {
            return;
        }
    }

    OCRProcessOptions options;
    options.debug = ocrDebug;
    if (ocrImport.autoProcess(ocrImagePath, ocrResult, options)) {
        ocrReview.loadResult(ocrResult);
        ++ocrPreviewVersion;
        std::ostringstream out;
        out << "OCR: " << ocrReview.givens() << " givens, "
            << ocrReview.lowConfidenceCount() << " low-confidence, "
            << solveResultName(ocrReview.validationResult()) << ".";
        ocrStatus = out.str();
    } else {
        ++ocrPreviewVersion;
        ocrStatus = "OCR: " + ocrResult.errorMessage;
    }
}

void App::confirmOCRImport() {
    if (!ocrReview.canConfirmImport()) {
        ocrStatus = "OCR: fix red conflict cells before confirming import.";
        return;
    }

    editBoard = ocrReview.toBoard();
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = "OCR Imported Puzzle";
    selectedRow = -1;
    selectedCol = -1;
    markAllGivens();
    clearSolutionState();
    ioStatus = "Imported puzzle from OCR review board.";
    ocrStatus = "OCR: imported successfully. " + ocrReview.validationMessage();
    closeOverlay();
}

void App::clearSelectedOCRCell() {
    int row = -1;
    int col = -1;
    if (!ocrReview.selectedCell(row, col)) {
        ocrStatus = "OCR: select a review cell first.";
        return;
    }
    ocrReview.clearCell(row, col);
    ocrStatus = "OCR: cleared selected cell.";
}

void App::toggleOCRDebug() {
    ocrDebug = !ocrDebug;
    ocrStatus = std::string("OCR Debug: ") + (ocrDebug ? "On. Intermediate images save to data/ocr_debug/." : "Off.");
}

void App::editSelectedOCRCell(int number) {
    int row = -1;
    int col = -1;
    if (!ocrReview.selectedCell(row, col)) {
        ocrStatus = "OCR: select a review cell first.";
        return;
    }
    ocrReview.editCell(row, col, number);
    ocrStatus = "OCR: edited selected cell.";
}

void App::copyCurrentPuzzleString() {
    PuzzleIO::copyPuzzleToClipboard(editBoard, &ioStatus);
}

void App::copyCurrentSolutionString() {
    if (finalBoard.isSolved()) {
        PuzzleIO::copySolutionToClipboard(finalBoard, &ioStatus);
        return;
    }
    ensureSolutionCache();
    PuzzleIO::copySolutionToClipboard(solutionCache, &ioStatus);
}

void App::saveCurrentPuzzleToLibrary() {
    ensureSolutionCache();
    const std::string difficulty = DifficultyAnalyzer::gradeName(difficultyReport.grade);
    const LibraryEntry entry = PuzzleLibrary::makeEntry(
        puzzleName.empty() ? "Untitled Puzzle" : puzzleName,
        difficulty,
        editBoard,
        hasSolutionCache ? &solutionCache : nullptr,
        lastSeed.empty() ? "-" : lastSeed);
    puzzleLibrary.saveEntry(entry, &libraryStatus);
    librarySelection = std::max(0, puzzleLibrary.count() - 1);
}

void App::toggleLibraryPanel() {
    if (currentOverlay == OverlayPage::Library) {
        closeOverlay();
        return;
    }
    currentOverlay = OverlayPage::Library;
    libraryVisible = true;
    puzzleLibrary.load(&libraryStatus);
    if (puzzleLibrary.count() == 0) {
        librarySelection = 0;
    } else {
        librarySelection = std::clamp(librarySelection, 0, puzzleLibrary.count() - 1);
    }
}

void App::moveLibrarySelection(int delta) {
    if (!libraryVisible || puzzleLibrary.count() == 0) {
        return;
    }
    librarySelection = std::clamp(librarySelection + delta, 0, puzzleLibrary.count() - 1);
    const LibraryEntry* entry = puzzleLibrary.entryAt(librarySelection);
    if (entry) {
        libraryStatus = "Selected " + entry->name + ".";
    }
}

void App::loadSelectedLibraryPuzzle() {
    const LibraryEntry* entry = puzzleLibrary.entryAt(librarySelection);
    if (!entry) {
        libraryStatus = "No library puzzle selected.";
        return;
    }
    Board loaded;
    std::string error;
    if (!PuzzleIO::parsePuzzleString(entry->puzzleString, loaded, &error)) {
        libraryStatus = "Failed to load library puzzle: " + error;
        return;
    }
    editBoard = loaded;
    initialBoard = editBoard;
    replayBoard = editBoard;
    resetSnapshot = editBoard;
    puzzleName = entry->name + " (" + entry->difficulty + ")";
    lastSeed = entry->seed;
    markAllGivens();
    clearSolutionState();
    libraryStatus = "Loaded " + entry->name + ".";
}

void App::analyzeCurrentBoard() {
    Solver analyzerSolver;
    const SolveResult result = analyzerSolver.solveUniqueOrMultiple(editBoard, SolverMode::Smart);
    difficultyReport = difficultyAnalyzer.analyze(editBoard, result, analyzerSolver.getSteps());
}

void App::openOverlay(OverlayPage page) {
    if (importTextEditing && page != OverlayPage::ImportExport) {
        SDL_StopTextInput();
        importTextEditing = false;
    }
    currentOverlay = page;
    if (page == OverlayPage::ImportExport) {
        importTextEditing = true;
        SDL_StartTextInput();
        if (ioStatus == "Ready.") {
            ioStatus = "Import box ready. Type or paste 81 characters.";
        }
    }
    if (page == OverlayPage::OCRImport && ocrStatus == "OCR: not run.") {
        ocrStatus = "OCR: open a PNG or JPG Sudoku image.";
    }
    if (page == OverlayPage::Library) {
        libraryVisible = true;
        puzzleLibrary.load(&libraryStatus);
        if (puzzleLibrary.count() > 0) {
            librarySelection = std::clamp(librarySelection, 0, puzzleLibrary.count() - 1);
        }
    }
    if (page == OverlayPage::Analytics) {
        analyzeCurrentBoard();
    }
}

void App::closeOverlay() {
    if (importTextEditing) {
        SDL_StopTextInput();
        importTextEditing = false;
    }
    currentOverlay = OverlayPage::None;
}

void App::cycleCommand(int delta) {
    const int total = CommandDeck::count();
    commandIndex = (commandIndex + delta) % total;
    if (commandIndex < 0) {
        commandIndex += total;
    }
}

CommandItem App::currentCommandItem() const {
    const CommandAction action = CommandDeck::actionAt(commandIndex);
    return CommandDeck::describe(action, commandEnabled(action));
}

bool App::commandEnabled(CommandAction action) const {
    switch (action) {
    case CommandAction::ApplyHint:
        return currentHint.available;
    case CommandAction::CopySolution:
        return finalBoard.isSolved() || hasSolutionCache;
    case CommandAction::OpenAnalytics:
        return !editBoard.isSolved() || hasResult || difficultyReport.stats.totalSteps > 0;
    default:
        return true;
    }
}

void App::executeCurrentCommand() {
    const CommandItem command = currentCommandItem();
    if (!command.enabled) {
        ioStatus = "Current action is not available yet.";
        return;
    }
    executeCommand(command.action);
}

void App::executeCommand(CommandAction action) {
    switch (action) {
    case CommandAction::Solve:
        startSolve();
        break;
    case CommandAction::GentleHint:
        requestHint(HintLevel::Gentle);
        break;
    case CommandAction::TechniqueHint:
        requestHint(HintLevel::Technique);
        break;
    case CommandAction::DirectHint:
        requestHint(HintLevel::Direct);
        break;
    case CommandAction::ApplyHint:
        applyCurrentHint();
        break;
    case CommandAction::GeneratePuzzle:
        generatePuzzle();
        break;
    case CommandAction::ChangeGeneratorDifficulty:
        cycleGeneratorDifficulty();
        break;
    case CommandAction::ToggleSolverMode:
        cycleSolverMode();
        break;
    case CommandAction::ToggleCandidateDisplay:
        cycleCandidateDisplayMode();
        break;
    case CommandAction::ToggleMistakeMode:
        cycleMistakeMode();
        break;
    case CommandAction::OCRImportImage:
        openOCRImportOverlay();
        break;
    case CommandAction::ImportClipboard:
        openOverlay(OverlayPage::ImportExport);
        break;
    case CommandAction::CopyPuzzle:
        copyCurrentPuzzleString();
        break;
    case CommandAction::CopySolution:
        copyCurrentSolutionString();
        break;
    case CommandAction::SaveLibrary:
        saveCurrentPuzzleToLibrary();
        break;
    case CommandAction::OpenLibrary:
        openOverlay(OverlayPage::Library);
        break;
    case CommandAction::OpenAnalytics:
        openOverlay(OverlayPage::Analytics);
        break;
    case CommandAction::OpenSettings:
        openOverlay(OverlayPage::Settings);
        break;
    case CommandAction::ClearBoard:
        clearBoard();
        break;
    case CommandAction::ResetBoard:
        resetBoard();
        break;
    case CommandAction::TurboSolve:
        startTurboSolve();
        break;
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
    hasSolutionCache = false;
    currentHint = Hint{};
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
    if (currentOverlay != OverlayPage::None) {
        std::vector<UIButton> overlayButtons = {
            UIButton{"overlay_close", "Close", SDL_Rect{0, 0, 0, 0}, true}
        };
        if (currentOverlay == OverlayPage::OCRImport) {
            overlayButtons.push_back(UIButton{"overlay_ocr_open", "Open Image", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_ocr_path_clip", "Path Clip", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_ocr_detect", "Detect Grid", SDL_Rect{0, 0, 0, 0}, !ocrResult.originalPreview.empty() || !ocrImagePath.empty()});
            overlayButtons.push_back(UIButton{"overlay_ocr_run", "Run OCR", SDL_Rect{0, 0, 0, 0}, !ocrResult.warpedGrid.empty() || !ocrResult.originalPreview.empty() || !ocrImagePath.empty()});
            overlayButtons.push_back(UIButton{"overlay_ocr_auto", "Auto Process", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_ocr_clear", "Clear Cell", SDL_Rect{0, 0, 0, 0}, Board::isInside(ocrReview.selectedRow(), ocrReview.selectedCol())});
            overlayButtons.push_back(UIButton{"overlay_ocr_confirm", "Confirm", SDL_Rect{0, 0, 0, 0}, ocrReview.canConfirmImport()});
            overlayButtons.push_back(UIButton{"overlay_ocr_debug", ocrDebug ? "Debug On" : "Debug Off", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_ocr_cancel", "Cancel", SDL_Rect{0, 0, 0, 0}, true});
        } else if (currentOverlay == OverlayPage::ImportExport) {
            overlayButtons.push_back(UIButton{"overlay_paste_clipboard", "Paste", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_import_text", "Import", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_copy_puzzle", "Copy Puzzle", SDL_Rect{0, 0, 0, 0}, true});
            overlayButtons.push_back(UIButton{"overlay_copy_solution", "Copy Sol.", SDL_Rect{0, 0, 0, 0}, finalBoard.isSolved() || hasSolutionCache});
        }
        renderer.layoutButtons(overlayButtons, mouseX, mouseY, pressedButtonId);
        return overlayButtons;
    }

    std::vector<UIButton> buttons = {
        UIButton{"icon_settings", "[SET]", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"icon_analytics", "[ANL]", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"icon_library", "[LIB]", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"icon_help", "[?]", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"deck_prev", "<", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"deck_next", ">", SDL_Rect{0, 0, 0, 0}, true},
        UIButton{"deck_exec", "Execute", SDL_Rect{0, 0, 0, 0}, currentCommandItem().enabled}
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

std::string App::mistakeModeText() const {
    switch (mistakeMode) {
    case MistakeMode::Off:
        return "Off";
    case MistakeMode::RuleCheck:
        return "Rule";
    case MistakeMode::SolutionCheck:
        return "Solution";
    }
    return "Off";
}

std::string App::generatorText() const {
    std::ostringstream out;
    out << PuzzleGenerator::difficultyName(generatorDifficulty);
    if (generating) {
        out << " | Generating...";
    } else if (lastGenerated.givens > 0) {
        out << " | Givens " << lastGenerated.givens
            << " | Seed " << lastGenerated.seed
            << " | " << lastGenerated.generateTimeMs << " ms";
    } else {
        out << " | " << generatorStatus;
    }
    return out.str();
}

std::string App::hintText() const {
    if (currentHint.available) {
        std::ostringstream out;
        out << HintCoach::levelName(currentHint.level)
            << " | " << HintCoach::techniqueName(currentHint.technique)
            << " | " << currentHint.message;
        return out.str();
    }
    return hintStatus;
}

std::string App::difficultyText() const {
    std::ostringstream out;
    out << DifficultyAnalyzer::gradeName(difficultyReport.grade)
        << " | Score " << difficultyReport.score
        << " | Hardest " << difficultyReport.hardestTechnique;
    return out.str();
}

std::string App::ioText() const {
    return puzzleStringPreview() + " | " + ioStatus;
}

std::string App::libraryText() const {
    std::ostringstream out;
    out << (libraryVisible ? "Open" : "Closed")
        << " | " << puzzleLibrary.count() << " saved";
    const LibraryEntry* entry = puzzleLibrary.entryAt(librarySelection);
    if (entry) {
        out << " | " << (librarySelection + 1) << ": " << entry->name;
    }
    out << " | " << libraryStatus;
    return out.str();
}

std::string App::focusText() const {
    if (currentHint.available) {
        return "Hint: " + currentHint.message;
    }
    if (!ocrStatus.empty() && ocrStatus != "OCR: not run.") {
        return ocrStatus;
    }
    if (!steps.empty() && currentStep >= 0 && currentStep < static_cast<int>(steps.size())) {
        const SolveStep& step = steps[static_cast<size_t>(currentStep)];
        std::ostringstream out;
        out << "Step " << (currentStep + 1) << "/" << steps.size() << ": ";
        if (step.number > 0 && Board::isInside(step.row, step.col)) {
            out << "r" << (step.row + 1) << "c" << (step.col + 1) << " = " << step.number << ". ";
        }
        out << (step.reason.empty() ? "Reasoning step recorded." : step.reason);
        return out.str();
    }
    if (editBoard.emptyCount() == Board::Size * Board::Size) {
        return "Ready to begin. Generate a puzzle or enter one manually.";
    }
    return "Ready. Solve, ask for a hint, or step through the reasoning trace.";
}

std::string App::shortStatusText() const {
    if (currentOverlay != OverlayPage::None) {
        return overlayTitle(currentOverlay) + " drawer open.";
    }
    if (generating) {
        return "Generating puzzle...";
    }
    if (!ioStatus.empty() && ioStatus != "Ready.") {
        return ioStatus;
    }
    if (!hintStatus.empty() && hintStatus != "No hint requested.") {
        return hintStatus;
    }
    if (!generatorStatus.empty() && generatorStatus.find("Generated") != std::string::npos) {
        return generatorStatus;
    }
    if (!ocrStatus.empty() && ocrStatus != "OCR: not run.") {
        return ocrStatus;
    }
    return statusText();
}

std::string App::ocrText() const {
    std::ostringstream out;
    out << ocrStatus
        << " | Givens " << ocrReview.givens()
        << " | Low " << ocrReview.lowConfidenceCount()
        << " | Conflicts " << ocrReview.conflictCount();
    if (!ocrImagePath.empty()) {
        out << " | " << ocrImagePath;
    }
    return out.str();
}

std::string App::overlayBodyText() const {
    std::ostringstream out;
    for (const std::string& line : overlayLines()) {
        out << line << "\n";
    }
    return out.str();
}

std::vector<std::string> App::overlayLines() const {
    std::vector<std::string> lines;
    switch (currentOverlay) {
    case OverlayPage::Settings:
        lines = {
            "Visual",
            "Candidate display: " + candidateModeText(),
            "Animation speed: " + std::to_string(static_cast<int>(speedMultiplier * 100.0)) + "%",
            renderer.isFullscreen() ? "Fullscreen: On" : "Fullscreen: Off",
            "Theme: Radar Dark",
            "",
            "Solver",
            "Solver mode: " + solverModeText(),
            "Mistake mode: " + mistakeModeText(),
            std::string("OCR Debug: ") + (ocrDebug ? "On" : "Off"),
            "",
            "Controls",
            paused ? "Autoplay: Paused" : "Autoplay: Ready",
            "Use Command Deck actions to change modes or execute commands.",
            "",
            "About",
            std::string("Version ") + AppVersion,
            "Project: Sudoku Reasoning Radar"
        };
        break;
    case OverlayPage::Analytics:
        lines = {
            "Grade: " + DifficultyAnalyzer::gradeName(difficultyReport.grade),
            "Score: " + std::to_string(difficultyReport.score),
            "Total steps: " + std::to_string(difficultyReport.stats.totalSteps),
            "Givens: " + std::to_string(difficultyReport.givens),
            "Empty cells: " + std::to_string(difficultyReport.emptyCells),
            "Hardest technique: " + difficultyReport.hardestTechnique,
            "",
            "Technique stats",
            "Naked Singles: " + std::to_string(difficultyReport.stats.nakedSingles),
            "Hidden Singles: " + std::to_string(difficultyReport.stats.hiddenSingles),
            "Locked Candidates: " + std::to_string(difficultyReport.stats.lockedCandidates),
            "Box-Line Reductions: " + std::to_string(difficultyReport.stats.boxLineReductions),
            "Naked Pairs: " + std::to_string(difficultyReport.stats.nakedPairs),
            "Hidden Pairs: " + std::to_string(difficultyReport.stats.hiddenPairs),
            "X-Wings: " + std::to_string(difficultyReport.stats.xWings),
            "Guesses: " + std::to_string(difficultyReport.stats.guesses),
            "Backtracks: " + std::to_string(difficultyReport.stats.backtracks),
            "Contradictions: " + std::to_string(difficultyReport.stats.contradictions),
            "",
            "Branch analysis",
            "Max branch depth: " + std::to_string(difficultyReport.maxBranchDepth),
            "",
            "Summary",
            difficultyReport.summary
        };
        break;
    case OverlayPage::Library: {
        lines.push_back("Saved puzzles: " + std::to_string(puzzleLibrary.count()));
        if (puzzleLibrary.count() == 0) {
            lines.push_back("No saved puzzles yet. Press Ctrl+S or Save Current to add one.");
        } else {
            lines.push_back("Use Up/Down to select and Enter to load.");
            const int start = std::max(0, librarySelection - 4);
            const int end = std::min(puzzleLibrary.count(), start + 9);
            for (int i = start; i < end; ++i) {
                const LibraryEntry* entry = puzzleLibrary.entryAt(i);
                if (!entry) {
                    continue;
                }
                std::ostringstream row;
                row << (i == librarySelection ? "> " : "  ")
                    << (i + 1) << ". " << entry->name
                    << " | " << entry->difficulty
                    << " | " << entry->createdAt;
                lines.push_back(row.str());
            }
        }
        lines.push_back("");
        lines.push_back(libraryStatus);
        break;
    }
    case OverlayPage::ImportExport:
        lines = {
            "Input format",
            "Type or paste 81 characters. Digits 1-9 are givens; 0 or . are empty cells.",
            "Enter imports the text box. Delete clears it. Ctrl+V pastes into it.",
            "Use OCR Import for PNG/JPG Sudoku screenshots.",
            "",
            "Current puzzle",
            PuzzleIO::boardToString(editBoard),
            "",
            ioStatus
        };
        break;
    case OverlayPage::OCRImport:
        lines = {
            "Workflow",
            "Open Image -> Detect Grid -> Run OCR -> Review cells -> Confirm Import.",
            "Auto Process runs the full pipeline in one step.",
            "",
            "Review",
            "Click a cell, press 1-9 to correct it, or Backspace/Delete/0 to clear.",
            "Low-confidence cells are amber. Rule conflicts are red.",
            "",
            "Limitations",
            "OCR is best for clear screenshots or straight photos of printed Sudoku grids.",
            "Strong perspective, shadows, blur, or handwriting may fail.",
            "Manual correction is part of the intended workflow.",
            "",
            ocrText()
        };
        break;
    case OverlayPage::Shortcuts:
        lines = {
            "Core",
            "Space: Solve / Play",
            "Enter: Execute selected command",
            "Left / Right: Step playback or command action",
            "Tab / Shift+Tab: Cycle command action",
            "O: OCR Import Assistant",
            "Esc: Close overlay / quit",
            "",
            "Puzzle",
            "G: Generate puzzle",
            "Shift+G: Generator page",
            "N: Built-in puzzle",
            "C: Clear",
            "R: Reset",
            "",
            "Hints",
            "F1: Gentle hint",
            "F2: Technique hint",
            "F3: Direct hint",
            "",
            "Modes",
            "M: Solver mode",
            "H: Candidate display",
            "K: Mistake mode",
            "F11: Fullscreen",
            "",
            "Library / I/O",
            "Ctrl+I: Import",
            "Ctrl+E: Copy puzzle",
            "Ctrl+Shift+E: Copy solution",
            "Ctrl+S: Save to library",
            "L: Library page"
        };
        break;
    case OverlayPage::Generator:
        lines = {
            "Difficulty: " + PuzzleGenerator::difficultyName(generatorDifficulty),
            "Current seed: " + (lastSeed.empty() ? "Random on next generation" : lastSeed),
            "Givens target: generated by selected difficulty",
            "",
            "Actions",
            "G: Generate current difficulty",
            "Command Deck: Change Difficulty",
            "Command Deck: Generate Puzzle",
            "",
            "Last generated",
            lastGenerated.givens > 0
                ? (std::string("Givens ") + std::to_string(lastGenerated.givens)
                    + ", time " + std::to_string(lastGenerated.generateTimeMs) + " ms"
                    + ", unique " + std::string(lastGenerated.unique ? "yes" : "no"))
                : "Not generated yet."
        };
        break;
    case OverlayPage::About:
        lines = {
            "Sudoku Reasoning Radar",
            std::string("Version ") + AppVersion,
            "A visual Sudoku engine that turns hidden logic into motion.",
            "GitHub: #github-link-placeholder"
        };
        break;
    case OverlayPage::None:
        break;
    }
    return lines;
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
    if (cellSources[selectedRow][selectedCol] == CellSource::Given && editBoard.isFixed(selectedRow, selectedCol)) {
        ioStatus = "Given cells are locked. Use Clear or import another puzzle to change givens.";
        return;
    }
    editBoard.setCellValue(selectedRow, selectedCol, number, false);
    cellSources[selectedRow][selectedCol] = number == 0 ? CellSource::Empty : CellSource::Player;
    initialBoard = editBoard;
    replayBoard = editBoard;
    lastSolveMs = 0;
    clearSolutionState();
    puzzleName = "Custom";
    updateMistakeDetection();
}

void App::markAllGivens() {
    clearCellSources();
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            if (editBoard.getCell(r, c) != 0) {
                cellSources[r][c] = CellSource::Given;
            }
        }
    }
}

void App::clearCellSources() {
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            cellSources[r][c] = CellSource::Empty;
        }
    }
}

void App::updateMistakeDetection() {
    mistakeCells.clear();
    if (mistakeMode == MistakeMode::Off) {
        mistakeStatus = "Mistake detection off.";
        return;
    }

    auto addMistake = [&](int row, int col) {
        if (!Board::isInside(row, col)) {
            return;
        }
        for (const CellRef& ref : mistakeCells) {
            if (ref.row == row && ref.col == col) {
                return;
            }
        }
        mistakeCells.push_back(CellRef{row, col});
    };

    if (mistakeMode == MistakeMode::RuleCheck) {
        for (int r = 0; r < Board::Size; ++r) {
            for (int c = 0; c < Board::Size; ++c) {
                const int value = editBoard.getCell(r, c);
                if (value == 0) {
                    continue;
                }
                for (int cc = 0; cc < Board::Size; ++cc) {
                    if (cc != c && editBoard.getCell(r, cc) == value) {
                        addMistake(r, c);
                        addMistake(r, cc);
                    }
                }
                for (int rr = 0; rr < Board::Size; ++rr) {
                    if (rr != r && editBoard.getCell(rr, c) == value) {
                        addMistake(r, c);
                        addMistake(rr, c);
                    }
                }
                const int box = Board::boxId(r, c);
                const int br = (box / Board::BoxSize) * Board::BoxSize;
                const int bc = (box % Board::BoxSize) * Board::BoxSize;
                for (int dr = 0; dr < Board::BoxSize; ++dr) {
                    for (int dc = 0; dc < Board::BoxSize; ++dc) {
                        const int rr = br + dr;
                        const int cc = bc + dc;
                        if ((rr != r || cc != c) && editBoard.getCell(rr, cc) == value) {
                            addMistake(r, c);
                            addMistake(rr, cc);
                        }
                    }
                }
            }
        }
        mistakeStatus = mistakeCells.empty()
            ? "RuleCheck: no row/column/box conflicts."
            : "RuleCheck: duplicate values highlighted.";
        return;
    }

    ensureSolutionCache();
    if (!hasSolutionCache) {
        mistakeStatus = "SolutionCheck needs a unique solution cache.";
        return;
    }
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int value = editBoard.getCell(r, c);
            if (value == 0 || cellSources[r][c] == CellSource::Given) {
                continue;
            }
            if (value != solutionCache.getCell(r, c)) {
                addMistake(r, c);
            }
        }
    }
    mistakeStatus = mistakeCells.empty()
        ? "SolutionCheck: player entries match the unique solution."
        : "SolutionCheck: highlighted entries differ from the unique solution.";
}

void App::ensureSolutionCache() {
    if (hasSolutionCache && solutionCache.isSolved()) {
        return;
    }

    Board base;
    base.clear();
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            if (cellSources[r][c] == CellSource::Given && editBoard.getCell(r, c) != 0) {
                base.setCellValue(r, c, editBoard.getCell(r, c), true);
            }
        }
    }
    if (base.emptyCount() == Board::Size * Board::Size) {
        base = editBoard;
    }

    Solver cacheSolver;
    const SolveResult result = cacheSolver.solveUniqueOrMultiple(base, SolverMode::Turbo);
    difficultyReport = difficultyAnalyzer.analyze(base, result, cacheSolver.getSteps());
    if (result == SolveResult::SolvedUnique && cacheSolver.getFinalBoard().isSolved()) {
        solutionCache = cacheSolver.getFinalBoard();
        hasSolutionCache = true;
    } else {
        solutionCache.clear();
        hasSolutionCache = false;
    }
}

std::string App::puzzleStringPreview() const {
    const std::string text = PuzzleIO::boardToString(editBoard);
    if (text.size() <= 20) {
        return text;
    }
    return text.substr(0, 20) + "...";
}
