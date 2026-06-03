# Sudoku Reasoning Radar

Version: v0.2.0

A Windows C++17 visual Sudoku solver built with SDL2 and SDL2_ttf. It supports editable 9x9 input, persistent pencil-mark candidates, animated solving steps, human-style logic, MRV search, a Turbo exact-cover solver, puzzle generation, hints, difficulty analysis, import/export, mistake detection, and a local puzzle library.

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

## v0.2.0 Features

- Puzzle Generator: press `G` to generate a unique puzzle; press `Shift+G` or `Diff` to cycle Easy / Medium / Hard / Expert generation targets.
- Hint Coach: `F1` gives a gentle location hint, `F2` names the technique, `F3` gives a direct move, and `Enter` applies the current direct move if available.
- Difficulty Analyzer: after solving or generating, the app reports grade, score, hardest technique, step stats, givens, guesses, and backtracks.
- Import / Export: 81-character puzzle strings use digits `1`-`9` for givens and `0` or `.` for empty cells.
- Mistake Detection: `K` cycles Off / RuleCheck / SolutionCheck. RuleCheck highlights duplicate row/column/box values; SolutionCheck compares player entries with a cached unique solution.
- Local Puzzle Library: `Ctrl+S` saves the current puzzle to `data/puzzles.txt`; `L` opens the library view; Up/Down selects entries; Enter loads the selected puzzle.

## v0.2.0 UI Declutter Update

- Command Deck replaces the old button wall. The main panel now shows one selected action, a short description, previous/next controls, and one Execute button.
- Overlay pages keep low-frequency information out of the main panel: Settings, Analytics, Library, Import/Export, Shortcuts, Generator, and About.
- Analytics moved into a dedicated drawer with grade, score, technique stats, branch stats, and summary.
- Settings drawer groups visual settings, solver mode, mistake mode, controls, and version/about information.
- Library drawer lists saved puzzles and keeps the main board focused.
- Shortcuts drawer replaces the old crowded bottom help text.
- Bottom status strip now shows one short message instead of another dense progress/control block.

## Local Puzzle Library Format

The local library is a simple text file:

```text
data/puzzles.txt
```

Each puzzle is stored as:

```text
name|difficulty|puzzleString|solutionString|createdAt|seed
```

