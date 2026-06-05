# Sudoku Reasoning Radar Browser Edition

Version: v0.3.0 Browser Edition

This folder contains the browser shell for the WebAssembly build. The web app compiles the solver core and a browser-specific SDL2 canvas front end from `web_main.cpp`.

## Features

- 9x9 editable Sudoku board.
- Visual candidates, selected-cell focus, scan-style highlights, candidate-removal marks, and animated solve trace playback.
- v0.3.0 Browser Edition refines highlight readability, placement pulses, candidate-removal strike-through animation, Focus Panel hierarchy, Command Deck spacing, and responsive layout polish.
- Solver modes: Human Logic, Smart Solver, and Turbo Exact Cover.
- Puzzle generation with Easy / Medium / Hard / Expert targets.
- Hint Coach, difficulty analyzer, mistake checking, import/export, and a lightweight browser library.
- Image-Assisted Manual Import for screenshots: open an image, press `Process Image` for lightweight local recognition, review the 81-character puzzle string, then import into the WebAssembly board.
- `SRR_ImportPuzzleString` is exported from WASM so JavaScript can pass validated puzzle strings into the C++ board.
- Command Deck actions match the desktop information architecture: one visible command at a time with previous / execute / next controls instead of a button wall.
- Focus Panel cards show compact status, selected cell candidates, current reasoning or hint text, progress, and the active Command Deck action.
- Settings, Analytics, Library, Import/Export, Shortcuts, Generator, About, and OCR information overlays render as separated cards with wrapped text.
- Full-viewport app shell with no marketing hero, no document scrolling, and a fullscreen button.
- Dynamic canvas resizing keeps CSS size, canvas backing store, SDL window size, renderer viewport, layout, and hit testing in the same logical pixel coordinate system.

## Browser OCR Strategy

Automatic OCR remains strongest in the Windows desktop version. The browser build now includes a lightweight OCR bridge:

- `web/ocr/browser_ocr_loader.js` detects optional OCR libraries if a future page supplies them.
- `web/ocr/browser_ocr_bridge.js` opens the Image-Assisted Manual Import panel and runs a no-dependency canvas/template recognizer for clean screenshots.
- No OpenCV.js, Tesseract.js, model data, or OCR workers are loaded at startup.
- The Windows ZIP remains the recommended path for automatic OCR recognition and confidence review.

See:

```text
web/ocr/README_BROWSER_OCR.md
```

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

- Large OCR libraries are not bundled; the browser uses lightweight local image processing, Image-Assisted Manual Import, and the exported `SRR_ImportPuzzleString` bridge.
- The browser library stores one puzzle in `localStorage`; the Windows build keeps the richer text-file library.
- Text rendering still uses a lightweight bitmap font in the Browser Edition. SDL_ttf font bundling remains a future Web polish item.
- The Browser Edition uses the same solver core and Command Deck model as desktop, but still has a browser-specific SDL canvas renderer.
- The Web UI intentionally keeps Windows release packaging, DLL collection, and full OCR assistant code separate.
