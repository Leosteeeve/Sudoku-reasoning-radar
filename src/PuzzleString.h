#pragma once

#include "Board.h"

#include <string>

class PuzzleString {
public:
    static bool parse(const std::string& text,
                      Board& board,
                      std::string* error = nullptr);
    static std::string serialize(const Board& board);
};
