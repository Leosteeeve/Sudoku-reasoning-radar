@echo off
setlocal

cd /d "%~dp0"

if exist "website\index.html" (
    echo Opening website\index.html...
    start "" "%CD%\website\index.html"
) else if exist "docs\index.html" (
    echo Opening docs\index.html...
    start "" "%CD%\docs\index.html"
) else (
    echo ERROR: No website index file found.
    exit /b 1
)

endlocal
