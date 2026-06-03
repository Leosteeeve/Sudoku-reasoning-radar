@echo off
setlocal EnableExtensions

cd /d "%~dp0\.."

if not exist "docs\play\index.html" (
    echo docs\play\index.html was not found.
    echo Run scripts\build_web.bat first.
    exit /b 1
)

if not exist "docs\play\SudokuReasoningRadar.js" (
    if exist "web_build\SudokuReasoningRadar.js" (
        echo Publishing SudokuReasoningRadar.js from web_build to docs\play...
        copy /Y "web_build\SudokuReasoningRadar.js" "docs\play\SudokuReasoningRadar.js" >nul
    )
)

if not exist "docs\play\SudokuReasoningRadar.wasm" (
    if exist "web_build\SudokuReasoningRadar.wasm" (
        echo Publishing SudokuReasoningRadar.wasm from web_build to docs\play...
        copy /Y "web_build\SudokuReasoningRadar.wasm" "docs\play\SudokuReasoningRadar.wasm" >nul
    )
)

if not exist "docs\play\SudokuReasoningRadar.js" (
    echo docs\play\SudokuReasoningRadar.js was not found.
    echo Run:
    echo D:\emsdk\emsdk_env.bat
    echo scripts\build_web.bat
    exit /b 1
)

if not exist "docs\play\SudokuReasoningRadar.wasm" (
    echo docs\play\SudokuReasoningRadar.wasm was not found.
    echo Run:
    echo D:\emsdk\emsdk_env.bat
    echo scripts\build_web.bat
    exit /b 1
)

set PORT=8000
if not "%~1"=="" set PORT=%~1

if exist "D:\MSYS2\ucrt64\bin\python.exe" (
    set PYTHON_EXE=D:\MSYS2\ucrt64\bin\python.exe
) else (
    set PYTHON_EXE=python
)

"%PYTHON_EXE%" "scripts\serve_web.py" --port %PORT% --directory "docs\play"

endlocal
