$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$MsysBin = "D:\MSYS2\ucrt64\bin"
$MsysUsrBin = "D:\MSYS2\usr\bin"
$Gxx = Join-Path $MsysBin "g++.exe"
$PkgConfig = Join-Path $MsysBin "pkg-config.exe"
$BuildTmp = Join-Path $Root "build_tmp"
$ObjDir = Join-Path $BuildTmp "obj"
$Exe = Join-Path $Root "SudokuSolver.exe"
$BuildLog = Join-Path $BuildTmp "build.log"

New-Item -ItemType Directory -Force -Path $BuildTmp | Out-Null
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

function Write-BuildLog([string]$Message) {
    $line = "[{0}] {1}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Message
    Add-Content -Path $BuildLog -Value $line
}

$env:PATH = "$MsysBin;$MsysUsrBin;$env:PATH"
$env:TEMP = $BuildTmp
$env:TMP = $BuildTmp
$env:TMPDIR = $BuildTmp

function Show-MissingDeps {
    Write-Host "Missing OCR dependencies."
    Write-Host "Install with:"
    Write-Host "pacman -S --needed mingw-w64-ucrt-x86_64-opencv mingw-w64-ucrt-x86_64-tesseract-ocr mingw-w64-ucrt-x86_64-tesseract-data-eng mingw-w64-ucrt-x86_64-pkgconf"
}

function Split-Flags([string]$Flags) {
    if ([string]::IsNullOrWhiteSpace($Flags)) {
        return @()
    }
    return @($Flags -split "\s+" | Where-Object { $_ -ne "" })
}

if (-not (Test-Path $Gxx)) {
    Write-Host "Missing compiler: $Gxx"
    exit 1
}

if (-not (Test-Path $PkgConfig)) {
    Show-MissingDeps
    exit 1
}

& $PkgConfig --exists opencv4 tesseract
if ($LASTEXITCODE -ne 0) {
    Show-MissingDeps
    exit 1
}

$CFlags = Split-Flags (& $PkgConfig --cflags opencv4 tesseract)
if ($LASTEXITCODE -ne 0) {
    Show-MissingDeps
    exit 1
}

$CommonFlags = @(
    "-std=c++17",
    "-O2",
    "-Wall",
    "-Wextra",
    "-ID:/MSYS2/ucrt64/include/SDL2"
) + $CFlags

$Sources = @(
    "main.cpp",
    "src/Board.cpp",
    "src/Solver.cpp",
    "src/DLXSolver.cpp",
    "src/StepRecorder.cpp",
    "src/CommandDeck.cpp",
    "src/OverlayPages.cpp",
    "src/DifficultyAnalyzer.cpp",
    "src/PuzzleIO.cpp",
    "src/HintCoach.cpp",
    "src/PuzzleGenerator.cpp",
    "src/PuzzleLibrary.cpp",
    "src/OCRReviewState.cpp",
    "src/ImagePreprocessor.cpp",
    "src/DigitRecognizer.cpp",
    "src/OCRImport.cpp",
    "src/Layout.cpp",
    "src/Animation.cpp",
    "src/Renderer.cpp",
    "src/App.cpp"
)

$Objects = @()
$Compiled = 0
$Started = Get-Date
Write-Host "Building Sudoku Reasoning Radar v0.2.1..."
Set-Content -Path $BuildLog -Value ("[{0}] Build started" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))

foreach ($Source in $Sources) {
    $SourcePath = Join-Path $Root $Source
    if (-not (Test-Path $SourcePath)) {
        Write-Host "Missing source file: $Source"
        exit 1
    }

    $ObjectName = ($Source -replace "[\\/]", "_") -replace "\.cpp$", ".o"
    $ObjectPath = Join-Path $ObjDir $ObjectName
    $DepPath = [System.IO.Path]::ChangeExtension($ObjectPath, ".d")
    $Objects += $ObjectPath

    $NeedsCompile = -not (Test-Path $ObjectPath)
    if (-not $NeedsCompile) {
        $NeedsCompile = (Get-Item $SourcePath).LastWriteTimeUtc -gt (Get-Item $ObjectPath).LastWriteTimeUtc
    }

    if ($NeedsCompile) {
        Write-Host "  CXX $Source"
        Write-BuildLog "CXX $Source"
        $CompileArgs = $CommonFlags + @("-MMD", "-MP", "-c", $Source, "-o", $ObjectPath)
        & $Gxx @CompileArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Build failed while compiling $Source."
            exit $LASTEXITCODE
        }
        $Compiled++
    }
}

$NeedsLink = -not (Test-Path $Exe)
if (-not $NeedsLink) {
    $ExeTime = (Get-Item $Exe).LastWriteTimeUtc
    foreach ($Object in $Objects) {
        if ((Get-Item $Object).LastWriteTimeUtc -gt $ExeTime) {
            $NeedsLink = $true
            break
        }
    }
    if ((Get-Item (Join-Path $Root "build.ps1")).LastWriteTimeUtc -gt $ExeTime) {
        $NeedsLink = $true
    }
}

if ($NeedsLink) {
    Write-Host "  LINK SudokuSolver.exe"
    Write-BuildLog "LINK SudokuSolver.exe"
    $LinkArgs = $Objects + @(
        "-o", $Exe,
        "-LD:/MSYS2/ucrt64/lib",
        "-lmingw32",
        "-lSDL2main",
        "-lSDL2",
        "-lSDL2_ttf",
        "-lopencv_imgcodecs",
        "-lopencv_imgproc",
        "-lopencv_core",
        "-ltesseract",
        "-lcomdlg32"
    )
    & $Gxx @LinkArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed while linking."
        exit $LASTEXITCODE
    }
} elseif ($Compiled -eq 0) {
    Write-Host "  Up to date."
    Write-BuildLog "Up to date"
}

$Elapsed = ((Get-Date) - $Started).TotalSeconds
Write-Host ("Build succeeded: SudokuSolver.exe ({0:N1}s, {1} file(s) compiled)" -f $Elapsed, $Compiled)
Write-BuildLog ("Build succeeded ({0:N1}s, {1} file(s) compiled)" -f $Elapsed, $Compiled)
exit 0
