# Sudoku Reasoning Radar v0.3.0 Release Candidate

Release target: v0.3.0 Browser Edition

## Highlights

- Promotes the WebAssembly build from preview labeling to the official Browser Edition.
- Keeps the Windows desktop app as the recommended automatic OCR path.
- Adds a browser OCR bridge without loading large OCR libraries at startup.
- Adds Image-Assisted Manual Import for browser screenshots, including a `Process Image` button for lightweight local recognition.
- Exports `SRR_ImportPuzzleString(const char*)` from WASM so JavaScript can import an 81-cell puzzle string into the C++ board.
- Publishes browser OCR helper files to `docs/play/ocr/` for GitHub Pages.
- Keeps VS Code Ctrl+F5 reserved for the Windows desktop app.

## Browser Edition Scope

The browser build includes the solver core, board editing, candidates, generator, hints, difficulty analytics, mistake checking, localStorage library, import/export, fullscreen, and animated reasoning playback.

The OCR command now opens a browser overlay where users can load a Sudoku screenshot, press `Process Image`, review or correct the recognized 81-character puzzle string, and import it. The imported puzzle still goes through the C++ board validation path before it replaces the current board.

## OCR Positioning

Large browser OCR libraries are not bundled in v0.3.0. The bridge includes a lightweight canvas/template recognizer and detects optional `window.cv` and `window.Tesseract` globals for future experiments, but OpenCV.js, Tesseract.js, model data, and workers are intentionally not loaded by default.

For best OCR accuracy, use the Windows ZIP. The desktop app keeps the full OpenCV/Tesseract assistant, OCR review board, low-confidence warnings, conflict checks, and debug image output.

## Build Commands

Windows desktop:

```bat
build.bat
```

Browser Edition:

```bat
D:\emsdk\emsdk_env.bat
scripts\build_web.bat
scripts\serve_web.bat
```

Open:

```text
http://localhost:8000/
```

## Publish Output

Expected browser files:

- `docs/play/index.html`
- `docs/play/web_style.css`
- `docs/play/web_launcher.js`
- `docs/play/SudokuReasoningRadar.js`
- `docs/play/SudokuReasoningRadar.wasm`
- `docs/play/ocr/browser_ocr_loader.js`
- `docs/play/ocr/browser_ocr_bridge.js`
- `docs/play/ocr/README_BROWSER_OCR.md`

## Known Limits

- Browser image processing is lightweight and experimental; review the 81 cells before import.
- Browser image-assisted import still requires manual review of the 81 cells.
- The browser library stores one puzzle in `localStorage`.
- Windows release packaging still depends on DLL collection from the MSYS2 UCRT64 environment.
