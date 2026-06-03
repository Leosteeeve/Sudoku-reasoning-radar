#include "Renderer.h"

#include "Animation.h"
#include "Theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

namespace {
SDL_Color color(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    return SDL_Color{r, g, b, a};
}

SDL_Color withAlpha(SDL_Color c, Uint8 a) {
    c.a = a;
    return c;
}

SDL_Color blend(SDL_Color a, SDL_Color b, float t) {
    t = Animation::clamp01(t);
    return color(static_cast<Uint8>(Animation::lerp(a.r, b.r, t)),
                 static_cast<Uint8>(Animation::lerp(a.g, b.g, t)),
                 static_cast<Uint8>(Animation::lerp(a.b, b.b, t)),
                 static_cast<Uint8>(Animation::lerp(a.a, b.a, t)));
}

bool pointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

bool sameCell(const SolveStep* step, int row, int col) {
    return step != nullptr && step->row == row && step->col == col;
}

bool hasPrimaryCell(const SolveStep* step) {
    return step != nullptr && Board::isInside(step->row, step->col);
}

bool sameBox(int rowA, int colA, int rowB, int colB) {
    return Board::isInside(rowA, colA)
        && Board::isInside(rowB, colB)
        && Board::boxId(rowA, colA) == Board::boxId(rowB, colB);
}

void logRuntime(const std::string& message) {
    std::ofstream log("D:/Soduku/sudoku_runtime.log", std::ios::app);
    log << message << "\n";
}

std::string groupedPuzzleText(const std::string& text) {
    if (text.empty()) {
        return "Type or paste an 81-character puzzle string here.";
    }

    std::string grouped;
    grouped.reserve(text.size() + 20);
    for (size_t i = 0; i < text.size(); ++i) {
        grouped.push_back(text[i]);
        if ((i + 1) % 9 == 0) {
            grouped.push_back('\n');
        } else if ((i + 1) % 3 == 0) {
            grouped.push_back(' ');
        }
    }
    return grouped;
}
}

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    logRuntime("Renderer initialize: SDL_Init");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        logRuntime(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }
    logRuntime("Renderer initialize: TTF_Init");
    if (TTF_Init() != 0) {
        logRuntime(std::string("TTF_Init failed: ") + TTF_GetError());
        SDL_Quit();
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    logRuntime("Renderer initialize: SDL_CreateWindow");
    window = SDL_CreateWindow("Sudoku Reasoning Radar",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WindowWidth,
                              WindowHeight,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        logRuntime(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        shutdown();
        return false;
    }
    SDL_ShowWindow(window);
    SDL_SetWindowMinimumSize(window, 900, 640);

    logRuntime("Renderer initialize: SDL_CreateRenderer");
    sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRenderer) {
        logRuntime(std::string("Accelerated renderer failed: ") + SDL_GetError());
        sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!sdlRenderer) {
        logRuntime(std::string("Fallback renderer failed: ") + SDL_GetError());
        shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
    refreshLayout();
    logRuntime("Renderer initialize: openFonts");
    if (!openFonts()) {
        logRuntime(std::string("openFonts failed: ") + TTF_GetError());
        shutdown();
        return false;
    }

    logRuntime("Renderer initialize: success");
    return true;
}

void Renderer::shutdown() {
    closeFonts();
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if (TTF_WasInit()) {
        TTF_Quit();
    }
    SDL_Quit();
}

void Renderer::render(const Board& board,
                      const std::vector<SolveStep>& steps,
                      int currentStep,
                      const RenderInfo& info,
                      const std::vector<UIButton>& buttons) {
    refreshLayout();

    const SolveStep* current = nullptr;
    if (currentStep >= 0 && currentStep < static_cast<int>(steps.size())) {
        current = &steps[static_cast<size_t>(currentStep)];
    }

    drawBackground(current);
    drawBoard(board, current, info);
    drawPanel(steps, currentStep, info, buttons);
    drawTimeline(steps, currentStep, info);
    drawOverlay(info, buttons);
    drawButtons(buttons);

    SDL_RenderPresent(sdlRenderer);
}

bool Renderer::cellFromPoint(int x, int y, int& row, int& col) const {
    refreshLayout();
    if (!pointInRect(x, y, layout.boardRect)) {
        return false;
    }
    col = (x - layout.boardRect.x) / layout.cellSize;
    row = (y - layout.boardRect.y) / layout.cellSize;
    return Board::isInside(row, col);
}

void Renderer::layoutButtons(std::vector<UIButton>& buttons,
                             int mouseX,
                             int mouseY,
                             const std::string& pressedId) const {
    refreshLayout();
    if (buttons.empty()) {
        return;
    }

    const int gap = 8;
    const int iconH = 28;
    const int iconW = 48;
    int iconX = layout.headerBlock.x + layout.headerBlock.w - iconW;
    const int iconY = layout.headerBlock.y + 2;
    const SDL_Rect deck = layout.controlsBlock;
    const int sideW = std::clamp(deck.w / 6, 42, 58);
    const int execH = std::clamp(deck.h / 3, 34, 44);
    const int selectorH = std::clamp(deck.h / 3, 34, 44);
    const int selectorY = deck.y + 8;
    const int execY = selectorY + selectorH + 10;
    const int drawerW = std::clamp(layout.width / 3, 420, 520);
    const SDL_Rect drawer{layout.width - drawerW - layout.safe / 2,
                          layout.safe / 2,
                          drawerW,
                          layout.height - layout.safe};
    const int drawerPad = std::clamp(drawer.w / 24, 18, 26);
    const int actionGap = 10;
    const int actionH = 34;
    const int actionW = (drawer.w - drawerPad * 2 - actionGap) / 2;
    const int actionRow2Y = drawer.y + drawer.h - drawerPad - actionH;
    const int actionRow1Y = actionRow2Y - actionGap - actionH;
    for (size_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i].id.rfind("icon_", 0) == 0) {
            buttons[i].rect = SDL_Rect{iconX, iconY, iconW, iconH};
            iconX -= iconW + gap;
        } else if (buttons[i].id == "deck_prev") {
            buttons[i].rect = SDL_Rect{deck.x, selectorY, sideW, selectorH};
        } else if (buttons[i].id == "deck_next") {
            buttons[i].rect = SDL_Rect{deck.x + deck.w - sideW, selectorY, sideW, selectorH};
        } else if (buttons[i].id == "deck_exec") {
            buttons[i].rect = SDL_Rect{deck.x, execY, deck.w, execH};
        } else if (buttons[i].id == "overlay_close") {
            buttons[i].rect = SDL_Rect{drawer.x + drawer.w - drawerPad - 86, drawer.y + 18, 86, 30};
        } else if (buttons[i].id == "overlay_paste_clipboard") {
            buttons[i].rect = SDL_Rect{drawer.x + drawerPad, actionRow1Y, actionW, actionH};
        } else if (buttons[i].id == "overlay_import_text") {
            buttons[i].rect = SDL_Rect{drawer.x + drawerPad + actionW + actionGap, actionRow1Y, actionW, actionH};
        } else if (buttons[i].id == "overlay_copy_puzzle") {
            buttons[i].rect = SDL_Rect{drawer.x + drawerPad, actionRow2Y, actionW, actionH};
        } else if (buttons[i].id == "overlay_copy_solution") {
            buttons[i].rect = SDL_Rect{drawer.x + drawerPad + actionW + actionGap, actionRow2Y, actionW, actionH};
        }
        buttons[i].hovered = buttons[i].enabled && pointInRect(mouseX, mouseY, buttons[i].rect);
        buttons[i].pressed = buttons[i].enabled && buttons[i].id == pressedId;
    }
}