No SQLite or external database is required.

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
- `Tab` / `Shift+Tab`: cycle the Command Deck action.
- `Enter`: execute the current Command Deck action, or load selected library puzzle when Library drawer is open.
- Left / Right: step playback when a trace is active; otherwise cycle Command Deck action.
- `M` or `Mode`: cycle Human Logic / Smart / Turbo.
- `T` or `Turbo`: switch to Turbo and solve.
- `H` or `Cand ...`: cycle candidate display through Off / Focused / All.
- `A` or `P`: auto playback pause/resume.
- `S` or Right arrow: step forward.
- `B` or Left arrow: step backward.
- `+`, `=`, or keypad `+`: speed up animation.
- `-`, `_`, or keypad `-`: slow animation.
- `F11`, `F`, or `Full`: toggle fullscreen/windowed mode.
- `G` or `Gen`: generate a new puzzle.
- `Shift+G`: open the Generator drawer.
- `F1` or `Hint`: gentle hint.
- `F2` or `Explain`: technique hint.
- `F3`: direct hint.
- `Enter` or `Apply`: apply the current hint, or load the selected library puzzle if the library is open.
- `Import / Export`: open the puzzle string drawer with an in-app text box, paste button, import button, and copy buttons.
- `Ctrl+I`: import an 81-character puzzle string directly from clipboard.
- `Ctrl+E` or `Export`: copy current puzzle string.
- `Ctrl+Shift+E`: copy current solution string.
- `K` or `Mistake`: cycle mistake detection mode.
- `Ctrl+S` or `Save`: save current puzzle to local library.
- `L` or `[LIB]`: open/close the Library drawer.
- `Up` / `Down`: select a library entry when library view is open.
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
if not exist build_tmp mkdir build_tmp
set PATH=D:\MSYS2\ucrt64\bin;D:\MSYS2\usr\bin;%PATH%
set TEMP=D:\Soduku\build_tmp
set TMP=D:\Soduku\build_tmp
set TMPDIR=D:\Soduku\build_tmp
D:/MSYS2/ucrt64/bin/g++.exe -std=c++17 -O2 -Wall -Wextra main.cpp src/Board.cpp src/Solver.cpp src/DLXSolver.cpp src/StepRecorder.cpp src/CommandDeck.cpp src/OverlayPages.cpp src/DifficultyAnalyzer.cpp src/PuzzleIO.cpp src/HintCoach.cpp src/PuzzleGenerator.cpp src/PuzzleLibrary.cpp src/Layout.cpp src/Animation.cpp src/Renderer.cpp src/App.cpp -o SudokuSolver.exe -ID:/MSYS2/ucrt64/include/SDL2 -LD:/MSYS2/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
```

`PATH` is set so `g++` can launch its own UCRT64 helper tools. `TEMP`, `TMP`, and `TMPDIR` are set to `build_tmp` because some MSYS2 tools can fail to create temporary object files when the Windows user profile path contains non-ASCII characters.

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

## Website And Publishing

This project now includes a static GitHub Pages-ready website.

- Website source: `website/`
- GitHub Pages folder: `docs/`
- Website preview script: `preview_website.bat`
- Website sync script: `sync_website_to_docs.bat`
- Packaging script: `package_windows.bat`
- Windows release package guide: `PACKAGE_WINDOWS.md`
- GitHub Pages deployment guide: `DEPLOY_GITHUB_PAGES.md`

Local preview:

```bat
preview_website.bat
```

GitHub Pages recommendation:

1. Edit the source website in `website/`.
2. Run `sync_website_to_docs.bat`.
3. Commit both `website/` and `docs/`.
4. In GitHub Pages settings, choose `main` branch and `/docs` folder.

The website now points the Windows download button at the latest GitHub Release asset:

```text
https://github.com/Leosteeeve/Sudoku-reasoning-radar/releases/latest/download/SudokuReasoningRadar_Windows.zip
```

The GitHub repository link placeholder is still:

- GitHub URL: `#github-link-placeholder`

Replace the placeholder after the repository URL is final. Do not commit passwords, tokens, cookies, API keys, or private personal files.

## Display

The SDL2 window is resizable and uses a real responsive layout instead of fixed logical scaling. The board, right-side panel, controls, and bottom timeline are recalculated from the renderer output size each frame. Fullscreen uses SDL's desktop fullscreen mode, which preserves the desktop display mode while expanding the app to the screen.

The visual style is a dark "reasoning radar" dashboard: subdued blueprint lines, neon technique accents, Focused candidate marks, animated scan highlights, a progress timeline, and separated controls so text and buttons do not overlap.

The main interface now uses progressive disclosure. The board and current reasoning focus stay visible, while analytics, settings, library, import/export, and shortcut details open as drawer pages.

## Current Limits

- v0.2.0 generator uses randomized solved boards plus uniqueness-preserving removal; difficulty targets are practical heuristics, not a full graded generator yet.
- Hint Coach currently prioritizes the first human-logic placement found by Human Logic Mode.
- Puzzle library is a simple local text file and does not include search/filter UI yet.
- X-Wing is implemented for row-based and column-based rectangles, but more exotic fish patterns are not included.
- Turbo mode focuses on speed and uniqueness detection, not detailed human-style animation.
- Visual animations are SDL2-based and intentionally lightweight.
- Human Logic Mode can stop on puzzles that require techniques beyond the implemented set.

## Roadmap

- v0.3.0: WebAssembly browser version, Daily Challenge, advanced generator tuning, richer Hint Coach explanations, puzzle library filters, and deeper difficulty history.
