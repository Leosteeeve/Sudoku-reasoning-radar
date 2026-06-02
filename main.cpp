#define SDL_MAIN_HANDLED

#include "src/App.h"

#include <fstream>
#include <iostream>

namespace {
constexpr const char* RuntimeLogPath = "D:/Soduku/sudoku_runtime.log";
}

int main(int, char**) {
    SDL_SetMainReady();

    {
        std::ofstream log(RuntimeLogPath, std::ios::trunc);
        log << "Starting SudokuSolver.exe\n";
    }

    App app;
    if (!app.initialize()) {
        const char* message = "Failed to initialize SDL2/SDL2_ttf application.";
        std::cerr << message << "\n";
        std::ofstream log(RuntimeLogPath, std::ios::app);
        log << message << "\n";
        return 1;
    }

    {
        std::ofstream log(RuntimeLogPath, std::ios::app);
        log << "Initialization succeeded; entering app loop.\n";
    }

    app.run();
    app.shutdown();

    {
        std::ofstream log(RuntimeLogPath, std::ios::app);
        log << "Application exited normally.\n";
    }
    return 0;
}