void Renderer::toggleFullscreen() {
    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

bool Renderer::isFullscreen() const {
    return fullscreen;
}

SDL_Rect Renderer::cellRect(int row, int col) const {
    return SDL_Rect{layout.boardRect.x + col * layout.cellSize,
                    layout.boardRect.y + row * layout.cellSize,
                    layout.cellSize,
                    layout.cellSize};
}

void Renderer::refreshLayout() const {
    int width = WindowWidth;
    int height = WindowHeight;
    if (sdlRenderer) {
        SDL_GetRendererOutputSize(sdlRenderer, &width, &height);
    } else if (window) {
        SDL_GetWindowSize(window, &width, &height);
    }
    layout = computeLayout(width, height);
}

void Renderer::drawBackground(const SolveStep* current) {
    const Theme& theme = appTheme();
    for (int y = 0; y < layout.height; y += 4) {
        const float t = layout.height <= 1 ? 0.0f : static_cast<float>(y) / static_cast<float>(layout.height - 1);
        SDL_Rect band{0, y, layout.width, 4};
        fillRect(band, blend(theme.backgroundTop, theme.backgroundBottom, t));
    }

    const int grid = std::clamp(layout.width / 28, 34, 54);
    for (int y = layout.safe / 2; y < layout.height; y += grid) {
        drawLine(0, y, layout.width, y, theme.gridBlueprint, 1);
    }
    for (int x = layout.safe / 2; x < layout.width; x += grid) {
        drawLine(x, 0, x, layout.height, theme.gridBlueprint, 1);
    }

    const float seconds = SDL_GetTicks() / 1000.0f;
    const int sweepX = layout.boardRect.x
        + static_cast<int>((std::sin(seconds * 0.85f) + 1.0f) * 0.5f * layout.boardRect.w);
    drawLine(sweepX,
             layout.boardRect.y - 12,
             sweepX,
             layout.boardRect.y + layout.boardRect.h + 12,
             color(theme.accent.r, theme.accent.g, theme.accent.b, 36),
             3);

    for (int i = 28; i >= 8; i -= 7) {
        SDL_Rect glow{layout.boardRect.x - i,
                      layout.boardRect.y - i,
                      layout.boardRect.w + i * 2,
                      layout.boardRect.h + i * 2};
        fillRect(glow, color(theme.boardGlow.r, theme.boardGlow.g, theme.boardGlow.b, static_cast<Uint8>(10 + i)));
    }

    if (hasPrimaryCell(current)) {
        const SDL_Rect cell = cellRect(current->row, current->col);
        const SDL_Color accent = accentForStep(current->type);
        for (int i = 18; i >= 6; i -= 6) {
            SDL_Rect halo{cell.x - i, cell.y - i, cell.w + i * 2, cell.h + i * 2};
            fillRect(halo, color(accent.r, accent.g, accent.b, static_cast<Uint8>(18 + i * 2)));
        }
    }
}

void Renderer::drawBoard(const Board& board, const SolveStep* current, const RenderInfo& info) {
    const Theme& theme = appTheme();
    const SDL_Rect boardBg = layout.boardRect;
    fillRect(boardBg, theme.boardSurface);
    strokeRect(boardBg, withAlpha(theme.accent, 115), 1);

    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            SDL_Rect rect = cellRect(r, c);
            rect.x += 1;
            rect.y += 1;
            rect.w -= 2;
            rect.h -= 2;
            fillRect(rect, board.isFixed(r, c) ? theme.givenSurface : theme.cellSurface);
        }
    }

    if (current && current->unitType == static_cast<int>(UnitType::Row) && current->unitIndex >= 0) {
        for (int c = 0; c < Board::Size; ++c) {
            fillRect(cellRect(current->unitIndex, c), color(20, 184, 166, 42));
        }
    } else if (current && current->unitType == static_cast<int>(UnitType::Column) && current->unitIndex >= 0) {
        for (int r = 0; r < Board::Size; ++r) {
            fillRect(cellRect(r, current->unitIndex), color(20, 184, 166, 42));
        }
    } else if (current && current->unitType == static_cast<int>(UnitType::Box) && current->unitIndex >= 0) {
        const int br = (current->unitIndex / 3) * 3;
        const int bc = (current->unitIndex % 3) * 3;
        for (int dr = 0; dr < 3; ++dr) {
            for (int dc = 0; dc < 3; ++dc) {
                fillRect(cellRect(br + dr, bc + dc), color(20, 184, 166, 48));
            }
        }
    }

    if (hasPrimaryCell(current)) {
        const int box = Board::boxId(current->row, current->col);
        const int boxRow = (box / Board::BoxSize) * Board::BoxSize;
        const int boxCol = (box % Board::BoxSize) * Board::BoxSize;
        for (int c = 0; c < Board::Size; ++c) {
            fillRect(cellRect(current->row, c), color(14, 165, 233, 36));
        }
        for (int r = 0; r < Board::Size; ++r) {
            fillRect(cellRect(r, current->col), color(14, 165, 233, 36));
        }
        for (int dr = 0; dr < Board::BoxSize; ++dr) {
            for (int dc = 0; dc < Board::BoxSize; ++dc) {
                fillRect(cellRect(boxRow + dr, boxCol + dc), color(168, 85, 247, 38));
            }
        }
    }

    if (current && Board::isInside(current->relatedRow, current->relatedCol)) {
        fillRect(cellRect(current->relatedRow, current->relatedCol), color(248, 113, 113, 76));
    }
    if (current && Board::isInside(current->row2, current->col2)) {
        fillRect(cellRect(current->row2, current->col2), color(217, 70, 239, 68));
    }

    for (const CellRef& ref : info.hintCells) {
        if (Board::isInside(ref.row, ref.col)) {
            fillRect(cellRect(ref.row, ref.col), color(250, 204, 21, 38));
        }
    }
    for (const CellRef& ref : info.mistakeCells) {
        if (Board::isInside(ref.row, ref.col)) {
            fillRect(cellRect(ref.row, ref.col), color(248, 113, 113, 92));
            strokeRect(cellRect(ref.row, ref.col), color(248, 113, 113, 235), std::max(2, layout.cellSize / 28));
        }
    }

    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int value = board.getCell(r, c);
            const SDL_Rect rect = cellRect(r, c);
            if (value == 0) {
                drawCandidates(board, r, c, current, info);
                continue;
            }

            const bool activePlace = sameCell(current, r, c)
                && (current->type == StepType::PlaceNumber
                    || current->type == StepType::NakedSingle
                    || current->type == StepType::HiddenSingle
                    || current->type == StepType::Guess
                    || current->type == StepType::TurboSolved);

            TTF_Font* font = fontNumber;
            if (layout.cellSize < 48) {
                font = fontMedium;
            } else if (activePlace && info.stepAgeMs < 240) {
                font = fontNumberLarge;
            }

            SDL_Color digitColor = board.isFixed(r, c) ? theme.textPrimary : color(96, 205, 255);
            if (activePlace) {
                digitColor = accentForStep(current->type);
            }
            drawCenteredText(std::to_string(value), rect, font, digitColor);
        }
    }

    if (hasPrimaryCell(current)) {
        SDL_Rect rect = cellRect(current->row, current->col);
        const SDL_Color accent = accentForStep(current->type);
        const float pulse = Animation::pulseAlpha(SDL_GetTicks() / 1000.0f, 8.5f);
        fillRect(rect, color(accent.r, accent.g, accent.b, static_cast<Uint8>(44 + pulse * 66)));
        strokeRect(rect, color(accent.r, accent.g, accent.b, 240), std::max(2, layout.cellSize / 20));

        const int cx = rect.x + rect.w / 2;
        const int cy = rect.y + rect.h / 2;
        const int inset = std::max(7, layout.cellSize / 8);
        drawLine(rect.x + inset, cy, rect.x + rect.w - inset, cy, color(accent.r, accent.g, accent.b, 175), 2);
        drawLine(cx, rect.y + inset, cx, rect.y + rect.h - inset, color(accent.r, accent.g, accent.b, 175), 2);
    }

    if (info.selectedRow >= 0 && info.selectedCol >= 0) {
        strokeRect(cellRect(info.selectedRow, info.selectedCol), theme.warning, std::max(2, layout.cellSize / 24));
    }

    for (int i = 0; i <= Board::Size; ++i) {
        const int thickness = (i % 3 == 0) ? std::max(3, layout.cellSize / 18) : 1;
        const int x = layout.boardRect.x + i * layout.cellSize;
        const int y = layout.boardRect.y + i * layout.cellSize;
        const SDL_Color line = (i % 3 == 0) ? theme.gridThick : theme.gridThin;
        drawLine(x, layout.boardRect.y, x, layout.boardRect.y + layout.boardRect.h, line, thickness);
        drawLine(layout.boardRect.x, y, layout.boardRect.x + layout.boardRect.w, y, line, thickness);
    }
}

