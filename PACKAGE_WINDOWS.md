# Windows Release Packaging

This guide explains how to create a downloadable Windows ZIP for Sudoku Reasoning Radar v0.2.1.

## Safety

- Do not include passwords, tokens, cookies, SSH keys, API keys, or private personal files.
- Review the release folder before publishing or uploading it.
- If you use GitHub Releases, upload only the final ZIP file you intend to share.

## Required Packaging Tool

The release package uses automatic MSYS2 DLL dependency collection. Install `ntldd` in MSYS2 UCRT64:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-ntldd
```

The script expects:

```text
D:\MSYS2\ucrt64\bin\ntldd.exe
D:\MSYS2\ucrt64\bin\python.exe
```

If `ntldd.exe` is missing, `package_windows.bat` stops with the install command above.

## Automatic Packaging

Run:

```bat
package_windows.bat
```

The script will:

1. Create `release/SudokuReasoningRadar_v0.2.1_Windows/`.
2. Build first if `SudokuSolver.exe` is missing.
3. Copy `SudokuSolver.exe`.
4. Copy optional `assets/`, `data/`, `fonts/`, and `LICENSE`.
5. Copy `D:\MSYS2\ucrt64\share\tessdata\eng.traineddata` into `tessdata/`.
6. Run `tools/collect_msys2_dlls.py` with `ntldd.exe`.
7. Recursively copy only required MSYS2 UCRT64 DLLs into the release folder.
8. Write `dependency_report.txt` and `dependency_missing.txt`.
9. Create `release/SudokuReasoningRadar_Windows.zip`.
10. Copy the ZIP to:
    - `website/downloads/SudokuReasoningRadar_Windows.zip`
    - `docs/downloads/SudokuReasoningRadar_Windows.zip`

The ZIP filename intentionally stays stable because GitHub Pages can point to the latest release asset:

```text
SudokuReasoningRadar_Windows.zip
```

## Dependency Reports

`dependency_report.txt` includes:

- scan root executable
- MSYS2 bin path
- copied DLL count
- copied DLL list
- skipped Windows system DLL list
- missing DLL list
- scanned executable/DLL list
- timestamp

`dependency_missing.txt` is empty when no non-system DLLs are missing. If it contains entries, do not publish that ZIP until the missing dependencies are understood.

## Release Self-Test

Run:

```bat
test_release.bat
```

The test enters the release folder and sets `PATH` to only:

```text
release folder
%SystemRoot%\System32
%SystemRoot%
%SystemRoot%\System32\Wbem
```

It intentionally does not include `D:\MSYS2\ucrt64\bin`, so it better simulates a normal user machine.

## OCR Notes

OCR Import requires:

```text
tessdata\eng.traineddata
```

The app first checks for `tessdata/eng.traineddata` next to `SudokuSolver.exe`, then falls back to the development MSYS2 tessdata folder and `TESSDATA_PREFIX`.

If OCR initialization fails, the UI should show:

```text
Tesseract OCR initialization failed. Make sure tessdata/eng.traineddata exists next to the executable.
```

## Manual Recovery

If ZIP creation fails but the release folder is valid:

1. Open `release/SudokuReasoningRadar_v0.2.1_Windows/`.
2. Confirm `SudokuSolver.exe`, DLLs, `tessdata/eng.traineddata`, and dependency reports are present.
3. Create a ZIP named:

```text
SudokuReasoningRadar_Windows.zip
```

4. Upload it to GitHub Releases or copy it to the local website download folders.

## Do Not

- Do not copy the entire `D:\MSYS2\ucrt64\bin` folder.
- Do not require users to install MSYS2.
- Do not require users to set `PATH`.
- Do not delete OCR to make packaging easier.
- Do not replace the GitHub Pages latest-release download link unless the repository URL changes.
