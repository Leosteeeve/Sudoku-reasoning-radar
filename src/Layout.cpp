#include "Layout.h"

#include <algorithm>

LayoutState computeLayout(int width, int height) {
    LayoutState l;
    l.width = std::max(width, 640);
    l.height = std::max(height, 520);
    l.safe = std::clamp(l.width / 36, 24, 52);
    l.gap = std::clamp(l.width / 48, 18, 34);
    const int timelineHeight = std::clamp(l.height / 16, 42, 64);
    const int usableHeight = l.height - l.safe * 2 - timelineHeight - l.gap;
    const int minPanel = 360;
    const int maxPanel = std::clamp(l.width / 3, 400, 520);
    const int sideBySideBoard = std::min({usableHeight, l.width - l.safe * 2 - minPanel - l.gap, 780});

    l.compact = sideBySideBoard < 420;
    if (!l.compact) {
        l.cellSize = std::max(44, sideBySideBoard / 9);
        const int boardSize = l.cellSize * 9;
        const int panelWidth = std::clamp(l.width - l.safe * 2 - boardSize - l.gap, minPanel, maxPanel);
        const int contentWidth = boardSize + l.gap + panelWidth;
        const int startX = std::max(l.safe, (l.width - contentWidth) / 2);
        const int top = std::max(l.safe, (l.height - timelineHeight - l.gap - boardSize) / 2);
        l.boardRect = SDL_Rect{startX, top, boardSize, boardSize};
        l.panelRect = SDL_Rect{startX + boardSize + l.gap, top, panelWidth, boardSize};
        l.timelineRect = SDL_Rect{startX, top + boardSize + l.gap / 2, contentWidth, timelineHeight};
    } else {
        const int boardLimit = std::min(l.width - l.safe * 2, usableHeight - 180);
        l.cellSize = std::max(36, std::min(boardLimit, 560) / 9);
        const int boardSize = l.cellSize * 9;
        l.boardRect = SDL_Rect{(l.width - boardSize) / 2, l.safe, boardSize, boardSize};
        const int panelTop = l.boardRect.y + l.boardRect.h + l.gap;
        const int panelHeight = std::max(180, l.height - panelTop - timelineHeight - l.safe);
        l.panelRect = SDL_Rect{l.safe, panelTop, l.width - l.safe * 2, panelHeight};
        l.timelineRect = SDL_Rect{l.safe, l.height - l.safe - timelineHeight, l.width - l.safe * 2, timelineHeight};
    }

    const int pad = std::clamp(l.panelRect.w / 24, 14, 22);
    int y = l.panelRect.y + pad;
    const int innerX = l.panelRect.x + pad;
    const int innerW = l.panelRect.w - pad * 2;
    auto block = [&](int h) {
        SDL_Rect r{innerX, y, innerW, h};
        y += h + 12;
        return r;
    };
    l.headerBlock = block(44);
    l.statusBlock = block(82);
    l.puzzleBlock = block(44);
    l.selectedBlock = block(62);
    const int controlsH = l.compact ? 112 : 190;
    const int progressH = 46;
    const int remaining = std::max(96, l.panelRect.y + l.panelRect.h - pad - y - controlsH - progressH - 24);
    l.stepBlock = block(remaining);
    l.progressBlock = block(progressH);
    l.controlsBlock = SDL_Rect{innerX, l.panelRect.y + l.panelRect.h - pad - controlsH, innerW, controlsH};
    l.controlsRect = l.controlsBlock;
    return l;
}
