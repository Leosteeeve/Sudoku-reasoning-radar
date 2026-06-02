#pragma once

#include <SDL.h>

struct LayoutState {
    int width = 1280;
    int height = 860;
    bool compact = false;
    int safe = 36;
    int gap = 28;
    int cellSize = 72;
    SDL_Rect boardRect{44, 64, 648, 648};
    SDL_Rect panelRect{760, 64, 420, 648};
    SDL_Rect timelineRect{44, 748, 1180, 44};
    SDL_Rect controlsRect{782, 526, 386, 194};
    SDL_Rect headerBlock{};
    SDL_Rect statusBlock{};
    SDL_Rect puzzleBlock{};
    SDL_Rect selectedBlock{};
    SDL_Rect stepBlock{};
    SDL_Rect progressBlock{};
    SDL_Rect controlsBlock{};
};

LayoutState computeLayout(int width, int height);