void Renderer::drawPanel(const std::vector<SolveStep>& steps,
                         int currentStep,
                         const RenderInfo& info,
                         const std::vector<UIButton>& buttons) {
    const Theme& theme = appTheme();
    const SDL_Rect panel = layout.panelRect;
    fillRect(panel, theme.panelSurface);
    strokeRect(panel, color(theme.panelBorder.r, theme.panelBorder.g, theme.panelBorder.b, 120), 1);

    drawText("Sudoku Reasoning Radar", layout.headerBlock.x, layout.headerBlock.y, fontMedium, theme.textPrimary);
    if (!info.versionText.empty()) {
        drawText(info.versionText,
                 layout.headerBlock.x,
                 layout.headerBlock.y + 30,
                 fontSmall,
                 theme.accent);
    }

    SDL_Rect row = layout.statusBlock;
    const int rowH = std::max(20, row.h / 5);
    drawLabelValue("Mode", info.solverModeText, SDL_Rect{row.x, row.y, row.w, rowH}, theme.textPrimary);
    drawLabelValue("Status", info.statusText, SDL_Rect{row.x, row.y + rowH, row.w, rowH}, theme.accent);
    drawLabelValue("Puzzle", info.puzzleName, SDL_Rect{row.x, row.y + rowH * 2, row.w, rowH}, theme.textPrimary);
    drawLabelValue("Difficulty", info.difficultyText, SDL_Rect{row.x, row.y + rowH * 3, row.w, rowH}, theme.textPrimary);
    drawLabelValue("Mistakes", info.mistakeModeText, SDL_Rect{row.x, row.y + rowH * 4, row.w, rowH}, theme.textPrimary);

    const int selectedRowH = std::max(24, layout.puzzleBlock.h / 2);
    drawLabelValue("Selected",
                   info.selectedCellText,
                   SDL_Rect{layout.puzzleBlock.x, layout.puzzleBlock.y, layout.puzzleBlock.w, selectedRowH},
                   theme.textPrimary);
    drawLabelValue("Cell Cand.",
                   info.selectedCandidatesText,
                   SDL_Rect{layout.puzzleBlock.x,
                            layout.puzzleBlock.y + selectedRowH,
                            layout.puzzleBlock.w,
                            selectedRowH},
                   theme.textPrimary);

    SDL_Rect stepPanel = layout.stepBlock;
    fillRect(stepPanel, color(8, 13, 25, 132));
    strokeRect(stepPanel, color(71, 85, 105, 92), 1);
    const int inset = std::clamp(stepPanel.w / 28, 10, 16);
    SDL_Rect stepInner{stepPanel.x + inset,
                       stepPanel.y + inset,
                       stepPanel.w - inset * 2,
                       stepPanel.h - inset * 2};

    drawText("Focus", stepInner.x, stepInner.y, fontSmall, theme.textSecondary);
    int y = stepInner.y + 24;
    y = drawWrappedText(info.focusText,
                        SDL_Rect{stepInner.x, y, stepInner.w, std::max(52, stepInner.h - 38)},
                        fontBody,
                        theme.textPrimary,
                        5,
                        4);

    if (!steps.empty() && currentStep >= 0 && currentStep < static_cast<int>(steps.size())) {
        const SolveStep& step = steps[static_cast<size_t>(currentStep)];
        const SDL_Color accent = accentForStep(step.type);
        drawWrappedText(describeStep(step, currentStep, static_cast<int>(steps.size())),
                        SDL_Rect{stepInner.x, y + 8, stepInner.w, 36},
                        fontSmall,
                        accent,
                        3,
                        1);
        if (step.depth > 0) {
            std::ostringstream depth;
            depth << "Assumption Depth: " << step.depth;
            drawText(depth.str(), stepInner.x, stepInner.y + stepInner.h - 24, fontSmall, theme.guess);
        }
    }

    drawText("Progress", layout.progressBlock.x, layout.progressBlock.y, fontSmall, theme.textSecondary);
    if (!steps.empty()) {
        const SDL_Rect progressBg{layout.progressBlock.x,
                                  layout.progressBlock.y + layout.progressBlock.h - 14,
                                  layout.progressBlock.w,
                                  8};
        fillRect(progressBg, color(30, 41, 59, 255));
        const double progress = currentStep < 0
            ? 0.0
            : static_cast<double>(currentStep + 1) / static_cast<double>(steps.size());
        SDL_Rect progressFill = progressBg;
        progressFill.w = static_cast<int>(progressBg.w * std::clamp(progress, 0.0, 1.0));
        fillRect(progressFill, accentForStep(steps[static_cast<size_t>(std::max(0, currentStep))].type));
        std::ostringstream label;
        label << "Step " << std::max(0, currentStep + 1) << " / " << steps.size();
        drawText(label.str(), layout.progressBlock.x + 72, layout.progressBlock.y, fontSmall, theme.textPrimary);
    } else {
        drawText("Step 0 / 0", layout.progressBlock.x + 72, layout.progressBlock.y, fontSmall, theme.textPrimary);
    }

    const SDL_Rect deck = layout.controlsBlock;
    fillRect(deck, color(8, 13, 25, 118));
    strokeRect(deck, color(71, 85, 105, 92), 1);
    const int deckInset = std::clamp(deck.w / 28, 10, 16);
    const SDL_Rect labelRect{deck.x + deckInset + 54, deck.y + 9, deck.w - deckInset * 2 - 108, 42};
    drawCenteredText(info.commandLabel, labelRect, fontBody, info.commandEnabled ? theme.textPrimary : theme.textMuted);
    std::ostringstream index;
    index << info.commandIndex << "/" << info.commandTotal;
    drawText(index.str(), deck.x + deckInset, deck.y + 62, fontSmall, theme.textSecondary);
    drawWrappedText(info.commandDescription,
                    SDL_Rect{deck.x + deckInset + 44, deck.y + 58, deck.w - deckInset * 2 - 44, 54},
                    fontSmall,
                    theme.textSecondary,
                    3,
                    2);

    (void)buttons;
}

