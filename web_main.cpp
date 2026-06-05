#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "src/Board.h"
#include "src/CommandDeck.h"
#include "src/DifficultyAnalyzer.h"
#include "src/HintCoach.h"
#include "src/OverlayPages.h"
#include "src/PuzzleGenerator.h"
#include "src/Solver.h"
#include "src/StepRecorder.h"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 860;
constexpr int Margin = 28;
constexpr const char* WebVersionLabel = "Web Preview v0.3.0-preview.2";

SDL_Color bg{5, 8, 9, 255};
SDL_Color panel{12, 22, 23, 238};
SDL_Color text{235, 248, 244, 255};
SDL_Color muted{150, 170, 166, 255};
SDL_Color cyan{57, 223, 255, 255};
SDL_Color green{83, 239, 181, 255};
SDL_Color amber{255, 209, 102, 255};
SDL_Color red{255, 94, 128, 255};
SDL_Color purple{188, 132, 255, 255};

SDL_Color withAlpha(SDL_Color color, Uint8 alpha) {
    color.a = alpha;
    return color;
}

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

double easeOutCubic(double t) {
    t = clamp01(t);
    const double inv = 1.0 - t;
    return 1.0 - inv * inv * inv;
}

double easeOutBack(double t) {
    t = clamp01(t);
    constexpr double c1 = 1.70158;
    constexpr double c3 = c1 + 1.0;
    return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
}

enum class CandidateMode {
    Off,
    Focused,
    All
};

enum class MistakeMode {
    Off,
    RuleCheck
};

struct Button {
    std::string id;
    std::string label;
    SDL_Rect rect{};
};

struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    bool fullscreen = false;
    int width = WindowWidth;
    int height = WindowHeight;
    int selectedRow = -1;
    int selectedCol = -1;
    int stepIndex = -1;
    int commandIndex = CommandDeck::indexOf(CommandAction::GeneratePuzzle);
    SolverMode solverMode = SolverMode::Smart;
    PuzzleDifficulty difficulty = PuzzleDifficulty::Easy;
    CandidateMode candidateMode = CandidateMode::Focused;
    MistakeMode mistakeMode = MistakeMode::Off;
    Board board;
    Board initialBoard;
    Board replayBoard;
    Board finalBoard;
    Solver solver;
    PuzzleGenerator generator;
    HintCoach hints;
    Hint currentHint;
    DifficultyAnalyzer analyzer;
    DifficultyReport report;
    SolveResult lastResult = SolveResult::NoSolution;
    std::vector<SolveStep> steps;
    std::vector<std::string> library;
    std::vector<Button> buttons;
    std::string puzzleName = "Custom Empty";
    std::string status = "Web Preview ready. Enter digits or generate a puzzle.";
    std::string importText;
    std::string overlayTitle;
    std::string overlayBody;
    OverlayPage overlayPage = OverlayPage::None;
    bool importTextEditing = false;
    bool showOverlay = false;
    bool playing = false;
    bool paused = false;
    double speedMultiplier = 1.0;
    Uint32 lastFrameTicks = 0;
    Uint32 stepStartedTicks = 0;
};

struct LayoutState {
    SDL_Rect board{};
    SDL_Rect panel{};
    bool compact = false;
};

AppState* gApp = nullptr;
#ifdef __EMSCRIPTEN__
bool gFirstFrameLogged = false;
#endif

int clampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

std::string modeName(SolverMode mode) {
    switch (mode) {
    case SolverMode::HumanLogic:
        return "Human Logic";
    case SolverMode::Smart:
        return "Smart Solver";
    case SolverMode::Turbo:
        return "Turbo Exact Cover";
    }
    return "Smart Solver";
}

std::string resultName(SolveResult result) {
    switch (result) {
    case SolveResult::InvalidInput:
        return "Invalid Input";
    case SolveResult::NoSolution:
        return "No Solution";
    case SolveResult::SolvedUnique:
        return "Solved Unique";
    case SolveResult::MultipleSolutions:
        return "Multiple Solutions";
    }
    return "Unknown";
}

std::string candidateModeName(CandidateMode mode) {
    switch (mode) {
    case CandidateMode::Off:
        return "Off";
    case CandidateMode::Focused:
        return "Focused";
    case CandidateMode::All:
        return "All";
    }
    return "Focused";
}

std::string mistakeModeName(MistakeMode mode) {
    switch (mode) {
    case MistakeMode::Off:
        return "Off";
    case MistakeMode::RuleCheck:
        return "Rule Check";
    }
    return "Off";
}

std::string difficultyName(PuzzleDifficulty difficulty) {
    return PuzzleGenerator::difficultyName(difficulty);
}

std::string gradeName(const DifficultyReport& report) {
    return DifficultyAnalyzer::gradeName(report.grade);
}

void webLog(const std::string& message) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ console.log(UTF8ToString($0)); }, message.c_str());
#else
    (void)message;
#endif
}

std::string stepName(StepType type) {
    switch (type) {
    case StepType::AnalyzeCell:
        return "Analyze";
    case StepType::RemoveCandidate:
    case StepType::CandidateRemovedByLogic:
        return "Remove Candidate";
    case StepType::NakedSingle:
        return "Naked Single";
    case StepType::HiddenSingle:
        return "Hidden Single";
    case StepType::LockedCandidate:
        return "Locked Candidate";
    case StepType::BoxLineReduction:
        return "Box-Line";
    case StepType::NakedPair:
        return "Naked Pair";
    case StepType::HiddenPair:
        return "Hidden Pair";
    case StepType::XWing:
        return "X-Wing";
    case StepType::Guess:
        return "Guess";
    case StepType::PlaceNumber:
        return "Place";
    case StepType::Contradiction:
        return "Contradiction";
    case StepType::Backtrack:
        return "Backtrack";
    case StepType::Solved:
    case StepType::TurboSolved:
        return "Solved";
    case StepType::NoSolution:
        return "No Solution";
    case StepType::MultipleSolutions:
        return "Multiple";
    case StepType::InvalidInput:
        return "Invalid";
    case StepType::ModeChanged:
        return "Mode";
    }
    return "Step";
}

std::string cellName(int row, int col) {
    return "r" + std::to_string(row + 1) + "c" + std::to_string(col + 1);
}

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fill(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    setColor(renderer, color);
    SDL_RenderFillRect(renderer, &rect);
}

void stroke(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color, int thickness = 1) {
    setColor(renderer, color);
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect r{rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2};
        SDL_RenderDrawRect(renderer, &r);
    }
}

