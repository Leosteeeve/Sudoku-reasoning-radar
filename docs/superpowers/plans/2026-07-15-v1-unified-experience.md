# Sudoku Reasoning Radar v1 Unified Experience Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans task-by-task. Every production behavior follows test-driven development.

**Goal:** Deliver a testable v0.4.0-beta foundation with a shared C++/WASM core, shared React renderer, secure Electron host, versioned local data, migration, bilingual teaching UI, and expandable logic constellation.

**Architecture:** A UI-free C++17 core emits SolveTrace v1 through a versioned JSON boundary and is compiled once to WebAssembly. React/Vite consumes a typed CoreClient in both GitHub Pages and Electron. Electron exposes only narrow, validated platform operations.

**Tech stack:** C++17, CMake/CTest/Ninja, pinned Emscripten SDK, Node 24.18, pnpm 11, React 19.2, TypeScript, Vite 8.1, Vitest, Testing Library, Playwright, Electron 43.1.

## Global constraints

- Keep legacy SDL and web_main code operational and do not add features to it.
- Do not use absolute machine paths in source, build files, tests, or documentation examples.
- Web and Electron must consume the exact same WASM artifact.
- Core output is language-neutral; Chinese and English prose is produced only in the renderer.
- Renderer has no arbitrary filesystem, process, or shell API.
- Migration copies and deduplicates data and never deletes or overwrites the legacy source.
- No account, cloud sync, telemetry, leaderboard, daily challenge, or new solving technique.
- Every new production behavior is preceded by a failing automated test.

---

### Task 1: Portable build and core baseline

Create root CMake configuration, presets, the pnpm workspace, bootstrap/build scripts, and a native core test executable. Build the existing solver-domain sources without SDL/OpenCV/Tesseract. Add characterization tests for valid unique, invalid, unsolvable, and multiple-solution puzzles plus deterministic puzzle serialization. Preserve legacy scripts as compatibility commands but remove absolute paths from the normal build path.

**Produces:** `srr_core` CMake target; `srr_core_tests`; `native-debug`, `native-release`, and `wasm-release` presets; unified pnpm commands.

### Task 2: SolveTrace v1 and WASM CoreClient boundary

Add language-neutral trace types and an adapter from existing SolveStep records. Add deterministic JSON serialization and input validation for solve, generate, hint, and analyze. Expose a narrow Emscripten C ABI and a TypeScript CoreClient wrapper. Characterization tests must fail first and then prove schema versioning, technique/action mapping, candidate deltas, evidence edges, branch metadata, invalid requests, and stable output.

**Consumes:** `srr_core`.

**Produces:** SolveTrace v1 JSON schema, `CoreClient`, and one WASM artifact with no SDL dependency.

### Task 3: Shared bilingual reasoning workspace

Scaffold the React/Vite renderer and implement the semantic 9x9 board, reducer-based session state, mode/language controls, undo/redo, lesson card, timeline, deterministic SVG constellation, command panel, and responsive bottom sheets. Use real CoreClient fixtures; do not duplicate solver logic in TypeScript. Add Chinese/English catalogs and accessibility/reduced-motion/high-contrast tests. Implement the confirmed editorial 70% / constellation 30% visual system.

**Consumes:** CoreClient and SolveTrace v1.

**Produces:** reusable renderer that runs in an ordinary browser without Electron globals.

### Task 4: Versioned storage, migration, backup, and Web shell

Add IndexedDB repositories and schemas for puzzles, sessions, and settings. Implement idempotent import from sudoku_reasoning_radar_last and legacy pipe-delimited puzzle text, plus .srr.json validation/import/export. Integrate the Web shell, offline application manifest/cache, lightweight image-assisted import adapter, and GitHub Pages base paths. Test malformed records, deduplication, rollback-safe upgrades, refresh recovery, and offline shell startup.

**Consumes:** shared renderer and versioned core schemas.

**Produces:** platform-neutral storage service and production Web build.

### Task 5: Secure Electron shell and Windows adapters

Add Electron main/preload processes with sandbox, contextIsolation, and nodeIntegration disabled. Expose only ocr.selectAndRecognize, legacy.import, backup.import/export, and update.check. Implement strict IPC schemas, scoped file dialogs, OCR sidecar JSON protocol with timeout/crash handling, legacy data discovery, once-daily GitHub Release checks, and NSIS/portable packaging configuration. Renderer tests must prove it operates when no desktop bridge exists; Electron tests must prove unknown channels and arbitrary paths are unavailable.

**Consumes:** shared renderer, storage service, and native OCR sources.

**Produces:** Windows development build and packaging targets.

### Task 6: Parity, CI, visual regression, and release documentation

Add GitHub Actions for native core tests, WASM/Web build, Electron tests, and Windows packaging. Add browser/electron end-to-end tests for generation, solve, hint, import/export, persistence, migration, and restart recovery. Add visual snapshots for 360x640, 768x1024, 1280x800, and 2560x1440 in Chinese/English, high contrast, and reduced motion. Verify both bundles use the same WASM hash, 2000-step playback avoids main-thread tasks over 50ms, and OCR remains lazy. Update README, migration guide, privacy statement, rollback guide, and v0.4.0-beta.1 release checklist.

**Produces:** release-gated v0.4.0-beta.1 artifacts and evidence.