void Renderer::drawTimeline(const std::vector<SolveStep>& steps, int currentStep, const RenderInfo& info) {
    const Theme& theme = appTheme();
    const SDL_Rect lineRect = layout.timelineRect;
    (void)steps;
    (void)currentStep;
    fillRect(lineRect, color(8, 13, 25, 104));
    const int pad = std::clamp(lineRect.w / 64, 12, 20);
    drawWrappedText(info.shortStatusText.empty() ? info.statusText : info.shortStatusText,
                    SDL_Rect{lineRect.x + pad, lineRect.y + 9, lineRect.w - pad * 2, lineRect.h - 12},
                    fontSmall,
                    theme.textSecondary,
                    2,
                    1);
}

void Renderer::drawOverlay(const RenderInfo& info, const std::vector<UIButton>& buttons) {
    if (info.overlayPage == OverlayPage::None) {
        return;
    }

    const Theme& theme = appTheme();
    const int drawerW = std::clamp(layout.width / 3, 420, 520);
    const SDL_Rect scrim{0, 0, layout.width, layout.height};
    const SDL_Rect drawer{layout.width - drawerW - layout.safe / 2,
                          layout.safe / 2,
                          drawerW,
                          layout.height - layout.safe};
    fillRect(scrim, color(0, 0, 0, 126));
    fillRect(drawer, color(7, 14, 15, 248));
    strokeRect(drawer, color(theme.accent.r, theme.accent.g, theme.accent.b, 170), 1);

    const int pad = std::clamp(drawer.w / 24, 18, 26);
    int y = drawer.y + pad;
    const int titleRightReserve = 112;
    drawWrappedText(info.overlayTitle.empty() ? "Panel" : info.overlayTitle,
                    SDL_Rect{drawer.x + pad, y, drawer.w - pad * 2 - titleRightReserve, 40},
                    fontMedium,
                    theme.textPrimary,
                    2,
                    1);
    y += 44;
    drawLine(drawer.x + pad, y, drawer.x + drawer.w - pad, y, color(theme.accent.r, theme.accent.g, theme.accent.b, 80), 1);
    y += 18;

    const int contentW = drawer.w - pad * 2;
    const bool hasBottomActions = std::any_of(buttons.begin(), buttons.end(), [](const UIButton& button) {
        return button.id.rfind("overlay_", 0) == 0 && button.id != "overlay_close";
    });
    const int actionReserve = hasBottomActions ? 92 : 0;
    const int contentBottom = drawer.y + drawer.h - pad - actionReserve;

    if (info.overlayPage == OverlayPage::ImportExport) {
        drawText("Puzzle String Input", drawer.x + pad, y, fontSmall, theme.accent);
        y += 25;

        const int inputH = std::clamp(drawer.h / 5, 118, 160);
        SDL_Rect inputRect{drawer.x + pad, y, contentW, inputH};
        fillRect(inputRect, color(4, 10, 11, 238));
        strokeRect(inputRect,
                   info.overlayInputActive ? theme.warning : color(theme.accent.r, theme.accent.g, theme.accent.b, 120),
                   info.overlayInputActive ? 2 : 1);

        std::ostringstream count;
        count << info.overlayInputText.size() << " / 81";
        drawText(count.str(), inputRect.x + inputRect.w - 72, inputRect.y + 8, fontSmall, theme.textSecondary);

        const std::string display = groupedPuzzleText(info.overlayInputText);
        drawWrappedText(display,
                        SDL_Rect{inputRect.x + 12, inputRect.y + 34, inputRect.w - 24, inputRect.h - 44},
                        fontSmall,
                        info.overlayInputText.empty() ? theme.textMuted : theme.textPrimary,
                        3,
                        6);
        y += inputRect.h + 16;
    }

    for (const std::string& line : info.overlayLines) {
        if (y > contentBottom - 20) {
            drawText("...", drawer.x + pad, y, fontSmall, theme.textMuted);
            break;
        }
        if (line.empty()) {
            y += 10;
            continue;
        }

        const bool heading = line.find(':') == std::string::npos && line.size() < 24;
        if (heading) {
            y += 6;
            drawText(line, drawer.x + pad, y, fontSmall, theme.accent);
            y += 25;
            continue;
        }

        SDL_Rect card{drawer.x + pad, y, contentW, 48};
        fillRect(card, color(14, 24, 25, 150));
        strokeRect(card, color(71, 85, 105, 88), 1);
        const int usedY = drawWrappedText(line,
                                          SDL_Rect{card.x + 12, card.y + 9, card.w - 24, card.h - 12},
                                          fontSmall,
                                          theme.textSecondary,
                                          2,
                                          2);
        y += std::max(48, usedY - card.y + 12) + 8;
    }

    if (hasBottomActions) {
        const int actionY = drawer.y + drawer.h - pad - 82;
        drawLine(drawer.x + pad,
                 actionY - 12,
                 drawer.x + drawer.w - pad,
                 actionY - 12,
                 color(theme.accent.r, theme.accent.g, theme.accent.b, 58),
                 1);
        drawText("Actions", drawer.x + pad, actionY - 34, fontSmall, theme.textSecondary);
    }
}

