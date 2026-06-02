# Sudoku Reasoning Radar

A Windows C++17 visual Sudoku solver built with SDL2 and SDL2_ttf. It supports editable 9x9 input, persistent pencil-mark candidates, animated solving steps, human-style logic, MRV search, and a Turbo exact-cover solver.

## Prompt 2 Features

- Three solver modes: Human Logic, Smart Solver, and Turbo Exact Cover.
- Persistent `candidateMask[9][9]` pencil marks instead of temporary per-frame candidates.
- Advanced human logic steps: Locked Candidate, Box-Line Reduction, Naked Pair, Hidden Pair, and row/column X-Wing.
- Step playback includes candidate removal, placement, guess, contradiction, backtrack, mode, and Turbo result steps.
- Right panel shows mode, status, puzzle difficulty, selected cell, candidates, solving time, speed, timeline, technique, reason, and controls.
- Built-in Easy, Medium, Hard, Expert, Invalid, and Multiple-solution puzzles.

## Prompt 3 UI Upgrade

- Responsive SDL2 layout: the board, right panel, buttons, and bottom step timeline are recomputed from the current window size.
- Fullscreen/windowed mode uses desktop fullscreen and keeps the board and panel readable.
- Candidate display has three modes: Off, Focused, and All. Focused is the default so an empty board no longer floods every cell with 1-9.
- Dark "reasoning radar" styling with blueprint grid, scan sweep, highlighted row/column/box bands, step timeline, hover/pressed buttons, and stronger visual accents for logic/guess/backtrack/contradiction.
- Text wrapping in the right panel prevents step reasons, status text, and controls from overlapping.

## Solver Modes

- Human Logic Mode: uses logical techniques only and never guesses. If logic cannot continue, the run stops with `Human logic stopped: no available logical move found.`
- Smart Solver Mode: default mode. It applies all logic techniques first, then uses MRV backtracking when logic stalls. It detects invalid input, no solution, unique solution, and multiple solutions.
- Turbo Exact Cover Mode: uses an Algorithm X style exact-cover solver over 729 possible placements and 324 Sudoku constraints. It focuses on speed and uniqueness detection rather than detailed human-style animation.

## Algorithm Notes

The board keeps fast bitmasks:

- `rowMask[9]`, `colMask[9]`, `boxMask[9]`
- `0x1FF` represents digits 1-9.
- `boxId = (r / 3) * 3 + (c / 3)`.
- `candidateMask[9][9]` stores persistent pencil marks.

Logic techniques:

- Naked Single: a cell has one remaining candidate.
- Hidden Single: a digit has one legal cell inside a row, column, or box.
- Locked Candidate / Pointing: candidates for a digit in a box are confined to one row or column.
- Box-Line Reduction / Claiming: candidates in a row or column are confined to one box.
- Naked Pair: two cells in a unit share the same two candidates, so those candidates are removed elsewhere in the unit.
- Hidden Pair: two digits appear only in the same two cells, so those cells keep only those two digits.
- X-Wing: row-based and column-based rectangles remove a digit from intersecting units.

Turbo exact cover constraints:

- Each cell receives exactly one digit.
- Each row contains each digit once.
- Each column contains each digit once.
- Each 3x3 box contains each digit once.

## Controls

- Mouse click: select a cell.
- `1`-`9`: enter a digit.
- `Backspace`, `Delete`, or `0`: clear selected cell.
- `Space` or `Solve`: solve using current mode.
- `M` or `Mode`: cycle Human Logic / Smart / Turbo.
- `T` or `Turbo`: switch to Turbo and solve.
- `H` or `Cand ...`: cycle candidate display through Off / Focused / All.
- `A` or `P`: auto playback pause/resume.
- `S` or Right arrow: step forward.
- `B` or Left arrow: step backward.
- `+`, `=`, or keypad `+`: speed up animation.
- `-`, `_`, or keypad `-`: slow animation.
- `F11`, `F`, or `Full`: toggle fullscreen/windowed mode.
- `R` or `Reset`: restore the current puzzle/input snapshot.
- `C` or `Clear`: clear the board.
- `N` or `Puzzle`: cycle built-in puzzles.
- `Esc`: quit.

## Built-In Puzzles

- Easy - Singles Practice
- Medium - Logic Techniques
- Hard - MRV Search
- Expert - Turbo Friendly
- Invalid - Duplicate In Row
- Multiple - Two Rows Open

## Build

Install dependencies in MSYS2 UCRT64 if needed:

```sh
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
```

Manual build command:

```sh
D:
cd \Soduku
set TEMP=D:\Soduku
set TMP=D:\Soduku
set TMPDIR=D:\Soduku
D:/MSYS2/ucrt64/bin/g++.exe -std=c++17 -O2 -Wall -Wextra main.cpp src/Board.cpp src/Solver.cpp src/DLXSolver.cpp src/StepRecorder.cpp src/Layout.cpp src/Animation.cpp src/Renderer.cpp src/App.cpp -o SudokuSolver.exe -ID:/MSYS2/ucrt64/include/SDL2 -LD:/MSYS2/ucrt64/lib -lSDL2 -lSDL2_ttf
```

`TEMP`, `TMP`, and `TMPDIR` are set to the project folder because some MSYS2 tools can fail to create temporary object files when the Windows user profile path contains non-ASCII characters.

## Run

From `D:\Soduku`:

```sh
set PATH=D:\MSYS2\ucrt64\bin;D:\MSYS2\usr\bin;%PATH%
SudokuSolver.exe
```

In VS Code, open this folder and press Ctrl+F5. The launch configuration builds first and runs:

```text
D:\Soduku\SudokuSolver.exe
```

The launch environment prepends MSYS2 UCRT64 paths so SDL2 DLLs can be found.

## Display

The SDL2 window is resizable and uses a real responsive layout instead of fixed logical scaling. The board, right-side panel, controls, and bottom timeline are recalculated from the renderer output size each frame. Fullscreen uses SDL's desktop fullscreen mode, which preserves the desktop display mode while expanding the app to the screen.

The visual style is a dark "reasoning radar" dashboard: subdued blueprint lines, neon technique accents, Focused candidate marks, animated scan highlights, a progress timeline, and separated controls so text and buttons do not overlap.

## Current Limits

- X-Wing is implemented for row-based and column-based rectangles, but more exotic fish patterns are not included.
- Turbo mode focuses on speed and uniqueness detection, not detailed human-style animation.
- Visual animations are SDL2-based and intentionally lightweight.
- Human Logic Mode can stop on puzzles that require techniques beyond the implemented set.