void lineTo(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, SDL_Color color) {
    setColor(renderer, color);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

const unsigned char* glyphRows(char ch) {
    static const unsigned char blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const unsigned char colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const unsigned char slash[7] = {1, 2, 2, 4, 8, 8, 16};
    static const unsigned char comma[7] = {0, 0, 0, 0, 0, 12, 4};
    static const unsigned char question[7] = {14, 17, 1, 2, 4, 0, 4};
    static const unsigned char lparen[7] = {2, 4, 8, 8, 8, 4, 2};
    static const unsigned char rparen[7] = {8, 4, 2, 2, 2, 4, 8};
    static const unsigned char lessThan[7] = {0, 2, 4, 8, 4, 2, 0};
    static const unsigned char greaterThan[7] = {0, 8, 4, 2, 4, 8, 0};
    static const unsigned char equals[7] = {0, 0, 31, 0, 31, 0, 0};
    static const unsigned char plus[7] = {0, 4, 4, 31, 4, 4, 0};
    static const unsigned char glyphs[43][7] = {
        {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14}, {14, 17, 1, 2, 4, 8, 31},
        {30, 1, 1, 14, 1, 1, 30}, {2, 6, 10, 18, 31, 2, 2}, {31, 16, 30, 1, 1, 17, 14},
        {6, 8, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8}, {14, 17, 17, 14, 17, 17, 14},
        {14, 17, 17, 15, 1, 2, 12}, {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
        {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31},
        {31, 16, 16, 30, 16, 16, 16}, {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17},
        {14, 4, 4, 4, 4, 4, 14}, {1, 1, 1, 1, 17, 17, 14}, {17, 18, 20, 24, 20, 18, 17},
        {16, 16, 16, 16, 16, 16, 31}, {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
        {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13},
        {30, 17, 17, 30, 20, 18, 17}, {15, 16, 16, 14, 1, 1, 30}, {31, 4, 4, 4, 4, 4, 4},
        {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4}, {17, 17, 17, 21, 21, 27, 17},
        {17, 17, 10, 4, 10, 17, 17}, {17, 17, 10, 4, 4, 4, 4}, {31, 1, 2, 4, 8, 16, 31},
        {0, 0, 0, 31, 0, 0, 0}, {0, 0, 0, 0, 0, 12, 12}, {0, 0, 0, 0, 0, 0, 4},
        {0, 0, 0, 0, 0, 0, 0}, {4, 4, 4, 4, 4, 0, 4}, {10, 10, 0, 0, 0, 0, 0}, {0, 4, 14, 31, 14, 4, 0}
    };
    if (ch >= '0' && ch <= '9') {
        return glyphs[ch - '0'];
    }
    if (ch >= 'a' && ch <= 'z') {
        ch = static_cast<char>(ch - 'a' + 'A');
    }
    if (ch >= 'A' && ch <= 'Z') {
        return glyphs[10 + ch - 'A'];
    }
    if (ch == '-') {
        return glyphs[36];
    }
    if (ch == '.') {
        return glyphs[37];
    }
    if (ch == '_') {
        return glyphs[38];
    }
    if (ch == ' ') {
        return glyphs[39];
    }
    if (ch == '!') {
        return glyphs[40];
    }
    if (ch == '"') {
        return glyphs[41];
    }
    if (ch == '*') {
        return glyphs[42];
    }
    if (ch == ':') {
        return colon;
    }
    if (ch == '/') {
        return slash;
    }
    if (ch == ',') {
        return comma;
    }
    if (ch == '?') {
        return question;
    }
    if (ch == '(') {
        return lparen;
    }
    if (ch == ')') {
        return rparen;
    }
    if (ch == '<') {
        return lessThan;
    }
    if (ch == '>') {
        return greaterThan;
    }
    if (ch == '=') {
        return equals;
    }
    if (ch == '+') {
        return plus;
    }
    return blank;
}

int textWidth(const std::string& text, int scale) {
    return static_cast<int>(text.size()) * 6 * scale;
}

void drawText(SDL_Renderer* renderer, std::string text, int x, int y, int scale, SDL_Color color) {
    setColor(renderer, color);
    for (char ch : text) {
        const unsigned char* rows = glyphRows(ch);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((rows[row] & (1 << (4 - col))) == 0) {
                    continue;
                }
                SDL_Rect px{x + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer, &px);
            }
        }
        x += 6 * scale;
    }
}

void drawCenteredText(SDL_Renderer* renderer, const std::string& text, const SDL_Rect& rect, int scale, SDL_Color color) {
    const int w = textWidth(text, scale);
    drawText(renderer, text, rect.x + (rect.w - w) / 2, rect.y + (rect.h - 7 * scale) / 2, scale, color);
}

std::string ellipsize(const std::string& input, int maxPixels, int scale) {
    if (maxPixels <= 0 || textWidth(input, scale) <= maxPixels) {
        return input;
    }
    const std::string suffix = "...";
    const int suffixW = textWidth(suffix, scale);
    std::string out;
    for (char ch : input) {
        if (textWidth(out, scale) + textWidth(std::string(1, ch), scale) + suffixW > maxPixels) {
            break;
        }
        out.push_back(ch);
    }
    return out + suffix;
}

void drawTextInRect(SDL_Renderer* renderer,
                    const std::string& value,
                    const SDL_Rect& rect,
                    int scale,
                    SDL_Color color) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    drawText(renderer, ellipsize(value, rect.w, scale), rect.x, rect.y, scale, color);
}

int drawWrappedText(SDL_Renderer* renderer,
                    const std::string& value,
                    const SDL_Rect& rect,
                    int scale,
                    SDL_Color color,
                    int maxLines = 0,
                    int lineSpacing = 4) {
    if (rect.w <= 0 || rect.h <= 0 || value.empty()) {
        return 0;
    }
    const int lineH = 7 * scale + lineSpacing;
    const int rectMaxLines = std::max(1, rect.h / std::max(1, lineH));
    const int allowedLines = maxLines > 0 ? std::min(maxLines, rectMaxLines) : rectMaxLines;
    std::vector<std::string> lines;
    std::string current;
    std::string word;
    auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (textWidth(candidate, scale) <= rect.w) {
            current = candidate;
        } else {
            if (!current.empty()) {
                lines.push_back(current);
            }
            current = word;
        }
        word.clear();
    };
    for (char ch : value) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            flushWord();
        } else {
            word.push_back(ch);
        }
    }
    flushWord();
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        lines.push_back(value);
    }
    int drawn = 0;
    for (int i = 0; i < static_cast<int>(lines.size()) && i < allowedLines; ++i) {
        std::string line = lines[static_cast<size_t>(i)];
        if (i == allowedLines - 1 && static_cast<int>(lines.size()) > allowedLines) {
            line = ellipsize(line + " ...", rect.w, scale);
        } else {
            line = ellipsize(line, rect.w, scale);
        }
        drawText(renderer, line, rect.x, rect.y + i * lineH, scale, color);
        ++drawn;
    }
    return drawn;
}

void drawCard(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color border = SDL_Color{61, 105, 111, 135}) {
    fill(renderer, rect, SDL_Color{7, 17, 18, 215});
    stroke(renderer, rect, border);
}

void drawLabelValue(SDL_Renderer* renderer,
                    const std::string& label,
                    const std::string& value,
                    const SDL_Rect& rect,
                    int scale,
                    SDL_Color valueColor = text) {
    const int labelW = std::min(rect.w / 2, 112);
    drawTextInRect(renderer, label, SDL_Rect{rect.x, rect.y, labelW, rect.h}, scale, muted);
    drawTextInRect(renderer, value, SDL_Rect{rect.x + labelW + 8, rect.y, rect.w - labelW - 8, rect.h}, scale, valueColor);
}

LayoutState layoutFor(const AppState& app) {
    LayoutState layout;
    const int margin = clampInt(app.width / 56, 16, Margin);
    const int headerH = app.height < 620 ? 40 : 56;
    const int gap = clampInt(app.width / 64, 24, 40);
    const int contentX = margin;
    const int contentY = margin + headerH;
    const int contentW = std::max(1, app.width - margin * 2);
    const int contentH = std::max(1, app.height - margin * 2 - headerH);
    layout.compact = app.width < 1040 || contentW < 820;

    if (!layout.compact) {
        const int panelW = clampInt(app.width / 4, 360, 460);
        const int boardLimit = std::max(240, contentW - panelW - gap);
        const int boardSize = std::max(260, std::min(contentH, boardLimit));
        const int totalW = boardSize + gap + panelW;
        const int startX = std::max(contentX, (app.width - totalW) / 2);
        const int startY = contentY + std::max(0, (contentH - boardSize) / 2);
        layout.board = SDL_Rect{startX, startY, boardSize, boardSize};
        layout.panel = SDL_Rect{startX + boardSize + gap, startY, panelW, boardSize};
        return layout;
    }

    const int panelH = clampInt(contentH / 2, 340, 440);
    const int boardSize = std::max(220, std::min(contentW, contentH - panelH - gap));
    const int startX = std::max(contentX, (app.width - boardSize) / 2);
    layout.board = SDL_Rect{startX, contentY, boardSize, boardSize};
    layout.panel = SDL_Rect{contentX, contentY + boardSize + gap, contentW, panelH};
    return layout;
}

SDL_Rect boardRect(const AppState& app) {
    return layoutFor(app).board;
}

SDL_Rect panelRect(const AppState& app) {
    return layoutFor(app).panel;
}

SDL_Rect overlayRect(const AppState& app) {
    const int w = std::min(app.width - 48, 760);
    const int h = std::min(app.height - 72, app.overlayPage == OverlayPage::ImportExport ? 430 : 520);
    return SDL_Rect{(app.width - w) / 2, (app.height - h) / 2, w, h};
}

void buildButtons(AppState& app) {
    app.buttons.clear();
    const SDL_Rect panelBox = panelRect(app);
    const int pad = 18;
    const int buttonH = panelBox.h < 300 ? 30 : 38;
    const int y = panelBox.y + panelBox.h - pad - buttonH;
    const int navW = clampInt(panelBox.w / 7, 42, 58);
    const int execW = std::max(100, panelBox.w - pad * 2 - navW * 2 - 16);
    int x = panelBox.x + pad;
    app.buttons.push_back(Button{"cmd_prev", "<", SDL_Rect{x, y, navW, buttonH}});
    x += navW + 8;
    app.buttons.push_back(Button{"cmd_exec", "Execute", SDL_Rect{x, y, execW, buttonH}});
    x += execW + 8;
    app.buttons.push_back(Button{"cmd_next", ">", SDL_Rect{x, y, navW, buttonH}});
}

