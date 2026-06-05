# Browser OCR Bridge

Version: v0.3.0 Browser Edition

The browser build does not bundle OpenCV.js, Tesseract.js, model data, or wasm OCR workers at startup. That keeps the playable Sudoku app light and avoids surprising network downloads on GitHub Pages.

## Current Browser Path

- The OCR command opens an Image-Assisted Manual Import panel.
- Users can load a PNG, JPG, WebP, or BMP Sudoku image as a visual guide.
- Users can press `Process Image` to run a lightweight no-dependency browser recognizer.
- The recognizer uses canvas image sampling, grid projection, cell thresholding, and template matching. It is intended for clean screenshots and always needs review.
- Users can correct or paste an 81-character puzzle string beside the image.
- `browser_ocr_loader.js` normalizes the string and calls the exported WASM function `SRR_ImportPuzzleString`.
- C++ validates the puzzle with the existing `Board::load(..., true)` rule checks before replacing the board.

## Optional Future Automatic OCR

`browser_ocr_loader.js` checks for optional global OCR libraries:

- `window.cv`
- `window.Tesseract`

If both are present, a future bridge can route processing to stronger OCR libraries. Those libraries are intentionally not included in v0.3.0 because they are large, slower to initialize, and less accurate than the Windows OpenCV + Tesseract workflow without additional tuning.

## Recommended OCR Path

Use the Windows ZIP for automatic OCR. The desktop app keeps the full OpenCV/Tesseract OCR assistant, review board, confidence display, conflict checks, and `data/ocr_debug/` output.
