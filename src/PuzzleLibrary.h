#pragma once

#include "Board.h"

#include <string>
#include <vector>

struct LibraryEntry {
    std::string name;
    std::string difficulty;
    std::string puzzleString;
    std::string solutionString;
    std::string createdAt;
    std::string seed;
};

class PuzzleLibrary {
public:
    explicit PuzzleLibrary(std::string path = "data/puzzles.txt");

    bool load(std::string* status = nullptr);
    bool saveEntry(const LibraryEntry& entry, std::string* status = nullptr);
    const std::vector<LibraryEntry>& entries() const;
    const LibraryEntry* entryAt(int index) const;
    int count() const;
    const std::string& filePath() const;

    static LibraryEntry makeEntry(const std::string& name,
                                  const std::string& difficulty,
                                  const Board& puzzle,
                                  const Board* solution,
                                  const std::string& seed);

private:
    std::string path;
    std::vector<LibraryEntry> items;
};