void applyResize(AppState& app, int width, int height) {
    width = std::max(1, width);
    height = std::max(1, height);
    if (width == app.width && height == app.height) {
        return;
    }
    app.width = width;
    app.height = height;
    if (app.window) {
        SDL_SetWindowSize(app.window, width, height);
    }
    if (app.renderer) {
        SDL_RenderSetScale(app.renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(app.renderer, nullptr);
    }
    buildButtons(app);
}

void syncCanvasSize(AppState& app) {
#ifdef __EMSCRIPTEN__
    double cssWidth = 0.0;
    double cssHeight = 0.0;
    emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
    const int width = std::max(1, static_cast<int>(std::lround(cssWidth)));
    const int height = std::max(1, static_cast<int>(std::lround(cssHeight)));
    if (width != app.width || height != app.height) {
        applyResize(app, width, height);
    }
#else
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(app.renderer, &width, &height);
    if (width > 0 && height > 0 && (width != app.width || height != app.height)) {
        applyResize(app, width, height);
    }
#endif
}

bool parsePuzzleString(const std::string& text, Board& out, std::string* status) {
    std::string digits;
    for (char ch : text) {
        if ((ch >= '0' && ch <= '9') || ch == '.') {
            digits.push_back(ch == '.' ? '0' : ch);
        }
    }
    if (digits.size() != 81) {
        if (status) {
            *status = "Need 81 digits or dots.";
        }
        return false;
    }
    std::array<std::array<int, Board::Size>, Board::Size> grid{};
    for (int i = 0; i < 81; ++i) {
        grid[i / 9][i % 9] = digits[i] - '0';
    }
    if (!out.load(grid, true)) {
        if (status) {
            *status = "Invalid puzzle string.";
        }
        return false;
    }
    return true;
}

std::string boardString(const Board& board) {
    std::string out;
    out.reserve(81);
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            out.push_back(static_cast<char>('0' + board.getCell(r, c)));
        }
    }
    return out;
}

void saveBrowserLibrary(const std::string& puzzle) {
#ifdef __EMSCRIPTEN__
    std::string script = "localStorage.setItem('sudoku_reasoning_radar_last', '" + puzzle + "');";
    emscripten_run_script(script.c_str());
#else
    (void)puzzle;
#endif
}

std::string loadBrowserLibrary() {
#ifdef __EMSCRIPTEN__
    char* raw = emscripten_run_script_string("localStorage.getItem('sudoku_reasoning_radar_last') || ''");
    std::string value = raw ? raw : "";
    if (raw) {
        free(raw);
    }
    return value;
#else
    return {};
#endif
}

void applyStep(Board& replay, const SolveStep& step) {
    if ((step.type == StepType::RemoveCandidate || step.type == StepType::CandidateRemovedByLogic
            || step.type == StepType::LockedCandidate || step.type == StepType::BoxLineReduction
            || step.type == StepType::NakedPair || step.type == StepType::HiddenPair
            || step.type == StepType::XWing)
        && Board::isInside(step.row, step.col)) {
        if (step.removedMask != 0) {
            replay.removeCandidates(step.row, step.col, step.removedMask);
        } else if (step.maskAfter != 0) {
            replay.setCandidateMask(step.row, step.col, step.maskAfter);
        }
    }
    if (step.type == StepType::PlaceNumber || step.type == StepType::NakedSingle || step.type == StepType::HiddenSingle
        || step.type == StepType::Guess || step.type == StepType::Solved || step.type == StepType::TurboSolved) {
        if (Board::isInside(step.row, step.col) && step.number >= 1 && step.number <= 9) {
            replay.setCellValue(step.row, step.col, step.number, false);
        }
    }
}

void rebuildReplay(AppState& app) {
    app.replayBoard = app.initialBoard;
    for (int i = 0; i <= app.stepIndex && i < static_cast<int>(app.steps.size()); ++i) {
        applyStep(app.replayBoard, app.steps[static_cast<size_t>(i)]);
    }
}

Uint32 delayForStep(const AppState& app) {
    if (app.stepIndex < 0 || app.stepIndex >= static_cast<int>(app.steps.size())) {
        return 220;
    }
    const StepType type = app.steps[static_cast<size_t>(app.stepIndex)].type;
    Uint32 base = 260;
    if (type == StepType::Guess) {
        base = 520;
    } else if (type == StepType::Contradiction || type == StepType::Backtrack) {
        base = 620;
    } else if (type == StepType::Solved || type == StepType::TurboSolved) {
        base = 720;
    } else if (type == StepType::RemoveCandidate || type == StepType::CandidateRemovedByLogic) {
        base = 180;
    }
    return static_cast<Uint32>(std::max(80.0, static_cast<double>(base) / std::max(0.25, app.speedMultiplier)));
}

void advanceStep(AppState& app, bool manual = false) {
    if (app.stepIndex + 1 < static_cast<int>(app.steps.size())) {
        ++app.stepIndex;
        rebuildReplay(app);
        app.stepStartedTicks = SDL_GetTicks();
        if (manual) {
            app.paused = true;
        }
        webLog("Current step: " + std::to_string(app.stepIndex + 1) + "/" + std::to_string(app.steps.size()));
    } else {
        app.playing = false;
        app.paused = true;
        if (app.lastResult == SolveResult::SolvedUnique) {
            app.board = app.finalBoard;
            app.replayBoard = app.finalBoard;
        }
    }
}

void retreatStep(AppState& app) {
    if (app.stepIndex >= 0) {
        --app.stepIndex;
        rebuildReplay(app);
        app.paused = true;
        app.playing = !app.steps.empty();
        app.stepStartedTicks = SDL_GetTicks();
    }
}

void clearTrace(AppState& app) {
    app.steps.clear();
    app.stepIndex = -1;
    app.playing = false;
    app.paused = false;
    app.currentHint = Hint{};
    app.replayBoard = app.board;
}

void solve(AppState& app) {
    app.initialBoard = app.board;
    app.replayBoard = app.initialBoard;
    app.status = "Solving...";
    webLog("Solve started");
    const SolveResult result = app.solver.solve(app.board, app.solverMode);
    app.lastResult = result;
    app.steps = app.solver.getSteps();
    app.finalBoard = app.solver.getFinalBoard();
    app.report = app.analyzer.analyze(app.board, result, app.steps);
    app.stepIndex = app.steps.empty() ? -1 : 0;
    app.playing = !app.steps.empty();
    app.paused = false;
    app.stepStartedTicks = SDL_GetTicks();
    app.status = resultName(result) + ". " + std::to_string(app.steps.size()) + " trace steps.";
    rebuildReplay(app);
    webLog("Solve result: " + resultName(result));
    webLog("Steps recorded: " + std::to_string(app.steps.size()));
    if (app.playing) {
        webLog("Playback started");
        webLog("Current step: 1/" + std::to_string(app.steps.size()));
    }
}

void generate(AppState& app) {
    GeneratedPuzzle generated = app.generator.generate(app.difficulty);
    app.board = generated.puzzle;
    app.initialBoard = app.board;
    app.replayBoard = app.board;
    app.finalBoard = generated.solution;
    clearTrace(app);
    app.replayBoard = app.board;
    app.puzzleName = difficultyName(app.difficulty) + " Generated";
    app.status = "Generated " + PuzzleGenerator::difficultyName(app.difficulty) + " puzzle. Seed " + generated.seed + ".";
    app.difficulty = PuzzleGenerator::nextDifficulty(app.difficulty);
}

void requestHint(AppState& app, HintLevel level = HintLevel::Technique) {
    Hint hint = app.hints.getHint(app.board, level);
    app.currentHint = hint;
    if (!hint.available) {
        app.status = "No hint available for this board.";
        return;
    }
    app.status = "Hint: " + HintCoach::techniqueName(hint.technique) + " at " + cellName(hint.row, hint.col) + ". " + hint.message;
}

void analyze(AppState& app) {
    Solver s;
    SolveResult result = s.solve(app.board, SolverMode::Smart);
    DifficultyReport report = app.analyzer.analyze(app.board, result, s.getSteps());
    std::ostringstream out;
    out << "Grade " << DifficultyAnalyzer::gradeName(report.grade)
        << " / score " << report.score
        << " / steps " << report.stats.totalSteps
        << " / hardest " << report.hardestTechnique;
    app.status = out.str();
}

