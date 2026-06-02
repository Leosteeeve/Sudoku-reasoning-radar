@echo off
setlocal

cd /d "%~dp0"

set "APP_EXE=SudokuSolver.exe"
set "APP_NAME=SudokuReasoningRadar_Windows"
set "RELEASE_ROOT=release"
set "RELEASE_DIR=%RELEASE_ROOT%\%APP_NAME%"
set "ZIP_PATH=%RELEASE_ROOT%\%APP_NAME%.zip"
set "MSYS2_BIN=D:\MSYS2\ucrt64\bin"
set "WEBSITE_ZIP=website\downloads\%APP_NAME%.zip"
set "DOCS_ZIP=docs\downloads\%APP_NAME%.zip"

echo Packaging Sudoku Reasoning Radar for Windows...

if not exist "%APP_EXE%" (
    echo ERROR: %APP_EXE% was not found.
    echo Build the C++ project first with VS Code Ctrl+F5 or the README build command.
    exit /b 1
)

if not exist "%RELEASE_ROOT%" mkdir "%RELEASE_ROOT%"
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

echo Copying executable...
copy /Y "%APP_EXE%" "%RELEASE_DIR%\%APP_EXE%" >nul
if errorlevel 1 (
    echo ERROR: Failed to copy %APP_EXE%.
    exit /b 1
)

echo Copying SDL2 runtime DLLs...
if exist "%MSYS2_BIN%\SDL2.dll" (
    copy /Y "%MSYS2_BIN%\SDL2.dll" "%RELEASE_DIR%\SDL2.dll" >nul
) else (
    echo WARNING: %MSYS2_BIN%\SDL2.dll was not found.
)

if exist "%MSYS2_BIN%\SDL2_ttf.dll" (
    copy /Y "%MSYS2_BIN%\SDL2_ttf.dll" "%RELEASE_DIR%\SDL2_ttf.dll" >nul
) else (
    echo WARNING: %MSYS2_BIN%\SDL2_ttf.dll was not found.
)

if exist "assets" (
    echo Copying assets folder...
    xcopy /E /I /Y "assets" "%RELEASE_DIR%\assets" >nul
)

if exist "fonts" (
    echo Copying fonts folder...
    xcopy /E /I /Y "fonts" "%RELEASE_DIR%\fonts" >nul
)

if exist "LICENSE.txt" (
    copy /Y "LICENSE.txt" "%RELEASE_DIR%\LICENSE.txt" >nul
) else if exist "LICENSE" (
    copy /Y "LICENSE" "%RELEASE_DIR%\LICENSE.txt" >nul
) else (
    echo No LICENSE file found. Add one before public release if needed.
)

echo Creating release README...
(
    echo Sudoku Reasoning Radar - Windows Release
    echo.
    echo Run:
    echo   SudokuSolver.exe
    echo.
    echo Included files should include:
    echo   SudokuSolver.exe
    echo   SDL2.dll
    echo   SDL2_ttf.dll
    echo.
    echo If the app does not start, make sure the SDL2 runtime DLLs are next to the executable.
    echo.
    echo Project website files can be found in website/ and docs/.
) > "%RELEASE_DIR%\README_RELEASE.txt"

echo Creating ZIP package...
if exist "%ZIP_PATH%" del /q "%ZIP_PATH%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"

if not exist "%ZIP_PATH%" (
    echo WARNING: Automatic ZIP creation failed.
    echo Please manually zip this folder:
    echo   %RELEASE_DIR%
    exit /b 1
)

if not exist "website\downloads" mkdir "website\downloads"
if not exist "docs\downloads" mkdir "docs\downloads"

copy /Y "%ZIP_PATH%" "%WEBSITE_ZIP%" >nul
if errorlevel 1 (
    echo WARNING: Failed to copy ZIP to %WEBSITE_ZIP%.
) else (
    echo Copied ZIP to %WEBSITE_ZIP%.
)

copy /Y "%ZIP_PATH%" "%DOCS_ZIP%" >nul
if errorlevel 1 (
    echo WARNING: Failed to copy ZIP to %DOCS_ZIP%.
) else (
    echo Copied ZIP to %DOCS_ZIP%.
)

echo.
echo Done:
echo   %ZIP_PATH%
echo   %WEBSITE_ZIP%
echo   %DOCS_ZIP%

endlocal
