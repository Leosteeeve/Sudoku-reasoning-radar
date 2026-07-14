#include "PuzzleString.h"

#include <array>
#include <cctype>

bool PuzzleString::parse(const std::string& text, Board& board, std::string* error) {
    std::string compact;
    compact.reserve(81);
    for (const unsigned char ch : text) {
        if (!std::isspace(ch)) {
            compact.push_back(static_cast<char>(ch));
        }
    }

    if (compact.size() != 81) {
        if (error) {
            *error = "Expected 81 characters using digits, 0, or dots.";
        }
        return false;
    }

    std::array<std::array<int, Board::Size>, Board::Size> values{};
    for (int index = 0; index < 81; ++index) {
        const char ch = compact[static_cast<std::size_t>(index)];
        int value = 0;
        if (ch >= '1' && ch <= '9') {
            value = ch - '0';
        } else if (ch != '0' && ch != '.') {
            if (error) {
                *error = "Expected 81 characters using digits, 0, or dots.";
            }
            return false;
        }
        values[static_cast<std::size_t>(index / Board::Size)]
              [static_cast<std::size_t>(index % Board::Size)] = value;
    }

    board.load(values, true);
    if (error) {
        error->clear();
    }
    return true;
}

std::string PuzzleString::serialize(const Board& board) {
    std::string text;
    text.reserve(81);
    for (int row = 0; row < Board::Size; ++row) {
        for (int column = 0; column < Board::Size; ++column) {
            const int value = board.getCell(row, column);
            text.push_back(value == 0 ? '0' : static_cast<char>('0' + value));
        }
    }
    return text;
}
