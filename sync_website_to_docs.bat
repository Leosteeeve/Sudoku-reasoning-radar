@echo off
setlocal

cd /d "%~dp0"

echo Syncing website/ to docs/...

if not exist "website\index.html" (
    echo ERROR: website\index.html was not found.
    exit /b 1
)

if not exist "docs" mkdir "docs"
if not exist "docs\assets" mkdir "docs\assets"
if not exist "docs\downloads" mkdir "docs\downloads"

copy /Y "website\index.html" "docs\index.html" >nul
copy /Y "website\style.css" "docs\style.css" >nul
copy /Y "website\script.js" "docs\script.js" >nul
copy /Y "website\assets\README_ASSETS.txt" "docs\assets\README_ASSETS.txt" >nul
copy /Y "website\downloads\README_DOWNLOADS.txt" "docs\downloads\README_DOWNLOADS.txt" >nul

if exist "website\assets\screenshot.png" (
    copy /Y "website\assets\screenshot.png" "docs\assets\screenshot.png" >nul
)

if exist "website\downloads\SudokuReasoningRadar_Windows.zip" (
    copy /Y "website\downloads\SudokuReasoningRadar_Windows.zip" "docs\downloads\SudokuReasoningRadar_Windows.zip" >nul
)

echo Done. docs/ is ready for GitHub Pages.
endlocal