void Renderer::drawCandidates(const Board& board,
                              int row,
                              int col,
                              const SolveStep* current,
                              const RenderInfo& info) {
    const int candidates = board.getCandidates(row, col);
    const SDL_Rect rect = cellRect(row, col);
    const int slot = std::max(8, layout.cellSize / 3);
    const bool visible = shouldShowCandidates(row, col, current, info);
    const bool removalCell = current
        && (current->type == StepType::RemoveCandidate || current->type == StepType::CandidateRemovedByLogic)
        && current->row == row
        && current->col == col;

    if (!visible && !removalCell) {
        return;
    }

    TTF_Font* candidateFont = layout.cellSize < 48 ? fontTiny : fontCandidate;
    for (int n = 1; n <= 9; ++n) {
        const int bit = Board::bitForNumber(n);
        const bool isCandidate = (candidates & bit) != 0;
        const bool removedNow = removalCell
            && ((current->number == n) || ((current->removedMask & bit) != 0));

        if (!isCandidate && !removedNow) {
            continue;
        }
        if (!visible && !removedNow) {
            continue;
        }

        const int sr = (n - 1) / 3;
        const int sc = (n - 1) % 3;
        SDL_Rect slotRect{rect.x + sc * slot + std::max(1, layout.cellSize / 34),
                          rect.y + sr * slot + std::max(1, layout.cellSize / 34),
                          slot - std::max(2, layout.cellSize / 18),
                          slot - std::max(2, layout.cellSize / 18)};

        SDL_Color candidateColor = removedNow ? appTheme().danger : color(148, 163, 184, 180);
        if (sameCell(current, row, col) && current->type == StepType::AnalyzeCell) {
            candidateColor = appTheme().accent;
        }
        if (info.candidateMode == CandidateDisplayMode::All && !sameCell(current, row, col)) {
            candidateColor.a = static_cast<Uint8>(std::min<int>(candidateColor.a, 128));
        }

        drawCenteredText(std::to_string(n), slotRect, candidateFont, candidateColor);
        if (removedNow) {
            drawLine(slotRect.x + 4,
                     slotRect.y + slotRect.h / 2,
                     slotRect.x + slotRect.w - 4,
                     slotRect.y + slotRect.h / 2,
                     appTheme().danger,
                     2);
        }
    }
}

