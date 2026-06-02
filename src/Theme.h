#pragma once

#include <SDL.h>

struct Theme {
    SDL_Color backgroundTop{5, 8, 10, 255};
    SDL_Color backgroundBottom{12, 18, 18, 255};
    SDL_Color gridBlueprint{57, 93, 84, 42};
    SDL_Color boardGlow{45, 212, 191, 30};
    SDL_Color boardSurface{9, 16, 17, 238};
    SDL_Color cellSurface{12, 19, 20, 255};
    SDL_Color givenSurface{27, 35, 33, 255};
    SDL_Color panelSurface{12, 18, 19, 236};
    SDL_Color panelBorder{80, 98, 91, 170};
    SDL_Color gridThin{96, 120, 111, 112};
    SDL_Color gridThick{210, 235, 222, 220};
    SDL_Color textPrimary{229, 235, 228, 255};
    SDL_Color textSecondary{148, 165, 156, 255};
    SDL_Color textMuted{104, 119, 113, 255};
    SDL_Color accent{56, 189, 248, 255};
    SDL_Color accentSoft{56, 189, 248, 60};
    SDL_Color logic{45, 212, 191, 255};
    SDL_Color success{74, 222, 128, 255};
    SDL_Color warning{251, 191, 36, 255};
    SDL_Color danger{248, 113, 113, 255};
    SDL_Color guess{192, 132, 252, 255};
    SDL_Color backtrack{148, 163, 184, 255};
    SDL_Color buttonSurface{25, 36, 35, 255};
    SDL_Color buttonHover{37, 55, 51, 255};
    SDL_Color buttonPressed{14, 116, 144, 255};
    SDL_Color buttonDisabled{15, 23, 42, 255};
};

inline const Theme& appTheme() {
    static const Theme theme;
    return theme;
}