void showOverlay(AppState& app, OverlayPage page, const std::string& body = "") {
    app.overlayPage = page;
    app.overlayTitle = overlayTitle(page);
    app.overlayBody = body;
    app.showOverlay = page != OverlayPage::None;
    app.importTextEditing = page == OverlayPage::ImportExport;
    if (page == OverlayPage::ImportExport && app.importText.empty()) {
        app.importText = boardString(app.board);
    }
}

void closeOverlay(AppState& app) {
    app.overlayPage = OverlayPage::None;
    app.overlayTitle.clear();
    app.overlayBody.clear();
    app.importTextEditing = false;
    app.showOverlay = false;
}

CommandItem currentCommand(const AppState& app) {
    return CommandDeck::describe(CommandDeck::actionAt(app.commandIndex), true);
}

void cycleCommand(AppState& app, int delta) {
    const int total = CommandDeck::count();
    app.commandIndex = (app.commandIndex + delta) % total;
    if (app.commandIndex < 0) {
        app.commandIndex += total;
    }
}

void cycleSolverMode(AppState& app) {
    app.solverMode = app.solverMode == SolverMode::HumanLogic ? SolverMode::Smart
        : app.solverMode == SolverMode::Smart ? SolverMode::Turbo : SolverMode::HumanLogic;
    app.status = "Mode: " + modeName(app.solverMode);
}

void cycleCandidateMode(AppState& app) {
    app.candidateMode = app.candidateMode == CandidateMode::Off ? CandidateMode::Focused
        : app.candidateMode == CandidateMode::Focused ? CandidateMode::All : CandidateMode::Off;
    app.status = "Candidates: " + candidateModeName(app.candidateMode);
}

void cycleMistakeMode(AppState& app) {
    app.mistakeMode = app.mistakeMode == MistakeMode::Off ? MistakeMode::RuleCheck : MistakeMode::Off;
    app.status = "Mistake mode: " + mistakeModeName(app.mistakeMode);
}

void clearBoard(AppState& app) {
    app.board.clear();
    app.initialBoard.clear();
    app.replayBoard.clear();
    app.finalBoard.clear();
    app.puzzleName = "Custom Empty";
    clearTrace(app);
    app.status = "Board cleared.";
}

void resetBoard(AppState& app) {
    app.board = app.initialBoard;
    app.replayBoard = app.initialBoard;
    clearTrace(app);
    app.status = "Reset to current puzzle snapshot.";
}

void importPuzzleFromText(AppState& app, const std::string& textValue) {
    Board imported;
    std::string status;
    if (parsePuzzleString(textValue, imported, &status)) {
        app.board = imported;
        app.initialBoard = imported;
        app.replayBoard = imported;
        app.puzzleName = "Imported Puzzle";
        clearTrace(app);
        app.importText = boardString(app.board);
        app.status = "Imported puzzle string.";
        closeOverlay(app);
    } else {
        app.status = status;
    }
}

void saveOrLoadLibrary(AppState& app) {
    const std::string stored = loadBrowserLibrary();
    if (!stored.empty() && stored != boardString(app.board)) {
        Board loaded;
        std::string status;
        if (parsePuzzleString(stored, loaded, &status)) {
            app.board = loaded;
            app.initialBoard = loaded;
            app.replayBoard = loaded;
            app.puzzleName = "Browser Library";
            clearTrace(app);
            app.status = "Loaded browser library puzzle.";
            showOverlay(app, OverlayPage::Library, "Loaded the last browser library puzzle. Browser persistence is limited in this preview.");
            return;
        }
    }
    saveBrowserLibrary(boardString(app.board));
    showOverlay(app, OverlayPage::Library, "Saved current puzzle to browser localStorage. Browser library persistence is limited in this preview.");
}

void executeCommand(AppState& app, CommandAction action) {
    switch (action) {
    case CommandAction::Solve:
        solve(app);
        break;
    case CommandAction::GentleHint:
        requestHint(app, HintLevel::Gentle);
        break;
    case CommandAction::TechniqueHint:
        requestHint(app, HintLevel::Technique);
        break;
    case CommandAction::DirectHint:
        requestHint(app, HintLevel::Direct);
        break;
    case CommandAction::ApplyHint: {
        std::string status;
        if (app.hints.applyHint(app.board, app.currentHint, &status)) {
            app.initialBoard = app.board;
            app.replayBoard = app.board;
            clearTrace(app);
        }
        app.status = status.empty() ? "No direct hint to apply." : status;
        break;
    }
    case CommandAction::GeneratePuzzle:
        generate(app);
        break;
    case CommandAction::ChangeGeneratorDifficulty:
        app.difficulty = PuzzleGenerator::nextDifficulty(app.difficulty);
        app.status = "Generator difficulty: " + difficultyName(app.difficulty);
        break;
    case CommandAction::ToggleSolverMode:
        cycleSolverMode(app);
        break;
    case CommandAction::ToggleCandidateDisplay:
        cycleCandidateMode(app);
        break;
    case CommandAction::ToggleMistakeMode:
        cycleMistakeMode(app);
        break;
    case CommandAction::OCRImportImage:
        showOverlay(app, OverlayPage::OCRImport, "OCR Import is available in the Windows version. Browser OCR support is planned for a later release.");
        break;
    case CommandAction::ImportClipboard:
        showOverlay(app, OverlayPage::ImportExport);
        break;
    case CommandAction::CopyPuzzle:
        SDL_SetClipboardText(boardString(app.board).c_str());
        app.status = "Copied puzzle string to clipboard.";
        break;
    case CommandAction::CopySolution:
        SDL_SetClipboardText(boardString(app.finalBoard).c_str());
        app.status = "Copied solution string to clipboard.";
        break;
    case CommandAction::SaveLibrary:
    case CommandAction::OpenLibrary:
        saveOrLoadLibrary(app);
        break;
    case CommandAction::OpenAnalytics:
        analyze(app);
        showOverlay(app, OverlayPage::Analytics);
        break;
    case CommandAction::OpenSettings:
        showOverlay(app, OverlayPage::Settings);
        break;
    case CommandAction::ClearBoard:
        clearBoard(app);
        break;
    case CommandAction::ResetBoard:
        resetBoard(app);
        break;
    case CommandAction::TurboSolve:
        app.solverMode = SolverMode::Turbo;
        solve(app);
        break;
    }
}

void activate(AppState& app, const std::string& id) {
    if (id == "cmd_prev") {
        cycleCommand(app, -1);
    } else if (id == "cmd_next") {
        cycleCommand(app, 1);
    } else if (id == "cmd_exec") {
        executeCommand(app, currentCommand(app).action);
    }
}

void handleText(AppState& app, const char* raw) {
    if (!raw) {
        return;
    }
    Board imported;
    std::string status;
    if (parsePuzzleString(raw, imported, &status)) {
        app.board = imported;
        app.initialBoard = imported;
        app.replayBoard = imported;
        app.puzzleName = "Clipboard Import";
        clearTrace(app);
        app.importText = boardString(app.board);
        app.status = "Imported puzzle string from clipboard.";
    } else {
        app.status = status;
    }
}