bool Renderer::shouldShowCandidates(int row, int col, const SolveStep* current, const RenderInfo& info) const {
    if (info.candidateMode == CandidateDisplayMode::All) {
        return true;
    }
    if (info.candidateMode == CandidateDisplayMode::Off) {
        return false;
    }
    if (Board::isInside(info.selectedRow, info.selectedCol)
        && info.selectedRow == row
        && info.selectedCol == col) {
        return true;
    }
    if (!current) {
        return false;
    }
    if (sameCell(current, row, col)
        || (current->relatedRow == row && current->relatedCol == col)
        || (current->row2 == row && current->col2 == col)) {
        return true;
    }
    if (hasPrimaryCell(current)) {
        return current->row == row || current->col == col || sameBox(current->row, current->col, row, col);
    }
    if (current->unitType == static_cast<int>(UnitType::Row) && current->unitIndex == row) {
        return true;
    }
    if (current->unitType == static_cast<int>(UnitType::Column) && current->unitIndex == col) {
        return true;
    }
    if (current->unitType == static_cast<int>(UnitType::Box) && current->unitIndex == Board::boxId(row, col)) {
        return true;
    }
    return false;
}

void Renderer::drawButtons(const std::vector<UIButton>& buttons) {
    const Theme& theme = appTheme();
    for (const UIButton& button : buttons) {
        SDL_Rect rect = button.rect;
        if (button.pressed) {
            rect.y += 1;
        }

        SDL_Color bg = theme.buttonSurface;
        SDL_Color border = color(theme.accent.r, theme.accent.g, theme.accent.b, 150);
        SDL_Color text = theme.textPrimary;
        if (!button.enabled) {
            bg = theme.buttonDisabled;
            border = color(51, 65, 85, 150);
            text = theme.textMuted;
        } else if (button.pressed) {
            bg = theme.buttonPressed;
            border = theme.accent;
        } else if (button.hovered) {
            bg = theme.buttonHover;
            border = theme.warning;
        }

        fillRect(rect, bg);
        strokeRect(rect, border, button.hovered && button.enabled ? 2 : 1);
        drawCenteredText(button.label, rect, rect.h <= 30 ? fontSmall : fontBody, text);
    }
}

