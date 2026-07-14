# Sudoku Reasoning Radar v1 Unified Experience Design

## Product intent

Sudoku Reasoning Radar v1 is an offline-first Sudoku learning application with equal-status Web and Windows editions. Both editions share the same C++ solver compiled to WebAssembly and the same React interface. Windows adds native OCR through a constrained sidecar; Web retains lightweight image-assisted import.

The release preserves v0.3.0 solver behavior and user data. Accounts, cloud sync, leaderboards, daily challenges, and new solving techniques are outside v1 scope.

## Experience

The interface is a single reasoning workspace. A semantic 9x9 board remains the visual anchor, a lesson panel explains each step as Observe, Eliminate, Conclude, and a bottom timeline controls replay. The visual direction is 70% editorial lesson studio and 30% logic constellation: the graph is hidden by default and expands for deeper explanation, advanced techniques, guesses, contradictions, and backtracking.

Wide screens show board and lesson rail together. Narrow screens keep the board primary and place the lesson and constellation in bottom sheets. Chinese and English are first-class. Color is always paired with labels, shapes, or line styles. Keyboard use, screen readers, high contrast, and reduced motion are required.

## Architecture

The existing Board, Solver, DLXSolver, PuzzleGenerator, HintCoach, and DifficultyAnalyzer become a UI-free C++17 library. It builds natively for tests and through a pinned Emscripten SDK as a single WebAssembly module. The legacy SDL and web_main interfaces remain as compatibility references until parity is proven.

React 19.2, TypeScript, and Vite 8.1 provide the shared renderer. Electron 43.1 hosts the identical renderer on Windows with sandboxing, context isolation, and no Node integration. IndexedDB stores versioned local data on both platforms. Electron preload exposes only OCR, legacy migration, backup import/export, and update-check operations.

## Data and migration

SolveTrace v1 replaces UI dependence on free-form English reasons. Each step includes a technique identifier, action, targets, candidate deltas, an evidence graph, branch metadata, and localization parameters.

On first launch, Windows copies entries from data/puzzles.txt and Web copies sudoku_reasoning_radar_last from localStorage. Imports normalize the 81-character puzzle, deduplicate by puzzle string, and never delete old data. A versioned .srr.json format provides portable backup.

## Failure behavior

Invalid core responses fail schema validation and produce a recoverable UI error. WASM load failure offers retry and diagnostics. OCR timeout, crash, or malformed JSON closes only the OCR session. Migration errors skip individual corrupt records, preserve the source, and return a report. Update-check network errors are silent and do not interrupt offline use.

## Release gates

All v0.3.0 built-in puzzles must preserve result and final solution. Web and Electron bundle the same WASM hash. Migration must be non-destructive and idempotent. Target viewports, both languages, high contrast, and reduced motion must pass visual tests without clipping or click drift. Publish v0.4.0-beta.1 first, then v1.0.0 after real migration and regression validation.
