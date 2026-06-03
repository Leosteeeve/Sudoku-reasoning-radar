@echo off
setlocal EnableExtensions

cd /d "%~dp0\.."

if not exist "docs\play" mkdir "docs\play"
copy /Y "web\index.html" "docs\play\index.html" >nul
copy /Y "web\web_style.css" "docs\play\web_style.css" >nul
copy /Y "web\web_launcher.js" "docs\play\web_launcher.js" >nul

where em++ >nul 2>nul
if errorlevel 1 (
    echo Emscripten not found.
    echo Run:
    echo D:\emsdk\emsdk_env.bat
    exit /b 1
)

if not exist "web_build" mkdir "web_build"
if not exist "web_build\tmp" mkdir "web_build\tmp"

set TMP=%CD%\web_build\tmp
set TEMP=%CD%\web_build\tmp

set OUT_JS=web_build\SudokuReasoningRadar.js
set OUT_WASM=web_build\SudokuReasoningRadar.wasm

del /Q "%OUT_JS%" "%OUT_WASM%" "web_build\SudokuReasoningRadar.data" 2>nul

echo Building Sudoku Reasoning Radar WebAssembly preview...

call em++ ^
  -std=c++17 ^
  -O2 ^
  -Wall ^
  -Wextra ^
  -DWEB_BUILD=1 ^
  -I. ^
  -Isrc ^
  web_main.cpp ^
  src\Board.cpp ^
  src\StepRecorder.cpp ^
  src\CommandDeck.cpp ^
  src\OverlayPages.cpp ^
  src\Solver.cpp ^
  src\DLXSolver.cpp ^
  src\PuzzleGenerator.cpp ^
  src\HintCoach.cpp ^
  src\DifficultyAnalyzer.cpp ^
  -sUSE_SDL=2 ^
  -sALLOW_MEMORY_GROWTH=1 ^
  -sINITIAL_MEMORY=134217728 ^
  -sEXIT_RUNTIME=0 ^
  -sENVIRONMENT=web ^
  -sASSERTIONS=1 ^
  -sEXPORTED_RUNTIME_METHODS=ccall ^
  -sEXPORTED_FUNCTIONS=_main,_SRR_OnCanvasResize ^
  -o "%OUT_JS%"

set BUILD_EXIT=%ERRORLEVEL%

if not exist "%OUT_JS%" (
    echo Web build failed: %OUT_JS% was not created.
    echo em++ exit code: %BUILD_EXIT%
    exit /b 1
)

if not exist "%OUT_WASM%" (
    echo Web build failed: %OUT_WASM% was not created.
    echo em++ exit code: %BUILD_EXIT%
    exit /b 1
)

if not "%BUILD_EXIT%"=="0" (
    echo Warning: em++ returned exit code %BUILD_EXIT%, but output files were created.
    echo Publishing the fresh WebAssembly files anyway.
)

copy /Y "%OUT_JS%" "docs\play\SudokuReasoningRadar.js" >nul
if errorlevel 1 (
    echo Failed to copy %OUT_JS% to docs\play.
    exit /b 1
)

copy /Y "%OUT_WASM%" "docs\play\SudokuReasoningRadar.wasm" >nul
if errorlevel 1 (
    echo Failed to copy %OUT_WASM% to docs\play.
    exit /b 1
)

if exist "web_build\SudokuReasoningRadar.data" copy /Y "web_build\SudokuReasoningRadar.data" "docs\play\SudokuReasoningRadar.data" >nul

echo Web build complete.
echo Output:
echo   docs\play\index.html
echo   docs\play\SudokuReasoningRadar.js
echo   docs\play\SudokuReasoningRadar.wasm

endlocal