void Renderer::drawLabelValue(const std::string& label,
                              const std::string& value,
                              const SDL_Rect& rect,
                              SDL_Color valueColor) {
    const int labelW = std::clamp(rect.w / 3, 82, 118);
    drawText(label, rect.x, rect.y + 2, fontSmall, appTheme().textSecondary);
    drawWrappedText(value,
                    SDL_Rect{rect.x + labelW, rect.y, rect.w - labelW, rect.h},
                    fontBody,
                    valueColor,
                    2,
                    1);
}

void Renderer::drawText(const std::string& text, int x, int y, TTF_Font* font, SDL_Color textColor, int wrap) {
    if (text.empty() || !font) {
        return;
    }
    SDL_Surface* surface = wrap > 0
        ? TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), textColor, static_cast<Uint32>(wrap))
        : TTF_RenderUTF8_Blended(font, text.c_str(), textColor);
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    const SDL_Rect dest{x, y, surface->w, surface->h};
    SDL_RenderCopy(sdlRenderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

int Renderer::drawWrappedText(const std::string& text,
                              const SDL_Rect& rect,
                              TTF_Font* font,
                              SDL_Color textColor,
                              int lineSpacing,
                              int maxLines) {
    if (text.empty() || !font || rect.w <= 0 || rect.h <= 0) {
        return rect.y;
    }

    std::vector<std::string> words;
    std::stringstream input(text);
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    if (words.empty()) {
        return rect.y;
    }

    std::vector<std::string> lines;
    std::string line;
    for (const std::string& next : words) {
        const std::string attempt = line.empty() ? next : line + " " + next;
        int width = 0;
        int height = 0;
        TTF_SizeUTF8(font, attempt.c_str(), &width, &height);
        if (width <= rect.w || line.empty()) {
            line = attempt;
        } else {
            lines.push_back(line);
            line = next;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }

    const int lineHeight = TTF_FontHeight(font);
    const int fitByHeight = std::max(1, (rect.h + lineSpacing) / std::max(1, lineHeight + lineSpacing));
    const int allowedLines = maxLines > 0 ? std::min(maxLines, fitByHeight) : fitByHeight;
    int y = rect.y;
    for (int i = 0; i < allowedLines && i < static_cast<int>(lines.size()); ++i) {
        std::string display = lines[static_cast<size_t>(i)];
        if (i == allowedLines - 1 && i + 1 < static_cast<int>(lines.size())) {
            display += "...";
        }
        int width = 0;
        int height = 0;
        TTF_SizeUTF8(font, display.c_str(), &width, &height);
        while (width > rect.w && display.size() > 4) {
            display.erase(display.end() - 4, display.end() - 3);
            TTF_SizeUTF8(font, display.c_str(), &width, &height);
        }
        drawText(display, rect.x, y, font, textColor);
        y += lineHeight + lineSpacing;
    }
    return y;
}

void Renderer::drawCenteredText(const std::string& text, const SDL_Rect& rect, TTF_Font* font, SDL_Color textColor) {
    if (text.empty() || !font || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    std::string fitted = text;
    int width = 0;
    int height = 0;
    TTF_SizeUTF8(font, fitted.c_str(), &width, &height);
    while (width > rect.w - 6 && fitted.size() > 3) {
        fitted.pop_back();
        std::string probe = fitted + ".";
        TTF_SizeUTF8(font, probe.c_str(), &width, &height);
        if (width <= rect.w - 6) {
            fitted = probe;
            break;
        }
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, fitted.c_str(), textColor);
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dest{rect.x + (rect.w - surface->w) / 2,
                  rect.y + (rect.h - surface->h) / 2,
                  surface->w,
                  surface->h};
    SDL_RenderCopy(sdlRenderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void Renderer::fillRect(const SDL_Rect& rect, SDL_Color fillColor) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    SDL_SetRenderDrawColor(sdlRenderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    SDL_RenderFillRect(sdlRenderer, &rect);
}

void Renderer::strokeRect(const SDL_Rect& rect, SDL_Color strokeColor, int thickness) {
    SDL_SetRenderDrawColor(sdlRenderer, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect r{rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2};
        if (r.w <= 0 || r.h <= 0) {
            break;
        }
        SDL_RenderDrawRect(sdlRenderer, &r);
    }
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, SDL_Color lineColor, int thickness) {
    SDL_SetRenderDrawColor(sdlRenderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
    const bool mostlyHorizontal = std::abs(x2 - x1) >= std::abs(y2 - y1);
    for (int i = 0; i < thickness; ++i) {
        const int offset = i - thickness / 2;
        if (mostlyHorizontal) {
            SDL_RenderDrawLine(sdlRenderer, x1, y1 + offset, x2, y2 + offset);
        } else {
            SDL_RenderDrawLine(sdlRenderer, x1 + offset, y1, x2 + offset, y2);
        }
    }
}

SDL_Color Renderer::accentForStep(StepType type) const {
    const Theme& theme = appTheme();
    switch (type) {
    case StepType::AnalyzeCell:
        return theme.accent;
    case StepType::RemoveCandidate:
    case StepType::CandidateRemovedByLogic:
        return theme.danger;
    case StepType::NakedSingle:
        return theme.success;
    case StepType::HiddenSingle:
        return theme.logic;
    case StepType::LockedCandidate:
        return color(45, 212, 191);
    case StepType::BoxLineReduction:
        return color(56, 189, 248);
    case StepType::NakedPair:
        return color(163, 230, 53);
    case StepType::HiddenPair:
        return color(34, 211, 238);
    case StepType::XWing:
        return color(250, 204, 21);
    case StepType::ModeChanged:
        return theme.textSecondary;
    case StepType::Guess:
        return theme.guess;
    case StepType::PlaceNumber:
        return color(251, 146, 60);
    case StepType::Contradiction:
    case StepType::InvalidInput:
    case StepType::NoSolution:
        return theme.danger;
    case StepType::Backtrack:
        return theme.backtrack;
    case StepType::Solved:
        return theme.success;
    case StepType::MultipleSolutions:
        return theme.guess;
    case StepType::TurboSolved:
        return color(129, 140, 248);
    }
    return theme.accent;
}

std::string Renderer::describeStep(const SolveStep& step, int index, int total) const {
    std::ostringstream out;
    out << "Step " << (index + 1) << "/" << total << ": " << stepTypeName(step.type);
    if (Board::isInside(step.row, step.col)) {
        out << " at r" << (step.row + 1) << "c" << (step.col + 1);
    }
    if (step.number > 0) {
        out << " = " << step.number;
    }
    return out.str();
}

std::string Renderer::stepTypeName(StepType type) const {
    switch (type) {
    case StepType::AnalyzeCell:
        return "Analyze Cell";
    case StepType::RemoveCandidate:
        return "Remove Candidate";
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
    case StepType::CandidateRemovedByLogic:
        return "Candidate Removed";
    case StepType::ModeChanged:
        return "Mode Changed";
    case StepType::Guess:
        return "Guess";
    case StepType::PlaceNumber:
        return "Place Number";
    case StepType::Contradiction:
        return "Contradiction";
    case StepType::Backtrack:
        return "Backtrack";
    case StepType::Solved:
        return "Solved";
    case StepType::NoSolution:
        return "No Solution";
    case StepType::MultipleSolutions:
        return "Multiple Solutions";
    case StepType::InvalidInput:
        return "Invalid Input";
    case StepType::TurboSolved:
        return "Turbo Solved";
    }
    return "Step";
}

bool Renderer::openFonts() {
    const std::array<std::string, 5> normalPaths = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "D:/MSYS2/ucrt64/share/fonts/TTF/DejaVuSans.ttf"
    };
    const std::array<std::string, 4> boldPaths = {
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/tahomabd.ttf",
        "C:/Windows/Fonts/calibrib.ttf"
    };
    const std::array<std::string, 3> monoPaths = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "D:/MSYS2/ucrt64/share/fonts/TTF/DejaVuSansMono.ttf"
    };

    auto findFont = [](const auto& paths) {
        for (const std::string& path : paths) {
            TTF_Font* probe = TTF_OpenFont(path.c_str(), 16);
            if (probe) {
                TTF_CloseFont(probe);
                return path;
            }
        }
        return std::string{};
    };

    const std::string normal = findFont(normalPaths);
    const std::string bold = findFont(boldPaths).empty() ? normal : findFont(boldPaths);
    const std::string monoProbe = findFont(monoPaths);
    const std::string mono = monoProbe.empty() ? normal : monoProbe;
    if (normal.empty()) {
        logRuntime("No usable font found in configured font paths.");
        return false;
    }

    logRuntime(std::string("Using UI font: ") + normal);
    fontTiny = TTF_OpenFont(normal.c_str(), 10);
    fontSmall = TTF_OpenFont(normal.c_str(), 14);
    fontCandidate = TTF_OpenFont(mono.c_str(), 15);
    fontBody = TTF_OpenFont(normal.c_str(), 18);
    fontMedium = TTF_OpenFont(bold.empty() ? normal.c_str() : bold.c_str(), 25);
    fontNumber = TTF_OpenFont(mono.c_str(), 38);
    fontNumberLarge = TTF_OpenFont(mono.c_str(), 50);

    const bool ok = fontTiny && fontSmall && fontCandidate && fontBody && fontMedium && fontNumber && fontNumberLarge;
    if (!ok) {
        logRuntime(std::string("Failed to open one or more font sizes: ") + TTF_GetError());
    }
    return ok;
}

void Renderer::closeFonts() {
    if (fontSmall) {
        TTF_CloseFont(fontSmall);
        fontSmall = nullptr;
    }
    if (fontTiny) {
        TTF_CloseFont(fontTiny);
        fontTiny = nullptr;
    }
    if (fontCandidate) {
        TTF_CloseFont(fontCandidate);
        fontCandidate = nullptr;
    }
    if (fontBody) {
        TTF_CloseFont(fontBody);
        fontBody = nullptr;
    }
    if (fontMedium) {
        TTF_CloseFont(fontMedium);
        fontMedium = nullptr;
    }
    if (fontNumber) {
        TTF_CloseFont(fontNumber);
        fontNumber = nullptr;
    }
    if (fontNumberLarge) {
        TTF_CloseFont(fontNumberLarge);
        fontNumberLarge = nullptr;
    }
}