void handleEvent(AppState& app, const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        app.running = false;
        return;
    }
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        applyResize(app, event.window.data1, event.window.data2);
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (app.showOverlay) {
            const SDL_Rect modal = overlayRect(app);
            SDL_Point p{event.button.x, event.button.y};
            if (!SDL_PointInRect(&p, &modal)) {
                closeOverlay(app);
            }
            return;
        }
        for (const Button& button : app.buttons) {
            SDL_Point p{event.button.x, event.button.y};
            if (SDL_PointInRect(&p, &button.rect)) {
                activate(app, button.id);
                return;
            }
        }
        const SDL_Rect br = boardRect(app);
        SDL_Point p{event.button.x, event.button.y};
        if (SDL_PointInRect(&p, &br)) {
            const int cell = br.w / 9;
            app.selectedCol = clampInt((event.button.x - br.x) / cell, 0, 8);
            app.selectedRow = clampInt((event.button.y - br.y) / cell, 0, 8);
        }
        return;
    }
    if (event.type == SDL_TEXTINPUT && app.importTextEditing) {
        app.importText += event.text.text;
        app.status = "Import text " + std::to_string(app.importText.size()) + " characters.";
        return;
    }
    if (event.type == SDL_KEYDOWN) {
        const SDL_Keycode key = event.key.keysym.sym;
        const bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
        const bool shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
        if (app.showOverlay) {
            if (key == SDLK_ESCAPE) {
                closeOverlay(app);
                return;
            }
            if (app.importTextEditing) {
                if (ctrl && key == SDLK_v && SDL_HasClipboardText()) {
                    char* raw = SDL_GetClipboardText();
                    if (raw) {
                        app.importText += raw;
                        SDL_free(raw);
                    }
                    app.status = "Pasted into import text box.";
                    return;
                }
                if (key == SDLK_BACKSPACE && !app.importText.empty()) {
                    app.importText.pop_back();
                    app.status = "Import text edited.";
                    return;
                }
                if (key == SDLK_DELETE) {
                    app.importText.clear();
                    app.status = "Import text cleared.";
                    return;
                }
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    importPuzzleFromText(app, app.importText);
                    return;
                }
            }
            return;
        }
        if (key >= SDLK_1 && key <= SDLK_9 && app.selectedRow >= 0 && app.selectedCol >= 0) {
            app.board.setCellValue(app.selectedRow, app.selectedCol, static_cast<int>(key - SDLK_0), true);
            app.initialBoard = app.board;
            app.replayBoard = app.board;
            clearTrace(app);
            return;
        }
        if ((key == SDLK_0 || key == SDLK_DELETE || key == SDLK_BACKSPACE) && app.selectedRow >= 0 && app.selectedCol >= 0) {
            app.board.removeNumber(app.selectedRow, app.selectedCol, true);
            app.initialBoard = app.board;
            app.replayBoard = app.board;
            clearTrace(app);
            return;
        }
        if (key == SDLK_TAB) {
            cycleCommand(app, shift ? -1 : 1);
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            executeCommand(app, currentCommand(app).action);
        } else if (key == SDLK_ESCAPE) {
            closeOverlay(app);
        } else if (key == SDLK_SPACE) {
            if (!app.steps.empty()) {
                app.paused = !app.paused;
                app.playing = true;
                app.status = app.paused ? "Playback paused." : "Playback resumed.";
            } else {
                solve(app);
            }
        } else if (key == SDLK_g) {
            generate(app);
        } else if (key == SDLK_h) {
            requestHint(app);
        } else if (key == SDLK_m) {
            cycleSolverMode(app);
        } else if (key == SDLK_RIGHT && app.stepIndex + 1 < static_cast<int>(app.steps.size())) {
            advanceStep(app, true);
        } else if (key == SDLK_LEFT && app.stepIndex >= 0) {
            retreatStep(app);
        } else if (ctrl && key == SDLK_v && SDL_HasClipboardText()) {
            char* raw = SDL_GetClipboardText();
            handleText(app, raw);
            SDL_free(raw);
        } else if (ctrl && key == SDLK_c) {
            SDL_SetClipboardText(boardString(app.board).c_str());
            app.status = "Copied puzzle string to clipboard.";
        } else if (key == SDLK_p) {
            app.paused = !app.paused;
            app.playing = !app.steps.empty();
        } else if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) {
            app.speedMultiplier = std::min(4.0, app.speedMultiplier * 1.25);
            app.status = "Animation speed " + std::to_string(static_cast<int>(app.speedMultiplier * 100.0)) + "%.";
        } else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
            app.speedMultiplier = std::max(0.25, app.speedMultiplier / 1.25);
            app.status = "Animation speed " + std::to_string(static_cast<int>(app.speedMultiplier * 100.0)) + "%.";
        }
    }
}

void drawBackground(AppState& app) {
    fill(app.renderer, SDL_Rect{0, 0, app.width, app.height}, bg);
    setColor(app.renderer, SDL_Color{34, 60, 62, 95});
    for (int x = 0; x < app.width; x += 42) {
        SDL_RenderDrawLine(app.renderer, x, 0, x, app.height);
    }
    for (int y = 0; y < app.height; y += 42) {
        SDL_RenderDrawLine(app.renderer, 0, y, app.width, y);
    }
}

SDL_Color accentForStep(StepType type) {
    switch (type) {
    case StepType::NakedSingle:
    case StepType::PlaceNumber:
    case StepType::Solved:
    case StepType::TurboSolved:
        return green;
    case StepType::HiddenSingle:
    case StepType::LockedCandidate:
    case StepType::BoxLineReduction:
    case StepType::NakedPair:
    case StepType::HiddenPair:
    case StepType::XWing:
        return cyan;
    case StepType::Guess:
        return purple;
    case StepType::Contradiction:
        return red;
    case StepType::Backtrack:
        return SDL_Color{130, 145, 148, 255};
    default:
        return amber;
    }
}

bool sameUnit(int r, int c, int sr, int sc) {
    if (!Board::isInside(sr, sc)) {
        return false;
    }
    return r == sr || c == sc || Board::boxId(r, c) == Board::boxId(sr, sc);
}

bool isPlacementStep(StepType type) {
    return type == StepType::PlaceNumber
        || type == StepType::NakedSingle
        || type == StepType::HiddenSingle
        || type == StepType::Guess
        || type == StepType::Solved
        || type == StepType::TurboSolved;
}

bool isRemovalStep(StepType type) {
    return type == StepType::RemoveCandidate
        || type == StepType::CandidateRemovedByLogic
        || type == StepType::LockedCandidate
        || type == StepType::BoxLineReduction
        || type == StepType::NakedPair
        || type == StepType::HiddenPair
        || type == StepType::XWing;
}

double stepAgeMs(const AppState& app) {
    if (app.stepStartedTicks == 0) {
        return 0.0;
    }
    const Uint32 now = SDL_GetTicks();
    return static_cast<double>(now - app.stepStartedTicks);
}

double stepProgress(const AppState& app, double durationMs) {
    return clamp01(stepAgeMs(app) / std::max(1.0, durationMs));
}

SDL_Rect insetRect(SDL_Rect rect, int inset) {
    return SDL_Rect{rect.x + inset, rect.y + inset, std::max(1, rect.w - inset * 2), std::max(1, rect.h - inset * 2)};
}

bool shouldShowCandidates(const AppState& app, int row, int col, const SolveStep* current) {
    if (app.candidateMode == CandidateMode::Off) {
        return false;
    }
    if (app.candidateMode == CandidateMode::All) {
        return true;
    }
    if (row == app.selectedRow && col == app.selectedCol) {
        return true;
    }
    if (current && sameUnit(row, col, current->row, current->col)) {
        return true;
    }
    return false;
}

