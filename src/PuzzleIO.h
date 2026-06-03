#pragma once

#include "Board.h"

#include <string>

class PuzzleIO {
public:
    static bool parsePuzzleString(const std::string& text,
                                  Board& board,
                                  std::string* error = nullptr);
    static std::string boardToString(const Board& board, bool dotsForEmpty = false);

    static bool importFromClipboard(Board& board, std::string* status = nullptr);
    static bool copyPuzzleToClipboard(const Board& board, std::string* status = nullptr);
    static bool copySolutionToClipboard(const Board& board, std::string* status = nullptr);

private:
    static std::string compactPuzzleText(const std::string& text);
};
