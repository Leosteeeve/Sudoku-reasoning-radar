# v0.4.0-beta.1 release checklist

## Verified gates

- [ ] Native C++17 core and protocol CTest suites pass.
- [ ] CoreClient, storage, renderer, desktop security/IPC tests and typechecks pass.
- [ ] Pinned Emscripten 6.0.3 builds the production Web renderer.
- [ ] Playwright 1.61.1 E2E covers analyze/solve, trace playback, text and backup import/export, persistence, Web and desktop migration, OCR review, and reload recovery.
- [ ] Visual snapshots pass at exactly 360x640, 768x1024, 1280x800, and 2560x1440 for English, Chinese, high contrast, and reduced motion.
- [ ] The 2000-step browser benchmark reports no main-thread task over 50 ms.
- [ ] OCR helper spawn remains lazy until the OCR command.
- [ ] SHA-256 parity passes for packaged Web and Electron `srr-core.js` and `srr-core.wasm`, copied from one synced renderer build.
- [ ] OCR staging contains an OCR-enabled helper, all resolved OpenCV/Tesseract DLLs, and `tessdata/eng.traineddata`.
- [ ] Migration preserves legacy sources; v0.3.0 downloads and old data remain available for at least one stable release cycle.
- [ ] `git diff --check` is clean and release artifacts contain no credentials or private files.

Record the command output or CI run beside every checked gate. An unchecked item is not verified.

## Environment-dependent pending gates

- [ ] Actual Electron runtime launch on a clean supported Windows host.
- [ ] Signed/unsigned installer smoke test: this beta creates unsigned NSIS and portable artifacts; no signing is configured.
- [ ] Dependency-equipped real OCR recognition using representative PNG/JPEG/WebP images.
- [ ] Playwright screenshot baselines generated and reviewed on the pinned CI Chromium runtime.

Pending gates stay visibly pending until performed on an equipped builder. Configuration, contract tests, or mocks do not count as execution evidence for these items.