void drawBoard(AppState& app) {
    const SDL_Rect br = boardRect(app);
    const int cell = br.w / 9;
    const Board& display = (!app.steps.empty() || app.stepIndex >= 0) ? app.replayBoard : app.board;
    const SolveStep* current = nullptr;
    if (app.stepIndex >= 0 && app.stepIndex < static_cast<int>(app.steps.size())) {
        current = &app.steps[static_cast<size_t>(app.stepIndex)];
    }

    fill(app.renderer, br, SDL_Color{5, 13, 14, 248});

    // 1-5: backgrounds and soft highlights. Text is drawn later so highlights never cover digits.
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            SDL_Rect cr{br.x + c * cell, br.y + r * cell, cell, cell};
            const bool related = current && sameUnit(r, c, current->row, current->col);
            const bool selected = r == app.selectedRow && c == app.selectedCol;
            const bool focused = current && r == current->row && c == current->col;

            fill(app.renderer, insetRect(cr, 1), SDL_Color{8, 20, 21, 92});

            if (related) {
                fill(app.renderer, cr, SDL_Color{57, 223, 255, 22});
            }
            if (selected) {
                fill(app.renderer, cr, SDL_Color{83, 239, 181, 42});
            }
            if (current && sameUnit(r, c, current->row, current->col)) {
                const double sweep = std::fmod(stepAgeMs(app) / 900.0, 1.0);
                const bool rowSweep = r == current->row && c == clampInt(static_cast<int>(sweep * 9.0), 0, 8);
                const bool colSweep = c == current->col && r == clampInt(static_cast<int>(sweep * 9.0), 0, 8);
                if (rowSweep || colSweep) {
                    fill(app.renderer, cr, SDL_Color{57, 223, 255, 20});
                }
            }
            if (focused) {
                SDL_Color focus = accentForStep(current->type);
                Uint8 fillAlpha = 52;
                if (current->type == StepType::Contradiction) {
                    focus = red;
                    fillAlpha = 44;
                } else if (current->type == StepType::Guess) {
                    focus = purple;
                    fillAlpha = 50;
                } else if (current->type == StepType::Backtrack) {
                    focus = SDL_Color{130, 145, 148, 255};
                    fillAlpha = 36;
                }
                fill(app.renderer, cr, withAlpha(focus, fillAlpha));
            }
            if (current && isRemovalStep(current->type) && r == current->row && c == current->col) {
                fill(app.renderer, cr, SDL_Color{255, 145, 79, 22});
            }
        }
    }

    // 6: grid lines.
    for (int i = 0; i <= 9; ++i) {
        SDL_Color color = (i % 3 == 0) ? SDL_Color{225, 242, 238, 214} : SDL_Color{125, 160, 154, 118};
        const int thickness = (i % 3 == 0) ? std::max(2, cell / 32) : 1;
        for (int t = 0; t < thickness; ++t) {
            lineTo(app.renderer, br.x, br.y + i * cell + t, br.x + br.w, br.y + i * cell + t, color);
            lineTo(app.renderer, br.x + i * cell + t, br.y, br.x + i * cell + t, br.y + br.h, color);
        }
    }

    // 7: candidates, muted by default and brighter only when relevant.
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            SDL_Rect cr{br.x + c * cell, br.y + r * cell, cell, cell};
            if (display.getCell(r, c) != 0 || !shouldShowCandidates(app, r, c, current)) {
                continue;
            }
            const int mask = display.getCandidates(r, c);
            const bool selected = r == app.selectedRow && c == app.selectedCol;
            const bool related = current && sameUnit(r, c, current->row, current->col);
            const int candidateScale = std::max(1, cell / 42);
            for (int n = 1; n <= 9; ++n) {
                const int bit = Board::bitForNumber(n);
                const bool present = (mask & bit) != 0;
                const bool removedNow = current && r == current->row && c == current->col && (current->removedMask & bit) != 0;
                if (!present && !removedNow) {
                    continue;
                }
                const int sx = (n - 1) % 3;
                const int sy = (n - 1) / 3;
                SDL_Rect mini{cr.x + sx * cell / 3 + cell / 24,
                              cr.y + sy * cell / 3 + cell / 26,
                              cell / 3 - cell / 12,
                              cell / 3 - cell / 12};
                SDL_Color candidateColor = muted;
                candidateColor.a = app.candidateMode == CandidateMode::All ? 118 : 172;
                if (selected || related) {
                    candidateColor = SDL_Color{182, 213, 207, 214};
                }
                if (removedNow) {
                    const double fade = 1.0 - easeOutCubic(stepProgress(app, 280.0));
                    candidateColor = SDL_Color{255, 126, 96, static_cast<Uint8>(std::max(72.0, 255.0 * fade))};
                }
                drawCenteredText(app.renderer, std::to_string(n), mini, candidateScale, candidateColor);
                if (removedNow) {
                    lineTo(app.renderer,
                           mini.x + mini.w / 5,
                           mini.y + mini.h / 2,
                           mini.x + mini.w * 4 / 5,
                           mini.y + mini.h / 2,
                           withAlpha(red, candidateColor.a));
                }
            }
        }
    }

    // 8: main digits.
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            SDL_Rect cr{br.x + c * cell, br.y + r * cell, cell, cell};
            const int value = display.getCell(r, c);
            if (value != 0) {
                const bool focused = current && r == current->row && c == current->col;
                SDL_Color color = display.isFixed(r, c) ? SDL_Color{235, 248, 244, 255} : SDL_Color{83, 225, 245, 255};
                if (focused) {
                    color = accentForStep(current->type);
                }
                int scale = std::max(3, cell / 17);
                if (focused && current && isPlacementStep(current->type)) {
                    const double t = easeOutBack(stepProgress(app, 220.0));
                    scale = std::max(scale, static_cast<int>(std::lround(scale * (1.0 + 0.35 * (1.0 - t)))));
                    color.a = static_cast<Uint8>(std::clamp(70.0 + 185.0 * t, 0.0, 255.0));
                }
                drawCenteredText(app.renderer, std::to_string(value), cr, scale, color);
            }
        }
    }

    // 9-10: outlines and radar pulses after text. Only strokes are used so digits remain readable.
    if (current && Board::isInside(current->row, current->col)) {
        SDL_Rect cr{br.x + current->col * cell, br.y + current->row * cell, cell, cell};
        SDL_Color accent = accentForStep(current->type);
        const double pulse = stepProgress(app, current->type == StepType::Contradiction ? 420.0 : 620.0);
        const int grow = static_cast<int>(std::lround(cell * 0.18 * easeOutCubic(pulse)));
        SDL_Rect ring{cr.x - grow / 2, cr.y - grow / 2, cr.w + grow, cr.h + grow};
        const Uint8 ringAlpha = static_cast<Uint8>(std::clamp(150.0 * (1.0 - pulse), 28.0, 150.0));
        stroke(app.renderer, ring, withAlpha(accent, ringAlpha), current->type == StepType::Contradiction ? 3 : 2);

        if (current->type == StepType::Guess) {
            stroke(app.renderer, insetRect(cr, 3), withAlpha(purple, 210), 2);
        } else if (current->type == StepType::Backtrack) {
            stroke(app.renderer, insetRect(cr, 3), SDL_Color{130, 145, 148, 190}, 2);
        } else if (current->type == StepType::Contradiction) {
            const double flash = 1.0 - stepProgress(app, 260.0);
            stroke(app.renderer, insetRect(cr, 2), withAlpha(red, static_cast<Uint8>(150 + flash * 95)), 3);
        } else {
            stroke(app.renderer, insetRect(cr, 3), withAlpha(accent, 170), 2);
        }
    }

    if (app.selectedRow >= 0 && app.selectedCol >= 0) {
        SDL_Rect cr{br.x + app.selectedCol * cell, br.y + app.selectedRow * cell, cell, cell};
        stroke(app.renderer, insetRect(cr, 4), SDL_Color{83, 239, 181, 165}, 1);
    }

    if (app.mistakeMode == MistakeMode::RuleCheck) {
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                const int value = display.getCell(r, c);
                if (value == 0) {
                    continue;
                }
                bool conflict = false;
                for (int i = 0; i < 9 && !conflict; ++i) {
                    conflict = (i != c && display.getCell(r, i) == value)
                        || (i != r && display.getCell(i, c) == value);
                }
                const int boxRow = (r / 3) * 3;
                const int boxCol = (c / 3) * 3;
                for (int rr = boxRow; rr < boxRow + 3 && !conflict; ++rr) {
                    for (int cc = boxCol; cc < boxCol + 3; ++cc) {
                        if ((rr != r || cc != c) && display.getCell(rr, cc) == value) {
                            conflict = true;
                            break;
                        }
                    }
                }
                if (conflict) {
                    SDL_Rect cr{br.x + c * cell, br.y + r * cell, cell, cell};
                    stroke(app.renderer, insetRect(cr, 5), withAlpha(red, 205), 2);
                }
            }
        }
    }
}

void drawButtons(AppState& app) {
    for (const Button& button : app.buttons) {
        const bool execute = button.id == "cmd_exec";
        fill(app.renderer, button.rect, execute ? SDL_Color{16, 35, 35, 238} : SDL_Color{11, 24, 25, 228});
        stroke(app.renderer, button.rect, execute ? SDL_Color{83, 239, 181, 170} : SDL_Color{82, 150, 162, 155});
        drawCenteredText(app.renderer,
                         button.label,
                         button.rect,
                         button.rect.h < 34 ? 2 : 3,
                         execute ? text : SDL_Color{195, 218, 213, 235});
    }
}

