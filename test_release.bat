@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "VERSION=v0.2.1"
set "RELEASE_DIR=release\SudokuReasoningRadar_%VERSION%_Windows"
set "APP_EXE=SudokuSolver.exe"

if not exist "%RELEASE_DIR%\%APP_EXE%" (
    echo ERROR: Release executable was not found:
    echo   %RELEASE_DIR%\%APP_EXE%
    echo Run package_windows.bat first.
    exit /b 1
)

pushd "%RELEASE_DIR%"

echo Testing release without MSYS2 PATH...
echo Release folder: %CD%
echo.

set "PATH=%CD%;%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem"
"%APP_EXE%"
set "APP_EXIT=%ERRORLEVEL%"

popd
exit /b %APP_EXIT%
