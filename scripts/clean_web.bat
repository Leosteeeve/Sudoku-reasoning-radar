@echo off
setlocal EnableExtensions

cd /d "%~dp0\.."

if exist "web_build" (
    rmdir /S /Q "web_build"
)

del /Q "docs\play\SudokuReasoningRadar.js" 2>nul
del /Q "docs\play\SudokuReasoningRadar.wasm" 2>nul
del /Q "docs\play\SudokuReasoningRadar.data" 2>nul

echo Web build artifacts cleaned.

endlocal