void drawPanel(AppState& app) {
    const SDL_Rect pr = panelRect(app);
    fill(app.renderer, pr, panel);
    stroke(app.renderer, pr, SDL_Color{57, 223, 255, 100});
    const bool compact = pr.h < 430;
    const int pad = compact ? 14 : 18;
    const int x = pr.x + pad;
    const int w = pr.w - pad * 2;
    int y = pr.y + pad;
    drawTextInRect(app.renderer, "Sudoku Reasoning Radar", SDL_Rect{x, y, w, 24}, compact ? 2 : 3, text);
    SDL_Rect badge{x, y + (compact ? 24 : 30), std::min(w, 252), 24};
    fill(app.renderer, badge, SDL_Color{10, 31, 30, 225});
    stroke(app.renderer, badge, SDL_Color{83, 239, 181, 120});
    drawTextInRect(app.renderer, WebVersionLabel, SDL_Rect{badge.x + 8, badge.y + 5, badge.w - 16, 14}, 2, green);
    y += compact ? 56 : 66;

    const int statusH = compact ? 82 : 136;
    SDL_Rect statusCard{x, y, w, statusH};
    drawCard(app.renderer, statusCard);
    int rowY = statusCard.y + 12;
    const int rowH = compact ? 16 : 19;
    drawLabelValue(app.renderer, "Mode", modeName(app.solverMode), SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2);
    rowY += rowH;
    drawLabelValue(app.renderer, "Status", app.playing && !app.paused ? "Playing Steps" : app.status, SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2, amber);
    rowY += rowH;
    drawLabelValue(app.renderer, "Puzzle", app.puzzleName, SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2);
    if (!compact) {
        rowY += rowH;
        drawLabelValue(app.renderer, "Difficulty", app.report.summary == "No analysis yet." ? difficultyName(app.difficulty) : gradeName(app.report),
                       SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2);
        rowY += rowH;
        drawLabelValue(app.renderer, "Candidates", candidateModeName(app.candidateMode), SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2);
        rowY += rowH;
        drawLabelValue(app.renderer, "Mistakes", mistakeModeName(app.mistakeMode), SDL_Rect{statusCard.x + 12, rowY, statusCard.w - 24, rowH}, 2);
    }
    y += statusH + 10;

    const Board& display = (!app.steps.empty() || app.stepIndex >= 0) ? app.replayBoard : app.board;
    if (!compact) {
        const int selectedH = 64;
        SDL_Rect selectedCard{x, y, w, selectedH};
        drawCard(app.renderer, selectedCard);
        const std::string selected = app.selectedRow >= 0 ? cellName(app.selectedRow, app.selectedCol) : "None";
        std::string candidates = "-";
        if (app.selectedRow >= 0 && app.selectedCol >= 0 && display.getCell(app.selectedRow, app.selectedCol) == 0) {
            candidates.clear();
            const int mask = display.getCandidates(app.selectedRow, app.selectedCol);
            for (int n = 1; n <= 9; ++n) {
                if ((mask & Board::bitForNumber(n)) != 0) {
                    candidates.push_back(static_cast<char>('0' + n));
                    candidates.push_back(' ');
                }
            }
        }
        drawLabelValue(app.renderer, "Selected", selected, SDL_Rect{selectedCard.x + 12, selectedCard.y + 11, selectedCard.w - 24, 18}, 2, cyan);
        drawLabelValue(app.renderer, "Candidates", candidates, SDL_Rect{selectedCard.x + 12, selectedCard.y + 34, selectedCard.w - 24, 18}, 2);
        y += selectedH + 10;
    }

    const int buttonY = app.buttons.empty() ? pr.y + pr.h - pad - 38 : app.buttons.front().rect.y;
    const int deckH = compact ? 72 : 94;
    const int progressH = 58;
    const int deckY = buttonY - deckH - 10;
    const int progressY = deckY - progressH - 10;
    const int reasonBottom = std::max(y + 84, progressY - 12);
    SDL_Rect reasonCard{x, y, w, std::max(54, reasonBottom - y)};
    drawCard(app.renderer, reasonCard, SDL_Color{57, 223, 255, 120});
    drawText(app.renderer, "Focus", reasonCard.x + 12, reasonCard.y + 12, 2, green);
    std::string reasonText = app.status;
    std::string badgeText = "Ready";
    SDL_Color reasonColor = text;
    SDL_Color badgeColor = green;
    if (!app.steps.empty() && app.stepIndex >= 0 && app.stepIndex < static_cast<int>(app.steps.size())) {
        const SolveStep& step = app.steps[static_cast<size_t>(app.stepIndex)];
        std::ostringstream out;
        out << "Step " << (app.stepIndex + 1) << "/" << app.steps.size() << ": " << stepName(step.type);
        if (Board::isInside(step.row, step.col)) {
            out << " at " << cellName(step.row, step.col);
        }
        if (step.number > 0) {
            out << " = " << step.number;
        }
        out << ". " << step.reason;
        reasonText = out.str();
        reasonColor = accentForStep(step.type);
        badgeText = stepName(step.type);
        badgeColor = reasonColor;
    } else if (app.currentHint.available) {
        reasonText = app.currentHint.message + " " + app.currentHint.explanation;
        reasonColor = cyan;
        badgeText = "Hint";
        badgeColor = cyan;
    }
    SDL_Rect techniqueBadge{reasonCard.x + reasonCard.w - 136, reasonCard.y + 10, 124, 24};
    fill(app.renderer, techniqueBadge, withAlpha(badgeColor, 34));
    stroke(app.renderer, techniqueBadge, withAlpha(badgeColor, 140));
    drawCenteredText(app.renderer, badgeText, techniqueBadge, 2, withAlpha(badgeColor, 235));
    const double reasonEase = easeOutCubic(stepProgress(app, 170.0));
    const int reasonOffset = static_cast<int>(std::lround(5.0 * (1.0 - reasonEase)));
    reasonColor.a = static_cast<Uint8>(std::clamp(176.0 + 79.0 * reasonEase, 176.0, 255.0));
    drawWrappedText(app.renderer,
                    reasonText,
                    SDL_Rect{reasonCard.x + 12, reasonCard.y + 42 + reasonOffset, reasonCard.w - 24, reasonCard.h - 54},
                    2,
                    reasonColor,
                    compact ? 2 : 4);

    SDL_Rect progressCard{x, progressY, w, progressH};
    drawCard(app.renderer, progressCard);
    const int totalSteps = static_cast<int>(app.steps.size());
    const int visibleStep = app.stepIndex < 0 ? 0 : app.stepIndex + 1;
    const SDL_Color stepAccent = totalSteps > 0 && app.stepIndex >= 0 ? accentForStep(app.steps[static_cast<size_t>(app.stepIndex)].type) : cyan;
    fill(app.renderer, SDL_Rect{progressCard.x + 12, progressCard.y + 12, 10, 10}, withAlpha(stepAccent, 220));
    drawTextInRect(app.renderer,
                   "Step " + std::to_string(visibleStep) + " / " + std::to_string(totalSteps),
                   SDL_Rect{progressCard.x + 30, progressCard.y + 8, progressCard.w - 42, 18},
                   2,
                   muted);
    SDL_Rect track{progressCard.x + 12, progressCard.y + 36, progressCard.w - 24, 8};
    fill(app.renderer, track, SDL_Color{35, 48, 49, 255});
    const double progress = totalSteps <= 0 ? 0.0 : static_cast<double>(visibleStep) / static_cast<double>(totalSteps);
    SDL_Rect bar{track.x, track.y, static_cast<int>(track.w * std::max(0.0, std::min(1.0, progress))), track.h};
    fill(app.renderer, bar, stepAccent);

    SDL_Rect deckCard{x, deckY, w, deckH};
    drawCard(app.renderer, deckCard, SDL_Color{188, 132, 255, 120});
    const CommandItem command = currentCommand(app);
    drawTextInRect(app.renderer, "Command Deck", SDL_Rect{deckCard.x + 12, deckCard.y + 10, deckCard.w - 24, 18}, 2, purple);
    drawTextInRect(app.renderer,
                   std::to_string(app.commandIndex + 1) + "/" + std::to_string(CommandDeck::count()) + "  " + command.label,
                   SDL_Rect{deckCard.x + 12, deckCard.y + 32, deckCard.w - 24, 18},
                   2,
                   text);
    if (!compact) {
        drawWrappedText(app.renderer, command.description, SDL_Rect{deckCard.x + 12, deckCard.y + 56, deckCard.w - 24, 28}, 1, muted, 2);
    }
    drawButtons(app);
}

