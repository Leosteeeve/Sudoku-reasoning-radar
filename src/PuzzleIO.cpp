#include "PuzzleIO.h"

#include <SDL.h>

#include <array>
#include <cctype>
#include <sstream>

bool PuzzleIO::parsePuzzleString(const std::string& text, Board& board, std::string* error) {
    const std::string compact = compactPuzzleText(text);
    if (compact.size() != 81) {
        if (error) {
            *error = "Invalid puzzle string. Expected 81 characters using digits, 0, or dots.";
        }
        return false;
    }

    std::array<std::array<int, Board::Size>, Board::Size> values{};
    for (int i = 0; i < 81; ++i) {
        const char ch = compact[static_cast<size_t>(i)];
        if (ch >= '1' && ch <= '9') {
            values[static_cast<size_t>(i / 9)][static_cast<size_t>(i % 9)] = ch - '0';
        } else if (ch == '0' || ch == '.') {
            values[static_cast<size_t>(i / 9)][static_cast<size_t>(i % 9)] = 0;
        } else {
            if (error) {
                *error = "Invalid puzzle string. Expected 81 characters using digits, 0, or dots.";
            }
            return false;
        }
    }

    board.load(values, true);
    std::string validationReason;
    if (!board.validateInitial(&validationReason)) {
        if (error) {
            *error = validationReason;
        }
        return false;
    }
    return true;
}

std::string PuzzleIO::boardToString(const Board& board, bool dotsForEmpty) {
    std::string out;
    out.reserve(81);
    for (int r = 0; r < Board::Size; ++r) {
        for (int c = 0; c < Board::Size; ++c) {
            const int value = board.getCell(r, c);
            out.push_back(value == 0 ? (dotsForEmpty ? '.' : '0') : static_cast<char>('0' + value));
        }
    }
    return out;
}

bool PuzzleIO::importFromClipboard(Board& board, std::string* status) {
    if (!SDL_HasClipboardText()) {
        if (status) {
            *status = "Clipboard does not contain puzzle text.";
        }
        return false;
    }

    char* raw = SDL_GetClipboardText();
    if (!raw) {
        if (status) {
            *status = "Failed to read clipboard text.";
        }
        return false;
    }
    const std::string text(raw);
    SDL_free(raw);

    std::string error;
    if (!parsePuzzleString(text, board, &error)) {
        if (status) {
            *status = error;
        }
        return false;
    }
    if (status) {
        *status = "Imported puzzle string from clipboard.";
    }
    return true;
}

bool PuzzleIO::copyPuzzleToClipboard(const Board& board, std::string* status) {
    const std::string text = boardToString(board);
    if (SDL_SetClipboardText(text.c_str()) != 0) {
        if (status) {
            *status = std::string("Failed to copy puzzle string: ") + SDL_GetError();
        }
        return false;
    }
    if (status) {
        *status = "Copied puzzle string to clipboard.";
    }
    return true;
}

bool PuzzleIO::copySolutionToClipboard(const Board& board, std::string* status) {
    if (!board.isSolved()) {
        if (status) {
            *status = "No solved board is available to copy.";
        }
        return false;
    }
    const std::string text = boardToString(board);
    if (SDL_SetClipboardText(text.c_str()) != 0) {
        if (status) {
            *status = std::string("Failed to copy solution string: ") + SDL_GetError();
        }
        return false;
    }
    if (status) {
        *status = "Copied solution string to clipboard.";
    }
    return true;
}

std::string PuzzleIO::compactPuzzleText(const std::string& text) {
    std::string compact;
    compact.reserve(81);
    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            continue;
        }
        compact.push_back(static_cast<char>(ch));
    }
    return compact;
}
