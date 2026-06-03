# Sudoku Reasoning Radar Web Preview

Version: v0.3.0 WebAssembly Preview

This folder contains the browser shell for the WebAssembly build. The web app compiles the solver core and a browser-specific SDL2 canvas front end from `web_main.cpp`.

## Features

- 9x9 editable Sudoku board.
- Visual candidates, selected-cell focus, scan-style highlights, candidate-removal marks, and animated solve trace playback.
- Solver modes: Human Logic, Smart Solver, and Turbo Exact Cover.
- Puzzle generation with Easy / Medium / Hard / Expert targets.
- Hint Coach, difficulty analyzer, mistake checking, import/export, and a lightweight browser library.
- Command Deck actions match the desktop information architecture: one visible command at a time with previous / execute / next controls instead of a button wall.
- Focus Panel cards show compact status, selected cell candidates, current reasoning or hint text, progress, and the active Command Deck action.
- Settings, Analytics, Library, Import/Export, Shortcuts, Generator, About, and OCR information overlays render as separated cards with wrapped text.
- Full-viewport app shell with no marketing hero, no document scrolling, and a fullscreen button.
- Dynamic canvas resizing keeps CSS size, canvas backing store, SDL window size, renderer viewport, layout, and hit testing in the same logical pixel coordinate system.

## Windows-Only Feature

OCR Import is currently available in the Windows desktop version. Browser OCR support is planned for a later release.

The web build intentionally does not link OpenCV or Tesseract, so the Windows OCR assistant and release packaging remain separate.

## App Shell Layout

The playable page is published at:

```text
docs/play/
```

It is an app shell, not a landing page. The top bar stays compact, and the SDL canvas fills the remaining viewport. Do not double-click `index.html` directly; browsers commonly block `.wasm` side files over `file://`.

## Requirements

- Emscripten SDK installed locally, expected at `D:\emsdk`.
- Activate the SDK before building:

```bat
D:\emsdk\emsdk_env.bat
```

## Build

From the repository root:

```bat
scripts\build_web.bat
```

If `em++` is not available, the script prints:

```text
Emscripten not found.
Run:
D:\emsdk\emsdk_env.bat
```

## Output

The script writes temporary compiler output to:

```text
web_build\
```

It publishes the GitHub Pages playable build to:

```text
docs\play\
```

Expected publish files:

- `docs/play/index.html`
- `docs/play/web_style.css`
- `docs/play/web_launcher.js`
- `docs/play/SudokuReasoningRadar.js`
- `docs/play/SudokuReasoningRadar.wasm`
- optional `docs/play/SudokuReasoningRadar.data`

## Local Run

```bat
scripts\serve_web.bat
```

Then open:

```text
http://localhost:8000/
```

Do not open the HTML directly from the filesystem, because browsers usually block WebAssembly side files over `file://`.

The canvas resizes automatically on browser resize, fullscreen changes, and orientation changes.

## Clean

```bat
scripts\clean_web.bat
```

## Current Limits

- Browser OCR is disabled in this preview.
- The browser library stores one puzzle in `localStorage`; the Windows build keeps the richer text-file library.
- Text rendering uses a lightweight bitmap font in the Web preview to avoid bundling font assets.
- The Web preview uses the same solver core and Command Deck model as desktop, but still has a browser-specific SDL canvas renderer.
- The Web UI is a preview front end and intentionally keeps Windows release packaging, DLL collection, and OCR untouched.