std::vector<std::string> overlayLines(const AppState& app) {
    std::vector<std::string> lines;
    switch (app.overlayPage) {
    case OverlayPage::Settings:
        lines.push_back("Solver mode: " + modeName(app.solverMode));
        lines.push_back("Candidate display: " + candidateModeName(app.candidateMode));
        lines.push_back("Mistake mode: " + mistakeModeName(app.mistakeMode));
        lines.push_back("Animation speed: " + std::to_string(static_cast<int>(app.speedMultiplier * 100.0)) + "%");
        lines.push_back("Use Command Deck actions to change these values.");
        break;
    case OverlayPage::Analytics:
        lines.push_back("Grade: " + gradeName(app.report));
        lines.push_back("Score: " + std::to_string(app.report.score));
        lines.push_back("Total steps: " + std::to_string(app.report.stats.totalSteps));
        lines.push_back("Givens: " + std::to_string(app.report.givens));
        lines.push_back("Empty cells: " + std::to_string(app.report.emptyCells));
        lines.push_back("Hardest technique: " + app.report.hardestTechnique);
        lines.push_back("Naked Singles: " + std::to_string(app.report.stats.nakedSingles));
        lines.push_back("Hidden Singles: " + std::to_string(app.report.stats.hiddenSingles));
        lines.push_back("Locked Candidates: " + std::to_string(app.report.stats.lockedCandidates));
        lines.push_back("Guesses: " + std::to_string(app.report.stats.guesses));
        lines.push_back("Backtracks: " + std::to_string(app.report.stats.backtracks));
        break;
    case OverlayPage::Library:
        lines.push_back(app.overlayBody.empty() ? "Browser library persistence is limited in this preview." : app.overlayBody);
        lines.push_back("This Web build stores one puzzle in browser localStorage.");
        lines.push_back("Use Save To Library or Open Library from the Command Deck.");
        break;
    case OverlayPage::ImportExport:
        lines.push_back("Paste or type an 81-character puzzle string. Digits 1-9 are givens; 0 or dot means empty.");
        lines.push_back("Press Enter to import. Ctrl+V pastes. Delete clears the text box.");
        lines.push_back("Current puzzle can be copied with the Copy Puzzle command.");
        break;
    case OverlayPage::OCRImport:
        lines.push_back(app.overlayBody.empty()
                            ? "OCR Import is available in the Windows version. Browser OCR support is planned for a later release."
                            : app.overlayBody);
        lines.push_back("The Web build intentionally excludes OpenCV and Tesseract.");
        break;
    case OverlayPage::Shortcuts:
        lines.push_back("Click a cell, then press 1-9 to enter a digit.");
        lines.push_back("0 / Backspace / Delete clears the selected cell.");
        lines.push_back("Tab / Shift+Tab cycles Command Deck actions.");
        lines.push_back("Enter executes the current command.");
        lines.push_back("Space solves or pauses/resumes playback.");
        lines.push_back("Left / Right steps through the reasoning trace.");
        lines.push_back("Ctrl+V imports a puzzle string from clipboard.");
        lines.push_back("Esc closes overlays.");
        break;
    case OverlayPage::Generator:
        lines.push_back("Generator difficulty: " + difficultyName(app.difficulty));
        lines.push_back("Use Change Difficulty or Generate Puzzle in the Command Deck.");
        break;
    case OverlayPage::About:
        lines.push_back(std::string("Sudoku Reasoning Radar ") + WebVersionLabel + ".");
        lines.push_back("This preview shares the solver core and Command Deck model, with OCR disabled for browser builds.");
        break;
    case OverlayPage::None:
        break;
    }
    return lines;
}

void drawOverlay(AppState& app) {
    if (!app.showOverlay) {
        return;
    }
    fill(app.renderer, SDL_Rect{0, 0, app.width, app.height}, SDL_Color{0, 0, 0, 130});
    SDL_Rect rect = overlayRect(app);
    fill(app.renderer, rect, SDL_Color{5, 10, 11, 246});
    stroke(app.renderer, rect, SDL_Color{57, 223, 255, 180}, 2);
    const int pad = 24;
    int y = rect.y + pad;
    drawTextInRect(app.renderer, app.overlayTitle.empty() ? "Panel" : app.overlayTitle, SDL_Rect{rect.x + pad, y, rect.w - pad * 2, 32}, 4, cyan);
    y += 50;

    if (app.overlayPage == OverlayPage::ImportExport) {
        SDL_Rect input{rect.x + pad, y, rect.w - pad * 2, 96};
        drawCard(app.renderer, input, SDL_Color{83, 239, 181, 130});
        drawText(app.renderer, "Puzzle String Text Box", input.x + 14, input.y + 12, 2, green);
        drawWrappedText(app.renderer,
                        app.importText.empty() ? "(empty)" : app.importText,
                        SDL_Rect{input.x + 14, input.y + 40, input.w - 28, input.h - 52},
                        2,
                        text,
                        2);
        y += input.h + 18;
    }

    const std::vector<std::string> lines = overlayLines(app);
    const int cardH = 48;
    const int gap = 10;
    for (const std::string& line : lines) {
        if (y + cardH > rect.y + rect.h - 52) {
            drawText(app.renderer, "...", rect.x + pad, y + 8, 2, muted);
            break;
        }
        SDL_Rect card{rect.x + pad, y, rect.w - pad * 2, cardH};
        drawCard(app.renderer, card);
        drawWrappedText(app.renderer, line, SDL_Rect{card.x + 14, card.y + 12, card.w - 28, card.h - 18}, 2, text, 2);
        y += cardH + gap;
    }
    drawText(app.renderer, "Esc closes this panel", rect.x + pad, rect.y + rect.h - 34, 2, amber);
}

void render(AppState& app) {
    drawBackground(app);
    const int headerX = clampInt(app.width / 48, 16, Margin);
    drawText(app.renderer, "Sudoku Reasoning Radar", headerX, 18, app.width < 900 ? 2 : 3, text);
    drawText(app.renderer, WebVersionLabel, headerX, app.width < 900 ? 40 : 48, 2, green);
    drawBoard(app);
    drawPanel(app);
    drawOverlay(app);
    SDL_RenderPresent(app.renderer);
}

void update(AppState& app) {
    const Uint32 now = SDL_GetTicks();
    if (app.lastFrameTicks == 0) {
        app.lastFrameTicks = now;
    }
    const Uint32 dt = std::min<Uint32>(now - app.lastFrameTicks, 50);
    app.lastFrameTicks = now;
    (void)dt;
    if (app.playing && !app.paused && !app.steps.empty()) {
        if (app.stepStartedTicks == 0) {
            app.stepStartedTicks = now;
        }
        if (now - app.stepStartedTicks >= delayForStep(app)) {
            advanceStep(app);
        }
    }
}

void frame(void* arg) {
    AppState* app = static_cast<AppState*>(arg);
    if (!app) {
        return;
    }
    syncCanvasSize(*app);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleEvent(*app, event);
    }
    update(*app);
    render(*app);
#ifdef __EMSCRIPTEN__
    if (!gFirstFrameLogged) {
        EM_ASM({ console.log("SRR first frame rendered"); });
        gFirstFrameLogged = true;
    }
#endif
}

bool initApp(AppState& app) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return false;
    }
    app.window = SDL_CreateWindow("Sudoku Reasoning Radar Web",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  WindowWidth,
                                  WindowHeight,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!app.window) {
        SDL_Quit();
        return false;
    }
#ifdef __EMSCRIPTEN__
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
#else
    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
    if (!app.renderer) {
        SDL_DestroyWindow(app.window);
        app.window = nullptr;
        SDL_Quit();
        return false;
    }
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(app.window, &app.width, &app.height);
    SDL_StartTextInput();
    app.board.initializeCandidates();
    app.initialBoard = app.board;
    app.replayBoard = app.board;
    buildButtons(app);
#ifndef __EMSCRIPTEN__
    syncCanvasSize(app);
#endif
    return true;
}

#ifndef __EMSCRIPTEN__
void shutdownApp(AppState& app) {
    if (app.renderer) {
        SDL_DestroyRenderer(app.renderer);
        app.renderer = nullptr;
    }
    if (app.window) {
        SDL_DestroyWindow(app.window);
        app.window = nullptr;
    }
    SDL_Quit();
}
#endif
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void SRR_OnCanvasResize(int width, int height) {
    if (gApp) {
        applyResize(*gApp, width, height);
    }
}
#endif

int main(int, char**) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ console.log("SRR Web main started"); });
    gApp = new AppState();
    if (!initApp(*gApp)) {
        std::string error = SDL_GetError();
        EM_ASM({ console.error("SRR App init failed: " + UTF8ToString($0)); }, error.c_str());
        delete gApp;
        gApp = nullptr;
        return 1;
    }
    EM_ASM({ console.log("SRR App init OK"); });
    EM_ASM({ console.log("SRR main loop registered"); });
    emscripten_set_main_loop_arg(frame, gApp, 0, 1);
    return 0;
#else
    AppState app;
    gApp = &app;
    if (!initApp(app)) {
        gApp = nullptr;
        return 1;
    }
    while (app.running) {
        frame(&app);
        SDL_Delay(16);
    }
    shutdownApp(app);
    gApp = nullptr;
    return 0;
#endif
}
