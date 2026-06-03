#include "PuzzleLibrary.h"

#include "PuzzleIO.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace {
std::string nowStamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
    return buffer;
}
}

PuzzleLibrary::PuzzleLibrary(std::string pathValue)
    : path(std::move(pathValue)) {
}

bool PuzzleLibrary::load(std::string* status) {
    items.clear();
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ifstream input(path);
    if (!input) {
        if (status) {
            *status = "Puzzle library is empty.";
        }
        return true;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, '|')) {
            parts.push_back(part);
        }
        if (parts.size() < 6) {
            continue;
        }
        items.push_back(LibraryEntry{parts[0], parts[1], parts[2], parts[3], parts[4], parts[5]});
    }
    if (status) {
        *status = "Loaded " + std::to_string(items.size()) + " library puzzle(s).";
    }
    return true;
}

bool PuzzleLibrary::saveEntry(const LibraryEntry& entry, std::string* status) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream output(path, std::ios::app);
    if (!output) {
        if (status) {
            *status = "Failed to open puzzle library for writing.";
        }
        return false;
    }
    output << entry.name << '|'
           << entry.difficulty << '|'
           << entry.puzzleString << '|'
           << entry.solutionString << '|'
           << entry.createdAt << '|'
           << entry.seed << '\n';
    if (!output) {
        if (status) {
            *status = "Failed to write puzzle library entry.";
        }
        return false;
    }
    items.push_back(entry);
    if (status) {
        *status = "Saved puzzle to local library.";
    }
    return true;
}

const std::vector<LibraryEntry>& PuzzleLibrary::entries() const {
    return items;
}

const LibraryEntry* PuzzleLibrary::entryAt(int index) const {
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return nullptr;
    }
    return &items[static_cast<size_t>(index)];
}

int PuzzleLibrary::count() const {
    return static_cast<int>(items.size());
}

const std::string& PuzzleLibrary::filePath() const {
    return path;
}

LibraryEntry PuzzleLibrary::makeEntry(const std::string& name,
                                      const std::string& difficulty,
                                      const Board& puzzle,
                                      const Board* solution,
                                      const std::string& seed) {
    LibraryEntry entry;
    entry.name = name;
    entry.difficulty = difficulty;
    entry.puzzleString = PuzzleIO::boardToString(puzzle);
    entry.solutionString = solution && solution->isSolved() ? PuzzleIO::boardToString(*solution) : "";
    entry.createdAt = nowStamp();
    entry.seed = seed;
    return entry;
}
