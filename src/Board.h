#pragma once

#include <array>
#include <string>

class Board {
public:
    static constexpr int Size = 9;
    static constexpr int BoxSize = 3;
    static constexpr int AllMask = 0x1FF;

    int grid[Size][Size];
    int rowMask[Size];
    int colMask[Size];
    int boxMask[Size];
    int candidateMask[Size][Size];
    bool fixed[Size][Size];

    Board();

    void clear();
    bool load(const int puzzle[Size][Size], bool markGivens = true);
    bool load(const std::array<std::array<int, Size>, Size>& puzzle, bool markGivens = true);

    bool setCellValue(int row, int col, int number, bool markFixed = false);
    bool placeNumber(int row, int col, int number, bool markFixed = false);
    void removeNumber(int row, int col, bool clearFixed = true);

    int getCell(int row, int col) const;
    bool isFixed(int row, int col) const;
    int getUsedMask(int row, int col) const;
    int getBaseCandidates(int row, int col) const;
    int getCandidates(int row, int col) const;
    int getCandidateCount(int row, int col) const;
    void initializeCandidates();
    int removeCandidates(int row, int col, int mask);
    bool setCandidateMask(int row, int col, int mask);
    int emptyCount() const;

    bool isSolved() const;
    bool hasContradiction(std::string* reason = nullptr, int* row = nullptr, int* col = nullptr) const;
    bool validateInitial(std::string* reason = nullptr,
                         int* row = nullptr,
                         int* col = nullptr,
                         int* relatedRow = nullptr,
                         int* relatedCol = nullptr) const;

    static bool isInside(int row, int col);
    static int boxId(int row, int col);
    static int bitForNumber(int number);
    static int numberFromBit(int bit);
    static int popCount(int mask);
    static bool isSingle(int mask);

private:
    void rebuildMasks();
    void rebuildMasksAndCandidates();
    void updateCandidatesAfterPlacement(int row, int col, int number);
};
