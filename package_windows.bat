@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"

set "VERSION=v0.2.1"
set "APP_EXE=SudokuSolver.exe"
set "APP_NAME=SudokuReasoningRadar_Windows"
set "PROJECT_ROOT=%CD%"
set "MSYS2_UCRT_BIN=D:\MSYS2\ucrt64\bin"
set "PYTHON=%MSYS2_UCRT_BIN%\python.exe"
set "NTLDD=%MSYS2_UCRT_BIN%\ntldd.exe"
set "TESSDATA_SRC=D:\MSYS2\ucrt64\share\tessdata\eng.traineddata"
set "RELEASE_ROOT=release"
set "RELEASE_DIR=%RELEASE_ROOT%\SudokuReasoningRadar_%VERSION%_Windows"
set "ZIP_PATH=%RELEASE_ROOT%\%APP_NAME%.zip"
set "WEBSITE_ZIP=website\downloads\%APP_NAME%.zip"
set "DOCS_ZIP=docs\downloads\%APP_NAME%.zip"
set "COPIED_DLL_COUNT=unknown"
set "MISSING_DLL_COUNT=unknown"
set "COLLECT_WARNING=0"

echo Packaging Sudoku Reasoning Radar %VERSION% for Windows...

if not exist "%PYTHON%" (
    echo ERROR: Missing MSYS2 UCRT64 Python:
    echo   %PYTHON%
    echo Install or repair MSYS2 UCRT64 before packaging.
    exit /b 1
)

if not exist "%NTLDD%" (
    echo Missing ntldd.
    echo Install it in MSYS2 UCRT64:
    echo   pacman -S --needed mingw-w64-ucrt-x86_64-ntldd
    exit /b 1
)

if not exist "%APP_EXE%" (
    echo %APP_EXE% was not found. Running build.bat first...
    if exist "build.bat" (
        call build.bat
        if errorlevel 1 (
            echo ERROR: Build failed. Packaging stopped.
            exit /b 1
        )
    ) else (
        echo ERROR: build.bat was not found.
        exit /b 1
    )
)

if not exist "%APP_EXE%" (
    echo ERROR: %APP_EXE% still does not exist after build.
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

if exist "assets" (
    echo Copying assets folder...
    xcopy /E /I /Y "assets" "%RELEASE_DIR%\assets" >nul
)

if exist "data" (
    echo Copying data folder...
    xcopy /E /I /Y "data" "%RELEASE_DIR%\data" >nul
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

if exist "%TESSDATA_SRC%" (
    echo Copying Tesseract English tessdata...
    if not exist "%RELEASE_DIR%\tessdata" mkdir "%RELEASE_DIR%\tessdata"
    copy /Y "%TESSDATA_SRC%" "%RELEASE_DIR%\tessdata\eng.traineddata" >nul
) else (
    echo WARNING: %TESSDATA_SRC% was not found. OCR Import needs English tessdata.
)

echo Creating release README...
(
    echo Sudoku Reasoning Radar - Windows Release
    echo Version %VERSION%
    echo.
    echo Run:
    echo   SudokuSolver.exe
    echo.
    echo Keep all DLL files and the tessdata folder next to SudokuSolver.exe.
    echo OCR Import requires:
    echo   tessdata\eng.traineddata
    echo.
    echo OCR works best with clear screenshots or straight photos of printed Sudoku grids.
    echo Handwritten Sudoku support is experimental.
    echo.
    echo If the app does not start, check:
    echo   dependency_report.txt
    echo   dependency_missing.txt
) > "%RELEASE_DIR%\README_RELEASE.txt"

echo Collecting MSYS2 UCRT64 DLL dependencies with ntldd...
"%PYTHON%" "tools\collect_msys2_dlls.py" ^
    --target "%RELEASE_DIR%\%APP_EXE%" ^
    --output "%RELEASE_DIR%" ^
    --msys-bin "%MSYS2_UCRT_BIN%" ^
    --ntldd "%NTLDD%"
set "COLLECT_EXIT=%ERRORLEVEL%"
if "%COLLECT_EXIT%"=="2" (
    echo ERROR: Dependency collection could not run.
    exit /b 1
)
if not "%COLLECT_EXIT%"=="0" (
    set "COLLECT_WARNING=1"
    echo WARNING: Dependency collection reported missing DLLs. See %RELEASE_DIR%\dependency_missing.txt.
)

if exist "%RELEASE_DIR%\dependency_report.txt" (
    for /f "tokens=2 delims=:" %%A in ('findstr /B /C:"copied DLL count:" "%RELEASE_DIR%\dependency_report.txt"') do set "COPIED_DLL_COUNT=%%A"
    for /f "tokens=2 delims=:" %%A in ('findstr /B /C:"missing DLL count:" "%RELEASE_DIR%\dependency_report.txt"') do set "MISSING_DLL_COUNT=%%A"
    set "COPIED_DLL_COUNT=!COPIED_DLL_COUNT: =!"
    set "MISSING_DLL_COUNT=!MISSING_DLL_COUNT: =!"
)

echo Creating ZIP package...
if exist "%ZIP_PATH%" del /q "%ZIP_PATH%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force -CompressionLevel Fastest"
if errorlevel 1 (
    echo ERROR: Automatic ZIP creation failed.
    exit /b 1
)

if not exist "%ZIP_PATH%" (
    echo ERROR: %ZIP_PATH% was not created.
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
echo Package complete.
echo Release folder: %PROJECT_ROOT%\%RELEASE_DIR%
echo ZIP: %PROJECT_ROOT%\%ZIP_PATH%
echo Copied DLL count: %COPIED_DLL_COUNT%
echo Missing DLL count: %MISSING_DLL_COUNT%

if "%COLLECT_WARNING%"=="1" (
    exit /b 1
)

exit /b 0
