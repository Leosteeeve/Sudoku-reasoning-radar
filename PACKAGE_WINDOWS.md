# Windows Release Packaging

This guide explains how to create a downloadable Windows ZIP for Sudoku Reasoning Radar.

## Safety

- Do not include passwords, tokens, cookies, SSH keys, API keys, or private personal files.
- Review the release folder before publishing or uploading it.
- If you use GitHub Releases, upload only the final ZIP file you intend to share.

## Expected Release Contents

The Windows release package should include:

- `SudokuSolver.exe`
- `SDL2.dll`
- `SDL2_ttf.dll`
- `libfreetype-6.dll`
- `libharfbuzz-0.dll`
- other UCRT64 runtime DLLs required by SDL2_ttf
- `README_RELEASE.txt`
- `LICENSE.txt` if available
- `data/` folder if the local puzzle library should be included
- Optional `assets/` folder if future versions need runtime assets
- Optional `fonts/` folder if future versions bundle fonts

## Build First

Build the C++ project first from VS Code with Ctrl+F5, or run the manual command from `README.md`.

The expected executable is:

```text
D:\Soduku\SudokuSolver.exe
```

## Automatic Packaging

Run:

```bat
package_windows.bat
```

The script will:

1. Create `release/SudokuReasoningRadar_v0.2.0_Windows/`.
2. Copy `SudokuSolver.exe`.
3. Try to copy `SDL2.dll`, `SDL2_ttf.dll`, `libfreetype-6.dll`, and the related UCRT64 runtime DLLs from `D:\MSYS2\ucrt64\bin`.
4. Copy `assets/` or `fonts/` if those folders exist.
5. Create `README_RELEASE.txt`.
6. Compress the release folder into `release/SudokuReasoningRadar_Windows.zip`.
7. Copy the ZIP to:
   - `website/downloads/SudokuReasoningRadar_Windows.zip`
   - `docs/downloads/SudokuReasoningRadar_Windows.zip`

If the script cannot find the executable or DLLs, it prints a clear warning or error.

## Manual Packaging

If automatic ZIP creation fails:

1. Open `release/SudokuReasoningRadar_v0.2.0_Windows/`.
2. Confirm the executable and DLLs are inside.
3. Right-click the folder and choose Send to > Compressed zipped folder.
4. Rename the ZIP to:

```text
SudokuReasoningRadar_Windows.zip
```

5. Copy it to:

```text
website\downloads\
docs\downloads\
```

## GitHub Release Option

For the public website, the recommended approach is GitHub Releases. Keep the asset filename stable:

```text
SudokuReasoningRadar_Windows.zip
```

The website can then use:

```text
https://github.com/Leosteeeve/Sudoku-reasoning-radar/releases/latest/download/SudokuReasoningRadar_Windows.zip
```

If you do not want to commit the ZIP into the repository, upload it to a GitHub Release instead. Then replace the download link in:

- `website/index.html`
- `docs/index.html`

Use the release asset URL GitHub gives you.
